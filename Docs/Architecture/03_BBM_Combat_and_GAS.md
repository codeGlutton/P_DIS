# 03. BBM 전투 시뮬레이션 · GAS · 전투원 (`Source/BBM/Combat/`)

1D 라인 전투 시뮬레이션, 전투원 폰·애니메이션, GAS 어트리뷰트 및 팀 태도 기능의 아키텍처 명세임.

## 1. 1D 전투 시뮬레이션 및 타일 맵 (`Source/BBM/Combat/`)
- **전투 매니저/시뮬레이터**: `UBBMCombatManagerSubsystem` (10단계 턴 시뮬레이션 `SimulateTurn`, 비주얼 이벤트 디스패치), `UBBMCombatSimulator` (`ApplyItemGrantedEffects`/`RemoveItemGrantedEffects`, 4단계 피해 해결 `ResolveDamage`/`ResolveGainArmor`)
- **1D 타일 맵 및 레인**: `ABBM1DTileMapActor`(아군/적군 각 3칸 ISM 및 배치 트랜스폼), `EBBMCombatLane`, `EBBMLaneShiftDirection`, `FBBM1DCombatLane`
- **전투 태그 체계**: `BBMStatusTags`, `BBMAnimationTags`, `BBMSetByCallerTags`

## 2. 전투 큐 모델 (설계/구현 대상 구조체)
- `FBBMSimulationBeatQueues`: 단일 비트 내 6대 실행 범주 큐 배열(`PhaseQueues[6]`, `GetQueue(EBBMActionCategory)`, `Reset()`)

## 3. 전투원 폰 · 애니메이션 · GAS · 팀 (`Source/BBM/Combat/{Pawn,Animation,GAS,Team}/`)
- **전투원 (`Pawn/`)**: `ABBMCombatantBase`(ASC/AttributeSet 보유, `InitialStatsCurveTable`, `InitializeStatsWithCurveTable`, `LaneIndex`, `bIsAlly`, `IsDead`), `BBMPlayerMerchantBase`(`CurrentGold`, `ApplyRunSaveData`), `BBMAICombatantBase`, 정의 에셋(`BBMCharacterDefinition`, `BBMAICombatantDefinition`, `BBMPlayerCharacterDefinition`), 동적 애님 레이어(`LinkAnimLayerByTag`/`UnlinkAnimLayerByTag`) 및 `StartDissolve`
- **애니메이션 (`Animation/`)**: `UBBMCombatantAnimInstance` (태그 기반 AnimLayer 매핑)
- **GAS (`GAS/`)**: `UBBMAttributeSet`(`Health`, `MaxHealth`, `Armor`, `TurnArmor`, `AttackPower`, `DefensePower`), 공격/방어 강화 GE(`UBBMAttackPowerEnhancementEffect`, `UBBMDefensePowerEnhancementEffect`), `AbilitySystemGlobals` 동적 `CompositeCurveTable` 스탯 초기화(`FAttributeSetInitterDiscreteLevels`)
- **팀 판별 (`Team/`)**: `GenericTeamId` 팀 관계 타입(`BBMTeamTypes`)
