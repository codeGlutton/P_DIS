/*****************************************************************//**
 * @file   DISItemStack.h
 * @brief  DIS 아이템 스택(Item Stack) UObject 클래스 정의
 * @details 동일한 정적 정의 및 강화 상태를 공유하는 복수의 아이템 엔트리를 묶어 관리하고, 복수 슬롯에서 공유 가능한 UObject 스택 클래스
 * @date   2026-10-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item/DISItemTypes.h"
#include "DISItemStack.generated.h"

class UDISItemEntry;

/**
 * 동일한 강화 스펙을 가진 아이템 엔트리들의 묶음을 관리하고, 공간 점유 슬롯 간 공유 가능한 UObject 스택 클래스
 */
UCLASS(BlueprintType, meta = (DisplayName = "DIS Item Stack"))
class DISRUNTIME_API UDISItemStack : public UObject
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UDISItemStack();

	/* 상태 조회 */
public:
	/* 스택이 비어있는지 여부 반환 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	bool IsEmpty() const;

	/* 현재 스택에 보관된 아이템 엔트리 총 수량 반환 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	int32 GetStackCount() const;

	/* 사용/소비 우선순위가 가장 높은 최상단(Top, 최저 내구도) 아이템 엔트리 반환 (비어있으면 nullptr) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	UDISItemEntry* GetTopEntry() const;

	/* 전체 보관 중인 아이템 엔트리 목록 반환 */
	const TArray<TObjectPtr<UDISItemEntry>>& GetEntries() const;

	/* 전체 보관 중인 아이템 엔트리 목록 반환 (블루프린트 지원용) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Entries"))
	TArray<UDISItemEntry*> K2_GetEntries() const;

	/* 스태킹 조작 */
public:
	/* 새 아이템 엔트리가 이 스택에 합쳐질 수 있는지 검증 (MaxStackCount, 정의 및 강화 상태 일치 검사) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	bool CanStackWith(const UDISItemEntry* InEntry) const;

	/* 아이템 엔트리를 스택에 추가 (내구도 오름차순 정렬 삽입: Top에 최저 내구도 위치) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable)
	bool PushEntry(UDISItemEntry* InEntry);

	/* 최상단(최저 내구도) 아이템 엔트리를 꺼내고 스택에서 제거 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable)
	UDISItemEntry* PopEntry();

	/* 스택을 완전히 비우고 보관 중이던 모든 엔트리 목록 반환 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable)
	TArray<UDISItemEntry*> Clear();

	/* 회전 및 공간 점유 */
public:
	/* 현재 스택의 회전 각도 반환 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	EDISItemRotation GetRotation() const;

	/* 스택의 회전 각도 직접 설정 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable)
	void SetRotation(EDISItemRotation NewRotation);

	/* 아이템 정의에 기반하여 회전이 허용되는지 여부 반환 */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	bool CanRotate() const;

	/* 시계 방향으로 90도 회전 (성공 시 true, 회전 불가능 시 false) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable)
	bool RotateClockwise();

	/* 반시계 방향으로 90도 회전 (성공 시 true, 회전 불가능 시 false) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable)
	bool RotateCounterClockwise();

	/* 현재 회전이 적용된 상대 타일 좌표 목록 반환 (비어있으면 빈 배열 반환) */
	UFUNCTION(Category = "DIS|Item Stack", BlueprintCallable, BlueprintPure)
	TArray<FIntPoint> GetOccupiedPoints() const;

	/* 내부 프로퍼티 */
protected:
	/* 스택에 포함된 실제 아이템 엔트리 배열 (내구도 오름차순 정렬 유지) */
	UPROPERTY(Category = "Item Stack", VisibleAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UDISItemEntry>> Entries;

	/* 현재 공간 그리드 상의 회전 각도 */
	UPROPERTY(Category = "Item Stack", VisibleAnywhere, BlueprintReadOnly)
	EDISItemRotation Rotation;
};
