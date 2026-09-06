#include "Customization/HMT_UnitDefinitionCustomization.h"
#include "Units/HMT_UnitDefinition.h"
#include "Units/HMT_UnitProjectileSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "HMT_UnitDefinitionCustomization"

namespace
{
	TArray<FName> GetSocketNamesForDefinition(const UHMT_UnitDefinition* Definition)
	{
		TArray<FName> Result;
		Result.Add(NAME_None);

		USkeletalMesh* Mesh = Definition && !Definition->Mesh.IsNull() ? Definition->Mesh.LoadSynchronous() : nullptr;
		if (!Mesh)
		{
			return Result;
		}

		USkeletalMeshComponent* ProbeComponent = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
		ProbeComponent->SetSkeletalMesh(Mesh);
		Result.Append(ProbeComponent->GetAllSocketNames());
		return Result;
	}
}

TSharedRef<IDetailCustomization> FHMT_UnitDefinitionCustomization::MakeInstance()
{
	return MakeShared<FHMT_UnitDefinitionCustomization>();
}

void FHMT_UnitDefinitionCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		return;
	}

	UnitDefinitionWeak = Cast<UHMT_UnitDefinition>(Objects[0].Get());
	if (!UnitDefinitionWeak.IsValid())
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> MergeFXSocketHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UHMT_UnitDefinition, MergeFXSocketName));
	DetailBuilder.HideProperty(MergeFXSocketHandle);
	DetailBuilder.EditCategory("HitzMergeTactics|Units|Merge Feedback")
		.AddCustomRow(LOCTEXT("MergeFXSocketRow", "Merge FX Socket Name"))
		.NameContent()
		[
			MergeFXSocketHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			BuildSocketPicker(MergeFXSocketHandle)
		];

	const TSharedPtr<IPropertyHandle> ProjectileSetHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UHMT_UnitDefinition, ProjectileSet));
	DetailBuilder.HideProperty(ProjectileSetHandle);
	IDetailCategoryBuilder& CombatCategory = DetailBuilder.EditCategory("HitzMergeTactics|Combat");

	if (const TSharedPtr<IPropertyHandle> ClassHandle = ProjectileSetHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHMT_UnitProjectileSet, ProjectileClass)))
	{
		CombatCategory.AddProperty(ClassHandle);
	}
	if (const TSharedPtr<IPropertyHandle> MuzzleHandle = ProjectileSetHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHMT_UnitProjectileSet, MuzzleSocketName)))
	{
		CombatCategory.AddCustomRow(LOCTEXT("MuzzleSocketRow", "Muzzle Socket Name"))
			.NameContent()
			[
				MuzzleHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				BuildSocketPicker(MuzzleHandle)
			];
	}
	if (const TSharedPtr<IPropertyHandle> TargetHandle = ProjectileSetHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHMT_UnitProjectileSet, TargetSocketName)))
	{
		CombatCategory.AddCustomRow(LOCTEXT("TargetSocketRow", "Target Socket Name"))
			.NameContent()
			[
				TargetHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				BuildSocketPicker(TargetHandle)
			];
	}
}

TSharedRef<SWidget> FHMT_UnitDefinitionCustomization::BuildSocketPicker(TSharedPtr<IPropertyHandle> NameHandle)
{
	return SNew(SComboButton)
		.OnGetMenuContent(this, &FHMT_UnitDefinitionCustomization::BuildSocketMenu, NameHandle)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text_Lambda([NameHandle]()
			{
				FName Current;
				NameHandle->GetValue(Current);
				return Current.IsNone() ? LOCTEXT("NoSocket", "<None>") : FText::FromName(Current);
			})
		];
}

TSharedRef<SWidget> FHMT_UnitDefinitionCustomization::BuildSocketMenu(TSharedPtr<IPropertyHandle> NameHandle)
{
	FMenuBuilder MenuBuilder( true, nullptr);

	for (const FName& SocketName : GetSocketNamesForDefinition(UnitDefinitionWeak.Get()))
	{
		MenuBuilder.AddMenuEntry(
			SocketName.IsNone() ? LOCTEXT("NoSocket", "<None>") : FText::FromName(SocketName),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([NameHandle, SocketName]() { NameHandle->SetValue(SocketName); })));
	}

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
