/*****************************************************************//**
 * @file   AssetLinterSettings.h
 * @brief  Project Settings에 노출되는 Asset Linter 전역 설정 클래스 정의
 * @details 에셋 린터의 대상 스코프, 네이밍 규칙, 위치 규칙 및 경고 모드 제어
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AssetLinterTypes.h"
#include "AssetLinterSettings.generated.h"

/**
 * @class UAssetLinterSettings
 * @brief Asset Linter의 검사 대상 스코프, 네이밍 규칙, 위치 규칙을 관리하는 프로젝트 설정 클래스
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Asset Linter"))
class ASSETLINTER_API UAssetLinterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	/* 생성자 및 설정 식별 메서드 */
public:
	UAssetLinterSettings();

	/** 설정 카테고리 명칭 반환 (Plugins) */
	virtual FName GetCategoryName() const override;

	/** 설정 섹션 명칭 반환 (Asset Linter) */
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	/** 에디터 UI 표시 섹션 제목 */
	virtual FText GetSectionText() const override;

	/** 에디터 UI 표시 섹션 설명 */
	virtual FText GetSectionDescription() const override;
#endif

	/* 에셋 린터 설정 속성 */
public:
	/** 검사 대상 디렉터리 스코프 목록 (각 스코프별 하위 폴더 재귀 검사 여부 지정) */
	UPROPERTY(EditAnywhere, config, Category = "Scope Settings", meta = (DisplayName = "Target Scopes"))
	TArray<FAssetLinterTargetScope> TargetScopes;

	/** 검사에서 제외할 예외 디렉터리 목록 (예: /Engine, /Developers 등) */
	UPROPERTY(EditAnywhere, config, Category = "Scope Settings", meta = (ContentDir, DisplayName = "Excluded Directories"))
	TArray<FDirectoryPath> ExcludedDirectories;

	/** 에셋 네이밍 검사 규칙 목록 */
	UPROPERTY(EditAnywhere, config, Category = "Rules", meta = (DisplayName = "Naming Rules"))
	TArray<FAssetNamingRule> NamingRules;

	/** 에셋 위치 검사 규칙 목록 */
	UPROPERTY(EditAnywhere, config, Category = "Rules", meta = (DisplayName = "Location Rules"))
	TArray<FAssetLocationRule> LocationRules;

	/** 위반 시 에러(Error) 대신 경고(Warning)로 처리할지 여부 */
	UPROPERTY(EditAnywhere, config, Category = "Behavior", meta = (DisplayName = "Treat As Warning"))
	bool bTreatAsWarning = false;
};
