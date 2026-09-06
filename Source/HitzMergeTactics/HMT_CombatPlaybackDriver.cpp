#include "HMT_CombatPlaybackDriver.h"
#include "HMT_PlayerState.h"
#include "HMT_PlayerController.h"
#include "HMT_GameState.h"
#include "HMT_GameUnitActor.h"
#include "HMT_UnitHealthBarInterface.h"
#include "HMT_UnitCastBarInterface.h"
#include "Combat/HMT_CombatPlaybackComponent.h"
#include "Combat/HMT_ProjectileActor.h"
#include "Grid/HMT_BoardComponent.h"
#include "Units/HMT_UnitActor.h"
#include "Units/HMT_UnitDefinition.h"
#include "Match/HMT_MatchStateComponent.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

UHMT_CombatPlaybackDriver::UHMT_CombatPlaybackDriver()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Cosmetic-only: never replicated, runs per-machine off replicated data.
	SetIsReplicatedByDefault(false);
}

void UHMT_CombatPlaybackDriver::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Playback = Owner->FindComponentByClass<UHMT_CombatPlaybackComponent>();
		ActiveSource = Playback;
		if (Playback)
		{
			Playback->OnCombatEvent.AddDynamic(this, &UHMT_CombatPlaybackDriver::HandleCombatEvent);
		}
	}
}

void UHMT_CombatPlaybackDriver::HandleCombatEvent(const FHMT_CombatEvent& Event)
{
	// Only the locally controlled player's own component drives visuals on this machine —
	// both paired players carry the same log, so driving from all of them would double-play,
	// and on a listen server the remote player's PlayerState is present but not local.
	APlayerState* OwnerPlayerState = Cast<APlayerState>(GetOwner());
	APlayerController* OwnerController = OwnerPlayerState ? OwnerPlayerState->GetPlayerController() : nullptr;
	if (!OwnerController || !OwnerController->IsLocalPlayerController())
	{
		return;
	}

	BufferedEvents.Add(Event);

	// CombatEnd is guaranteed last in the log — full fight is buffered once it arrives.
	if (Event.EventType == EHMT_CombatEventType::CombatEnd)
	{
		StartPlayback();
	}
}

void UHMT_CombatPlaybackDriver::StartPlayback()
{
	if (bPlaying)
	{
		FinishPlayback();
	}

	// Fresh battle report per combat — the previous round's lines linger through Preparation
	// (a feed widget keeps showing them) until the next fight actually starts.
	FeedLines.Reset();

	// Always our own fight to start — spectating someone else's Combat-phase battle is an
	// explicit, separate action (SwitchPlaybackSource, called directly by the controller the
	// instant its spectate target changes), not something detected here.
	ActiveSource = Playback;
	BeginPlaybackFromSource();
}

void UHMT_CombatPlaybackDriver::SwitchPlaybackSource(UHMT_CombatPlaybackComponent* NewSource)
{
	UHMT_CombatPlaybackComponent* Target = NewSource ? NewSource : Playback.Get();
	if (!Target || Target == ActiveSource)
	{
		return;
	}
	if (!bPlaying)
	{
		// Nothing running right now — the next StartPlayback always begins with our own fight
		// (by design, see above), so there's nothing useful to retarget yet.
		return;
	}

	const bool bTargetLogComplete = Target->GetEvents().ContainsByPredicate(
		[](const FHMT_CombatEvent& E) { return E.EventType == EHMT_CombatEventType::CombatEnd; });
	if (!bTargetLogComplete)
	{
		UE_LOG(LogTemp, Log, TEXT("[HMT Playback] SwitchPlaybackSource: target's combat log hasn't fully replicated yet — ignoring."));
		return;
	}

	TearDownProxiesOnly();
	ActiveSource = Target;
	BufferedEvents = Target->GetEvents();
	FeedLines.Reset();
	BeginPlaybackFromSource();
}

void UHMT_CombatPlaybackDriver::BeginPlaybackFromSource()
{
	// The resolver emits (and this buffers) events in per-unit processing order within a tick, not
	// strictly by Timestamp — a ranged attack's DamageDealt now carries a Timestamp offset forward
	// by ProjectileTravelTime (see HMT_CombatResolver.cpp), which can land after another unit's
	// same-tick event that was appended later in the array. The consumer below is a strict linear
	// scan assuming non-decreasing Timestamp; a stable sort restores that invariant regardless of
	// emission order, and preserves relative order for any genuine timestamp ties.
	BufferedEvents.StableSort([](const FHMT_CombatEvent& A, const FHMT_CombatEvent& B) { return A.Timestamp < B.Timestamp; });

	const FHMT_BoardSnapshot& SideA = ActiveSource->GetSideASnapshot();
	const FHMT_BoardSnapshot& SideB = ActiveSource->GetSideBSnapshot();
	ArenaColumns = HMT_CombatArena::GetArenaColumns(SideA, SideB);
	ArenaRows = HMT_CombatArena::GetArenaRows(SideA, SideB);

	// Spawn proxies at each unit's arena-space start cell (Side B mirrored, matching the resolver).
	// "Enemy" is relative to the VIEWED player's side (the spectated player's while spectating).
	const bool bLocalIsSideA = ActiveSource->IsOwnerSideA();
	for (const FHMT_SnapshotUnit& Unit : SideA.Units)
	{
		SpawnProxy(Unit, Unit.Coord, /*bIsEnemy=*/ !bLocalIsSideA);
	}
	for (const FHMT_SnapshotUnit& Unit : SideB.Units)
	{
		SpawnProxy(Unit, HMT_CombatArena::MirrorAcrossArena(Unit.Coord, ArenaColumns, ArenaRows), /*bIsEnemy=*/ bLocalIsSideA);
	}

	SetRealUnitMeshesVisible(false);

	// Seeded from the shared server clock, not 0 — see GetSyncedCombatElapsedSeconds. The
	// existing catch-up loop in TickComponent (processes every buffered event whose Timestamp is
	// already <= PlaybackTime before the next frame renders) means a late joiner correctly lands
	// on the fight's CURRENT state instead of replaying history from the start.
	PlaybackTime = GetSyncedCombatElapsedSeconds();
	CatchUpCutoffTime = PlaybackTime;
	NextEventIndex = 0;
	bPlaying = true;

	UE_LOG(LogTemp, Log, TEXT("[HMT Playback] Playing: %d events, %d proxies (%dx%d arena), starting at t=%.2f."),
		BufferedEvents.Num(), Proxies.Num(), ArenaColumns, ArenaRows, PlaybackTime);
}

