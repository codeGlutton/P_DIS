/*****************************************************************//**
 * @file   BBMGameModeBase.h
 * @brief  BBM(비트 배낭 상인) 기본 게임모드 베이스 클래스 정의
 * @details 기본 컨트롤러/카메라 폰 지정 및 월드 레벨 전역 위젯(HUD, FadeLoading 등) 라이프사이클 총괄
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMGameModeBase.generated.h"

/**
 * BBM 기본 게임모드 베이스 클래스
 * 기본 카메라 폰 및 플레이어 컨트롤러를 구성하고, 월드 위젯 서브시스템과 연동하여
 * 인카운터 초기화 전/후 위젯 자동 생성 및 수명 주기를 관리함
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "BBM Game Mode Base"))
class BBM_API ABBMGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMGameModeBase();

	/* AActor 라이프사이클 오버라이드 */
public:
	virtual void BeginPlay() override;

	/* 위젯 자동 생성 헬퍼 */
public:
	/**
	 * 특정 생성 타이밍(인카운터 초기화 전/후)에 해당하는 UI(위젯 및 ViewModel)를 월드 UI 서브시스템에 일괄 생성/등록
	 * @param Phase 생성 타이밍 (BeforeEncounter / AfterEncounter)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	void CreateWorldUIsByPhase(EBBMWidgetCreationPhase Phase);

	/* 레벨 특화 게임모드 라이프사이클 오버라이드 지점 */
protected:
	/**
	 * 파생 게임모드(타이틀, 스테이지 등)에서 고유 레벨 로직을 수행하기 위한 가상 함수
	 * BeforeEncounter UI가 생성된 직후, AfterEncounter UI가 생성되기 전에 호출되며 기본 구현에서는 PC 환경의 마우스 커서 표시를 활성화함
	 */
	virtual void InitializeLevelSpecificGameMode();

	/* 설정 프로퍼티 */
protected:
	/* 해당 게임모드가 적용된 레벨 진입 시 월드 UI 서브시스템에 자동 등록/생성할 UI 목록 */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	TArray<FBBMWorldUIConfig> AutoCreateUIs;
};
