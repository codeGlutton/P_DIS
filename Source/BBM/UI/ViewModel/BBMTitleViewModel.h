/*****************************************************************//**
 * @file   BBMTitleViewModel.h
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 MVVM 뷰모델 클래스 정의
 * @details 이어하기 가시성, 런 데이터 상태 요약 및 타이틀 5대 메뉴(시작/이어하기/옵션/크레딧/종료) 비즈니스 로직 전담
 * @date   2026-11-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"
#include "Components/SlateWrapperTypes.h"
#include "BBMTitleViewModel.generated.h"

class UBBMSettingsViewModel;
class UBBMCharacterSelectViewModel;
class UBBMCreditsViewModel;

/**
 * BBM 타이틀 화면 전용 MVVM 뷰모델
 * UMG 위젯과 결합되어 데이터 바인딩 및 사용자 입력 처리를 총괄함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Title ViewModel"))
class BBM_API UBBMTitleViewModel : public UBBMViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMTitleViewModel();

protected:
	virtual void OnOpen() override;
	virtual void OnFinishOpenAnim() override;
	virtual void OnClosed() override;

	/* 필드 노티파이 프로퍼티 (Getter / Setter) */
public:
	/* 이어하기 가능 여부 반환 */
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	bool GetCanContinue() const { return bCanContinue; }

	/* 이어하기 가능 여부 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetCanContinue(bool bInCanContinue);

	/* 현재 진행 중인 런의 조우 인덱스 반환 (활성 런 미존재 시 0) */
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	int32 GetActiveRunEncounterIndex() const { return ActiveRunEncounterIndex; }

	/* 현재 진행 중인 런의 조우 인덱스 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetActiveRunEncounterIndex(int32 InEncounterIndex);

	/* 서브 ViewModel 접근자 */
public:
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMSettingsViewModel* GetSettingsViewModel() const { return SettingsViewModel; }

	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMCharacterSelectViewModel* GetCharacterSelectViewModel() const { return CharacterSelectViewModel; }

	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMCreditsViewModel* GetCreditsViewModel() const { return CreditsViewModel; }

	/* 타이틀 전환 모듈 접근자 */
public:
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMSingleAnimTransition* GetShiningTransition() const { return ShiningTransition; }

	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMDualAnimTransition* GetBlockingTransition() const { return BlockingTransition; }

	/* 비즈니스 로직 및 메뉴 액션 인터페이스 */
public:
	/**
	 * GameDataSubsystem을 조회하여 현재 런 세이브 유무에 따라 이어하기 버튼 가시성을 갱신
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void RefreshTitleState();

	/**
	 * 이어하기 버튼 클릭 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void ContinueGame();

	/**
	 * 옵션(설정) 창 열기 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void OpenSettings();

	/**
	 * 크레딧 창 열기 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void OpenCredits();

	/**
	 * 캐릭터 선택 창 열기 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void OpenCharacterSelections();

	/**
	 * 게임 종료 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void QuitGame();

	/* 내부 프로퍼티 */
private:
	/* 서브 ViewModel 컴포지션 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetSettingsViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMSettingsViewModel> SettingsViewModel;

	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetCharacterSelectViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMCharacterSelectViewModel> CharacterSelectViewModel;

	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetCreditsViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMCreditsViewModel> CreditsViewModel;

	/* 타이틀 화면 표시 중 무한 반복 재생되는 반짝임 전환 모듈 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetShiningTransition, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMSingleAnimTransition> ShiningTransition;

	/* 타이틀 블로킹 오버레이 시작/종료 전환 모듈 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetBlockingTransition, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMDualAnimTransition> BlockingTransition;

	/* 이어하기 가능 여부 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetCanContinue, Getter = GetCanContinue, meta = (AllowPrivateAccess = "true"))
	bool bCanContinue = false;

	/* 진행 중인 런의 현재 조우 인덱스 (활성 런 미존재 시 0, UI 텍스트 포맷 바인딩용) */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetActiveRunEncounterIndex, Getter = GetActiveRunEncounterIndex, meta = (AllowPrivateAccess = "true"))
	int32 ActiveRunEncounterIndex = 0;
};

