/*****************************************************************//**
 * @file   BBMCombatManagerSubsystem.cpp
 * @brief  BBM(비트 배낭 상인) 인카운터 액터 스폰/제거 및 1D 타일 라인 전투 관리 월드 서브시스템 구현
 * @date   2026-11-04
 *********************************************************************/

#include "Combat/BBMCombatManagerSubsystem.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMAICombatantBase.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Stage/BBMStageTypes.h"
#include "Stage/BBMEncounterDefinition.h"
#include "Stage/BBMChestActorBase.h"
#include "Stage/BBMEventNpcBase.h"
#include "Level/BBMWorldSettings.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Item/BBMItemEntry.h"
#include "Item/DISItemStack.h"
#include "Inventory/BBMInventoryComponent.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

UBBMCombatManagerSubsystem::UBBMCombatManagerSubsystem()
{
}

void UBBMCombatManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ClearCurrentEncounter();
}

void UBBMCombatManagerSubsystem::Deinitialize()
{
	ClearCurrentEncounter();
	Super::Deinitialize();
}

bool UBBMCombatManagerSubsystem::SetupEncounter(
	const FBBMRuntimeEncounterData* RuntimeData,
	AActor* RoomStartPointActor,
	const UBBMPlayerCharacterDefinition* PlayerDef
)
{
	// 1단계: 이전 만남 액터 전량 정리
	ClearCurrentEncounter();

	if (RuntimeData == nullptr)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const UBBMEncounterDefinition* EncounterDef = RuntimeData->EncounterDefinition.LoadSynchronous();
	if (EncounterDef == nullptr)
	{
		return false;
	}

	// 1D 배틀 타일 라인의 기준 중앙점(Center Point) 및 방향 계산
	const FTransform CenterTransform = RoomStartPointActor ? RoomStartPointActor->GetActorTransform() : FTransform::Identity;
	const FVector CenterLocation = CenterTransform.GetLocation();
	const FRotator CenterRotation = CenterTransform.GetRotation().Rotator();
	const FVector LineForward = CenterTransform.GetRotation().GetForwardVector();

	// 1D 배틀 라인 중앙점 기준 좌우 배치 간격 (Centimeters)
	constexpr float BattleLineCenterOffset = 180.0f;  // 중앙점에서 최전방 전투원까지의 거리 (총 대치 간격 360cm)
	constexpr float BattleLineSlotSpacing = 120.0f;   // 슬롯(2열, 3열...) 간격

	// 2단계: 플레이어 상인 지연 스폰(SpawnActorDeferred) 또는 기존 상인 위치 텔레포트 (아군: 중앙점 왼쪽 -LineForward 방향)
	const FVector PlayerLocation = CenterLocation - (LineForward * BattleLineCenterOffset);
	const FRotator PlayerRotation = CenterRotation; // 오른쪽(적 방향)을 바라봄
	const FTransform PlayerTransform(PlayerRotation, PlayerLocation);

	if (ActivePlayerMerchant == nullptr || !IsValid(ActivePlayerMerchant))
	{
		TSubclassOf<ABBMPlayerMerchantBase> MerchantClass = ABBMPlayerMerchantBase::StaticClass();
		if (PlayerDef != nullptr)
		{
			if (UClass* LoadedClass = PlayerDef->GetPawnClass().LoadSynchronous())
			{
				if (LoadedClass->IsChildOf(ABBMPlayerMerchantBase::StaticClass()))
				{
					MerchantClass = LoadedClass;
				}
			}
		}

		ActivePlayerMerchant = World->SpawnActorDeferred<ABBMPlayerMerchantBase>(
			MerchantClass,
			PlayerTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (ActivePlayerMerchant != nullptr)
		{
			// PostInitializeComponents 및 BeginPlay 실행 전에 CharacterDefinition을 우선 주입하여 초기화 무결성 보장
			if (PlayerDef != nullptr)
			{
				ActivePlayerMerchant->SetCharacterDefinition(PlayerDef);
			}

			ActivePlayerMerchant->FinishSpawning(PlayerTransform);

			// 런 세이브 데이터가 존재할 경우 플레이어 상인에게 세이브 데이터(체력, 골드, 배낭 슬롯, 장비) 역방향 복원 적용
			if (UGameInstance* GI = World->GetGameInstance())
			{
				if (UBBMGameDataSubsystem* GameData = GI->GetSubsystem<UBBMGameDataSubsystem>())
				{
					if (GameData->HasActiveRun() == true)
					{
						GameData->ApplyRunDataToMerchant(ActivePlayerMerchant);
					}
				}
			}
		}
	}
	else
	{
		// 이전 인카운터에서 유지된 상인 위치 텔레포트
		ActivePlayerMerchant->SetActorLocationAndRotation(
			PlayerLocation,
			PlayerRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);
	}

	const EBBMEncounterType EncType = EncounterDef->GetEncounterType();

	// 3단계: 만남 유형별 액터 스폰 및 배치 (적/상자/NPC: 중앙점 오른쪽 +LineForward 방향)
	const FRotator FacingPlayerRotation = (CenterRotation + FRotator(0.0f, 180.0f, 0.0f)).GetNormalized();

	if (EncType == EBBMEncounterType::Treasure)
	{
		if (const UBBMTreasureEncounterDefinition* TreasureDef = Cast<UBBMTreasureEncounterDefinition>(EncounterDef))
		{
			TSubclassOf<ABBMChestActorBase> ChestClass = TreasureDef->ChestClass.LoadSynchronous();
			if (ChestClass == nullptr)
			{
				ChestClass = ABBMChestActorBase::StaticClass();
			}

			// 상자는 중앙선 약간 오른쪽 또는 중앙에 플레이어를 마주보게 배치
			const FVector ChestLocation = CenterLocation + (LineForward * (BattleLineCenterOffset * 0.5f));
			const FTransform ChestTransform(FacingPlayerRotation, ChestLocation);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ABBMChestActorBase* SpawnedChest = World->SpawnActor<ABBMChestActorBase>(ChestClass, ChestTransform, SpawnParams);
			if (SpawnedChest != nullptr)
			{
				SpawnedEncounterActors.Add(SpawnedChest);
			}
		}
		return true;
	}
	else if (EncType == EBBMEncounterType::Event)
	{
		const FBBMRuntimeEventEncounterData* EventData = static_cast<const FBBMRuntimeEventEncounterData*>(RuntimeData);
		const TArray<FBBMRuntimeNpcData>& NpcList = EventData->EventNpcs;

		for (int32 i = 0; i < NpcList.Num(); ++i)
		{
			TSubclassOf<ABBMEventNpcBase> NpcClass = NpcList[i].NpcClass.LoadSynchronous();
			if (NpcClass == nullptr)
			{
				NpcClass = ABBMEventNpcBase::StaticClass();
			}

			const FVector NpcLocation = CenterLocation + (LineForward * (BattleLineCenterOffset + (i * BattleLineSlotSpacing)));
			const FTransform NpcTransform(FacingPlayerRotation, NpcLocation);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ABBMEventNpcBase* SpawnedNpc = World->SpawnActor<ABBMEventNpcBase>(NpcClass, NpcTransform, SpawnParams);
			if (SpawnedNpc != nullptr)
			{
				SpawnedEncounterActors.Add(SpawnedNpc);
			}
		}
		return true;
	}
	else if (EncType == EBBMEncounterType::Combat || EncType == EBBMEncounterType::BossCombat)
	{
		ActiveCombatEncounterDef = Cast<UBBMCombatEncounterDefinition>(EncounterDef);
		TArray<ABBMAICombatantBase*> SpawnedMonsters;
		if (const UBBMCombatEncounterDefinition* CombatDef = Cast<UBBMCombatEncounterDefinition>(EncounterDef))
		{
			int32 MonsterIndex = 0;
			for (const TSoftObjectPtr<UBBMAICombatantDefinition>& MonsterDefPtr : CombatDef->GetMonsterList())
			{
				if (const UBBMAICombatantDefinition* MonsterDef = MonsterDefPtr.LoadSynchronous())
				{
					TSubclassOf<ABBMAICombatantBase> AIClass = ABBMAICombatantBase::StaticClass();
					if (UClass* LoadedClass = MonsterDef->GetPawnClass().LoadSynchronous())
					{
						if (LoadedClass->IsChildOf(ABBMAICombatantBase::StaticClass()))
						{
							AIClass = LoadedClass;
						}
					}

					const FVector MonsterLocation = CenterLocation + (LineForward * (BattleLineCenterOffset + (MonsterIndex * BattleLineSlotSpacing)));
					const FTransform MonsterTransform(FacingPlayerRotation, MonsterLocation);

					ABBMAICombatantBase* MonsterActor = World->SpawnActorDeferred<ABBMAICombatantBase>(
						AIClass,
						MonsterTransform,
						nullptr,
						nullptr,
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn
					);

					if (MonsterActor != nullptr)
					{
						// PostInitializeComponents 및 BeginPlay 실행 전에 CharacterDefinition을 우선 주입하여 초기화 무결성 보장
						MonsterActor->SetCharacterDefinition(MonsterDef);
						MonsterActor->FinishSpawning(MonsterTransform);

						SpawnedMonsters.Add(MonsterActor);
						SpawnedEncounterActors.Add(MonsterActor);
					}
					++MonsterIndex;
				}
			}
		}

		// 1D 전투 라인 초기 진형 구축 (아군: ActivePlayerMerchant, 적군: SpawnedMonsters)
		InitializeBattleFormation(ActivePlayerMerchant, SpawnedMonsters);
		return true;
	}

	return false;
}

bool UBBMCombatManagerSubsystem::SetupEncounterFromInstancedStruct(
	const TInstancedStruct<FBBMRuntimeEncounterData>& InEncounterData,
	AActor* RoomStartPointActor,
	const UBBMPlayerCharacterDefinition* PlayerDef
)
{
	if (InEncounterData.IsValid() == false)
	{
		return false;
	}

	return SetupEncounter(InEncounterData.GetPtr<FBBMRuntimeEncounterData>(), RoomStartPointActor, PlayerDef);
}

void UBBMCombatManagerSubsystem::ClearCurrentEncounter()
{
	ActiveCombatEncounterDef = nullptr;
	ActiveBeatRangeHandles.Empty();
	ActiveBeatAnimLayerTags.Empty();
	PendingRewardItemIds.Empty();

	// 1D 라인 슬롯 비우기
	AllyLane.Slots.Empty();
	EnemyLane.Slots.Empty();

	// 스폰되었던 인카운터 액터들 안전 파괴
	for (const TWeakObjectPtr<AActor>& ActorPtr : SpawnedEncounterActors)
	{
		if (ActorPtr.IsValid() == true)
		{
			ActorPtr->Destroy();
		}
	}
	SpawnedEncounterActors.Empty();
}

void UBBMCombatManagerSubsystem::InitializeBattleFormation(ABBMPlayerMerchantBase* Merchant, const TArray<ABBMAICombatantBase*>& Monsters)
{
	AllyLane.Slots.Empty();
	EnemyLane.Slots.Empty();

	// 아군 레인: 플레이어 상인을 최전방 Index 0에 배치
	if (Merchant != nullptr)
	{
		AllyLane.Slots.Add(Merchant);
	}

	// 적군 레인: 몬스터들을 Index 0부터 차례로 배치
	for (ABBMAICombatantBase* Monster : Monsters)
	{
		if (Monster != nullptr)
		{
			EnemyLane.Slots.Add(Monster);
		}
	}
}

bool UBBMCombatManagerSubsystem::AddCombatantToLane(EBBMCombatLane Lane, ABBMCombatantBase* Combatant, int32 PreferredSlotIndex)
{
	if (Combatant == nullptr)
	{
		return false;
	}

	FBBM1DCombatLane& TargetLane = (Lane == EBBMCombatLane::AllyLane) ? AllyLane : EnemyLane;

	if (PreferredSlotIndex >= 0 && PreferredSlotIndex < TargetLane.Slots.Num())
	{
		TargetLane.Slots.Insert(Combatant, PreferredSlotIndex);
	}
	else
	{
		TargetLane.Slots.Add(Combatant);
	}

	return true;
}

bool UBBMCombatManagerSubsystem::RemoveCombatantFromLane(ABBMCombatantBase* Combatant)
{
	if (Combatant == nullptr)
	{
		return false;
	}

	bool bRemoved = false;
	for (int32 i = 0; i < AllyLane.Slots.Num(); ++i)
	{
		if (AllyLane.Slots[i].Get() == Combatant)
		{
			AllyLane.Slots.RemoveAt(i);
			bRemoved = true;
			break;
		}
	}

	if (bRemoved == false)
	{
		for (int32 i = 0; i < EnemyLane.Slots.Num(); ++i)
		{
			if (EnemyLane.Slots[i].Get() == Combatant)
			{
				EnemyLane.Slots.RemoveAt(i);
				bRemoved = true;
				break;
			}
		}
	}

	return bRemoved;
}

ABBMCombatantBase* UBBMCombatManagerSubsystem::GetFrontlineCombatant(EBBMCombatLane Lane) const
{
	const FBBM1DCombatLane& TargetLane = (Lane == EBBMCombatLane::AllyLane) ? AllyLane : EnemyLane;

	for (const TWeakObjectPtr<ABBMCombatantBase>& SlotPtr : TargetLane.Slots)
	{
		if (ABBMCombatantBase* Combatant = SlotPtr.Get())
		{
			const UBBMAttributeSet* AttrSet = Combatant->GetAttributeSet();
			if (AttrSet != nullptr && AttrSet->GetHealth() > 0.0f)
			{
				return Combatant;
			}
		}
	}

	return nullptr;
}

TArray<ABBMCombatantBase*> UBBMCombatManagerSubsystem::GetLivingCombatantsInLane(EBBMCombatLane Lane) const
{
	TArray<ABBMCombatantBase*> LivingList;
	const FBBM1DCombatLane& TargetLane = (Lane == EBBMCombatLane::AllyLane) ? AllyLane : EnemyLane;

	for (const TWeakObjectPtr<ABBMCombatantBase>& SlotPtr : TargetLane.Slots)
	{
		if (ABBMCombatantBase* Combatant = SlotPtr.Get())
		{
			const UBBMAttributeSet* AttrSet = Combatant->GetAttributeSet();
			if (AttrSet != nullptr && AttrSet->GetHealth() > 0.0f)
			{
				LivingList.Add(Combatant);
			}
		}
	}

	return LivingList;
}

void UBBMCombatManagerSubsystem::ShiftLane(EBBMCombatLane Lane, EBBMLaneShiftDirection Direction, int32 StepCount)
{
	if (StepCount <= 0)
	{
		return;
	}

	FBBM1DCombatLane& TargetLane = (Lane == EBBMCombatLane::AllyLane) ? AllyLane : EnemyLane;
	if (TargetLane.Slots.IsEmpty() == true)
	{
		return;
	}

	if (Direction == EBBMLaneShiftDirection::Backward)
	{
		// 후퇴: 앞쪽에 nullptr 빈 슬롯을 삽입하여 전체를 뒤로 밀어냄
		for (int32 i = 0; i < StepCount; ++i)
		{
			TargetLane.Slots.Insert(nullptr, 0);
		}
	}
	else if (Direction == EBBMLaneShiftDirection::Frontward)
	{
		// 전진: 앞쪽의 빈 슬롯이 있으면 우선 제거하고 앞으로 당김
		for (int32 i = 0; i < StepCount && TargetLane.Slots.Num() > 0; ++i)
		{
			if (TargetLane.Slots[0].IsValid() == false)
			{
				TargetLane.Slots.RemoveAt(0);
			}
			else
			{
				break;
			}
		}
	}
}

void UBBMCombatManagerSubsystem::SwapSlots(EBBMCombatLane Lane, int32 SlotA, int32 SlotB)
{
	FBBM1DCombatLane& TargetLane = (Lane == EBBMCombatLane::AllyLane) ? AllyLane : EnemyLane;
	if (TargetLane.Slots.IsValidIndex(SlotA) == true && TargetLane.Slots.IsValidIndex(SlotB) == true)
	{
		TargetLane.Slots.Swap(SlotA, SlotB);
	}
}

float UBBMCombatManagerSubsystem::ResolveAttack(ABBMCombatantBase* Instigator, ABBMCombatantBase* Target, float BaseDamage, bool bTargetParrying)
{
	if (Target == nullptr || BaseDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 1단계: 패링 체크 (패링 성공 시 공격 완전 무효화, 0 피해, 방어도/체력 100% 보존)
	if (bTargetParrying == true)
	{
		return 0.0f;
	}

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	UBBMAttributeSet* TargetAttr = Target->GetAttributeSet();
	if (TargetASC == nullptr || TargetAttr == nullptr)
	{
		return 0.0f;
	}

	float RemainingDamage = BaseDamage;

	// 2단계: 턴 방어도(TurnArmor) 차감 (해당 턴에 획득한 방어도로 우선 흡수)
	const float CurrentTurnArmor = TargetAttr->GetTurnArmor();
	const float TurnAbsorb = FMath::Min(CurrentTurnArmor, RemainingDamage);
	const float NewTurnArmor = FMath::Max(0.0f, CurrentTurnArmor - TurnAbsorb);
	TargetASC->SetNumericAttributeBase(UBBMAttributeSet::GetTurnArmorAttribute(), NewTurnArmor);
	RemainingDamage -= TurnAbsorb;

	if (RemainingDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 3단계: 지속 방어도(Armor) 차감 (전투 내내 누적된 지속 방어력으로 2차 흡수)
	const float CurrentArmor = TargetAttr->GetArmor();
	const float ArmorAbsorb = FMath::Min(CurrentArmor, RemainingDamage);
	const float NewArmor = FMath::Max(0.0f, CurrentArmor - ArmorAbsorb);
	TargetASC->SetNumericAttributeBase(UBBMAttributeSet::GetArmorAttribute(), NewArmor);
	RemainingDamage -= ArmorAbsorb;

	if (RemainingDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 4단계: 남은 데미지 처리 (최종 체력 차감)
	const float CurrentHealth = TargetAttr->GetHealth();
	const float ActualHealthDamage = FMath::Min(CurrentHealth, RemainingDamage);
	const float NewHealth = FMath::Max(0.0f, CurrentHealth - RemainingDamage);
	TargetASC->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), NewHealth);

	return ActualHealthDamage;
}

TArray<float> UBBMCombatManagerSubsystem::ResolveMultiAttack(ABBMCombatantBase* Instigator, const TArray<ABBMCombatantBase*>& Targets, float BaseDamage)
{
	TArray<float> DamageResults;
	DamageResults.Reserve(Targets.Num());

	for (ABBMCombatantBase* Target : Targets)
	{
		const float Dealt = ResolveAttack(Instigator, Target, BaseDamage, false);
		DamageResults.Add(Dealt);
	}

	return DamageResults;
}

void UBBMCombatManagerSubsystem::AddPersistentArmor(ABBMCombatantBase* Target, float Amount)
{
	if (Target == nullptr || Amount <= 0.0f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
	UBBMAttributeSet* AttrSet = Target->GetAttributeSet();
	if (ASC != nullptr && AttrSet != nullptr)
	{
		const float NewArmor = AttrSet->GetArmor() + Amount;
		ASC->SetNumericAttributeBase(UBBMAttributeSet::GetArmorAttribute(), NewArmor);
	}
}

void UBBMCombatManagerSubsystem::AddTurnArmor(ABBMCombatantBase* Target, float Amount)
{
	if (Target == nullptr || Amount <= 0.0f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
	UBBMAttributeSet* AttrSet = Target->GetAttributeSet();
	if (ASC != nullptr && AttrSet != nullptr)
	{
		const float NewTurnArmor = AttrSet->GetTurnArmor() + Amount;
		ASC->SetNumericAttributeBase(UBBMAttributeSet::GetTurnArmorAttribute(), NewTurnArmor);
	}
}

void UBBMCombatManagerSubsystem::ClearTurnArmor(ABBMCombatantBase* Target)
{
	if (Target == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
	if (ASC != nullptr)
	{
		ASC->SetNumericAttributeBase(UBBMAttributeSet::GetTurnArmorAttribute(), 0.0f);
	}
}

void UBBMCombatManagerSubsystem::ClearAllTurnArmors()
{
	for (ABBMCombatantBase* Combatant : GetLivingCombatantsInLane(EBBMCombatLane::AllyLane))
	{
		ClearTurnArmor(Combatant);
	}

	for (ABBMCombatantBase* Combatant : GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane))
	{
		ClearTurnArmor(Combatant);
	}
}

void UBBMCombatManagerSubsystem::ApplyStatusEffect(ABBMCombatantBase* Target, const FGameplayTag& StatusTag, int32 StacksToAdd)
{
	if (Target == nullptr || StatusTag.IsValid() == false || StacksToAdd <= 0)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(StatusTag, StacksToAdd);
	}
}

int32 UBBMCombatManagerSubsystem::GetStatusStacks(const ABBMCombatantBase* Target, const FGameplayTag& StatusTag) const
{
	if (Target == nullptr || StatusTag.IsValid() == false)
	{
		return 0;
	}

	if (UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent())
	{
		return ASC->GetTagCount(StatusTag);
	}

	return 0;
}

void UBBMCombatManagerSubsystem::ProcessTurnEndStatusTicks()
{
	// 모든 살아있는 전투원 수집
	TArray<ABBMCombatantBase*> AllCombatants = GetLivingCombatantsInLane(EBBMCombatLane::AllyLane);
	AllCombatants.Append(GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane));

	for (ABBMCombatantBase* Combatant : AllCombatants)
	{
		if (Combatant == nullptr)
		{
			continue;
		}

		UAbilitySystemComponent* ASC = Combatant->GetAbilitySystemComponent();
		if (ASC == nullptr)
		{
			continue;
		}

		// 1. 독 상태이상 처리 (현재 누적 스택 수만큼 피해 적용 후 1스택 차감)
		const int32 PoisonStacks = ASC->GetTagCount(BBMStatusTags::Status_Poison);
		if (PoisonStacks > 0)
		{
			ResolveAttack(nullptr, Combatant, static_cast<float>(PoisonStacks), false);
			ASC->RemoveLooseGameplayTag(BBMStatusTags::Status_Poison, 1);
		}

		// 2. 화상 상태이상 처리 (기본 고정 피해 5.0f 적용 후 1스택 차감)
		const int32 BurnStacks = ASC->GetTagCount(BBMStatusTags::Status_Burn);
		if (BurnStacks > 0)
		{
			ResolveAttack(nullptr, Combatant, 5.0f, false);
			ASC->RemoveLooseGameplayTag(BBMStatusTags::Status_Burn, 1);
		}
	}
}

bool UBBMCombatManagerSubsystem::ApplyItemGrantedEffects(ABBMCombatantBase* TargetCombatant, UBBMItemEntry* ItemEntry, TArray<FActiveGameplayEffectHandle>& OutHandles)
{
	if (TargetCombatant == nullptr || ItemEntry == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = TargetCombatant->GetAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return false;
	}

	for (const FBBMBeatEffectRange& Range : ItemEntry->GetEffectRanges())
	{
		for (const FBBMItemGameplayEffectConfig& Config : Range.GrantedEffects)
		{
			TSubclassOf<UGameplayEffect> EffectClass = Config.EffectClass.Get();
			if (EffectClass == nullptr)
			{
				EffectClass = Config.EffectClass.LoadSynchronous();
			}

			if (EffectClass == nullptr)
			{
				continue;
			}

			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddInstigator(TargetCombatant, TargetCombatant);

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
			if (SpecHandle.IsValid() == true && SpecHandle.Data.IsValid() == true)
			{
				// 필수 기본 스탯 태그가 설정되어 있다면 0.0f로 초기화하여 SetByCaller 미지정 에러 방지
				static const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.AttackPower"), false);
				static const FGameplayTag DefenseTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.DefensePower"), false);
				static const FGameplayTag ArmorTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.Armor"), false);

				if (AttackTag.IsValid() == true)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(AttackTag, 0.0f);
				}
				if (DefenseTag.IsValid() == true)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(DefenseTag, 0.0f);
				}
				if (ArmorTag.IsValid() == true)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(ArmorTag, 0.0f);
				}

				// SetByCallerMagnitude 동적 주입
				for (const TPair<FGameplayTag, float>& Pair : Config.SetByCallerMagnitudes)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
				}

				FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				if (ActiveHandle.IsValid() == true)
				{
					OutHandles.Add(ActiveHandle);
				}
			}
		}
	}

	return true;
}