float UHMT_CombatPlaybackDriver::GetSyncedCombatElapsedSeconds() const
{
	return ActiveSource ? ActiveSource->GetElapsedSinceCombatStart() : 0.f;
}

void UHMT_CombatPlaybackDriver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPlaying)
	{
		return;
	}

	PlaybackTime += DeltaTime;

	// World-space UWidgetComponents don't auto-face the camera the way a particle sprite would —
	// without this they render edge-on from most viewing angles, which reads as "not visible" even
	// though the widget is present and correctly positioned above the proxy's head.
	if (const APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (const APlayerCameraManager* CameraManager = LocalController->PlayerCameraManager)
		{
			const FVector CameraLocation = CameraManager->GetCameraLocation();
			auto FaceCamera = [&CameraLocation](UWidgetComponent* Widget)
			{
				if (Widget)
				{
					Widget->SetWorldRotation((CameraLocation - Widget->GetComponentLocation()).Rotation());
				}
			};
			for (const TPair<FGuid, TObjectPtr<UWidgetComponent>>& Pair : HealthBars)
			{
				FaceCamera(Pair.Value);
			}
			for (const TPair<FGuid, TObjectPtr<UWidgetComponent>>& Pair : CastBars)
			{
				FaceCamera(Pair.Value);
			}
		}
	}

	while (NextEventIndex < BufferedEvents.Num() && BufferedEvents[NextEventIndex].Timestamp <= PlaybackTime)
	{
		ProcessCombatEvent(BufferedEvents[NextEventIndex]);
		++NextEventIndex;
	}

	for (int32 Index = ActiveMoves.Num() - 1; Index >= 0; --Index)
	{
		FActiveMove& Move = ActiveMoves[Index];
		if (!Move.Proxy.IsValid())
		{
			ActiveMoves.RemoveAt(Index);
			continue;
		}

		const float Alpha = Move.Duration > 0.f ? FMath::Clamp((PlaybackTime - Move.StartTime) / Move.Duration, 0.f, 1.f) : 1.f;
		Move.Proxy->SetActorLocation(FMath::Lerp(Move.From, Move.To, Alpha));
		if (Alpha >= 1.f)
		{
			// Walk finished and nothing else queued an animation — settle back to idle.
			if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Move.InstanceId); Anims && !ReturnToIdleAt.Contains(Move.InstanceId) && !HideProxyAt.Contains(Move.InstanceId))
			{
				PlayProxyAnim(Move.InstanceId, Anims->Idle, true, false);
			}
			ActiveMoves.RemoveAt(Index);
		}
	}

	// Push a smooth 0..1 fill to every active cast bar — the bar itself is shown/hidden by the
	// CastStart/AbilityCast/CastInterrupted event handlers, not by this loop reaching 1.0.
	for (const FActiveCast& Cast : ActiveCasts)
	{
		if (TObjectPtr<UWidgetComponent>* CastBarComponent = CastBars.Find(Cast.InstanceId); CastBarComponent && *CastBarComponent)
		{
			if (UUserWidget* CastBarWidget = (*CastBarComponent)->GetUserWidgetObject(); CastBarWidget && CastBarWidget->GetClass()->ImplementsInterface(UHMT_UnitCastBarInterface::StaticClass()))
			{
				const float Alpha = FMath::Clamp((PlaybackTime - Cast.StartTime) / Cast.Duration, 0.f, 1.f);
				IHMT_UnitCastBarInterface::Execute_SetCastProgress(CastBarWidget, Alpha);
			}
		}
	}

	// One-shot montages (attack/cast/stagger) hand back to the idle loop when they finish.
	for (auto It = ReturnToIdleAt.CreateIterator(); It; ++It)
	{
		if (It->Value <= PlaybackTime)
		{
			if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(It->Key))
			{
				PlayProxyAnim(It->Key, Anims->Idle, true, false);
			}
			It.RemoveCurrent();
		}
	}

	// Death montages that have played out — hide the corpse now.
	for (auto It = HideProxyAt.CreateIterator(); It; ++It)
	{
		if (It->Value <= PlaybackTime)
		{
			if (TObjectPtr<ASkeletalMeshActor>* Proxy = Proxies.Find(It->Key); Proxy && *Proxy)
			{
				(*Proxy)->SetActorHiddenInGame(true);
			}
			It.RemoveCurrent();
		}
	}

	// No WBP health bar assigned — draw simple debug bars so combat is still readable out of the
	// box (red backing line + green fill). One-frame lines, redrawn every tick while playing.
	if (bPlaying && !HealthBarWidgetClass)
	{
		for (const TPair<FGuid, TObjectPtr<ASkeletalMeshActor>>& Pair : Proxies)
		{
			const ASkeletalMeshActor* Proxy = Pair.Value;
			const float* CurrentHealth = CurrentHealthByInstance.Find(Pair.Key);
			const float* MaxHealth = MaxHealthByInstance.Find(Pair.Key);
			if (!Proxy || Proxy->IsHidden() || !CurrentHealth || !MaxHealth || *MaxHealth <= 0.f)
			{
				continue;
			}

			const bool bIsEnemy = bIsEnemyByInstance.FindRef(Pair.Key);
			const FVector Anchor = BarAnchorByInstance.Contains(Pair.Key)
				? BarAnchorByInstance[Pair.Key] + FVector(0.f, 0.f, 20.f)
				: FVector(0.f, 0.f, 200.f);
			const FVector Left = Proxy->GetActorLocation() + Anchor + FVector(-50.f, 0.f, 0.f);
			DrawDebugLine(GetWorld(), Left, Left + FVector(100.f, 0.f, 0.f), FColor(30, 30, 30), false, -1.f, 0, 8.f);
			DrawDebugLine(GetWorld(), Left, Left + FVector(100.f * (*CurrentHealth / *MaxHealth), 0.f, 0.f),
				bIsEnemy ? FColor(209, 23, 23) : FColor(255, 106, 0), false, -1.f, 0, 8.f);
		}
	}

	if (NextEventIndex >= BufferedEvents.Num() && ActiveMoves.Num() == 0)
	{
		// Victory lap: instead of tearing playback down the instant the log ends, survivors linger
		// on their tiles looping Victory (or Idle when unauthored) for VictoryDisplayDuration, THEN
		// the normal restore happens. Zero duration = old instant-teardown behavior.
		if (!bVictoryLapStarted && VictoryDisplayDuration > 0.f)
		{
			bVictoryLapStarted = true;
			VictoryLapEndAt = PlaybackTime + VictoryDisplayDuration;
			for (const TPair<FGuid, TObjectPtr<ASkeletalMeshActor>>& Pair : Proxies)
			{
				if (!Pair.Value || Pair.Value->IsHidden())
				{
					continue;   // dead units stay down — only survivors celebrate
				}
				if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Pair.Key))
				{
					PlayProxyAnim(Pair.Key, Anims->Victory ? Anims->Victory.Get() : Anims->Idle.Get(), true, false);
				}
			}
		}

		// Don't restore placement visuals just because local playback+the victory lap finished —
		// the server-authoritative round isn't over until the phase has actually moved past Combat
		// (Results still needs to tally HP/income). A short fight's replicated log can easily finish
		// playing well before the wall-clock CombatPhaseDuration server-side timer does, which used
		// to snap visuals back to placement while the match was still nominally mid-combat. RoundEnd
		// itself is unreliable to poll for — UHMT_MatchStateComponent::BeginRoundEndPhase sets it and
		// synchronously cascades into the next round's Preparation in the same call, so a client can
		// easily never observe it; "no longer Combat" is the robust equivalent.
		const bool bMinimumHoldElapsed = !bVictoryLapStarted || PlaybackTime >= VictoryLapEndAt;
		if (bMinimumHoldElapsed && IsRoundPastCombat())
		{
			FinishPlayback();
		}
	}
}

