#include "GameFeatureAction_AddBBMGlobalDefinition.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalLogic.h"
#include "Global/BBMGlobalDefinition.h"
#include "Global/BBMGlobalLogic.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

UGameFeatureAction_AddBBMGlobalDefinition::UGameFeatureAction_AddBBMGlobalDefinition()
{
}

void UGameFeatureAction_AddBBMGlobalDefinition::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FActiveContextData& ContextData = ActiveContextDataMap.FindOrAdd(Context);

	// 1. 설정된 UDISGlobalDefinition 소프트 참조 에셋들을 동기 로드
	ContextData.LoadedDefinitions.Reset();
	ContextData.LoadedDefinitions.Reserve(GlobalDefinitions.Num());

	for (const TSoftObjectPtr<const UDISGlobalDefinition>& SoftDefinition : GlobalDefinitions)
	{
		if (SoftDefinition.IsNull() == true)
		{
			continue;
		}

		const UDISGlobalDefinition* LoadedDef = SoftDefinition.LoadSynchronous();
		if (LoadedDef != nullptr)
		{
			ContextData.LoadedDefinitions.Add(LoadedDef);
		}
	}

	// 2. 현재 실행 중인 모든 유효 GameInstance에 정의 등록
	if (GEngine != nullptr)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (GameInstance != nullptr)
			{
				AddToGameInstance(GameInstance, ContextData.LoadedDefinitions);
				ContextData.ActiveGameInstances.AddUnique(GameInstance);
			}
		}
	}

	// 3. 향후 새로운 월드/게임 세션이 시작될 때 자동 등록할 수 있도록 델리게이트 바인딩
	const FGameFeatureStateChangeContext StateChangeContext = Context;
	ContextData.WorldInitializedHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this,
		&UGameFeatureAction_AddBBMGlobalDefinition::HandleWorldInitialized,
		StateChangeContext
	);

	// 4. GAS 전역 싱글톤(AbilitySystemGlobals)에 AttributeDefault CurveTable 동적 등록
	if (AttributeDefaultTableNames.Num() > 0)
	{
		UAbilitySystemGlobals::Get().AddAttributeDefaultTables(GetFName(), AttributeDefaultTableNames);
	}
}

void UGameFeatureAction_AddBBMGlobalDefinition::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	// 1. GAS 전역 싱글톤(AbilitySystemGlobals)에서 등록했던 AttributeDefault CurveTable 동적 해제
	if (AttributeDefaultTableNames.Num() > 0)
	{
		UAbilitySystemGlobals::Get().RemoveAttributeDefaultTables(GetFName(), AttributeDefaultTableNames);
	}

	FActiveContextData* ContextDataPtr = ActiveContextDataMap.Find(Context);
	if (ContextDataPtr == nullptr)
	{
		return;
	}

	// 1. 월드 초기화 감지 델리게이트 해제
	if (ContextDataPtr->WorldInitializedHandle.IsValid() == true)
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(ContextDataPtr->WorldInitializedHandle);
		ContextDataPtr->WorldInitializedHandle.Reset();
	}

	// 2. 등록되어 있던 모든 GameInstance에서 정의 해제
	for (const TWeakObjectPtr<UGameInstance>& WeakInstance : ContextDataPtr->ActiveGameInstances)
	{
		UGameInstance* GameInstance = WeakInstance.Get();
		if (GameInstance != nullptr)
		{
			RemoveFromGameInstance(GameInstance, ContextDataPtr->LoadedDefinitions);
		}
	}

	// 3. 컨텍스트 데이터 정리
	ContextDataPtr->ActiveGameInstances.Empty();
	ContextDataPtr->LoadedDefinitions.Empty();
	ActiveContextDataMap.Remove(Context);
}

void UGameFeatureAction_AddBBMGlobalDefinition::AddToGameInstance(UGameInstance* TargetGameInstance, const TArray<TObjectPtr<const UDISGlobalDefinition>>& LoadedDefinitions)
{
	if (TargetGameInstance == nullptr)
	{
		return;
	}

	UDISGlobalLogic* GlobalLogic = UDISGlobalSubsystem::GetLogic(TargetGameInstance);
	if (GlobalLogic == nullptr)
	{
		return;
	}

	UBBMGlobalLogic* BBMLogic = Cast<UBBMGlobalLogic>(GlobalLogic);

	for (const TObjectPtr<const UDISGlobalDefinition>& Definition : LoadedDefinitions)
	{
		if (Definition != nullptr)
		{
			// 기본 전역 정의 등록
			GlobalLogic->RegisterGlobalDefinition(Definition);

			// BBM 확장 정의 에셋인 경우 아이템 그룹 프로파일도 함께 등록
			if (BBMLogic != nullptr)
			{
				if (const UBBMGlobalDefinition* BBMDef = Cast<UBBMGlobalDefinition>(Definition.Get()))
				{
					BBMLogic->RegisterBBMGlobalDefinition(BBMDef);
				}
			}
		}
	}
}

void UGameFeatureAction_AddBBMGlobalDefinition::RemoveFromGameInstance(UGameInstance* TargetGameInstance, const TArray<TObjectPtr<const UDISGlobalDefinition>>& LoadedDefinitions)
{
	if (TargetGameInstance == nullptr)
	{
		return;
	}

	UDISGlobalLogic* GlobalLogic = UDISGlobalSubsystem::GetLogic(TargetGameInstance);
	if (GlobalLogic == nullptr)
	{
		return;
	}

	UBBMGlobalLogic* BBMLogic = Cast<UBBMGlobalLogic>(GlobalLogic);

	for (const TObjectPtr<const UDISGlobalDefinition>& Definition : LoadedDefinitions)
	{
		if (Definition != nullptr)
		{
			// BBM 확장 정의 에셋인 경우 아이템 그룹 프로파일 먼저 해제
			if (BBMLogic != nullptr)
			{
				if (const UBBMGlobalDefinition* BBMDef = Cast<UBBMGlobalDefinition>(Definition.Get()))
				{
					BBMLogic->UnregisterBBMGlobalDefinition(BBMDef);
				}
			}

			// 기본 전역 정의 해제
			GlobalLogic->UnregisterGlobalDefinition(Definition);
		}
	}
}

void UGameFeatureAction_AddBBMGlobalDefinition::HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS, FGameFeatureStateChangeContext ChangeContext)
{
	if (World == nullptr || World->IsGameWorld() == false)
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	FActiveContextData* ContextDataPtr = ActiveContextDataMap.Find(ChangeContext);
	if (ContextDataPtr == nullptr)
	{
		return;
	}

	AddToGameInstance(GameInstance, ContextDataPtr->LoadedDefinitions);
	ContextDataPtr->ActiveGameInstances.AddUnique(GameInstance);
}