void UBBMCombatManagerSubsystem::RemoveItemGrantedEffects(ABBMCombatantBase* TargetCombatant, const TArray<FActiveGameplayEffectHandle>& InHandles)
{
	if (TargetCombatant == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* ASC = TargetCombatant->GetAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : InHandles)
	{
		if (Handle.IsValid() == true)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
}

namespace
{
	FORCEINLINE EBBMVisualWindow MapActionCategoryToVisualWindow(EBBMActionCategory Category)
	{
		switch (Category)
		{
		case EBBMActionCategory::PreMovement:
			return EBBMVisualWindow::PreMoveWindow;
		case EBBMActionCategory::PreBuffDebuff:
		case EBBMActionCategory::Defense:
			return EBBMVisualWindow::PrepDefenseWindow;
		case EBBMActionCategory::Damage:
		case EBBMActionCategory::PostBuffDebuff:
			return EBBMVisualWindow::ImpactWindow;
		case EBBMActionCategory::PostMovement:
			return EBBMVisualWindow::PostMoveWindow;
		default:
			return EBBMVisualWindow::ImpactWindow;
		}
	}
}

bool UBBMCombatManagerSubsystem::SimulateTurn(TArray<FBBMCombatStepVisualEvent>& OutVisualEvents)
{
	OutVisualEvents.Empty();

	// 2. 한 턴 시작 시 행동 처리 (플레이어 비트 시퀀스 업데이트) 및 캐릭터 죽음 및 승패 체크
	for (ABBMCombatantBase* Ally : GetLivingCombatantsInLane(EBBMCombatLane::AllyLane))
	{
		if (Ally != nullptr)
		{
			Ally->UpdateBeatSequence();
		}
	}

	bool bEnded = false;
	bool bVictory = false;
	if (CheckBattleOutcome(bEnded, bVictory) == true && bEnded == true)
	{
		return false;
	}

	// 3. 비트 루프 (비트 0 ~ 7 순회: 4~8번 반복)
	for (int32 BeatIndex = 0; BeatIndex < 8; ++BeatIndex)
	{
		// 4. 한 비트 시작: Effect Range / Anim Layer 시작점 일치 시 적용
		ProcessBeatStart(BeatIndex);

		// 5. EBBMActionCategory의 순서대로(0~5) 논리 처리. 동일 카테고리에서는 아군->적군 순서대로 순차 처리
		for (uint8 CatIdx = 0; CatIdx <= static_cast<uint8>(EBBMActionCategory::PostMovement); ++CatIdx)
		{
			const EBBMActionCategory CurrentCategory = static_cast<EBBMActionCategory>(CatIdx);
			const EBBMVisualWindow CurrentWindow = MapActionCategoryToVisualWindow(CurrentCategory);

			// 아군 처리
			ExecuteCombatantsActionForCategory(EBBMCombatLane::AllyLane, BeatIndex, CurrentCategory, CurrentWindow, OutVisualEvents);

			// 적군 처리
			ExecuteCombatantsActionForCategory(EBBMCombatLane::EnemyLane, BeatIndex, CurrentCategory, CurrentWindow, OutVisualEvents);
		}

		// 7. 하나의 비트 종료: Effect Range / Anim Layer 종료점 일치 시 제거
		ProcessBeatEnd(BeatIndex);

		// 8. 캐릭터 죽음 및 승패 체크
		CheckAndProcessDeadCombatants(OutVisualEvents, BeatIndex);

		if (CheckBattleOutcome(bEnded, bVictory) == true && bEnded == true)
		{
			break;
		}
	}

	// 6. UI쪽 ViewModel에 변화 목록 전달 (델리게이트 브로드캐스트)
	if (OutVisualEvents.IsEmpty() == false)
	{
		OnVisualEventsDispatched.Broadcast(OutVisualEvents);
	}

	// 10. 턴 종료 시 행동 처리: 상태이상 틱, 턴 방어도 초기화, AI들의 비트 시퀀스 업데이트
	ProcessTurnEndStatusTicks();
	ClearAllTurnArmors();

	for (ABBMCombatantBase* Enemy : GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane))
	{
		if (Enemy != nullptr)
		{
			Enemy->UpdateBeatSequence();
		}
	}

	CheckAndProcessDeadCombatants(OutVisualEvents, 7);
	CheckBattleOutcome(bEnded, bVictory);

	// UI ViewModel 연출을 위한 비주얼 이벤트 디스패치
	OnVisualEventsDispatched.Broadcast(OutVisualEvents);
	OnVisualEventsDispatchedNative.Broadcast(OutVisualEvents);

	return true;
}

