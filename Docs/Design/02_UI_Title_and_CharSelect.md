# 02. 타이틀 · 캐릭터 선택 UI 상세 규격

[.agents/rules/art-design-guide.md](file:///e:/Unreal/P_AI_Inven/DIS/.agents/rules/art-design-guide.md)에 기반한 **타이틀 화면** 및 **캐릭터 선택 모달**의 상세 레이아웃 레퍼런스임.

## 1. 타이틀 화면 (`WBP_Title`)
- **화면 비율**: 16:9 와이드스크린 (`Right-Aligned UI Layout`)
- **좌측 전경~중앙**: 몬스터 전투 액션 아트워크(Var 2 패링 또는 Var 4 승리 구도)의 주인공 실루엣 및 전투 이펙트 노출
- **우측 영역 (Right Column)**: 상단 게임 로고 + 직하단 5대 메뉴 버튼 수직 배치
  1. `[ 처음 시작 / New Game ]`: 기본 포커스, 황동 테두리 및 앰버 골드 하이라이트
  2. `[ 이어하기 / Continue ]`: 세이브 데이터 유무에 따라 활성/비활성(`Disabled`) 분기
  3. `[ 환경설정 / Settings ]`: 톱니바퀴 아이콘 연동
  4. `[ 크레딧 / Credits ]`: 두루마리 리본 모티브
  5. `[ 게임 종료 / Quit ]`: 하단 배치, 차분한 가죽 테두리
- **버튼 규격**: 가로 `340px × 세로 72px` (Nine-Slice 가죽 버튼, 모서리 슬라이스 마진 `20px`)
  - 황동 코너 브래킷 + 모서리당 Triple Rivets + 상/하 중앙 황동 클램프, 내부는 점선 스티치를 제거한 웜 브라운 가죽 그라데이션
  - 4대 상태: `Normal` / `Hover`(앰버 골드 테두리 `#FFDA5A`, 외곽 잔여선 없는 단일 클린 림) / `Pressed`(음각 침하 + 1px 오프셋) / `Disabled`(저채도 가죽/무광 림)

---

## 2. 캐릭터 선택 패널 (`WBP_CharacterSelect`)
- **공식 확정 목업**: `Title/Mockup/T_Mockup_Modal_CharSelect_Official.jpg` (`1920 × 1080 px` 기준 중앙 모달)
- **외곽 보드**: 3단 분할 와이드 패널 (`T_Frame_CharSelect_3Panel_Board`, `1440 × 820 px` + 상단 `T_Header_Banner_BrassPlate`)
- **모듈러 3단 구성**:
  1. **좌측 상인 명부 윈도우 (`T_Panel_Inner_DarkSlate_Window`, 폭 320px 계열)**:
     - 상단 필(`T_Frame_CharSelect_RosterTitlePill`, `284 × 44`), 카드(`T_Card_CharSelect_RosterItem_Normal/Selected`, `300 × 84`), 원형 초상화 링(`T_Frame_CharSelect_PortraitRing_Normal/Selected`, `128 × 128` + UI 머티리얼 `M_UI_PortraitRing`), 선택 별(`T_Icon_CharSelect_SelectedStar`, `64 × 64`), 상인 초상화 3종(`T_Portrait_Merchant_{Gogglebeard,Tinkerbelt,Shadowbag}`, `512 × 512`)
  2. **중앙 3D 캐릭터 포즈 프리뷰 (`T_Panel_Inner_DarkSlate_Window`, 폭 528px 계열)**: 회전 조작 없이 석조 페데스탈(`T_Stage_CharSelect_Pedestal`) 위 고정 스탠딩 포즈 표시
  3. **우측 통합 상인 등록 증명서 + 액션 버튼 (폭 448px 계열)**:
     - 양피지(`T_Panel_Inner_Parchment_Standard`): 우상단 길드 밀납 인장(`T_Icon_WaxSeal_MerchantGuild`), 2×2 스탯 셀(`T_Panel_CharSelect_StatCell`, `256 × 64` 텍스트 무삽입), 단일 특수 비트 배너(`T_Banner_CharSelect_SpecialTrait`, `420 × 44`, BPM 표기 제외), 초기 3×3 배낭 인벤토리 박스(`T_Panel_CharSelect_InventoryBox`, `T_Icon_CharSelect_CastArrow`, `T_Frame_CharSelect_Backpack_Empty`, `T_Slot_CharSelect_{Normal,Enhanced}`, 시작 아이템 `T_Icon_Item_Starter{Dagger,Shield}`)
     - 하단 액션 버튼: `[ 원정 출격! (EMBARK) ]` (`T_Btn_Modal_Gold_*`), `[ 닫기 ]` (`T_Btn_Modal_Leather_*`)
