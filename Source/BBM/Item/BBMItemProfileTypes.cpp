#include "Item/BBMItemProfileTypes.h"

namespace BBMItemTypeTags
{
	// 무기 대분류 및 하위 6종 정의 (방패는 무기 프로파일 소속)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon, "Item.Weapon", "무기 대분류 (내구도 없음, 스태킹 불가, 매 턴 반복 사용)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Shield, "Item.Weapon.Shield", "무기 - 방패");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_OneHandSword, "Item.Weapon.OneHandSword", "무기 - 한손검");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Spear, "Item.Weapon.Spear", "무기 - 창");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Staff, "Item.Weapon.Staff", "무기 - 지팡이");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Bow, "Item.Weapon.Bow", "무기 - 활");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Dagger, "Item.Weapon.Dagger", "무기 - 단검");

	// 장비 대분류 및 하위 4종 정의
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Equipment, "Item.Equipment", "장비 대분류 (내구도 없음, 스태킹 불가, 전투당 1회 사용 후 별도 칸 보관)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Equipment_Gloves, "Item.Equipment.Gloves", "장비 - 장갑");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Equipment_Clothes, "Item.Equipment.Clothes", "장비 - 옷");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Equipment_Shoes, "Item.Equipment.Shoes", "장비 - 신발");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Equipment_Pants, "Item.Equipment.Pants", "장비 - 바지");

	// 소모품 대분류 및 하위 3종 정의
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable, "Item.Consumable", "소모품 대분류 (내구도 없음, 기본 최대 5스택, 효과 후 소멸)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Potion, "Item.Consumable.Potion", "소모품 - 물약");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Throwable, "Item.Consumable.Throwable", "소모품 - 투척물");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Scroll, "Item.Consumable.Scroll", "소모품 - 일회성 주문서");
}

FBBMItemTypeCustomData::FBBMItemTypeCustomData() :
	FDISItemTypeCustomData(BBMItemProfileDefaults::NonStackableCount),
	UsageRule(EBBMItemUsageRule::Unlimited)
{
}

FBBMItemTypeCustomData::FBBMItemTypeCustomData(int32 InMaxStackCount, EBBMItemUsageRule InUsageRule) :
	FDISItemTypeCustomData(InMaxStackCount),
	UsageRule(InUsageRule)
{
}

