/*****************************************************************//**
 * @file   BBMCombatantBase.h
 * @brief  BBM(비트 배낭 상인) 전투원 캐릭터 베이스 폰 클래스 정의
 * @details GAS, UBBMCharacterDefinition 정적 정의, FGenericTeamId 팀 시스템 및 1D 레인 진형을 지원함
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "Item/BBMBeatTypes.h"
#include "Containers/Queue.h"
#include "BBMCombatantBase.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UCapsuleComponent;
class USkeletalMeshComponent;
class UArrowComponent;
class UAbilitySystemComponent;
class UBBMAttributeSet;
class UBBMCharacterDefinition;
class UAnimInstance;
struct FGameplayAttribute;

/**
 * BBM 전투원 캐릭터 베이스 폰 클래스 (주인공 상인, 동료 소환수, 몬스터 공통)
 * 언리얼 공식 GAS 및 GenericTeamAgentInterface를 구현하며, 정적 Definition 기반 스탯 초기화를 전담함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Combatant Base"))
class BBM_API ABBMCombatantBase : public APawn, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMCombatantBase();

	/* APawn 가상 함수 오버라이드 */
public:
	virtual void PostInitializeComponents() override;

	/* IAbilitySystemInterface 인터페이스 구현 */
public:
	/* 전투원의 AbilitySystemComponent 반환 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/* IGenericTeamAgentInterface 인터페이스 구현 */
public:
	/* 팀 식별자 설정 */
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;

	/* 팀 식별자 반환 */
	virtual FGenericTeamId GetGenericTeamId() const override;

	/* 대상 액터에 대한 상호 태도(Friendly, Hostile, Neutral) 반환 */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/* 정적 정의 에셋(CharacterDefinition) 관리 */
public:
	/* 캐릭터 정적 정의 에셋 반환 */
	UFUNCTION(Category = "Combat|Character", BlueprintCallable, BlueprintPure)
	const UBBMCharacterDefinition* GetCharacterDefinition() const;

	/* 캐릭터 정적 정의 에셋 설정 및 초기화 동기화 */
	UFUNCTION(Category = "Combat|Character", BlueprintCallable)
	void SetCharacterDefinition(const UBBMCharacterDefinition* InDef);

	/* 캐릭터 정적 정의 기반 초기화 가상 함수 (팀 설정, 스탯 초기화 등, 자식 클래스에서 확장 가능) */
	UFUNCTION(Category = "Combat|Character", BlueprintCallable)
	virtual void InitializeFromDefinition();

	/* 비트 행동 시퀀스 관리 */
public:
	/* 이번 턴에 실행할 8비트 행동 시퀀스 갱신 (자식 클래스에서 오버라이드하여 캐시 업데이트) */
	UFUNCTION(Category = "BBM|Beat", BlueprintCallable)
	virtual void UpdateBeatSequence();

	/* 이번 턴에 캐시된 8비트 행동 시퀀스 반환 */
	UFUNCTION(Category = "BBM|Beat", BlueprintCallable, BlueprintPure)
	virtual FBBMBeatSequence GetCurrentBeatSequence() const;

	/* 어트리뷰트 세트 조회 */
public:
	/* 전투원의 AttributeSet 반환 */
	UFUNCTION(Category = "Combat|GAS", BlueprintCallable, BlueprintPure)
	UBBMAttributeSet* GetAttributeSet() const;

	/* GAS 스탯 초기화 */
public:
	/* CharacterDefinition의 고정 TableKeyName과 현재 CharacterLevel을 사용하여 스탯 초기화 */
	UFUNCTION(Category = "Combat|Stats", BlueprintCallable)
	void InitializeStats();

	/* 지정된 레벨로 갱신하여 CharacterDefinition의 고정 TableKeyName 기반 스탯 초기화 */
	UFUNCTION(Category = "Combat|Stats", BlueprintCallable)
	void InitializeStatsForLevel(int32 InLevel);

	/* 어트리뷰트 수치 범용 조회 헬퍼 */
public:
	/* 특정 어트리뷰트의 현재 수치(CurrentValue) 반환 (ASC 부재 시 0.0f) */
	UFUNCTION(Category = "Combat|Stats", BlueprintCallable, BlueprintPure)
	float GetAttributeValue(FGameplayAttribute Attribute) const;

	/* 특정 어트리뷰트의 기본 수치(BaseValue) 반환 */
	UFUNCTION(Category = "Combat|Stats", BlueprintCallable, BlueprintPure)
	float GetAttributeBaseValue(FGameplayAttribute Attribute) const;

	/* 전투원 사망 여부 확인 (Health <= 0.0f) */
	UFUNCTION(Category = "Combat|State", BlueprintCallable, BlueprintPure)
	bool IsDead() const;

	/* 팀 편의 조회 및 관계 헬퍼 */
public:
	/* 열거형 기반 팀 설정 */
	UFUNCTION(Category = "Combat|Team", BlueprintCallable)
	void SetTeamType(EBBMTeamType InTeamType);

	/* 열거형 기반 팀 반환 */
	UFUNCTION(Category = "Combat|Team", BlueprintCallable, BlueprintPure)
	EBBMTeamType GetTeamType() const;

	/* 대상 액터와의 우호(Friendly) 여부 확인 */
	UFUNCTION(Category = "Combat|Team", BlueprintCallable, BlueprintPure)
	bool IsFriendlyTowards(const AActor* Other) const;

	/* 대상 액터와의 적대(Hostile) 여부 확인 */
	UFUNCTION(Category = "Combat|Team", BlueprintCallable, BlueprintPure)
	bool IsHostileTowards(const AActor* Other) const;

	/* 1D 레인 진형 위치 조회 및 설정 */
