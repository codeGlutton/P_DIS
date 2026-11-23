/*****************************************************************//**
 * @file   BBMCharacterSelectViewModel.h
 * @brief  BBM(비트 배낭 상인) 캐릭터 선택 화면 MVVM 뷰모델 클래스 정의
 * @details UBBMTabViewModelBase를 상속하여 상인 명부 엔트리 목록과 선택 상태 및 원정 출격 액션 인터페이스를 제공함
 * @date   2026-11-20
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/BBMTabViewModelBase.h"
#include "BBMCharacterSelectViewModel.generated.h"

/**
 * 캐릭터 선택 패널 ViewModel
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Character Select ViewModel"))
class BBM_API UBBMCharacterSelectViewModel : public UBBMTabViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMCharacterSelectViewModel();

protected:
	virtual void BeginViewModel() override;
	virtual void EndViewModel() override;
	virtual void OnOpen() override;
	virtual void OnClosed() override;
	virtual void OnSelectedTabChanged(int32 InOldTabIndex, int32 InNewTabIndex) override;

	/* 비즈니스 로직 및 액션 인터페이스 */
public:
	/**
	 * 선택한 캐릭터로 새 게임을 시작하는 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|CharacterSelect")
	void StartNewGame();
};
