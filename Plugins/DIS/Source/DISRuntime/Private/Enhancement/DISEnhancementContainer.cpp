#include "Enhancement/DISEnhancementContainer.h"
#include "Enhancement/DISEnhancement.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "Enhancement/DISEnhanceableInterface.h"
#include "Global/DISGlobalSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

/* FScopedEnhancementContainerLock */

FScopedEnhancementContainerLock::FScopedEnhancementContainerLock(const UDISEnhancementContainer& InContainer) :
	Container(InContainer)
{
	Container.IncrementLock();
}

FScopedEnhancementContainerLock::~FScopedEnhancementContainerLock()
{
	const_cast<UDISEnhancementContainer&>(Container).DecrementLock();
}

/* UDISEnhancementContainer */

UDISEnhancementContainer::UDISEnhancementContainer() :
	OwnerInterface(nullptr),
	ScopedLockCount(0),
	PendingEffectRemoves(0),
	PendingEnhancementRemoves(0),
	bIsDisplayNameDirty(true)
{
}

void UDISEnhancementContainer::Initialize(const TScriptInterface<IDISEnhanceableInterface>& InOwner, UObject* InEffectTarget)
{
	OwnerInterface = InOwner;
	SetEffectTarget(InEffectTarget);
	MarkDisplayNameDirty();
}

TScriptInterface<IDISEnhanceableInterface> UDISEnhancementContainer::GetOwnerInterface() const
{
	if (OwnerInterface.GetObject() != nullptr)
	{
		return OwnerInterface;
	}

	// 명시적 소유자가 지정되지 않은 경우 Outer 객체로부터 인터페이스 구현 여부 탐색
	UObject* OuterObject = GetOuter();
	if (OuterObject != nullptr && OuterObject->Implements<UDISEnhanceableInterface>() == true)
	{
		TScriptInterface<IDISEnhanceableInterface> ResolvedInterface;
		ResolvedInterface.SetObject(OuterObject);
		ResolvedInterface.SetInterface(Cast<IDISEnhanceableInterface>(OuterObject));
		return ResolvedInterface;
	}

	return nullptr;
}

void UDISEnhancementContainer::AddEffectTarget(UObject* InEffectTarget)
{
	if (InEffectTarget == nullptr)
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& ExistingTarget : EffectTargets)
	{
		if (ExistingTarget.Get() == InEffectTarget)
		{
			return;
		}
	}

	EffectTargets.Add(InEffectTarget);
	OnTargetAdded(InEffectTarget);
}

