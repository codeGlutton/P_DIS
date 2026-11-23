/*****************************************************************//**
 * @file   AssetLinter.h
 * @brief  Asset Linter 플러그인 에디터 모듈 인터페이스 선언
 * @details 에디터 구동 시 DataValidation 시스템과 연동되는 AssetLinter 모듈의 시작 및 종료를 관리함
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @class FAssetLinterModule
 * @brief Asset Linter 플러그인의 에디터 모듈 구현체
 */
class FAssetLinterModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	/** IModuleInterface 구현 */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
