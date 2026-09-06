#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Widgets/SWidget.h"
#include "Types/SlateEnums.h"
#include "Styling/SlateColor.h"
#include "Input/Reply.h"

class UHMT_BoardLayoutAsset;
class IPropertyHandle;

class FHMT_BoardLayoutAssetCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TWeakObjectPtr<UHMT_BoardLayoutAsset> BoardWeak;
	TSharedPtr<IPropertyHandle> BlockedCellsHandle;
	IDetailLayoutBuilder* CachedDetailBuilder = nullptr;

	TSharedRef<SWidget> BuildGrid();
	ECheckBoxState IsCellBlocked(int32 Column, int32 Row) const;
	void OnCellToggled(ECheckBoxState NewState, int32 Column, int32 Row);
	void OnDimensionsChanged();
	FSlateColor GetCellColor(int32 Column, int32 Row) const;

	FReply OnPreviewClicked();
	FReply OnClearPreviewClicked();
	void ClearPreviewActors() const;
};
