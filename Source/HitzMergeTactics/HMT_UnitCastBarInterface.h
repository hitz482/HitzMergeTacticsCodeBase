#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HMT_UnitCastBarInterface.generated.h"

/**
 * The one C++ touchpoint a cast-bar widget needs — everything else (the progress bar, the text,
 * the colors, the layout) is pure Blueprint. Build your cast bar as an ordinary WBP (parent class
 * UUserWidget), implement this interface (Class Settings -> Implemented Interfaces), and handle
 * SetCastProgress in the Event Graph however you like — set a progress bar's percent, whatever.
 *
 * UHMT_CombatPlaybackDriver calls this on whatever widget class you assign to
 * CastBarWidgetClass every frame while a unit is channeling (see UHMT_CombatResolver's optional
 * CastTimeStatTag); it never knows or cares about your widget's internal layout. It also controls
 * the widget's world-space visibility directly (shown for the duration of a cast, hidden the rest
 * of the time) — SetCastProgress only ever fires while the bar is meant to be visible.
 */
UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_UnitCastBarInterface : public UInterface
{
	GENERATED_BODY()
};

class IHMT_UnitCastBarInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "HMT Sample")
	void SetCastProgress(float Fraction);
};
