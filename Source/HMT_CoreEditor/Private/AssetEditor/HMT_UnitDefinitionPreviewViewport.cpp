#include "AssetEditor/HMT_UnitDefinitionPreviewViewport.h"
#include "Units/HMT_UnitDefinition.h"
#include "PreviewScene.h"
#include "EditorViewportClient.h"
#include "EditorModeManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimMontage.h"

void SHMT_UnitDefinitionPreviewViewport::Construct(const FArguments& InArgs)
{
	PreviewScene = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues());
	ModeTools = MakeShared<FEditorModeTools>();

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

void SHMT_UnitDefinitionPreviewViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (PreviewMeshComponent && !bIsScrubbing)
	{
		PreviewMeshComponent->TickAnimation(InDeltaTime, false);
		PreviewMeshComponent->RefreshBoneTransforms();
		PreviewMeshComponent->FinalizeBoneTransform();
	}
}

void SHMT_UnitDefinitionPreviewViewport::SetUnitDefinition(UHMT_UnitDefinition* InUnitDefinition)
{
	UnitDefinition = InUnitDefinition;

	if (!PreviewMeshComponent)
	{
		PreviewMeshComponent = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
		PreviewScene->AddComponent(PreviewMeshComponent, FTransform::Identity);
	}

	USkeletalMesh* Mesh = InUnitDefinition && !InUnitDefinition->Mesh.IsNull() ? InUnitDefinition->Mesh.LoadSynchronous() : nullptr;
	PreviewMeshComponent->SetSkeletalMesh(Mesh);

	if (InUnitDefinition)
	{
		PlayPreviewAnimation(InUnitDefinition->AnimSet.Idle, true);
	}
}

void SHMT_UnitDefinitionPreviewViewport::PlayPreviewAnimation(UAnimMontage* Anim, bool bLoop)
{
	if (!Anim || !PreviewMeshComponent)
	{
		return;
	}

	CurrentAnim = Anim;
	bIsScrubbing = false;
	PreviewMeshComponent->PlayAnimation(Anim, bLoop);
}

float SHMT_UnitDefinitionPreviewViewport::GetScrubMax() const
{
	return CurrentAnim.IsValid() ? CurrentAnim->GetPlayLength() : 0.f;
}

float SHMT_UnitDefinitionPreviewViewport::GetScrubPosition() const
{
	return PreviewMeshComponent ? PreviewMeshComponent->GetPosition() : 0.f;
}

void SHMT_UnitDefinitionPreviewViewport::SetScrubPosition(float NewPosition)
{
	if (PreviewMeshComponent)
	{
		PreviewMeshComponent->SetPosition(NewPosition,  false);
	}
}

void SHMT_UnitDefinitionPreviewViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PreviewMeshComponent);
}

TSharedRef<FEditorViewportClient> SHMT_UnitDefinitionPreviewViewport::MakeEditorViewportClient()
{
	TSharedRef<FEditorViewportClient> NewClient = MakeShared<FEditorViewportClient>(ModeTools.Get(), PreviewScene.Get(), SharedThis(this));
	NewClient->SetViewportType(LVT_Perspective);
	NewClient->SetRealtime(true);
	NewClient->EngineShowFlags.DisableAdvancedFeatures();
	NewClient->EngineShowFlags.SetLighting(true);
	NewClient->EngineShowFlags.SetMaterials(true);
	NewClient->SetViewLocationForOrbiting(FVector(0.f, 0.f, 100.f), 400.f);
	NewClient->SetShowStats(false);
	return NewClient;
}