bool UHMT_CombatPlaybackDriver::IsRoundPastCombat() const
{
	const UWorld* World = GetWorld();
	const AHMT_GameState* SampleGameState = World ? World->GetGameState<AHMT_GameState>() : nullptr;
	return !SampleGameState || !SampleGameState->MatchStateComponent
		|| SampleGameState->MatchStateComponent->GetCurrentPhase() != EHMT_MatchPhase::Combat;
}

void UHMT_CombatPlaybackDriver::ProcessCombatEvent(const FHMT_CombatEvent& Event)
{
	AppendFeedLine(Event);

	switch (Event.EventType)
	{
	case EHMT_CombatEventType::UnitMoved:
	{
		if (TObjectPtr<ASkeletalMeshActor>* Proxy = Proxies.Find(Event.SourceInstanceId))
		{
			// One lerp per proxy at a time: with the sim's BusyUntilTime gating, back-to-back move
			// events shouldn't overlap anymore — but if playback processes several buffered events
			// in one frame (hitch, late join), stacked lerps on the same proxy fight each other and
			// snap. Kill the old one and glide from wherever the proxy ACTUALLY is right now.
			ActiveMoves.RemoveAll([&Event](const FActiveMove& Existing) { return Existing.InstanceId == Event.SourceInstanceId; });

			const float Duration = FMath::Max(Event.MoveDuration, 0.05f);
			const FVector Destination = ArenaCoordToWorld(Event.ToCoord);

			// PlaybackTime can jump forward (SwitchPlaybackSource seeds it from
			// GetSyncedCombatElapsedSeconds instead of 0, and freshly spawned proxies always start
			// at their arena-start position) — a whole backlog of past events gets processed in one
			// burst right after that. A move whose window has already fully elapsed by PlaybackTime
			// must SNAP straight to its destination, not animate a fresh walk from the proxy's
			// current (spawn) position — that fresh-walk-from-spawn is exactly what read as
			// "movement restarts from starting position" every time playback resynced.
			const FVector OriginBeforeThisMove = (*Proxy)->GetActorLocation();
			if (Event.Timestamp + Duration <= PlaybackTime)
			{
				(*Proxy)->SetActorLocation(Destination);
			}
			else
			{
				FActiveMove Move;
				Move.Proxy = *Proxy;
				Move.InstanceId = Event.SourceInstanceId;
				Move.From = OriginBeforeThisMove;
				Move.To = Destination;
				// Event.Timestamp, not PlaybackTime: if this move is caught up WHILE still nominally
				// in progress (rare, but possible right at a resync boundary), Alpha below reflects
				// its true fractional progress instead of restarting the lerp at 0%.
				Move.StartTime = Event.Timestamp;
				Move.Duration = Duration;
				ActiveMoves.Add(Move);
			}

			// Walks face where they're going; knockbacks face where they came FROM (you're shoved
			// backward, not sprinting away) and read as a stagger instead of a move loop.
			FaceProxy(Event.SourceInstanceId, Event.bIsForcedDisplacement ? OriginBeforeThisMove - Destination : Destination - OriginBeforeThisMove);
			if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Event.SourceInstanceId))
			{
				if (Event.bIsForcedDisplacement)
				{
					PlayProxyAnim(Event.SourceInstanceId, Anims->Stagger, false, true);
				}
				else
				{
					PlayProxyAnim(Event.SourceInstanceId, Anims->Move, true, false);
				}
			}
		}
		break;
	}
	case EHMT_CombatEventType::AttackStart:
	{
		FaceProxyAtProxy(Event.SourceInstanceId, Event.TargetInstanceId);
		if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Event.SourceInstanceId))
		{
			// Stretch/squash the montage to fill the actual attack-cycle window (1/AttackSpeed)
			// instead of always playing at its authored length — otherwise a high-AttackSpeed unit
			// fires AttackStart events faster than its own swing animation can finish, and a
			// low-AttackSpeed one leaves the swing frozen on its last frame between hits. Clamped so
			// an extreme stat value can't spin the animation into a stroboscopic blur or a near-freeze.
			float PlayRate = 1.f;
			if (const float* AttackSpeed = AttackSpeedByInstance.Find(Event.SourceInstanceId); AttackSpeed && *AttackSpeed > 0.f && Anims->Attack)
			{
				PlayRate = FMath::Clamp(Anims->Attack->GetPlayLength() * *AttackSpeed, 0.25f, 4.f);
			}
			PlayProxyAnim(Event.SourceInstanceId, Anims->Attack, false, true, PlayRate);
		}
		if (const FHMT_UnitAudioSet* Sounds = AudioSets.Find(Event.SourceInstanceId); Sounds && !Sounds->Attack.IsNull())
		{
			PlayProxySound(Event.SourceInstanceId, Sounds->Attack.LoadSynchronous());
		}
		// Ranged attacks get either a real projectile actor (if the attacker's UnitDefinition
		// configures one) or a brief tracer line fallback, so distance attacks read as attacks at all.
		TObjectPtr<ASkeletalMeshActor>* Source = Proxies.Find(Event.SourceInstanceId);
		TObjectPtr<ASkeletalMeshActor>* Target = Proxies.Find(Event.TargetInstanceId);
		if (Source && *Source && Target && *Target)
		{
			const float CellSize = ActiveSource ? ActiveSource->GetSideASnapshot().CellSize : 100.f;
			auto ResolveSocketOrRootLocation = [](ASkeletalMeshActor* ProxyActor, FName SocketName) -> FVector
			{
				if (!SocketName.IsNone() && ProxyActor->GetSkeletalMeshComponent()->DoesSocketExist(SocketName))
				{
					return ProxyActor->GetSkeletalMeshComponent()->GetSocketLocation(SocketName);
				}
				return ProxyActor->GetActorLocation() + FVector(0, 0, 80.f);
			};

			const FHMT_UnitProjectileSet* ProjectileSet = ProjectileSets.Find(Event.SourceInstanceId);
			if (FVector::Dist2D((*Source)->GetActorLocation(), (*Target)->GetActorLocation()) > CellSize * 1.5f)
			{
				if (ProjectileSet && ProjectileSet->ProjectileClass)
				{
					// Spawn-and-launch is native orchestration (same tier as spawning a placement
					// unit) — every cosmetic decision (mesh/particles/sound/travel curve/impact FX)
					// lives in the buyer's AHMT_ProjectileActor Blueprint subclass, not here.
					const FVector From = ResolveSocketOrRootLocation(*Source, ProjectileSet->MuzzleSocketName);
					const FVector To = ResolveSocketOrRootLocation(*Target, ProjectileSet->TargetSocketName);
					if (AHMT_ProjectileActor* Projectile = GetWorld()->SpawnActor<AHMT_ProjectileActor>(ProjectileSet->ProjectileClass, FTransform(From)))
					{
						Projectile->Launch(From, To, Event.ProjectileTravelTime);
					}
				}
				else
				{
					const FVector From = (*Source)->GetActorLocation() + FVector(0, 0, 80.f);
					const FVector To = (*Target)->GetActorLocation() + FVector(0, 0, 80.f);
					// Line lifetime matches the resolver's ProjectileTravelTime hint, so the tracer's
					// visible duration roughly agrees with when the hit is supposed to land.
					DrawDebugLine(GetWorld(), From, To, FColor::Yellow, false, FMath::Max(Event.ProjectileTravelTime, 0.15f), 0, 3.f);
				}
			}
		}
		break;
	}
	case EHMT_CombatEventType::DamageDealt:
	{
		// Negative magnitude = heal (see UHMT_CombatContext::Heal) — green +N instead of red -N.
		// Crits read bigger and yellow so the new crit stats are visible without any UI work.
		const bool bHeal = Event.Magnitude < 0.f;
		const bool bCrit = Event.bIsCritical && !bHeal;
		if (TObjectPtr<ASkeletalMeshActor>* Target = Proxies.Find(Event.TargetInstanceId); Target && *Target)
		{
			DrawDebugString(GetWorld(), (*Target)->GetActorLocation() + FVector(0, 0, 120.f),
				FString::Printf(TEXT("%s%s%.0f"), bCrit ? TEXT("CRIT ") : TEXT(""), bHeal ? TEXT("+") : TEXT("-"), FMath::Abs(Event.Magnitude)),
				nullptr, bHeal ? FColor::Green : (bCrit ? FColor::Yellow : FColor::Red), 1.0f, true, bCrit ? 1.6f : 1.f);
		}
		ApplyDamageToHealthBar(Event.TargetInstanceId, Event.Magnitude);
		UE_LOG(LogTemp, Log, TEXT("[HMT Playback] t=%.1f DamageDealt %.0f"), Event.Timestamp, Event.Magnitude);
		break;
	}
	case EHMT_CombatEventType::CastStart:
		{
			FaceProxyAtProxy(Event.SourceInstanceId, Event.TargetInstanceId);
			if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Event.SourceInstanceId))
			{
				PlayProxyAnim(Event.SourceInstanceId, Anims->Ability ? Anims->Ability.Get() : Anims->Attack.Get(), true, false);
			}
			if (TObjectPtr<ASkeletalMeshActor>* Source = Proxies.Find(Event.SourceInstanceId); Source && *Source)
			{
				DrawDebugString(GetWorld(), (*Source)->GetActorLocation() + FVector(0, 0, 140.f),
					TEXT("CASTING"), nullptr, FColor::Cyan, FMath::Max(Event.CastDuration, 0.1f), true);
			}
			ShowCastBar(Event.SourceInstanceId, Event.CastDuration);
			break;
		}
		case EHMT_CombatEventType::CastInterrupted:
		{
			HideCastBar(Event.SourceInstanceId);
			ReturnToIdleAt.Remove(Event.SourceInstanceId);
			if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Event.SourceInstanceId))
			{
				PlayProxyAnim(Event.SourceInstanceId, Anims->Idle, true, false);
			}
			if (TObjectPtr<ASkeletalMeshActor>* Source = Proxies.Find(Event.SourceInstanceId); Source && *Source)
			{
				DrawDebugString(GetWorld(), (*Source)->GetActorLocation() + FVector(0, 0, 140.f),
					TEXT("INTERRUPTED"), nullptr, FColor::Red, 1.0f, true);
			}
			break;
		}
		case EHMT_CombatEventType::AbilityCast:
	{
		// A channeled cast's bar is still showing when this fires (the channel's own completion);
		// an instant cast never called ShowCastBar, so this is a harmless no-op for it.
		HideCastBar(Event.SourceInstanceId);
		FaceProxyAtProxy(Event.SourceInstanceId, Event.TargetInstanceId);
		if (const FHMT_UnitAnimSet* Anims = AnimSets.Find(Event.SourceInstanceId))
		{
			// Fall back to the attack montage so a cast still reads without dedicated content.
			PlayProxyAnim(Event.SourceInstanceId, Anims->Ability ? Anims->Ability.Get() : Anims->Attack.Get(), false, true);
		}
		if (const FHMT_UnitAudioSet* Sounds = AudioSets.Find(Event.SourceInstanceId); Sounds && !Sounds->Ability.IsNull())
		{
			PlayProxySound(Event.SourceInstanceId, Sounds->Ability.LoadSynchronous());
		}
		if (TObjectPtr<ASkeletalMeshActor>* Source = Proxies.Find(Event.SourceInstanceId); Source && *Source)
		{
			DrawDebugString(GetWorld(), (*Source)->GetActorLocation() + FVector(0, 0, 140.f),
				TEXT("CAST"), nullptr, FColor::Cyan, 1.0f, true);
		}
		break;
	}
	case EHMT_CombatEventType::UnitDeath:
	{
		// Target = the unit that died, Source = its killer (may be empty/unattributed) — see the
		// resolver/context comments on this convention flip; every other event type already used
		// Source=actor/Target=recipient, death used to be the one exception.
		//
		// A death that already happened before this viewing session started (late join, or a
		// spectate switch onto a fight that's mid/post-combat) is caught up in the same instant
		// burst as every other historical event — playing the montage AND its sound here would
		// make an already-dead unit visibly pop back up for a frame and die all over again every
		// single time the viewer (re)spectates. Snap straight to hidden instead; only a genuinely
		// live death (happening at/after the moment this playback started) gets the real sequence.
		const bool bIsCatchUp = Event.Timestamp <= CatchUpCutoffTime;

		if (!bIsCatchUp)
		{
			if (const FHMT_UnitAudioSet* Sounds = AudioSets.Find(Event.TargetInstanceId); Sounds && !Sounds->Death.IsNull())
			{
				PlayProxySound(Event.TargetInstanceId, Sounds->Death.LoadSynchronous());
			}
		}
		const FHMT_UnitAnimSet* Anims = AnimSets.Find(Event.TargetInstanceId);
		UAnimMontage* DeathAnim = Anims ? Anims->Death.Get() : nullptr;
		if (DeathAnim && !bIsCatchUp)
		{
			// Let the death montage play out, then hide — instant hide only when unauthored (or
			// caught up, see above).
			PlayProxyAnim(Event.TargetInstanceId, DeathAnim, false, false);
			ReturnToIdleAt.Remove(Event.TargetInstanceId);
			HideProxyAt.Add(Event.TargetInstanceId, PlaybackTime + DeathAnim->GetPlayLength());
		}
		else if (TObjectPtr<ASkeletalMeshActor>* Proxy = Proxies.Find(Event.TargetInstanceId); Proxy && *Proxy)
		{
			(*Proxy)->SetActorHiddenInGame(true);
		}
		if (TObjectPtr<UWidgetComponent>* HealthBarComponent = HealthBars.Find(Event.TargetInstanceId); HealthBarComponent && *HealthBarComponent)
		{
			(*HealthBarComponent)->SetVisibility(false);
		}
		// A dying unit shouldn't keep sliding to a cell it never reached.
		ActiveMoves.RemoveAll([&Event](const FActiveMove& Move) { return Move.InstanceId == Event.TargetInstanceId; });
		UE_LOG(LogTemp, Log, TEXT("[HMT Playback] t=%.1f UnitDeath (killer=%s)"), Event.Timestamp, *Event.SourceInstanceId.ToString());
		break;
	}
	case EHMT_CombatEventType::StatusApplied:
	{
		if (TObjectPtr<ASkeletalMeshActor>* Target = Proxies.Find(Event.TargetInstanceId); Target && *Target)
		{
			DrawDebugString(GetWorld(), (*Target)->GetActorLocation() + FVector(0, 0, 160.f),
				Event.EffectTag.ToString(), nullptr, FColor::Magenta, 1.5f, true);
		}
		break;
	}
	case EHMT_CombatEventType::StatusExpired:
		// Purely informational for now — no persistent status icon widget exists yet (see UI-phase
		// gap notes); the StatusApplied text is a one-shot callout, nothing needs cleaning up here.
		break;
	case EHMT_CombatEventType::UnitSpawned:
	{
		// Summons don't come from a board-placement snapshot, so they need a synthetic FHMT_SnapshotUnit
		// to reuse the normal SpawnProxy path — same mesh/animset/health-bar setup as every other proxy.
		FHMT_SnapshotUnit SyntheticUnit;
		SyntheticUnit.UnitDefinition = Event.SpawnedUnitDefinition;
		SyntheticUnit.StarLevel = Event.SpawnedStarLevel;
		SyntheticUnit.InstanceId = Event.TargetInstanceId;
		SyntheticUnit.Coord = Event.ToCoord;

		const bool bSummonerIsEnemy = bIsEnemyByInstance.FindRef(Event.SourceInstanceId);
		SpawnProxy(SyntheticUnit, Event.ToCoord, bSummonerIsEnemy);
		break;
	}
	case EHMT_CombatEventType::CombatEnd:
		UE_LOG(LogTemp, Log, TEXT("[HMT Playback] t=%.1f CombatEnd winner=%s"), Event.Timestamp,
			Event.Magnitude == 0.f ? TEXT("SideA") : Event.Magnitude == 1.f ? TEXT("SideB") : TEXT("draw"));
		break;
	default:
		break;
	}
}

