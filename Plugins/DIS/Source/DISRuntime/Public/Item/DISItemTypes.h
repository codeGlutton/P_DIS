/*****************************************************************//**
 * @file   DISItemTypes.h
 * @brief  DIS 아이템 시스템 공용 구조체 및 타입 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "DISAssetBundles.h"
#include "DISItemTypes.generated.h"

class UTexture2D;

/**
 * 2D 인벤토리 그리드 상의 90도 단위 4방향 회전 상태
 */
UENUM(BlueprintType)
enum class EDISItemRotation : uint8
{
	Rotate0   = 0 UMETA(DisplayName = "0°"),
	Rotate90  = 1 UMETA(DisplayName = "90°"),
	Rotate180 = 2 UMETA(DisplayName = "180°"),
	Rotate270 = 3 UMETA(DisplayName = "270°")
};

namespace DISItemRotation
{
	/* 회전 열거형에 해당하는 각도(도) 반환 */
	inline float ToDegrees(EDISItemRotation Rotation)
	{
		switch (Rotation)
		{
		case EDISItemRotation::Rotate90:  return 90.0f;
		case EDISItemRotation::Rotate180: return 180.0f;
		case EDISItemRotation::Rotate270: return 270.0f;
		case EDISItemRotation::Rotate0:
		default:                          return 0.0f;
		}
	}

	/* 시계 방향으로 90도 회전한 다음 회전 값 반환 */
	inline EDISItemRotation GetNextClockwise(EDISItemRotation CurrentRotation)
	{
		return static_cast<EDISItemRotation>((static_cast<uint8>(CurrentRotation) + 1) % 4);
	}
}

/**
 * 아이템 타입별 기본 동작 규칙 및 표시 정보를 정의하는 프로파일 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISItemTypeProfile
{
	GENERATED_BODY()

public:
	/* 아이템 타입 표시 이름 */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	/* 아이템 타입 대표 2D 아이콘 (UI 번들) */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> Icon;

	/* 스택킹 가능 여부 (기본값 = true) */
	UPROPERTY(Category = "Stacking", EditAnywhere, BlueprintReadWrite)
	bool bCanStack = true;

	/* 스택킹 최대 수량 (기본값 = 5) */
	UPROPERTY(Category = "Stacking", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackCount = 5;

	/* 아이템 강화 가능 여부 (기본값 = false) */
	UPROPERTY(Category = "Enhancement", EditAnywhere, BlueprintReadWrite)
	bool bCanEnhance = false;

	/* 아이템 강화 최대 수치 (기본값 = 0) */
	UPROPERTY(Category = "Enhancement", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bCanEnhance", ClampMin = "0", UIMin = "0"))
	int32 MaxEnhanceLevel = 0;
};

/**
 * 2D 인벤토리 그리드 상의 아이템 점유 형상을 정의하는 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISItemShape
{
	GENERATED_BODY()

public:
	FDISItemShape();
	FDISItemShape(const TArray<FIntPoint>& InPoints);

public:
	/* 특정 좌표의 점유 여부 확인 */
	bool IsOccupied(const FIntPoint& Coord) const;
	bool IsOccupied(int32 X, int32 Y) const;

	/* 특정 좌표 점유 설정/해제 (변경 여부 반환) */
	bool SetOccupied(int32 X, int32 Y, bool bOccupied);

	/* 상/하/좌/우 불필요한 빈 여백을 지우고 최소 좌표가 (0,0)에 오도록 좌상단 밀착 정렬 */
	void Normalize();

	/* 모든 점유 타일 비우기 */
	void Clear();

	/* 점유 타일의 바운딩 크기(Width, Height) 계산 */
	FIntPoint GetBoundingDimensions() const;

	/* 지정된 회전 각도에 맞게 2D 회전 및 정규화된 좌표 목록 반환 */
	TArray<FIntPoint> GetRotatedPoints(EDISItemRotation Rotation) const;

	/* 지정된 회전 상태에서 (0,0) 로컬 좌상단 기준 바운딩 중앙 타일 좌표 반환 (반올림 처리) */
	FIntPoint GetCenter(EDISItemRotation Rotation = EDISItemRotation::Rotate0) const;

public:
	/* 점유 중인 2D 상대 타일 좌표 목록 (기준 피벗에 상대적) */
	UPROPERTY(Category = "Shape", EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> Points;
};