void UBBMCombatManagerSubsystem::ProcessBeatStart(int32 BeatIndex)
{
	TArray<ABBMCombatantBase*> AllCombatants = GetLivingCombatantsInLane(EBBMCombatLane::AllyLane);
	AllCombatants.Append(GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane));

	for (ABBMCombatantBase* Combatant : AllCombatants)
	{
		if (Combatant == nullptr || Combatant->IsDead() == true)
		{
			continue;
		}

		UAbilitySystemComponent* ASC = Combatant->GetAbilitySystemComponent();
		if (ASC == nullptr)
		{
			continue;
		}

		const FBBMBeatSequence Seq = Combatant->GetCurrentBeatSequence();

		// 1. EffectRange 시작점 적용
		for (const FBBMBeatEffectRange& Range : Seq.EffectRanges)
		{
			if (Range.IsStartingAtBeat(BeatIndex) == true)
			{
				for (const FBBMItemGameplayEffectConfig& Config : Range.GrantedEffects)
				{
					TSubclassOf<UGameplayEffect> EffectClass = Config.EffectClass.Get();
					if (EffectClass == nullptr)
					{
						EffectClass = Config.EffectClass.LoadSynchronous();
					}

					if (EffectClass == nullptr)
					{
						continue;
					}

					FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
					ContextHandle.AddInstigator(Combatant, Combatant);

					FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
					if (SpecHandle.IsValid() == true && SpecHandle.Data.IsValid() == true)
					{
						for (const TPair<FGameplayTag, float>& Pair : Config.SetByCallerMagnitudes)
						{
							SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
						}

						FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						if (ActiveHandle.IsValid() == true)
						{
							ActiveBeatRangeHandles.FindOrAdd(Combatant).Add(ActiveHandle);
						}
					}
				}
			}
		}

		// 2. AnimLayer 시작점 링크
		for (const FBBMBeatAnimLayerRange& LayerRange : Seq.AnimLayerRanges)
		{
			if (LayerRange.IsStartingAtBeat(BeatIndex) == true && LayerRange.AnimLayerTag.IsValid() == true)
			{
				Combatant->LinkAnimLayerByTag(LayerRange.AnimLayerTag);
				ActiveBeatAnimLayerTags.FindOrAdd(Combatant).Add(LayerRange.AnimLayerTag);
			}
		}
	}
}