void UHMT_CombatPlaybackDriver::TearDownProxiesOnly()
{
	for (TPair<FGuid, TObjectPtr<ASkeletalMeshActor>>& Pair : Proxies)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	Proxies.Empty();
	// Widget components are attached children of their proxy actor — destroyed along with it above,
	// no separate cleanup needed; just drop the now-dangling references and the health tracking.
	HealthBars.Empty();
	CastBars.Empty();
	ActiveCasts.Empty();
	CurrentHealthByInstance.Empty();
	MaxHealthByInstance.Empty();
	AttackSpeedByInstance.Empty();
	BarAnchorByInstance.Empty();
	bIsEnemyByInstance.Empty();
	AnimSets.Empty();
	AudioSets.Empty();
	ProjectileSets.Empty();
	ReturnToIdleAt.Empty();
	HideProxyAt.Empty();
	ActiveMoves.Empty();
	BufferedEvents.Empty();
	NextEventIndex = 0;
}

void UHMT_CombatPlaybackDriver::FinishPlayback()
{
	TearDownProxiesOnly();
	// Next playback defaults back to our own fight unless StartPlayback/SwitchPlaybackSource
	// override again.
	ActiveSource = Playback;
	bPlaying = false;
	bVictoryLapStarted = false;
	VictoryLapEndAt = 0.f;

	SetRealUnitMeshesVisible(true);
}

