/*****************************************************************//**
 * @file   BBMComboBoxTextViewModel.h
 * @brief  URichComboBoxKey와 양방향 연동되는 모듈형 텍스트 콤보박스 MVVM 뷰모델 정의
 * @details AddComboBoxTextEntry/RemoveAllComboBoxTextEntries를 통해 FRichComboBoxKeyViewModelItem 배열을 관리하고,
 *          SelectedKey FieldNotify 양방향 바인딩 및 SelectedKey(FName) 변경 대리자를 제공함
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Types/RichComboBoxTypes.h"
#include "BBMComboBoxTextViewModel.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMComboBoxTextEntryViewModel;

/* 텍스트 콤보박스 SelectedKey 변경 통지용 네이티브/동적 대리자 (선택 키 전달) */
DECLARE_MULTICAST_DELEGATE_OneParam(FBBMOnComboBoxTextSelectedKeyChangedNativeSignature, FName /*SelectedKey*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBBMOnComboBoxTextSelectedKeyChangedDynamicSignature, FName, SelectedKey);

/**
 * URichComboBoxKey에 바인딩할 텍스트 엔트리 목록과 선택 키 상태를 캡슐화한 재사용 가능 MVVM 모듈
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM ComboBox Text ViewModel"))
class BBM_API UBBMComboBoxTextViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 항목 목록 및 선택 상태 조회/설정 */
public:
	/* URichComboBoxKey.ViewModelItems 바인딩용 키-엔트리 VM 배열 반환 */
	const TArray<FRichComboBoxKeyViewModelItem>& GetComboBoxItems() const { return ComboBoxItems; }

	/* URichComboBoxKey.SelectedKey 양방향 바인딩용 선택 키 반환 */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable, BlueprintPure)
	FName GetSelectedKey() const { return SelectedKey; }

	/* 선택 키 변경 및 대리자 발송 (동일 키 입력 시 무시) */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable)
	void SetSelectedKey(FName InSelectedKey);

	/* 항목 추가 및 일괄 제거 인터페이스 */
public:
	/* 새 텍스트 항목(UBBMComboBoxTextEntryViewModel)을 추가하고 ComboBoxItems를 갱신함 */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable)
	UBBMComboBoxTextEntryViewModel* AddComboBoxTextEntry(const FName& InKey, const FText& InDisplayText);

	/* 모든 텍스트 항목과 선택 상태를 초기화함 */
	UFUNCTION(Category = "BBM|UI|ComboBox", BlueprintCallable)
	void RemoveAllComboBoxTextEntries();

	/* C++ / 블루프린트 선택 변경 대리자 */
public:
	/* SelectedKey가 변경될 때 새 키를 전달하는 네이티브 대리자 */
	FBBMOnComboBoxTextSelectedKeyChangedNativeSignature OnSelectedKeyChangedNative;

	/* SelectedKey가 변경될 때 새 키를 전달하는 블루프린트 대리자 */
	UPROPERTY(Category = "BBM|UI|ComboBox|Event", BlueprintAssignable)
	FBBMOnComboBoxTextSelectedKeyChangedDynamicSignature OnSelectedKeyChanged;

	/* FieldNotify 바인딩 프로퍼티 */
private:
	/* URichComboBoxKey.ViewModelItems와 연결되는 키-ViewModel 배열 */
	UPROPERTY(Category = "BBM|UI|ComboBox", BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	TArray<FRichComboBoxKeyViewModelItem> ComboBoxItems;

	/* URichComboBoxKey.SelectedKey와 TwoWay 연결되는 선택 키 */
	UPROPERTY(Category = "BBM|UI|ComboBox", BlueprintReadWrite, FieldNotify, Getter, Setter, meta = (AllowPrivateAccess = "true"))
	FName SelectedKey = NAME_None;
};
