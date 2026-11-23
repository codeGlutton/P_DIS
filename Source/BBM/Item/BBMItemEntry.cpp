#include "Item/BBMItemEntry.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMBeatEnhancement.h"
#include "Enhancement/DISEnhancementContainer.h"

FBBMScopedBeatSequenceBatch::FBBMScopedBeatSequenceBatch(UBBMItemEntry* InEntry) :
	TargetEntry(InEntry)
{
	if (TargetEntry.IsValid() == true)
	{
		TargetEntry->SuspendBeatSequenceRebuild();
	}
}

FBBMScopedBeatSequenceBatch::~FBBMScopedBeatSequenceBatch()
{
	if (TargetEntry.IsValid() == true)
	{
		TargetEntry->ResumeBeatSequenceRebuild();
	}
}

UBBMItemEntry::UBBMItemEntry() :
	DynamicBeatSequence(),
	bIsRebuilding(false),
	bIsBeatSequenceDirty(false),
	SuspendRebuildCounter(0)
{
}

void UBBMItemEntry::InitializeEntry_Implementation(const UDISItemDefinition* InItemDefinition, float InDurability)
{
	Super::InitializeEntry_Implementation(InItemDefinition, InDurability);

	const UBBMItemDefinition* BBMDef = Cast<UBBMItemDefinition>(InItemDefinition);
	if (BBMDef != nullptr)
	{
		DynamicBeatSequence = BBMDef->BeatSequence;
	}

	if (UDISEnhancementContainer* Container = GetEnhancementContainer())
	{
		Container->SetEffectTarget(this);
	}
}

const FBBMBeatSequence& UBBMItemEntry::GetDynamicBeatSequence() const
{
	return DynamicBeatSequence;
}

FBBMBeatSequence& UBBMItemEntry::GetMutableDynamicBeatSequence()
{
	return DynamicBeatSequence;
}

const TArray<FBBMBeatDefinition>& UBBMItemEntry::GetDynamicBeats() const
{
	return DynamicBeatSequence.Beats;
}

TArray<FBBMBeatDefinition>& UBBMItemEntry::GetMutableDynamicBeats()
{
	return DynamicBeatSequence.Beats;
}

bool UBBMItemEntry::AddActionToBeat(int32 BeatIndex, const TInstancedStruct<FBBMBeatAction>& InAction)
{
	if (DynamicBeatSequence.Beats.IsValidIndex(BeatIndex) == false || InAction.IsValid() == false)
	{
		return false;
	}

	DynamicBeatSequence.Beats[BeatIndex].Actions.Add(InAction);
	return true;
}

bool UBBMItemEntry::K2_AddActionToBeat(int32 BeatIndex, const FInstancedStruct& InAction)
{
	if (DynamicBeatSequence.Beats.IsValidIndex(BeatIndex) == false || InAction.IsValid() == false)
	{
		return false;
	}

	const UScriptStruct* ScriptStruct = InAction.GetScriptStruct();
	if (ScriptStruct == nullptr || ScriptStruct->IsChildOf(FBBMBeatAction::StaticStruct()) == false)
	{
		return false;
	}

	TInstancedStruct<FBBMBeatAction> TypedAction;
	TypedAction.InitializeAsScriptStruct(ScriptStruct, InAction.GetMemory());
	DynamicBeatSequence.Beats[BeatIndex].Actions.Add(TypedAction);
	return true;
}

const TArray<FBBMBeatEffectRange>& UBBMItemEntry::GetEffectRanges() const
{
	return DynamicBeatSequence.EffectRanges;
}

TArray<FBBMBeatEffectRange>& UBBMItemEntry::GetMutableEffectRanges()
{
	return DynamicBeatSequence.EffectRanges;
}

void UBBMItemEntry::AddEffectRange(const FBBMBeatEffectRange& InRange)
{
	DynamicBeatSequence.EffectRanges.Add(InRange);
}

bool UBBMItemEntry::RemoveEffectRange(int32 RangeIndex)
{
	if (DynamicBeatSequence.EffectRanges.IsValidIndex(RangeIndex) == false)
	{
		return false;
	}

	DynamicBeatSequence.EffectRanges.RemoveAt(RangeIndex);
	return true;
}

void UBBMItemEntry::SuspendBeatSequenceRebuild()
{
	++SuspendRebuildCounter;
}

