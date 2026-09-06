#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Grid/HMT_GridTypes.h"
#include "Grid/IHMT_GridLayout.h"
#include "Combat/HMT_CombatTypes.h"
#include "HMT_CombatContext.generated.h"

class UHMT_CombatResolver;
class AHMT_CombatUnitActor;
class UHMT_UnitDefinition;

struct FHMT_ResolverUnitState
{
	TWeakObjectPtr<AHMT_CombatUnitActor> Actor;
	FHMT_GridCoord Coord;
	float CurrentHealth = 0.f;
	int32 Side = 0;
	FGuid InstanceId;
	TWeakObjectPtr<UObject> TargetingStrategy;
	TWeakObjectPtr<UObject> AbilityExecutor;
	TWeakObjectPtr<AHMT_CombatUnitActor> CurrentTarget;
	float NextAttackTime = 0.f;

	float BusyUntilTime = 0.f;

	bool bIsCasting = false;
	float CastEndTime = 0.f;

	TMap<FGameplayTag, float> ActiveStatusEffects;

	TWeakObjectPtr<AHMT_CombatUnitActor> TauntedBy;

	bool IsAlive() const { return CurrentHealth > 0.f; }
	bool HasStatus(const FGameplayTag& Tag) const { return ActiveStatusEffects.Contains(Tag); }
};

UCLASS(BlueprintType)
class HMT_CORE_API UHMT_CombatContext : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	TArray<AActor*> GetEnemies(AActor* SelfActor) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	TArray<AActor*> GetAllies(AActor* SelfActor) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	FHMT_GridCoord GetUnitCoord(AActor* Unit, bool& bOutFound) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	int32 GetDistanceBetween(AActor* UnitA, AActor* UnitB) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	TArray<AActor*> GetUnitsInRangeOf(AActor* CenterUnit, int32 Range, bool bEnemiesOfCenterOnly = true) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	float GetCurrentHealth(AActor* Unit) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	float GetMaxHealth(AActor* Unit) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	AActor* FindUnitByInstanceId(FGuid InstanceId) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	int32 GetUnitSide(AActor* Unit) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void DealDamage(AActor* SourceUnit, AActor* TargetUnit, float Damage, bool bApplyArmor = true);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void Heal(AActor* SourceUnit, AActor* TargetUnit, float Amount);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	bool MoveUnit(AActor* Unit, FHMT_GridCoord ToCoord);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void ApplyStatusEffect(AActor* SourceUnit, AActor* TargetUnit, FGameplayTag EffectTag, float DurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void RemoveStatusEffect(AActor* Unit, FGameplayTag EffectTag);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	bool HasStatusEffect(AActor* Unit, FGameplayTag EffectTag) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	AActor* SpawnUnit(AActor* Summoner, UHMT_UnitDefinition* Definition, int32 StarLevel, FHMT_GridCoord Coord);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	bool IsCellFree(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	TArray<FHMT_GridCoord> GetNeighborCoords(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	int32 GetGridDistance(FHMT_GridCoord CoordA, FHMT_GridCoord CoordB) const;

	TArray<FHMT_ResolverUnitState>* Units = nullptr;
	UPROPERTY()
	TObjectPtr<UObject> GridLayoutObject = nullptr;
	UPROPERTY()
	TObjectPtr<UHMT_CombatResolver> Resolver = nullptr;
	UPROPERTY()
	TObjectPtr<UWorld> World = nullptr;
	TFunction<void(const FHMT_CombatEvent&)> EmitEvent;
	float CurrentSimTime = 0.f;
	int32 ArenaColumns = 0;
	int32 ArenaRows = 0;

	TArray<FHMT_ResolverUnitState> PendingSpawns;

private:
	FHMT_ResolverUnitState* FindState(AActor* Unit) const;
};