bool UDISEnhancementContainer::RemoveEffectTarget(UObject* InEffectTarget)
{
	if (InEffectTarget == nullptr)
	{
		return false;
	}

	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < EffectTargets.Num(); ++Index)
	{
		if (EffectTargets[Index].Get() == InEffectTarget)
		{
			FoundIndex = Index;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	OnTargetRemoved(InEffectTarget);
	EffectTargets.RemoveAt(FoundIndex);
	return true;
}

void UDISEnhancementContainer::ClearEffectTargets()
{
	for (int32 Index = EffectTargets.Num() - 1; Index >= 0; --Index)
	{
		if (UObject* Target = EffectTargets[Index].Get())
		{
			OnTargetRemoved(Target);
		}
	}
	EffectTargets.Empty();
}

TArray<UObject*> UDISEnhancementContainer::GetEffectTargets() const
{
	TArray<UObject*> Result;
	Result.Reserve(EffectTargets.Num());

	for (const TWeakObjectPtr<UObject>& TargetPtr : EffectTargets)
	{
		if (UObject* Target = TargetPtr.Get())
		{
			Result.Add(Target);
		}
	}

	return Result;
}

void UDISEnhancementContainer::SetEffectTargets(const TArray<UObject*>& InEffectTargets)
{
	// 기존 타깃 중 InEffectTargets에 없는 타깃은 제거
	for (int32 Index = EffectTargets.Num() - 1; Index >= 0; --Index)
	{
		UObject* CurrentTarget = EffectTargets[Index].Get();
		if (CurrentTarget == nullptr || InEffectTargets.Contains(CurrentTarget) == false)
		{
			if (CurrentTarget != nullptr)
			{
				OnTargetRemoved(CurrentTarget);
			}
			EffectTargets.RemoveAt(Index);
		}
	}

	// 신규 타깃 중 아직 등록되지 않은 타깃 추가
	for (UObject* NewTarget : InEffectTargets)
	{
		if (NewTarget == nullptr)
		{
			continue;
		}

		bool bAlreadyExists = false;
		for (const TWeakObjectPtr<UObject>& ExistingPtr : EffectTargets)
		{
			if (ExistingPtr.Get() == NewTarget)
			{
				bAlreadyExists = true;
				break;
			}
		}

		if (bAlreadyExists == false)
		{
			EffectTargets.Add(NewTarget);
			OnTargetAdded(NewTarget);
		}
	}
}

void UDISEnhancementContainer::SetEffectTarget(UObject* InEffectTarget)
{
	if (InEffectTarget == nullptr)
	{
		ClearEffectTargets();
		return;
	}

	if (EffectTargets.Num() == 1 && EffectTargets[0].Get() == InEffectTarget)
	{
		return;
	}

	SetEffectTargets({ InEffectTarget });
}

UObject* UDISEnhancementContainer::GetEffectTarget() const
{
	for (const TWeakObjectPtr<UObject>& TargetPtr : EffectTargets)
	{
		if (UObject* Target = TargetPtr.Get())
		{
			return Target;
		}
	}
	return nullptr;
}

FDISActiveEnhancementEffectHandle UDISEnhancementContainer::ApplyEnhancementEffectByClass(TSubclassOf<UDISEnhancementEffect> InEffectClass)
{
	if (InEffectClass == nullptr)
	{
		return FDISActiveEnhancementEffectHandle();
	}

	return ApplyEnhancementEffectSpec(FDISEnhancementEffectSpec(InEffectClass));
}

FDISActiveEnhancementEffectHandle UDISEnhancementContainer::ApplyEnhancementEffectSpec(const FDISEnhancementEffectSpec& InSpec)
{
	if (InSpec.IsValid() == false)
	{
		return FDISActiveEnhancementEffectHandle();
	}

	ENHANCEMENT_SCOPE_LOCK();

	const FDISActiveEnhancementEffectHandle NewHandle = AllocateEffectHandle();
	if (NewHandle.IsValid() == false)
	{
		return FDISActiveEnhancementEffectHandle();
	}

	FDISActiveEnhancementEffect NewEffect(InSpec, NewHandle);

	// 락 중이므로 PendingAddEffects에 지연 추가 (스코프 종료 시 ActiveEffects로 플러시)
	FDISActiveEnhancementEffect& AddedEffect = PendingAddEffects.Add_GetRef(MoveTemp(NewEffect));

	// 영향을 받는 대상 강화 스탯들에 모디파이어 추가 및 라이프사이클 전파 (즉시 적용)
	for (const auto& Pair : AddedEffect.GetCalculatedModifiers())
	{
		AddModifierToEnhancement(Pair.Key, Pair.Value.Target, Pair.Value.LevelOp, NewHandle, Pair.Value.LevelMagnitude);
	}

	MarkDisplayNameDirty();

	return NewHandle;
}

bool UDISEnhancementContainer::RemoveEnhancementEffect(const FDISActiveEnhancementEffectHandle& InHandle)
{
	if (InHandle.IsValid() == false)
	{
		return false;
	}

	ENHANCEMENT_SCOPE_LOCK();

	FDISActiveEnhancementEffect* FoundEffect = FindActiveEnhancementEffect(InHandle);
	if (FoundEffect == nullptr || FoundEffect->IsPendingRemove() == true)
	{
		// 대상이 없거나 이미 삭제 대기 중인 경우 중복 삭제 방지
		return false;
	}

	// 1. 삭제 대기 플래그 마킹
	FoundEffect->MarkPendingRemove();
	PendingEffectRemoves++;

	// 2. 핸들 해제
	ReleaseEffectHandle(InHandle);

	// 3. 영향받은 모든 대상 강화 스탯에서 모디파이어 제거 및 라이프사이클 처리 (즉시 적용)
	const TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier> ModifiersToRemove = FoundEffect->GetCalculatedModifiers();
	for (const auto& Pair : ModifiersToRemove)
	{
		RemoveModifierFromEnhancement(Pair.Key, InHandle);
	}

	MarkDisplayNameDirty();

	return true;
}

int32 UDISEnhancementContainer::RemoveEnhancementEffectsByClass(TSubclassOf<UDISEnhancementEffect> InEffectClass)
{
	if (InEffectClass == nullptr)
	{
		return 0;
	}

	ENHANCEMENT_SCOPE_LOCK();

	TArray<FDISActiveEnhancementEffectHandle> HandlesToRemove;
	for (FConstEffectIterator It(*this); It; ++It)
	{
		if (It->GetEffectClass() == InEffectClass)
		{
			HandlesToRemove.Add(It->GetHandle());
		}
	}

	int32 RemovedTotalCount = 0;
	for (const FDISActiveEnhancementEffectHandle& Handle : HandlesToRemove)
	{
		if (RemoveEnhancementEffect(Handle) == true)
		{
			RemovedTotalCount++;
		}
	}

	return RemovedTotalCount;
}

const FDISActiveEnhancementEffect* UDISEnhancementContainer::FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle) const
{
	return const_cast<UDISEnhancementContainer*>(this)->FindActiveEnhancementEffect(Handle);
}

