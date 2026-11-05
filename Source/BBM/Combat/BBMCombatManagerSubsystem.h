/*****************************************************************//**
 * @file   BBMCombatManagerSubsystem.h
 * @brief  BBM(비트 배낭 상인) 인카운터 액터 스폰/제거 및 1D 타일 라인 전투 관리 월드 서브시스템 정의
 * @details 월드 수명 주기에 맞춰 인카운터(전투/이벤트/보물) 액터 스폰 및 제거, 1D 타일 라인 진형/시프트, 공격/방어도/상태이상 해결을 총괄함
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/BBMCombatTypes.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "StructUtils/InstancedStruct.h"
#include "Stage/BBMStageTypes.h"
#include "Item/BBMBeatTypes.h"
#include "BBMCombatManagerSubsystem.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class AActor;
class ABBMCombatantBase;
class ABBMPlayerMerchantBase;
class ABBMAICombatantBase;
class ABBMChestActorBase;
class ABBMEventNpcBase;
class UBBMAICombatantDefinition;
class UBBMCombatEncounterDefinition;
class UBBMItemEntry;
class UDISItemStack;
struct FBBMRuntimeEncounterData;

/**
 * 한 비트 단위 액션 실행 시 발생하는 비주얼 연출 및 상태 변화 이벤트 페이로드
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMCombatStepVisualEvent
{
	GENERATED_BODY()

	/* 비트 인덱스 (0~7) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BeatIndex = 0;

	/* 6대 논리 실행 범주 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBBMActionCategory ActionCategory = EBBMActionCategory::PreMovement;

	/* 4대 비주얼 윈도우 매칭 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBBMVisualWindow VisualWindow = EBBMVisualWindow::PreMoveWindow;

	/* 행동 주체 전투원 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<ABBMCombatantBase> Instigator = nullptr;

	/* 행동 대상 전투원 (단수 또는 최전방) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<ABBMCombatantBase> Target = nullptr;

	/* 적용된 최종 체력 피해 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DamageDealt = 0.0f;

	/* 흡수된 턴 방어도 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TurnArmorAbsorbed = 0.0f;

	/* 흡수된 지속 방어도 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ArmorAbsorbed = 0.0f;

	/* 대상의 사망 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTargetDied = false;

	/* 부여된 상태이상 태그 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTag StatusTag = FGameplayTag();

	/* 부여된 상태이상 스택 수 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StatusStacks = 0;

	/* 비트 윈도우에서 실행할 애니메이션 상태 태그 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTag AnimationStateTag = FGameplayTag();
};

/* 전투 승리 델리게이트 (UsedEquipmentStorage 보관소 장비 목록 및 보상 아이템 PAD 목록 전달) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FBBMOnCombatVictorySignature,
	const TArray<UDISItemStack*>&, UsedEquipment,
	const TArray<FPrimaryAssetId>&, RewardItemIds
);
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FBBMOnCombatVictoryNativeSignature,
	const TArray<UDISItemStack*>&, /* UsedEquipment */
	const TArray<FPrimaryAssetId>& /* RewardItemIds */
);

/* 전투 패배 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBBMOnCombatDefeatSignature);
DECLARE_MULTICAST_DELEGATE(FBBMOnCombatDefeatNativeSignature);

/* 비주얼 연출 이벤트 디스패치 델리게이트 (UI ViewModel 전달용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FBBMOnVisualEventsDispatchedSignature,
	const TArray<FBBMCombatStepVisualEvent>&, Events
);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FBBMOnVisualEventsDispatchedNativeSignature,
	const TArray<FBBMCombatStepVisualEvent>& /* Events */
);

/**
 * BBM 인카운터 액터 라이프사이클 및 1D 타일 라인 전투 관리 월드 서브시스템
 */
UCLASS(meta = (DisplayName = "BBM Combat Manager Subsystem"))
class BBM_API UBBMCombatManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/* 생성자 및 서브시스템 라이프사이클 */
public:
	UBBMCombatManagerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 1. 인카운터 액터 라이프사이클 관리 (스폰 및 퇴장 정리) */