FVector UHMT_CombatPlaybackDriver::ArenaCoordToWorld(const FHMT_GridCoord& ArenaCoord) const
{
	const AHMT_PlayerState* OwnerPlayerState = Cast<AHMT_PlayerState>(GetOwner());
	if (!OwnerPlayerState)
	{
		return FVector::ZeroVector;
	}

	// Side B's owner sees the arena mirrored so their own army stays on their board half.
	// ActiveSource, not Playback: while spectating, the mirror question is about the VIEWED
	// player's side of THEIR fight.
	const FHMT_GridCoord DisplayCoord = (ActiveSource ? ActiveSource.Get() : Playback.Get())->IsOwnerSideA()
		? ArenaCoord
		: HMT_CombatArena::MirrorAcrossArena(ArenaCoord, ArenaColumns, ArenaRows);

	// GridToWorld is pure topology math — coords beyond the owner's board rows extend past its
	// edge, which is where the opposing army fights from. +Z lift matches the debug tiles.
	return OwnerPlayerState->GetBoardWorldOrigin()
		+ OwnerPlayerState->BoardComponent->GridToWorld(DisplayCoord)
		+ FVector(0.f, 0.f, 5.f);
}

ASkeletalMeshActor* UHMT_CombatPlaybackDriver::SpawnProxy(const FHMT_SnapshotUnit& Unit, const FHMT_GridCoord& ArenaCoord, bool bIsEnemy)
{
	UWorld* World = GetWorld();
	if (!World || !Unit.UnitDefinition)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASkeletalMeshActor* Proxy = World->SpawnActor<ASkeletalMeshActor>(SpawnParams);
	if (!Proxy)
	{
		return nullptr;
	}

	// Local-only cosmetic actor: must never replicate (a listen server would otherwise push its
	// proxies to clients on top of their own).
	Proxy->SetReplicates(false);
	Proxy->GetSkeletalMeshComponent()->SetMobility(EComponentMobility::Movable);
	if (!Unit.UnitDefinition->Mesh.IsNull())
	{
		Proxy->GetSkeletalMeshComponent()->SetSkeletalMesh(Unit.UnitDefinition->Mesh.LoadSynchronous());
	}
	Proxy->SetActorLocation(ArenaCoordToWorld(ArenaCoord));

	// Overhead-widget anchor: authored DA offset when set, otherwise just above THIS mesh's
	// measured bounds — a fixed height (the old behavior) buried the bar inside taller meshes
	// and floated it over shorter ones, so it only read correctly on one specific character.
	float MeshBoundsTopZ = 155.f; // sensible humanoid default when no mesh is assigned
	if (const USkeletalMesh* ProxyMesh = Proxy->GetSkeletalMeshComponent()->GetSkeletalMeshAsset())
	{
		const FBoxSphereBounds MeshBounds = ProxyMesh->GetBounds();
		MeshBoundsTopZ = MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z;
	}
	const FVector BarAnchor = Unit.UnitDefinition->GetHealthBarAnchor(MeshBoundsTopZ, Unit.StarLevel);
	BarAnchorByInstance.Add(Unit.InstanceId, BarAnchor);

	Proxies.Add(Unit.InstanceId, Proxy);
	AnimSets.Add(Unit.InstanceId, Unit.UnitDefinition->AnimSet);
	PlayProxyAnim(Unit.InstanceId, Unit.UnitDefinition->AnimSet.Idle, true, false);
	AudioSets.Add(Unit.InstanceId, Unit.UnitDefinition->AudioSet);
	if (!Unit.UnitDefinition->AudioSet.Spawn.IsNull())
	{
		PlayProxySound(Unit.InstanceId, Unit.UnitDefinition->AudioSet.Spawn.LoadSynchronous());
	}
	ProjectileSets.Add(Unit.InstanceId, Unit.UnitDefinition->ProjectileSet);

	// Armies square off at spawn: in display space the local player's units occupy the near rows
	// and fight toward +Y, the opponent's toward -Y.
	FaceProxy(Unit.InstanceId, bIsEnemy ? FVector(0.f, -1.f, 0.f) : FVector(0.f, 1.f, 0.f));

	// Max health comes from the same scaled-stats path UHMT_UnitInstanceComponent uses at
	// placement (GetScaledStats), read against whichever tag MatchRules designates as Health —
	// the resolver copies the same tag onto itself in StartMatch, this just reads the source.
	float MaxHealth = 0.f;
	const AHMT_GameState* SampleGameState = World->GetGameState<AHMT_GameState>();
	const UHMT_MatchRulesAsset* MatchRules = SampleGameState && SampleGameState->MatchStateComponent
		? SampleGameState->MatchStateComponent->GetMatchRules() : nullptr;
	float AttackSpeed = 0.f;
	if (MatchRules)
	{
		for (const FHMT_BaseStat& Stat : Unit.UnitDefinition->GetScaledStats(Unit.StarLevel))
		{
			if (MatchRules->HealthStatTag.IsValid() && Stat.StatTag == MatchRules->HealthStatTag)
			{
				MaxHealth = Stat.Value;
			}
			else if (MatchRules->AttackSpeedStatTag.IsValid() && Stat.StatTag == MatchRules->AttackSpeedStatTag)
			{
				AttackSpeed = Stat.Value;
			}
		}
	}
	MaxHealthByInstance.Add(Unit.InstanceId, MaxHealth);
	CurrentHealthByInstance.Add(Unit.InstanceId, MaxHealth);
	AttackSpeedByInstance.Add(Unit.InstanceId, AttackSpeed);
	bIsEnemyByInstance.Add(Unit.InstanceId, bIsEnemy);

	if (HealthBarWidgetClass && MaxHealth > 0.f)
	{
		UWidgetComponent* HealthBarComponent = NewObject<UWidgetComponent>(Proxy);
		HealthBarComponent->SetWidgetSpace(EWidgetSpace::World);
		HealthBarComponent->SetDrawSize(FVector2D(100.f, 12.f));
		HealthBarComponent->SetWidgetClass(HealthBarWidgetClass);
		HealthBarComponent->SetupAttachment(Proxy->GetRootComponent());
		HealthBarComponent->SetRelativeLocation(BarAnchor);
		HealthBarComponent->RegisterComponent();

		if (UUserWidget* HealthBarWidget = HealthBarComponent->GetUserWidgetObject(); HealthBarWidget && HealthBarWidget->GetClass()->ImplementsInterface(UHMT_UnitHealthBarInterface::StaticClass()))
		{
			IHMT_UnitHealthBarInterface::Execute_SetHealthFraction(HealthBarWidget, MaxHealth, MaxHealth, bIsEnemy);
		}
		HealthBars.Add(Unit.InstanceId, HealthBarComponent);
	}

	if (CastBarWidgetClass)
	{
		UWidgetComponent* CastBarComponent = NewObject<UWidgetComponent>(Proxy);
		CastBarComponent->SetWidgetSpace(EWidgetSpace::World);
		CastBarComponent->SetDrawSize(FVector2D(100.f, 10.f));
		CastBarComponent->SetWidgetClass(CastBarWidgetClass);
		CastBarComponent->SetupAttachment(Proxy->GetRootComponent());
		// Sits just below the health bar so both can be visible together mid-cast.
		CastBarComponent->SetRelativeLocation(BarAnchor - FVector(0.f, 0.f, 20.f));
		CastBarComponent->RegisterComponent();
		// Hidden until a CastStart event actually needs it — most attacks in a fight never cast.
		CastBarComponent->SetVisibility(false);
		CastBars.Add(Unit.InstanceId, CastBarComponent);
	}

	return Proxy;
}