bool UDISEnhancementContainer::K2_FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle, FDISActiveEnhancementEffect& OutActiveEffect) const
{
	if (const FDISActiveEnhancementEffect* Found = FindActiveEnhancementEffect(Handle))
	{
		OutActiveEffect = *Found;
		return true;
	}

	OutActiveEffect = FDISActiveEnhancementEffect();
	return false;
}

const TArray<FDISActiveEnhancementEffect>& UDISEnhancementContainer::GetActiveEnhancementEffects() const
{
	return ActiveEffects;
}

const FDISActiveEnhancement* UDISEnhancementContainer::FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	return const_cast<UDISEnhancementContainer*>(this)->FindActiveEnhancement(InEnhancementClass);
}

bool UDISEnhancementContainer::K2_FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, FDISActiveEnhancement& OutActiveEnhancement) const
{
	if (const FDISActiveEnhancement* Found = FindActiveEnhancement(InEnhancementClass))
	{
		OutActiveEnhancement = *Found;
		return true;
	}

	OutActiveEnhancement = FDISActiveEnhancement();
	return false;
}

const TArray<FDISActiveEnhancement>& UDISEnhancementContainer::GetActiveEnhancements() const
{
	return ActiveEnhancements;
}

bool UDISEnhancementContainer::HasIdenticalEnhancements(const UDISEnhancementContainer* OtherContainer) const
{
	// 유효한 활성 강화 클래스 및 레벨(Level > 0)을 수집하는 람다
	// (Level이 0 이하인 강화는 실질적인 효과가 없으므로 타 컨테이너에 미등록 상태여도 동일한 것으로 취급)
	auto CollectEffectiveLevels = [](const UDISEnhancementContainer* Container, TMap<TSubclassOf<UDISEnhancement>, int32>& OutLevels)
	{
		if (Container == nullptr)
		{
			return;
		}

		auto ProcessEnhancementList = [&](const TArray<FDISActiveEnhancement>& EnhancementList)
		{
			for (const FDISActiveEnhancement& ActiveEnhancement : EnhancementList)
			{
				if (ActiveEnhancement.IsPendingRemove() == true)
				{
					continue;
				}

				const int32 Level = ActiveEnhancement.GetLevel();
				// Level이 0 이하인 강화는 실질적인 효과가 없으므로 수집하지 않음 (타 컨테이너에 미등록 상태와 동등하게 일치 간주)
				if (Level > 0)
				{
					OutLevels.Add(ActiveEnhancement.GetEnhancementClass(), Level);
				}
			}
		};

		ProcessEnhancementList(Container->ActiveEnhancements);
		ProcessEnhancementList(Container->PendingAddEnhancements);
	};

	TMap<TSubclassOf<UDISEnhancement>, int32> MyLevels;
	CollectEffectiveLevels(this, MyLevels);

	TMap<TSubclassOf<UDISEnhancement>, int32> OtherLevels;
	CollectEffectiveLevels(OtherContainer, OtherLevels);

	// 유효 강화 개수가 다르면 불일치
	if (MyLevels.Num() != OtherLevels.Num())
	{
		return false;
	}

	// 모든 항목의 종류 및 레벨 1:1 비교
	for (const auto& Pair : MyLevels)
	{
		const int32* FoundLevel = OtherLevels.Find(Pair.Key);
		if (FoundLevel == nullptr || *FoundLevel != Pair.Value)
		{
			return false;
		}
	}

	return true;
}

