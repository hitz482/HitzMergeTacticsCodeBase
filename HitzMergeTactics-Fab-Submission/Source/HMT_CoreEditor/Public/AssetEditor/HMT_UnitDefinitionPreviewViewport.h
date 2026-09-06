#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "UObject/GCObject.h"

class UHMT_UnitDefinition;
class USkeletalMeshComponent;
class UAnimMontage;
class FPreviewScene;
class FEditorModeTools;

class SHMT_UnitDefinitionPreviewViewport : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SHMT_UnitDefinitionPreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void SetUnitDefinition(UHMT_UnitDefinition* InUnitDefinition);

	void PlayPreviewAnimation(UAnimMontage* Anim, bool bLoop);

	float GetScrubMax() const;

	float GetScrubPosition() const;

	void SetScrubPosition(float NewPosition);

	void SetIsScrubbing(bool bInIsScrubbing) { bIsScrubbing = bInIsScrubbing; }

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SHMT_UnitDefinitionPreviewViewport"); }

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	TWeakObjectPtr<UHMT_UnitDefinition> UnitDefinition;
	TSharedPtr<FPreviewScene> PreviewScene;
	TSharedPtr<FEditorModeTools> ModeTools;
	TObjectPtr<USkeletalMeshComponent> PreviewMeshComponent;

	TWeakObjectPtr<UAnimMontage> CurrentAnim;

	bool bIsScrubbing = false;
};
