#pragma once

#include "Modules/ModuleManager.h"

class FHMT_CoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