void UDISEnhancementContainer::UpdateCachedAffixes() const
{
	if (bIsDisplayNameDirty == false)
	{
		return;
	}

	CachedPrefix.Reset();
	CachedSuffix.Reset();

	// 1. 유효한 활성 강화의 접두사와 접미사를 수집
	struct FAffixSortEntry
	{
		FText Text;
		int32 Priority = 0;
		int32 OrderIndex = 0;
	};

	TArray<FAffixSortEntry> ValidPrefixes;
	TArray<FAffixSortEntry> ValidSuffixes;

	auto CollectAffixes = [&](const TArray<FDISActiveEnhancement>& InEnhancements)
	{
		for (int32 Index = 0; Index < InEnhancements.Num(); ++Index)
		{
			const FDISActiveEnhancement& ActiveEnhancement = InEnhancements[Index];
			// 삭제 대기 중이거나 강화 레벨이 0 이하인 경우 수식어(Affixes)로 수집하지 않음
			if (ActiveEnhancement.IsPendingRemove() == true || ActiveEnhancement.GetLevel() <= 0)
			{
				continue;
			}

			const UDISEnhancement* CDO = ActiveEnhancement.GetEnhancementCDO();
			if (CDO == nullptr || ActiveEnhancement.bIsActive == false)
			{
				continue;
			}

			const int32 CurrentLevel = ActiveEnhancement.GetLevel();
			const FText PrefixText = CDO->GetPrefixText(CurrentLevel);
			if (PrefixText.IsEmptyOrWhitespace() == false)
			{
				ValidPrefixes.Add({ PrefixText, CDO->GetPrefixPriority(), Index });
			}

			const FText SuffixText = CDO->GetSuffixText(CurrentLevel);
			if (SuffixText.IsEmptyOrWhitespace() == false)
			{
				ValidSuffixes.Add({ SuffixText, CDO->GetSuffixPriority(), Index });
			}
		}
	};

	CollectAffixes(ActiveEnhancements);
	CollectAffixes(PendingAddEnhancements);

	// 2. 우선순위 정렬 람다 (우선순위 내림차순: 높은 값이 앞쪽, 동률 시 등록 순서 유지)
	auto AffixSorter = [](const FAffixSortEntry& A, const FAffixSortEntry& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}
		return A.OrderIndex < B.OrderIndex;
	};

	// 3. 접두사 정렬 및 결합 (우선순위 높은 것이 앞쪽)
	if (ValidPrefixes.IsEmpty() == false)
	{
		ValidPrefixes.Sort(AffixSorter);
		for (int32 Index = 0; Index < ValidPrefixes.Num(); ++Index)
		{
			if (Index > 0)
			{
				CachedPrefix.Append(TEXT(" "));
			}
			CachedPrefix.Append(ValidPrefixes[Index].Text.ToString());
		}
	}

	// 4. 접미사 정렬 및 결합 (우선순위 높은 것이 앞쪽)
	if (ValidSuffixes.IsEmpty() == false)
	{
		ValidSuffixes.Sort(AffixSorter);
		for (int32 Index = 0; Index < ValidSuffixes.Num(); ++Index)
		{
			if (Index > 0)
			{
				CachedSuffix.Append(TEXT(" "));
			}
			CachedSuffix.Append(ValidSuffixes[Index].Text.ToString());
		}
	}

	bIsDisplayNameDirty = false;
}

