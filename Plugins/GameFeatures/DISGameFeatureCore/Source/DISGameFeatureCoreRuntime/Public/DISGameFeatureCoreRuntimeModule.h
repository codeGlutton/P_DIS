#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDISGameFeatureCoreRuntimeModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
