/*****************************************************************//**
 * @file   AssetLinterTypes.h
 * @brief  Asset Linter 검사 대상 스코프 및 린트 규칙 데이터 구조체 정의
 * @details 대상 디렉터리 스코프, 네이밍 룰(Prefix/Suffix, 대소문자 옵션), 위치 룰(AllowedDirectories), 일괄 수정 제안 항목 구조체 정의
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/SoftObjectPath.h"
#include "AssetLinterTypes.generated.h"

/**
 * @struct FAssetLinterTargetScope
 * @brief 린터 검사가 적용될 대상 디렉터리 경로 및 하위 폴더 재귀 검사 여부 설정 구조체
 */
USTRUCT(BlueprintType)
struct ASSETLINTER_API FAssetLinterTargetScope
{
	GENERATED_BODY()

	/* 스코프 속성 */
	/** 검사 대상 디렉터리 경로 (예: /Game/UI, /BBMGameFeatureCore 등) */
	UPROPERTY(EditAnywhere, config, Category = "Scope", meta = (ContentDir))
	FDirectoryPath TargetDirectory;

	/** 해당 디렉터리의 하위 폴더까지 재귀적으로 검사할지 여부 */
	UPROPERTY(EditAnywhere, config, Category = "Scope")
	bool bIncludeSubdirectories = true;

	/** 스코프 설명 (예: "BBM UI 리소스 경로") */
	UPROPERTY(EditAnywhere, config, Category = "Scope")
	FString Description;
};

/**
 * @struct FAssetNamingRule
 * @brief 특정 클래스 타입(및 파생형)에 강제할 에셋 이름 접두사/접미사 규칙 구조체
 */
USTRUCT(BlueprintType)
struct ASSETLINTER_API FAssetNamingRule
{
	GENERATED_BODY()

	/* 네이밍 규칙 속성 */
	/** 검사 대상 클래스 (상속 관계 판정용) */
	UPROPERTY(EditAnywhere, config, Category = "Naming Rule")
	TSoftClassPtr<UObject> TargetClass;

	/** 필수 Prefix (비어 있으면 접두사 검사 생략, 예: "T_", "WBP_") */
	UPROPERTY(EditAnywhere, config, Category = "Naming Rule")
	FString RequiredPrefix;

	/** 필수 Suffix (비어 있으면 접미사 검사 생략, 예: "_Inst", "_Data") */
	UPROPERTY(EditAnywhere, config, Category = "Naming Rule")
	FString RequiredSuffix;

	/** 대소문자 정확히 일치 여부 (기본값: true - 엄격한 대소문자 구분) */
	UPROPERTY(EditAnywhere, config, Category = "Naming Rule")
	bool bMatchCase = true;

	/** 규칙 설명 (예: "Texture 에셋은 T_ 접두사 강제") */
	UPROPERTY(EditAnywhere, config, Category = "Naming Rule")
	FString Description;
};

/**
 * @struct FAssetLocationRule
 * @brief 특정 클래스 타입(및 파생형)에 강제할 에셋 보관 디렉터리 위치 규칙 구조체
 */
USTRUCT(BlueprintType)
struct ASSETLINTER_API FAssetLocationRule
{
	GENERATED_BODY()

	/* 위치 규칙 속성 */
	/** 검사 대상 클래스 */
	UPROPERTY(EditAnywhere, config, Category = "Location Rule")
	TSoftClassPtr<UObject> TargetClass;

	/** 허용된 디렉터리 경로 목록 (복수 경로 허용 가능) */
	UPROPERTY(EditAnywhere, config, Category = "Location Rule", meta = (ContentDir))
	TArray<FDirectoryPath> AllowedDirectories;

	/** 허용 디렉터리의 하위 폴더 위치 허용 여부 */
	UPROPERTY(EditAnywhere, config, Category = "Location Rule")
	bool bAllowSubdirectories = true;

	/** 규칙 설명 (예: "Sound 에셋은 /Game/Audio 하위 강제") */
	UPROPERTY(EditAnywhere, config, Category = "Location Rule")
	FString Description;
};

/**
 * @struct FAssetLintFixItem
 * @brief 일괄 수정 대상 에셋의 현재 상태와 변경될 제안 상태를 보관하는 데이터 구조체
 */
struct ASSETLINTER_API FAssetLintFixItem
{
	/** 대상 에셋 정보 */
	FAssetData AssetData;

	/** 현재 에셋 명칭 */
	FString CurrentName;

	/** 변경 제안 명칭 */
	FString ProposedName;

	/** 이름 변경 필요 여부 */
	bool bNeedsRename = false;

	/** 현재 패키지 경로 */
	FString CurrentPath;

	/** 이동 제안 패키지 경로 */
	FString ProposedPath;

	/** 위치 이동 필요 여부 */
	bool bNeedsMove = false;

	/** 위반 사유 설명 */
	FText ViolationReason;

	/** 팝업 UI 체크박스 선택 여부 */
	bool bIsSelected = true;
};
