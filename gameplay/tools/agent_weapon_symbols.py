"""Build gameplay/notes/weapon_symbols.csv (weapons / damage / targeting / vehicles investigation).

  python agent_weapon_symbols.py          write ../notes/weapon_symbols.csv and print a summary

Rows = hand-curated rows below + rows generated from the weapon catalog/registry (create / item / resource hooks
named after the game's own "%s::Initialize" names) + the targeting and vehicle sub-investigation rows (embedded
verbatim). Duplicates: first occurrence wins (order: manual, generated, targeting, vehicle). Addresses already present
in ../symbols_curated.csv are skipped. Commas inside evidence must already be ';'.
Vehicle rows are imported from agent_weapon_veh_symbols.ROWS.
"""
import csv
import io
import os
from agent_weapon_catalog import rows as catalog_rows, word
from dol import get_dol

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, '..', 'notes', 'weapon_symbols.csv'))
CURATED = os.path.normpath(os.path.join(HERE, '..', 'symbols_curated.csv'))

MANUAL = """
800C6A4C,CharaColli_DispatchAttackPair,damage,PROVEN,both directions: attacker+0x54 && target+0x55 && level(+0x38)>defense(+0x3C) (0x800C6C30) -> param.power=+0x34; invokes lists +0x10/+0x1C/+0x28
800C6608,CharaColli_DispatchAttackOneWay,damage,LIKELY,same enable/level test and param build for one contact; callers are shape tests 0x800C44AC..0x800C615C
800C69E8,SonicteamUSA::System::CharaColli::AttackCallbackParam::AttackCallbackParam,damage,PROVEN,zeroes +0x34..+0x50 and +0x54; smart ptrs +0/+8; stack object of the dispatchers
800C698C,SonicteamUSA::System::CharaColli::AttackCallbackParam::~AttackCallbackParam,damage,LIKELY,called with -1 at the end of both dispatchers
800C694C,AttackCallbackParam_SetColliRef,damage,STRONG,copies a shared_ptr<CharaColliAttack> into param+0 (attacker) / param+8 (target)
800C6920,AttackCallbackList_Invoke,damage,STRONG,calls fn_80078B9C(list;param) when list non-empty
800C4414,AttackCallbackList_IsEmpty,damage,PROVEN,returns first word == 0
800C6FE0,CharaColli_GetContactPoint,damage,STRONG,contact+0x14 position or contact+0xC+4; default zero; -> param+0x18
800C6F6C,CharaColli_GetContactNormal,damage,STRONG,contact+0x14 normal; default (0;1;0); -> param+0x24
8005A968,SonicteamUSA::System::CharaColli::CharaColliAttack::CharaColliAttack,damage,PROVEN,0x58 bytes; lists +0x10/+0x1C/+0x28; power 0; level 3; defense 0; attr 0; +0x54/+0x55=0; sp_counted_base_impl<CharaColliAttack*> at 8005A934
800C475C,CharaColliAttack_CreateForColli,damage,PROVEN,dynamic_cast colli to CharaColliBaseCharInfo; new(0x58) CharaColliAttack stored as weak ptr colli+0x38
80054EF8,CharaColliAttack_SetAttack,damage,PROVEN,(attack;level r4;attr r5;onAttack r6;after r7;power f1): +0x54=1 +0x38 +0x40 +0x34; binds lists +0x10/+0x28
80096E54,CharaColliAttack_SetDefense,damage,PROVEN,(attack;defense r4;onDamaged r5): +0x55=1 +0x3C; binds list +0x1C
8005AFD4,CharaColliAttack_IsIgnored,damage,LIKELY,walks attack+0x44 weak-ref list; true skips the pair in the dispatchers
8005A428,Bullet_CreateCharaColliAttack,damage,PROVEN,new CharaColliAttack; SetAttack(level 5;attr 0x40;power f1); owner weak ref at +8; used by BulletGun/Cannon/Homing/Laser/ChaosSpear/ThrowObject
8005A350,Bullet_ComputeLaunchVelocity,weapon,STRONG,dir=normalize(shot+0x1C); velocity=dir*(f1+max(0;dot(shot+0x28 shooter vel;dir)))
80095F14,PlayerCharaColli_OnDamaged,damage,PROVEN,bound {0;-1;80095F14} at 80521D64; team/proxy filters (result 8); power>0 -> DamageCommand(dir param+0x34;prio 4;power 10.0) result 1
80096C44,PlayerCharaColli_Setup,damage,STRONG,player body collision; CharaColliAttack_SetDefense(defense 0;PlayerCharaColli_OnDamaged)
80078098,PlayerAttack_ApplyType,damage,PROVEN,row = 8052124C + 0x14*type: attach id; radius; attr; power; mode -> SetAttack(level 3;attr;power) @80078324
800788A0,PlayerAttack_SetType,damage,PROVEN,impl+0x20=type (if changed) then PlayerAttack_ApplyType
80077A84,Player::Attack::SetAttackType,damage,PROVEN,wrapper on +0x28 impl; callers: Jump/Fall/HomingJump/Launch 4; JumpDash/LightDash/SpinDash 5; Sliding 6; HomingAttack 7; ChaosBlast 0xA; ChaosControl 0xB
80077AA8,Player::Attack::SetComboAttack,damage,STRONG,event 0..6 via jump table 80521370 -> types 0;1;2;3;4;8;9; +0x30 scale; caller motion-event handler fn_800AA4BC
80077A3C,Player::Attack::SetRadius,damage,STRONG,wrapper -> PlayerAttackImpl_SetRadius; ChaosBlast grows 10->200
80078830,PlayerAttackImpl_SetRadius,damage,STRONG,colli vslot 0x10 (+0x48)(-2;r;..)
80077A34,Player::Attack::GetImpl,damage,LIKELY,returns [this+0]
801DBC04,DamageCommand::DamageCommand,damage,PROVEN,second ctor (this;priority r4;power f1); direction = zero vector 805E1860; used by PlayerController_CheckDamageContact/Grind::Update/fn_8012C928
80055EB4,BulletGun_OnAttack,damage,PROVEN,param+0x34 = normalize(bullet+0x10 velocity); param+0x4C = bullet+0x44 (500)
80055E28,BulletGun_OnAttackResult,damage,PROVEN,contact+0x38 = result; results 1/3/5 -> bullet+0x48=1 and contact+0x34=1 (jump table 8051ED88)
80055F00,BulletGun_SpawnImpactEffect,weapon,STRONG,switch bullet type +0x4C (table 8051EDAC); type 3 spawns CannonBomb type 6
80056170,BulletGun_UpdateFlight,weapon,PROVEN,+0x38 += travelled distance; terrain ray fn_8005B10C; delete when +0x38 >= +0x40 range
800566C4,Weapon::Bullet::BulletGun::BulletGun,weapon,PROVEN,(this;shot;effect r5;type r6;power f1;range f2;speed f3); power -> Bullet_CreateCharaColliAttack
80056BB0,BulletGunTable_StaticInit,weapon,PROVEN,.ctors; writes range +0x10 of rows 8051EC54..8051ED58; 805EF2F4=500 speed; 805EF2F8=300
80055C54,BulletCreate_Gun_HandGun,weapon,PROVEN,BulletGun(power 2;range 8051EC64;type 0;effect 0xE2); bullet-type row 8051EC54 (idx 1)
80055BF8,BulletCreate_Gun_SubMachineGun,weapon,PROVEN,power 2; type 0; effect 0x1C3; row 8051EC68 (idx 2)
80055B9C,BulletCreate_Gun_AutoRifle,weapon,PROVEN,power 4; row 8051EC7C (idx 3)
80055B40,BulletCreate_Gun_AntiAircraftRifle,weapon,PROVEN,power 6; row 8051EC90; also the factory of row 8051ECA4 (Vulcan idx 5)
80055AE4,BulletCreate_Gun_EggGun,weapon,PROVEN,power 2; effect 0x185; row 8051ECB8 (idx 7)
80055A88,BulletCreate_Gun_LightShot,weapon,PROVEN,power 2; type 1; row 8051ECCC (idx 8)
80055A2C,BulletCreate_Gun_FlashShot,weapon,PROVEN,power 2; type 1; row 8051ECE0 (idx 9)
800559D0,BulletCreate_Gun_HeavyShot,weapon,PROVEN,power 5; type 1; effect 0x282; row 8051ECF4 (idx 11)
80055974,BulletCreate_Gun_GunBattery,weapon,PROVEN,power 6; type 2; range 2000; row 8051ED08 (idx 28)
80055918,BulletCreate_Gun_BKBattery,weapon,PROVEN,power 8; type 3 (impact CannonBomb 6); range 2000; row 8051ED1C (idx 29)
800558BC,BulletCreate_Gun_Katana01,weapon,PROVEN,power 4; type 5; range 100; row 8051ED30 (idx 57)
80055860,BulletCreate_Gun_Katana02,weapon,PROVEN,power 8; type 6; range 200; row 8051ED44 (idx 58)
80055804,BulletCreate_Gun_ShadowRifle,weapon,PROVEN,power 32; type 7; range 600; row 8051ED58 (idx 67)
80053FC0,Weapon::Bullet::Cannon::Cannon,weapon,PROVEN,(this;shot;type r5); table 8051E9A0[type]: power -> Bullet_CreateCharaColliAttack @8005414C; +4 -> +0x3C
80054534,CannonTable_StaticInit,weapon,PROVEN,.ctors; cannon +4 speeds 200/300/300/200/200/300/9990/300/300; bullet-type ranges 8051EA34..8051EAC0
80052C8C,BulletCreate_Cannon_Grenade,weapon,PROVEN,Cannon type 0 (power 4); row 8051EA24 (idx 12)
80052C44,BulletCreate_Cannon_Bazooka,weapon,PROVEN,Cannon type 1 (power 8); row 8051EA38 (idx 13)
80052BFC,BulletCreate_Cannon_TankCannon,weapon,PROVEN,Cannon type 2 (power 16); row 8051EA4C (idx 14)
80052BB4,BulletCreate_Cannon_BlackBarrel,weapon,PROVEN,Cannon type 3 (power 4); row 8051EA60 (idx 15)
80052B6C,BulletCreate_Cannon_BigBarrel,weapon,PROVEN,Cannon type 4 (power 8); row 8051EA74 (idx 16)
80052B24,BulletCreate_Cannon_EggBazooka,weapon,PROVEN,Cannon type 5 (power 8); row 8051EA88 (idx 17)
80052ADC,BulletCreate_Cannon_HealCannon01,weapon,PROVEN,Cannon type 7 (power 0); row 8051EA9C (idx 65)
80052A94,BulletCreate_Cannon_HealCannon02,weapon,PROVEN,Cannon type 8 (power 0); row 8051EAB0 (idx 66)
80053138,Cannon_Explode,weapon,PROVEN,types 7/8 -> HealingBlast fn_80107448(level 0/1) + SE 0xE02E; else CannonBomb_Create(type)
800545D0,CannonBomb_Create,weapon,PROVEN,new(0x58) Weapon::Bullet::CannonBomb(shell;type;..)
80055360,Weapon::Bullet::CannonBomb::CannonBomb,weapon,PROVEN,table 8051EB48[type] stride 0x14: power; radius; SE; effect; 0.5
80054B04,CannonBomb_CreateAttack,damage,PROVEN,SetAttack(level 5;attr 0x200;power table+0) @80054D14
80055564,CannonBombTable_StaticInit,weapon,PROVEN,.ctors; +4 = 30/50/100/40/70/50/40/0/0 (radius LIKELY)
80057E24,Weapon::Bullet::Homing::Homing,weapon,PROVEN,(this;shot;kind r5;row r6); power .rodata 804AD4C0[kind]+0x10 -> Bullet_CreateCharaColliAttack @80058190; attr |= 0x800
80059510,BulletCreate_Ring_RingShot,weapon,PROVEN,Ring(power 4;speed 400;effect 0x123); row 8051EFD0 (idx 10)
8005A164,Weapon::Bullet::Ring::Ring,weapon,PROVEN,(this;shot;effect r5;power f1;speed f2)
80059984,Ring_CreateAttack,damage,PROVEN,SetAttack(level 5;attr 0x40;power) @80059BD4
8005A32C,RingTable_StaticInit,weapon,PROVEN,.ctors; bullet-type 8051EFE0 range = 400
802213A0,@unnamed@BulletLaser_cpp@::Laser::Laser,weapon,STRONG,writes vtbl .data 80547190; table 8054707C[type] stride 0x1C (power; speed; range)
80220BB8,BulletLaser_CreateAttack,damage,PROVEN,power = 8054707C[type]+0 (lfsx @80220CCC) -> Bullet_CreateCharaColliAttack
802217E0,LaserTable_StaticInit,weapon,PROVEN,.ctors; speed 1000/500/1000/300/300 and range 300/400/800/800/800
8022060C,BulletCreate_Laser_LaserRifle,weapon,PROVEN,Laser type 0 (power 3); bullet-type row 80547120 (idx 25)
802205C8,BulletCreate_Laser_Splitter,weapon,PROVEN,Laser type 1 (power 4); row 80547134 (idx 26)
80220584,BulletCreate_Laser_Reflector,weapon,PROVEN,Laser type 2 (power 5); row 80547148 (idx 27)
80220540,BulletCreate_Laser_OmochaoGun01,weapon,PROVEN,Laser type 3 (power 6); row 8054715C (idx 63)
802204FC,BulletCreate_Laser_OmochaoGun02,weapon,PROVEN,Laser type 4 (power 10); row 80547170 (idx 64)
802F2128,BulletCreate_ChaosSpear,weapon,PROVEN,new(0x64) Weapon::Bullet::ChaosSpear::ChaosSpear(shot;..;charge f1); caller ChaosSpear weapon vf0C
802F0CB8,Weapon_Create_ChaosSpear,weapon,PROVEN,only caller SuperShadow_Setup 0x8027F4E4
802F1504,Weapon::ChaosSpear::ChaosSpear::Update,weapon,PROVEN,RTTI vf02; charge +0x7C += dt clamp 3.0; >= 2.4 -> +0x80 and cue 0xF
802F1194,Weapon::ChaosSpear::ChaosSpear::FireCharged,weapon,PROVEN,RTTI vf0C (trigger release); charge = t/3 -> BulletCreate_ChaosSpear; power 6+14c
8027F3B8,SuperShadow_Setup,player,STRONG,caller Player::Shadow::Super::Super; flags 0x24/0x25 (stage 0x2C6)/0x33; equips ChaosSpear with 999 ammo via ShadowWeapon vslot 3
8027D968,ThrowObject_CreateAttack,damage,PROVEN,Bullet_CreateCharaColliAttack(power 6.0 805F91C8); ThrowObject ctors 8027D58C/8027D5A8
80068978,StickAttack_Construct,weapon,PROVEN,0x54 component of WeaponStickBase; +0x48 power; owner+0x5C==1 -> level 3 else 4 (+0x50)
80068060,StickAttack_CreateAttack,damage,PROVEN,SetAttack(level +0x50;attr 0x80;power +0x48) @800681B8
8023E7D4,Torch_CreateFireAttack,damage,PROVEN,SetAttack(level 5;attr 0x400;power 0.0 805F83FC)
8023F308,@unnamed@WeaponTorch_cpp@::Torch::Torch,weapon,STRONG,WeaponStickBase(power 1.0) + fire attack; created by Torch0300 factory 8023E0AC
80051BF0,BulletType_GetRange,weapon,PROVEN,table 8051E840[w]+0x10; =WeaponBase::vf05; BulletGun deletes after travelling this far
80051C18,BulletType_CreateAlt,weapon,PROVEN,table 8051E840[w]+0xC else fn_80051C80
80051D5C,BulletType_Spawn,weapon,PROVEN,table 8051E840[w]+8(shot)
80051DA8,BulletType_Release,weapon,PROVEN,table 8051E840[w]+4()
80051DF0,BulletType_Init,weapon,PROVEN,table 8051E840[w]+0(); called by each weapon resource hook with its index
8005B888,WeaponRegistry_MarkLoaded,weapon,PROVEN,registry 8051F058[w]+4 = 1; called by every weapon resource hook
8005B8A4,WeaponRegistry_ResetLoaded,weapon,PROVEN,clears +4 of all 72 rows and of list 8051F4D8 (0x44..0x47); WeaponDataEnd release hook
8005B944,WeaponRegistry_GetFlagC,weapon,PROVEN,returns row+0xC byte (1 ranged / 0 melee+special); single reader fn_801C0A1C
8005B95C,WeaponRegistry_GetFireRumble,weapon,PROVEN,row+8; ShadowWeapon::Update -> fn_8034ED50(pattern;pad) on shot frames
8005B974,WeaponRegistry_CreateExtra,weapon,PROVEN,info+0xC(arg); callers HeavyDog RocketLauncher; fn_800E9348; fn_800EB444; fn_80348BE0
8005B9C0,WeaponInfo_GetPickupColliSize,weapon,PROVEN,info+0x10/+0x14 (1.0/5.0) -> SetWeapon::Arms pickup collision
8005BA04,WeaponInfo_GetFireSlowdown,weapon,PROVEN,info+0x2C; Ground::Update multiplies vel and accel by (1-w) on shot frames
8005BA2C,WeaponInfo_GetRecoil,weapon,PROVEN,info+0x28; added to ShadowWeapon+0x40 on shot frames
8005BA54,WeaponInfo_GetHoldAngle,weapon,PROVEN,info+0x24; x90 deg -> ShadowWeapon+0x58 (arm IK)
8005BA7C,WeaponInfo_GetPickupSE,weapon,PROVEN,info+0x20 low 16 bits (0x2001..0x2004); ShadowWeapon::AddWeapon -> SoundManager_PlaySE
8005BAAC,WeaponInfo_GetHoldType,weapon,PROVEN,info+0x1C (1/2/3) -> ShadowMotion vslot 4 in ShadowWeapon_Equip
8005BAD4,WeaponInfo_GetDefaultAmmo,weapon,PROVEN,info+0x18; used when AddWeapon/Equip ammo < 0
8005BAFC,WeaponRegistry_CreateItem,weapon,PROVEN,(out;w;ammo): if row loaded: item = info+8(); item+8 = w; item.SetAmmo(ammo)
8005C014,WeaponRegistry_GetName,weapon,PROVEN,[info+0] else empty string 805F2EDC
8005C03C,WeaponRegistry_IsLoaded,weapon,PROVEN,row+4 byte
8005E428,WeaponDataStart_LoadResources,weapon,STRONG,hook04 of SET marker 0x2584; fn_8005B924 + SpecialWeapons_LoadAll
8005E394,WeaponDataEnd_ReleaseResources,weapon,STRONG,hook08 of SET marker 0x2585; WeaponRegistry_ResetLoaded
80275EEC,SpecialWeapons_LoadAll,weapon,PROVEN,loads Katana/Satellite/VacuumEgg/Omochao/HealCannon/ShadowRifle resources and marks idx 57..67
8033F77C,SpecialWeapon_SpawnUnlocked,weapon,STRONG,save bits (save+0x1710) pairs -> level -> table 8055EC98 {57..66}; bit 10 -> 67; ScatteringWeapon_Spawn
8005D21C,Weapon::WeaponBase::WeaponBase,weapon,PROVEN,(this;owner;index r5;kind r6): +0xC owner; +0x14 index; +0x18 kind; +0x1C model 0
8005D118,Weapon::WeaponBase::SetMuzzle,weapon,PROVEN,RTTI vf01; +0x20 = pos; +0x2C = dir
8005D08C,Weapon::WeaponBase::ShowModel,weapon,LIKELY,RTTI vf03; fn_80063258(+0x1C)
8005D060,Weapon::WeaponBase::HideModel,weapon,LIKELY,RTTI vf04; fn_8006327C(+0x1C)
8005D020,Weapon::WeaponBase::GetHoldOffset,weapon,LIKELY,RTTI vf06; constant (x;-1;-2.703) -> ShadowWeapon+0x4C
8005CFFC,Weapon::WeaponBase::GetRecoil,weapon,PROVEN,RTTI vf07; WeaponInfo_GetRecoil(+0x14)
8005CFD8,Weapon::WeaponBase::GetFireSlowdown,weapon,PROVEN,RTTI vf08; WeaponInfo_GetFireSlowdown(+0x14)
8005ECCC,Weapon::WeaponGunBase::BeginFrame,weapon,PROVEN,RTTI vf02; +0x38 (shots this frame) = 0
8005ECBC,WeaponGun_AddShots,weapon,PROVEN,+0x38 += n; read by controls for ammo and recoil
8005EE3C,HandGun_MuzzleEffect,weapon,STRONG,SE 0x2008 at muzzle; effects 0x147/0xD1
8006954C,BulletShot_BuildMuzzleForward,weapon,PROVEN,dir = muzzle rot * (0;0;1); team 0 if owner+0x5C==1 else 1
8005EF94,Weapon::HandGun::Fire,weapon,PROVEN,RTTI vf0A; spawn bullet; returns -1.0 (semi-automatic)
8005EF10,Weapon::HandGun::FireAt,weapon,PROVEN,RTTI vf0B; aimed shot; returns -1.0
80068D80,Weapon::SubMachineGun::Fire,weapon,PROVEN,RTTI vf0A; returns 1/12 s (805F32E0)
80068CFC,Weapon::SubMachineGun::FireAt,weapon,PROVEN,RTTI vf0B; returns 1/12 s
8005CE40,Weapon::AutoRifle::Fire,weapon,PROVEN,RTTI vf0A; returns 0.1 s (805F2F28)
8005CDBC,Weapon::AutoRifle::FireAt,weapon,PROVEN,RTTI vf0B; returns 0.1 s
8005C5A0,Weapon::AntiAircraftRifle::Fire,weapon,PROVEN,RTTI vf0A; returns 1/6 s (805F2EF4)
8005C464,Weapon::AntiAircraftRifle::FireAt,weapon,PROVEN,RTTI vf0B; returns 1/6 s
80069A34,Weapon::Vulcan::Fire,weapon,PROVEN,RTTI vf0A; returns 1/12 s (805F3328)
800699B0,Weapon::Vulcan::FireAt,weapon,PROVEN,RTTI vf0B; returns 1/12 s
801F4444,Weapon::EggGun::Fire,weapon,PROVEN,RTTI vf0A; returns -1.0 (805F7558)
801F43C0,Weapon::EggGun::FireAt,weapon,PROVEN,RTTI vf0B; returns -1.0
8005F7C4,Weapon::LightShot::Fire,weapon,PROVEN,RTTI vf0A; returns 0.5 s (805F308C)
8005F740,Weapon::LightShot::FireAt,weapon,PROVEN,RTTI vf0B; returns 0.5 s
8005E8EC,Weapon::FlashShot::Fire,weapon,PROVEN,RTTI vf0A; returns 1/12 s (805F301C)
8005E868,Weapon::FlashShot::FireAt,weapon,PROVEN,RTTI vf0B; returns 1/12 s
80064AD0,Weapon::RingShot::Fire,weapon,PROVEN,RTTI vf0A; returns 1.0 s (805F3104)
80064A4C,Weapon::RingShot::FireAt,weapon,PROVEN,RTTI vf0B; returns 1.0 s
802F1F94,Weapon::HeavyShot::Fire,weapon,PROVEN,RTTI vf0A; returns 0.125 s (805FA474)
802F1F10,Weapon::HeavyShot::FireAt,weapon,PROVEN,RTTI vf0B; returns 0.125 s
8005E1A4,Weapon::WeaponCannonBase::Fire,weapon,PROVEN,RTTI vf0A; spawn shell; +0x38++; returns 0.5 s (805F300C)
8005E13C,Weapon::WeaponCannonBase::CreateAltProjectile,weapon,LIKELY,RTTI vf0B; BulletType_CreateAlt(index) with f1 2.0; not called by ControlCannon
8005BD14,Weapon::Item::SetAmmo,weapon,PROVEN,RTTI vf01; +0xC = ammo (-1 = weapon default)
8023A490,ShadowWeapon_RemoveWeapon,weapon,PROVEN,release weapon/control; clear player flag 0x1F; HUD record +0x10/+0x14 = 0
8023A5CC,ShadowWeapon_CreateAndEquip,weapon,PROVEN,fn_8005BD1C(w) create via info+4 then ShadowWeapon_Equip(ammo)
8023A92C,ShadowWeapon_Equip,weapon,PROVEN,ammo; control by kind (1/6 gun;2 cannon;3 lockon;4 vacuum;5 stick); hold type/angle; IK bind; flag 0x1F; HUD record
8023ADAC,ShadowWeapon_UpdateMuzzleFromHand,weapon,STRONG,motion locator 5 -> weapon SetMuzzle
8023AF24,ShadowWeapon_SetAmmo,weapon,PROVEN,player record+0x1C = n (fn_8016F358)
8023B08C,ShadowWeapon_ThrowWeapon,weapon,PROVEN,spawns thrown pickup with remaining ammo (fn_80221E8C); SE 0x3025; RemoveWeapon
8023B6D0,Player::Shadow::ShadowWeapon::Update,weapon,PROVEN,RTTI vf01; weapon Update + control Update; recoil (+0x40) and fire rumble on shot frames; decay x0.859375
8023B620,Player::Shadow::ShadowWeapon::AddWeapon,weapon,PROVEN,RTTI vf02 (AddWeaponCommand); pickup SE; same weapon -> AddAmmo; else Remove + CreateAndEquip
8023B598,Player::Shadow::ShadowWeapon::EquipWeaponObject,weapon,PROVEN,RTTI vf03; same-index -> AddAmmo else Remove + Equip
8023B574,Player::Shadow::ShadowWeapon::GetWeapon,weapon,PROVEN,RTTI vf04; copies shared_ptr +0x34
8023B538,Player::Shadow::ShadowWeapon::GetWeaponIndex,weapon,PROVEN,RTTI vf05; weapon+0x14 or 0
8023B374,Player::Shadow::ShadowWeapon::ThrowWeaponIfAllowed,weapon,PROVEN,RTTI vf0B; unless flag 0x33: clear bit0/flag 0x19; ShadowWeapon_ThrowWeapon
8023B328,Player::Shadow::ShadowWeapon::DiscardWeapon,weapon,PROVEN,RTTI vf0C; clear bit0/flag 0x19; RemoveWeapon
8023B2F0,Player::Shadow::ShadowWeapon::HideWeaponModel,weapon,LIKELY,RTTI vf0D; weapon slot 4
8023B2B8,Player::Shadow::ShadowWeapon::ShowWeaponModel,weapon,LIKELY,RTTI vf0E; weapon slot 3
8023B288,Player::Shadow::ShadowWeapon::IsFiring,weapon,PROVEN,RTTI vf10; +0x30 bit 2
8023B260,Player::Shadow::ShadowWeapon::ShotFiredThisFrame,weapon,PROVEN,RTTI vf11; +0x30 bit 3; Ground::Update fire slowdown
8023BAB8,ControlCannon_Create,weapon,PROVEN,new(0x20) ControlCannon (ctor 8023C188) for weapon kind 2
8023C260,ControlGun_Create,weapon,PROVEN,new(0x18) ControlGun (ctor 8023CF48) for weapon kind 1/6
8023D4D4,ControlLockon_Create,weapon,PROVEN,new(0x18) ControlLockon (ctor 8023D80C) for weapon kind 3
8023D8E4,ControlStick_Create,weapon,PROVEN,new(0x10) ControlStick (ctor 8023DA8C) for weapon kind 5
8023DB54,ControlVacuum_Create,weapon,PROVEN,new(0x18) ControlVacuum (ctor 8023DF84) for weapon kind 4
8011F010,SetWeapon_Create,weapon,PROVEN,SET 0x0020 create hook: new(0x48) SetWeapon::SetWeaponTask on layer 0xB
8011F0C8,SetWeaponTask_ReadSetParams,weapon,PROVEN,pos/rot; param -> +0x40/+0x44 (weapon index) and model+0x34
8011F764,SetWeapon_OnCommand,weapon,PROVEN,UseCommand (0xD) from player -> AddWeaponCommand(index;ammo) and hide pickup
8011ED20,WeaponPickup_SpawnAtGround,weapon,STRONG,ray down 1000 then WeaponRegistry_CreateItem(w;-1) + pickup task; callers STICK (Stick::Stick::vf02) and fn_800DC878
8011FDC0,SetWeapon::Arms::Arms,weapon,LIKELY,0x1C object (checked_deleter<SetWeapon::Arms> typeinfo); model + pickup collision from WeaponInfo_GetPickupColliSize
8011EF70,SetWeapon_GetParamLabel,weapon,LIKELY,returns registry name or "Not Used"
8009BAD8,Player::AddWeaponCommand::Execute,weapon,PROVEN,RTTI vf01; different weapon -> ShadowWeapon vslot 0xB; ShadowWeapon::AddWeapon(+0x14;+0x18); rumble 6
8009BD98,Player::PickupWeaponCommand::Execute,weapon,PROVEN,RTTI vf01; grounded -> PickupWeapon behavior; else throw current + UseCommand to object
8005328C,SoundManager_Get,sound,STRONG,lazy global object bss 80571C34; used before every PlaySE
801D59AC,SoundManager_PlaySE,sound,STRONG,(mgr;id;..) id 0xFFFF = none; ids 0x201D missile; 0xE0xx item pickups; weapon pickup SE
801D5894,SoundManager_PlaySEAt,sound,LIKELY,(mgr;id;pos;..) positional variant (explosions; Enemy_PlaySE)
8034ED50,Pad_Rumble,input,LIKELY,(out;pattern;padIndex); patterns 4 damage; 6 AddWeapon; registry+8 on fire
"""