void UBBMCombatManagerSubsystem::ProcessBeatEnd(int32 BeatIndex)
{
	TArray<ABBMCombatantBase*> AllCombatants = GetLivingCombatantsInLane(EBBMCombatLane::AllyLane);
	AllCombatants.Append(GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane));

	for (ABBMCombatantBase* Combatant : AllCombatants)
	{
		if (Combatant == nullptr || Combatant->IsDead() == true)
		{
			continue;
		}

		UAbilitySystemComponent* ASC = Combatant->GetAbilitySystemComponent();
		const FBBMBeatSequence Seq = Combatant->GetCurrentBeatSequence();

		// 1. EffectRange 종료점 해제
		for (const FBBMBeatEffectRange& Range : Seq.EffectRanges)
		{
			if (Range.IsEndingAtBeat(BeatIndex) == true)
			{
				if (ASC != nullptr)
				{
					if (TArray<FActiveGameplayEffectHandle>* HandlesPtr = ActiveBeatRangeHandles.Find(Combatant))
					{
						for (const FActiveGameplayEffectHandle& Handle : *HandlesPtr)
						{
							if (Handle.IsValid() == true)
							{
								ASC->RemoveActiveGameplayEffect(Handle);
							}
						}
						HandlesPtr->Empty();
					}
				}
			}
		}

		// 2. AnimLayer 종료점 언링크
		for (const FBBMBeatAnimLayerRange& LayerRange : Seq.AnimLayerRanges)
		{
			if (LayerRange.IsEndingAtBeat(BeatIndex) == true && LayerRange.AnimLayerTag.IsValid() == true)
			{
				Combatant->UnlinkAnimLayerByTag(LayerRange.AnimLayerTag);
				if (TArray<FGameplayTag>* TagsPtr = ActiveBeatAnimLayerTags.Find(Combatant))
				{
					TagsPtr->Remove(LayerRange.AnimLayerTag);
				}
			}
		}
	}
}

