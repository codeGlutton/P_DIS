/*****************************************************************//**
 * @file   BBMTitleViewModel.h
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 MVVM 뷰모델 클래스 정의
 * @details 이어하기 가시성, 런 데이터 상태 요약 및 타이틀 5대 메뉴(시작/이어하기/옵션/크레딧/종료) 비즈니스 로직 전담
 * @date   2026-11-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "ViewModel/WorldViewModelBase.h"
#include "ViewModel/SingleAnimTransition.h"
#include "ViewModel/DualAnimTransition.h"
#include "Components/SlateWrapperTypes.h"
#include "BBMTitleViewModel.generated.h"

class UBBMSettingsViewModel;
class UBBMCharacterSelectViewModel;
class UBBMCreditsViewModel;
class UBBMGameDataSubsystem;

/**
 * BBM 타이틀 화면 전용 MVVM 뷰모델
 * UMG 위젯과 결합되어 데이터 바인딩 및 사용자 입력 처리를 총괄함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Title ViewModel"))
class BBM_API UBBMTitleViewModel : public UWorldViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMTitleViewModel();

protected:
	virtual void BeginViewModel() override;
	virtual void EndViewModel() override;
	virtual void OnOpen() override;
	virtual void OnFinishOpenAnim(bool bInterrupted) override;
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

	/* 블로킹 패널 가시성 반환 */
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	ESlateVisibility GetBlockingPanelVisibility() const { return BlockingPanelVisibility; }

	/* 블로킹 패널 가시성 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetBlockingPanelVisibility(ESlateVisibility InVisibility);

	/* 서브 ViewModel 접근자 */
public:
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMSettingsViewModel* GetSettingsViewModel() const { return SettingsViewModel; }

	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetSettingsViewModel(UBBMSettingsViewModel* InSettingsViewModel);

	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMCharacterSelectViewModel* GetCharacterSelectViewModel() const { return CharacterSelectViewModel; }

	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetCharacterSelectViewModel(UBBMCharacterSelectViewModel* InCharacterSelectViewModel);

	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UBBMCreditsViewModel* GetCreditsViewModel() const { return CreditsViewModel; }
	
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetCreditsViewModel(UBBMCreditsViewModel* InCreditsViewModel);

	/* 타이틀 전환 모듈 접근자 */
public:
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	USingleAnimTransition* GetShiningTransition() const { return ShiningTransition; }

	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	UDualAnimTransition* GetBlockingTransition() const { return BlockingTransition; }

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

	/* 내부 대리자 콜백 및 바인딩 헬퍼 */
private:
	/* GameDataSubsystem::OnSaveStartedNative 수신 시 타이틀 이어하기 상태 갱신 */
	void HandleSaveStarted();

	/* BlockingTransition 시작/종료 시 블로킹 패널 가시성 전환 */
	void HandleBlockingTransitionStarting(bool bWithAnim);
	void HandleBlockingTransitionEnded(bool bInterrupted);

	/* 서브 패널 OpenClose 전환 시작/종료 카운팅 및 블로킹 전환 구동 */
	void HandleSubPanelStarting(bool bWithAnim);
	void HandleSubPanelEnded(bool bInterrupted);

	/* 서브 ViewModel의 OpenCloseTransition 훅 바인딩·해제 헬퍼 */
	void BindSubViewModelOpenClose(UWorldViewModelBase* InSubViewModel);
	void UnbindSubViewModelOpenClose();

	/* 내부 프로퍼티 */
private:
	/* 서브 ViewModel 컴포지션 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetSettingsViewModel, Getter = GetSettingsViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMSettingsViewModel> SettingsViewModel;

	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetCharacterSelectViewModel, Getter = GetCharacterSelectViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMCharacterSelectViewModel> CharacterSelectViewModel;

	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetCreditsViewModel, Getter = GetCreditsViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMCreditsViewModel> CreditsViewModel;

	/* 타이틀 화면 표시 중 무한 반복 재생되는 반짝임 전환 모듈 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetShiningTransition, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USingleAnimTransition> ShiningTransition;

	/* 타이틀 블로킹 오버레이 시작/종료 전환 모듈 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadOnly, FieldNotify, Getter = GetBlockingTransition, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDualAnimTransition> BlockingTransition;

	/* 이어하기 가능 여부 */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetCanContinue, Getter = GetCanContinue, meta = (AllowPrivateAccess = "true"))
	bool bCanContinue = false;

	/* 진행 중인 런의 현재 조우 인덱스 (활성 런 미존재 시 0, UI 텍스트 포맷 바인딩용) */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetActiveRunEncounterIndex, Getter = GetActiveRunEncounterIndex, meta = (AllowPrivateAccess = "true"))
	int32 ActiveRunEncounterIndex = 0;

	/* 타이틀 블로킹 오버레이 패널 가시성 (애니메이션 시작 시 Visible, 종료 완료 시 Collapsed) */
	UPROPERTY(Category = "BBM|UI|Title", BlueprintReadWrite, FieldNotify, Setter = SetBlockingPanelVisibility, Getter = GetBlockingPanelVisibility, meta = (AllowPrivateAccess = "true"))
	ESlateVisibility BlockingPanelVisibility = ESlateVisibility::Collapsed;

	/* OnSaveStartedNative 대리자 등록 핸들 */
	FDelegateHandle SaveStartedHandle;

	/* BlockingTransition 시작/종료 대리자 등록 핸들 */
	FDelegateHandle BlockingStartingHandle;
	FDelegateHandle BlockingEndedHandle;

	/* 서브 패널 전환 훅 바인딩 정보 보관 (GC 순서와 무관한 안전 언바인딩용) */
	struct FSubPanelHookBinding
	{
		TWeakObjectPtr<UDualAnimTransition> Transition;
		FDelegateHandle StartingHandle;
		FDelegateHandle EndedHandle;
	};
	TArray<FSubPanelHookBinding> SubPanelHookBindings;

	/* 현재 열려 있거나 열림 전환 중인 서브 패널 개수 */
	int32 OpenSubPanelCount = 0;
};

