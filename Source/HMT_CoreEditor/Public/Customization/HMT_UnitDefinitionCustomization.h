#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UHMT_UnitDefinition;
class IPropertyHandle;
class SWidget;

class FHMT_UnitDefinitionCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TWeakObjectPtr<UHMT_UnitDefinition> UnitDefinitionWeak;

	TSharedRef<SWidget> BuildSocketPicker(TSharedPtr<IPropertyHandle> NameHandle);
	TSharedRef<SWidget> BuildSocketMenu(TSharedPtr<IPropertyHandle> NameHandle);
};
