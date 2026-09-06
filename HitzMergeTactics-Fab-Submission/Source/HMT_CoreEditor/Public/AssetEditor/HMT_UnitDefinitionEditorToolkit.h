#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UHMT_UnitDefinition;
class SHMT_UnitDefinitionPreviewViewport;
class IDetailsView;
class UAnimMontage;

class FHMT_UnitDefinitionEditorToolkit : public FAssetEditorToolkit
{
public:
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UHMT_UnitDefinition* Asset);

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	UAnimMontage* GetAnim(int32 SlotIndex) const;
	void PlayAnimSlot(int32 SlotIndex);

	TObjectPtr<UHMT_UnitDefinition> UnitDefinition;
	TSharedPtr<SHMT_UnitDefinitionPreviewViewport> Viewport;
	TSharedPtr<IDetailsView> DetailsView;

	static const FName ViewportTabId;
	static const FName DetailsTabId;
};