void UHMT_CombatPlaybackDriver::FaceProxy(const FGuid& InstanceId, const FVector& WorldDirection)
{
	TObjectPtr<ASkeletalMeshActor>* Proxy = Proxies.Find(InstanceId);
	FVector Flat = WorldDirection;
	Flat.Z = 0.f;
	if (!Proxy || !*Proxy || Flat.IsNearlyZero())
	{
		return;
	}

	(*Proxy)->SetActorRotation(FRotator(0.f, Flat.Rotation().Yaw + ProxyMeshYawOffset, 0.f));
}

void UHMT_CombatPlaybackDriver::FaceProxyAtProxy(const FGuid& SourceId, const FGuid& TargetId)
{
	TObjectPtr<ASkeletalMeshActor>* Source = Proxies.Find(SourceId);
	TObjectPtr<ASkeletalMeshActor>* Target = Proxies.Find(TargetId);
	if (Source && *Source && Target && *Target)
	{
		FaceProxy(SourceId, (*Target)->GetActorLocation() - (*Source)->GetActorLocation());
	}
}

void UHMT_CombatPlaybackDriver::PlayProxyAnim(const FGuid& InstanceId, UAnimMontage* Anim, bool bLoop, bool bReturnToIdleAfter, float PlayRate)
{
	if (!Anim)
	{
		return;
	}

	TObjectPtr<ASkeletalMeshActor>* Proxy = Proxies.Find(InstanceId);
	if (!Proxy || !*Proxy || !(*Proxy)->GetSkeletalMeshComponent())
	{
		return;
	}

	PlayRate = FMath::Max(PlayRate, KINDA_SMALL_NUMBER);

	// Single-node playback — no AnimBP required on the proxy, montages play directly.
	USkeletalMeshComponent* MeshComponent = (*Proxy)->GetSkeletalMeshComponent();
	MeshComponent->PlayAnimation(Anim, bLoop);
	MeshComponent->SetPlayRate(PlayRate);

	if (bReturnToIdleAfter)
	{
		ReturnToIdleAt.Add(InstanceId, PlaybackTime + Anim->GetPlayLength() / PlayRate);
	}
}

