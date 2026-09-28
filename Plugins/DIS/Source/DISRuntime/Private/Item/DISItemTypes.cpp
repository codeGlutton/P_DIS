#include "Item/DISItemTypes.h"

FDISItemShape::FDISItemShape()
{
	// 기본값 1x1 점유
	Points.Add(FIntPoint(0, 0));
}

FDISItemShape::FDISItemShape(const TArray<FIntPoint>& InPoints) :
	Points(InPoints)
{
}

bool FDISItemShape::IsOccupied(const FIntPoint& Coord) const
{
	return Points.Contains(Coord);
}

bool FDISItemShape::IsOccupied(int32 X, int32 Y) const
{
	return Points.Contains(FIntPoint(X, Y));
}

bool FDISItemShape::SetOccupied(int32 X, int32 Y, bool bOccupied)
{
	const FIntPoint Coord(X, Y);
	if (bOccupied == true)
	{
		if (Points.Contains(Coord) == false)
		{
			Points.Add(Coord);
			return true;
		}
	}
	else
	{
		return Points.Remove(Coord) > 0;
	}
	return false;
}

void FDISItemShape::Normalize()
{
	if (Points.IsEmpty() == true)
	{
		return;
	}

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;

	for (const FIntPoint& Point : Points)
	{
		MinX = FMath::Min(MinX, Point.X);
		MinY = FMath::Min(MinY, Point.Y);
	}

	if (MinX != 0 || MinY != 0)
	{
		for (FIntPoint& Point : Points)
		{
			Point.X -= MinX;
			Point.Y -= MinY;
		}
	}
}

void FDISItemShape::Clear()
{
	Points.Empty();
}

FIntPoint FDISItemShape::GetBoundingDimensions() const
{
	if (Points.IsEmpty() == true)
	{
		return FIntPoint(0, 0);
	}

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;
	int32 MaxX = MIN_int32;
	int32 MaxY = MIN_int32;

	for (const FIntPoint& Point : Points)
	{
		MinX = FMath::Min(MinX, Point.X);
		MinY = FMath::Min(MinY, Point.Y);
		MaxX = FMath::Max(MaxX, Point.X);
		MaxY = FMath::Max(MaxY, Point.Y);
	}

	return FIntPoint(MaxX - MinX + 1, MaxY - MinY + 1);
}

TArray<FIntPoint> FDISItemShape::GetRotatedPoints(EDISItemRotation Rotation) const
{
	if (Points.IsEmpty() == true)
	{
		return TArray<FIntPoint>{ FIntPoint(0, 0) };
	}

	if (Rotation == EDISItemRotation::Rotate0)
	{
		return Points;
	}

	TArray<FIntPoint> RotatedPoints;
	RotatedPoints.Reserve(Points.Num());

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;

	// 1. 회전 변환 적용 (2D 화면 좌표계: 오른쪽 +X, 아래 +Y 기준 시계방향 90도 회전)
	for (const FIntPoint& Point : Points)
	{
		FIntPoint Transformed;
		switch (Rotation)
		{
		case EDISItemRotation::Rotate90:
			Transformed = FIntPoint(-Point.Y, Point.X);
			break;
		case EDISItemRotation::Rotate180:
			Transformed = FIntPoint(-Point.X, -Point.Y);
			break;
		case EDISItemRotation::Rotate270:
			Transformed = FIntPoint(Point.Y, -Point.X);
			break;
		case EDISItemRotation::Rotate0:
		default:
			Transformed = Point;
			break;
		}

		MinX = FMath::Min(MinX, Transformed.X);
		MinY = FMath::Min(MinY, Transformed.Y);
		RotatedPoints.Add(Transformed);
	}

	// 2. 좌상단 기준점(0, 0)이 되도록 정규화(Normalization)
	for (FIntPoint& Point : RotatedPoints)
	{
		Point.X -= MinX;
		Point.Y -= MinY;
	}

	return RotatedPoints;
}

FIntPoint FDISItemShape::GetCenter(EDISItemRotation Rotation) const
{
	const TArray<FIntPoint> RotatedPoints = GetRotatedPoints(Rotation);
	if (RotatedPoints.IsEmpty() == true)
	{
		return FIntPoint(0, 0);
	}

	int32 MaxX = 0;
	int32 MaxY = 0;
	for (const FIntPoint& Point : RotatedPoints)
	{
		MaxX = FMath::Max(MaxX, Point.X);
		MaxY = FMath::Max(MaxY, Point.Y);
	}

	// (0,0) 로컬 기준 바운딩 중앙 타일 좌표 계산 (반올림 처리)
	const int32 CenterX = FMath::RoundToInt32(MaxX * 0.5f);
	const int32 CenterY = FMath::RoundToInt32(MaxY * 0.5f);

	return FIntPoint(CenterX, CenterY);
}
