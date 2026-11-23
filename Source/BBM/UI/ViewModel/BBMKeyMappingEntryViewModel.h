/*****************************************************************//**
 * @file   BBMKeyMappingEntryViewModel.h
 * @brief  키 매핑 ListView 단일 항목용 MVVM 뷰모델 클래스 정의
 * @details 매핑 식별 키(MappingName), 표시 이름(DisplayName), 입력 키 조합(InputChord), 배열 인덱스(EntryIndex)를
 *          FieldNotify로 노출하고 InputChord Setter 변경 시 커스텀 대리자를 통해 상위 ViewModel에 통지함
 * @date   2026-11-17
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Save/BBMSaveTypes.h"
#include "BBMKeyMappingEntryViewModel.generated.h"

/* 키 매핑 엔트리 입력 조합 변경 대리자 선언 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FBBMOnKeyMappingChordChangedNativeSignature, int32 /* EntryIndex */, FName /* MappingName */, const FInputChord& /* NewChord */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBBMOnKeyMappingChordChangedDynamicSignature, int32, EntryIndex, FName, MappingName, const FInputChord&, NewChord);

/**
 * 키 매핑 ListView 단일 행을 표현하는 MVVM 엔트리 ViewModel
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Key Mapping Entry ViewModel"))
class BBM_API UBBMKeyMappingEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 초기화 및 조회·설정 (FieldNotify) */
public:
	/**
	 * 키 매핑 구조체 데이터(매핑명·표시 이름·입력 키 조합)를 일괄 설정함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetKeyMapping(const FBBMKeyMappingData& InKeyMappingData);

	/**
	 * 소속 카테고리 내 배열 인덱스를 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable, BlueprintPure, FieldNotify)
	int32 GetEntryIndex() const { return EntryIndex; }

	/**
	 * 소속 카테고리 내 배열 인덱스를 설정함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetEntryIndex(int32 InEntryIndex);

	/**
	 * 바인딩 대상 키 식별 이름(MappingName)을 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable, BlueprintPure, FieldNotify)
	FName GetMappingName() const { return MappingName; }

	/**
	 * 바인딩 대상 키 식별 이름(MappingName)을 설정함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetMappingName(FName InMappingName);

	/**
	 * UI에 시각적으로 표시할 항목 이름을 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable, BlueprintPure, FieldNotify)
	const FText& GetDisplayName() const { return DisplayName; }

	/**
	 * UI에 시각적으로 표시할 항목 이름을 설정함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetDisplayName(const FText& InDisplayName);

	/**
	 * 현재 할당된 입력 키 조합(FInputChord)을 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable, BlueprintPure, FieldNotify)
	const FInputChord& GetInputChord() const { return InputChord; }

	/**
	 * 입력 키 조합(FInputChord)을 설정하고 값 변경 시 FieldNotify 및 커스텀 대리자를 호출함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetInputChord(const FInputChord& InInputChord);

	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void RequestToChangeInputChord(const FInputChord& InInputChord);

	/* C++ 네이티브 및 블루프린트 커스텀 대리자 */
public:
	/* SetInputChord를 통해 입력 키 조합이 실제로 변경되었을 때 호출되는 C++ 대리자 */
	FBBMOnKeyMappingChordChangedNativeSignature OnInputChordChangedNative;

	/* SetInputChord를 통해 입력 키 조합이 실제로 변경되었을 때 호출되는 블루프린트 대리자 */
	UPROPERTY(Category = "BBM|UI|Settings|Event", BlueprintAssignable)
	FBBMOnKeyMappingChordChangedDynamicSignature OnInputChordChanged;

	/* 데이터 프로퍼티 (FieldNotify) */
private:
	/* 소속 카테고리 내 배열 인덱스 */
	UPROPERTY(Category = "BBM|UI|Settings", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetEntryIndex, Getter = GetEntryIndex, meta = (AllowPrivateAccess = "true"))
	int32 EntryIndex = INDEX_NONE;

	/* 바인딩 대상 IMC 매핑 이름 */
	UPROPERTY(Category = "BBM|UI|Settings", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetMappingName, Getter = GetMappingName, meta = (AllowPrivateAccess = "true"))
	FName MappingName;

	/* UI 표시용 액션 이름 */
	UPROPERTY(Category = "BBM|UI|Settings", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetDisplayName, Getter = GetDisplayName, meta = (AllowPrivateAccess = "true"))
	FText DisplayName;

	/* 대상 설정 입력 키 조합 */
	UPROPERTY(Category = "BBM|UI|Settings", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetInputChord, Getter = GetInputChord, meta = (AllowPrivateAccess = "true"))
	FInputChord InputChord;
};