void UBBMCombatManagerSubsystem::ExecuteCombatantsActionForCategory(EBBMCombatLane Lane, int32 BeatIndex, EBBMActionCategory Category, EBBMVisualWindow Window, TArray<FBBMCombatStepVisualEvent>& OutVisualEvents)
{
	TArray<ABBMCombatantBase*> LivingCombatants = GetLivingCombatantsInLane(Lane);
	for (ABBMCombatantBase* Combatant : LivingCombatants)
	{
		if (Combatant == nullptr || Combatant->IsDead() == true)
		{
			continue;
		}

		const FBBMBeatSequence Seq = Combatant->GetCurrentBeatSequence();
		if (Seq.Beats.IsValidIndex(BeatIndex) == false)
		{
			continue;
		}

		const FBBMBeatDefinition& BeatDef = Seq.Beats[BeatIndex];

		// 해당 윈도우에 매칭되는 애니메이션 태그 조회
		FGameplayTag MatchingAnimTag;
		for (const FBBMBeatAnimationConfig& AnimConfig : BeatDef.Animations)
		{
			if (AnimConfig.VisualWindow == Window && AnimConfig.StateTag.IsValid())
			{
				MatchingAnimTag = AnimConfig.StateTag;
				break;
			}
		}

		bool bExecutedActionForCategory = false;

		for (const TInstancedStruct<FBBMBeatAction>& ActionStruct : BeatDef.Actions)
		{
			if (const FBBMBeatAction* Action = ActionStruct.GetPtr<FBBMBeatAction>())
			{
				if (Action->GetActionCategory() == Category)
				{
					bExecutedActionForCategory = true;

					// 행동 전 상대 대상 스냅샷 (이벤트 기록용)
					ABBMCombatantBase* TargetCombatant = nullptr;
					if (Lane == EBBMCombatLane::AllyLane)
					{
						TargetCombatant = GetFrontlineCombatant(EBBMCombatLane::EnemyLane);
					}
					else
					{
						TargetCombatant = GetFrontlineCombatant(EBBMCombatLane::AllyLane);
					}

					float PrevTurnArmor = 0.0f;
					float PrevArmor = 0.0f;
					float PrevHealth = 0.0f;
					if (TargetCombatant != nullptr && TargetCombatant->GetAttributeSet() != nullptr)
					{
						PrevTurnArmor = TargetCombatant->GetAttributeSet()->GetTurnArmor();
						PrevArmor = TargetCombatant->GetAttributeSet()->GetArmor();
						PrevHealth = TargetCombatant->GetAttributeSet()->GetHealth();
					}

					// 다형성 커맨드 패턴 실행
					Action->ExecuteAction(this, Combatant);

					// 이벤트 기록
					FBBMCombatStepVisualEvent Event;
					Event.BeatIndex = BeatIndex;
					Event.ActionCategory = Category;
					Event.VisualWindow = Window;
					Event.Instigator = Combatant;
					Event.Target = TargetCombatant;
					Event.AnimationStateTag = MatchingAnimTag;

					if (TargetCombatant != nullptr && TargetCombatant->GetAttributeSet() != nullptr)
					{
						const float PostTurnArmor = TargetCombatant->GetAttributeSet()->GetTurnArmor();
						const float PostArmor = TargetCombatant->GetAttributeSet()->GetArmor();
						const float PostHealth = TargetCombatant->GetAttributeSet()->GetHealth();

						Event.TurnArmorAbsorbed = FMath::Max(0.0f, PrevTurnArmor - PostTurnArmor);
						Event.ArmorAbsorbed = FMath::Max(0.0f, PrevArmor - PostArmor);
						Event.DamageDealt = FMath::Max(0.0f, PrevHealth - PostHealth);
						Event.bTargetDied = TargetCombatant->IsDead();
					}

					OutVisualEvents.Add(Event);
				}
			}
		}

		// 액션이 실행되지 않았지만 해당 윈도우에 매칭되는 애니메이션 태그가 있고, 동일 윈도우 내 액션이 전혀 없는 경우 순수 애니메이션 이벤트 발행
		if (bExecutedActionForCategory == false && MatchingAnimTag.IsValid())
		{
			const bool bIsPrimaryCategoryForWindow =
				(Window == EBBMVisualWindow::PreMoveWindow && Category == EBBMActionCategory::PreMovement) ||
				(Window == EBBMVisualWindow::PrepDefenseWindow && Category == EBBMActionCategory::PreBuffDebuff) ||
				(Window == EBBMVisualWindow::ImpactWindow && Category == EBBMActionCategory::Damage) ||
				(Window == EBBMVisualWindow::PostMoveWindow && Category == EBBMActionCategory::PostMovement);

			if (bIsPrimaryCategoryForWindow)
			{
				bool bHasAnyActionInSameWindow = false;
				for (const TInstancedStruct<FBBMBeatAction>& ActionStruct : BeatDef.Actions)
				{
					if (const FBBMBeatAction* Action = ActionStruct.GetPtr<FBBMBeatAction>())
					{
						if (MapActionCategoryToVisualWindow(Action->GetActionCategory()) == Window)
						{
							bHasAnyActionInSameWindow = true;
							break;
						}
					}
				}

				if (bHasAnyActionInSameWindow == false)
				{
					FBBMCombatStepVisualEvent AnimEvent;
					AnimEvent.BeatIndex = BeatIndex;
					AnimEvent.ActionCategory = Category;
					AnimEvent.VisualWindow = Window;
					AnimEvent.Instigator = Combatant;
					AnimEvent.AnimationStateTag = MatchingAnimTag;
					OutVisualEvents.Add(AnimEvent);
				}
			}
		}
	}
}

