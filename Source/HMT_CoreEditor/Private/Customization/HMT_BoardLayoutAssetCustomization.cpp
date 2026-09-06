#include "Customization/HMT_BoardLayoutAssetCustomization.h"
#include "Grid/HMT_BoardLayoutAsset.h"
#include "Grid/HMT_HexGridLayout.h"
#include "Grid/HMT_SquareGridLayout.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Grid/HMT_TileMeshUtils.h"
#include "ProceduralMeshComponent.h"

namespace
{
	const FName HMT_BoardPreviewTag(TEXT("HMT_BoardLayoutPreview"));
}

TSharedRef<IDetailCustomization> FHMT_BoardLayoutAssetCustomization::MakeInstance()
{
	return MakeShared<FHMT_BoardLayoutAssetCustomization>();
}

void FHMT_BoardLayoutAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedDetailBuilder = &DetailBuilder;

	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		return;
	}

	BoardWeak = Cast<UHMT_BoardLayoutAsset>(Objects[0].Get());
	if (!BoardWeak.IsValid())
	{
		return;
	}

	BlockedCellsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UHMT_BoardLayoutAsset, BlockedCells));
	DetailBuilder.HideProperty(BlockedCellsHandle);

	const TSharedPtr<IPropertyHandle> ColumnsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UHMT_BoardLayoutAsset, Columns));
	const TSharedPtr<IPropertyHandle> RowsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UHMT_BoardLayoutAsset, Rows));
	if (ColumnsHandle.IsValid())
	{
		ColumnsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FHMT_BoardLayoutAssetCustomization::OnDimensionsChanged));
	}
	if (RowsHandle.IsValid())
	{
		RowsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FHMT_BoardLayoutAssetCustomization::OnDimensionsChanged));
	}

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("HitzMergeTactics|Board");
	Category.AddCustomRow(NSLOCTEXT("HMT", "BoardPainterRow", "Board Painter"))
		.WholeRowContent()
		[
			BuildGrid()
		];

	Category.AddCustomRow(NSLOCTEXT("HMT", "BoardPreviewRow", "Board Preview"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("HMT", "PreviewBoard", "Preview Board In Level"))
				.ToolTipText(NSLOCTEXT("HMT", "PreviewBoardTip", "Spawns transient tile actors (never saved with the map) at the current level origin using this asset's TileMesh/materials and topology."))
				.OnClicked(this, &FHMT_BoardLayoutAssetCustomization::OnPreviewClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("HMT", "ClearPreview", "Clear Preview"))
				.OnClicked(this, &FHMT_BoardLayoutAssetCustomization::OnClearPreviewClicked)
			]
		];
}

void FHMT_BoardLayoutAssetCustomization::ClearPreviewActors() const
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(HMT_BoardPreviewTag))
		{
			World->DestroyActor(*It);
		}
	}
}

FReply FHMT_BoardLayoutAssetCustomization::OnClearPreviewClicked()
{
	ClearPreviewActors();
	return FReply::Handled();
}

FReply FHMT_BoardLayoutAssetCustomization::OnPreviewClicked()
{
	UHMT_BoardLayoutAsset* Board = BoardWeak.Get();
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!Board || !World)
	{
		return FReply::Handled();
	}

	ClearPreviewActors();

	UObject* LayoutObject = nullptr;
	if (Board->Topology == EHMT_GridTopology::Hex)
	{
		UHMT_HexGridLayout* Hex = NewObject<UHMT_HexGridLayout>(GetTransientPackage());
		Hex->CellSize = Board->CellSize;
		LayoutObject = Hex;
	}
	else
	{
		UHMT_SquareGridLayout* Square = NewObject<UHMT_SquareGridLayout>(GetTransientPackage());
		Square->CellSize = Board->CellSize;
		LayoutObject = Square;
	}

	float Scale = (Board->CellSize / 100.f) * Board->TileVisualScale;
	if (Board->TileMesh)
	{
		const FVector MeshExtent = Board->TileMesh->GetBounds().BoxExtent;
		const float MeshFootprint = FMath::Max(MeshExtent.X, MeshExtent.Y) * 2.f;
		if (MeshFootprint > KINDA_SMALL_NUMBER)
		{
			Scale = (Board->CellSize / MeshFootprint) * Board->TileVisualScale;
		}
	}
	int32 SpawnedCount = 0;

	for (int32 Column = 0; Column < Board->Columns; ++Column)
	{
		for (int32 Row = 0; Row < Board->Rows; ++Row)
		{
			const FHMT_GridCoord Coord(Column, Row);
			const bool bBlocked = Board->BlockedCells.Contains(Coord);
			if (bBlocked && !Board->BlockedTileMaterial)
			{
				continue;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;

			const FVector TileLocation = IHMT_GridLayout::Execute_GridToWorld(LayoutObject, Coord) + FVector(0.f, 0.f, 5.f);
			UMaterialInterface* Material = bBlocked ? Board->BlockedTileMaterial.Get() : Board->TileMaterial.Get();

			AActor* Spawned = nullptr;
			if (Board->TileMesh)
			{
				AStaticMeshActor* Tile = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnParams);
				if (!Tile)
				{
					continue;
				}
				Tile->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
				Tile->SetActorLocation(TileLocation);
				Tile->GetStaticMeshComponent()->SetStaticMesh(Board->TileMesh);
				if (Material)
				{
					Tile->GetStaticMeshComponent()->SetMaterial(0, Material);
				}
				Spawned = Tile;
			}
			else
			{
				AActor* Tile = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
				if (!Tile)
				{
					continue;
				}
				USceneComponent* Root = NewObject<USceneComponent>(Tile, TEXT("Root"));
				Tile->SetRootComponent(Root);
				Root->RegisterComponent();
				UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(Tile, TEXT("PreviewTile"));
				PMC->SetupAttachment(Root);
				PMC->RegisterComponent();
				if (Board->Topology == EHMT_GridTopology::Hex)
				{
					HMT_TileMeshUtils::BuildHexTile(PMC);
				}
				else
				{
					HMT_TileMeshUtils::BuildQuadTile(PMC);
				}
				if (Material)
				{
					PMC->SetMaterial(0, Material);
				}
				Tile->SetActorLocation(TileLocation);
				Spawned = Tile;
			}

			Spawned->Tags.Add(HMT_BoardPreviewTag);
			Spawned->SetActorLabel(FString::Printf(TEXT("HMT_Preview_%d_%d"), Column, Row));
			Spawned->SetActorScale3D(FVector(Scale));
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Editor] Board preview: spawned %d tiles for %s."), SpawnedCount, *Board->GetName());
	return FReply::Handled();
}

