/*****************************************************************//**
 * @file   BBMStageSubsystem.cpp
 * @brief  BBM(비트 배낭 상인) 5회 만남 및 지역(Region) 진행 총괄 서브시스템 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Stage/BBMStageSubsystem.h"
#include "Stage/BBMRegionDefinition.h"
#include "Stage/BBMEncounterDefinition.h"
#include "Stage/BBMEventNpcBase.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemProfileTypes.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/BBMGlobalLogic.h"
#include "Level/BBMWorldSettings.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Combat/BBMCombatManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/AssetManager.h"
#include "DISAssetBundles.h"
#include "GameFramework/PlayerController.h"

UBBMStageSubsystem::UBBMStageSubsystem() :
	CurrentRegion(nullptr),
	CurrentEncounterIndex(0)
{
}

void UBBMStageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StageRngStream.GenerateNewSeed();
}

void UBBMStageSubsystem::Deinitialize()
{
	if (CurrentEncounterStreamableHandle.IsValid() == true)
	{
		CurrentEncounterStreamableHandle->CancelHandle();
		CurrentEncounterStreamableHandle.Reset();
	}
	Super::Deinitialize();
}

bool UBBMStageSubsystem::StartNewRun(const UBBMPlayerCharacterDefinition* PlayerDef)
{
	if (PlayerDef == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMStageSubsystem::StartNewRun] PlayerDef is nullptr."));
		return false;
	}

	const UBBMRegionDefinition* StartRegion = PlayerDef->GetStartRegionDefinition();
	if (StartRegion == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMStageSubsystem::StartNewRun] PlayerDef->StartRegionDefinition is nullptr."));
		return false;
	}

	StageRngStream.GenerateNewSeed();
	CurrentRegion = StartRegion;
	CurrentEncounterIndex = 1;

	// 5회 만남 시퀀스 및 다형성 런타임 데이터 생성
	GenerateRuntimeEncounters(CurrentRegion.Get());

	// 1번 만남 에셋 비동기 사전 로드(Preload) 트리거 (초기 진입 시 팝인 방지)
	PreloadEncounterAssets(CurrentEncounterIndex);

	// GameDataSubsystem 초기 런 상태 동기화
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBBMGameDataSubsystem* GameData = GI->GetSubsystem<UBBMGameDataSubsystem>())
		{
			TArray<FPrimaryAssetId> PlannedIds;
			for (const TInstancedStruct<FBBMRuntimeEncounterData>& EncInst : PlannedEncounters)
			{
				if (EncInst.IsValid() == true)
				{
					const FBBMRuntimeEncounterData& BaseData = EncInst.Get<FBBMRuntimeEncounterData>();
					if (const UBBMEncounterDefinition* LoadedDef = BaseData.EncounterDefinition.LoadSynchronous())
					{
						PlannedIds.Add(LoadedDef->GetPrimaryAssetId());
					}
				}
			}

			GameData->ExtractLevelData(
				nullptr,
				CurrentRegion->GetPrimaryAssetId(),
				CurrentEncounterIndex,
				PlannedIds,
				StageRngStream.GetCurrentSeed(),
				PlannedEncounters
			);
		}
	}

	OnEncounterChanged.Broadcast(GetCurrentEncounter());
	return true;
}

const UBBMRegionDefinition* UBBMStageSubsystem::GetCurrentRegion() const
{
	return CurrentRegion.Get();
}

int32 UBBMStageSubsystem::GetCurrentEncounterIndex() const
{
	return CurrentEncounterIndex;
}

const UBBMEncounterDefinition* UBBMStageSubsystem::GetCurrentEncounter() const
{
	const int32 ArrayIdx = CurrentEncounterIndex - 1;
	if (PlannedEncounters.IsValidIndex(ArrayIdx) == true && PlannedEncounters[ArrayIdx].IsValid() == true)
	{
		const FBBMRuntimeEncounterData& BaseData = PlannedEncounters[ArrayIdx].Get<FBBMRuntimeEncounterData>();
		return BaseData.EncounterDefinition.LoadSynchronous();
	}
	return nullptr;
}

const FBBMRuntimeEncounterData* UBBMStageSubsystem::GetCurrentRuntimeEncounterData() const
{
	const int32 ArrayIdx = CurrentEncounterIndex - 1;
	if (PlannedEncounters.IsValidIndex(ArrayIdx) == true && PlannedEncounters[ArrayIdx].IsValid() == true)
	{
		return &PlannedEncounters[ArrayIdx].Get<FBBMRuntimeEncounterData>();
	}
	return nullptr;
}

const TArray<TInstancedStruct<FBBMRuntimeEncounterData>>& UBBMStageSubsystem::GetPlannedEncounters() const
{
	return PlannedEncounters;
}

int32 UBBMStageSubsystem::GetPlannedEncountersCount() const
{
	return PlannedEncounters.Num();
}

bool UBBMStageSubsystem::IsCurrentRegionCompleted() const
{
	return CurrentEncounterIndex > 5;
}

bool UBBMStageSubsystem::PreloadEncounterAssets(int32 EncounterIndex)
{
	return PreloadEncounterAssets(EncounterIndex, FSimpleDelegate());
}

bool UBBMStageSubsystem::PreloadEncounterAssets(int32 EncounterIndex, FSimpleDelegate OnComplete)
{
	const int32 ArrayIdx = EncounterIndex - 1;
	if (PlannedEncounters.IsValidIndex(ArrayIdx) == false || PlannedEncounters[ArrayIdx].IsValid() == false)
	{
		OnComplete.ExecuteIfBound();
		return false;
	}

	const FBBMRuntimeEncounterData& RuntimeData = PlannedEncounters[ArrayIdx].Get<FBBMRuntimeEncounterData>();
	TArray<FPrimaryAssetId> AssetIdsToLoad;
	RuntimeData.CollectPrimaryAssetIds(AssetIdsToLoad);

	if (AssetIdsToLoad.IsEmpty() == true)
	{
		OnComplete.ExecuteIfBound();
		return true;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager == nullptr)
	{
		OnComplete.ExecuteIfBound();
		return true;
	}

	// Data, World, UI 번들을 포함하여 비동기 프리로드 실행 (실제 방 전환 시 팝인 방지)
	const TArray<FName> BundlesToLoad = { DISAssetBundles::Data, DISAssetBundles::World, DISAssetBundles::UI };
	FStreamableDelegate StreamableDelegate;
	if (OnComplete.IsBound() == true)
	{
		StreamableDelegate.BindLambda([OnComplete]()
		{
			OnComplete.ExecuteIfBound();
		});
	}

	CurrentEncounterStreamableHandle = AssetManager->LoadPrimaryAssets(AssetIdsToLoad, BundlesToLoad, StreamableDelegate);
	return true;
}

TSharedPtr<FStreamableHandle> UBBMStageSubsystem::GetCurrentEncounterStreamableHandle() const
{
	return CurrentEncounterStreamableHandle;
}

void UBBMStageSubsystem::AdvanceToNextEncounter(ABBMPlayerMerchantBase* CurrentMerchant)
{
	// 1단계: 트랜지션 UI 표시 및 Fade-out 애니메이션 브로드캐스트
	OnTransitionStarted.Broadcast();

	const int32 NextEncounterIndex = CurrentEncounterIndex + 1;

	// 2단계: 다음 인카운터가 유효한 범위(<= 5)인 경우 비동기 프리로드를 즉시 실행하여 저장과 병렬 수행
	if (NextEncounterIndex <= 5)
	{
		PreloadEncounterAssets(NextEncounterIndex);
	}

	// 3단계: 현재 레벨 객체 데이터 추출 및 비동기 저장
	UBBMGameDataSubsystem* GameData = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		GameData = GI->GetSubsystem<UBBMGameDataSubsystem>();
	}

	TArray<FPrimaryAssetId> PlannedIds;
	for (const TInstancedStruct<FBBMRuntimeEncounterData>& EncInst : PlannedEncounters)
	{
		if (EncInst.IsValid() == true)
		{
			const FBBMRuntimeEncounterData& BaseData = EncInst.Get<FBBMRuntimeEncounterData>();
			if (const UBBMEncounterDefinition* LoadedDef = BaseData.EncounterDefinition.LoadSynchronous())
			{
				PlannedIds.Add(LoadedDef->GetPrimaryAssetId());
			}
		}
	}

	auto FinalizeTransition = [this, CurrentMerchant, NextEncounterIndex]()
	{
		CurrentEncounterIndex = NextEncounterIndex;

		// 5회 만남 완료 여부 분기
		if (CurrentEncounterIndex > 5)
		{
			// 지역 클리어 및 다음 분기 선택 대기
			OnRegionCompleted.Broadcast(CurrentRegion.Get());
			OnTransitionFinished.Broadcast();
		}
		else
		{
			// 동일 레벨 내 다음 만남 스폰/카메라 적용 (사전 프리로드로 팝인 없음)
			ApplyCurrentEncounterToLevel(CurrentMerchant);
			OnEncounterChanged.Broadcast(GetCurrentEncounter());
			OnTransitionFinished.Broadcast();
		}
	};

	if (GameData != nullptr)
	{
		const FPrimaryAssetId RegionId = (CurrentRegion != nullptr) ? CurrentRegion->GetPrimaryAssetId() : FPrimaryAssetId();
		GameData->ExtractLevelData(
			CurrentMerchant,
			RegionId,
			NextEncounterIndex,
			PlannedIds,
			StageRngStream.GetCurrentSeed(),
			PlannedEncounters
		);

		// 4단계: 비동기 파일 저장 완료 대기 후 프리로드 완료 상태와 결합하여 전환 마무리
		FSimpleDelegate OnSaveComplete;
		OnSaveComplete.BindWeakLambda(this, [this, FinalizeTransition]()
		{
			if (CurrentEncounterStreamableHandle.IsValid() == true && CurrentEncounterStreamableHandle->HasLoadCompleted() == false)
			{
				const bool bBound = CurrentEncounterStreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, [FinalizeTransition]()
				{
					FinalizeTransition();
				}));

				if (bBound == false)
				{
					FinalizeTransition();
				}
			}
			else
			{
				FinalizeTransition();
			}
		});

		GameData->SaveCurrentRunAsync(OnSaveComplete);
	}
	else
	{
		// GameDataSubsystem이 없는 경우 프리로드 완료 후 즉시 전환
		if (CurrentEncounterStreamableHandle.IsValid() == true && CurrentEncounterStreamableHandle->HasLoadCompleted() == false)
		{
			const bool bBound = CurrentEncounterStreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, [FinalizeTransition]()
			{
				FinalizeTransition();
			}));

			if (bBound == false)
			{
				FinalizeTransition();
			}
		}
		else
		{
			FinalizeTransition();
		}
	}
}

bool UBBMStageSubsystem::ApplyCurrentEncounterToLevel(ABBMPlayerMerchantBase* CurrentMerchant)
{
	const UBBMEncounterDefinition* Encounter = GetCurrentEncounter();
	if (Encounter == nullptr || CurrentMerchant == nullptr)
	{
		return false;
	}

	const ABBMWorldSettings* WorldSettings = ABBMWorldSettings::GetBBMWorldSettings(CurrentMerchant);
	if (WorldSettings == nullptr)
	{
		return false;
	}

	const FName& SpawnKey = Encounter->GetSpawnSettingKey();
	AActor* CameraActor = WorldSettings->GetMainCameraPoint(SpawnKey);
	AActor* StartPointActor = WorldSettings->GetRoomStartPoint(SpawnKey);

	// 카메라 뷰타깃 전환
	if (CameraActor != nullptr)
	{
		if (APlayerController* PC = Cast<APlayerController>(CurrentMerchant->GetController()))
		{
			PC->SetViewTargetWithBlend(CameraActor, 0.0f);
		}
	}

	// 플레이어 폰을 해당 룸 시작 지점으로 텔레포트
	if (StartPointActor != nullptr)
	{
		CurrentMerchant->SetActorLocationAndRotation(
			StartPointActor->GetActorLocation(),
			StartPointActor->GetActorRotation(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);
	}

	// WorldSubsystem을 통해 해당 만남의 인카운터 액터(보물 상자 / NPC / 몬스터) 스폰 및 1D 타일 진형 배치 위임
	if (UWorld* World = CurrentMerchant->GetWorld())
	{
		if (UBBMCombatManagerSubsystem* CombatMgr = World->GetSubsystem<UBBMCombatManagerSubsystem>())
		{
			CombatMgr->SetupEncounter(GetCurrentRuntimeEncounterData(), CurrentMerchant);
		}
	}

	return true;
}

TArray<UBBMRegionDefinition*> UBBMStageSubsystem::GetNextRegionOptions(int32 MaxOptions) const
{
	if (CurrentRegion != nullptr)
	{
		return CurrentRegion->GetNextRegionOptions(MaxOptions, StageRngStream);
	}
	return TArray<UBBMRegionDefinition*>();
}

bool UBBMStageSubsystem::SelectNextRegion(const UBBMRegionDefinition* InChosenRegion, ABBMPlayerMerchantBase* CurrentMerchant)
{
	if (InChosenRegion == nullptr)
	{
		return false;
	}

	CurrentRegion = InChosenRegion;
	CurrentEncounterIndex = 1;

	// 신규 5회 만남 시퀀스 및 다형성 런타임 데이터 생성
	GenerateRuntimeEncounters(CurrentRegion.Get());

	// 신규 지역 1번 만남 에셋 비동기 사전 로드(Preload) 트리거 (초기 진입 시 팝인 방지)
	PreloadEncounterAssets(CurrentEncounterIndex);

	// 새 레벨 적용
	if (CurrentMerchant != nullptr)
	{
		ApplyCurrentEncounterToLevel(CurrentMerchant);
	}

	OnEncounterChanged.Broadcast(GetCurrentEncounter());
	return true;
}

void UBBMStageSubsystem::GenerateRuntimeEncounters(const UBBMRegionDefinition* InRegion)
{
	PlannedEncounters.Empty();
	if (InRegion == nullptr)
	{
		return;
	}

	// 1. 5회 만남 시퀀스 정의 생성
	TArray<UBBMEncounterDefinition*> GeneratedEncounters = InRegion->GenerateFiveEncounters(StageRngStream);

	// 2. 전역 로직 서브오브젝트 획득 및 유효 희귀도 가중치/NPC 요구 아이템 풀 획득
	UBBMGlobalLogic* BBMGlobal = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDISGlobalSubsystem* GlobalSubsystem = GI->GetSubsystem<UDISGlobalSubsystem>())
		{
			BBMGlobal = GlobalSubsystem->GetLogic<UBBMGlobalLogic>();
		}
	}

	const TMap<FGameplayTag, float> EffectiveRarityWeights = InRegion->GetEffectiveRarityWeights();
	TArray<FGameplayTag> EffectiveNpcTypes = InRegion->GetEffectiveNpcItemTypePool(BBMGlobal);

	// NPC 요구 카테고리 후보군이 3개 미만일 경우 표준 태그로 안전 보충
	if (EffectiveNpcTypes.Num() < 3)
	{
		EffectiveNpcTypes.AddUnique(BBMItemTypeTags::Item_Weapon_OneHandSword);
		EffectiveNpcTypes.AddUnique(BBMItemTypeTags::Item_Weapon_Shield);
		EffectiveNpcTypes.AddUnique(BBMItemTypeTags::Item_Weapon_Bow);
		EffectiveNpcTypes.AddUnique(BBMItemTypeTags::Item_Equipment_Clothes);
		EffectiveNpcTypes.AddUnique(BBMItemTypeTags::Item_Equipment_Gloves);
		EffectiveNpcTypes.AddUnique(BBMItemTypeTags::Item_Consumable_Potion);
	}

	// 3. 인카운터별 다형성 런타임 데이터 인스턴스 생성
	for (UBBMEncounterDefinition* EncDef : GeneratedEncounters)
	{
		if (EncDef == nullptr)
		{
			continue;
		}

		const EBBMEncounterType EncType = EncDef->GetEncounterType();
		if (EncType == EBBMEncounterType::Treasure)
		{
			TInstancedStruct<FBBMRuntimeEncounterData> StructInst = TInstancedStruct<FBBMRuntimeEncounterData>::Make<FBBMRuntimeTreasureEncounterData>();
			FBBMRuntimeTreasureEncounterData& TreasureData = StructInst.GetMutable<FBBMRuntimeTreasureEncounterData>();
			TreasureData.EncounterDefinition = EncDef;

			// 보물 방: 3개 아이템 중복 없이 가중치 추첨
			TArray<UBBMItemDefinition*> PickedItems = InRegion->PickRandomItemsWithoutDuplicates(
				InRegion->AvailableItems,
				EffectiveRarityWeights,
				3,
				StageRngStream
			);

			for (UBBMItemDefinition* Item : PickedItems)
			{
				if (Item != nullptr)
				{
					TreasureData.TreasureOptions.Add(Item);
				}
			}

			PlannedEncounters.Add(MoveTemp(StructInst));
		}
		else if (EncType == EBBMEncounterType::Combat || EncType == EBBMEncounterType::BossCombat)
		{
			TInstancedStruct<FBBMRuntimeEncounterData> StructInst = TInstancedStruct<FBBMRuntimeEncounterData>::Make<FBBMRuntimeCombatEncounterData>();
			FBBMRuntimeCombatEncounterData& CombatData = StructInst.GetMutable<FBBMRuntimeCombatEncounterData>();
			CombatData.EncounterDefinition = EncDef;

			// 전투 방: 스폰될 AI 몬스터들의 DropItemPool에서 보상 아이템 사전 추첨
			if (const UBBMCombatEncounterDefinition* CombatDef = Cast<UBBMCombatEncounterDefinition>(EncDef))
			{
				for (const TSoftObjectPtr<UBBMAICombatantDefinition>& SoftMonster : CombatDef->GetMonsterList())
				{
					if (const UBBMAICombatantDefinition* MonsterDef = SoftMonster.LoadSynchronous())
					{
						if (UBBMItemDefinition* DropItem = MonsterDef->PickRandomDropItem(EffectiveRarityWeights, StageRngStream))
						{
							CombatData.CombatRewardItems.Add(DropItem);
						}
					}
				}
			}

			PlannedEncounters.Add(MoveTemp(StructInst));
		}
		else if (EncType == EBBMEncounterType::Event)
		{
			TInstancedStruct<FBBMRuntimeEncounterData> StructInst = TInstancedStruct<FBBMRuntimeEncounterData>::Make<FBBMRuntimeEventEncounterData>();
			FBBMRuntimeEventEncounterData& EventData = StructInst.GetMutable<FBBMRuntimeEventEncounterData>();
			EventData.EncounterDefinition = EncDef;

			const UBBMEventEncounterDefinition* EventDef = Cast<UBBMEventEncounterDefinition>(EncDef);
			TArray<TSoftClassPtr<ABBMEventNpcBase>> NpcClasses = EventDef ? EventDef->GetNpcClasses() : TArray<TSoftClassPtr<ABBMEventNpcBase>>();

			// NPC 요구 아이템 타입 3종 무작위 셔플 (중복 배제)
			TArray<FGameplayTag> ShuffledTypes = EffectiveNpcTypes;
			for (int32 i = ShuffledTypes.Num() - 1; i > 0; --i)
			{
				const int32 j = StageRngStream.RandRange(0, i);
				ShuffledTypes.Swap(i, j);
			}

			// 지역 기본 아이템 풀 + 외부 유입 아이템 풀 통합
			TArray<TSoftObjectPtr<UBBMItemDefinition>> CombinedPool = InRegion->AvailableItems;
			CombinedPool.Append(InRegion->InflowItems);

			// 3명의 NPC 데이터 구성
			for (int32 NpcIdx = 0; NpcIdx < 3; ++NpcIdx)
			{
				FBBMRuntimeNpcData NpcData;
				if (NpcClasses.IsValidIndex(NpcIdx))
				{
					NpcData.NpcClass = NpcClasses[NpcIdx];
				}

				// 희귀도 확률 기반 소지 아이템 1개 추첨
				UBBMItemDefinition* Offered = InRegion->PickRandomItem(CombinedPool, EffectiveRarityWeights, StageRngStream);
				if (Offered != nullptr)
				{
					NpcData.OfferedItem = Offered;
				}

				// 중복 없는 요구 세부 아이템 카테고리 태그 1개 배정
				if (ShuffledTypes.IsValidIndex(NpcIdx))
				{
					NpcData.DesiredItemTypeTag = ShuffledTypes[NpcIdx];
				}

				EventData.EventNpcs.Add(NpcData);
			}

			PlannedEncounters.Add(MoveTemp(StructInst));
		}
	}
}

