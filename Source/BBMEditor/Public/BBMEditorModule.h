/*****************************************************************//**
 * @file   BBMEditorModule.h
 * @brief  BBMEditor 에디터 모듈 인터페이스 정의
 * @details 호스트 게임(BBM)의 디테일 패널 프로퍼티 커스터마이제이션 및 에디터 툴 확장 모듈
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * BBM(비트 배낭 상인) 호스트 게임 전용 에디터 모듈
 */
class FBBMEditorModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

