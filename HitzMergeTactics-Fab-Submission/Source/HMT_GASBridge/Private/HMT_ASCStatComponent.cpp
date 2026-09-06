#include "HMT_ASCStatComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "UObject/Package.h"

UHMT_ASCStatComponent::UHMT_ASCStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHMT_ASCStatComponent::InitializeBridge(UHMT_AttributeTagMapAsset* InAttributeTagMap)
{
	AttributeTagMap = InAttributeTagMap;
}

UAbilitySystemComponent* UHMT_ASCStatComponent::GetAbilitySystemComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return Owner->FindComponentByClass<UAbilitySystemComponent>();
}

float UHMT_ASCStatComponent::GetBaseStatValue_Implementation(FGameplayTag StatTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AttributeTagMap)
	{
		return 0.f;
	}

	const FGameplayAttribute Attribute = AttributeTagMap->ResolveAttribute(StatTag);
	return Attribute.IsValid() ? ASC->GetNumericAttributeBase(Attribute) : 0.f;
}

void UHMT_ASCStatComponent::SetBaseStatValue_Implementation(FGameplayTag StatTag, float Value)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AttributeTagMap)
	{
		return;
	}

	const FGameplayAttribute Attribute = AttributeTagMap->ResolveAttribute(StatTag);
	if (Attribute.IsValid())
	{
		ASC->SetNumericAttributeBase(Attribute, Value);
	}
}

float UHMT_ASCStatComponent::GetFinalStatValue_Implementation(FGameplayTag StatTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AttributeTagMap)
	{
		return 0.f;
	}

	const FGameplayAttribute Attribute = AttributeTagMap->ResolveAttribute(StatTag);
	return Attribute.IsValid() ? ASC->GetNumericAttribute(Attribute) : 0.f;
}

void UHMT_ASCStatComponent::ApplyStatModifier_Implementation(FGameplayTag StatTag, EHMT_StatModOp ModOp, float Magnitude, EHMT_ModifierDuration Duration, float TimedDurationSeconds, EHMT_StackingRule StackingRule, FGameplayTag SourceTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AttributeTagMap)
	{
		return;
	}

	const FGameplayAttribute Attribute = AttributeTagMap->ResolveAttribute(StatTag);
	if (!Attribute.IsValid())
	{
		return;
	}

	const FHMT_ModifierKey Key{ StatTag, SourceTag };

	if (StackingRule != EHMT_StackingRule::Stack)
	{
		if (const FActiveGameplayEffectHandle* Existing = ActiveHandles.Find(Key))
		{
			if (StackingRule == EHMT_StackingRule::Ignore)
			{
				return;
			}

			ASC->RemoveActiveGameplayEffect(*Existing);
			ActiveHandles.Remove(Key);
		}
	}

	EGameplayModOp::Type GasModOp = EGameplayModOp::Additive;
	float GasMagnitude = Magnitude;

	switch (ModOp)
	{
	case EHMT_StatModOp::Additive:
		GasModOp = EGameplayModOp::Additive;
		break;
	case EHMT_StatModOp::PercentAdditive:
		GasModOp = EGameplayModOp::Additive;
		GasMagnitude = Magnitude * ASC->GetNumericAttributeBase(Attribute);
		break;
	case EHMT_StatModOp::Multiplicative:
		GasModOp = EGameplayModOp::Multiplicitive;
		break;
	case EHMT_StatModOp::Override:
		GasModOp = EGameplayModOp::Override;
		break;
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	Effect->DurationPolicy = (Duration == EHMT_ModifierDuration::Timed) ? EGameplayEffectDurationType::HasDuration : EGameplayEffectDurationType::Infinite;
	if (Duration == EHMT_ModifierDuration::Timed)
	{
		Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(TimedDurationSeconds));
	}

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = Attribute;
	ModifierInfo.ModifierOp = GasModOp;
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(GasMagnitude));
	Effect->Modifiers.Add(ModifierInfo);

	FGameplayEffectSpec Spec(Effect, FGameplayEffectContextHandle(), 1.f);
	const FActiveGameplayEffectHandle NewHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	if (NewHandle.IsValid())
	{
		ActiveHandles.Add(Key, NewHandle);
		SourceTagToKeys.FindOrAdd(SourceTag).Add(Key);
	}
}

void UHMT_ASCStatComponent::RemoveModifiersBySource_Implementation(FGameplayTag SourceTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	TArray<FHMT_ModifierKey>* Keys = SourceTagToKeys.Find(SourceTag);
	if (!Keys)
	{
		return;
	}

	for (const FHMT_ModifierKey& Key : *Keys)
	{
		if (const FActiveGameplayEffectHandle* Handle = ActiveHandles.Find(Key))
		{
			ASC->RemoveActiveGameplayEffect(*Handle);
			ActiveHandles.Remove(Key);
		}
	}

	SourceTagToKeys.Remove(SourceTag);
}
