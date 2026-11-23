/*****************************************************************//**
 * @file   DISGlobalSettings.h
 * @brief  DIS 플러그인 전역 환경설정(DeveloperSettings) 클래스 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Item/DISItemTypes.h"
#include "DISGlobalSettings.generated.h"

class UDISGlobalLogic;

/**
 * DIS 플러그인의 프로젝트 전역 환경설정을 관리하는 클래스
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "DIS Global Settings"))
class DISRUNTIME_API UDISGlobalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDISGlobalSettings();

	/* UDeveloperSettings 인터페이스 */
public:
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	/* 설정 헬퍼 메서드 */
public:
	/* 유효한 전역 로직 서브오브젝트 클래스 반환 (미지정 시 기본 UDISGlobalLogic 반환) */
	UClass* GetGlobalLogicClass() const;

	/* 아이템 스태킹 판정 시 제외할 강화 효과 태그 컨테이너 반환 */
	const FGameplayTagContainer& GetStackingIgnoredEnhancementTags() const { return StackingIgnoredEnhancementTags; }

	/* 다중 수식어 결합 시 사용하는 구분자 텍스트 반환 */
	const FText& GetAffixDelimiter() const { return AffixDelimiter; }

	/* 접두·접미사가 모두 존재할 때 사용하는 표시 이름 서식 반환 */
	const FText& GetEnhancedDisplayNamePrefixAndSuffixFormat() const { return EnhancedDisplayNamePrefixAndSuffixFormat; }

	/* 접두사만 존재할 때 사용하는 표시 이름 서식 반환 */
	const FText& GetEnhancedDisplayNamePrefixOnlyFormat() const { return EnhancedDisplayNamePrefixOnlyFormat; }

	/* 접미사만 존재할 때 사용하는 표시 이름 서식 반환 */
	const FText& GetEnhancedDisplayNameSuffixOnlyFormat() const { return EnhancedDisplayNameSuffixOnlyFormat; }

	/* 설정 프로퍼티 */
public:
	/* 프로젝트에서 사용할 전역 DIS 로직 서브오브젝트 클래스 (C++ 및 Blueprint 파생 클래스 지정 가능) */
	UPROPERTY(Category = "Global Logic", EditAnywhere, BlueprintReadOnly, Config, meta = (AllowAbstract = "false"))
	TSubclassOf<UDISGlobalLogic> GlobalLogicClass;

	/* 등록되지 않은 아이템 타입에 적용되는 기본 폴백 프로파일 */
	UPROPERTY(Category = "Item Profile", EditAnywhere, BlueprintReadOnly, Config)
	FDISItemTypeProfile DefaultItemProfile;

	/* DIS 전역 런타임 설정 PrimaryDataAsset ID */
	UPROPERTY(Category = "Global Definition", EditAnywhere, BlueprintReadOnly, Config, meta = (AllowedTypes = "DISGlobal"))
	FPrimaryAssetId GlobalDefinitionAssetId;

	/* 프로젝트 전역 단일 아이템 최대 점유 타일 크기 (Width, Height) */
	UPROPERTY(Category = "Item Spatial", EditAnywhere, BlueprintReadOnly, Config, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint MaxItemTileDimensions;

	/* 아이템 스태킹(CanStackWith / HasIdenticalEnhancements) 판정 시 레벨 계산에서 제외할 강화 효과 태그 컨테이너 */
	UPROPERTY(Category = "Enhancement|Stacking", EditAnywhere, BlueprintReadOnly, Config)
	FGameplayTagContainer StackingIgnoredEnhancementTags;

	/* 동일 계열 다중 수식어 또는 기본 이름 미존재 시 접두·접미사 결합에 사용할 구분자 텍스트 */
	UPROPERTY(Category = "Enhancement|Display", EditAnywhere, BlueprintReadOnly, Config)
	FText AffixDelimiter;

	/* 접두사({Prefix}), 기본 이름({BaseName}), 접미사({Suffix})가 모두 존재할 때 사용하는 최종 이름 서식 포맷 */
	UPROPERTY(Category = "Enhancement|Display", EditAnywhere, BlueprintReadOnly, Config)
	FText EnhancedDisplayNamePrefixAndSuffixFormat;

	/* 접두사({Prefix})와 기본 이름({BaseName})만 존재할 때 사용하는 최종 이름 서식 포맷 */
	UPROPERTY(Category = "Enhancement|Display", EditAnywhere, BlueprintReadOnly, Config)
	FText EnhancedDisplayNamePrefixOnlyFormat;

	/* 기본 이름({BaseName})과 접미사({Suffix})만 존재할 때 사용하는 최종 이름 서식 포맷 */
	UPROPERTY(Category = "Enhancement|Display", EditAnywhere, BlueprintReadOnly, Config)
	FText EnhancedDisplayNameSuffixOnlyFormat;
};
