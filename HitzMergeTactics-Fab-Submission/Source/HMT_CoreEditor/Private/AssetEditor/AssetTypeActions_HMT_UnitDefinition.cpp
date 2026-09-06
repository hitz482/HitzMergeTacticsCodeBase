#include "AssetEditor/AssetTypeActions_HMT_UnitDefinition.h"
#include "AssetEditor/HMT_UnitDefinitionEditorToolkit.h"
#include "Units/HMT_UnitDefinition.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_HMT_UnitDefinition"

FText FAssetTypeActions_HMT_UnitDefinition::GetName() const
{
	return LOCTEXT("Name", "HMT Unit Definition");
}

UClass* FAssetTypeActions_HMT_UnitDefinition::GetSupportedClass() const
{
	return UHMT_UnitDefinition::StaticClass();
}

FColor FAssetTypeActions_HMT_UnitDefinition::GetTypeColor() const
{
	return FColor(240, 166, 58);
}

uint32 FAssetTypeActions_HMT_UnitDefinition::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

void FAssetTypeActions_HMT_UnitDefinition::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UHMT_UnitDefinition* UnitDefinition = Cast<UHMT_UnitDefinition>(Object))
		{
			TSharedRef<FHMT_UnitDefinitionEditorToolkit> Toolkit = MakeShared<FHMT_UnitDefinitionEditorToolkit>();
			Toolkit->InitEditor(Mode, EditWithinLevelEditor, UnitDefinition);
		}
	}
}

#undef LOCTEXT_NAMESPACE
