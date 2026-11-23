/*****************************************************************//**
 * @file   BBMKeyMappingCategoryEntryViewModel.h
 * @brief  키 매핑 카테고리 ListView 단일 섹션용 MVVM 뷰모델 클래스 정의
 * @details 카테고리 이름(CategoryName), 배열 인덱스(CategoryIndex), 하위 UBBMKeyMappingEntryViewModel 배열을 소유하며
 *          하위 엔트리 InputChord 변경 시 카테고리·엔트리 인덱스를 포함해 상위 ViewModel로 재전파함
 * @date   2026-11-17
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Save/BBMSaveTypes.h"
#include "BBMKeyMappingCategoryEntryViewModel.generated.h"

/* 독립 전방 선언 */
class UBBMKeyMappingEntryViewModel;

/* 키 매핑 카테고리 내 엔트리 입력 조합 변경 대리자 선언 */
DECLARE_MULTICAST_DELEGATE_FourParams(FBBMOnCategoryKeyChordChangedNativeSignature, int32 /* CategoryIndex */, int32 /* EntryIndex */, FName /* MappingName */, const FInputChord& /* NewChord */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBBMOnCategoryKeyChordChangedDynamicSignature, int32, CategoryIndex, int32, EntryIndex, FName, MappingName, const FInputChord&, NewChord);

/**
 * 키 매핑 카테고리 섹션을 표현하는 MVVM 엔트리 ViewModel
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Key Mapping Category Entry ViewModel"))
class BBM_API UBBMKeyMappingCategoryEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 수명 주기 */
public:
	virtual void BeginDestroy() override;

	/* 초기화 및 조회·설정 (FieldNotify) */
public:
	/**
	 * 초기 생성 시 카테고리 매핑 구조체 데이터를 받아 하위 엔트리 VM 배열을 생성·바인딩함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetKeyMappings(const FBBMKeyCategoryMappingData& InCategoryMappingData);

	/* 엔트리 인덱스로 단일 키 갱신하고 엔트리 VM 동기화 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void RemapKey(int32 InEntryIndex, const FInputChord& InNewChord);

	/**
	 * 전체 카테고리 배열 내 인덱스를 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable, BlueprintPure, FieldNotify)
	int32 GetCategoryIndex() const { return CategoryIndex; }

	/**
	 * 전체 카테고리 배열 내 인덱스를 설정함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetCategoryIndex(int32 InCategoryIndex);

	/**
	 * 카테고리 표시 이름을 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable, BlueprintPure, FieldNotify)
	const FText& GetCategoryName() const { return CategoryName; }

	/**
	 * 카테고리 표시 이름을 설정함
	 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetCategoryName(const FText& InCategoryName);

	/**
	 * 소유 중인 하위 키 매핑 엔트리 ViewModel 배열 참조를 반환함
	 */
	const TArray<TObjectPtr<UBBMKeyMappingEntryViewModel>>& GetKeyMappingEntries() const { return KeyMappingEntries; }

	/* 키 맵 엔트리 배열을 새 목록으로 교체하고 대리자 재바인딩을 수행함 */
	void SetKeyMappingEntries(const TArray<TObjectPtr<UBBMKeyMappingEntryViewModel>>& InKeyMappingEntries);

	/* 키 설정 헬퍼 */
private:
	/* 모든 키 맵 엔트리를 제거 */
	void RemoveAllKeyMappingEntries();

	/* 모든 KeyMappingEntries VM의 커스텀 대리자에 바인딩함 */
	void RebindKeyMappingEntries();

	/** 하위 KeyMappingEntries VM의 대리자 바인딩을 전부 해제함 */
	void UnbindKeyMappingEntries();

	/* 내부 대리자 핸들러 */
private:
	void HandleEntryInputChordChanged(int32 InEntryIndex, FName InMappingName, const FInputChord& InNewChord);

	/* C++ 네이티브 및 블루프린트 커스텀 대리자 */
public:
	/* 하위 엔트리 InputChord 변경 시 CategoryIndex와 EntryIndex를 포함해 호출되는 C++ 대리자 */
	FBBMOnCategoryKeyChordChangedNativeSignature OnCategoryInputChordChangedNative;

	/* 하위 엔트리 InputChord 변경 시 CategoryIndex와 EntryIndex를 포함해 호출되는 블루프린트 대리자 */
	UPROPERTY(Category = "BBM|UI|Settings|Event", BlueprintAssignable)
	FBBMOnCategoryKeyChordChangedDynamicSignature OnCategoryInputChordChanged;

	/* 데이터 프로퍼티 (FieldNotify) 및 내부 상태 */
private:
	struct FKeyMappingEntryBinding
	{
		TWeakObjectPtr<UBBMKeyMappingEntryViewModel> Entry;
		FDelegateHandle ChordChangedHandle;
	};

	/* 전체 KeyMappings 배열 내 카테고리 인덱스 */
	UPROPERTY(Category = "BBM|UI|Settings", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetCategoryIndex, Getter = GetCategoryIndex, meta = (AllowPrivateAccess = "true"))
	int32 CategoryIndex = INDEX_NONE;

	/* UI 표시용 카테고리 이름 */
	UPROPERTY(Category = "BBM|UI|Settings", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetCategoryName, Getter = GetCategoryName, meta = (AllowPrivateAccess = "true"))
	FText CategoryName;

	/* 해당 카테고리에 소속된 키 매핑 엔트리 ViewModel 목록 (FieldNotify) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Setter = SetKeyMappingEntries, Getter = GetKeyMappingEntries, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UBBMKeyMappingEntryViewModel>> KeyMappingEntries;

	/* 하위 엔트리 VM 대리자 등록 핸들 목록 */
	TArray<FKeyMappingEntryBinding> KeyMappingEntryBindings;
};
