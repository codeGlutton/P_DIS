/*****************************************************************//**
 * @file   AssetLinterActionUtility.h
 * @brief  콘텐츠 브라우저 선택 에셋 대상 린트 검증 및 일괄 수정 에셋 액션 유틸리티 선언
 * @details UAssetActionUtility를 상속받아 선택 에셋 검증 및 확인 모달 다이얼로그를 통한 원자적 일괄 수정을 제공함
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AssetActionUtility.h"
#include "AssetLinterTypes.h"
#include "AssetLinterActionUtility.generated.h"

/**
 * @class UAssetLinterActionUtility
 * @brief 콘텐츠 브라우저에서 선택한 에셋들에 대해 린트 유효성 검사 및 일괄 수정을 실행하는 에디터 유틸리티 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Asset Linter Action Utility"))
class ASSETLINTER_API UAssetLinterActionUtility : public UAssetActionUtility
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UAssetLinterActionUtility();

	/* 에셋 컨텍스트 메뉴 액션 메서드 */
public:
	/**
	 * 선택한 에셋들에 대해 린트 규칙(네이밍/위치) 유효성을 즉시 검사합니다.
	 */
	UFUNCTION(CallInEditor, Category = "Asset Linter", meta = (DisplayName = "Validate Selected Assets", ToolTip = "선택한 에셋들의 린트 규칙(네이밍 및 위치) 준수 여부를 검사합니다."))
	void ValidateSelectedAssets();

	/**
	 * 선택한 에셋들 중 린트 규칙을 위반한 항목을 수집하고, 원자적 검증 및 확인 팝업 UI를 거쳐 일괄 자동 수정합니다.
	 */
	UFUNCTION(CallInEditor, Category = "Asset Linter", meta = (DisplayName = "Batch Fix Selected Assets", ToolTip = "선택한 에셋들의 네이밍 및 위치를 확인 팝업 후 일괄 자동 수정합니다."))
	void BatchFixSelectedAssets();

	/* 내부 보정 산출 및 실행 메서드 */
private:
	/**
	 * 선택 에셋들에 대해 수정 제안 항목 목록 산출 및 원자적 유효성 검증을 수행합니다.
	 * @param InSelectedAssets 선택된 에셋 데이터 배열
	 * @param OutFixItems 산출된 수정 제안 항목 목록
	 * @param OutAtomicErrorMessage 위치 이동 불가 등으로 원자적 실패가 발생한 경우 에러 메시지
	 * @return 원자적 유효성을 통과하면 true, 위치 규칙 미비 등으로 원자적 실패가 발생하면 false
	 */
	bool BuildFixItems(const TArray<FAssetData>& InSelectedAssets, TArray<TSharedPtr<FAssetLintFixItem>>& OutFixItems, FText& OutAtomicErrorMessage);

	/**
	 * IAssetTools::RenameAssets를 호출하여 선택된 항목들을 일괄 리네임 및 위치 이동 처리합니다.
	 * @param InFixItems 수정 대상 항목 목록
	 */
	void ExecuteBatchFix(const TArray<TSharedPtr<FAssetLintFixItem>>& InFixItems);
};