TARGETING = """
80060180,TargetManager_Get,targeting,PROVEN,singleton bss 80576F54 ctor fn_801E7F68; cached sbss 805EF2A0
801E7F68,TargetManager_Construct,targeting,PROVEN,+0 u8 debug flag; +4 TargetList (weak_ptr<Target> list at +8)
801E79AC,TargetManageTask_Create,targeting,PROVEN,new(0x28) Task "TargetManager"; StageManager_InitStage @801785EC layer 15
801E7A08,TargetManageTask::DebugDrawUpdate,targeting,STRONG,RTTI vf01; for_each target fn_801E7B5C only if manager+0 byte set
801E7B5C,TargetManager_DebugDrawTarget,targeting,LIKELY,marker radius 11 color 80/FF/FF/FF via fn_802E1494
801E7438,Target_Create,targeting,PROVEN,new(0x38) Target(owner;mask); deleter fn_801E7680; TargetManager_Register
801E75EC,Target_CreateNoOwner,targeting,PROVEN,Target(mask) ctor 801E783C then TargetManager_Register
801E7680,Target_Deleter,targeting,PROVEN,virtual dtor then TargetManager_PruneExpired
801E7E04,TargetManager_Register,targeting,PROVEN,flags|=0x800000 once; push weak_ptr into manager list
801E7D30,TargetManager_PruneExpired,targeting,PROVEN,erases expired weak_ptr nodes
801E76FC,Target::GetPosition,targeting,PROVEN,RTTI Target::vf00; returns this+8; used by all distance predicates
801E76C0,Target_SetApproachDirection,targeting,STRONG,+0x14=dir; flags|=2 (approach cone uses cos at +0x24)
800CD220,Target_SetPosition,targeting,PROVEN,Vec3_Copy(this+8;v)
801E4470,Target_CreateInteractable,targeting,PROVEN,mask|0x2020; stores at obj+0x6C; Target+0x20=f1 (30 for Bomb/Container/HealingUnit)
801E7C40,TargetManager_CreateSearchList,targeting,PROVEN,new(0x14) TargetSearchList(mgr+4); remove_if !(flags&1)
801E83D0,TargetSearchList_KeepMask,targeting,PROVEN,remove_if fn_801E926C (flags&mask)==0
801E8448,TargetSearchList_ExcludeMask,targeting,PROVEN,remove_if fn_801E92CC flags&mask
801E8158,TargetSearchList_ExcludeTarget,targeting,PROVEN,remove_if fn_801E906C pointer equality
801E7FA4,TargetSearchList_ExcludeOwner,targeting,PROVEN,remove_if fn_801E8F84 owner(+0x28)==given
801E8358,TargetSearchList_KeepOwnerKind,targeting,PROVEN,remove_if fn_801E91A4 owner+0x5C!=value or no owner
801E82C4,TargetSearchList_KeepCone,targeting,PROVEN,fn_801E90F0 removes dot(dir;n(t-origin))<=cos
801E8528,TargetSearchList_KeepWithinRange,targeting,PROVEN,caches dist2 in Target+4 (fn_801E94E8); removes dist2>r*r (fn_801E9430)
801E84C0,TargetSearchList_KeepApproachable,targeting,PROVEN,fn_801E9324 flag-2 targets need dot(n(origin-t);+0x14)>=+0x24
801E879C,TargetSearchList_FindNearest,targeting,PROVEN,min 3D dist2 from origin; strict <; first wins ties
801E90F0,TargetPred_OutsideCone,targeting,PROVEN,returns !(dot>cos)
801E9430,TargetPred_OutOfRange,targeting,PROVEN,Target+4 > r2
801E94E8,TargetOp_CacheDistSq,targeting,PROVEN,Target+4=|pos-origin|^2
801E926C,TargetPred_LacksMask,targeting,PROVEN,(flags&mask)==0
801E92CC,TargetPred_HasMask,targeting,PROVEN,(flags&mask)!=0
801E906C,TargetPred_IsSame,targeting,PROVEN,locked pointers equal
801E8F84,TargetPred_OwnedBy,targeting,PROVEN,owner pointer equal
801E91A4,TargetPred_OwnerKindMismatch,targeting,PROVEN,owner+0x5C!=value or no owner
801E9324,TargetPred_NotApproachable,targeting,PROVEN,flag 2 and dot(n(origin-t);+0x14)<+0x24
801E9498,TargetPred_Disabled,targeting,PROVEN,!(flags&1)
80206F34,Player::PlayerTargetBase::Update,targeting,PROVEN,RTTI vf01; own Target pos/enable (flags 0x45/0x10/0x27/0x44); grind aim-yaw +-40deg at 200deg/s
80206E58,PlayerTargetBase_GetAimYawOffset,targeting,PROVEN,returns +0x14; read by fn_800B6E38 and ShadowWeapon IK
80206E60,Player::PlayerTargetBase::AimLocalToWorld,targeting,STRONG,RTTI vf09 Player_LocalToWorldDir
80206EF8,Player::PlayerTargetBase::GetLookPoint,targeting,STRONG,RTTI vf05 default zero vec 805E1860
80206ED0,Player::PlayerTargetBase::GetAimPoint,targeting,STRONG,RTTI vf06 default zero vec
80206EB0,Player::PlayerTargetBase::GetAimTarget,targeting,STRONG,RTTI vf07 empty ptr
80206E90,Player::PlayerTargetBase::GetNearestTarget,targeting,STRONG,RTTI vf08 empty ptr
8020695C,PlayerTarget_SearchInCone,targeting,STRONG,mask r7; cone f1; exclude self; approach; nearest; used by LightDash chain fn_80089B90
800B6A88,PlayerTarget_Create,targeting,PROVEN,new(0x1C) Player::Shadow::PlayerTarget; stored player+0x24C
800B6CC0,Player::Shadow::PlayerTarget::Update,targeting,PROVEN,RTTI vf01 base update then fn_800B76B0
800B6C98,Player::Shadow::PlayerTarget::HasLookTarget,targeting,STRONG,RTTI vf02 impl bit1
800B6C70,Player::Shadow::PlayerTarget::HasAimTarget,targeting,STRONG,RTTI vf03 impl bit0; ControlGun chooses aimed fire
800B6C4C,Player::Shadow::PlayerTarget::SetLookPoint,targeting,STRONG,RTTI vf04 impl+8=v; +4=duration
800B6C1C,Player::Shadow::PlayerTarget::GetLookPoint,targeting,STRONG,RTTI vf05 impl+0x14
800B6BEC,Player::Shadow::PlayerTarget::GetAimPoint,targeting,STRONG,RTTI vf06 impl+0x20 smoothed
800B6BC4,Player::Shadow::PlayerTarget::GetAimTarget,targeting,STRONG,RTTI vf07 impl+0x44
800B6B9C,Player::Shadow::PlayerTarget::GetNearestTarget,targeting,STRONG,RTTI vf08 impl+0x4C; LightDash/UserInput use
800B6AD0,Player::Shadow::PlayerTarget::AimLocalToWorld,targeting,STRONG,RTTI vf09 rotate by impl+0x38/+0x3C then world
800B7920,PlayerTargetImpl_Construct,targeting,PROVEN,0x64 object from PlayerTarget ctor
800B77C8,PlayerTargetImpl_Init,targeting,PROVEN,own Target flags|=0x90080; |=0x2C00 if stage row+8 bit0/bit2
800B76B0,PlayerTargetImpl_Update,targeting,PROVEN,timers +4/+0x54; search; look point selection
800B6E38,PlayerTargetImpl_Search,targeting,PROVEN,nearest front target +0x4C; 0x2000 look; 0x800 aim (flag 0x41; 45deg; +-20deg; weapon range)
800B73C8,PlayerTargetImpl_SmoothAimPoint,targeting,PROVEN,max turn 360deg*dt (805F3E2C)
800B759C,PlayerTargetImpl_AimDirToWorld,targeting,STRONG,flag 0x40 Player_LocalToWorldDir else heading+player+0x198
800B7670,PlayerTargetImpl_SetLookPoint,targeting,PROVEN,+8=v; +4=t
800A18B0,UserInput_FindLightDashTarget,targeting,PROVEN,A0.3; nearest target flag 0x8; dist<50; LOS ray
800A29BC,LightDashRange_StaticInit,targeting,PROVEN,sbss 805EF608=50
80093F50,Player_ClassifyInputDirection,targeting,PROVEN,0 fwd(dot>0.707) 3 back(<-0.707) 1/2 sides via cross(fwd;groundNormal)
8031B3A4,EnemyTarget_Create,targeting,STRONG,new(0x1C) EnemyTarget; called from EnemyStatusCommon::SetupCollision
8031B1DC,EnemyTarget_Construct,targeting,STRONG,category 0x10000/0x40000/0x20000 from enemy vslot2; one Target per (part;mask) pair
801A65EC,EnemyParamCommon::GetTargetPartMasks,targeting,STRONG,RTTI vf1B; table 8053BB48[type] of (part;mask) pairs -1 terminated
8017B5CC,BossTarget_RegisterParts,targeting,STRONG,Target_Create per boss part mask list
8005FD8C,LockonState_SearchNext,targeting,PROVEN,mask 0x1800; cone 0.93969; range +0x20; exclude locked; nearest; lock timer 0.1
80060354,LockonState_Update,targeting,PROVEN,lock every 0.1 s up to +0x44; volley via weapon vf0B
80060784,LockonState_Construct,targeting,PROVEN,0x58 bytes; +0x44=maxLocks; debug color C8/FF/00/00
800606CC,LockonState_SetWeapon,targeting,PROVEN,+0=+0x34=weapon
80060270,LockonState_SetConeFromMuzzle,targeting,PROVEN,+8=M.t; +0x14=M*(0;0;range); +0x20=range
800602F4,LockonState_GetLockCount,targeting,PROVEN,locked vector size
80060318,LockonState_Cancel,targeting,PROVEN,clear bit0; clear list
800606A0,LockonState_BeginFrame,targeting,PROVEN,+0x48=0; prune invalid locks
800609A8,LockonList_PruneInvalid,targeting,PROVEN,erase expired or disabled targets
80060AB4,LockonList_Add,targeting,PROVEN,weapon vf0A; new LockonTarget + LockonMark; push
80060A5C,LockonList_ExcludeFromSearch,targeting,PROVEN,TargetSearchList_ExcludeTarget per lock
800608B0,LockonList_UpdateMarks,targeting,PROVEN,drop dead marks; fn_80061E38 per mark
8006024C,LockonState_SetDebugColor,targeting,LIKELY,+0x24..+0x27 RGBA
800612B4,LockonTarget_Construct,targeting,STRONG,0xC bytes {weak Target; LockonMark*}
8005F958,WeaponLockon_SetDebugColor,targeting,LIKELY,wrapper fn_8006024C
8005F97C,WeaponLockon_SetCone,targeting,PROVEN,wrapper fn_80060270
8005F9A0,WeaponLockon_NotifyShot,targeting,PROVEN,+0x48+=n; +0x4C++
8005F9F0,WeaponLockon_IsLocking,targeting,PROVEN,bit1
8005FAC8,WeaponLockon_GetShotsThisFrame,targeting,PROVEN,+0x48 (ammo decrement)
8005FAF0,WeaponLockon_GetLockCount,targeting,PROVEN,wrapper
8005FB14,WeaponLockon_GetMaxLocks,targeting,PROVEN,+0x44
8005FB3C,WeaponLockon_Cancel,targeting,PROVEN,wrapper
8005FB60,WeaponLockon_StopLocking,targeting,PROVEN,clear bit0
8005FB90,WeaponLockon_StartLocking,targeting,PROVEN,set bit0
8005FBC0,WeaponLockon_BeginVolley,targeting,PROVEN,set bit2; returns 0.25
8005FBF4,WeaponLockon_Update,targeting,PROVEN,wrapper LockonState_Update
8005FC18,WeaponLockon_BeginFrame,targeting,PROVEN,wrapper fn_800606A0
8005C94C,Weapon::AntiTankMissile::FireAtTarget,targeting,PROVEN,RTTI vf0B fn_8006920C; returns 0.25
8005C91C,Weapon::AntiTankMissile::OnLockAcquired,targeting,STRONG,RTTI vf0A sound 0x201D
8005C9C8,Weapon::AntiTankMissile::Update,targeting,PROVEN,RTTI vf02 prune; cone range; update
800650B4,Weapon::RocketLauncher::FireAtTarget,targeting,PROVEN,RTTI vf0B returns .sdata 805E4DD8 0.25
80065084,Weapon::RocketLauncher::OnLockAcquired,targeting,STRONG,RTTI vf0A sound 0x201D
80065180,Weapon::RocketLauncher::Update,targeting,PROVEN,RTTI vf02
802CD3B4,Weapon::AirBolt::AirBolt::FireAtTarget,targeting,PROVEN,RTTI vf0B bullet type 8051EEAC; 0.25
802CD384,Weapon::AirBolt::AirBolt::OnLockAcquired,targeting,STRONG,RTTI vf0A sound 0x2020
802CD43C,Weapon::AirBolt::AirBolt::Update,targeting,PROVEN,RTTI vf02 range 8051EEBC (2000)
80275AA0,@unnamed@WeaponWormShooter_cpp@::WormShooterBase::WormShooterBase,targeting,PROVEN,vtbl 8054E2B8 typeinfo 805EBB10; WeaponLockonBase(maxLocks;idx)
8006920C,BulletShot_BuildMuzzleForwardWithTarget,targeting,PROVEN,dir=muzzle rot*(0;0;1); team; shooter vel; target
800693C0,BulletShot_BuildAimedAtPoint,targeting,PROVEN,dir=normalize(point-muzzle); team from owner+0x5C
8005D03C,Weapon::WeaponBase::GetRange,targeting,PROVEN,RTTI vf05 BulletType_GetRange(+0x14)
80058A98,BulletParams_StaticInit_Homing,targeting,PROVEN,.ctors; homing speeds 150/150/500; ranges 300/2000/1500
80057310,Weapon::Bullet::Homing::Update,targeting,PROVEN,RTTI vf01 steer; sweep; self-destruct after +0x44
80057644,Homing_Steer,targeting,PROVEN,turn toward target <= rate*dt; speed kept
800579BC,Vec3_RotateTowardsLimited,targeting,PROVEN,acos compare; axis-angle rotate by max
80056F74,BulletCreate_Homing_ATM,targeting,PROVEN,Homing(kind 0;300) bullet idx 0x12
80056F2C,BulletCreate_Homing_RL4,targeting,PROVEN,Homing(kind 1;300) idx 0x13
80056EE4,BulletCreate_Homing_RL8,targeting,PROVEN,Homing(kind 1;300) idx 0x14
80056DA4,BulletCreate_Homing_AirBolt,targeting,PROVEN,Homing(kind 4) type 8051EEAC
80061E60,Weapon::LockonMark::Update,targeting,PROVEN,RTTI vf01 kills task when impl update false
80062C9C,LockonMarkImpl_Construct,targeting,STRONG,textures am_tgt0/am_tgt1; AnnounceAttack kind 1
800628B4,LockonMarkImpl_Update,targeting,PROVEN,states 0 lock-in/1 track/2 fade
8006261C,LockonMarkImpl_LockIn,targeting,PROVEN,t+=10dt; scale 1-0.25t -> 0.75
800625B8,LockonMarkImpl_Track,targeting,STRONG,position = target position
800627C4,LockonMarkImpl_AnnounceLocked,targeting,STRONG,AnnounceAttackCommand kind 1 to owner
800626D4,LockonMarkImpl_AnnounceReleased,targeting,STRONG,AnnounceAttackCommand kind 2
80061E38,LockonMark_SetOrigin,targeting,STRONG,impl+8=v
80062034,LockonTarget_GetTargetPosition,targeting,PROVEN,expired -> zero vector
800620D4,LockonMarkImpl_Draw,targeting,LIKELY,bound RednerDrawArgs callback; size 32
8023D80C,@unnamed@PlayerShadowWeaponLockon_cpp@::ControlLockon::ControlLockon,targeting,PROVEN,writes vtbl 80548DB4 (typeinfo 805EB000)
8023D784,@unnamed@PlayerShadowWeaponLockon_cpp@::ControlLockon::~ControlLockon,targeting,PROVEN,vtbl 80548DB4 slot 0
8023D524,@unnamed@PlayerShadowWeaponLockon_cpp@::ControlLockon::Update,targeting,PROVEN,slot 1; locks limited by ammo record+0x1C
8023CF48,@unnamed@PlayerShadowWeaponGun_cpp@::ControlGun::ControlGun,targeting,PROVEN,writes vtbl 80548CF8; sets player flag 0x41 @8023D150
8023CB84,@unnamed@PlayerShadowWeaponGun_cpp@::ControlGun::~ControlGun,targeting,PROVEN,clears flag 0x41 @8023CBA4
8023C814,@unnamed@PlayerShadowWeaponGun_cpp@::ControlGun::Update,targeting,PROVEN,slot 1; HasAimTarget -> weapon vf0B(aimPoint) else vf0A
8023C688,ControlGun_AnnounceShotToAimTarget,targeting,PROVEN,AnnounceAttackCommand(src;4;4) to aim target owner
8023C2B0,ControlGun_OnLaserReflect,targeting,STRONG,PMF; mask 0x800; front half-space; range 805F0FD4
8023D4C8,LaserReflectRange_StaticInit,targeting,PROVEN,805F0FD4=100
8023C188,@unnamed@PlayerShadowWeaponCannon_cpp@::ControlCannon::ControlCannon,targeting,PROVEN,writes vtbl 80548CB4 @8023C1B0
8023C0A8,@unnamed@PlayerShadowWeaponCannon_cpp@::ControlCannon::~ControlCannon,targeting,PROVEN,vtbl 80548CB4 slot 0
8023BE44,@unnamed@PlayerShadowWeaponCannon_cpp@::ControlCannon::Update,targeting,STRONG,vtbl 80548CB4 slot 1
8023DA8C,@unnamed@PlayerShadowWeaponStick_cpp@::ControlStick::ControlStick,targeting,PROVEN,writes vtbl 80548DD4
8023DA1C,@unnamed@PlayerShadowWeaponStick_cpp@::ControlStick::~ControlStick,targeting,PROVEN,vtbl 80548DD4 slot 0
8023D934,@unnamed@PlayerShadowWeaponStick_cpp@::ControlStick::Update,targeting,STRONG,vtbl 80548DD4 slot 1
8023DF84,@unnamed@PlayerShadowWeaponVacuum_cpp@::ControlVacuum::ControlVacuum,targeting,PROVEN,writes vtbl 80548DF4
8023DEE8,@unnamed@PlayerShadowWeaponVacuum_cpp@::ControlVacuum::~ControlVacuum,targeting,PROVEN,clears flag 0x41
8023DBA4,@unnamed@PlayerShadowWeaponVacuum_cpp@::ControlVacuum::Update,targeting,STRONG,slot 1; sets/clears flag 0x41
80239988,VacuumPod_SearchTarget,targeting,STRONG,mask 0x80; cone cos 0.5 or 0.866; owner-kind 1 filter
8023B4BC,Player::Shadow::ShadowWeapon::GetWeaponRange,targeting,PROVEN,RTTI vf07 weapon vf05; 0 if none
8023B46C,Player::Shadow::ShadowWeapon::StartUse,targeting,PROVEN,RTTI vf08 +0x30 bit0 set; flag 0x19
8023B41C,Player::Shadow::ShadowWeapon::CancelUse,targeting,PROVEN,RTTI vf09 bit1 set; bit0 clr
8023B3D8,Player::Shadow::ShadowWeapon::EndUse,targeting,PROVEN,RTTI vf0A bit0 clr; flag 0x19 clr
8023B4FC,Player::Shadow::ShadowWeapon::GetWeaponKind,targeting,PROVEN,RTTI vf06 weapon+0x18 (1 gun;2 cannon;3 lock-on;4 vacuum;5 stick)
8023AE38,ShadowWeapon_HasAmmo,targeting,PROVEN,flag 0x3F or record+0x1C > 0
8023AEB4,ShadowWeapon_AddAmmo,targeting,PROVEN,record+0x1C += n unless infinite (0x3F) for n<=0
8023AF74,ShadowWeapon_GetAmmo,targeting,PROVEN,record+0x1C
8023A384,ShadowWeapon_ComputeArmIKTarget,targeting,STRONG,IK callback bound in ShadowWeapon_Equip; angle = 90deg*info+0x24 + aim yaw; adds recoil +0x40
"""

