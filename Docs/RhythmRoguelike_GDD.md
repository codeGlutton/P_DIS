# [GDD] 비트 배낭 상인 (Beat Backpack Merchant)
## 리듬 액션 × 그리드 인벤토리 × 로그라이크 기획 및 테크니컬 디자인 마스터 인덱스

- **문서 버전**: 1.4.0 (모듈러 분할 아키텍처 개편)
- **최종 수정일**: 2026-11-11
- **장르**: 리듬 액션 타임라인 + 인벤토리 퍼즐 + 턴제 로그라이크
- **엔진 버전**: Unreal Engine 5.8.3 (C++ 20)
- **기반 플러그인**: DIS (Data-Driven Inventory System) — 범용 데이터 드리븐 구조 유지

---

## 1. 개요 및 모듈러 문서 구조 안내

본 문서는 `BBM(비트 배낭 상인)` 프로젝트의 전체 기획 및 테크니컬 디자인 규격을 총괄하는 **마스터 인덱스 허브(Master Hub)**임.  
에이전트 및 개발자가 필요한 기획 영역만을 신속하게 참조하고 유지보수할 수 있도록, 기획 문서를 핵심 주제별 **4대 하위 단락 문서**로 분할하여 관리함.

```
Docs/
├── RhythmRoguelike_GDD.md                           <-- [현재 파일] 마스터 인덱스 및 총괄 허브
└── GDD/
    ├── 01_Game_Overview_and_UI.md                  <-- 1. 게임 개요, 아트 디렉션 및 화면 레이아웃
    ├── 02_Inventory_and_Item_Profiles.md           <-- 2. 인벤토리, 아이템 프로파일 및 타임라인 계산 규칙
    ├── 03_Combat_and_Lifecycle.md                  <-- 3. 1D 라인 전투, 리듬 행동 및 라이프사이클 프레임워크
    └── 04_Technical_Architecture_and_Roadmap.md    <-- 4. 테크니컬 아키텍처 및 단계별 로드맵
```

---

## 2. 4대 세부 기획 문서 인덱스 및 참조 가이드

| 문서 번호 | 파일 경로 | 주요 다루는 기획 주제 | 읽어야 하는 작업 (Reference) | 기획 변동 시 갱신 대상 (Update) |
| :---: | :--- | :--- | :--- | :--- |
| **01** | [01_Game_Overview_and_UI.md](file:///e:/Unreal/P_AI_Inven/DIS/Docs/GDD/01_Game_Overview_and_UI.md) | - 세계관 콘셉트 & 5대 게임플레이 필라<br>- 아트 비주얼 테마 & 컬러 코딩<br>- 3단 화면 레이아웃 (상단 트랙, 중앙 배낭, 하단 듀얼 프리뷰) | - UI/UX 에셋 및 목업 제작 시<br>- UMG 위젯 레이아웃 및 앵커 배치 시<br>- 배경/스프라이트/버튼 아트 디자인 시 | - 화면 UI 구조 변경<br>- 비주얼 테마/아트 스타일 변경<br>- 게임 핵심 콘셉트 변동 시 |
| **02** | [02_Inventory_and_Item_Profiles.md](file:///e:/Unreal/P_AI_Inven/DIS/Docs/GDD/02_Inventory_and_Item_Profiles.md) | - 배낭 그리드 Y/X 정렬 및 비트 계산<br>- 3대 상위/13종 세부 아이템 프로파일<br>- 내구도(0.0f 무제한) & 스택 규칙<br>- 전투당 1회 장비 보관 & 소모품 소멸<br>- CustomData 계층 상속 및 Override | - DIS 인벤토리 연동 및 배치 로직 작성 시<br>- 신규 아이템 정의(ItemDefinition) 구현 시<br>- 아이템 사용 규칙 및 스택 수명주기 구현 시 | - 신규 아이템 타입/태그 추가<br>- 아이템 점유 비트 계산 공식 변경<br>- 장비/소모품 사용 규칙 변동 시 |
| **03** | [03_Combat_and_Lifecycle.md](file:///e:/Unreal/P_AI_Inven/DIS/Docs/GDD/03_Combat_and_Lifecycle.md) | - 1D 레인 진형 및 타깃 범위(5종)<br>- 3단계 방어 체계 (패링→가드→방어도→HP)<br>- 2종 상태이상 (독/화상)<br>- 6대 논리 실행 페이즈 & 4분할 연출 윈도우<br>- 8초 턴 라이프사이클 & 전투 상태 머신 | - 전투 시뮬레이터(`UBBMCombatSimulator`) 작성 시<br>- 교전 데미지/피격/패링/방어도 계산 시<br>- 상태이상(독/화상) 틱 구현 시<br>- 게임모드 전투 턴 루프 및 8초 타이머 구현 시 | - 교전 피해 해결 순서 및 공식 변경<br>- 신규 상태이상 또는 타깃 범위 추가<br>- 턴 진행 페이즈 및 타이머 규격 변동 시 |
| **04** | [04_Technical_Architecture_and_Roadmap.md](file:///e:/Unreal/P_AI_Inven/DIS/Docs/GDD/04_Technical_Architecture_and_Roadmap.md) | - C++ 클래스 다이어그램 & 모듈 의존성<br>- 시뮬레이션 큐 및 핵심 구조체 정의<br>- GAS 어트리뷰트 및 Effect 연동 체계<br>- 단계별 개발 로드맵 및 마일스톤 달성도 | - C++ 클래스 설계 및 상속 구조 파악 시<br>- 마일스톤 구현 현황 및 단위 테스트 범위 확인 시<br>- 프로젝트 개발 우선순위 및 태스크 착수 시 | - C++ 클래스/구조체 인터페이스 변경<br>- 마일스톤 달성 및 완료 체크박스 갱신<br>- 신규 개발 태스크 및 로드맵 일정 추가 시 |

---

## 3. 핵심 게임플레이 필라 요약 (Core Pillars)

1. **인벤토리 공간 최적화 (Grid Spatial Puzzle)**: DIS 플러그인 2D 그리드 배낭에 형상이 다양한 아이템을 효율적으로 테트리스 배치.
2. **초단기 실시간 판단 (8-Second Real-time Prep)**: 몬스터 조우 시 8초 카운트다운 동안 아이템을 신속하게 드래그·재배치하여 전투 시퀀스 재구성.
3. **점유 칸 수 비례 8비트 타임라인 (Cell-Count Proportional Beats)**: 아이템 점유 면적 = 비트 소비량. 좌상단부터 우하단 순서로 8비트 타임라인에 순차 누적.
4. **예측과 능동적 카운터 (Monster Intent & Dual Preview)**: 적의 8비트 예고를 보고 주인공 프리뷰를 실시간 대응하여 방패 가드/패링 및 콤보 구성.
5. **로그라이크 빌드 성장 (Roguelike Progression)**: 전투 승리 보상, 가방 용량 확장(`ExpandCapacity`), 젬 강화를 통한 매 판 새로운 빌드 완성.
