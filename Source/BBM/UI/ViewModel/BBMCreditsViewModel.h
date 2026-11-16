/*****************************************************************//**
 * @file   BBMCreditsViewModel.h
 * @brief  BBM(비트 배낭 상인) 크레딧 패널 전용 MVVM 뷰모델 클래스 정의
 * @details BeginViewModel 초기화 시 UBBMGlobalSettings의 크레딧 카테고리 목록을 읽어 ListView용 엔트리 ViewModel 배열을 구축함
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/BBMUITypes.h"
#include "ViewModel/WorldViewModelBase.h"
#include "BBMCreditsViewModel.generated.h"

/* 독립 전방 선언 */
class UBBMCreditEntryViewModel;

/**
 * 크레딧 패널 ViewModel
 */
UCLASS(BlueprintType, Blueprintable)
class BBM_API UBBMCreditsViewModel : public UWorldViewModelBase
{
	GENERATED_BODY()

	/* 크레딧 항목 설정 및 조회 */
public:
	/**
	 * 전달받은 FBBMCreditCategory 배열로부터 ListView용 엔트리 ViewModel 배열을 생성 및 대입함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Credits")
	void SetCreditCategories(const TArray<FBBMCreditCategory>& InCreditCategories);

	/**
	 * 생성된 크레딧 엔트리 ViewModel 배열 읽기 전용 참조를 반환함 (C++ 및 테스트용)
	 */
	const TArray<TObjectPtr<UBBMCreditEntryViewModel>>& GetCreditEntries() const { return CreditEntries; }

	/* ViewModel 수명 주기 훅 */
protected:
	virtual void BeginViewModel() override;
	virtual void EndViewModel() override;

	/* 뷰 바인딩 프로퍼티 */
private:
	/* ListView Items에 바인딩할 카테고리별 크레딧 엔트리 ViewModel 목록 (FieldNotify) */
	UPROPERTY(Category = "BBM|UI|Credits", BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UBBMCreditEntryViewModel>> CreditEntries;
};