def vehicle_rows():
    """Rows of the vehicle sub-investigation (tools/agent_weapon_veh_symbols.py ROWS)."""
    from agent_weapon_veh_symbols import ROWS
    return [(int(a, 16), n, 'vehicle', c, e.replace(',', ';')) for a, n, c, e in ROWS]


def gen_catalog_rows():
    d = get_dol()
    out = []
    for r in catalog_rows():
        nm = r['name']
        if not nm:
            continue
        sid = '%04X' % r['id']
        if r['create']:
            out.append((r['create'], 'Weapon_Create_%s' % nm, 'weapon', 'PROVEN',
                        'SET %s info+4 (desc %08X); new(0x%X) %s%s' % (
                            sid, r['desc'], r['size'] or 0, r['ctor'] or '?',
                            ('; vptr ' + r['vt']) if r['vt'] else '')))
        if r['item']:
            out.append((r['item'], 'WeaponItem_Create_%s' % nm, 'weapon', 'PROVEN',
                        'SET %s info+8; new(0x10) Weapon::Item' % sid))
        if r['hook']:
            out.append((r['hook'], 'Weapon_InitResources_%s' % nm, 'weapon', 'PROVEN',
                        'SET %s hook04; "%%s::Initialize" with name %s; BulletType_Init/WeaponRegistry_MarkLoaded(%s)' % (
                            sid, nm, ';'.join(str(x) for x in (r['binit'] or r['oinit'])))))
    # registry-only (special / vehicle) weapons
    for w in list(range(57, 68)) + [28, 29]:
        info = d.u32(0x8051F058 + 16 * w)
        name, _ = word(info)
        nm = d.cstr(name).decode('latin-1') if name else None
        if not nm:
            continue
        create, item = d.u32(info + 4), d.u32(info + 8)
        if create:
            out.append((create, 'Weapon_Create_%s' % nm, 'weapon', 'PROVEN',
                        'weapon registry idx %d info %08X +4' % (w, info)))
        if item:
            out.append((item, 'WeaponItem_Create_%s' % nm, 'weapon', 'PROVEN',
                        'weapon registry idx %d info %08X +8' % (w, info)))
    return out


