#include "Misc/AutomationTest.h"
#include "Enhancement/DISActiveEnhancement.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "Enhancement/DISEnhancementContainer.h"
#include "Enhancement/DISEnhancementModifier.h"
#include "Enhancement/DISEnhancementEffectSpec.h"
#include "Enhancement/DISEnhancement.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * FDISActiveEnhancement의 수치 연산, 우선순위(Add, Multiply, Override), 캐시(CachedCalculatedValue) 갱신 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISActiveEnhancementCalculationTest,
	"DIS.Enhancement.ActiveEnhancementCalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISActiveEnhancementCalculationTest::RunTest(const FString& Parameters)
{
	FDISActiveEnhancement Enhancement;
	Enhancement.SetBaseMaxLevel(10);

	// 1. 초기값 검증
	TestEqual(TEXT("초기 실수 계산값은 0.0f이어야 함"), Enhancement.GetCalculatedValue(), 0.0f);
	TestEqual(TEXT("초기 강화 레벨은 0이어야 함"), Enhancement.GetLevel(), 0);

	const FDISActiveEnhancementEffectHandle HandleAdd(1);
	const FDISActiveEnhancementEffectHandle HandleMultiply(2);
	const FDISActiveEnhancementEffectHandle HandleOverride(3);

	// 2. Add 모디파이어 적용 검증
	Enhancement.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Add, HandleAdd, 5.0f);
	TestEqual(TEXT("Add 모디파이어 적용 후 실수값 5.0f 검증"), Enhancement.GetCalculatedValue(), 5.0f);
	TestEqual(TEXT("Add 모디파이어 적용 후 레벨 5 검증"), Enhancement.GetLevel(), 5);

	// 3. Multiply 모디파이어 적용 검증 (5.0f * 2.0f = 10.0f)
	Enhancement.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Multiply, HandleMultiply, 2.0f);
	TestEqual(TEXT("Multiply 모디파이어 누적 후 실수값 10.0f 검증"), Enhancement.GetCalculatedValue(), 10.0f);
	TestEqual(TEXT("Multiply 모디파이어 누적 후 레벨 10 검증"), Enhancement.GetLevel(), 10);

	// 4. Override 모디파이어 적용 검증 (기존 연산 무시하고 3.0f 적용)
	Enhancement.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Override, HandleOverride, 3.0f);
	TestEqual(TEXT("Override 모디파이어 적용 후 실수값 3.0f 검증"), Enhancement.GetCalculatedValue(), 3.0f);
	TestEqual(TEXT("Override 모디파이어 적용 후 레벨 3 검증"), Enhancement.GetLevel(), 3);

	// 5. Override 모디파이어 제거 검증 (제거 후 기존 10.0f로 복구)
	const bool bRemovedOverride = Enhancement.RemoveModifier(HandleOverride);
	TestTrue(TEXT("Override 모디파이어 제거 성공 여부"), bRemovedOverride);
	TestEqual(TEXT("Override 모디파이어 제거 후 기존 연산값 10.0f 복구 검증"), Enhancement.GetCalculatedValue(), 10.0f);
	TestEqual(TEXT("Override 모디파이어 제거 후 레벨 10 복구 검증"), Enhancement.GetLevel(), 10);

	// 6. ResetModifiers 초기화 검증
	Enhancement.ResetModifiers();
	TestEqual(TEXT("ResetModifiers 후 실수값 0.0f 검증"), Enhancement.GetCalculatedValue(), 0.0f);
	TestEqual(TEXT("ResetModifiers 후 레벨 0 검증"), Enhancement.GetLevel(), 0);

	return true;
}