FText UDISEnhancementContainer::GetEnhancedDisplayName(const FText& BaseName) const
{
	UpdateCachedAffixes();

	// 수식어가 모두 없는 경우 기본 이름 그대로 반환
	if (CachedPrefix.IsEmpty() == true && CachedSuffix.IsEmpty() == true)
	{
		return BaseName;
	}

	FString ResultString;
	if (CachedPrefix.IsEmpty() == false)
	{
		ResultString.Append(CachedPrefix);
		ResultString.Append(TEXT(" "));
	}

	ResultString.Append(BaseName.ToString());

	if (CachedSuffix.IsEmpty() == false)
	{
		ResultString.Append(TEXT(" "));
		ResultString.Append(CachedSuffix);
	}

	return FText::FromString(ResultString.TrimStartAndEnd());
}

FString UDISEnhancementContainer::GetCachedPrefix() const
{
	UpdateCachedAffixes();
	return CachedPrefix;
}

FString UDISEnhancementContainer::GetCachedSuffix() const
{
	UpdateCachedAffixes();
	return CachedSuffix;
}

void UDISEnhancementContainer::MarkDisplayNameDirty()
{
	bIsDisplayNameDirty = true;
}

void UDISEnhancementContainer::IncrementLock() const
{
	ScopedLockCount++;
}

void UDISEnhancementContainer::DecrementLock()
{
	check(ScopedLockCount > 0);
	if (--ScopedLockCount == 0)
	{
		// 1. PendingAddEffects 중 삭제 대기(bPendingRemove)가 아닌 항목들을 ActiveEffects로 이동
		for (FDISActiveEnhancementEffect& PendingEffect : PendingAddEffects)
		{
			if (PendingEffect.IsPendingRemove() == false)
			{
				ActiveEffects.Add(MoveTemp(PendingEffect));
			}
		}
		PendingAddEffects.Empty();

		// 2. PendingAddEnhancements 중 삭제 대기(bPendingRemove)가 아닌 항목들을 ActiveEnhancements로 이동
		for (FDISActiveEnhancement& PendingEnh : PendingAddEnhancements)
		{
			if (PendingEnh.IsPendingRemove() == false)
			{
				ActiveEnhancements.Add(MoveTemp(PendingEnh));
			}
		}
		PendingAddEnhancements.Empty();

		// 3. ActiveEffects에서 삭제 대기(bPendingRemove) 마킹된 항목들을 뒤에서부터 일괄 제거
		if (PendingEffectRemoves > 0)
		{
			for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
			{
				if (ActiveEffects[Index].IsPendingRemove() == true)
				{
					ActiveEffects.RemoveAtSwap(Index, EAllowShrinking::No);
				}
			}
			PendingEffectRemoves = 0;
		}

		// 4. ActiveEnhancements에서 삭제 대기(bPendingRemove) 마킹된 항목들을 뒤에서부터 일괄 제거
		if (PendingEnhancementRemoves > 0)
		{
			for (int32 Index = ActiveEnhancements.Num() - 1; Index >= 0; --Index)
			{
				if (ActiveEnhancements[Index].IsPendingRemove() == true)
				{
					ActiveEnhancements.RemoveAtSwap(Index, EAllowShrinking::No);
				}
			}
			PendingEnhancementRemoves = 0;
		}

		MarkDisplayNameDirty();
	}
}