def parse(block):
    rows = []
    for line in block.strip().splitlines():
        if not line.strip() or line.startswith('address,'):
            continue
        parts = next(csv.reader(io.StringIO(line)))
        if len(parts) != 5:
            raise SystemExit('bad row (need 5 fields): %s' % line)
        rows.append((int(parts[0], 16), parts[1], parts[2], parts[3], parts[4]))
    return rows


def main():
    curated = set()
    with open(CURATED, newline='') as f:
        for rec in csv.DictReader(f):
            try:
                curated.add(int(rec['address'], 16))
            except (KeyError, ValueError):
                pass
    seen = {}
    order = []
    skipped = 0
    for src in (parse(MANUAL), gen_catalog_rows(), parse(TARGETING), vehicle_rows()):
        for a, n, sub, conf, ev in src:
            if a in curated:
                skipped += 1
                continue
            if a in seen:
                continue
            seen[a] = (n, sub, conf, ev.replace(',', ';').replace('"', "'"))
            order.append(a)
    with open(OUT, 'w', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(['address', 'current_name', 'subsystem', 'confidence', 'evidence'])
        for a in sorted(order):
            n, sub, conf, ev = seen[a]
            w.writerow(['%08X' % a, n, sub, conf, ev])
    print('wrote %d rows to %s (skipped %d already curated)' % (len(order), OUT, skipped))


if __name__ == '__main__':
    main()
