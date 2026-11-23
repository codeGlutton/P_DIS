# 01. 범용 DIS 인벤토리 플러그인 코어 (`Plugins/DIS/`)

호스트 게임(`BBM`)과 독립적으로 재사용되는 범용 Data-Driven 인벤토리 플러그인(`DISRuntime`, `DISEditor`)의 기능별 아키텍처 명세임.

## 1. 런타임 코어 (`Plugins/DIS/Source/DISRuntime/`)
- **인벤토리 모델 (`Inventory/`)**: 2D 그리드 공간 인벤토리(`UDISInventory`, `UDISInventory::GetItemStackBaseCoord()` 등) 및 1D 공유/단축 인벤토리·슬롯 관리
- **아이템 데이터 (`Item/`)**: 아이템 정의(`UDISItemDefinition`, 기본 `MaxDurability = 0.0f` 무제한), 런타임 엔트리(`UDISItemEntry`), 스택(`UDISItemStack`), 배치 규칙 및 확장용 커스텀 데이터(`FDISItemTypeProfile::bOverrideCustomData`, `FDISItemTypeCustomData`)
- **강화 시스템 (`Enhancement/`)**: 슬롯 및 아이템 단위 스탯/효과 강화 컨테이너(`UDISEnhancementContainer`) 및 규칙 처리
- **전역 로직 (`Global/`)**: 전역 정의(`UDISGlobalDefinition`), 전역 서브오브젝트(`UDISGlobalLogic`) 및 호스트 서브시스템 연동
- **플러그인 콘텐츠 (`Plugins/DIS/Content/Global/`)**: 전역 설정 및 수식어 표시 서식 스트링 테이블(`ST_DIS_Global`)

## 2. 에디터 확장 및 리소스 (`Plugins/DIS/Source/DISEditor/`, `Resources/`)
- 인벤토리 데이터 에셋 유효성 검사, 에셋 액션, 디테일 커스터마이제이션 및 마켓플레이스 아이콘 리소스 보관부
