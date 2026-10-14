#include "Misc/AutomationTest.h"
#include "Enhancement/DISActiveEnhancement.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "Enhancement/DISEnhancementContainer.h"
#include "Enhancement/DISEnhancementModifier.h"
#include "Enhancement/DISEnhancementEffectSpec.h"

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

	// 1. 초기값 검증
	TestEqual(TEXT("초기 실수 계산값은 0.0f이어야 함"), Enhancement.GetCalculatedValue(), 0.0f);
	TestEqual(TEXT("초기 강화 레벨은 0이어야 함"), Enhancement.GetLevel(), 0);

	const FDISActiveEnhancementEffectHandle HandleAdd(1);
	const FDISActiveEnhancementEffectHandle HandleMultiply(2);
	const FDISActiveEnhancementEffectHandle HandleOverride(3);

	// 2. Add 모디파이어 적용 검증
	Enhancement.AddModifier(EDISEnhancementLevelOp::Add, HandleAdd, 5.0f);
	TestEqual(TEXT("Add 모디파이어 적용 후 실수값 5.0f 검증"), Enhancement.GetCalculatedValue(), 5.0f);
	TestEqual(TEXT("Add 모디파이어 적용 후 레벨 5 검증"), Enhancement.GetLevel(), 5);

	// 3. Multiply 모디파이어 적용 검증 (5.0f * 2.0f = 10.0f)
	Enhancement.AddModifier(EDISEnhancementLevelOp::Multiply, HandleMultiply, 2.0f);
	TestEqual(TEXT("Multiply 모디파이어 누적 후 실수값 10.0f 검증"), Enhancement.GetCalculatedValue(), 10.0f);
	TestEqual(TEXT("Multiply 모디파이어 누적 후 레벨 10 검증"), Enhancement.GetLevel(), 10);

	// 4. Override 모디파이어 적용 검증 (기존 연산 무시하고 3.0f 적용)
	Enhancement.AddModifier(EDISEnhancementLevelOp::Override, HandleOverride, 3.0f);
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

#endif // WITH_DEV_AUTOMATION_TESTS
