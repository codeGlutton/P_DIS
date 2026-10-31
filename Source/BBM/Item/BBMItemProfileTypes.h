/*****************************************************************//**
 * @file   BBMItemProfileTypes.h
 * @brief  BBM(비트 배낭 상인) 아이템 타입 네이티브 태그 및 프로파일 커스텀 데이터 정의
 * @date   2026-10-29
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Item/DISItemTypes.h"
#include "BBMItemProfileTypes.generated.h"

namespace BBMItemProfileDefaults
{
	// 무기 및 장비의 기본 최대 스택 수량 (스태킹 불가)
	constexpr int32 NonStackableCount = 1;

	// 소모품의 기본 최대 스택 수량
	constexpr int32 ConsumableMaxStackCount = 5;
}

namespace BBMItemTypeTags
{
	// 무기 대분류 및 세부 타입 (내구도 없음, 스태킹 불가, 매 턴 반복 사용 가능)
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Shield);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_OneHandSword);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Spear);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Staff);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Bow);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Dagger);

	// 장비 대분류 및 세부 타입 (내구도 없음, 스태킹 불가, 전투당 1회 사용 후 별도 칸 보관)
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Gloves);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Clothes);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Shoes);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Pants);

	// 소모품 대분류 및 세부 타입 (내구도 없음, 스태킹 가능, 효과 발동 후 사라짐)
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Potion);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Throwable);
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Scroll);
}

/**
 * BBM 아이템 타입별 전투 사용 횟수 및 사후 처리 규칙 열거형
 */
UENUM(BlueprintType)
enum class EBBMItemUsageRule : uint8
{
	/* 매 턴 제한 없이 반복 발동하며 배낭 칸에 유지 (무기) */
	Unlimited		= 0 UMETA(DisplayName = "무제한 반복 사용 (배낭 유지)"),

	/* 전투당 1회만 사용 가능하며, 사용 후 다음 턴부터 배낭 칸을 비우고 별도 보관 칸으로 데이터 이관 (장비) */
	OncePerBattle	= 1 UMETA(DisplayName = "전투당 1회 사용 (별도 칸 보관)"),

	/* 효과 발동 즉시 스택 수량 1개가 영구 소멸 (소모품) */
	ConsumeOnUse	= 2 UMETA(DisplayName = "일회성 소모 (효과 후 소멸)")
};

/**
 * BBM(비트 배낭 상인) 전용 아이템 타입 프로파일 커스텀 데이터 페이로드
 * 부모 FDISItemTypeCustomData의 MaxStackCount와 함께 BBM 전투 사용 규칙을 보유함
 */
USTRUCT(BlueprintType, meta = (DisplayName = "BBM Item Type Custom Data"))
struct BBM_API FBBMItemTypeCustomData : public FDISItemTypeCustomData
{
	GENERATED_BODY()

	/* 생성자 및 라이프사이클 */
public:
	FBBMItemTypeCustomData();
	FBBMItemTypeCustomData(int32 InMaxStackCount, EBBMItemUsageRule InUsageRule);
	virtual ~FBBMItemTypeCustomData() override = default;

	/* 프로퍼티 */
public:
	/* 전투 시 아이템 사용 제한 및 사후 보관/소멸 처리 규칙 */
	UPROPERTY(Category = "BBM|Usage", EditAnywhere, BlueprintReadWrite)
	EBBMItemUsageRule UsageRule = EBBMItemUsageRule::Unlimited;
};
