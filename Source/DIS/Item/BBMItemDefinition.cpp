#include "Item/BBMItemDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BBMItemDefinition"

UBBMItemDefinition::UBBMItemDefinition() :
	Price(0),
	Beats(),
	LinkedAnimLayers(),
	DefaultGrantedEffects(),
	ItemGroupTags()
{
}

#if WITH_EDITOR
EDataValidationResult UBBMItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// BBM 아이템은 모두 내구도 없음(0.0)을 기본 원칙으로 하므로 양수 내구도 설정 시 경고 출력
	if (MaxDurability > 0.0f)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("BBMItemDurabilityNotSupportedWarning", "BBM 아이템 '{0}'에 MaxDurability({1})가 설정되어 있습니다. BBM 무기/장비/소모품은 내구도를 사용하지 않으므로 0.0을 권장합니다."),
			FText::FromName(GetFName()),
			FText::AsNumber(MaxDurability)
		));
	}

	// 비트 구성 TArray 크기와 점유 타일 수(Shape.Points.Num())의 일치 여부 검증
	const int32 OccupiedTileCount = Shape.Points.Num();
	if (Beats.Num() != OccupiedTileCount)
	{
		Context.AddError(FText::Format(
			LOCTEXT("BBMItemBeatCountMismatchError", "BBM 아이템 '{0}'의 비트 구성 수({1})가 점유 타일 수({2})와 일치하지 않습니다. (점유 타일 1칸당 1비트 구성 필수)"),
			FText::FromName(GetFName()),
			FText::AsNumber(Beats.Num()),
			FText::AsNumber(OccupiedTileCount)
		));
		Result = EDataValidationResult::Invalid;
	}

	// 검증 에러가 없고 기존 검증 상태가 NotValidated인 경우 Valid로 명시 승격
	if (Context.GetNumErrors() == 0 && Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}

	return Result;
}
#endif

int32 UBBMItemDefinition::GetBeatCount() const
{
	return Shape.Points.Num();
}

bool UBBMItemDefinition::HasItemGroup(const FGameplayTag& InGroupTag) const
{
	if (InGroupTag.IsValid() == false)
	{
		return false;
	}

	return ItemGroupTags.HasTagExact(InGroupTag);
}

const FGameplayTagContainer& UBBMItemDefinition::GetItemGroupTags() const
{
	return ItemGroupTags;
}

#undef LOCTEXT_NAMESPACE
