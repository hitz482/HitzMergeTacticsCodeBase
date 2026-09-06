#include "Ruler/HMT_RulerActor.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UnrealType.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AHMT_RulerActor::AHMT_RulerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	SetReplicates(true);
}

void AHMT_RulerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!OverheadWidgetComponent)
	{
		return;
	}
	const APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* CameraManager = LocalController ? LocalController->PlayerCameraManager : nullptr;
	if (CameraManager)
	{
		const FVector CameraLocation = CameraManager->GetCameraLocation();
		OverheadWidgetComponent->SetWorldRotation((CameraLocation - OverheadWidgetComponent->GetComponentLocation()).Rotation());
	}
}

void AHMT_RulerActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHMT_RulerActor, ReplicatedRulerDefinition);
	DOREPLIFETIME(AHMT_RulerActor, CurrentPose);
	DOREPLIFETIME(AHMT_RulerActor, CurrentHP);
	DOREPLIFETIME(AHMT_RulerActor, MaxHP);
}

void AHMT_RulerActor::InitializeFromDefinition(UHMT_RulerDefinition* Definition)
{
	ReplicatedRulerDefinition = Definition;
	ApplyMeshFromDefinition();
	CurrentPose = EHMT_RulerPose::Idle;
	PlayPoseAnim();
	EnsureOverheadWidgetSpawned();
}

void AHMT_RulerActor::OnRep_RulerDefinition()
{
	ApplyMeshFromDefinition();
	PlayPoseAnim();
	EnsureOverheadWidgetSpawned();
}

void AHMT_RulerActor::ApplyMeshFromDefinition()
{
	if (ReplicatedRulerDefinition && !ReplicatedRulerDefinition->Mesh.IsNull())
	{
		MeshComponent->SetSkeletalMesh(ReplicatedRulerDefinition->Mesh.LoadSynchronous());
	}
}

void AHMT_RulerActor::SetPose(EHMT_RulerPose NewPose)
{
	CurrentPose = NewPose;
	PlayPoseAnim();
}

void AHMT_RulerActor::OnRep_Pose()
{
	PlayPoseAnim();
}

void AHMT_RulerActor::SetHealth(int32 InCurrentHP, int32 InMaxHP)
{
	CurrentHP = InCurrentHP;
	MaxHP = InMaxHP;
}

void AHMT_RulerActor::OnRep_Health()
{
}

void AHMT_RulerActor::EnsureOverheadWidgetSpawned()
{
	if (OverheadWidgetComponent || !OverheadWidgetClass)
	{
		return;
	}

	OverheadWidgetComponent = NewObject<UWidgetComponent>(this, TEXT("OverheadWidgetComponent"));
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	OverheadWidgetComponent->SetDrawSize(FVector2D(160.f, 60.f));
	OverheadWidgetComponent->SetWidgetClass(OverheadWidgetClass);
	OverheadWidgetComponent->SetupAttachment(MeshComponent);

	float MeshBoundsTopZ = 190.f;
	if (const USkeletalMesh* Mesh = MeshComponent->GetSkeletalMeshAsset())
	{
		const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
		MeshBoundsTopZ = MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z;
	}
	OverheadWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, MeshBoundsTopZ + 25.f));
	OverheadWidgetComponent->RegisterComponent();
	SetActorTickEnabled(true);

	if (UUserWidget* Widget = OverheadWidgetComponent->GetUserWidgetObject())
	{
		if (FObjectProperty* OwnerProp = FindFProperty<FObjectProperty>(Widget->GetClass(), TEXT("OwningRulerActor")))
		{
			OwnerProp->SetObjectPropertyValue_InContainer(Widget, this);
		}
	}
}

void AHMT_RulerActor::SetLocalVisibility(bool bVisible)
{
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(bVisible, true);
	}
}

void AHMT_RulerActor::PlayPoseAnim()
{
	if (!ReplicatedRulerDefinition)
	{
		return;
	}

	UAnimMontage* Anim = nullptr;
	bool bLoop = false;
	switch (CurrentPose)
	{
	case EHMT_RulerPose::Idle:
		Anim = ReplicatedRulerDefinition->AnimSet.Idle;
		bLoop = true;
		break;
	case EHMT_RulerPose::Victory:
		Anim = ReplicatedRulerDefinition->AnimSet.Victory ? ReplicatedRulerDefinition->AnimSet.Victory.Get() : ReplicatedRulerDefinition->AnimSet.Idle.Get();
		bLoop = true;
		break;
	case EHMT_RulerPose::Defeat:
		Anim = ReplicatedRulerDefinition->AnimSet.Death;
		bLoop = false;
		break;
	}

	if (Anim)
	{
		MeshComponent->PlayAnimation(Anim, bLoop);
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT_RulerActor] %s pose -> %s (%s)"), *GetName(),
		*UEnum::GetValueAsString(CurrentPose), Anim ? *Anim->GetName() : TEXT("no anim authored"));
}
