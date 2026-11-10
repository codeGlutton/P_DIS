/*****************************************************************//**
 * @file   AssetLinterValidator.h
 * @brief  DataValidation 프레임워크 기반 에셋 네이밍 및 위치 검증기 정의
 * @details UEditorValidatorBase를 상속받아 Most-Derived 상속 거리를 기반으로 에셋 유효성을 검사함
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "AssetLinterValidator.generated.h"

class UAssetLinterSettings;
struct FAssetNamingRule;
struct FAssetLocationRule;

/**
 * @class UAssetLinterValidator
 * @brief UEditorValidatorBase를 상속받아 프로젝트 설정에 정의된 스코프, 네이밍 규칙, 위치 규칙에 따라 에셋 유효성을 검사하는 검증기
 */
UCLASS()
class ASSETLINTER_API UAssetLinterValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UAssetLinterValidator();

	/* UEditorValidatorBase 검증 인터페이스 */
protected:
	/**
	 * 에셋이 검사 대상 스코프에 속하는지 여부를 판정합니다.
	 * @param InAssetData 대상 에셋 데이터
	 * @param InObject 대상 에셋 객체 (nullptr 가능)
	 * @param InContext 유효성 검사 컨텍스트
	 * @return 대상 스코프에 포함되면 true, 아니면 false
	 */
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	/**
	 * 로드된 에셋에 대해 Most-Derived 네이밍 규칙 및 위치 규칙을 검사합니다.
	 * @param InAssetData 대상 에셋 데이터
	 * @param InAsset 로드된 에셋 객체
	 * @param Context 유효성 검사 컨텍스트
	 * @return 검사 결과 (Valid, Invalid)
	 */
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

	/* 공용 검증 헬퍼 메서드 */
public:
	/**
	 * InAssetClass와 InRuleClass 간 상속 트리 단계 거리(Distance)를 계산합니다.
	 * @param InAssetClass 검사 대상 클래스
	 * @param InRuleClass 규칙에 정의된 상위 클래스
	 * @return 일치하면 0, 직계 부모이면 1..., 상속 관계가 아니면 -1
	 */
	static int32 CalculateClassDistance(const UClass* InAssetClass, const UClass* InRuleClass);

	/**
	 * 에셋의 후보 클래스 목록을 추출합니다 (일반 클래스 및 블루프린트 부모/생성 클래스 고려).
	 */
	static void GatherCandidateClasses(const FAssetData& InAssetData, const UObject* InAsset, TArray<const UClass*>& OutClasses);

	/**
	 * 등록된 네이밍 규칙 중 상속 거리상 가장 가까운(Most-Derived) 규칙을 탐색합니다.
	 */
	static const FAssetNamingRule* FindMostDerivedNamingRule(const TArray<const UClass*>& InCandidateClasses, const UAssetLinterSettings* InSettings);

	/**
	 * 등록된 위치 규칙 중 상속 거리상 가장 가까운(Most-Derived) 규칙을 탐색합니다.
	 */
	static const FAssetLocationRule* FindMostDerivedLocationRule(const TArray<const UClass*>& InCandidateClasses, const UAssetLinterSettings* InSettings);

	/**
	 * 주어진 패키지 경로가 규칙의 허용 디렉터리 중 하나와 일치하는지 검사합니다.
	 */
	static bool IsInAllowedDirectory(const FString& InPackagePath, const FAssetLocationRule& InRule);
};
