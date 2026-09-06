#include "AssetEditor/HMT_UnitDefinitionEditorToolkit.h"
#include "AssetEditor/HMT_UnitDefinitionPreviewViewport.h"
#include "Units/HMT_UnitDefinition.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "HMT_UnitDefinitionEditor"

const FName FHMT_UnitDefinitionEditorToolkit::ViewportTabId(TEXT("HMT_UnitDefinitionEditor_Viewport"));
const FName FHMT_UnitDefinitionEditorToolkit::DetailsTabId(TEXT("HMT_UnitDefinitionEditor_Details"));

void FHMT_UnitDefinitionEditorToolkit::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UHMT_UnitDefinition* Asset)
{
	UnitDefinition = Asset;

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("HMT_UnitDefinitionEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, TEXT("HMT_UnitDefinitionEditor"), Layout,  true,  true, Asset);
}

void FHMT_UnitDefinitionEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FHMT_UnitDefinitionEditorToolkit::SpawnViewportTab))
		.SetDisplayName(LOCTEXT("ViewportTab", "Preview"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FHMT_UnitDefinitionEditorToolkit::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"));
}

void FHMT_UnitDefinitionEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

TSharedRef<SDockTab> FHMT_UnitDefinitionEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	SAssignNew(Viewport, SHMT_UnitDefinitionPreviewViewport);
	Viewport->SetUnitDefinition(UnitDefinition);

	auto AnimButton = [this](const FText& Label, int32 SlotIndex)
	{
		return SNew(SButton)
			.Text(Label)
			.OnClicked_Lambda([this, SlotIndex]() { PlayAnimSlot(SlotIndex); return FReply::Handled(); });
	};

	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTabLabel", "Preview"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f) [ AnimButton(LOCTEXT("Idle", "Idle"), 0) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f) [ AnimButton(LOCTEXT("Move", "Move"), 1) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f) [ AnimButton(LOCTEXT("Attack", "Attack"), 2) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f) [ AnimButton(LOCTEXT("Ability", "Ability"), 3) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f) [ AnimButton(LOCTEXT("Death", "Death"), 4) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f) [ AnimButton(LOCTEXT("Stagger", "Stagger"), 5) ]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				Viewport.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SSlider)
				.Value_Lambda([this]()
				{
					const float Max = Viewport.IsValid() ? Viewport->GetScrubMax() : 0.f;
					return (Viewport.IsValid() && Max > 0.f) ? (Viewport->GetScrubPosition() / Max) : 0.f;
				})
				.OnMouseCaptureBegin_Lambda([this]() { if (Viewport.IsValid()) { Viewport->SetIsScrubbing(true); } })
				.OnMouseCaptureEnd_Lambda([this]() { if (Viewport.IsValid()) { Viewport->SetIsScrubbing(false); } })
				.OnValueChanged_Lambda([this](float NewValue)
				{
					if (Viewport.IsValid())
					{
						Viewport->SetScrubPosition(NewValue * Viewport->GetScrubMax());
					}
				})
			]
		];
}

TSharedRef<SDockTab> FHMT_UnitDefinitionEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(UnitDefinition);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

UAnimMontage* FHMT_UnitDefinitionEditorToolkit::GetAnim(int32 SlotIndex) const
{
	if (!UnitDefinition)
	{
		return nullptr;
	}

	switch (SlotIndex)
	{
	case 0: return UnitDefinition->AnimSet.Idle;
	case 1: return UnitDefinition->AnimSet.Move;
	case 2: return UnitDefinition->AnimSet.Attack;
	case 3: return UnitDefinition->AnimSet.Ability;
	case 4: return UnitDefinition->AnimSet.Death;
	case 5: return UnitDefinition->AnimSet.Stagger;
	default: return nullptr;
	}
}

void FHMT_UnitDefinitionEditorToolkit::PlayAnimSlot(int32 SlotIndex)
{
	if (Viewport.IsValid())
	{
		Viewport->PlayPreviewAnimation(GetAnim(SlotIndex), SlotIndex == 0 || SlotIndex == 1);
	}
}

FName FHMT_UnitDefinitionEditorToolkit::GetToolkitFName() const
{
	return FName("HMT_UnitDefinitionEditor");
}

FText FHMT_UnitDefinitionEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Unit Definition Editor");
}

FString FHMT_UnitDefinitionEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("HMT_UnitDefinition ");
}

FLinearColor FHMT_UnitDefinitionEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.2f, 0.6f, 0.9f, 0.5f);
}

#undef LOCTEXT_NAMESPACE
