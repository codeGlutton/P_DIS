/*****************************************************************//**
 * @file   BBMUITypes.h
 * @brief  BBM(비트 배낭 상인) 호스트 게임 전용 UI 타입 정의
 * @details 인카운터 기준 UI 생성 타이밍, 게임모드 통합 UI 설정 및 크레딧 카테고리 구조체 선언
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "WorldUITypes.h"
#include "BBMUITypes.generated.h"

/**
 * 인카운터 초기화 전/후 UI 생성 타이밍 열거형
 */
UENUM(BlueprintType)
enum class EBBMWidgetCreationPhase : uint8
{
	/* 인카운터 배치(상인/몬스터 스폰 및 세이브 복원) 전 생성 (예: FadeLoading 위젯) */
	BeforeEncounter UMETA(DisplayName = "Before Encounter"),

	/* 인카운터 배치 완료 후 생성 (예: 인게임 전투 HUD) */
	AfterEncounter  UMETA(DisplayName = "After Encounter")
};

/**
 * 게임모드 자동 등록 및 생성용 통합 UI(위젯 + ViewModel + 타이밍) 설정 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMWorldUIConfig
{
	GENERATED_BODY()

	/* 프로퍼티 */
public:
	/* 위젯 및 ViewModel 공용 식별 키 (예: "LoadingScreen", "MainHUD", "TitleMenu") */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	FName WidgetKey = NAME_None;

	/* UI 생성 타이밍 */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	EBBMWidgetCreationPhase CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;

	/* 순수 위젯 생성 설정 */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	FWorldWidgetConfig WidgetConfig;

	/* 순수 ViewModel 생성 설정 */
	UPROPERTY(Category = "BBM|UI|MVVM", EditDefaultsOnly, BlueprintReadOnly)
	FWorldViewModelConfig ViewModelConfig;
};

/**
 * 크레딧 화면의 단일 카테고리 및 소속 인물·팀 목록 정의 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMCreditCategory
{
	GENERATED_BODY()

	/* 비교 연산자 */
public:
	bool operator==(const FBBMCreditCategory& Other) const
	{
		if (CategoryName.EqualTo(Other.CategoryName) == false)
		{
			return false;
		}

		if (Entries.Num() != Other.Entries.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			if (Entries[Index].EqualTo(Other.Entries[Index]) == false)
			{
				return false;
			}
		}

		return true;
	}

	bool operator!=(const FBBMCreditCategory& Other) const
	{
		return (*this == Other) == false;
	}

	/* 프로퍼티 */
public:
	/* 크레딧 카테고리 이름 (예: 디렉터, 프로그래밍, 아트, 폰트 라이선스 등) */
	UPROPERTY(Category = "BBM|Credits", EditAnywhere, BlueprintReadWrite)
	FText CategoryName;

	/* 해당 카테고리에 표시할 연관 인물, 팀명 또는 라이선스 문구 배열 */
	UPROPERTY(Category = "BBM|Credits", EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	TArray<FText> Entries;
};