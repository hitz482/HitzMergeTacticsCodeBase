#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/HMT_GridTypes.h"
#include "Grid/IHMT_GridLayout.h"
#include "Grid/HMT_BoardLayoutAsset.h"
#include "Combat/HMT_CombatTypes.h"
#include "HMT_BoardComponent.generated.h"

class UHMT_TileModifierDefinition;

USTRUCT()
struct FHMT_BoardCell
{
	GENERATED_BODY()

	UPROPERTY()
	FHMT_GridCoord Coord;

	UPROPERTY()
	TObjectPtr<AActor> Occupant = nullptr;
};

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_BoardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_BoardComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	void InitializeBoard(UHMT_BoardLayoutAsset* Layout);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	bool IsValidCoord(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	bool IsCellOccupied(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	AActor* GetOccupantAt(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	TArray<AActor*> GetAllOccupants() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	bool PlaceUnit(AActor* Unit, FHMT_GridCoord Coord);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	bool RemoveUnitAt(FHMT_GridCoord Coord);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	bool MoveOrSwapUnit(FHMT_GridCoord From, FHMT_GridCoord To);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	bool RemoveOccupant(AActor* Unit);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	FHMT_GridCoord GetCoordOfOccupant(AActor* Unit, bool& bOutFound) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	FHMT_GridCoord GetNextStepToward(FHMT_GridCoord From, FHMT_GridCoord Target) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	FVector GridToWorld(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	FHMT_GridCoord WorldToGrid(FVector WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	int32 GetDistance(FHMT_GridCoord A, FHMT_GridCoord B) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	TArray<FHMT_GridCoord> GetNeighbors(FHMT_GridCoord Coord) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	EHMT_GridTopology GetTopology() const { return Topology; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	float GetCellSize() const { return CellSize; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	int32 GetColumns() const { return BoardColumns; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	int32 GetRows() const { return BoardRows; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board|Tiles")
	void ApplyTileModifier(FHMT_GridCoord Coord, UHMT_TileModifierDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board|Tiles")
	void ClearTileModifierAt(FHMT_GridCoord Coord);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board|Tiles")
	void ClearAllTileModifiers() { ActiveTileModifiers.Reset(); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board|Tiles")
	bool GetTileModifierAt(FHMT_GridCoord Coord, FHMT_TileModifier& OutModifier) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board|Tiles")
	TArray<FHMT_TileModifier> GetActiveTileModifiers() const { return ActiveTileModifiers; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	void GrantBonusUnitCapacity(int32 Delta) { BonusUnitCapacity = FMath::Max(0, BonusUnitCapacity + Delta); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Board")
	int32 GetBonusUnitCapacity() const { return BonusUnitCapacity; }

protected:
	UPROPERTY(Replicated)
	EHMT_GridTopology Topology = EHMT_GridTopology::Hex;

	UPROPERTY(Replicated)
	float CellSize = 100.f;

	UPROPERTY(Replicated)
	TArray<FHMT_BoardCell> Cells;

	UPROPERTY(Replicated)
	int32 BoardColumns = 7;

	UPROPERTY(Replicated)
	int32 BoardRows = 4;

	UPROPERTY(Replicated)
	TArray<FHMT_GridCoord> BlockedCells;

	UPROPERTY(Replicated)
	TArray<FHMT_TileModifier> ActiveTileModifiers;

	UPROPERTY(Replicated)
	int32 BonusUnitCapacity = 0;

	UPROPERTY()
	TScriptInterface<IHMT_GridLayout> GridLayout;

private:
	int32 FindCellIndex(FHMT_GridCoord Coord) const;
};
