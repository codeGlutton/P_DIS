/*****************************************************************//**
 * @file   BBMAssetRenameActionUtility.h
 * @brief  에디터에서 선택한 에셋들의 이름을 일괄 갱신하는 에셋 액션 유틸리티 클래스 정의
 * @details Prefix 추가, Suffix 추가, 특정 문자 치환 등 3대 리네임 액션을 에셋 우클릭 컨텍스트 메뉴로 제공함
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AssetActionUtility.h"
#include "BBMAssetRenameActionUtility.generated.h"

/**
 * 에디터 콘텐츠 브라우저에서 선택한 에셋들의 이름을 일괄 변경하는 유틸리티
 * 콘텐츠 브라우저에서 하나 이상의 에셋을 선택하고 우클릭하여 'BBM Asset Tools' 메뉴에서 호출 가능
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Asset Rename Utility"))
class BBMEDITOR_API UBBMAssetRenameActionUtility : public UAssetActionUtility
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMAssetRenameActionUtility();

	/* 에셋 이름 일괄 갱신 액션 */
public:
	/**
	 * 선택한 모든 에셋의 이름 앞에 지정한 Prefix(접두사)를 추가함
	 * @param Prefix 에셋 이름 앞에 추가할 접두사 문자열 (예: "T_", "BP_")
	 */
	UFUNCTION(CallInEditor, Category = "BBM|Asset Tools", meta = (DisplayName = "Add Prefix to Assets", ToolTip = "선택한 에셋들의 이름 앞에 지정한 접두사를 추가합니다."))
	void AddPrefixToAssets(const FString& Prefix);

	/**
	 * 선택한 모든 에셋의 이름 뒤에 지정한 Suffix(접미사)를 추가함
	 * @param Suffix 에셋 이름 뒤에 추가할 접미사 문자열 (예: "_Normal", "_Inst")
	 */
	UFUNCTION(CallInEditor, Category = "BBM|Asset Tools", meta = (DisplayName = "Add Suffix to Assets", ToolTip = "선택한 에셋들의 이름 뒤에 지정한 접미사를 추가합니다."))
	void AddSuffixToAssets(const FString& Suffix);

	/**
	 * 선택한 모든 에셋의 이름에서 특정 문자열을 찾아 새 문자열로 대체(치환)함
	 * @param SearchText 찾을 대상 기존 문자열
	 * @param ReplacementText 대체할 새 문자열
	 * @param bIgnoreCase 대소문자 무시 여부 (기본 true)
	 */
	UFUNCTION(CallInEditor, Category = "BBM|Asset Tools", meta = (DisplayName = "Replace Characters in Assets", ToolTip = "선택한 에셋들의 이름에서 일치하는 문자열을 찾아 새 문자열로 교체합니다."))
	void ReplaceCharactersInAssets(const FString& SearchText, const FString& ReplacementText, bool bIgnoreCase = true);

	/* 내부 공통 리네임 처리 헬퍼 */
private:
	/**
	 * 선택된 에셋 목록을 순회하며 변환 함수에 따라 새 이름을 계산하고 일괄 리네임을 수행함
	 * @param ActionName 로그 표기용 액션 명칭
	 * @param NameTransformFunc 기존 에셋 이름을 받아 새 이름을 반환하는 람다 함수
	 */
	void ExecuteBatchRename(const FString& ActionName, TFunctionRef<FString(const FString&)> NameTransformFunc);
};