int32 UDISEnhancementContainer::ResolveBaseMaxEnhancementLevel(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	if (InEnhancementClass == nullptr)
	{
		return 0;
	}

	if (const TScriptInterface<IDISEnhanceableInterface> Owner = GetOwnerInterface())
	{
		return Owner->GetBaseMaxEnhancementLevel(InEnhancementClass);
	}

	if (const UDISEnhancement* CDO = InEnhancementClass->GetDefaultObject<UDISEnhancement>())
	{
		return CDO->GetDefaultMaxLevel();
	}

	return 0;
}

bool UDISEnhancementContainer::IsLocked() const
{
	return ScopedLockCount > 0;
}

UDISEnhancementContainer::FConstEffectIterator UDISEnhancementContainer::CreateConstEffectIterator() const
{
	return FConstEffectIterator(*this);
}

UDISEnhancementContainer::FEffectIterator UDISEnhancementContainer::CreateEffectIterator()
{
	return FEffectIterator(*this);
}

UDISGlobalSubsystem* UDISEnhancementContainer::GetGlobalSubsystem() const
{
	const UObject* CurrentOuter = this;
	while (CurrentOuter != nullptr)
	{
		if (const UWorld* World = CurrentOuter->GetWorld())
		{
			if (const UGameInstance* GI = World->GetGameInstance())
			{
				return GI->GetSubsystem<UDISGlobalSubsystem>();
			}
		}
		CurrentOuter = CurrentOuter->GetOuter();
	}
	return nullptr;
}

FDISActiveEnhancementEffectHandle UDISEnhancementContainer::AllocateEffectHandle()
{
	if (UDISGlobalSubsystem* GlobalSubsystem = GetGlobalSubsystem())
	{
		return GlobalSubsystem->AllocateEnhancementEffectHandle(this);
	}

	return FDISActiveEnhancementEffectHandle();
}

void UDISEnhancementContainer::ReleaseEffectHandle(const FDISActiveEnhancementEffectHandle& Handle)
{
	if (Handle.IsValid() == false)
	{
		return;
	}

	if (UDISGlobalSubsystem* GlobalSubsystem = GetGlobalSubsystem())
	{
		GlobalSubsystem->ReleaseEnhancementEffectHandle(Handle);
	}
}

FDISActiveEnhancementEffect* UDISEnhancementContainer::FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle)
{
	if (Handle.IsValid() == false)
	{
		return nullptr;
	}

	for (FDISActiveEnhancementEffect& Effect : ActiveEffects)
	{
		if (Effect.GetHandle() == Handle && Effect.IsPendingRemove() == false)
		{
			return &Effect;
		}
	}

	for (FDISActiveEnhancementEffect& PendingEffect : PendingAddEffects)
	{
		if (PendingEffect.GetHandle() == Handle && PendingEffect.IsPendingRemove() == false)
		{
			return &PendingEffect;
		}
	}

	return nullptr;
}

FDISActiveEnhancement* UDISEnhancementContainer::FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	for (FDISActiveEnhancement& Existing : ActiveEnhancements)
	{
		if (Existing.GetEnhancementClass() == InEnhancementClass && Existing.IsPendingRemove() == false)
		{
			return &Existing;
		}
	}

	for (FDISActiveEnhancement& PendingExisting : PendingAddEnhancements)
	{
		if (PendingExisting.GetEnhancementClass() == InEnhancementClass && PendingExisting.IsPendingRemove() == false)
		{
			return &PendingExisting;
		}
	}

	return nullptr;
}

