#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/HMT_CombatTypes.h"
#include "HMT_CombatPlaybackComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMT_OnCombatEventSignature, const FHMT_CombatEvent&, Event);

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_CombatPlaybackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_CombatPlaybackComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void SetCombatLog(const FHMT_CombatEventLog& NewLog, const FHMT_BoardSnapshot& InSideA, const FHMT_BoardSnapshot& InSideB, bool bInOwnerIsSideA);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	const TArray<FHMT_CombatEvent>& GetEvents() const { return EventLog.Events; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	const FHMT_BoardSnapshot& GetSideASnapshot() const { return SideASnapshot; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	const FHMT_BoardSnapshot& GetSideBSnapshot() const { return SideBSnapshot; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	bool IsOwnerSideA() const { return bOwnerIsSideA; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	float GetElapsedSinceCombatStart() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void GetLastCombatResult(bool& bOutHasResult, bool& bOutWon, bool& bOutDraw) const
	{
		bOutHasResult = false;
		bOutWon = false;
		bOutDraw = false;
		for (const FHMT_CombatEvent& Event : EventLog.Events)
		{
			if (Event.EventType == EHMT_CombatEventType::CombatEnd)
			{
				bOutHasResult = true;
				bOutDraw = Event.Magnitude != 0.f && Event.Magnitude != 1.f;
				bOutWon = !bOutDraw && ((Event.Magnitude == 0.f) == bOwnerIsSideA);
				return;
			}
		}
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void GetUnitDisplayInfoForInstance(FGuid InstanceId, bool& bOutFound, FText& OutDisplayName, int32& OutStarLevel, bool& bOutIsOwnerUnit) const
	{
		bOutFound = false;
		OutDisplayName = FText::GetEmpty();
		OutStarLevel = 0;
		bOutIsOwnerUnit = false;

		const FHMT_BoardSnapshot* Sides[2] = { &SideASnapshot, &SideBSnapshot };
		for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
		{
			for (const FHMT_SnapshotUnit& Unit : Sides[SideIndex]->Units)
			{
				if (Unit.InstanceId == InstanceId)
				{
					bOutFound = true;
					OutDisplayName = Unit.UnitDefinition ? Unit.UnitDefinition->DisplayName : FText::GetEmpty();
					OutStarLevel = Unit.StarLevel;
					bOutIsOwnerUnit = (SideIndex == 0) == bOwnerIsSideA;
					return;
				}
			}
		}
	}

	UPROPERTY(BlueprintAssignable, Category = "HitzMergeTactics|Combat")
	FHMT_OnCombatEventSignature OnCombatEvent;

	void NotifyCombatEventReceived(const FHMT_CombatEvent& Event);

protected:
	UPROPERTY(Replicated)
	FHMT_CombatEventLog EventLog;

	UPROPERTY(Replicated)
	FHMT_BoardSnapshot SideASnapshot;

	UPROPERTY(Replicated)
	FHMT_BoardSnapshot SideBSnapshot;

	UPROPERTY(Replicated)
	bool bOwnerIsSideA = true;

	UPROPERTY(Replicated)
	float CombatStartServerTime = 0.f;

	UFUNCTION(BlueprintImplementableEvent, Category = "HitzMergeTactics|Combat")
	void OnCombatEventReceived(const FHMT_CombatEvent& Event);
};