public:
	/**
	 * 런타임 만남 데이터를 기반으로 적절한 인카운터 액터들을 월드에 스폰하고 초기 진형을 구축 (C++ 전용)
	 * @param RuntimeData 다형성 만남 런타임 데이터 포인터
	 * @param Merchant 레벨 내 플레이어 상인 폰
	 * @return 인카운터 초기화 성공 여부
	 */
	bool SetupEncounter(const FBBMRuntimeEncounterData* RuntimeData, ABBMPlayerMerchantBase* Merchant);

	/**
	 * TInstancedStruct 래퍼 버전 인카운터 구축 (블루프린트 지원)
	 * @param InEncounterData 다형성 만남 인스턴스 구조체
	 * @param Merchant 레벨 내 플레이어 상인 폰
	 * @return 인카운터 초기화 성공 여부
	 */
	UFUNCTION(Category = "BBM|CombatManager", BlueprintCallable)
	bool SetupEncounterFromInstancedStruct(const TInstancedStruct<FBBMRuntimeEncounterData>& InEncounterData, ABBMPlayerMerchantBase* Merchant);

	/**
	 * 현재 만남에서 스폰되었던 모든 월드 액터를 안전하게 파괴하고 레인 슬롯을 초기화
	 */
	UFUNCTION(Category = "BBM|CombatManager", BlueprintCallable)
	void ClearCurrentEncounter();

	/* 2. 1D 타일 라인 진형 및 배치 관리 */
public:
	/**
	 * 아군(플레이어) 및 적군(몬스터 목록)을 각각의 1D 타일 라인에 초기 배치
	 * @param Merchant 플레이어 상인 (아군 레인 최전방 Index 0에 배치)
	 * @param Monsters 스폰된 몬스터 목록 (적군 레인 Index 0부터 순차 배치)
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable)
	void InitializeBattleFormation(ABBMPlayerMerchantBase* Merchant, const TArray<ABBMAICombatantBase*>& Monsters);

	/**
	 * 특정 레인의 타일 슬롯에 전투원 추가 (소환수 소환 또는 적 증원군)
	 * @param Lane 대상 진영 레인
	 * @param Combatant 추가할 전투원
	 * @param PreferredSlotIndex 희망 슬롯 인덱스 (-1이면 빈 슬롯 탐색 후 추가)
	 * @return 추가 성공 여부
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable)
	bool AddCombatantToLane(EBBMCombatLane Lane, ABBMCombatantBase* Combatant, int32 PreferredSlotIndex = -1);

	/**
	 * 특정 레인에서 전투원 제거 (사망 또는 퇴장)
	 * @param Combatant 제거할 전투원
	 * @return 제거 성공 여부
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable)
	bool RemoveCombatantFromLane(ABBMCombatantBase* Combatant);

	/**
	 * 상대 팀 배열과 맞닿아 있는 첫 번째 생존 최전방 객체 반환
	 * @param Lane 대상 진영 레인
	 * @return 상대 팀과 맞닿아 있는 유효한 첫 번째 전투원 포인터 (없으면 nullptr)
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable, BlueprintPure)
	ABBMCombatantBase* GetFrontlineCombatant(EBBMCombatLane Lane) const;

	/**
	 * 특정 레인에서 살아있는 모든 전투원 목록 반환
	 * @param Lane 대상 진영 레인
	 * @return 생존 전투원 포인터 배열
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable, BlueprintPure)
	TArray<ABBMCombatantBase*> GetLivingCombatantsInLane(EBBMCombatLane Lane) const;

	/**
	 * 특정 레인의 1D 타일 슬롯들을 전진(Frontward) 또는 후퇴(Backward) 방향으로 N칸 시프트
	 * @param Lane 대상 진영 레인
	 * @param Direction 시프트 방향 (Frontward: 상대 쪽 밀착, Backward: 후퇴/넉백)
	 * @param StepCount 이동할 칸 수
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable)
	void ShiftLane(EBBMCombatLane Lane, EBBMLaneShiftDirection Direction, int32 StepCount = 1);

	/**
	 * 특정 레인 내 두 슬롯의 위치를 교환
	 * @param Lane 대상 진영 레인
	 * @param SlotA 교환할 슬롯 A 인덱스
	 * @param SlotB 교환할 슬롯 B 인덱스
	 */
	UFUNCTION(Category = "BBM|CombatManager|Formation", BlueprintCallable)
	void SwapSlots(EBBMCombatLane Lane, int32 SlotA, int32 SlotB);

	/* 3. 공격 및 피해 처리 (Instant Attack) */
