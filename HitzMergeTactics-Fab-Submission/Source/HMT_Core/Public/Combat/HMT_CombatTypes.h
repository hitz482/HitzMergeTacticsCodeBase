#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Grid/HMT_GridTypes.h"
#include "Stats/HMT_StatTypes.h"
#include "Units/HMT_UnitDefinition.h"
#include "HMT_CombatTypes.generated.h"

class UHMT_CombatPlaybackComponent;
class APlayerState;

UENUM(BlueprintType)
enum class EHMT_CombatEventType : uint8
{
	AttackStart,
	DamageDealt,
	AbilityCast,
	UnitMoved,
	UnitDeath,
	CombatEnd,
	StatusApplied,
	StatusExpired,
	UnitSpawned,
	CastStart,
	CastInterrupted,
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_CombatEvent : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	EHMT_CombatEventType EventType = EHMT_CombatEventType::AttackStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float Timestamp = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGuid SourceInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGuid TargetInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float Magnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FHMT_GridCoord FromCoord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FHMT_GridCoord ToCoord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float MoveDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	bool bIsForcedDisplacement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	bool bIsCritical = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float ProjectileTravelTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat", meta = (Categories = "Status"))
	FGameplayTag EffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TObjectPtr<UHMT_UnitDefinition> SpawnedUnitDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	int32 SpawnedStarLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float CastDuration = 0.f;

	void PostReplicatedAdd(const struct FHMT_CombatEventLog& InArraySerializer);
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_CombatEventLog : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FHMT_CombatEvent> Events;

	TWeakObjectPtr<UHMT_CombatPlaybackComponent> OwningComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FHMT_CombatEvent, FHMT_CombatEventLog>(Events, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FHMT_CombatEventLog> : public TStructOpsTypeTraitsBase2<FHMT_CombatEventLog>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_SnapshotUnit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FHMT_GridCoord Coord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TObjectPtr<UHMT_UnitDefinition> UnitDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	int32 StarLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TArray<FHMT_StatModifier> ActiveModifiers;
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_TileModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FHMT_GridCoord Coord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TSubclassOf<UObject> TileEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TObjectPtr<class UHMT_TileModifierDefinition> Definition = nullptr;
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_BoardSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TArray<FHMT_SnapshotUnit> Units;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	EHMT_GridTopology Topology = EHMT_GridTopology::Hex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	int32 Columns = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	int32 Rows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TArray<TSubclassOf<UObject>> ActiveTraitEffectClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TObjectPtr<APlayerState> OwningPlayer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TSubclassOf<UObject> ActiveRulerPassiveClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TArray<TSubclassOf<UObject>> ActiveModifierEffectClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TArray<FHMT_TileModifier> TileModifiers;
};

namespace HMT_CombatArena
{
	inline int32 GetArenaColumns(const FHMT_BoardSnapshot& SideA, const FHMT_BoardSnapshot& SideB)
	{
		return FMath::Max(SideA.Columns, SideB.Columns);
	}

	inline int32 GetArenaRows(const FHMT_BoardSnapshot& SideA, const FHMT_BoardSnapshot& SideB)
	{
		return SideA.Rows + SideB.Rows;
	}

	inline FHMT_GridCoord MirrorAcrossArena(const FHMT_GridCoord& Coord, int32 ArenaColumns, int32 ArenaRows)
	{
		return FHMT_GridCoord(ArenaColumns - 1 - Coord.Q, ArenaRows - 1 - Coord.R);
	}
}