TSharedRef<SWidget> FHMT_BoardLayoutAssetCustomization::BuildGrid()
{
	UHMT_BoardLayoutAsset* Board = BoardWeak.Get();
	if (!Board || Board->Columns <= 0 || Board->Rows <= 0)
	{
		return SNullWidget::NullWidget;
	}

	TSharedRef<SVerticalBox> RowsBox = SNew(SVerticalBox);

	for (int32 Row = 0; Row < Board->Rows; ++Row)
	{
		TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

		for (int32 Column = 0; Column < Board->Columns; ++Column)
		{
			RowBox->AddSlot()
				.AutoWidth()
				.Padding(1.f)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.ToolTipText(FText::Format(NSLOCTEXT("HMT", "BoardCellTooltip", "({0}, {1}) — click to toggle blocked/valid"), FText::AsNumber(Column), FText::AsNumber(Row)))
					.IsChecked(this, &FHMT_BoardLayoutAssetCustomization::IsCellBlocked, Column, Row)
					.OnCheckStateChanged(this, &FHMT_BoardLayoutAssetCustomization::OnCellToggled, Column, Row)
					[
						SNew(SBox)
						.WidthOverride(24.f)
						.HeightOverride(24.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.BorderBackgroundColor(this, &FHMT_BoardLayoutAssetCustomization::GetCellColor, Column, Row)
						]
					]
				];
		}

		RowsBox->AddSlot()
			.AutoHeight()
			[
				RowBox
			];
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			RowsBox
		];
}

FSlateColor FHMT_BoardLayoutAssetCustomization::GetCellColor(int32 Column, int32 Row) const
{
	return IsCellBlocked(Column, Row) == ECheckBoxState::Checked
		? FSlateColor(FLinearColor(0.6f, 0.12f, 0.1f))
		: FSlateColor(FLinearColor(0.15f, 0.5f, 0.2f));
}

ECheckBoxState FHMT_BoardLayoutAssetCustomization::IsCellBlocked(int32 Column, int32 Row) const
{
	if (const UHMT_BoardLayoutAsset* Board = BoardWeak.Get())
	{
		const FHMT_GridCoord Coord(Column, Row);
		return Board->BlockedCells.Contains(Coord) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Unchecked;
}

void FHMT_BoardLayoutAssetCustomization::OnCellToggled(ECheckBoxState NewState, int32 Column, int32 Row)
{
	UHMT_BoardLayoutAsset* Board = BoardWeak.Get();
	if (!Board || !BlockedCellsHandle.IsValid())
	{
		return;
	}

	BlockedCellsHandle->NotifyPreChange();

	const FHMT_GridCoord Coord(Column, Row);
	if (NewState == ECheckBoxState::Checked)
	{
		if (!Board->BlockedCells.Contains(Coord))
		{
			Board->BlockedCells.Add(Coord);
		}
	}
	else
	{
		Board->BlockedCells.Remove(Coord);
	}

	BlockedCellsHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	Board->MarkPackageDirty();
}

void FHMT_BoardLayoutAssetCustomization::OnDimensionsChanged()
{
	if (CachedDetailBuilder)
	{
		CachedDetailBuilder->ForceRefreshDetails();
	}
}
