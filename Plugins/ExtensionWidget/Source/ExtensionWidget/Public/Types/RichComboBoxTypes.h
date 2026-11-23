/*****************************************************************//**
 * @file   RichComboBoxTypes.h
 * @brief  URichComboBoxKey에서 사용하는 ViewModel 매핑 아이템 구조체 정의
 * @details FName 키와 엔트리 ViewModel 인스턴스(UObject) 쌍을 보관하여 MVVM 배열 바인딩 및 내부 맵 캐싱에 활용함.
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "RichComboBoxTypes.generated.h"

/**
 * 콤보박스 옵션 키와 해당 키에 바인딩할 ViewModel 객체 쌍
 */
USTRUCT(BlueprintType)
struct EXTENSIONWIDGET_API FRichComboBoxKeyViewModelItem
{
	GENERATED_BODY()

	/* 비교 연산자 */
public:
	bool operator==(const FRichComboBoxKeyViewModelItem& Other) const
	{
		return Key == Other.Key && ViewModel == Other.ViewModel;
	}

	bool operator!=(const FRichComboBoxKeyViewModelItem& Other) const
	{
		return (*this == Other) == false;
	}

	/* 데이터 프로퍼티 */
public:
	/* 콤보박스 옵션 식별 키 */
	UPROPERTY(Category = "Rich ComboBox|ViewModel", EditAnywhere, BlueprintReadWrite)
	FName Key = NAME_None;

	/* 해당 키에 매칭될 엔트리 ViewModel 인스턴스 (INotifyFieldValueChanged 구현 객체) */
	UPROPERTY(Category = "Rich ComboBox|ViewModel", EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UObject> ViewModel = nullptr;
};