public:
	/**
	 * 특정 객체가 상대 단일 객체를 공격하는 데미지 해결 계산 (Instant Effect 방식)
	 * 1단계: 패링 체크 (피해 0) -> 2단계: TurnArmor 차감 -> 3단계: Armor 차감 -> 4단계: Health 차감
	 * @param Instigator 공격 시전자
	 * @param Target 피격 대상
	 * @param BaseDamage 기본 피해량
	 * @param bTargetParrying 대상이 현재 패링 상태인지 여부
	 * @return 최종적으로 대상의 Health에 적용된 실제 피해량
	 */
	UFUNCTION(Category = "BBM|CombatManager|Combat", BlueprintCallable)
	float ResolveAttack(ABBMCombatantBase* Instigator, ABBMCombatantBase* Target, float BaseDamage, bool bTargetParrying = false);

	/**
	 * 특정 객체가 상대 복수 객체를 동시 공격하는 광역 데미지 해결 계산
	 * @param Instigator 공격 시전자
	 * @param Targets 피격 대상 목록
	 * @param BaseDamage 기본 피해량
	 * @return 각 대상의 Health에 적용된 실제 피해량 배열
	 */
	UFUNCTION(Category = "BBM|CombatManager|Combat", BlueprintCallable)
	TArray<float> ResolveMultiAttack(ABBMCombatantBase* Instigator, const TArray<ABBMCombatantBase*>& Targets, float BaseDamage);

	/* 4. 방어도 증가 관리 (지속 방어도 vs 턴 방어도) */
public:
	/**
	 * 대상 객체의 지속 방어도(Armor) 증가 (전투 내내 유지됨)
	 * @param Target 대상 전투원
	 * @param Amount 증가할 방어도 수치
	 */
	UFUNCTION(Category = "BBM|CombatManager|Armor", BlueprintCallable)
	void AddPersistentArmor(ABBMCombatantBase* Target, float Amount);

	/**
	 * 대상 객체의 턴 방어도(TurnArmor) 증가 (해당 턴(8비트) 동안만 유지됨)
	 * @param Target 대상 전투원
	 * @param Amount 증가할 턴 방어도 수치
	 */
	UFUNCTION(Category = "BBM|CombatManager|Armor", BlueprintCallable)
	void AddTurnArmor(ABBMCombatantBase* Target, float Amount);

	/**
	 * 특정 대상 객체의 턴 방어도(TurnArmor)를 0으로 리셋
	 * @param Target 대상 전투원
	 */
	UFUNCTION(Category = "BBM|CombatManager|Armor", BlueprintCallable)
	void ClearTurnArmor(ABBMCombatantBase* Target);

	/**
	 * 전투에 참여 중인 모든 객체의 턴 방어도(TurnArmor)를 0으로 일괄 초기화 (턴 종료 시 호출)
	 */
	UFUNCTION(Category = "BBM|CombatManager|Armor", BlueprintCallable)
	void ClearAllTurnArmors();

	/* 5. GameplayTag 기반 상태이상 관리 */
public:
	/**
	 * 대상 객체에 상태이상 태그 스택을 추가
	 * @param Target 대상 전투원
	 * @param StatusTag 상태이상 태그 (BBM.Status.Poison, BBM.Status.Burn 등)
	 * @param StacksToAdd 추가할 스택 수
	 */
	UFUNCTION(Category = "BBM|CombatManager|Status", BlueprintCallable)
	void ApplyStatusEffect(ABBMCombatantBase* Target, const FGameplayTag& StatusTag, int32 StacksToAdd = 1);

	/**
	 * 대상 객체의 특정 상태이상 태그 현재 누적 스택 수 반환
	 * @param Target 대상 전투원
	 * @param StatusTag 상태이상 태그
	 * @return 현재 누적 스택 수 (없으면 0)
	 */
	UFUNCTION(Category = "BBM|CombatManager|Status", BlueprintCallable, BlueprintPure)
	int32 GetStatusStacks(const ABBMCombatantBase* Target, const FGameplayTag& StatusTag) const;

	/**
	 * 턴(8비트) 종료 시 등록된 모든 전투원의 상태이상 틱(Tick)을 일괄 처리하고 스택을 1씩 차감
	 * 독: 현재 스택 수만큼 피해 적용 후 스택 1 감소
	 * 화상: 고정 기본 피해(5.0f) 적용 후 스택 1 감소
	 */
	UFUNCTION(Category = "BBM|CombatManager|Status", BlueprintCallable)
	void ProcessTurnEndStatusTicks();

	/* 6. 비트 단위 아이템 GAS GameplayEffect 라이프사이클 관리 */
