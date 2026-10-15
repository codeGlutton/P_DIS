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

	/* 반시계 방향으로 90도 회전한 다음 회전 값 반환 */
	inline EDISItemRotation GetNextCounterClockwise(EDISItemRotation CurrentRotation)
	{
		return static_cast<EDISItemRotation>((static_cast<uint8>(CurrentRotation) + 3) % 4);
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
	/* 스택킹 가능 여부 헬퍼 (MaxStackCount가 2 이상인지 여부) */
	bool CanStack() const { return MaxStackCount > 1; }

public:
	/* 아이템 타입 표시 이름 */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	/* 아이템 타입 대표 2D 아이콘 (UI 번들) */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> Icon;

	/* 스택킹 최대 수량 (1 이하일 경우 스택 불가, 2 이상일 경우 스택 가능, 기본값 = 1) */
	UPROPERTY(Category = "Stacking", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackCount = 1;
};

/**
 * 아이템 희귀도별 표시 정보 및 서열(Rank)을 정의하는 프로파일 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISRarityProfile
{
	GENERATED_BODY()

public:
	/* 희귀도 표시 이름 */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	/* 희귀도 대표 색상 (UI 텍스트, 슬롯 테두리, 배경 연출 등에 활용) */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor::White;

	/* 희귀도 엠블럼 또는 뱃지 2D 아이콘 (UI 번들) */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> Icon;

	/* 희귀도 서열/우선순위 (숫자가 클수록 더 높은 희귀도) */
	UPROPERTY(Category = "Order", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	int32 Rank = 0;
};

/**
 * 2D 인벤토리 및 아이템에서 사용하는 공용 2D 타일 형상 정의 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISTileShape
{
	GENERATED_BODY()

public:
	FDISTileShape();
	FDISTileShape(const TArray<FIntPoint>& InPoints);

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

#if WITH_EDITORONLY_DATA
	/* 에디터 타일맵 편집 시 전역 세팅 대신 고정으로 적용할 그리드 가로/세로 규격 (0 이하 시 기본 전역 세팅 적용) */
	UPROPERTY(Category = "Shape|Editor", EditDefaultsOnly, meta = (ClampMin = "0", UIMin = "0"))
	FIntPoint EditorFixedGridDimensions = FIntPoint::ZeroValue;
#endif
};
