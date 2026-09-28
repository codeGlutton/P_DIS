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

public:
	/* 등록되지 않은 아이템 타입에 적용되는 기본 폴백 프로파일 */
	UPROPERTY(Category = "Item Profile", EditAnywhere, BlueprintReadOnly, Config)
	FDISItemTypeProfile DefaultItemProfile;

	/* DIS 전역 런타임 설정 PrimaryDataAsset ID */
	UPROPERTY(Category = "Global Definition", EditAnywhere, BlueprintReadOnly, Config, meta = (AllowedTypes = "DISGlobal"))
	FPrimaryAssetId GlobalDefinitionAssetId;

	/* 프로젝트 전역 단일 아이템 최대 점유 타일 크기 (Width, Height) */
	UPROPERTY(Category = "Item Spatial", EditAnywhere, BlueprintReadOnly, Config, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint MaxItemTileDimensions;
};
