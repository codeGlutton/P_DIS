/*****************************************************************//**
 * @file   DISActiveEnhancementEffectIterator.h
 * @brief  UDISEnhancementContainer 전용 활성 강화 효과 2-Stage 순회 반복자 정의
 * @details 기존 배열과 지연 등록 대기열을 연속 순회하며 삭제 대기 객체를 자동 스킵하고 스코프 락을 유지함
 * @date   2026-10-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"

/**
 * UDISEnhancementContainer에 등록된 활성 효과를 안전하게 순회하는 양방향 2-Stage Iterator
 * (기존 ActiveEffects와 락 중 추가된 PendingAddEffects를 연속 순회하며 bPendingRemove 원소 자동 스킵)
 */
template<typename ElementType, typename ContainerType>
class TDISActiveEnhancementEffectIterator
{
public:
	TDISActiveEnhancementEffectIterator(const ContainerType& InContainer, int32 StartIdx = 0) :
		Index(StartIdx),
		bIteratingPending(false),
		Current(nullptr),
		Container(const_cast<ContainerType&>(InContainer))
	{
		Container.IncrementLock();
		UpdateCurrent();
	}

	~TDISActiveEnhancementEffectIterator()
	{
		Container.DecrementLock();
	}

	void operator++()
	{
		Advance();
	}

	ElementType& operator*() const
	{
		check(Current != nullptr);
		return *Current;
	}

	ElementType* operator->() const
	{
		check(Current != nullptr);
		return Current;
	}

	explicit operator bool() const
	{
		return Current != nullptr;
	}

	ElementType* Get() const
	{
		return Current;
	}

	friend bool operator==(const TDISActiveEnhancementEffectIterator& Lhs, const TDISActiveEnhancementEffectIterator& Rhs)
	{
		return Lhs.Current == Rhs.Current;
	}

	friend bool operator!=(const TDISActiveEnhancementEffectIterator& Lhs, const TDISActiveEnhancementEffectIterator& Rhs)
	{
		return Lhs.Current != Rhs.Current;
	}

private:
	void Advance()
	{
		Index++;
		UpdateCurrent();
	}

	void UpdateCurrent()
	{
		Current = nullptr;

		// 1단계: 메인 ActiveEffects 배열 순회
		if (bIteratingPending == false)
		{
			while (Index < Container.ActiveEffects.Num())
			{
				ElementType* Candidate = &Container.ActiveEffects[Index];
				if (Candidate->IsPendingRemove() == false)
				{
					Current = Candidate;
					return;
				}
				Index++;
			}

			// ActiveEffects 순회 완료 후 PendingAddEffects 순회로 전환
			bIteratingPending = true;
			Index = 0;
		}

		// 2단계: PendingAddEffects 대기열 순회
		if (bIteratingPending == true)
		{
			while (Index < Container.PendingAddEffects.Num())
			{
				ElementType* Candidate = &Container.PendingAddEffects[Index];
				if (Candidate->IsPendingRemove() == false)
				{
					Current = Candidate;
					return;
				}
				Index++;
			}
		}
	}

private:
	int32 Index;
	bool bIteratingPending;
	ElementType* Current;
	ContainerType& Container;
};