void UBBMItemEntry::ResumeBeatSequenceRebuild()
{
	if (--SuspendRebuildCounter <= 0)
	{
		SuspendRebuildCounter = 0;
		if (bIsBeatSequenceDirty == true)
		{
			RebuildDynamicBeatSequence();
		}
	}
}

void UBBMItemEntry::RebuildDynamicBeatSequence()
{
	// 1. 배치 보류 상태인 경우: 즉시 실행하지 않고 더티 플래그만 세팅
	if (SuspendRebuildCounter > 0)
	{
		bIsBeatSequenceDirty = true;
		return;
	}

	// 2. 이미 리빌드가 진행 중인 경우(재귀 진입): 더티 플래그만 마킹하고 즉시 반환
	if (bIsRebuilding == true)
	{
		bIsBeatSequenceDirty = true;
		return;
	}

	TGuardValue<bool> RebuildGuard(bIsRebuilding, true);

	// do-while 루프로 재계산 도중 발생한 추가 더티 변경사항까지 안전하게 흡수
	do
	{
		bIsBeatSequenceDirty = false;
		ExecuteRebuildDynamicBeatSequence();
	}
	while (bIsBeatSequenceDirty == true && SuspendRebuildCounter == 0);

	// 최종 완료 시 UI 및 외부 리스너에게 단 1회 알림
	OnDynamicBeatSequenceChanged.Broadcast(this);
}

void UBBMItemEntry::ExecuteRebuildDynamicBeatSequence()
{
	const UBBMItemDefinition* BBMDef = Cast<UBBMItemDefinition>(GetItemDefinition());
	if (BBMDef == nullptr)
	{
		return;
	}

	// 0. 불변 원본 정의로부터 비트 시퀀스 초기화 (100% 무손실 롤백의 토대)
	DynamicBeatSequence = BBMDef->BeatSequence;

	UDISEnhancementContainer* Container = GetEnhancementContainer();

	// 이 아이템의 EnhancementContainer에 등록된 활성 강화 스탯 목록 평가
	// (아이템 자체 기본 강화 및 슬롯으로부터 UDISEnhancementEffect를 통해 부여된 강화 효과 일괄 집계)
	TMap<TSubclassOf<UDISEnhancement>, int32> EvaluatedEnhancements;

	if (Container != nullptr)
	{
		Container->ForEachActiveEnhancement([&EvaluatedEnhancements](const FDISActiveEnhancement& Active)
		{
			if (Active.EnhancementClass != nullptr && Active.GetLevel() > 0)
			{
				EvaluatedEnhancements.Add(Active.EnhancementClass, Active.GetLevel());
			}
		});
	}

	// 활성 강화 스탯 목록 중 유효한 최종 레벨(Level > 0)의 UBBMBeatEnhancement CDO 및 레벨 수집
	struct FEnhancementEvalItem
	{
		const UBBMBeatEnhancement* EnhanceCDO;
		int32 Level;
	};
	TArray<FEnhancementEvalItem> ActiveBeatEnhancements;

	for (const TPair<TSubclassOf<UDISEnhancement>, int32>& Pair : EvaluatedEnhancements)
	{
		if (Pair.Value <= 0 || Pair.Key == nullptr)
		{
			continue;
		}

		if (const UBBMBeatEnhancement* CDO = Cast<UBBMBeatEnhancement>(Pair.Key->GetDefaultObject()))
		{
			ActiveBeatEnhancements.Add({ CDO, Pair.Value });
		}
	}

	// ExecutionPriority 기준 안정 정렬 (낮은 우선순위가 먼저 실행됨)
	ActiveBeatEnhancements.StableSort([](const FEnhancementEvalItem& A, const FEnhancementEvalItem& B)
	{
		return A.EnhanceCDO->ExecutionPriority < B.EnhanceCDO->ExecutionPriority;
	});

	// 다형성 가상함수 순차 호출 (파생 클래스에서 각자의 변형 로직 수행)
	for (const FEnhancementEvalItem& Item : ActiveBeatEnhancements)
	{
		Item.EnhanceCDO->ApplyBeatModification(Item.Level, BBMDef->BeatSequence, DynamicBeatSequence);
	}
}

int32 UBBMItemEntry::GetSlotEnhancementBonusLevel(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	if (const UDISEnhancementContainer* Container = GetEnhancementContainer())
	{
		if (const FDISActiveEnhancement* Active = Container->FindActiveEnhancement(InEnhancementClass))
		{
			return Active->GetLevel();
		}
	}

	return 0;
}
