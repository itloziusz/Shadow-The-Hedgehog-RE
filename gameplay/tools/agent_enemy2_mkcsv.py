"""Write gameplay/notes/enemy2_symbols.csv from the ROWS below (BkSoldier / EggPawn / GunBeetle investigation),
skipping addresses already present in gameplay/symbols_curated.csv.
usage: python agent_enemy2_mkcsv.py
Row format: address|name|subsystem|confidence|evidence   (commas in evidence are written as ';')
"""
import csv
import os

HERE = os.path.dirname(os.path.abspath(__file__))
GP = os.path.dirname(HERE)

ROWS = r"""
8018CAD0|BkSoldier_Create|enemy|PROVEN|SET desc 0x80531E70 hook+0C; new(0x438); BkSoldier ctor on layer 0xE
8018CB28|BkSoldier_ReleaseResources|enemy|PROVEN|desc hook+08; ResourceManager slot 0xC (type 5 + 7)
8018CB50|BkSoldier_LoadResources|enemy|PROVEN|desc hook+04; ResourceOneFile enemy/BkSoldierData.one -> slot 0xC
8018CEA8|BkSoldier_GetSetParamLabel|enemy|STRONG|dataptr; TNumEditHex; jump table 0x80532778 on param idx 8..22; label tables 0x80531A40 (Weapon) 0x80531A64 (AppearType) 0x80531A7C (WaitType)
8018E618|BkSoldierBase::InitFromSet|enemy|PROVEN|RTTI vf21 (SB slot 6); TEnemySetBase::vf06; SetSlot_GetParams(0x60); clamps; SoldierCommon_ReadSetParams; OFFSETPOS/WARP spawn offset; WARP delay 1.0
8018E964|BkSoldierBase::ReinitFromSet|enemy|LIKELY|RTTI vf20 (SB slot 5); re-runs InitFromSet + Disp slot 0xC + AI slot 6; no caller found
8018E478|BkSoldierBase::UpdateActivation|enemy|PROVEN|RTTI vf0F (Status slot 0xC); AppearType 4/5 hidden+inactive until EnemySensor_IsInSearchArea; delay E+0x360; warp effect vf22 once
8018D81C|BkSoldierBase::TakeHit|enemy|PROVEN|RTTI vf10 (Status slot 0x1A); shield E+0x34C frontal block/absorb (SE 0x6076/0x6077); always ApplyHit; E+0x358 = hit from front
8018D3A0|BkSoldierBase::OnDeath|enemy|STRONG|RTTI vf11 (Status slot 0x26) at death edge; EnemyStatusCommon::vf26; SE 0x6016; drop shield; EnemyWeapon_Drop; E+0x354=0; E+0x308=0
8018D360|BkSoldierBase::OnKnockback|enemy|LIKELY|RTTI vf12 (Status slot 0x2E; called by EnemyStatusCommon::Update when S+0x75 before Knockback); dismounts AirSaucer (fn_8018D52C) if handle E+0x368 valid
8018E9F4|BkSoldierBase::GetScoreId|enemy|PROVEN|RTTI vf0E (Status slot 0x2C); HaveShield(E+0x304) ? 0x59 BkSoldierShield : 0x58 BkSoldier
8018E0A4|BkSoldierBase::InitDisplay|enemy|PROVEN|RTTI vf14 (Disp slot 0); shield model; new(0x68) EnemyWeapon kind 0x804C9B18[WeaponType] -> E+0x354; RideAirSaucer -> Vehicle_Create(7)
8018DE04|BkSoldierBase::UpdateWeaponAndVoice|enemy|STRONG|RTTI vf0B (Disp slot 2); shield break check; weapon placement; SE 0x6013/0x6014 every rand(8;12) s while player in search area
8018DD6C|BkSoldierBase::OnMotionEvent|enemy|STRONG|RTTI vf15 (Disp slot 0xE); event 0x5000 data 2 -> fn_801AB790 (weapon fire); event 0xD000 -> E+0x365
8018DC94|BkSoldierBase::OnMotionSoundEvent|enemy|LIKELY|RTTI vf16 (Disp slot 0xF); events 3/4/7 -> SE 0x6011/0x6010/0x2005 and effects
8018D7C8|BkSoldierBase::PlayWarpInEffect|enemy|LIKELY|RTTI vf22; EnemyDispCommon::vf12 + positional SE 0x6012; called once by UpdateActivation for WARP/OFFSETPOS
8018D204|BkSoldierBase::UpdateMove|enemy|STRONG|RTTI vf1F (Move slot 1); RideAirSaucer: AccelerateInputCommand(kind 4) to vehicle E+0x368 and copy pose; else EnemyMoveCommon::Update
8018DA48|BkSoldierBase::GetPatrolMode|enemy|PROVEN|RTTI vf0D; AppearType 1/2/3 -> 1/2/3 else 0
8018DA98|BkSoldierBase::GetPointWaitMotion|enemy|STRONG|RTTI vf0C; per-point WaitType 0 ATTACK / 1 HIDE (motion 0xF) / 2 KAMAE
8018D52C|BkSoldier_DismountAirSaucer|enemy|LIKELY|E+0x344=0; releases vehicle handle E+0x368; zero pitch/roll
8018D684|BkSoldier_ReleaseAirSaucer|enemy|LIKELY|fn_801B6CE4 on vehicle handle E+0x368 then reset; called from ~BkSoldierBase
8018D4D4|BkSoldier_IsTargetInSearchArea|enemy|PROVEN|EnemySensor_IsInSearchArea(E+0xA8; AI GetTargetPosition)
8018D700|BkSoldier_WeaponEndAttack|enemy|LIKELY|EnemyWeapon helpers fn_801AAF08/fn_801AAC8C on E+0x354; AttackVacuum Exit and fire-phase end
8018D764|BkSoldier_WeaponBeginAttack|enemy|LIKELY|EnemyWeapon helpers fn_801AAF74/fn_801AAD88 on E+0x354; AttackVacuum fire-phase start
80180770|BkSoldierAI::ReturnToWait|enemy_ai|PROVEN|RTTI vf0D (also initial state via EnemyBaseAI::vf0C); AppearType 0 Standing; 1-3 Moving; 4/5 AppearHome once (AI+0x7C) then Standing
80180738|BkSoldierAI::OnTargetFound|enemy_ai|PROVEN|RTTI vf0F; ChangeState(BkSoldierAI_Caution)
80180560|BkSoldierAI_StartCombat|enemy_ai|PROVEN|WeaponType 0/1 -> ChaseTarget; 6 VACUUM_POD -> AttackVacuum; else Attack
8017F460|BkSoldierAI_GetState_Caution|enemy_ai|PROVEN|static EnemyAIState_BkSoldierAI_Caution at 0x805F0514
8017F8D4|BkSoldierAI_GetState_GoHome|enemy_ai|PROVEN|static EnemyAIState_BkSoldierAI_GoHome at 0x805F0550
8018009C|BkSoldierAI_GetState_ChaseTarget|enemy_ai|PROVEN|static at 0x805F0520
8018031C|BkSoldierAI_GetState_NearAttack|enemy_ai|PROVEN|static at 0x805F052C
801805E0|BkSoldierAI_GetState_Attack|enemy_ai|PROVEN|static at 0x805F0538
8018068C|BkSoldierAI_GetState_AttackVacuum|enemy_ai|PROVEN|static at 0x805F0544
801808E8|BkSoldierAI_GetState_AppearHome|enemy_ai|PROVEN|static at 0x805F055C
801804E0|EnemyAIState_BkSoldierAI_Caution::Enter|enemy_ai|PROVEN|state vt 0x8052EEB8 slot 2
8018043C|EnemyAIState_BkSoldierAI_Caution::Update|enemy_ai|PROVEN|slot 3; WeaponType 0/1 -> ChaseTarget; motion flag B -> StartCombat
8017FE34|EnemyAIState_BkSoldierAI_Attack::Enter|enemy_ai|PROVEN|state vt 0x8052EE34 slot 2
8017FA80|EnemyAIState_BkSoldierAI_Attack::Update|enemy_ai|PROVEN|slot 3; aim/fire cycle; not in search area -> BkSoldierAI_GoHome
8017FA30|EnemyAIState_BkSoldierAI_Attack::Exit|enemy_ai|PROVEN|slot 4; E+0x359=0
8017F980|EnemyAIState_BkSoldierAI_AttackVacuum::Enter|enemy_ai|PROVEN|state vt 0x8052EE08 slot 2
8017F5EC|EnemyAIState_BkSoldierAI_AttackVacuum::Update|enemy_ai|PROVEN|slot 3; motion 0xD; weapon begin/end; not in area -> GoHome
8017F590|EnemyAIState_BkSoldierAI_AttackVacuum::Exit|enemy_ai|PROVEN|slot 4; BkSoldier_WeaponEndAttack
801803C8|EnemyAIState_BkSoldierAI_ChaseTarget::Enter|enemy_ai|PROVEN|state vt 0x8052EE8C slot 2
80180234|EnemyAIState_BkSoldierAI_ChaseTarget::Update|enemy_ai|PROVEN|slot 3; not in area -> EnemyBaseAI_GoHome; distance < 30 -> NearAttack
801801A4|EnemyAIState_BkSoldierAI_NearAttack::Enter|enemy_ai|PROVEN|state vt 0x8052EE60 slot 2
8017FF00|EnemyAIState_BkSoldierAI_NearAttack::Update|enemy_ai|PROVEN|slot 3; not in area -> EnemyBaseAI_GoHome; motion end and distance > 30 -> ChaseTarget
8017F50C|EnemyAIState_BkSoldierAI_GoHome::Enter|enemy_ai|PROVEN|state vt 0x8052EDDC slot 2; home := SET pos
8017F374|EnemyAIState_BkSoldierAI_GoHome::Update|enemy_ai|PROVEN|slot 3; in area -> Caution; XZ < 10 -> AI slot 0xD
8017F2EC|EnemyAIState_BkSoldierAI_AppearHome::Enter|enemy_ai|PROVEN|state vt 0x8052EDB0 slot 2
8017F258|EnemyAIState_BkSoldierAI_AppearHome::Update|enemy_ai|PROVEN|slot 3; XZ < 10 -> BkSoldierAI_StartCombat
8019D894|EnemyBaseAI::EnterInitialState|enemy_ai|PROVEN|RTTI vf0C (AI slot 0xC); default = call AI slot 0xD
8019D070|EnemyBaseAI::OnDead|enemy_ai|PROVEN|RTTI vf14 (AI slot 0x14); ChangeState(EnemyBaseAI_Dead) unless locked; used by BkSoldier/EggPawn
8019CF08|EnemyBaseAI::OnRecovered|enemy_ai|PROVEN|RTTI vf16 (AI slot 0x16); unlock; ChangeState(EnemyBaseAI_Recover); RecoverCommand 0x8019E600
8019D8C0|EnemyBaseAI::ToIdle|enemy_ai|PROVEN|RTTI vf0B (AI slot 0xB); ChangeState(Idle); CharCommand 0x201
8019DA1C|EnemyBaseAI::GetAttackPoint|enemy_ai|LIKELY|RTTI vf05 (AI slot 5); nearest player + camera NDC tests (fn_8000F57C); used as charge/hold target by Pawn and BkSoldier attacks
8019BC30|EnemyAIState_EnemyBaseAI_Dead::Enter|enemy_ai|PROVEN|state vt 0x8053B7D4 slot 2; lock; death motion = param slot 0x16; timer 0.5 s
8019BB58|EnemyAIState_EnemyBaseAI_Dead::Update|enemy_ai|PROVEN|slot 3; timer<0 or motion flag A -> Disp slot 0x11 death effect + SB slot 2 RequestDelete
8019BB48|EnemyAIState_EnemyBaseAI_Dead::Exit|enemy_ai|PROVEN|slot 4; unlock
8019BA2C|EnemyAIState_EnemyBaseAI_Recover::Enter|enemy_ai|PROVEN|state vt 0x8053B7A8 slot 2; S bit 0xD; status slot 0x1D(0;1) (collision off LIKELY); timer 60 s
8019B934|EnemyAIState_EnemyBaseAI_Recover::Update|enemy_ai|PROVEN|slot 3; timer<0 or WasDamaged -> AI slot 0xD
8019B8BC|EnemyAIState_EnemyBaseAI_Recover::Exit|enemy_ai|PROVEN|slot 4; status slot 0x1D(1;1) (collision on LIKELY); clear S bit 0xD
8019BF90|EnemyAIState_EnemyBaseAI_GoHome::Enter|enemy_ai|PROVEN|state vt 0x8053B858 slot 2; home := SET pos
8019BF30|EnemyAIState_EnemyBaseAI_GoHome::Update|enemy_ai|PROVEN|slot 3; Enemy::MoveTowardHome arrived -> AI slot 0xD; no sensor test
8019C944|EnemyAIState_EnemyBaseAI_WarpWait::Enter|enemy_ai|PROVEN|state vt 0x8053B908 slot 2; alpha 0; status slot 0x1D(0;0) (collision off LIKELY); status slot 0x22(0); S bit 0xA; timer 0.5 s; lock
8019C8D4|EnemyAIState_EnemyBaseAI_WarpWait::Update|enemy_ai|PROVEN|slot 3; timer<0 -> unlock; AI slot 0xC (initial state)
8019B0C8|Enemy::MoveTowardHome|enemy|STRONG|RTTI Enemy::vf01; XZ dist to E+0x6C < 10 -> return 1; else steer/move toward home
8019AFB0|Enemy::TurnToTargetYaw|enemy|LIKELY|RTTI Enemy::vf02; rotates yaw toward E+0x88; returns aligned (< 1 deg)
801A09A8|EnemyDispCommon::PlayDeathEffect|enemy|STRONG|RTTI vf11 (Disp slot 0x11); team 0 (type!=0): effect 0x72 + SE 0x6007 (type 2: 0x6008); team 1: SE 0x6007 + 0x72; team 2: SE 0x6009 + effect 0x78
8019B2C0|Enemy_DetectNearestPlayer|enemy|STRONG|nearest player (fn_800742BC/fn_800A8E64); EnemySensor_IsInSearchArea; stores player ref at E+0x40; used by EnemyWalkerAI wait states
8019AE98|Enemy_PickRandomPointNear|enemy|STRONG|random distance rand(0.5r;r) at random angle; clamped into MoveRange (EnemySensor_IsInsideMoveRange); EggPawn WeaponAttack r=30
8019A85C|EnemyWalkerAI::OnTargetFound|enemy_ai|PROVEN|RTTI vf0F; ChangeState(EnemyWalkerAI_Caution)
8019A894|EnemyWalkerAI_GetState_Caution|enemy_ai|PROVEN|static EnemyAIState_EnemyWalkerAI_Caution at 0x805F04B4
80199E0C|EnemyAIState_EnemyWalkerAI_Caution::Enter|enemy_ai|PROVEN|state vt 0x8053B3FC slot 2; motion param 0x13; timer 1 s
80199D28|EnemyAIState_EnemyWalkerAI_Caution::Update|enemy_ai|PROVEN|slot 3; not in search area -> AI slot 0xD; timer<0 -> AI slot 0x10
8019A60C|EnemyAIState_EnemyWalkerAI_WaitAction_Standing::Enter|enemy_ai|PROVEN|state vt 0x8053B454 slot 2
8019A300|EnemyAIState_EnemyWalkerAI_WaitAction_Standing::Update|enemy_ai|PROVEN|slot 3; Enemy_DetectNearestPlayer -> AI slot 0xF
8019A218|EnemyAIState_EnemyWalkerAI_WaitAction_Moving::Enter|enemy_ai|PROVEN|state vt 0x8053B428 slot 2
80199E90|EnemyAIState_EnemyWalkerAI_WaitAction_Moving::Update|enemy_ai|PROVEN|slot 3; Enemy_DetectNearestPlayer -> AI slot 0xF; patrol points
8019A7D8|EnemyAIState_EnemyWalkerAI_ReturnHome::Enter|enemy_ai|PROVEN|state vt 0x8053B4AC slot 2; home := SET pos
8019A77C|EnemyAIState_EnemyWalkerAI_ReturnHome::Update|enemy_ai|PROVEN|slot 3; Enemy::MoveTowardHome arrived -> AI slot 0xD
8019A6FC|EnemyAIState_EnemyWalkerAI_ReturnHomeDash::Enter|enemy_ai|PROVEN|state vt 0x8053B480 slot 2; dash motion (primary slot 0x254)
8019A6A0|EnemyAIState_EnemyWalkerAI_ReturnHomeDash::Update|enemy_ai|PROVEN|slot 3; arrived -> AI slot 0xD
8017BD90|EggPawn_Create|enemy|PROVEN|SET desc 0x8052D4C4 hook+0C; new(0x3B0); EggPawn ctor on layer 0xE
8017BDE8|EggPawn_ReleaseResources|enemy|PROVEN|desc hook+08; ResourceManager slot 0x13 (type 12 + 7)
8017BE10|EggPawn_LoadResources|enemy|PROVEN|desc hook+04; enemy/EggPawnData.one -> slot 0x13
8017BCA8|EggPawn_GetSetParamLabel|enemy|STRONG|dataptr; param idx 8 table 0x8052D048 (Weapon); idx 9 0x8052D038 (AppearType); idx 10-12 OffsetPos; else EggPawn_GetSetParamLabel2
8017DBC4|EggPawn_GetSetParamLabel2|enemy|LIKELY|label tables 0x8052D058 (ActionType NONE/ATTACK/HIDE) and 0x8052E3E0 (WaitActMoveType STAND/LINEAR/TRIANGLE/RANDOM)
8017D7C0|EggPawnBase::InitFromSet|enemy|PROVEN|RTTI vf16 (SB slot 6); SetSlot_GetParams(E+0x274;0x6C) unclamped; OffsetPos; HaveShield -> E+0x270; WARP -> E+0x2EC=1.0
8017C9A4|EggPawnBase::ReinitFromSet|enemy|LIKELY|RTTI vf17 (SB slot 5); re-runs InitFromSet + Disp slot 0xC + AI slot 6; no caller found
8017C5C4|EggPawnBase::UpdateActivation|enemy|PROVEN|RTTI vf18 (Status slot 0xC); AppearType != 0 hidden+inactive until detected; E+0x2EC delay; Disp slot 0x12 once
8017C168|EggPawnBase::TakeHit|enemy|PROVEN|RTTI vf19 (Status slot 0x1A); shield E+0x2E4 block/absorb (SE 0x6074/0x6075); ApplyHit
8017D4BC|EggPawnBase::OnDeath|enemy|STRONG|RTTI vf15 (Status slot 0x26); vf26 collision; release shield; EnemyWeapon_Drop; E+0x2E0=0; E+0x294=0
8017D880|EggPawnBase::GetScoreId|enemy|PROVEN|RTTI vf14 (Status slot 0x2C); Enemy_IsEggmanSpecificStage ? 0x69/0x6A : 0x63/0x64 by HaveShield
8017D09C|EggPawnBase::InitDisplay|enemy|PROVEN|RTTI vf23 (Disp slot 0); shield model; EnemyWeapon kind 0x804C7374[WeaponType] -> E+0x2E0
8017CE84|EggPawnBase::UpdateWeaponAndSound|enemy|STRONG|RTTI vf0B (Disp slot 2); weapon placement/alpha; loop SE 0x601A
8017CC80|EggPawnBase::OnMotionEvent|enemy|STRONG|RTTI vf26 (Disp slot 0xE); event 0x5000 data 2 -> fn_801AB790 (weapon fire)
8017CCE8|EggPawnBase::OnMotionSoundEvent|enemy|LIKELY|RTTI vf25 (Disp slot 0xF); motion events 0xB/0xC/0x22/0x23 -> SE 0x6017/0x6018 and effect
8017CA4C|EggPawnBase::GetMoveSpeed|enemy|STRONG|RTTI vf22 (param slot 6); EnemyParamCommon::vf06 x2 while current motion == dash motion (vf13 = 4)
8017CAC0|EggPawnBase::GetDashMotion|enemy|PROVEN|RTTI vf13; returns 4
8017C780|EggPawnBase::GetPatrolMode|enemy|PROVEN|RTTI vf12; returns WaitActMoveType E+0x2A8
8017C788|EggPawnBase::GetPointWaitMotion|enemy|STRONG|RTTI vf11; point ActionType 1 -> attack motion (vf1E) else wait motion (vf1A)
8017C848|EggPawnBase::GetPatrolPoint|enemy|PROVEN|RTTI vf0F; point 0 = SET pos; 1/2 = SET pos + (PosN_X;PosN_Z)
8017C938|EggPawnBase::GetPointActionTime|enemy|PROVEN|RTTI vf0E; Pos0/1/2_ActionTime (params 14/19/24)
8017C95C|EggPawnBase::GetPointActionType|enemy|PROVEN|RTTI vf0D; Pos0/1/2_ActionType (params 15/20/25)
8017C980|EggPawnBase::GetPointMoveSpeedRatio|enemy|PROVEN|RTTI vf0C; Pos0/1/2_MoveSpeedRatio (params 16/21/26); no caller found
801A23EC|Enemy_IsEggmanSpecificStage|enemy|PROVEN|stage id [0x8057E808] in {300 HorrorCastle; 402 canyon2; 403 eWorld}; selects EggPawn*Specific / EggRecoverySpecific score ids
8017ECD0|EggPawnAI::EnterInitialState|enemy_ai|PROVEN|RTTI vf0C; AppearType 0 -> Standing/Moving by WaitActMoveType; 1/2 -> ReturnHome; 3 -> ReturnHomeDash
8017EB58|EggPawnAI::ReturnToWait|enemy_ai|PROVEN|RTTI vf0D; WaitActMoveType 0 -> Standing else Moving
8017E8D4|EggPawnAI::StartAttack|enemy_ai|PROVEN|RTTI vf10 (AI slot 0x10 from EnemyWalkerAI_Caution); WeaponType 0 DashAttack; 3 LANCE TukiAttack; else WeaponAttack
8017E954|EggPawnAI_GetState_WeaponAttack|enemy_ai|PROVEN|static at 0x805F04EC
8017EA00|EggPawnAI_GetState_TukiAttack|enemy_ai|PROVEN|static at 0x805F04E0
8017EAAC|EggPawnAI_GetState_DashAttack|enemy_ai|PROVEN|static at 0x805F04D4
8017E1F8|EnemyAIState_EggPawnAI_WeaponAttack::Enter|enemy_ai|PROVEN|state vt 0x8052E88C slot 2; attack motion
8017DF84|EnemyAIState_EggPawnAI_WeaponAttack::Update|enemy_ai|PROVEN|slot 3; fire; reposition 15-30 u (2 s); face target; not in area -> AI slot 0xD
8017E508|EnemyAIState_EggPawnAI_TukiAttack::Enter|enemy_ai|PROVEN|state vt 0x8052E8B8 slot 2; charge motion 0xC/0xD; target point AI vf05; timer 3 s
8017E270|EnemyAIState_EggPawnAI_TukiAttack::Update|enemy_ai|PROVEN|slot 3; distance < 15 or arrived -> thrust motion 0xE; pause 1 s; not in area -> AI slot 0xD
8017E7F8|EnemyAIState_EggPawnAI_DashAttack::Enter|enemy_ai|PROVEN|state vt 0x8052E8E4 slot 2
8017E5E8|EnemyAIState_EggPawnAI_DashAttack::Update|enemy_ai|PROVEN|slot 3; charge <= 3 s; pause 1 s; not in area -> AI slot 0xD
80193DD4|GunBeetle_Create|enemy|PROVEN|SET desc 0x80537158 hook+0C; new(0x470); GunBeetle ctor on layer 0xE
80193E2C|GunBeetle_ReleaseResources|enemy|PROVEN|desc hook+08; ResourceManager slot 8 (type 1 + 7)
80193E54|GunBeetle_LoadResources|enemy|PROVEN|desc hook+04; enemy/GunBeetleData.one -> slot 8
80194328|GunBeetleBase::InitFromSet|enemy|PROVEN|RTTI vf1D (SB slot 6); SetSlot_GetParams(E+0x310;0x48); FlyerCommonBase_ReadSetParams
80194378|GunBeetleBase::ReinitFromSet|enemy|LIKELY|RTTI vf1C (SB slot 5); re-init; rebuild weapon mount; model reload when BodyType changed
80193B44|FlyerCommonBase_ReadSetParams|enemy|STRONG|path mode (UsesPath): EnemyPath_Setup(pathId=param9; PathMirror); speed = MoveSpeedRatio x param vf06; attack window; else float pattern/FloatWidth
80193CAC|FlyerCommonBase::UpdateActivation|enemy|PROVEN|RTTI vf0B; path flyers hidden+inactive until EnemySensor_IsInSearchArea; others active
801942A4|GunBeetleBase::UpdateActivation|enemy|PROVEN|RTTI vf0B (Status slot 0xC); FlyerCommonBase::UpdateActivation + weapon hide/show by IsActive
80194A90|GunBeetleBase::TakeHit|enemy|PROVEN|RTTI vf11 (Status slot 0x1A); while spark state 2 ignore hits without attack flag 0x10; else ApplyHit
80194984|GunBeetleBase::OnDeath|enemy|STRONG|RTTI vf13 (Status slot 0x26); vf26 collision; EnemyWeapon_Drop velocity (0;0;5); delete weapon mount
80194A78|GunBeetleBase::GetScoreId|enemy|PROVEN|RTTI vf12 (Status slot 0x2C); BodyType ? 0x52 GunBeetleG : 0x51 GunBeetle
80194258|GunBeetleBase::GetMaxHP|enemy|PROVEN|RTTI vf14 (param slot 0x19 override); BodyType 0 -> 2.0 else 4.0
80194848|GunBeetleBase::InitDisplay|enemy|PROVEN|RTTI vf1F (Disp slot 0); WeaponType != 4 -> GunBeetle_CreateWeaponMount; WeaponType 2 -> mount+0x18 = SparkDischarge
80194538|GunBeetleBase::UpdateWeaponAndSpark|enemy|STRONG|RTTI vf20 (Disp slot 2); mount update; hover SE 0x6036; ActionType 1 spark cycle (SparkWait/SparkDischarge; status slot 0x1F; effect 0x7E; SE 0x603A)
801948DC|GunBeetleBase::LoadBodyModel|enemy|LIKELY|RTTI vf1E (Disp slot 0x17); model resource selected by BodyType (fn_802CE060)
801849D4|GunBeetleBase::UsesPath|enemy|PROVEN|RTTI vf0C (primary slot 0x238); AppearType == 1
80193DC0|GunBeetleBase::HasAttackWindow|enemy|PROVEN|RTTI vf0D (primary slot 0x23C); ActionType == 2
8019428C|GunBeetleBase::StartFiring|enemy|PROVEN|RTTI vf0F (primary slot 0x244); weapon mount+0x10 = 1
80194274|GunBeetleBase::StopFiring|enemy|PROVEN|RTTI vf10 (primary slot 0x248); weapon mount+0x10 = 0
80195038|GunBeetle_CreateWeaponMount|enemy|STRONG|new(0x1C); +8 -> 0x804CAD20+0xC*type; EnemyWeapon kind 0x804CAD5C[type] (3;0x13;0;2) if nonzero
80194DBC|GunBeetle_UpdateWeaponMount|enemy|LIKELY|per frame; firing -> fn_801AB790 or (kind 0) spawns fn_80209EDC object on layer 0xE + SE 0x6037
80193814|FlyerCommon_AdvanceOnPath|enemy|STRONG|E+0x2E4 += speed*dt; sample path fn_801A6944; velocity E+0x300 clamped; attack-window aiming
80193AD4|FlyerCommon_IsInAttackWindow|enemy|PROVEN|HasAttackWindow && E+0x2E8 <= E+0x2E4 <= E+0x2EC
801A6A44|EnemyPath_Setup|enemy|STRONG|path follower init: entry EnemyPath_GetEntry(pathId) of resource slot 6 (EnemyPath.one); mirror transform when PathMirror
801A6B4C|EnemyPath_GetEntry|enemy|PROVEN|index table 0x804CD8D8 {2..10}[pathId] -> fn_8001277C(mgr; slot 6; idx)
801A238C|EnemyPath_LoadResources|enemy|PROVEN|hook+04 of SET marker 0x258E EnemyEntryStart (desc 0x8053BAB0); enemy/EnemyPath.one -> ResourceManager slot 6
80184318|FlyerCommonAI::OnDead|enemy_ai|PROVEN|RTTI vf14 (AI slot 0x14); Disp slot 0x11 death effect + SB slot 2 RequestDelete immediately (no Dead state)
80184914|GunBeetleAI::ReturnToWait|enemy_ai|PROVEN|RTTI vf0D; UsesPath ? FlyerCommonAI_MoveOnPath : FlyerCommonAI_WaitFloating
801848DC|GunBeetleAI::OnTargetFound|enemy_ai|PROVEN|RTTI vf0F; ChangeState(GunBeetleAI_Caution)
80184684|GunBeetleAI_GetState_Caution|enemy_ai|PROVEN|static at 0x805F0624
8018481C|GunBeetleAI_GetState_Attack|enemy_ai|PROVEN|static at 0x805F0630
801848C8|EnemyAIState_GunBeetleAI_Caution::Enter|enemy_ai|PROVEN|state vt 0x80530500 slot 2; timer 1 s
80184784|EnemyAIState_GunBeetleAI_Caution::Update|enemy_ai|PROVEN|slot 3; aim at target+(0;10;0); timer<0 -> Attack
80184730|EnemyAIState_GunBeetleAI_Attack::Enter|enemy_ai|PROVEN|state vt 0x805304D4 slot 2; timer 3 s; StartFiring
80184574|EnemyAIState_GunBeetleAI_Attack::Update|enemy_ai|PROVEN|slot 3; not in search area -> WaitFloating; timer<0 -> Caution
80184540|EnemyAIState_GunBeetleAI_Attack::Exit|enemy_ai|PROVEN|slot 4; StopFiring
80184270|EnemyAIState_FlyerCommonAI_WaitFloating::Enter|enemy_ai|PROVEN|state vt 0x80530070 slot 2
80183F30|EnemyAIState_FlyerCommonAI_WaitFloating::Update|enemy_ai|PROVEN|slot 3; in area: turn; HasAttackWindow -> AI slot 0xF; else hover pattern (FloatWidth)
80183E8C|EnemyAIState_FlyerCommonAI_MoveOnPath::Enter|enemy_ai|PROVEN|state vt 0x80530044 slot 2; E+0x30C=1
80183CC4|EnemyAIState_FlyerCommonAI_MoveOnPath::Update|enemy_ai|PROVEN|slot 3; attack window -> Start/StopFiring; FlyerCommon_AdvanceOnPath
80183CB0|EnemyAIState_FlyerCommonAI_MoveOnPath::Exit|enemy_ai|PROVEN|slot 4; E+0x30C=0
801B4C74|Vehicle_Create|vehicle|PROVEN|kind table 0x8053C890[kind] create fn; Vehicle::CharaColliMove + Vehicle::VehicleTask on layer 0xC; kind 7 = Vehicle::AirSaucer (fn_801AF9EC)
801B4EFC|Vehicle_IsKindAvailable|vehicle|PROVEN|returns u8 0x8053C890[kind].enabled
80169430|LinkEvent_Get|set|PROVEN|returns table[link*8] (0..0xFF) or 0; 59 call sites in gimmicks (Door; Elevator; SetGenerator; ...)
801694B4|LinkEvent_SetValue|set|PROVEN|table[link*8] = value without touching the flag word
800CBA84|LinkEventTable_Get|set|PROVEN|lazy singleton sbss 0x805EF7A0 (fn_800CBAB8); arg of LinkEvent_Set/Get
8027CBD0|SetGenerator_UpdateState|set|STRONG|state machine at obj+0; finds target slot (FindArgs; predicate fn_8027CFB8); re-arms record flag 0x40; sends GenerateCommand via fn_800C9B24; num counter +0xC; delay +0x10
8027CFB8|SetGenerator_MatchSlot|set|LIKELY|predicate for fn_800CB864: record flag 0x40000 -> store slot in args+8
801A8864|EnemyStatusCommon::OnKnockback|enemy|LIKELY|default nop for Status slot 0x2E (mislabelled BkChaos::vf2E); called by EnemyStatusCommon::Update 0x801A8B08 when S+0x75
8017BC80|EnemyStatusCommon::SuppressLifeReport|enemy|LIKELY|Status slot 0x2F default returns 0 (mislabelled BkChaos::vf2F); nonzero would skip ReportLifeEvent(1) at the death edge
"""


def main():
    curated = set()
    with open(os.path.join(GP, 'symbols_curated.csv'), newline='', encoding='utf-8') as f:
        for r in csv.reader(f):
            if r and r[0] != 'address':
                curated.add(r[0].strip().upper())
    out, seen, skipped = [], set(), []
    for line in ROWS.strip().splitlines():
        a, n, s, c, e = [x.strip() for x in line.split('|')]
        a = a.upper()
        if a in seen:
            continue
        seen.add(a)
        if a in curated:
            skipped.append((a, n))
            continue
        out.append((a, n, s, c, e.replace(',', ';')))
    out.sort()
    path = os.path.join(GP, 'notes', 'enemy2_symbols.csv')
    with open(path, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['address', 'current_name', 'subsystem', 'confidence', 'evidence'])
        w.writerows(out)
    print('written %d rows to %s; skipped (already curated): %s' % (len(out), path, skipped))


if __name__ == '__main__':
    main()