public:
	/* 1D 라인 진형 인덱스 반환 */
	UFUNCTION(Category = "Combat|Formation", BlueprintCallable, BlueprintPure)
	int32 GetLaneIndex() const;

	/* 1D 라인 진형 인덱스 설정 */
	UFUNCTION(Category = "Combat|Formation", BlueprintCallable)
	void SetLaneIndex(int32 InLaneIndex);

	/* 기본 캐릭터 컴포넌트 접근자 (ACharacter 규격) */
public:
	/* 캡슐 콜리전 컴포넌트 반환 */
	UFUNCTION(Category = "Combat|Component", BlueprintCallable, BlueprintPure)
	UCapsuleComponent* GetCapsuleComponent() const;

	/* 스켈레탈 메시 컴포넌트 반환 */
	UFUNCTION(Category = "Combat|Component", BlueprintCallable, BlueprintPure)
	USkeletalMeshComponent* GetMesh() const;

	/* Anim Layer 링크 및 언링크 인터페이스 */
public:
	/* 특정 태그에 해당하는 캐릭터 전용 Linked Anim Layer를 스켈레탈 메시에 동적 링크 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable)
	virtual void LinkAnimLayerByTag(const FGameplayTag& LayerTag);

	/* 특정 태그에 해당하는 캐릭터 전용 Linked Anim Layer를 스켈레탈 메시에서 동적 언링크 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable)
	virtual void UnlinkAnimLayerByTag(const FGameplayTag& LayerTag);

	/* Anim Layer 클래스 목록을 스켈레탈 메시에 동적 링크 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable)
	virtual void LinkAnimLayers(const TArray<TSubclassOf<UAnimInstance>>& InLayers);

	/* Anim Layer 클래스 목록을 스켈레탈 메시에서 동적 언링크 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable)
	virtual void UnlinkAnimLayers(const TArray<TSubclassOf<UAnimInstance>>& InLayers);

	/* 애니메이션 상태 태그 큐 관리 인터페이스 */
public:
	/* 애니메이션 상태 태그를 FIFO 큐에 삽입 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable)
	void EnqueueAnimationTag(const FGameplayTag& InTag);

	/* 애니메이션 상태 태그 큐에서 1개 추출 (큐가 비어있으면 false) */
	bool DequeueAnimationTag(FGameplayTag& OutTag);

	/* 대기 중인 애니메이션 태그가 존재하는지 여부 반환 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable, BlueprintPure)
	bool HasPendingAnimationTags() const;

	/* 애니메이션 상태 태그 큐 전체 비우기 */
	UFUNCTION(Category = "Combat|Animation", BlueprintCallable)
	void ClearAnimationTags();

	/* 사망 시 디졸브(Dissolve) 연출 시작 가상 함수 (껍데기 함수) */
	UFUNCTION(Category = "Combat|Visual", BlueprintCallable)
	virtual void StartDissolve();

#if WITH_EDITORONLY_DATA
	/* 에디터 전용 방향 표시 화살표 컴포넌트 반환 */
	UArrowComponent* GetArrowComponent() const;
#endif

	/* 기본 캐릭터 컴포넌트 프로퍼티 (ACharacter 규격) */
protected:
	/* 이동 및 충돌 판정용 캡슐 콜리전 컴포넌트 (루트 컴포넌트) */
	UPROPERTY(Category = "Combat|Character", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/* 캐릭터 3D 외형을 렌더링하는 스켈레탈 메시 컴포넌트 */
	UPROPERTY(Category = "Combat|Character", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;

#if WITH_EDITORONLY_DATA
	/* 에디터 뷰포트에서 캐릭터 정면 방향을 시각화하는 화살표 컴포넌트 */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

	/* 컴포넌트 프로퍼티 */
protected:
	/* 언리얼 Gameplay Ability System 컴포넌트 */
	UPROPERTY(Category = "Combat|GAS", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/* BBM 5대 핵심 스탯 어트리뷰트 세트 */
	UPROPERTY(Category = "Combat|GAS", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBBMAttributeSet> AttributeSet;

	/* 정적 정의 에셋 */
protected:
	/* 캐릭터 정적 정의 에셋 (표시명, 스탯 테이블 키, 기본 팀) */
	UPROPERTY(Category = "Combat|Character", EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true", AssetBundles = "Data"))
	TObjectPtr<const UBBMCharacterDefinition> CharacterDefinition;

	/* 런타임 상태 프로퍼티 */
protected:
	/* 런타임 팀 식별자 */
	UPROPERTY(Category = "Combat|Team", EditAnywhere, BlueprintReadWrite)
	FGenericTeamId TeamId;

	/* 런타임 캐릭터 레벨 (기본값 1) */
	UPROPERTY(Category = "Combat|Stats", EditAnywhere, BlueprintReadWrite)
	int32 CharacterLevel;

	/* 1D 레인 진형 인덱스 (0: 최전방 Frontline, 1 이상: 후방 Backline) */
	UPROPERTY(Category = "Combat|Formation", EditAnywhere, BlueprintReadWrite)
	int32 LaneIndex;

	/* 이번 턴에 실행할 캐시된 8비트 행동 시퀀스 */
	UPROPERTY(Category = "BBM|Beat", Transient, BlueprintReadOnly)
	FBBMBeatSequence CachedBeatSequence;

	/* 애님 인스턴스가 순차 소비할 애니메이션 상태 태그 FIFO 큐 */
	TQueue<FGameplayTag> AnimationTagQueue;
};
