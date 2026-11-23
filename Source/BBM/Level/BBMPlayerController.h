/*****************************************************************//**
 * @file   BBMPlayerController.h
 * @brief  BBM(비트 배낭 상인) 기본 플레이어 컨트롤러 클래스 정의
 * @details UI 부재 시 타이틀/인게임 레벨 진행, 방 전환 및 전투 턴 시뮬레이션을 위한 디버그 치트/단축키 지원
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BBMPlayerController.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMPlayerCharacterDefinition;
class ABBMPlayerMerchantBase;

/**
 * BBM 기본 플레이어 컨트롤러 클래스
 * F1~F5 키 입력 및 bbm.* 콘솔 명령어를 통해 서브시스템 함수들을 직접 호출하여 UI 없이도 전체 루프 테스트를 가능하게 함
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM Player Controller"))
class BBM_API ABBMPlayerController : public APlayerController
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMPlayerController();

	/* APlayerController 라이프사이클 오버라이드 */
public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

#if WITH_EDITOR || !UE_BUILD_SHIPPING
	/* 콘솔 커맨드 ProcessConsoleExec 오버라이드 (bbm.* 명령어 파싱) */
	virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) override;
#endif

	/* 디버그 치트 및 런 진행 인터페이스 */
public:
#if WITH_EDITOR || !UE_BUILD_SHIPPING
	/**
	 * F1 키 / bbm.StartRun: 신규 런 시작 및 인게임 레벨 진입
	 */
	UFUNCTION(Exec, Category = "BBM|Debug")
	void StartRun();

	/**
	 * F2 키 / bbm.Next: 현재 만남 완료 후 다음 만남/방으로 전진
	 */
	UFUNCTION(Exec, Category = "BBM|Debug")
	void NextEncounter();

	/**
	 * F3 키 / bbm.SimulateTurn: 전투 1턴 시뮬레이션 즉시 실행
	 */
	UFUNCTION(Exec, Category = "BBM|Debug")
	void SimulateCombatTurn();

	/**
	 * F4 키 / bbm.Win: 적 전체 체력을 0으로 만들어 강제 승리 유도
	 */
	UFUNCTION(Exec, Category = "BBM|Debug")
	void ForceWinCombat();

	/**
	 * F5 키 / bbm.SelectRegion [Index]: 다음 지역 분기 선택지로 진입 (기본: 0번 인덱스)
	 * @param RegionIndex 선택할 다음 지역 옵션 인덱스
	 */
	UFUNCTION(Exec, Category = "BBM|Debug")
	void SelectNextRegionIndex(int32 RegionIndex = 0);
#endif

	/* 헬퍼 함수 */
protected:
	/* 레벨 내 플레이어 상인 폰 탐색 */
	ABBMPlayerMerchantBase* FindPlayerMerchant() const;

	/* 입력 이벤트 핸들러 */
protected:
#if WITH_EDITOR || !UE_BUILD_SHIPPING
	void OnF1Pressed();
	void OnF2Pressed();
	void OnF3Pressed();
	void OnF4Pressed();
	void OnF5Pressed();
#endif

	/* 프로퍼티 */
protected:
	/* 런 시작 시 사용할 기본 플레이어 상인 정의 에셋 */
	UPROPERTY(Category = "BBM|Debug", EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UBBMPlayerCharacterDefinition> DefaultMerchantDefinition;

	/* 신규 런 시작 시 전환할 인게임 레벨 이름 (기본값: L_Test) */
	UPROPERTY(Category = "BBM|Debug", EditDefaultsOnly, BlueprintReadOnly)
	FName InGameLevelName;

	/* 타이틀 레벨 이름 (기본값: L_Title) */
	UPROPERTY(Category = "BBM|Debug", EditDefaultsOnly, BlueprintReadOnly)
	FName TitleLevelName;
};

