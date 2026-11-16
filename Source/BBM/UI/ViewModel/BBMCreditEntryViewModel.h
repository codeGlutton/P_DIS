/*****************************************************************//**
 * @file   BBMCreditEntryViewModel.h
 * @brief  크레딧 ListView 단일 카테고리 항목용 MVVM 뷰모델 클래스 정의
 * @details FBBMCreditCategory 구조체를 소유하고 카테고리 이름 및 줄바꿈으로 합쳐진 항목 텍스트를 FieldNotify로 노출함
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "UI/BBMUITypes.h"
#include "BBMCreditEntryViewModel.generated.h"

/**
 * 크레딧 ListView의 단일 카테고리 행을 표현하는 경량 ViewModel
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Credit Entry ViewModel"))
class BBM_API UBBMCreditEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 조회 및 설정 (FieldNotify) */
public:
	/**
	 * 소유 중인 크레딧 카테고리의 표시 이름을 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|Credits")
	FText GetCategoryName() const;

	/**
	 * 소유 중인 크레딧 항목 배열을 줄바꿈(\n)으로 이어 붙인 단일 텍스트로 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|Credits")
	FText GetEntriesText() const;

	/**
	 * 소유 중인 크레딧 카테고리 구조체 원본을 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|Credits")
	const FBBMCreditCategory& GetCreditCategory() const { return CreditCategory; }

	/**
	 * 크레딧 카테고리 구조체를 설정하고 변경 시 관련 FieldNotify를 일괄 발송함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Credits")
	void SetCreditCategory(const FBBMCreditCategory& InCreditCategory);

	/* 데이터 프로퍼티 */
private:
	/* 현재 엔트리가 표현하는 크레딧 카테고리 및 인물·팀 목록 */
	UPROPERTY(Category = "BBM|UI|Credits", BlueprintReadWrite, FieldNotify, Setter = SetCreditCategory, Getter = GetCreditCategory, meta = (AllowPrivateAccess = "true"))
	FBBMCreditCategory CreditCategory;
};