public:
	/**
	 * 아이템 사용 확정(비트 시작) 시 ItemEntry의 동적 Granted GameplayEffect들을 전투원 ASC에 적용
	 * @param TargetCombatant 대상 전투원 액터
	 * @param ItemEntry 적용할 아이템 엔트리
	 * @param OutHandles 적용된 ActiveGameplayEffectHandle 목록 반환
	 * @return 적용 성공 여부
	 */
	UFUNCTION(Category = "BBM|CombatManager|ItemEffect", BlueprintCallable)
	static bool ApplyItemGrantedEffects(ABBMCombatantBase* TargetCombatant, UBBMItemEntry* ItemEntry, TArray<FActiveGameplayEffectHandle>& OutHandles);

	/**
	 * 비트 종료 시 부여되었던 아이템의 Active GameplayEffect들을 전투원 ASC에서 일괄 제거하여 스탯 원복
	 * @param TargetCombatant 대상 전투원 액터
	 * @param InHandles 제거할 ActiveGameplayEffectHandle 목록
	 */
	UFUNCTION(Category = "BBM|CombatManager|ItemEffect", BlueprintCallable)
	static void RemoveItemGrantedEffects(ABBMCombatantBase* TargetCombatant, const TArray<FActiveGameplayEffectHandle>& InHandles);

	/* 7. 기획 표준 턴(8비트) 시뮬레이션 및 교전 파이프라인 */
public:
	/**
	 * 1턴(8비트) 동안의 전체 전투 교전을 시뮬레이션하고, UI 연출용 비주얼 이벤트 목록 반환
	 * @param OutVisualEvents 1비트씩 순차 누적된 단계별 비주얼 이벤트 목록
	 * @return 턴 시뮬레이션 정상 완료 여부
	 */
	UFUNCTION(Category = "BBM|CombatManager|Simulation", BlueprintCallable)
	bool SimulateTurn(TArray<FBBMCombatStepVisualEvent>& OutVisualEvents);

	/**
	 * 전투원 사망 시 공통 정리 처리
	 * 1. 걸려있던 Effect Range의 Effect들 제거, 걸려있던 Anim Layer를 제거
	 * 2. 캐싱된 자신과 연관된 행동, Effect Range, Anim Layer 제거
	 * 3. 죽은 대상에게 죽음에 대한 Animation Tag 부여
	 * 4. 대상 1초 뒤 디졸브 가상 함수 호출 (StartDissolve)
	 * 5. 1D 라인 슬롯에서 제거
	 * @param DeadCombatant 사망한 전투원 액터
	 */
	UFUNCTION(Category = "BBM|CombatManager|Simulation", BlueprintCallable)
	void HandleCombatantDeath(ABBMCombatantBase* DeadCombatant);

	/**
	 * 현재 전투 진영의 승패를 검사
	 * @param bOutCombatEnded 전투가 완전히 종료되었는지 여부
	 * @param bOutPlayerVictory 플레이어의 승리 여부
	 * @return 승패 상태 변경 여부
	 */
	UFUNCTION(Category = "BBM|CombatManager|Simulation", BlueprintCallable)
	bool CheckBattleOutcome(bool& bOutCombatEnded, bool& bOutPlayerVictory);

	/* 8. 전투 보상 아이템 관리 */
public:
	/* 전투 승리 시 지급할 보상 아이템 PAD 목록 설정 */
	UFUNCTION(Category = "BBM|CombatManager|Reward", BlueprintCallable)
	void SetPendingRewardItems(const TArray<FPrimaryAssetId>& InRewardItemIds);

	/* 전투 승리 시 지급할 보상 아이템 PAD 추가 */
	UFUNCTION(Category = "BBM|CombatManager|Reward", BlueprintCallable)
	void AddPendingRewardItem(const FPrimaryAssetId& InRewardItemId);

	/* 현재 등록된 전투 승리 보상 아이템 PAD 목록 반환 */
	UFUNCTION(Category = "BBM|CombatManager|Reward", BlueprintCallable, BlueprintPure)
	const TArray<FPrimaryAssetId>& GetPendingRewardItems() const;

	/* 현재 등록된 전투 승리 보상 아이템 PAD 목록 초기화 */
	UFUNCTION(Category = "BBM|CombatManager|Reward", BlueprintCallable)
	void ClearPendingRewardItems();

	/* 델리게이트 이벤트 */