void UDISEnhancementContainer::AddModifierToEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude)
{
	if (InEnhancementClass == nullptr)
	{
		return;
	}

	ENHANCEMENT_SCOPE_LOCK();

	FDISActiveEnhancement* FoundEnhancement = FindActiveEnhancement(InEnhancementClass);
	if (FoundEnhancement == nullptr)
	{
		FoundEnhancement = RegisterEnhancement(InEnhancementClass);
		if (FoundEnhancement == nullptr)
		{
			return;
		}
	}

	const int32 OldLevel = FoundEnhancement->GetLevel();
	FoundEnhancement->AddModifier(InTarget, InLevelOp, InHandle, InMagnitude);
	const int32 NewLevel = FoundEnhancement->GetLevel();

	OnEnhancementLevelChange(*FoundEnhancement, OldLevel, NewLevel);
}

void UDISEnhancementContainer::RemoveModifierFromEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, const FDISActiveEnhancementEffectHandle& InHandle)
{
	if (InEnhancementClass == nullptr || InHandle.IsValid() == false)
	{
		return;
	}

	ENHANCEMENT_SCOPE_LOCK();

	FDISActiveEnhancement* FoundEnhancement = FindActiveEnhancement(InEnhancementClass);
	if (FoundEnhancement == nullptr)
	{
		return;
	}

	const int32 OldLevel = FoundEnhancement->GetLevel();
	FoundEnhancement->RemoveModifier(InHandle);
	const int32 NewLevel = FoundEnhancement->GetLevel();

	OnEnhancementLevelChange(*FoundEnhancement, OldLevel, NewLevel);

	// 영향을 주는 모디파이어가 더 이상 남아있지 않으면 컨테이너에서 완전 해제
	if (FoundEnhancement->HasAnyModifiers() == false)
	{
		UnregisterEnhancement(InEnhancementClass);
	}
}

FDISActiveEnhancement* UDISEnhancementContainer::RegisterEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	if (FDISActiveEnhancement* Existing = FindActiveEnhancement(InEnhancementClass))
	{
		return Existing;
	}

	const int32 BaseMax = ResolveBaseMaxEnhancementLevel(InEnhancementClass);

	// 락 상태 보호를 위해 PendingAddEnhancements에 추가 (스코프 종료 시 ActiveEnhancements로 플러시)
	FDISActiveEnhancement& FoundEnhancement = PendingAddEnhancements.Add_GetRef(FDISActiveEnhancement(InEnhancementClass, false, BaseMax));
	const UDISEnhancement* CDO = FoundEnhancement.GetEnhancementCDO();

	// 스탯이 컨테이너에 최초 등록된 경우 등록된 모든 유효 타깃에 OnEnhancementAdded 호출 (즉시 실행)
	if (CDO != nullptr)
	{
		for (int32 Index = EffectTargets.Num() - 1; Index >= 0; --Index)
		{
			UObject* Target = EffectTargets[Index].Get();
			if (Target != nullptr)
			{
				CDO->OnEnhancementAdded(Target, FoundEnhancement);
			}
			else
			{
				EffectTargets.RemoveAtSwap(Index);
			}
		}
	}

	MarkDisplayNameDirty();

	return &FoundEnhancement;
}

void UDISEnhancementContainer::UnregisterEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return;
	}

	FDISActiveEnhancement* FoundEnhancement = FindActiveEnhancement(InEnhancementClass);
	if (FoundEnhancement == nullptr || FoundEnhancement->IsPendingRemove() == true)
	{
		return;
	}

	// 1. 삭제 대기 마킹 (중복 제거 방지)
	FoundEnhancement->MarkPendingRemove();
	PendingEnhancementRemoves++;
	MarkDisplayNameDirty();

	const UDISEnhancement* CDO = FoundEnhancement->GetEnhancementCDO();

	// 2. 스탯 완전 제거 훅 호출 (모든 유효 타깃 대상 즉시 실행)
	if (CDO != nullptr)
	{
		for (int32 Index = EffectTargets.Num() - 1; Index >= 0; --Index)
		{
			UObject* Target = EffectTargets[Index].Get();
			if (Target != nullptr)
			{
				CDO->OnEnhancementRemoved(Target, *FoundEnhancement);
			}
			else
			{
				EffectTargets.RemoveAtSwap(Index);
			}
		}
	}

	// 실제 배열 메모리 제거는 DecrementLock에서 일괄 처리
}