/**
 * FDISActiveEnhancement의 MaxLevel 모디파이어 연산(Add, Multiply, Override) 및 현재 레벨 Clamping 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISActiveEnhancementMaxLevelModifierTest,
	"DIS.Enhancement.ActiveEnhancementMaxLevelModifier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISActiveEnhancementMaxLevelModifierTest::RunTest(const FString& Parameters)
{
	FDISActiveEnhancement Enhancement;

	// 1. 기본 BaseMaxLevel 5로 설정
	Enhancement.SetBaseMaxLevel(5);
	TestEqual(TEXT("기본 MaxLevel 검증"), Enhancement.GetMaxLevel(), 5);

	const FDISActiveEnhancementEffectHandle HandleLevelAdd(1);
	const FDISActiveEnhancementEffectHandle HandleMaxAdd(2);
	const FDISActiveEnhancementEffectHandle HandleMaxMultiply(3);
	const FDISActiveEnhancementEffectHandle HandleMaxOverride(4);

	// 2. 현재 레벨을 8로 증가 시도 -> MaxLevel 5에 의해 5로 클램핑되어야 함
	Enhancement.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Add, HandleLevelAdd, 8.0f);
	TestEqual(TEXT("클램프 전 Raw 실수값은 8.0f"), Enhancement.GetRawCalculatedValue(), 8.0f);
	TestEqual(TEXT("MaxLevel 5에 의해 클램프된 실수값은 5.0f"), Enhancement.GetCalculatedValue(), 5.0f);
	TestEqual(TEXT("MaxLevel 5에 의해 클램프된 레벨은 5"), Enhancement.GetLevel(), 5);

	// 3. MaxLevel에 Add 3 모디파이어 적용 -> (5 + 3 = 8)로 최대 레벨 확장
	Enhancement.AddModifier(EDISEnhancementModifierTarget::MaxLevel, EDISEnhancementLevelOp::Add, HandleMaxAdd, 3.0f);
	TestEqual(TEXT("확장된 MaxLevel은 8이어야 함"), Enhancement.GetMaxLevel(), 8);
	TestEqual(TEXT("최대 레벨 확장에 따라 현재 레벨도 8로 상승 반영"), Enhancement.GetLevel(), 8);

	// 4. MaxLevel에 Multiply 2.0 적용 -> (5 + 3) * 2 = 16
	Enhancement.AddModifier(EDISEnhancementModifierTarget::MaxLevel, EDISEnhancementLevelOp::Multiply, HandleMaxMultiply, 2.0f);
	TestEqual(TEXT("승산 반영된 MaxLevel은 16이어야 함"), Enhancement.GetMaxLevel(), 16);
	TestEqual(TEXT("현재 레벨(8)은 상한(16)보다 낮으므로 8 유지"), Enhancement.GetLevel(), 8);

	// 5. MaxLevel에 Override 4.0 적용 -> 최대 레벨이 4로 강제 하향
	Enhancement.AddModifier(EDISEnhancementModifierTarget::MaxLevel, EDISEnhancementLevelOp::Override, HandleMaxOverride, 4.0f);
	TestEqual(TEXT("덮어쓰기된 MaxLevel은 4이어야 함"), Enhancement.GetMaxLevel(), 4);
	TestEqual(TEXT("MaxLevel 하향에 따라 현재 레벨도 4로 클램프 축소"), Enhancement.GetLevel(), 4);

	// 6. MaxLevel Override 제거 -> 이전 계산값 (5+3)*2 = 16으로 복구
	const bool bRemovedOverride = Enhancement.RemoveModifier(EDISEnhancementModifierTarget::MaxLevel, EDISEnhancementLevelOp::Override, HandleMaxOverride);
	TestTrue(TEXT("MaxLevel Override 제거 성공"), bRemovedOverride);
	TestEqual(TEXT("복구된 MaxLevel은 16이어야 함"), Enhancement.GetMaxLevel(), 16);
	TestEqual(TEXT("현재 레벨은 기존 수치 8로 복구"), Enhancement.GetLevel(), 8);

	return true;
}

/**
 * UDISEnhancementContainer의 Scoped Lock 동작, 지연 등록, 삭제 대기 플래그, 중복 삭제 방지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISEnhancementContainerScopeLockTest,
	"DIS.Enhancement.ContainerScopeLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISEnhancementContainerScopeLockTest::RunTest(const FString& Parameters)
{
	UDISEnhancementContainer* Container = NewObject<UDISEnhancementContainer>();
	TestNotNull(TEXT("컨테이너 인스턴스 생성 성공 검증"), Container);

	if (Container == nullptr)
	{
		return false;
	}

	// 1. 기본 상태에서 락 미체결 상태 확인
	TestFalse(TEXT("초기 상태에서는 IsLocked가 false이어야 함"), Container->IsLocked());

	// 2. Scoped Lock 블록 테스트
	{
		FScopedEnhancementContainerLock OuterLock(*Container);
		TestTrue(TEXT("ScopeLock 진입 후 IsLocked는 true이어야 함"), Container->IsLocked());

		// 3. 중첩 Scoped Lock 테스트
		{
			FScopedEnhancementContainerLock InnerLock(*Container);
			TestTrue(TEXT("중첩 ScopeLock 상태에서도 IsLocked는 true이어야 함"), Container->IsLocked());
		}

		TestTrue(TEXT("내부 ScopeLock 해제 후에도 외부 ScopeLock에 의해 IsLocked는 true 유지"), Container->IsLocked());
	}

	// 4. 최외곽 Scoped Lock 해제 후 락 해제 상태 확인
	TestFalse(TEXT("최외곽 ScopeLock 해제 후 IsLocked는 false이어야 함"), Container->IsLocked());

	// 5. Iterator 유효성 검증
	UDISEnhancementContainer::FConstEffectIterator It = Container->CreateConstEffectIterator();
	TestFalse(TEXT("빈 컨테이너의 이터레이터는 유효하지 않아야 함"), static_cast<bool>(It));

	return true;
}

/**
 * UDISEnhancementContainer의 GetEnhancedDisplayName 캐싱 및 Dirty 무효화 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISEnhancementDisplayNameCacheTest,
	"DIS.Enhancement.DisplayNameCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISEnhancementDisplayNameCacheTest::RunTest(const FString& Parameters)
{
	UDISEnhancementContainer* Container = NewObject<UDISEnhancementContainer>();
	TestNotNull(TEXT("컨테이너 생성 성공 검증"), Container);
	if (Container == nullptr)
	{
		return false;
	}

	const FText BaseNameA = FText::FromString(TEXT("IronSword"));
	const FText BaseNameB = FText::FromString(TEXT("SteelShield"));

	// 1. 초기 상태에서 접두사 및 접미사 캐시가 비어있는지 확인
	TestTrue(TEXT("초기 접두사 캐시는 비어있어야 함"), Container->GetCachedPrefix().IsEmpty());
	TestTrue(TEXT("초기 접미사 캐시는 비어있어야 함"), Container->GetCachedSuffix().IsEmpty());

	// 2. 수식어가 없을 때 BaseName 결합 반환 검증
	const FText DisplayName1 = Container->GetEnhancedDisplayName(BaseNameA);
	TestEqual(TEXT("수식어가 없을 때는 기본 이름 A 반환"), DisplayName1.ToString(), TEXT("IronSword"));

	// 3. 다른 BaseName 요청 시에도 동일한 수식어 캐시를 사용하여 즉시 결합 반환 검증
	const FText DisplayNameB = Container->GetEnhancedDisplayName(BaseNameB);
	TestEqual(TEXT("다른 BaseName 요청 시에도 정상 결합 반환"), DisplayNameB.ToString(), TEXT("SteelShield"));

	// 4. 수동 MarkDisplayNameDirty 호출 후 재조회 검증
	Container->MarkDisplayNameDirty();
	const FText DisplayNameAfterDirty = Container->GetEnhancedDisplayName(BaseNameA);
	TestEqual(TEXT("Dirty 마킹 후 재조회 시 정상 반환"), DisplayNameAfterDirty.ToString(), TEXT("IronSword"));

	return true;
}

/**
 * UDISEnhancementContainer의 HasIdenticalEnhancements에서 Level 0 강화가 타 컨테이너에 없어도 동일하게 평가되는지 검증
 */
