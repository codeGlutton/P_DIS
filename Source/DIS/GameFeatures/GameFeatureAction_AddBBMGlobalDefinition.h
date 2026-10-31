/*****************************************************************//**
 * @file   GameFeatureAction_AddBBMGlobalDefinition.h
 * @brief  GameFeature 활성화 시 전역 정의 에셋 및 BBM 그룹 프로파일을 동적 주입하는 GameFeatureAction 클래스
 * @date   2026-10-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "Engine/World.h"
#include "Global/DISGlobalDefinition.h"
#include "GameFeatureAction_AddBBMGlobalDefinition.generated.h"

class UGameInstance;

/**
 * GameFeature 활성화/비활성화 시 전역 정의 에셋들을 UDISGlobalLogic 및 UBBMGlobalLogic에 동적으로 등록/해제하는 액션
 */
UCLASS(meta = (DisplayName = "Add BBM Global Definition"))
class DIS_API UGameFeatureAction_AddBBMGlobalDefinition : public UGameFeatureAction
{
	GENERATED_BODY()

	/* 액션 라이프사이클 */
public:
	UGameFeatureAction_AddBBMGlobalDefinition();

	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

	/* 게임인스턴스 등록/해제 내부 처리 */
protected:
	void AddToGameInstance(UGameInstance* TargetGameInstance, const TArray<TObjectPtr<const UDISGlobalDefinition>>& LoadedDefinitions);
	void RemoveFromGameInstance(UGameInstance* TargetGameInstance, const TArray<TObjectPtr<const UDISGlobalDefinition>>& LoadedDefinitions);

	void HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS, FGameFeatureStateChangeContext ChangeContext);

	/* 액션 설정 프로퍼티 */
public:
	/* 본 피쳐에서 주입할 전역 정의(희귀도/아이템/그룹 프로파일) DataAsset 목록 (UDISGlobalDefinition 및 UBBMGlobalDefinition 지원) */
	UPROPERTY(Category = "BBM", EditAnywhere, meta = (AssetBundles = "Data"))
	TArray<TSoftObjectPtr<const UDISGlobalDefinition>> GlobalDefinitions;

	/* GAS 싱글톤(AbilitySystemGlobals)에 동적으로 등록할 어트리뷰트 기본값 CurveTable/CompositeCurveTable 에셋 경로 목록 */
	UPROPERTY(Category = "BBM|Combat", EditAnywhere, meta = (AssetBundles = "Data"))
	TArray<FSoftObjectPath> AttributeDefaultTableNames;

	/* 활성화 컨텍스트 내부 상태 관리 */
private:
	struct FActiveContextData
	{
		/* 사전 로드된 전역 정의 에셋 인스턴스 보관 */
		TArray<TObjectPtr<const UDISGlobalDefinition>> LoadedDefinitions;

		/* 현재 등록이 적용된 GameInstance 목록 추적 */
		TArray<TWeakObjectPtr<UGameInstance>> ActiveGameInstances;

		/* 월드 초기화 감지용 델리게이트 핸들 */
		FDelegateHandle WorldInitializedHandle;
	};

	/* StateChangeContext별 활성화 데이터 매핑 */
	TMap<FGameFeatureStateChangeContext, FActiveContextData> ActiveContextDataMap;
};
