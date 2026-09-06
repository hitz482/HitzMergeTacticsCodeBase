#include "Units/HMT_UnitActor.h"
#include "Stats/HMT_StatComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Net/UnrealNetwork.h"

AHMT_UnitActor::AHMT_UnitActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	StatComponent = CreateDefaultSubobject<UHMT_StatComponent>(TEXT("StatComponent"));
	UnitInstanceComponent = CreateDefaultSubobject<UHMT_UnitInstanceComponent>(TEXT("UnitInstanceComponent"));
	UnitInstanceComponent->OnStarLevelChanged.AddDynamic(this, &AHMT_UnitActor::HandleStarLevelChanged);
}

void AHMT_UnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHMT_UnitActor, ReplicatedUnitDefinition);
}

void AHMT_UnitActor::InitializeFromDefinition(UHMT_UnitDefinition* Definition, int32 StarLevel)
{
	if (!Definition)
	{
		return;
	}

	UnitInstanceComponent->InitializeUnit(Definition, StarLevel);

	ReplicatedUnitDefinition = Definition;
	ApplyMeshFromDefinition();
	OnUnitDefinitionApplied();
}

void AHMT_UnitActor::OnRep_UnitDefinition()
{
	ApplyMeshFromDefinition();
	OnUnitDefinitionApplied();
}

void AHMT_UnitActor::ApplyMeshFromDefinition()
{
	if (ReplicatedUnitDefinition && !ReplicatedUnitDefinition->Mesh.IsNull())
	{
		MeshComponent->SetSkeletalMesh(ReplicatedUnitDefinition->Mesh.LoadSynchronous());
	}
}

void AHMT_UnitActor::HandleStarLevelChanged(int32 OldStarLevel, int32 NewStarLevel)
{
	OnStarLevelChanged(OldStarLevel, NewStarLevel);
}

FTransform AHMT_UnitActor::GetSocketOrRootTransform(FName SocketName) const
{
	if (!SocketName.IsNone() && MeshComponent && MeshComponent->DoesSocketExist(SocketName))
	{
		return MeshComponent->GetSocketTransform(SocketName);
	}
	return GetActorTransform();
}

FText AHMT_UnitActor::GetStatSummaryText()
{
	if (!StatComponent)
	{
		return FText::GetEmpty();
	}

	TArray<FString> Lines;
	for (const FHMT_BaseStat& Stat : StatComponent->GetAllBaseStats())
	{
		FString StatName = Stat.StatTag.ToString();
		int32 LastDot = INDEX_NONE;
		if (StatName.FindLastChar(TEXT('.'), LastDot))
		{
			StatName.MidInline(LastDot + 1);
		}
		const float FinalValue = IHMT_StatProvider::Execute_GetFinalStatValue(StatComponent, Stat.StatTag);
		Lines.Add(FString::Printf(TEXT("%s  %.0f"), *StatName, FinalValue));
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}