void UBBMCombatManagerSubsystem::CheckAndProcessDeadCombatants(TArray<FBBMCombatStepVisualEvent>& OutVisualEvents, int32 BeatIndex)
{
	TArray<ABBMCombatantBase*> AllCombatants;
	for (const TWeakObjectPtr<ABBMCombatantBase>& Slot : AllyLane.Slots)
	{
		if (ABBMCombatantBase* C = Slot.Get())
		{
			AllCombatants.Add(C);
		}
	}
	for (const TWeakObjectPtr<ABBMCombatantBase>& Slot : EnemyLane.Slots)
	{
		if (ABBMCombatantBase* C = Slot.Get())
		{
			AllCombatants.Add(C);
		}
	}

	for (ABBMCombatantBase* Combatant : AllCombatants)
	{
		if (Combatant != nullptr && Combatant->IsDead() == true)
		{
			FBBMCombatStepVisualEvent DeathEvent;
			DeathEvent.BeatIndex = BeatIndex;
			DeathEvent.Target = Combatant;
			DeathEvent.bTargetDied = true;
			OutVisualEvents.Add(DeathEvent);

			HandleCombatantDeath(Combatant);
		}
	}
}

void UBBMCombatManagerSubsystem::HandleCombatantDeath(ABBMCombatantBase* DeadCombatant)
{
	if (DeadCombatant == nullptr)
	{
		return;
	}

	// 1. 걸려있던 Effect Range의 Effect들 제거
	if (TArray<FActiveGameplayEffectHandle>* HandlesPtr = ActiveBeatRangeHandles.Find(DeadCombatant))
	{
		if (UAbilitySystemComponent* ASC = DeadCombatant->GetAbilitySystemComponent())
		{
			for (const FActiveGameplayEffectHandle& Handle : *HandlesPtr)
			{
				if (Handle.IsValid() == true)
				{
					ASC->RemoveActiveGameplayEffect(Handle);
				}
			}
		}
		ActiveBeatRangeHandles.Remove(DeadCombatant);
	}

	// 1-2. 걸려있던 Anim Layer를 제거
	if (TArray<FGameplayTag>* TagsPtr = ActiveBeatAnimLayerTags.Find(DeadCombatant))
	{
		for (const FGameplayTag& Tag : *TagsPtr)
		{
			DeadCombatant->UnlinkAnimLayerByTag(Tag);
		}
		ActiveBeatAnimLayerTags.Remove(DeadCombatant);
	}

	// 2. 캐싱된 자신과 연관된 행동, Effect Range, Anim Layer 제거 (비트 시퀀스 초기화)
	// (슬롯에서 제거되면 다음 비트 순회 대상에서 제외됨)

	// 3. 죽은 대상에게 죽음에 대한 Animation Tag 부여
	if (UAbilitySystemComponent* ASC = DeadCombatant->GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(BBMAnimationTags::Animation_Death, 1);
	}

	// 4. 대상 1초 정도 뒤에 Dissolve 처리 (가상 함수 껍데기 호출)
	DeadCombatant->StartDissolve();

	// 5. 1D 라인 슬롯에서 제거
	RemoveCombatantFromLane(DeadCombatant);
}