public:
	/* 승리 시 이벤트 디스패처 (블루프린트용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|CombatManager|Event")
	FBBMOnCombatVictorySignature OnCombatVictory;

	/* 승리 시 이벤트 디스패처 (C++ 네이티브 바인딩용) */
	FBBMOnCombatVictoryNativeSignature OnCombatVictoryNative;

	/* 패배 시 이벤트 디스패처 (블루프린트용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|CombatManager|Event")
	FBBMOnCombatDefeatSignature OnCombatDefeat;

	/* 패배 시 이벤트 디스패처 (C++ 네이티브 바인딩용) */
	FBBMOnCombatDefeatNativeSignature OnCombatDefeatNative;

	/* 비트 연출 이벤트 목록 디스패처 (UI ViewModel 통지용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|CombatManager|Event")
	FBBMOnVisualEventsDispatchedSignature OnVisualEventsDispatched;

	/* 비트 연출 이벤트 목록 디스패처 (C++ 네이티브 바인딩용) */
	FBBMOnVisualEventsDispatchedNativeSignature OnVisualEventsDispatchedNative;

	/* 헬퍼 및 내부 관리 */
protected:
	/* 비트 시작 시 Effect Range 및 Anim Layer 부여/링크 */
	void ProcessBeatStart(int32 BeatIndex);

	/* 비트 종료 시 Effect Range 및 Anim Layer 해제/언링크 */
	void ProcessBeatEnd(int32 BeatIndex);

	/* 특정 비트 및 특정 카테고리의 전투원 액션 실행 및 비주얼 이벤트 기록 */
	void ExecuteCombatantsActionForCategory(EBBMCombatLane Lane, int32 BeatIndex, EBBMActionCategory Category, EBBMVisualWindow Window, TArray<FBBMCombatStepVisualEvent>& OutVisualEvents);

	/* 체력이 0 이하인 전투원 검사 및 사망 처리 수행 */
	void CheckAndProcessDeadCombatants(TArray<FBBMCombatStepVisualEvent>& OutVisualEvents, int32 BeatIndex);

	/* 1D 타일 앵커 포인트 및 유닛들의 월드 좌표 동기화 */
	void SynchronizeLaneWorldTransforms();

	/* 내부 프로퍼티 */
protected:
	/* 활성화된 전투 만남 정의 에셋 (보상 아이템 조회용) */
	UPROPERTY(Transient)
	TObjectPtr<const UBBMCombatEncounterDefinition> ActiveCombatEncounterDef;

	/* 비트 진행 중 각 전투원에 부여된 Active Effect 핸들 관리 */
	TMap<TWeakObjectPtr<ABBMCombatantBase>, TArray<FActiveGameplayEffectHandle>> ActiveBeatRangeHandles;

	/* 비트 진행 중 각 전투원에 링크된 Anim Layer 식별 태그 관리 */
	TMap<TWeakObjectPtr<ABBMCombatantBase>, TArray<FGameplayTag>> ActiveBeatAnimLayerTags;

	/* 아군 1D 라인 레인 */
	UPROPERTY(VisibleAnywhere, Category = "BBM|CombatManager")
	FBBM1DCombatLane AllyLane;

	/* 적군 1D 라인 레인 */
	UPROPERTY(VisibleAnywhere, Category = "BBM|CombatManager")
	FBBM1DCombatLane EnemyLane;

	/* 현재 만남에서 스폰되어 관리 중인 모든 월드 액터 목록 (룸 퇴장 시 일괄 파괴) */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> SpawnedEncounterActors;

	/* 전투 승리 시 전달할 보상 아이템 PrimaryAssetId 목록 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "BBM|CombatManager|Reward")
	TArray<FPrimaryAssetId> PendingRewardItemIds;
};
