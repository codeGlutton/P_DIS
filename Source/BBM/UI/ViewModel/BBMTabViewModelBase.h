/*****************************************************************//**
 * @file   BBMTabViewModelBase.h
 * @brief  UWorldViewModelBase에서 파생되는 범용 MVVM 탭 패널 뷰모델 클래스 정의
 * @details TabEntries 배열과 SelectedTabIndex(FieldNotify), PreSelectedTabIndex를 관리하며
 *          탭 추가·제거·전체 제거·스위치 및 엔트리 대리자 자동 재바인딩과 상태 재갱신을 수행함
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "ViewModel/WorldViewModelBase.h"
#include "BBMTabViewModelBase.generated.h"

/* 독립 전방 선언 */
class UBBMTabEntryViewModel;

DECLARE_MULTICAST_DELEGATE_TwoParams(FBBMOnTabSelectionChangedNativeSignature, int32 /*OldTabIndex*/, int32 /*NewTabIndex*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBBMOnTabSelectionChangedDynamicSignature, int32, OldTabIndex, int32, NewTabIndex);

/**
 * 탭 버튼 엔트리 목록(TabEntries)과 현재 선택 인덱스(SelectedTabIndex)를 소유하고
 * 탭 전환에 따른 창 변경 상태를 관리하는 MVVM 뷰모델 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Tab ViewModel Base"))
class BBM_API UBBMTabViewModelBase : public UWorldViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMTabViewModelBase();

	/* 탭 목록 조회 및 일괄 갱신 */
public:
	/**
	 * 현재 보유 중인 탭 엔트리 뷰모델 배열 읽기 전용 참조를 반환함
	 */
	const TArray<TObjectPtr<UBBMTabEntryViewModel>>& GetTabEntries() const { return TabEntries; }

	/**
	 * 탭 엔트리 배열을 새 목록으로 교체하고 선택 인덱스 동기화 및 대리자 재바인딩을 수행함
	 */
	void SetTabEntries(const TArray<TObjectPtr<UBBMTabEntryViewModel>>& InTabEntries);

	/**
	 * 현재 선택된 탭 인덱스를 반환함 (보유 탭이 없으면 INDEX_NONE)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Tab")
	int32 GetSelectedTabIndex() const { return SelectedTabIndex; }

	/**
	 * 선택된 탭 인덱스를 변경하고 이전 탭 해제 및 새 탭 선택 처리를 수행함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void SetSelectedTabIndex(int32 InTabIndex);

	/**
	 * 직전에 선택되어 있던 탭 인덱스를 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Tab")
	int32 GetPreSelectedTabIndex() const { return PreSelectedTabIndex; }

	/**
	 * 현재 선택된 탭 엔트리 뷰모델을 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Tab")
	UBBMTabEntryViewModel* GetSelectedTabEntry() const;

	/**
	 * 현재 탭 개수를 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Tab")
	int32 GetTabCount() const { return TabEntries.Num(); }

	/* 탭 조작 인터페이스 (선택·추가·제거·스위치·재갱신) */
public:
	/**
	 * 지정 인덱스의 탭을 선택함 (현재 SelectedTabIndex와 동일하면 무시)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void SelectTab(int32 InTabIndex);

	/**
	 * 최상위 인덱스(배열 맨 뒤)에 탭 엔트리를 추가하고 추가된 탭으로 SelectedTabIndex를 전환함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	int32 AddTab(UBBMTabEntryViewModel* InEntry);

	/**
	 * 지정 텍스트를 가진 새 UBBMTabEntryViewModel을 생성해 맨 뒤에 추가하고 자동 선택함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	UBBMTabEntryViewModel* AddTabWithText(const FText& InTabText);

	/**
	 * 지정 인덱스의 탭을 제거함 (선택 중인 탭 제거 시 PreSelectedTabIndex로 복귀, 남은 탭이 없으면 INDEX_NONE)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	bool RemoveTabAt(int32 InTabIndex);

	/**
	 * 지정 엔트리 인스턴스에 해당하는 탭을 제거함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	bool RemoveTab(UBBMTabEntryViewModel* InEntry);

	/**
	 * 모든 탭을 제거하고 SelectedTabIndex와 PreSelectedTabIndex를 INDEX_NONE으로 초기화함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void RemoveAllTabs();

	/**
	 * 두 인덱스의 탭 위치를 맞바꾸고 SelectedTabIndex와 PreSelectedTabIndex도 엔트리를 따라 동반 이동시킴
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	bool SwapTabs(int32 InIndexA, int32 InIndexB);

	/* ViewModel 수명 주기 및 파생 클래스 확장 훅 */
protected:
	virtual void EndViewModel() override;

	/**
	 * 유효한 탭 선택 변경이 완료된 직후 호출되는 C++ 가상 훅
	 */
	virtual void OnSelectedTabChanged(int32 InOldTabIndex, int32 InNewTabIndex);

	/* 내부 대리자 핸들러 및 동기화 헬퍼 */
private:
	void HandleEntrySelectRequested(int32 InTabIndex);

	void UpdateSelectedAndPreSelectedIndices(int32 InNewSelectedIndex, int32 InNewPreSelectedIndex);
	void SyncEntriesIndexAndSelection();
	void RebindTabEntries();
	void UnbindTabEntries();

	static int32 ShiftIndexAfterRemoval(int32 InTargetIndex, int32 InRemovedIndex);
	static int32 RemapIndexAfterSwap(int32 InTargetIndex, int32 InIndexA, int32 InIndexB);

	/* C++ 및 블루프린트 선택 변경 이벤트 */
public:
	/* SelectedTabIndex 변경 시 이전·신규 인덱스와 함께 발송되는 C++ 대리자 */
	FBBMOnTabSelectionChangedNativeSignature OnTabSelectionChangedNative;

	/* SelectedTabIndex 변경 시 이전·신규 인덱스와 함께 발송되는 블루프린트 대리자 */
	UPROPERTY(Category = "BBM|UI|Tab|Event", BlueprintAssignable)
	FBBMOnTabSelectionChangedDynamicSignature OnTabSelectionChanged;

	/* 멤버 프로퍼티 */
private:
	struct FTabEntryBinding
	{
		TWeakObjectPtr<UBBMTabEntryViewModel> Entry;
		FDelegateHandle SelectRequestedHandle;
	};

	/* 소유 중인 탭 버튼 엔트리 뷰모델 목록 (FieldNotify) */
	UPROPERTY(Category = "BBM|UI|Tab", BlueprintReadOnly, FieldNotify, Setter = SetTabEntries, Getter = GetTabEntries, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UBBMTabEntryViewModel>> TabEntries;

	/* 현재 선택된 탭 인덱스 (탭이 없을 때만 INDEX_NONE) */
	UPROPERTY(Category = "BBM|UI|Tab", BlueprintReadWrite, FieldNotify, Setter = SetSelectedTabIndex, Getter = GetSelectedTabIndex, meta = (AllowPrivateAccess = "true"))
	int32 SelectedTabIndex = INDEX_NONE;

	/* 직전에 선택되어 있던 탭 인덱스 (이전 선택 이력이 없으면 INDEX_NONE) */
	UPROPERTY(Category = "BBM|UI|Tab", BlueprintReadOnly, FieldNotify, Getter = GetPreSelectedTabIndex, meta = (AllowPrivateAccess = "true"))
	int32 PreSelectedTabIndex = INDEX_NONE;

	/* 현재 엔트리 VM들에 바인딩된 선택 요청 대리자 핸들 목록 */
	TArray<FTabEntryBinding> EntryBindings;
};
