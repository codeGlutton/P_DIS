# [BBM GDD] 02. 인벤토리, 아이템 프로파일 및 타임라인 계산 규칙 (Inventory & Items)

> **상위 마스터 문서**: [RhythmRoguelike_GDD.md](file:///e:/Unreal/P_AI_Inven/DIS/Docs/RhythmRoguelike_GDD.md)  
> **문서 버전**: 1.3.0 | **최종 수정일**: 2026-10-29  
> **주요 역할**: 배낭 그리드 배치 순서, 점유 면적 공식, 3대 상위/13종 세부 프로파일, 내구도/스택 한도, 사용 수명주기, CustomData 계층 상속 단일 진실 공급원

---

## 4. 인벤토리, 아이템 프로파일 및 타임라인 계산 규칙

### 4.1 아이템 정렬 및 실행 순서 규격
전투 시뮬레이션에서 아이템의 실행 순서는 **인벤토리 공간 내 배치 위치**에 의해 100% 결정됨.

1. **단일 기준점: 좌상단 루트 슬롯 (Top-Left Root Slot)**:
   - 다중 타일 점유 아이템은 DIS 플러그인의 `UDISInventory::GetItemStackBaseCoord()`를 기준으로 판정.
   - **우선순위 1**: 그리드 행 좌표 `Y` 오름차순 (위쪽 줄 우선: `Y=0` -> `Y=1` -> `Y=2`...)
   - **우선순위 2**: 동일 행 내 열 좌표 `X` 오름차순 (왼쪽 칸 우선: `X=0` -> `X=1` -> `X=2`...)
2. **비트 비용 계산 (점유 면적 연동 공식)**:
   $$\text{ItemBeatCost} = \text{InItemStack}->\text{GetOccupiedPoints}().\text{Num}()$$
3. **타임라인 순차 누적 큐 (Strict Sequential Accumulation)**:
   - 정렬된 아이템들을 순회하며 비트 누적:
     - $\text{StartBeat} = \text{CurrentBeat}$
     - $\text{EndBeat} = \text{StartBeat} + \text{ItemBeatCost}$
   - 만약 $\text{EndBeat} \le 8$ 이면 타임라인 등록 확정 및 $\text{CurrentBeat} = \text{EndBeat}$.
   - 만약 $\text{EndBeat} > 8$ 이면 해당 아이템 및 이후 대기 중인 모든 아이템은 **비트 초과(Overflow/Deactivated)**로 처리되어 이번 턴에 미실행.

---

### 4.2 3대 아이템 프로파일 및 사후 처리 규격 (Item Type Profiles & Usage Lifecycle)
BBM의 모든 아이템은 **내구도를 사용하지 않으며(`MaxDurability = 0.0f`, 무제한)**, 역할과 전투 중 소비·보관 방식에 따라 3대 상위 프로파일과 13종 세부 프로파일로 구분됨.

| 상위 분류 (Tag) | 하위 세부 프로파일 | 내구도 (`MaxDurability`) | 기본 `MaxStackCount` | 사용 규칙 (`EBBMItemUsageRule`) | 전투 중 사후 처리 방식 |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **무기 (`Item.Weapon`)** | **방패(`Shield`)**, 한손검(`OneHandSword`), 창(`Spear`), 지팡이(`Staff`), 활(`Bow`), 단검(`Dagger`) | 없음 (`0.0f`) | `1` (스태킹 불가) | `Unlimited` | 매 턴 제한 없이 반복 발동하며 중앙 그리드 배낭 칸을 계속 유지함 (**방패는 `Equipment`가 아닌 `Weapon` 프로파일 소속**) |
| **장비 (`Item.Equipment`)** | 장갑(`Gloves`), 옷(`Clothes`), 신발(`Shoes`), 바지(`Pants`) | 없음 (`0.0f`) | `1` (스태킹 불가) | `OncePerBattle` | **전투당 1회만 사용 가능**. 타임라인에서 사용된 장비는 **다음 턴부터 인벤토리 배낭 칸을 차지하지 않고 화면에 보이지 않으며**, 데이터는 별도 보관 칸(`UsedEquipmentStorage`)에 안전히 분리 보관됨 (전투 종료 시 복원) |
| **소모품 (`Item.Consumable`)** | 물약(`Potion`), 투척물(`Throwable`), 일회성 주문서(`Scroll`) | 없음 (`0.0f`) | **`5`** (스태킹 가능) | `ConsumeOnUse` | 효과 발동 직후 스택 수량 1개가 즉시 소모되며, 잔여 수량이 `0`이 되면 배낭 인벤토리에서 영구 소멸함 |

---

### 4.3 프로파일 `CustomData` 계층 상속 및 Override 규칙
1. **`CustomData` 내장 스택 및 확장 규칙**:
   - DIS 플러그인의 `FDISItemTypeCustomData`가 `MaxStackCount`(기본값 `1`)를 보유하며, BBM은 이를 상속받은 `FBBMItemTypeCustomData`에 `UsageRule`(`EBBMItemUsageRule`)을 확장 정의함.
2. **`bOverrideCustomData` 기반 선택적 상속 제어**:
   - `FDISItemTypeProfile::bOverrideCustomData == true`인 프로파일(예: `Item.Weapon`, `Item.Equipment`, `Item.Consumable` 3대 대분류)은 부모 프로파일의 `CustomData`를 상속받지 않고 자기 자신의 `CustomData`를 적용함.
   - `FDISItemTypeProfile::bOverrideCustomData == false`인 하위 프로파일(예: 방패, 한손검, 옷, 물약 등 13종 기본 세부 타입)은 **자기만의 고유 `DisplayName`과 `Icon`을 가지면서 `CustomData`(`MaxStackCount`, `UsageRule`)는 상위 부모 프로파일로부터 그대로 상속**받음.
   - 특정 희귀 주문서나 예외적인 하위 타입이 독자적인 스택 한도를 가져야 할 경우에만 해당 하위 프로파일에서 `bOverrideCustomData = true`를 활성화하여 개별 오버라이드함.