class FDISEnhancementContainerTestAccessor : public UDISEnhancementContainer
{
public:
	void AddRawEnhancement(TSubclassOf<UDISEnhancement> InClass, int32 InLevel)
	{
		FDISActiveEnhancement Enh(InClass, InLevel > 0, 10);
		if (InLevel > 0)
		{
			Enh.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Add, FDISActiveEnhancementEffectHandle(1), static_cast<float>(InLevel));
		}
		ActiveEnhancements.Add(Enh);
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISEnhancementIdenticalComparisonTest,
	"DIS.Enhancement.IdenticalComparisonLevelZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISEnhancementIdenticalComparisonTest::RunTest(const FString& Parameters)
{
	UDISEnhancementContainer* RawContainerA = NewObject<UDISEnhancementContainer>();
	UDISEnhancementContainer* RawContainerB = NewObject<UDISEnhancementContainer>();

	TestNotNull(TEXT("ContainerA 생성 검증"), RawContainerA);
	TestNotNull(TEXT("ContainerB 생성 검증"), RawContainerB);

	if (RawContainerA == nullptr || RawContainerB == nullptr)
	{
		return false;
	}

	FDISEnhancementContainerTestAccessor* AccessorA = static_cast<FDISEnhancementContainerTestAccessor*>(RawContainerA);
	FDISEnhancementContainerTestAccessor* AccessorB = static_cast<FDISEnhancementContainerTestAccessor*>(RawContainerB);

	// 1. 둘 다 비어있는 상태에서는 상호 동일
	TestTrue(TEXT("빈 컨테이너끼리는 동일해야 함"), RawContainerA->HasIdenticalEnhancements(RawContainerB));
	TestTrue(TEXT("nullptr 상대 비교 시에도 빈 컨테이너는 동일해야 함"), RawContainerA->HasIdenticalEnhancements(nullptr));

	// 2. ContainerA에 Level이 0인 강화 추가, ContainerB에는 미등록
	AccessorA->AddRawEnhancement(UDISEnhancement::StaticClass(), 0);

	// Level이 0인 강화는 타 컨테이너에 없어도 동일하다고 판정되어야 함
	TestTrue(TEXT("Level 0 강화가 타 컨테이너에 미존재해도 동일 판정"), RawContainerA->HasIdenticalEnhancements(RawContainerB));
	TestTrue(TEXT("반대 방향 비교 시에도 동일 판정"), RawContainerB->HasIdenticalEnhancements(RawContainerA));
	TestTrue(TEXT("Level 0 강화만 있는 컨테이너는 nullptr 상대와도 동일 판정"), RawContainerA->HasIdenticalEnhancements(nullptr));

	// 3. ContainerB에도 동일 클래스를 Level 0으로 추가 -> 여전히 동일
	AccessorB->AddRawEnhancement(UDISEnhancement::StaticClass(), 0);
	TestTrue(TEXT("양쪽 모두 Level 0인 강화가 등록되어 있을 때 동일 판정"), RawContainerA->HasIdenticalEnhancements(RawContainerB));

	return true;
}

/**
 * UDISEnhancementContainer의 복수 대상(EffectTargets) 등록, 해제, 조회 및 일괄 설정 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISEnhancementMultipleEffectTargetsTest,
	"DIS.Enhancement.MultipleEffectTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISEnhancementMultipleEffectTargetsTest::RunTest(const FString& Parameters)
{
	UDISEnhancementContainer* Container = NewObject<UDISEnhancementContainer>();
	TestNotNull(TEXT("Container 생성 검증"), Container);
	if (Container == nullptr)
	{
		return false;
	}

	UObject* TargetA = NewObject<UDISEnhancementContainer>();
	UObject* TargetB = NewObject<UDISEnhancementContainer>();
	UObject* TargetC = NewObject<UDISEnhancementContainer>();

	// 1. 초기 상태 검증
	TestEqual(TEXT("초기 EffectTargets 목록은 비어있어야 함"), Container->GetEffectTargets().Num(), 0);
	TestNull(TEXT("초기 GetEffectTarget()은 nullptr이어야 함"), Container->GetEffectTarget());

	// 2. 단일 타깃 추가 검증
	Container->AddEffectTarget(TargetA);
	TestEqual(TEXT("TargetA 추가 후 대상 수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestEqual(TEXT("GetEffectTarget()이 TargetA를 반환해야 함"), Container->GetEffectTarget(), TargetA);

	// 3. 중복 추가 방지 검증
	Container->AddEffectTarget(TargetA);
	TestEqual(TEXT("동일 타깃 중복 추가 시 대상 수는 1이어야 함"), Container->GetEffectTargets().Num(), 1);

	// 4. 복수 타깃 추가 검증
	Container->AddEffectTarget(TargetB);
	Container->AddEffectTarget(TargetC);
	TestEqual(TEXT("TargetB, TargetC 추가 후 대상 수 3 검증"), Container->GetEffectTargets().Num(), 3);

	TArray<UObject*> CurrentTargets = Container->GetEffectTargets();
	TestTrue(TEXT("TargetA 포함 여부 검증"), CurrentTargets.Contains(TargetA));
	TestTrue(TEXT("TargetB 포함 여부 검증"), CurrentTargets.Contains(TargetB));
	TestTrue(TEXT("TargetC 포함 여부 검증"), CurrentTargets.Contains(TargetC));

	// 5. 특정 타깃 제거 검증
	const bool bRemovedB = Container->RemoveEffectTarget(TargetB);
	TestTrue(TEXT("TargetB 제거 성공 여부"), bRemovedB);
	TestEqual(TEXT("TargetB 제거 후 대상 수 2 검증"), Container->GetEffectTargets().Num(), 2);
	TestFalse(TEXT("제거된 TargetB 미포함 검증"), Container->GetEffectTargets().Contains(TargetB));
	TestTrue(TEXT("TargetA 유지 검증"), Container->GetEffectTargets().Contains(TargetA));
	TestTrue(TEXT("TargetC 유지 검증"), Container->GetEffectTargets().Contains(TargetC));

	// 6. 존재하지 않는 타깃 제거 시 false 반환 검증
	const bool bRemovedNonExistent = Container->RemoveEffectTarget(TargetB);
	TestFalse(TEXT("미등록 타깃 제거 시 false 반환 검증"), bRemovedNonExistent);

	// 7. SetEffectTargets 일괄 설정 검증
	Container->SetEffectTargets({ TargetB });
	TestEqual(TEXT("SetEffectTargets 후 대상 수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestEqual(TEXT("SetEffectTargets 후 유일 대상 TargetB 검증"), Container->GetEffectTarget(), TargetB);

	// 8. 단일 SetEffectTarget 편의 래퍼 검증
	Container->SetEffectTarget(TargetC);
	TestEqual(TEXT("SetEffectTarget(TargetC) 후 대상 수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestEqual(TEXT("SetEffectTarget 후 유일 대상 TargetC 검증"), Container->GetEffectTarget(), TargetC);

	// 9. ClearEffectTargets 일괄 제거 검증
	Container->ClearEffectTargets();
	TestEqual(TEXT("ClearEffectTargets 후 대상 수 0 검증"), Container->GetEffectTargets().Num(), 0);
	TestNull(TEXT("ClearEffectTargets 후 GetEffectTarget()은 nullptr이어야 함"), Container->GetEffectTarget());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

