/*****************************************************************//**
 * @file   BBMTabEntryViewModel.h
 * @brief  탭 패널 내 개별 탭 버튼용 MVVM 엔트리 뷰모델 클래스 정의
 * @details 내부 텍스트(TabText), 선택 여부(bIsSelected), 탭 인덱스(TabIndex) FieldNotify와
 *          선택 시도(OnSelectRequested) 및 선택/해제 변경(OnSelected/OnDeselected) 대리자를 제공함
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BBMTabEntryViewModel.generated.h"

/* 탭 엔트리 인덱스 전달 대리자 선언 */
DECLARE_MULTICAST_DELEGATE_OneParam(FBBMOnTabEntryEventNativeSignature, int32 /*TabIndex*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBBMOnTabEntryEventDynamicSignature, int32, TabIndex);

/**
 * 개별 탭 버튼 UI에 바인딩되는 경량 MVVM 엔트리 뷰모델
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Tab Entry ViewModel"))
class BBM_API UBBMTabEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 조회 및 설정 (FieldNotify) */
public:
	/**
	 * 탭 버튼 표시 텍스트를 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|Tab")
	const FText& GetTabText() const { return TabText; }

	/**
	 * 탭 버튼 표시 텍스트를 설정함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void SetTabText(const FText& InTabText);

	/**
	 * 현재 탭의 선택 상태를 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|Tab")
	bool IsSelected() const { return bIsSelected; }

	/**
	 * 탭의 선택 상태를 설정하고 실제 변경 시 선택/해제 대리자를 호출함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void SetIsSelected(bool bInIsSelected);

	/**
	 * 탭 컨테이너 내 현재 인덱스 값을 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|Tab")
	int32 GetTabIndex() const { return TabIndex; }

	/**
	 * 탭 컨테이너 내 현재 인덱스 값을 설정함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void SetTabIndex(int32 InTabIndex);

	/* 사용자 입력 액션 */
public:
	/**
	 * 탭 선택 시도를 요청하여 OnSelectRequested 대리자를 자신의 TabIndex와 함께 브로드캐스트함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Tab")
	void RequestSelect();

	/* C++ 네이티브 대리자 */
public:
	/* 사용자가 이 탭의 선택을 시도할 때 자신의 TabIndex와 함께 호출됨 */
	FBBMOnTabEntryEventNativeSignature OnSelectRequestedNative;

	/* 선택됨(bIsSelected == true) 상태로 변경될 때 자신의 TabIndex와 함께 호출됨 */
	FBBMOnTabEntryEventNativeSignature OnSelectedNative;

	/* 선택 해제(bIsSelected == false) 상태로 변경될 때 자신의 TabIndex와 함께 호출됨 */
	FBBMOnTabEntryEventNativeSignature OnDeselectedNative;

	/* 블루프린트 연동용 동적 멀티캐스트 대리자 */
public:
	/* 사용자가 이 탭의 선택을 시도할 때 자신의 TabIndex와 함께 호출됨 */
	UPROPERTY(Category = "BBM|UI|Tab|Event", BlueprintAssignable)
	FBBMOnTabEntryEventDynamicSignature OnSelectRequested;

	/* 선택됨(bIsSelected == true) 상태로 변경될 때 자신의 TabIndex와 함께 호출됨 */
	UPROPERTY(Category = "BBM|UI|Tab|Event", BlueprintAssignable)
	FBBMOnTabEntryEventDynamicSignature OnSelected;

	/* 선택 해제(bIsSelected == false) 상태로 변경될 때 자신의 TabIndex와 함께 호출됨 */
	UPROPERTY(Category = "BBM|UI|Tab|Event", BlueprintAssignable)
	FBBMOnTabEntryEventDynamicSignature OnDeselected;

	/* 데이터 프로퍼티 */
private:
	/* 탭 버튼에 표시할 텍스트 */
	UPROPERTY(Category = "BBM|UI|Tab", EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetTabText, Getter = GetTabText, meta = (AllowPrivateAccess = "true"))
	FText TabText;

	/* 현재 탭이 선택된 상태인지 여부 */
	UPROPERTY(Category = "BBM|UI|Tab", BlueprintReadWrite, FieldNotify, Setter = SetIsSelected, Getter = IsSelected, meta = (AllowPrivateAccess = "true"))
	bool bIsSelected = false;

	/* 부모 탭 뷰모델 배열 내 인덱스 (미할당 시 INDEX_NONE) */
	UPROPERTY(Category = "BBM|UI|Tab", BlueprintReadWrite, FieldNotify, Setter = SetTabIndex, Getter = GetTabIndex, meta = (AllowPrivateAccess = "true"))
	int32 TabIndex = INDEX_NONE;
};