void UHMT_CombatPlaybackDriver::PlayProxySound(const FGuid& InstanceId, USoundBase* Sound) const
{
	if (!Sound)
	{
		return;
	}

	const TObjectPtr<ASkeletalMeshActor>* Proxy = Proxies.Find(InstanceId);
	if (Proxy && *Proxy)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, (*Proxy)->GetActorLocation());
	}
}

void UHMT_CombatPlaybackDriver::ApplyDamageToHealthBar(const FGuid& InstanceId, float Damage)
{
	float* CurrentHealth = CurrentHealthByInstance.Find(InstanceId);
	const float* MaxHealth = MaxHealthByInstance.Find(InstanceId);
	if (!CurrentHealth || !MaxHealth)
	{
		return;
	}

	// Damage may be negative (heal) — clamp both ends.
	*CurrentHealth = FMath::Clamp(*CurrentHealth - Damage, 0.f, *MaxHealth);

	if (TObjectPtr<UWidgetComponent>* HealthBarComponent = HealthBars.Find(InstanceId); HealthBarComponent && *HealthBarComponent)
	{
		if (UUserWidget* HealthBarWidget = (*HealthBarComponent)->GetUserWidgetObject(); HealthBarWidget && HealthBarWidget->GetClass()->ImplementsInterface(UHMT_UnitHealthBarInterface::StaticClass()))
		{
			IHMT_UnitHealthBarInterface::Execute_SetHealthFraction(HealthBarWidget, *CurrentHealth, *MaxHealth, bIsEnemyByInstance.FindRef(InstanceId));
		}
	}
}

