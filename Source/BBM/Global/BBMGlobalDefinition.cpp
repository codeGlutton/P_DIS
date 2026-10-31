#include "Global/BBMGlobalDefinition.h"
#include "Item/BBMItemProfileTypes.h"

#define LOCTEXT_NAMESPACE "BBMGlobalDefinition"

namespace
{
	FDISItemTypeProfile MakeCategoryProfile(const FText& InDisplayName, int32 InMaxStackCount, EBBMItemUsageRule InUsageRule)
	{
		// 상위 대분류는 자체 CustomData를 보유하도록 Override 활성화
		FDISItemTypeProfile Profile;
		Profile.DisplayName = InDisplayName;
		Profile.bOverrideCustomData = true;
		Profile.CustomData = TInstancedStruct<FDISItemTypeCustomData>::Make<FBBMItemTypeCustomData>(InMaxStackCount, InUsageRule);
		return Profile;
	}

	FDISItemTypeProfile MakeSubTypeProfile(const FText& InDisplayName)
	{
		// 하위 세부 타입은 고유 표시 이름만 설정하고 CustomData는 부모 대분류를 상속받음 (bOverrideCustomData = false)
		FDISItemTypeProfile Profile;
		Profile.DisplayName = InDisplayName;
		Profile.bOverrideCustomData = false;
		return Profile;
	}
}

UBBMGlobalDefinition::UBBMGlobalDefinition()
{
	// 1. 무기 프로파일 (방패 포함 6종: 내구도 없음, 스태킹 불가, 매 턴 반복 사용)
	ItemTypeProfiles.Add(
		BBMItemTypeTags::Item_Weapon.GetTag(),
		MakeCategoryProfile(LOCTEXT("Type_Weapon", "무기"), BBMItemProfileDefaults::NonStackableCount, EBBMItemUsageRule::Unlimited)
	);
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Weapon_Shield.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Shield", "방패")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Weapon_OneHandSword.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_OneHandSword", "한손검")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Weapon_Spear.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Spear", "창")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Weapon_Staff.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Staff", "지팡이")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Weapon_Bow.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Bow", "활")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Weapon_Dagger.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Dagger", "단검")));

	// 2. 장비 프로파일 (4종: 내구도 없음, 스태킹 불가, 전투당 1회 사용 후 별도 칸 보관)
	ItemTypeProfiles.Add(
		BBMItemTypeTags::Item_Equipment.GetTag(),
		MakeCategoryProfile(LOCTEXT("Type_Equipment", "장비"), BBMItemProfileDefaults::NonStackableCount, EBBMItemUsageRule::OncePerBattle)
	);
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Equipment_Gloves.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Gloves", "장갑")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Equipment_Clothes.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Clothes", "옷")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Equipment_Shoes.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Shoes", "신발")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Equipment_Pants.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Pants", "바지")));

	// 3. 소모품 프로파일 (3종: 내구도 없음, 기본 MaxStackCount = 5, 효과 후 소멸)
	ItemTypeProfiles.Add(
		BBMItemTypeTags::Item_Consumable.GetTag(),
		MakeCategoryProfile(LOCTEXT("Type_Consumable", "소모품"), BBMItemProfileDefaults::ConsumableMaxStackCount, EBBMItemUsageRule::ConsumeOnUse)
	);
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Consumable_Potion.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Potion", "물약")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Consumable_Throwable.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Throwable", "투척물")));
	ItemTypeProfiles.Add(BBMItemTypeTags::Item_Consumable_Scroll.GetTag(), MakeSubTypeProfile(LOCTEXT("Type_Scroll", "일회성 주문서")));
}

#undef LOCTEXT_NAMESPACE
