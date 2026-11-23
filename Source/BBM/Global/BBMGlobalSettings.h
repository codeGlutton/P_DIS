/*****************************************************************//**
 * @file   BBMGlobalSettings.h
 * @brief  BBM 전역 개발자 환경설정(오디오·전투 팀·크레딧 등) 정의
 * @details Project Settings의 Game -> BBM Global Settings에서 오디오 믹스/클래스, 팀 태도 매핑, 크레딧 목록 등을 통합 관리함
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Global/BBMGlobalTypes.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "UI/BBMUITypes.h"
#include "BBMGlobalSettings.generated.h"

/* 독립 전방 선언 */
class USoundMix;
class USoundClass;
class UInputMappingContext;
class UInputAction;

/**
 * BBM 프로젝트의 전역 개발자 환경설정 통합 클래스
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "BBM Global Settings"))
class BBM_API UBBMGlobalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	/* 생성자 및 UDeveloperSettings 인터페이스 */
public:
	UBBMGlobalSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	/* FGenericTeamId 태도 해결 솔버 (Static Attitude Solver) */
public:
	/* 두 팀 식별자 간의 상호 태도(Friendly, Hostile, Neutral) 판정 */
	static ETeamAttitude::Type GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId);

	/* 오디오 설정 */
public:
	/* 옵션 볼륨 오버라이드 적용 대상 전역 사운드 믹스 */
	UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<USoundMix> UserSettingsSoundMix;

	/* 전체 마스터 볼륨 제어용 루트 사운드 클래스 */
	UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	/* 배경음악(BGM) 볼륨 제어용 사운드 클래스 */
	UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<USoundClass> BGMSoundClass;

	/* 효과음(SFX) 볼륨 제어용 사운드 클래스 */
	UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	/* 다이얼로그/음성 볼륨 제어용 사운드 클래스 */
	UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<USoundClass> DialogueSoundClass;

	/* 전투 및 팀 설정 */
public:
	/* 팀별 상호 태도 매핑 관계 */
	UPROPERTY(Category = "Combat|Team", EditAnywhere, BlueprintReadOnly, Config)
	TMap<EBBMTeamType, FBBMTeamRelation> TeamRelations;

	/* 크레딧 설정 */
public:
	/* 크레딧 화면에 순서대로 표시할 카테고리별 크레딧 항목 목록 */
	UPROPERTY(Category = "UI|Credits", EditAnywhere, BlueprintReadOnly, Config)
	TArray<FBBMCreditCategory> CreditCategories;

	/* 입력 설정 */
public:
	/* 런타임 복제 및 키 리매핑 대상 기본 Input Mapping Context */
	UPROPERTY(Category = "Input", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	/* Shift·Alt·Ctrl 모디파이어 판정용 전용 Input Mapping Context (비복제 등록) */
	UPROPERTY(Category = "Input", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<UInputMappingContext> ModifierInputMappingContext;

	/* Chord Trigger Shift 조합 판정용 Input Action */
	UPROPERTY(Category = "Input", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<UInputAction> ShiftChordAction;

	/* Chord Trigger Ctrl 조합 판정용 Input Action */
	UPROPERTY(Category = "Input", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<UInputAction> CtrlChordAction;

	/* Chord Trigger Alt 조합 판정용 Input Action */
	UPROPERTY(Category = "Input", EditAnywhere, BlueprintReadOnly, Config)
	TSoftObjectPtr<UInputAction> AltChordAction;

	/* 환경설정 선택 옵션 목록 (배열 순서가 UI 표시 순서를 결정함) */
public:
	/* 선택 가능한 언어 옵션 목록 */
	UPROPERTY(Category = "Settings|Options", EditAnywhere, BlueprintReadOnly, Config)
	TArray<FBBMStringSelectionData> LanguageOptions;

	/* 선택 가능한 창 모드 옵션 목록 */
	UPROPERTY(Category = "Settings|Options", EditAnywhere, BlueprintReadOnly, Config)
	TArray<FBBMWindowModeSelectionData> WindowModeOptions;

	/* 선택 가능한 화면 해상도 옵션 목록 */
	UPROPERTY(Category = "Settings|Options", EditAnywhere, BlueprintReadOnly, Config)
	TArray<FBBMIntPointSelectionData> ScreenResolutionOptions;
};
