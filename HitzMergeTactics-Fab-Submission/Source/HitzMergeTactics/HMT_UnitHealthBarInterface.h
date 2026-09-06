#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HMT_UnitHealthBarInterface.generated.h"

/**
 * The one C++ touchpoint a health-bar widget needs — everything else (the progress bar, the text,
 * the colors, the layout) is pure Blueprint. Build your health bar as an ordinary WBP (parent
 * class UUserWidget), implement this interface (Class Settings -> Implemented Interfaces), and
 * handle SetHealthFraction in the Event Graph however you like — set a progress bar's percent,
 * tint it red/green off bIsEnemy, whatever.
 *
 * UHMT_CombatPlaybackDriver calls this on whatever widget class you assign to
 * HealthBarWidgetClass; it never knows or cares about your widget's internal layout.
 */
UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_UnitHealthBarInterface : public UInterface
{
	GENERATED_BODY()
};

class IHMT_UnitHealthBarInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "HMT Sample")
	void SetHealthFraction(float CurrentHealth, float MaxHealth, bool bIsEnemy);
};