void UHMT_CombatPlaybackDriver::ShowCastBar(const FGuid& InstanceId, float Duration)
{
	// Defensive dedup, same reasoning as ActiveMoves: a stray double CastStart shouldn't stack.
	ActiveCasts.RemoveAll([&InstanceId](const FActiveCast& Existing) { return Existing.InstanceId == InstanceId; });

	FActiveCast Cast;
	Cast.InstanceId = InstanceId;
	Cast.StartTime = PlaybackTime;
	Cast.Duration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	ActiveCasts.Add(Cast);

	if (TObjectPtr<UWidgetComponent>* CastBarComponent = CastBars.Find(InstanceId); CastBarComponent && *CastBarComponent)
	{
		(*CastBarComponent)->SetVisibility(true);
		if (UUserWidget* CastBarWidget = (*CastBarComponent)->GetUserWidgetObject(); CastBarWidget && CastBarWidget->GetClass()->ImplementsInterface(UHMT_UnitCastBarInterface::StaticClass()))
		{
			IHMT_UnitCastBarInterface::Execute_SetCastProgress(CastBarWidget, 0.f);
		}
	}
}

void UHMT_CombatPlaybackDriver::HideCastBar(const FGuid& InstanceId)
{
	ActiveCasts.RemoveAll([&InstanceId](const FActiveCast& Existing) { return Existing.InstanceId == InstanceId; });

	if (TObjectPtr<UWidgetComponent>* CastBarComponent = CastBars.Find(InstanceId); CastBarComponent && *CastBarComponent)
	{
		(*CastBarComponent)->SetVisibility(false);
	}
}

void UHMT_CombatPlaybackDriver::SetRealUnitMeshesVisible(bool bVisible)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Component visibility is not replicated — safe to toggle locally even on a listen server.
	for (TActorIterator<AHMT_UnitActor> It(World); It; ++It)
	{
		// Restoring visibility must go through the per-machine ownership policy, not a blanket
		// "true" — that would re-reveal OPPONENT placement units this machine deliberately hides
		// (see AHMT_GameUnitActor). Hiding is unconditional; only the restore is policy-driven.
		if (AHMT_GameUnitActor* SampleUnit = Cast<AHMT_GameUnitActor>(*It); SampleUnit && bVisible)
		{
			SampleUnit->RefreshLocalVisibility();
		}
		else if (It->MeshComponent)
		{
			It->MeshComponent->SetVisibility(bVisible, true);
		}
	}
}

FString UHMT_CombatPlaybackDriver::GetFeedNameForInstance(const FGuid& InstanceId) const
{
	const UHMT_CombatPlaybackComponent* Source = ActiveSource ? ActiveSource.Get() : Playback.Get();
	if (!Source)
	{
		return FString();
	}
	bool bFound = false;
	FText DisplayName;
	int32 StarLevel = 0;
	bool bIsOwnerUnit = false;
	Source->GetUnitDisplayInfoForInstance(InstanceId, bFound, DisplayName, StarLevel, bIsOwnerUnit);
	if (!bFound)
	{
		return FString();
	}
	return StarLevel > 1
		? FString::Printf(TEXT("%s %d★"), *DisplayName.ToString(), StarLevel)
		: DisplayName.ToString();
}

void UHMT_CombatPlaybackDriver::AppendFeedLine(const FHMT_CombatEvent& Event)
{
	FString Line;
	switch (Event.EventType)
	{
	case EHMT_CombatEventType::DamageDealt:
	{
		const FString Source = GetFeedNameForInstance(Event.SourceInstanceId);
		const FString Target = GetFeedNameForInstance(Event.TargetInstanceId);
		if (Source.IsEmpty() || Target.IsEmpty())
		{
			return;
		}
		// Negative magnitude is a heal by convention (see the resolver) — flip wording, not sign.
		Line = Event.Magnitude < 0.f
			? FString::Printf(TEXT("%s heals %s  +%.0f"), *Source, *Target, -Event.Magnitude)
			: FString::Printf(TEXT("%s > %s  -%.0f%s"), *Source, *Target, Event.Magnitude, Event.bIsCritical ? TEXT("  CRIT") : TEXT(""));
		break;
	}
	case EHMT_CombatEventType::AbilityCast:
		Line = FString::Printf(TEXT("%s casts an ability"), *GetFeedNameForInstance(Event.SourceInstanceId));
		break;
	case EHMT_CombatEventType::UnitDeath:
		Line = FString::Printf(TEXT("%s is eliminated"), *GetFeedNameForInstance(Event.SourceInstanceId));
		break;
	case EHMT_CombatEventType::UnitSpawned:
		Line = FString::Printf(TEXT("%s joins the fight"), *GetFeedNameForInstance(Event.TargetInstanceId));
		break;
	case EHMT_CombatEventType::CombatEnd:
	{
		bool bHasResult = false, bWon = false, bDraw = false;
		if (const UHMT_CombatPlaybackComponent* Source = ActiveSource ? ActiveSource.Get() : Playback.Get())
		{
			Source->GetLastCombatResult(bHasResult, bWon, bDraw);
		}
		Line = !bHasResult ? TEXT("Combat over") : bDraw ? TEXT("DRAW") : bWon ? TEXT("VICTORY") : TEXT("DEFEAT");
		break;
	}
	default:
		return; // moves/attack-windup/status ticks would drown the report in noise
	}

	if (!Line.IsEmpty())
	{
		FeedLines.Add(Line);
		// Rolling window: the widget shows the tail anyway, no reason to grow unbounded.
		constexpr int32 MaxRetainedLines = 64;
		if (FeedLines.Num() > MaxRetainedLines)
		{
			FeedLines.RemoveAt(0, FeedLines.Num() - MaxRetainedLines);
		}
	}
}

FText UHMT_CombatPlaybackDriver::GetCombatFeedText(int32 MaxLines) const
{
	if (FeedLines.Num() == 0 || MaxLines <= 0)
	{
		return FText::GetEmpty();
	}
	const int32 First = FMath::Max(0, FeedLines.Num() - MaxLines);
	TArray<FString> Tail;
	for (int32 Index = First; Index < FeedLines.Num(); ++Index)
	{
		Tail.Add(FeedLines[Index]);
	}
	return FText::FromString(FString::Join(Tail, TEXT("\n")));
}
