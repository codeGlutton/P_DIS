# 03. 공용 시스템 모달 UI 상세 규격

[.agents/rules/art-design-guide.md](file:///e:/Unreal/P_AI_Inven/DIS/.agents/rules/art-design-guide.md)에 기반한 **옵션, 크레딧, 로딩, 세이브 알림 모달**의 상세 레이아웃 레퍼런스임.

## 1. 옵션 패널 (`WBP_Settings`)
- **화면 형태**: 화면 중앙 모달 팝업 (`1040 × 700 px`, 배경 딤드 암전 `Opacity: 0.65`)
- **모듈러 분리 아키텍처**:
  - **외곽 창틀 2종**:
    1. 표준 모달 창틀(`T_Frame_Modal_LeatherBrass_Standard`, `1040 × 700 px`): 내부 100% 투명 순수 외곽 9-슬라이스 창틀, 4각 브래킷(`44px`) 및 중앙 클램프
    2. 상단 폭 확장 탭바 대응 창틀(`T_Frame_Modal_LeatherBrass_TopExpanded`, `1040 × 700 px`): 상단 가죽 바 폭을 `38px → 92px`로 확장하여 4대 탭 버튼 수용
  - **내부 양피지 인레이**: `T_Panel_Inner_Parchment_Standard` (`960 × 620 px`)
  - **상단 헤더 명판**: `T_Header_Banner_BrassPlate` (`560 × 56 px`)
- **4대 탭 바**:
  1. `[ 게임 (Game) ]`: 언어(한국어/English 콤보박스), 컨트롤러 진동 토글
  2. `[ 오디오 (Audio) ]`: 마스터/BGM/SFX/다이얼로그 슬라이더, 비트 시각 보조 효과 토글 (싱크 칼리브레이션 제외)
  3. `[ 그래픽 (Graphics) ]`: 해상도, 화면 모드(전체화면/창모드/테두리없는창), 최대 FPS 제한(60/120/144/무제한), V-Sync 토글
  4. `[ 조작 (Controls) ]`: 단축키 설정 및 키 재매핑(Rebind) 목록
- **하단 액션 버튼**: `[ 기본값 복원 (Reset) ]` / `[ 설정 적용 (Apply) ]` / `[ 닫기 (Close) ]`

---

## 2. 크레딧 패널 (`WBP_Credits`)
- **화면 형태**: 양피지 두루마리 모티브 (`760 × 860 px`)
- **공식 확정 에셋**: `T_Frame_Credits_Parchment_Scroll` (다크 브라운 가죽 받침 + 상하단 컬링 양피지, 금속 코너 브래킷 배제)
- **스크롤 인터랙션**: 초당 `30px` 자동 스크롤, 마우스 휠/드래그, 스페이스바 일시정지/재개, ESC/닫기 종료
- **콘텐츠**: 공식 로고, 프로덕션/프로그래밍/아트/사운드 크레딧, 상업용 무료 폰트 출처 및 엔진 스페셜 땡스

---

## 3. 로딩 화면 (`WBP_Loading`) & 세이브 알림 (`WBP_Save`)
- **로딩 화면**: 전체 배경(`T_Loading_Background_Main`), 황동 TIP 태그(`T_Tag_Loading_Tip_Brass`), 슬림 프로그레스 바(`T_Loading_Bar_Track_Slim` / `Fill_Slim`, 끝단 스파크 `T_FX_Loading_Bar_TipGlow`), 등불 걷기 엠블럼(`T_Icon_Loading_LanternEmblem`)
- **세이브 알림**: 우측 상단 토스트 배너(`T_Frame_SaveIndicator_LeatherBrass`, `440 × 96`), 장부/깃펜 아이콘(`T_Icon_Save_LedgerQuill`), 펄스 링 FX(`T_FX_Save_PulseRing`), 진행 도트(`T_Icon_Save_Dot_{On,Off}`)
