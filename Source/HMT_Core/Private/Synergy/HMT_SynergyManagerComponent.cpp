#include "Synergy/HMT_SynergyManagerComponent.h"
#include "Synergy/IHMT_TraitEffect.h"
#include "Units/HMT_UnitCollectionComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Units/HMT_UnitDefinition.h"
#include "Grid/HMT_BoardComponent.h"
#include "Stats/IHMT_StatProvider.h"
#include "Net/UnrealNetwork.h"

UHMT_SynergyManagerComponent::UHMT_SynergyManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_SynergyManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_SynergyManagerComponent, ActiveBreakpoints);
	DOREPLIFETIME(UHMT_SynergyManagerComponent, RegisteredTraits);
}

int32 UHMT_SynergyManagerComponent::GetActiveBreakpointIndex(FGameplayTag TraitTag) const
{
	const FHMT_ActiveTraitBreakpoint* Found = ActiveBreakpoints.FindByPredicate(
		[&TraitTag](const FHMT_ActiveTraitBreakpoint& Entry) { return Entry.TraitTag == TraitTag; });
	return Found ? Found->ActiveBreakpointIndex : INDEX_NONE;
}

TArray<TSubclassOf<UObject>> UHMT_SynergyManagerComponent::GetActiveTraitEffectClasses() const
{
	TArray<TSubclassOf<UObject>> Result;

	for (const FHMT_ActiveTraitBreakpoint& Active : ActiveBreakpoints)
	{
		if (Active.ActiveBreakpointIndex == INDEX_NONE)
		{
			continue;
		}

		const TObjectPtr<UHMT_TraitDefinition>* TraitDef = RegisteredTraits.FindByPredicate(
			[&Active](const UHMT_TraitDefinition* Trait) { return Trait && Trait->TraitTag == Active.TraitTag; });

		if (TraitDef && (*TraitDef)->Breakpoints.IsValidIndex(Active.ActiveBreakpointIndex))
		{
			if (UClass* EffectClass = (*TraitDef)->Breakpoints[Active.ActiveBreakpointIndex].TraitEffectClass)
			{
				Result.Add(EffectClass);
			}
		}
	}

	return Result;
}

void UHMT_SynergyManagerComponent::RecomputeSynergies()
{
	UHMT_UnitCollectionComponent* Collection = GetOwner() ? GetOwner()->FindComponentByClass<UHMT_UnitCollectionComponent>() : nullptr;
	UHMT_BoardComponent* Board = GetOwner() ? GetOwner()->FindComponentByClass<UHMT_BoardComponent>() : nullptr;
	if (!Collection || !Board)
	{
		return;
	}

	const TArray<AActor*> AllUnits = Collection->GetAllOwnedUnits();
	const TArray<AActor*> DeployedUnits = Board->GetAllOccupants();

	TMap<FGameplayTag, TArray<AActor*>> UnitsByTrait;
	for (AActor* Unit : DeployedUnits)
	{
		const UHMT_UnitInstanceComponent* Instance = Unit->FindComponentByClass<UHMT_UnitInstanceComponent>();
		if (!Instance || !Instance->GetUnitDefinition())
		{
			continue;
		}

		for (const FGameplayTag& Trait : Instance->GetUnitDefinition()->Traits)
		{
			UnitsByTrait.FindOrAdd(Trait).Add(Unit);
		}
	}

	for (UHMT_TraitDefinition* TraitDef : RegisteredTraits)
	{
		if (!TraitDef)
		{
			continue;
		}

		const TArray<AActor*>* AffectedUnits = UnitsByTrait.Find(TraitDef->TraitTag);
		const int32 Count = AffectedUnits ? AffectedUnits->Num() : 0;
		const int32 NewBreakpointIndex = TraitDef->GetHighestMetBreakpointIndex(Count);

		FHMT_ActiveTraitBreakpoint* ActiveEntry = ActiveBreakpoints.FindByPredicate(
			[TraitDef](const FHMT_ActiveTraitBreakpoint& Entry) { return Entry.TraitTag == TraitDef->TraitTag; });

		const int32 OldBreakpointIndex = ActiveEntry ? ActiveEntry->ActiveBreakpointIndex : INDEX_NONE;
		if (NewBreakpointIndex == OldBreakpointIndex)
		{
			continue;
		}

		for (AActor* Unit : AllUnits)
		{
			if (UActorComponent* StatProviderComponent = Unit->FindComponentByInterface(UHMT_StatProvider::StaticClass()))
			{
				IHMT_StatProvider::Execute_RemoveModifiersBySource(StatProviderComponent, TraitDef->TraitTag);
			}
		}

		if (OldBreakpointIndex != INDEX_NONE && TraitDef->Breakpoints.IsValidIndex(OldBreakpointIndex))
		{
			if (UClass* EffectClass = TraitDef->Breakpoints[OldBreakpointIndex].TraitEffectClass)
			{
				UObject* EffectInstance = NewObject<UObject>(this, EffectClass);
				IHMT_TraitEffect::Execute_OnDeactivate(EffectInstance, AffectedUnits ? *AffectedUnits : TArray<AActor*>());
			}
		}

		if (NewBreakpointIndex != INDEX_NONE && AffectedUnits)
		{
			const FHMT_TraitBreakpoint& Breakpoint = TraitDef->Breakpoints[NewBreakpointIndex];

			for (AActor* Unit : *AffectedUnits)
			{
				UActorComponent* StatProviderComponent = Unit->FindComponentByInterface(UHMT_StatProvider::StaticClass());
				if (!StatProviderComponent)
				{
					continue;
				}

				for (const FHMT_TraitStatGrant& Grant : Breakpoint.StatGrants)
				{
					IHMT_StatProvider::Execute_ApplyStatModifier(StatProviderComponent, Grant.StatTag, Grant.ModOp, Grant.Magnitude,
						EHMT_ModifierDuration::Permanent, 0.f, EHMT_StackingRule::Stack, TraitDef->TraitTag);
				}
			}

			if (UClass* EffectClass = Breakpoint.TraitEffectClass)
			{
				UObject* EffectInstance = NewObject<UObject>(this, EffectClass);
				IHMT_TraitEffect::Execute_OnActivate(EffectInstance, *AffectedUnits);
			}
		}

		if (ActiveEntry)
		{
			ActiveEntry->ActiveBreakpointIndex = NewBreakpointIndex;
		}
		else
		{
			FHMT_ActiveTraitBreakpoint NewEntry;
			NewEntry.TraitTag = TraitDef->TraitTag;
			NewEntry.ActiveBreakpointIndex = NewBreakpointIndex;
			ActiveBreakpoints.Add(NewEntry);
		}

		OnTraitActivated.Broadcast(TraitDef->TraitTag, NewBreakpointIndex);
	}
}
