/*****************************************************************//**
 * @file   BBMComboBoxTextEntryViewModel.h
 * @brief  텍스트 기반 콤보박스 단일 항목용 경량 MVVM 엔트리 뷰모델 클래스 정의
 * @details 표시 텍스트(EntryText)와 항목 인덱스(EntryIndex) FieldNotify 프로퍼티를 제공함
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BBMComboBoxTextEntryViewModel.generated.h"

/**
 * 텍스트 콤보박스의 개별 Content/Item 위젯에 주입되는 엔트리 뷰모델
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM ComboBox Text Entry ViewModel"))
class BBM_API UBBMComboBoxTextEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 조회 및 설정 (FieldNotify) */
public:
	/* 콤보박스에 표시할 항목 텍스트 반환 */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable, BlueprintPure)
	const FText& GetEntryText() const { return EntryText; }

	/* 콤보박스에 표시할 항목 텍스트 설정 */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable)
	void SetEntryText(const FText& InEntryText);

	/* 콤보박스 목록 내 항목 인덱스 반환 (미설정 시 INDEX_NONE) */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable, BlueprintPure)
	int32 GetEntryIndex() const { return EntryIndex; }

	/* 콤보박스 목록 내 항목 인덱스 설정 */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable)
	void SetEntryIndex(int32 InEntryIndex);

	/* 데이터 프로퍼티 */
private:
	/* UI에 표시될 항목 텍스트 */
	UPROPERTY(Category = "BBM|UI|ComboBox", BlueprintReadWrite, FieldNotify, Getter, Setter, meta = (AllowPrivateAccess = "true"))
	FText EntryText;

	/* 콤보박스 목록 내 0 기반 인덱스 */
	UPROPERTY(Category = "BBM|UI|ComboBox", BlueprintReadWrite, FieldNotify, Getter, Setter, meta = (AllowPrivateAccess = "true"))
	int32 EntryIndex = INDEX_NONE;
};
