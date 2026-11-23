# 02. BBM 배낭 인벤토리 · 아이템 프로파일 · 비트 액션 (`Source/BBM/{Inventory,Item}/`)

범용 `DIS` 코어를 상속·확장하여 샘플 게임의 배낭 규칙, 3대 아이템 프로파일, 다형성 비트 액션을 구현한 계층임.

## 1. 배낭 인벤토리 및 슬롯 강화 (`Source/BBM/Inventory/`)
- **배낭 컴포넌트·런타임**: `UBBMInventoryComponent`(세이브 레코드 복원 `RestoreInventoryFromSaveRecords` 지원), `UBBMInventory`, `UBBMInventorySlot`
- **배낭 정의 및 사후 보관**: 추출 순서·슬롯 강화 정의(`UBBMInventoryDefinition`), 전투당 1회 장비 분리 보관소(`UsedEquipmentStorage`)
- **슬롯 강화 체계**: `BBMSlotEnhancement`, `BBMSlotEnhancementEffect`, 네이티브 태그(`BBMEnhancementTags`)

## 2. 확장 아이템 정의 · 프로파일 · 비트 액션 (`Source/BBM/Item/`)
- **전역 프로파일 확장**: `UBBMGlobalDefinition` ← `UDISGlobalDefinition` (`ItemTypeProfiles`, `ItemGroupProfiles`), `UBBMGlobalLogic` ← `UDISGlobalLogic`, `FBBMItemTypeCustomData` ← `FDISItemTypeCustomData` (`MaxStackCount`, `UsageRule`)
- **아이템 정의/엔트리 확장**:
  - `UBBMItemDefinition` ← `UDISItemDefinition`: `Price`, `Beats`, `LinkedAnimLayers`, `DefaultGrantedEffects`(`FBBMItemGameplayEffectConfig`: `EffectClass`, `SetByCallerMagnitudes`), `ItemGroupTags`, `GetBeatCount()`, `HasItemGroup()`
  - `UBBMItemEntry` ← `UDISItemEntry`: 런타임 가변 비트(`DynamicBeats`), 동적 Granted GE(`DynamicGrantedEffects`), `AddEffectToBeat`, `AddDynamicBeat`, `SetByCallerMagnitude` API
- **다형성 비트 액션 및 연출 구성**:
  - `TInstancedStruct<FBBMBeatAction>` (`FBBMBeatAction::GetActionCategory`, `ExecuteAction`, `FBBMCombatAction` [`TargetScope`, `Coefficient`, `BaseValue`, `CalculateMagnitude`], `FBBMMovementAction` [`StepCount`], `FBBMArmorAction`)
  - `FBBMBeatDefinition` (`Actions`, `FBBMBeatAnimationConfig` 4분할 윈도우 애니메이션)
  - 비트 시퀀스 강화 (`BBMEnhancementTypes`, `UBBMBeatEnhancement`, `UBBMEffectRangeBeatEnhancement`, `UBBMConditionalActionBeatEnhancement`, `FBBMBeatAnimLayerRange`)
