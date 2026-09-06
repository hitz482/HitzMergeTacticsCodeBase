#include "Stats/HMT_StatComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

UHMT_StatComponent::UHMT_StatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_StatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_StatComponent, BaseStats);
	DOREPLIFETIME(UHMT_StatComponent, ModifierArray);
}

FHMT_BaseStat* UHMT_StatComponent::FindBaseStat(FGameplayTag StatTag)
{
	return BaseStats.FindByPredicate([&StatTag](const FHMT_BaseStat& Stat) { return Stat.StatTag == StatTag; });
}

const FHMT_BaseStat* UHMT_StatComponent::FindBaseStat(FGameplayTag StatTag) const
{
	return BaseStats.FindByPredicate([&StatTag](const FHMT_BaseStat& Stat) { return Stat.StatTag == StatTag; });
}

float UHMT_StatComponent::GetBaseStatValue_Implementation(FGameplayTag StatTag)
{
	const FHMT_BaseStat* Found = FindBaseStat(StatTag);
	return Found ? Found->Value : 0.f;
}

void UHMT_StatComponent::SetBaseStatValue_Implementation(FGameplayTag StatTag, float Value)
{
	if (FHMT_BaseStat* Found = FindBaseStat(StatTag))
	{
		Found->Value = Value;
	}
	else
	{
		FHMT_BaseStat NewStat;
		NewStat.StatTag = StatTag;
		NewStat.Value = Value;
		BaseStats.Add(NewStat);
	}
}

float UHMT_StatComponent::GetFinalStatValue_Implementation(FGameplayTag StatTag)
{
	const float Base = GetBaseStatValue_Implementation(StatTag);

	float AdditiveSum = 0.f;
	float PercentAdditiveSum = 0.f;
	float MultiplicativeProduct = 1.f;
	bool bHasOverride = false;
	float OverrideValue = 0.f;

	for (const FHMT_StatModifier& Modifier : ModifierArray.Modifiers)
	{
		if (Modifier.StatTag != StatTag)
		{
			continue;
		}

		switch (Modifier.ModOp)
		{
		case EHMT_StatModOp::Additive:
			AdditiveSum += Modifier.Magnitude;
			break;
		case EHMT_StatModOp::PercentAdditive:
			PercentAdditiveSum += Modifier.Magnitude;
			break;
		case EHMT_StatModOp::Multiplicative:
			MultiplicativeProduct *= Modifier.Magnitude;
			break;
		case EHMT_StatModOp::Override:
			bHasOverride = true;
			OverrideValue = Modifier.Magnitude;
			break;
		}
	}

	if (bHasOverride)
	{
		return OverrideValue;
	}

	return (Base + AdditiveSum) * (1.f + PercentAdditiveSum) * MultiplicativeProduct;
}

void UHMT_StatComponent::ApplyStatModifier_Implementation(FGameplayTag StatTag, EHMT_StatModOp ModOp, float Magnitude, EHMT_ModifierDuration Duration, float TimedDurationSeconds, EHMT_StackingRule StackingRule, FGameplayTag SourceTag)
{
	if (StackingRule != EHMT_StackingRule::Stack)
	{
		const int32 ExistingIndex = ModifierArray.Modifiers.IndexOfByPredicate([&StatTag, &SourceTag](const FHMT_StatModifier& Modifier)
		{
			return Modifier.StatTag == StatTag && Modifier.SourceTag == SourceTag;
		});

		if (ExistingIndex != INDEX_NONE)
		{
			if (StackingRule == EHMT_StackingRule::Ignore)
			{
				return;
			}

			FHMT_StatModifier& Existing = ModifierArray.Modifiers[ExistingIndex];
			Existing.Magnitude = Magnitude;
			Existing.TimedDurationSeconds = TimedDurationSeconds;
			ModifierArray.MarkItemDirty(Existing);

			if (Existing.Duration == EHMT_ModifierDuration::Timed && TimedDurationSeconds > 0.f && GetWorld())
			{
				FTimerHandle& Timer = TimedExpiryHandles.FindOrAdd(Existing.ModifierHandle);
				GetWorld()->GetTimerManager().SetTimer(Timer,
					FTimerDelegate::CreateUObject(this, &UHMT_StatComponent::RemoveModifierByHandle, Existing.ModifierHandle),
					TimedDurationSeconds, false);
			}
			return;
		}
	}

	FHMT_StatModifier NewModifier;
	NewModifier.StatTag = StatTag;
	NewModifier.ModOp = ModOp;
	NewModifier.Magnitude = Magnitude;
	NewModifier.Duration = Duration;
	NewModifier.TimedDurationSeconds = TimedDurationSeconds;
	NewModifier.StackingRule = StackingRule;
	NewModifier.SourceTag = SourceTag;
	NewModifier.ModifierHandle = NextModifierHandle++;

	const int32 NewIndex = ModifierArray.Modifiers.Add(NewModifier);
	ModifierArray.MarkItemDirty(ModifierArray.Modifiers[NewIndex]);

	if (Duration == EHMT_ModifierDuration::Timed && TimedDurationSeconds > 0.f && GetWorld())
	{
		FTimerHandle& Timer = TimedExpiryHandles.FindOrAdd(NewModifier.ModifierHandle);
		GetWorld()->GetTimerManager().SetTimer(Timer,
			FTimerDelegate::CreateUObject(this, &UHMT_StatComponent::RemoveModifierByHandle, NewModifier.ModifierHandle),
			TimedDurationSeconds, false);
	}
}

void UHMT_StatComponent::RemoveModifierByHandle(int32 Handle)
{
	TimedExpiryHandles.Remove(Handle);

	const int32 RemovedCount = ModifierArray.Modifiers.RemoveAll([Handle](const FHMT_StatModifier& Modifier)
	{
		return Modifier.ModifierHandle == Handle;
	});
	if (RemovedCount > 0)
	{
		ModifierArray.MarkArrayDirty();
	}
}

TArray<FHMT_StatModifier> UHMT_StatComponent::GetActiveModifiers_Implementation()
{
	return ModifierArray.Modifiers;
}

void UHMT_StatComponent::RemoveModifiersByDuration_Implementation(EHMT_ModifierDuration Duration)
{
	const int32 RemovedCount = ModifierArray.Modifiers.RemoveAll([Duration](const FHMT_StatModifier& Modifier)
	{
		return Modifier.Duration == Duration;
	});
	if (RemovedCount > 0)
	{
		ModifierArray.MarkArrayDirty();
	}
}

void UHMT_StatComponent::RemoveModifiersBySource_Implementation(FGameplayTag SourceTag)
{
	const int32 RemovedCount = ModifierArray.Modifiers.RemoveAll([&SourceTag](const FHMT_StatModifier& Modifier)
	{
		return Modifier.SourceTag == SourceTag;
	});

	if (RemovedCount > 0)
	{
		ModifierArray.MarkArrayDirty();
	}
}
