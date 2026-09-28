#include "Item/DISItemDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "Global/DISGlobalSettings.h"
#endif

#define LOCTEXT_NAMESPACE "DISItemDefinition"

UDISItemDefinition::UDISItemDefinition() :
	InitialRotation(EDISItemRotation::Rotate0),
	bCanRotate(true),
	Weight(1.0f),
	MaxDurability(100.0f)
{
}

FPrimaryAssetId UDISItemDefinition::GetPrimaryAssetId() const
{
	// 에셋 매니저에서 식별 가능한 PrimaryAssetType 반환
	return FPrimaryAssetId(DISPrimaryAssetTypes::Item, GetFName());
}

#if WITH_EDITOR
EDataValidationResult UDISItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 점유 타일의 바운딩 크기 검증
	const FIntPoint BoundingDimensions = Shape.GetBoundingDimensions();

	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	const FIntPoint MaxAllowedDimensions = (Settings != nullptr && Settings->MaxItemTileDimensions.X > 0 && Settings->MaxItemTileDimensions.Y > 0)
		? Settings->MaxItemTileDimensions
		: FIntPoint(8, 8);

	if (BoundingDimensions.X > MaxAllowedDimensions.X || BoundingDimensions.Y > MaxAllowedDimensions.Y)
	{
		Context.AddError(FText::Format(
			LOCTEXT("ItemDimensionsExceededError", "아이템 '{0}'의 점유 크기({1}x{2})가 UDISGlobalSettings에 정의된 최대 허용 타일 규격({3}x{4})을 초과했습니다."),
			FText::FromName(GetFName()),
			BoundingDimensions.X,
			BoundingDimensions.Y,
			MaxAllowedDimensions.X,
			MaxAllowedDimensions.Y
		));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