void UBBMCombatManagerSubsystem::SetPendingRewardItems(const TArray<FPrimaryAssetId>& InRewardItemIds)
{
	PendingRewardItemIds = InRewardItemIds;
}

void UBBMCombatManagerSubsystem::AddPendingRewardItem(const FPrimaryAssetId& InRewardItemId)
{
	PendingRewardItemIds.Add(InRewardItemId);
}

const TArray<FPrimaryAssetId>& UBBMCombatManagerSubsystem::GetPendingRewardItems() const
{
	return PendingRewardItemIds;
}

void UBBMCombatManagerSubsystem::ClearPendingRewardItems()
{
	PendingRewardItemIds.Empty();
}

bool UBBMCombatManagerSubsystem::CheckBattleOutcome(bool& bOutCombatEnded, bool& bOutPlayerVictory)
{
	bOutCombatEnded = false;
	bOutPlayerVictory = false;

	const bool bAlliesDead = GetLivingCombatantsInLane(EBBMCombatLane::AllyLane).IsEmpty();
	const bool bEnemiesDead = GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane).IsEmpty();

	if (bAlliesDead == true)
	{
		bOutCombatEnded = true;
		bOutPlayerVictory = false;
		OnCombatDefeat.Broadcast();
		OnCombatDefeatNative.Broadcast();
		return true;
	}
	else if (bEnemiesDead == true)
	{
		bOutCombatEnded = true;
		bOutPlayerVictory = true;

		// 1. UsedEquipmentStorage 전달
		TArray<UDISItemStack*> UsedEquipmentList;
		if (ABBMCombatantBase* AllyFront = GetFrontlineCombatant(EBBMCombatLane::AllyLane))
		{
			if (ABBMPlayerMerchantBase* Merchant = Cast<ABBMPlayerMerchantBase>(AllyFront))
			{
				if (UBBMInventoryComponent* InvenComp = Merchant->GetInventoryComponent())
				{
					UsedEquipmentList = InvenComp->GetArchivedEquipmentStacks();
				}
			}
		}

		// 2. 보상 아이템 PAD 목록 전달 (PendingRewardItemIds)
		const TArray<FPrimaryAssetId> RewardItemIds = PendingRewardItemIds;

		OnCombatVictory.Broadcast(UsedEquipmentList, RewardItemIds);
		OnCombatVictoryNative.Broadcast(UsedEquipmentList, RewardItemIds);
		return true;
	}

	return false;
}


