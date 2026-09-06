#include "HMT_CoreEditor.h"
#include "Customization/HMT_BoardLayoutAssetCustomization.h"
#include "Customization/HMT_UnitDefinitionCustomization.h"
#include "AssetEditor/AssetTypeActions_HMT_UnitDefinition.h"
#include "Grid/HMT_BoardLayoutAsset.h"
#include "Units/HMT_UnitDefinition.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FHMT_CoreEditorModule"

void FHMT_CoreEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UHMT_BoardLayoutAsset::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FHMT_BoardLayoutAssetCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(
		UHMT_UnitDefinition::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FHMT_UnitDefinitionCustomization::MakeInstance));

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UnitDefinitionAssetTypeActions = MakeShared<FAssetTypeActions_HMT_UnitDefinition>();
	AssetTools.RegisterAssetTypeActions(UnitDefinitionAssetTypeActions.ToSharedRef());
}

void FHMT_CoreEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UHMT_BoardLayoutAsset::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(UHMT_UnitDefinition::StaticClass()->GetFName());
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools") && UnitDefinitionAssetTypeActions.IsValid())
	{
		FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get().UnregisterAssetTypeActions(UnitDefinitionAssetTypeActions.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHMT_CoreEditorModule, HMT_CoreEditor)
