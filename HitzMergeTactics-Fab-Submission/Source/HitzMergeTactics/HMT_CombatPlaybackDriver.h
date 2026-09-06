#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/HMT_CombatTypes.h"
#include "Units/HMT_UnitAnimSet.h"
#include "Units/HMT_UnitFXSet.h"
#include "Units/HMT_UnitProjectileSet.h"
#include "HMT_CombatPlaybackDriver.generated.h"

class UHMT_CombatPlaybackComponent;
class ASkeletalMeshActor;
class UWidgetComponent;
class UUserWidget;
class USoundBase;

/**
 * Sample visual playback of the replicated combat log: local-only skeletal-mesh proxies
 * spawned from the locked snapshots, events scheduled by their sim timestamps, moves
 * lerped, damage shown as floating debug text, deaths hiding the proxy — everything
 * restored at CombatEnd. Runs ONLY for the locally controlled player's own playback
 * component, and touches zero replicated state (proxies are never replicated; the real
 * placement actors only get local mesh-visibility toggled) — purely cosmetic by design.
 */
UCLASS(ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class UHMT_CombatPlaybackDriver : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_CombatPlaybackDriver();

	/** Assign a Designer-built WBP implementing IHMT_UnitHealthBarInterface to show health
	 *  bars above combat proxies — pure Blueprint, no C++ parent class required. Left unset,
	 *  playback runs exactly as before (no bars). */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	/** Assign a Designer-built WBP implementing IHMT_UnitCastBarInterface to show a cast bar
	 *  above a proxy while it's channeling an ability (see UHMT_CombatResolver's optional
	 *  CastTimeStatTag) — pure Blueprint, no C++ parent class required. Left unset, playback runs
	 *  exactly as before cast-time existed (no bar; casts that ARE instant show nothing either way). */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	TSubclassOf<UUserWidget> CastBarWidgetClass;

	/** Added to every facing rotation so the MESH looks where the actor "faces" — UE mannequin-
	 *  convention skeletal meshes are authored facing +Y in mesh space (the Character template
	 *  compensates with a -90 relative yaw on its mesh component; bare ASkeletalMeshActor proxies
	 *  have no such correction, so this does it). Set to 0 for meshes authored facing +X. */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	float ProxyMeshYawOffset = -90.f;

	/** Seconds surviving units linger on their tiles after the last combat event, looping their
	 *  AnimSet's Victory montage (Idle when unauthored), before playback tears down and the real
	 *  placement units reappear. 0 = tear down instantly, the old behavior. */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	float VictoryDisplayDuration = 3.f;

	/** Rolling battle report ("Yin > Crunch  -18  CRIT", deaths, the round result), revealed in
	 *  step with playback pacing — never ahead of what's on screen, so it can't spoil the outcome.
	 *  Client-only by construction (this driver runs solely for the locally viewed playback). A
	 *  feed widget polls this on the standard 0.2s self-refresh; empty text = nothing to show. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	FText GetCombatFeedText(int32 MaxLines = 8) const;

	/** Retargets an ALREADY-RUNNING playback to a different source mid-combat — the explicit path
	 *  a controller calls the instant its spectate target changes while the match phase is
	 *  Combat (see AHMT_PlayerController::SetSpectateTarget). NewSource null means "back to our
	 *  own fight". No-op if nothing is currently playing (bPlaying false — the next StartPlayback
	 *  will pick correctly on its own), already showing NewSource, or NewSource's log hasn't
	 *  finished replicating yet (no CombatEnd present — logs a message and waits rather than
	 *  showing a half-fight). Tears down current proxies and restarts the new fight from the top;
	 *  this is a replay-log swap, not a synchronized resume. */
	void SwitchPlaybackSource(class UHMT_CombatPlaybackComponent* NewSource);

	/** Safety-net cleanup: tears down playback (proxies, health/cast bars, everything
	 *  TearDownProxiesOnly clears) if anything is still playing, exactly the same guard
	 *  StartPlayback itself uses before starting the next fight. Called from AHMT_GameMode's
	 *  OnRoundStart handler on every player at the start of every round, regardless of whether the
	 *  normal post-combat teardown (victory lap end + IsRoundPastCombat) already ran — see that
	 *  handler's comment for why this exists as a second line of defense. No-op if nothing is
	 *  playing, so it never cuts off a legitimately fresh playback that just started. */
	void EnsureNotPlaying() { if (bPlaying) { FinishPlayback(); } }

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleCombatEvent(const FHMT_CombatEvent& Event);

	void StartPlayback();
	// Named to avoid hiding UObject::ProcessEvent(UFunction*, void*) — a core engine virtual.
	void ProcessCombatEvent(const FHMT_CombatEvent& Event);
	void FinishPlayback();

	/** Shared proxy/tracking-map cleanup used by both FinishPlayback (combat's genuinely over)
	 *  and SwitchPlaybackSource (still mid-combat, just showing a different fight) — leaves
	 *  bPlaying/ActiveSource/real-unit visibility alone; callers decide those. */
	void TearDownProxiesOnly();

	/** Spawns proxies and starts ticking from whatever's already in ActiveSource/BufferedEvents —
	 *  the shared second half of StartPlayback and SwitchPlaybackSource once each has decided
	 *  which source and events to play. Seeds PlaybackTime from GetSyncedCombatElapsedSeconds,
	 *  not 0 — see that function's comment for why. */
	void BeginPlaybackFromSource();

	/** Seconds into ActiveSource's fight RIGHT NOW, off UHMT_CombatPlaybackComponent::
	 *  GetElapsedSinceCombatStart (the server-synced timestamp stamped directly on the log
	 *  component itself — see that function's comment for why it's NOT read from
	 *  UHMT_MatchStateComponent's separate phase timer, a different actor with an independent,
	 *  unordered replication race against the log). Every viewer of the same log — its owner, or
	 *  any spectator joining at any moment — computes the same value at the same real instant.
	 *  Restarting PlaybackTime at 0 for whoever happens to press SPECTATE was the actual bug
	 *  behind two machines disagreeing about the same fight: each was running its own untethered
	 *  replay clock instead of the fight's real one. 0 if ActiveSource is unset. */
	float GetSyncedCombatElapsedSeconds() const;

	/** True once the match phase has moved past Combat (Results/RoundEnd/next Preparation) — gates
	 *  FinishPlayback so visuals don't restore to placement while still nominally mid-combat. See
	 *  the call site's comment for why this checks "not Combat" instead of polling for RoundEnd. */
	bool IsRoundPastCombat() const;

	FVector ArenaCoordToWorld(const FHMT_GridCoord& ArenaCoord) const;
	ASkeletalMeshActor* SpawnProxy(const FHMT_SnapshotUnit& Unit, const FHMT_GridCoord& ArenaCoord, bool bIsEnemy);
	void SetRealUnitMeshesVisible(bool bVisible);

	UPROPERTY()
	TObjectPtr<UHMT_CombatPlaybackComponent> Playback;

	/** The component whose log/snapshots the CURRENT playback is actually showing: normally
	 *  Playback (our own fight), but while spectating another player at combat start it's THEIR
	 *  playback component — their fight, projected onto our board (see StartPlayback). Everything
	 *  that resolves names/sides/results during playback must read this, never Playback directly. */
	UPROPERTY()
	TObjectPtr<UHMT_CombatPlaybackComponent> ActiveSource;

	TArray<FHMT_CombatEvent> BufferedEvents;
	int32 NextEventIndex = 0;
	float PlaybackTime = 0.f;
	bool bPlaying = false;

	/** PlaybackTime's value at the moment BeginPlaybackFromSource seeded it — everything with
	 *  Event.Timestamp at or before this is history that already happened before this viewing
	 *  session started (a late join, or a spectate switch mid/post-fight), processed in one instant
	 *  catch-up burst rather than watched live tick-by-tick. One-shot animated events (death, in
	 *  particular) check this to snap straight to their resolved end state instead of replaying the
	 *  full animation — without it, switching spectate onto an already-dead unit made it visibly pop
	 *  back to life for one frame and play its death animation all over again. */
	float CatchUpCutoffTime = 0.f;

	/** Set once when the event log is exhausted — survivors are switched to Victory/Idle loops and
	 *  FinishPlayback is deferred until VictoryLapEndAt. See VictoryDisplayDuration. */
	bool bVictoryLapStarted = false;
	float VictoryLapEndAt = 0.f;

	UPROPERTY()
	TMap<FGuid, TObjectPtr<ASkeletalMeshActor>> Proxies;

	UPROPERTY()
	TMap<FGuid, TObjectPtr<UWidgetComponent>> HealthBars;

	UPROPERTY()
	TMap<FGuid, TObjectPtr<UWidgetComponent>> CastBars;

	TMap<FGuid, float> CurrentHealthByInstance;
	TMap<FGuid, float> MaxHealthByInstance;

	/** AttackSpeed (attacks/sec) resolved the same way as MaxHealth at spawn — used to scale the
	 *  Attack montage's PlayRate in PlayProxyAnim so animation cadence matches the sim's actual
	 *  attack cadence instead of always playing at the montage's authored speed. */
	TMap<FGuid, float> AttackSpeedByInstance;

	/** Resolved overhead-widget anchor per proxy (DA override or mesh-bounds-derived — see
	 *  SpawnProxy), shared by the widget components and the debug-line fallback bars. */
	TMap<FGuid, FVector> BarAnchorByInstance;

	/** Backing lines for GetCombatFeedText, appended per processed event, reset per playback. */
	TArray<FString> FeedLines;
	void AppendFeedLine(const FHMT_CombatEvent& Event);

	/** "Name" or "Name ★N" for merged units, empty-safe for unknown instance ids. */
	FString GetFeedNameForInstance(const FGuid& InstanceId) const;

	/** Relative to the locally viewing player — drives red (enemy) vs green (ally) health bars. */
	TMap<FGuid, bool> bIsEnemyByInstance;

	void ApplyDamageToHealthBar(const FGuid& InstanceId, float Damage);

	/** Per-proxy montage set (from the unit definition's AnimSet), captured at proxy spawn. */
	TMap<FGuid, FHMT_UnitAnimSet> AnimSets;

	/** Per-proxy sound set (from the unit definition's AudioSet), captured at proxy spawn. FX
	 *  (FHMT_UnitFXSet) is deliberately NOT wired here — FHMT_UnitFXSet is typed as the shared
	 *  UFXSystemAsset base (both UParticleSystem/Cascade and UNiagaraSystem derive from it, no hard
	 *  Niagara dependency needed just to reference it), but actually SPAWNING one still needs a
	 *  Cast<> + branch to the concrete type's own spawn API (UGameplayStatics::
	 *  SpawnEmitterAtLocation for Cascade, UNiagaraFunctionLibrary::SpawnSystemAtLocation for
	 *  Niagara) — that branch is a buyer-side call, not something this framework can make once for
	 *  both. Buyers wire FX playback themselves (the data is authored and available via
	 *  UHMT_UnitDefinition::FXSet either way). */
	TMap<FGuid, FHMT_UnitAudioSet> AudioSets;

	/** Per-proxy projectile config (from the unit definition's ProjectileSet), captured at proxy
	 *  spawn. Null ProjectileClass = the existing debug tracer fallback in AttackStart handling. */
	TMap<FGuid, FHMT_UnitProjectileSet> ProjectileSets;

	/** Null Sound is a no-op, matching PlayProxyAnim's degrade-gracefully-when-unauthored rule. */
	void PlayProxySound(const FGuid& InstanceId, USoundBase* Sound) const;

	/** Playback timestamps at which a proxy's one-shot montage ends and Idle should resume. */
	TMap<FGuid, float> ReturnToIdleAt;

	/** Dead proxies stay visible until their Death montage finishes, then hide at this timestamp. */
	TMap<FGuid, float> HideProxyAt;

	/** Plays a montage on the proxy via single-node playback (no AnimBP needed). Null Anim is a
	 *  no-op, so unauthored AnimSet fields degrade to the un-animated behavior. PlayRate defaults
	 *  to 1 (authored speed); the Attack case computes one from AttackSpeedByInstance so faster
	 *  attackers visibly swing faster instead of the anim lagging behind/ahead of the sim. */
	void PlayProxyAnim(const FGuid& InstanceId, UAnimMontage* Anim, bool bLoop, bool bReturnToIdleAfter, float PlayRate = 1.f);

	/** Yaw-snaps the proxy to look along WorldDirection (Z ignored), respecting ProxyMeshYawOffset. */
	void FaceProxy(const FGuid& InstanceId, const FVector& WorldDirection);

	/** Faces SourceId toward TargetId's current proxy location — attack/cast feedback. */
	void FaceProxyAtProxy(const FGuid& SourceId, const FGuid& TargetId);

	struct FActiveMove
	{
		TWeakObjectPtr<ASkeletalMeshActor> Proxy;
		FGuid InstanceId;
		FVector From = FVector::ZeroVector;
		FVector To = FVector::ZeroVector;
		float StartTime = 0.f;
		float Duration = 0.f;
	};
	TArray<FActiveMove> ActiveMoves;

	/** One entry per proxy currently channeling — driven by CastStart/AbilityCast/CastInterrupted
	 *  events, ticked each frame to push a smooth 0..1 fill to the cast bar widget in between. */
	struct FActiveCast
	{
		FGuid InstanceId;
		float StartTime = 0.f;
		float Duration = 0.f;
	};
	TArray<FActiveCast> ActiveCasts;
	void ShowCastBar(const FGuid& InstanceId, float Duration);
	void HideCastBar(const FGuid& InstanceId);

	int32 ArenaColumns = 0;
	int32 ArenaRows = 0;
};