void UDISEnhancementContainer::OnEnhancementLevelChange(FDISActiveEnhancement& Enhancement, int32 OldLevel, int32 NewLevel)
{
	MarkDisplayNameDirty();

	const UDISEnhancement* CDO = Enhancement.GetEnhancementCDO();

	for (int32 Index = EffectTargets.Num() - 1; Index >= 0; --Index)
	{
		UObject* Target = EffectTargets[Index].Get();
		if (Target == nullptr)
		{
			EffectTargets.RemoveAtSwap(Index);
			continue;
		}

		if (CDO != nullptr)
		{
			if (OldLevel == 0 && NewLevel > 0)
			{
				// 신규 활성화 (0 -> 양수)
				CDO->OnEnhancementActivated(Target, Enhancement);
				CDO->OnEnhancementLevelChanged(Target, Enhancement, 0, NewLevel);
			}
			else if (OldLevel > 0 && NewLevel > 0)
			{
				// 활성 중 레벨 수치 변경
				if (OldLevel != NewLevel)
				{
					CDO->OnEnhancementLevelChanged(Target, Enhancement, OldLevel, NewLevel);
				}
			}
			else if (OldLevel > 0 && NewLevel <= 0)
			{
				// 완전 비활성화 (양수 -> 0)
				CDO->OnEnhancementLevelChanged(Target, Enhancement, OldLevel, 0);
				CDO->OnEnhancementDeactivated(Target, Enhancement);
			}
		}
	}

	Enhancement.bIsActive = (NewLevel > 0);
}

void UDISEnhancementContainer::OnTargetAdded(UObject* InTarget)
{
	if (InTarget == nullptr)
	{
		return;
	}

	auto NotifyEnhancement = [InTarget](FDISActiveEnhancement& Enhancement)
	{
		if (Enhancement.IsPendingRemove() == true)
		{
			return;
		}

		const UDISEnhancement* CDO = Enhancement.GetEnhancementCDO();
		if (CDO == nullptr)
		{
			return;
		}

		CDO->OnEnhancementAdded(InTarget, Enhancement);
		if (Enhancement.GetLevel() > 0)
		{
			CDO->OnEnhancementActivated(InTarget, Enhancement);
			CDO->OnEnhancementLevelChanged(InTarget, Enhancement, 0, Enhancement.GetLevel());
		}
	};

	for (FDISActiveEnhancement& ActiveEnhancement : ActiveEnhancements)
	{
		NotifyEnhancement(ActiveEnhancement);
	}
	for (FDISActiveEnhancement& PendingEnhancement : PendingAddEnhancements)
	{
		NotifyEnhancement(PendingEnhancement);
	}
}

void UDISEnhancementContainer::OnTargetRemoved(UObject* InTarget)
{
	if (InTarget == nullptr)
	{
		return;
	}

	auto NotifyEnhancement = [InTarget](const FDISActiveEnhancement& Enhancement)
	{
		if (Enhancement.IsPendingRemove() == true)
		{
			return;
		}

		const UDISEnhancement* CDO = Enhancement.GetEnhancementCDO();
		if (CDO == nullptr)
		{
			return;
		}

		if (Enhancement.GetLevel() > 0)
		{
			CDO->OnEnhancementLevelChanged(InTarget, Enhancement, Enhancement.GetLevel(), 0);
			CDO->OnEnhancementDeactivated(InTarget, Enhancement);
		}
		CDO->OnEnhancementRemoved(InTarget, Enhancement);
	};

	for (const FDISActiveEnhancement& ActiveEnhancement : ActiveEnhancements)
	{
		NotifyEnhancement(ActiveEnhancement);
	}
	for (const FDISActiveEnhancement& PendingEnhancement : PendingAddEnhancements)
	{
		NotifyEnhancement(PendingEnhancement);
	}
}

