#include "sithWeapon.h"

#include "World/sithThing.h"
#include "World/jkPlayer.h"
#include "World/sithSector.h"
#include "Engine/sithCollision.h"
#include "World/sithUnk4.h"
#include "Engine/sithSurface.h"
#include "Engine/sithAdjoin.h"
#include "Engine/sithTemplate.h"
#include "Engine/sithNet.h"
#include "Engine/sithTime.h"
#include "Engine/sithControl.h"
#include "Engine/sithCamera.h"
#include "AI/sithAI.h"
#include "AI/sithAIAwareness.h"
#include "Engine/sithSoundSys.h"
#include "Engine/sithSoundClass.h"
#include "Engine/sithPuppet.h"
#include "Engine/sithPhysics.h"
#include "Cog/sithCog.h"
#include "stdPlatform.h"
#include "Main/jkGame.h"
#include "Win95/DebugConsole.h"
#include "Dss/sithDSSThing.h"
#include "jk.h"

void sithWeapon_InitDefaults()
{
    sithWeapon_bAutoPickup = 1;
    sithWeapon_bAutoSwitch = 3;
    sithWeapon_bAutoReload = 0;
    sithWeapon_bMultiAutoPickup = 15;
    sithWeapon_bMultiplayerAutoSwitch = 3;
    sithWeapon_bMultiAutoReload = 3;
    sithWeapon_bAutoAim = 1;
    g_flt_8BD040 = 0.0;
    g_flt_8BD044 = 5.0;
    g_flt_8BD048 = 10.0;
    g_flt_8BD04C = 30.0;
    g_flt_8BD050 = 1.5;
    g_flt_8BD054 = 0.5;
    g_flt_8BD058 = 2.0;
}

void sithWeapon_Startup()
{
    sithWeapon_InitDefaults();
}

void sithWeapon_Tick(sithThing *weapon, float deltaSeconds)
{
    int typeFlags = weapon->weaponParams.typeflags;
    if (typeFlags & THING_TYPEFLAGS_ISBLOCKING) // shooting walls?
    {
        sithWeapon_sub_4D35E0(weapon);
    }
    else if (typeFlags & THING_TYPEFLAGS_SCREAMING)
    {
        sithWeapon_sub_4D3920(weapon);
    }
    else
    {
        if (typeFlags & SITH_TF_NOHARD 
            && weapon->weaponParams.damage > (double)weapon->weaponParams.mindDamage)
        {
            float v3 = weapon->weaponParams.damage - weapon->weaponParams.rate * deltaSeconds;
            weapon->weaponParams.damage = v3;
            // no idea if this is even correct but it makes sense?
            // c0 | c3, https://c9x.me/x86/html/file_module_x86_id_87.html
            if (v3 <= 0.0)
                v3 = weapon->weaponParams.mindDamage;
            weapon->weaponParams.damage = v3;
        }
        if ( (typeFlags & SITH_TF_TIMER) != 0 && (((uint8_t)bShowInvisibleThings + (weapon->thingIdx & 0xFF)) & 7) == 0 )
            sithAIAwareness_AddEntry(weapon->sector, &weapon->position, 2, 2.0, weapon);
    }
}

void sithWeapon_sub_4D35E0(sithThing *weapon)
{
    float damage; // ecx
    rdVector3 *weaponPos; // edx
    sithSector *sector; // ebx
    sithCollisionSearchEntry *searchRes; // edi
    sithThing *damageReceiver; // eax
    sithThing *explodeTemplate; // eax
    sithThing *trailThing; // eax
    double v19; // st7
    float moveSize; // [esp-8h] [ebp-40h]
    float damage_; // [esp+10h] [ebp-28h]
    rdVector3 weaponPos_; // [esp+14h] [ebp-24h] BYREF
    rdVector3 tmp; // [esp+20h] [ebp-18h] BYREF
    rdVector3 tmp2; // [esp+2Ch] [ebp-Ch] BYREF
    float elementSize; // [esp+3Ch] [ebp+4h]

    damage = weapon->weaponParams.damage;
    weaponPos = &weapon->lookOrientation.lvec;
    elementSize = weapon->weaponParams.elementSize;
    damage_ = damage;
    sector = weapon->sector;
    rdVector_Copy3(&weaponPos_, weaponPos);
    moveSize = weapon->moveSize;
    sithCollision_SearchRadiusForThings(sector, weapon, &weapon->position, &weaponPos_, weapon->weaponParams.range, moveSize, 0);
    searchRes = sithCollision_NextSearchResult();
    if ( searchRes )
    {
        while ( 1 )
        {
            if ( (weapon->weaponParams.typeflags & THING_TYPEFLAGS_8000) != 0
              && weapon->weaponParams.trailThing
              && elementSize < (double)searchRes->distance )
            {
                do
                {
                    rdVector_Copy3(&tmp, &weapon->position);
                    rdVector_MultAcc3(&tmp, &weaponPos_, elementSize);
                    sithThing_Create(weapon->weaponParams.trailThing, &tmp, &weapon->lookOrientation, sector, 0);
                    elementSize += weapon->weaponParams.elementSize;
                }
                while ( elementSize < searchRes->distance );
            }
            if ( (searchRes->collideType & 0x20) == 0 )
                break;
            sector = searchRes->surface->adjoin->sector;
            searchRes = sithCollision_NextSearchResult();
            if ( !searchRes )
                goto LABEL_20;
        }
        if ( (weapon->weaponParams.typeflags & THING_TYPEFLAGS_4000) != 0 )
        {
            damage_ = damage_ - weapon->weaponParams.rate * searchRes->distance;
            if ( weapon->weaponParams.mindDamage > (double)damage_ )
                damage_ = weapon->weaponParams.mindDamage;
        }
        if ( (searchRes->collideType & 1) != 0 )
        {
            sithThing_Damage(searchRes->receiver, weapon, damage_, weapon->weaponParams.damageClass);
            if ( weapon->weaponParams.force != 0.0 )
            {
                damageReceiver = searchRes->receiver;
                if ( damageReceiver->moveType == SITH_MT_PHYSICS )
                {
                    rdVector_Scale3(&tmp2, &weaponPos_, weapon->weaponParams.force);
                    sithPhysics_ThingApplyForce(damageReceiver, &tmp2);
                }
            }
        }
        else if ( (searchRes->collideType & 2) != 0 )
        {
            sithSurface_SendDamageToThing(searchRes->surface, weapon, damage_, weapon->weaponParams.damageClass);
        }
        if ( weapon->weaponParams.explodeTemplate )
        {
            rdVector_Copy3(&tmp2, &weapon->position);
            rdVector_MultAcc3(&tmp2, &weaponPos_, searchRes->distance);
            sithThing_Create(weapon->weaponParams.explodeTemplate, &tmp2, &rdroid_identMatrix34, sector, 0);
        }
    }

LABEL_20:
    sithCollision_SearchClose();
    if ( !searchRes
      && (weapon->weaponParams.typeflags & THING_TYPEFLAGS_8000) != 0
      && weapon->weaponParams.trailThing
      && elementSize < (double)weapon->weaponParams.range )
    {
        do
        {
            rdVector_Copy3(&tmp, &weapon->position);
            rdVector_MultAcc3(&tmp, &weaponPos_, elementSize);
            sithThing_Create(weapon->weaponParams.trailThing, &tmp, &weapon->lookOrientation, sector, 0);
            elementSize += weapon->weaponParams.elementSize;
        }
        while ( elementSize < weapon->weaponParams.range );
    }
    sithThing_Destroy(weapon);
}

void sithWeapon_sub_4D3920(sithThing *weapon)
{
    float elementSize; // eax
    float elementSize__; // edx
    sithCollisionSearchEntry *searchRes; // ebp
    double v5; // st6
    double v6; // st5
    double v7; // st7
    double v8; // st7
    double v9; // st6
    double v10; // st5
    double v11; // st7
    uint8_t v17; // c0
    uint8_t v18; // c3
    double v19; // st6
    double normAng; // st7
    sithSector *sectorLook; // eax
    double v22; // st7
    sithThing *receiveThing; // eax
    double v24; // st7
    double v25; // st6
    sithThing *explodeTemplate; // eax
    double v27; // st6
    double v29; // st6
    double v30; // st5
    double v31; // st7
    double v32; // st7
    double v33; // st6
    double v34; // st5
    double v35; // st7
    double v36; // rtt
    double v37; // st5
    double v38; // st7
    double v40; // st6
    uint8_t v41; // c0
    uint8_t v42; // c3
    double v43; // st6
    double v44; // st7
    sithSector *sectorLook_; // eax
    float range; // [esp-Ch] [ebp-CCh]
    float moveSize; // [esp-8h] [ebp-C8h]
    float elementSize_; // [esp+14h] [ebp-ACh]
    float v49; // [esp+14h] [ebp-ACh]
    float v50; // [esp+18h] [ebp-A8h]
    float v51; // [esp+18h] [ebp-A8h]
    rdVector3 lookOrient; // [esp+1Ch] [ebp-A4h] BYREF
    rdVector3 weaponPos; // [esp+28h] [ebp-98h] BYREF
    rdVector3 a1a; // [esp+34h] [ebp-8Ch] BYREF
    float amount; // [esp+40h] [ebp-80h]
    rdVector3 vertex_out; // [esp+44h] [ebp-7Ch] BYREF
    sithSector *sector; // [esp+50h] [ebp-70h]
    rdVector3 a3; // [esp+54h] [ebp-6Ch] BYREF
    rdVector3 rot; // [esp+60h] [ebp-60h] BYREF
    rdVector3 tmp; // [esp+6Ch] [ebp-54h] BYREF
    rdVector3 tmp2; // [esp+78h] [ebp-48h] BYREF
    rdVector3 vertex; // [esp+84h] [ebp-3Ch] BYREF
    rdMatrix34 camera; // [esp+90h] [ebp-30h] BYREF

    amount = weapon->weaponParams.damage;
    elementSize = weapon->weaponParams.elementSize;
    rdVector_Copy3(&lookOrient, &weapon->lookOrientation.lvec);
    rdVector_Copy3(&weaponPos, &weapon->position);
    moveSize = weapon->moveSize;
    range = weapon->weaponParams.range;
    elementSize_ = elementSize;
    sector = weapon->sector;
    sithCollision_SearchRadiusForThings(sector, weapon, &weapon->position, &lookOrient, range, moveSize, 0);
    elementSize__ = weapon->weaponParams.elementSize;
    _memcpy(&camera, &weapon->lookOrientation, sizeof(camera));
    vertex.x = 0.0;
    vertex.y = elementSize__;
    vertex.z = 0.0;
    searchRes = sithCollision_NextSearchResult();
    if ( searchRes )
    {
        while ( 1 )
        {
            if ( (weapon->weaponParams.typeflags & 0x8000u) != 0
              && weapon->weaponParams.trailThing
              && elementSize_ < (double)searchRes->distance )
            {
                do
                {
                    rdVector_Sub3(&vertex_out, &weaponPos, &weapon->position);
                    elementSize_ = rdVector_Dot3(&vertex_out, &lookOrient);
                    if ( elementSize_ > searchRes->distance )
                        break;
                    rdVector_Copy3(&a3, &weapon->position);
                    rdVector_MultAcc3(&a3, &lookOrient, elementSize_);
                    if (rdVector_Len3(&a3) <= 0.0) // TODO verify this
                    {
                        rot.x = _frand()
                              * (weapon->weaponParams.trainRandAngle + weapon->weaponParams.trainRandAngle)
                              - weapon->weaponParams.trainRandAngle;
                        rot.y = _frand()
                              * (weapon->weaponParams.trainRandAngle + weapon->weaponParams.trainRandAngle)
                              - weapon->weaponParams.trainRandAngle;
                        rot.z = _frand()
                              * (weapon->weaponParams.trainRandAngle + weapon->weaponParams.trainRandAngle)
                              - weapon->weaponParams.trainRandAngle;
                        rdMatrix_PreRotate34(&camera, &rot);
                        if ( camera.lvec.x * lookOrient.x + camera.lvec.y * lookOrient.y + camera.lvec.z * lookOrient.z < 0.0 )
                            _memcpy(&camera, &weapon->lookOrientation, sizeof(camera));
                    }
                    else
                    {
                        rdVector_Sub3(&a1a, &a3, &weaponPos);
                        rdVector_Normalize3Acc(&a1a);
                        
                        v19 = _frand() * 0.5;
                        rdVector_Scale3Acc(&a1a, v19);
                        v50 = 1.0 - v19;
                        
                        rdVector_Copy3(&tmp, &a1a);
                        rdVector_MultAcc3(&tmp, &lookOrient, v50);
                        rdVector_Normalize3Acc(&tmp);
                        rdVector_ExtractAngle(&tmp, &rot);
                        rdMatrix_BuildRotate34(&camera, &rot);
                    }
                    sectorLook = sithCollision_GetSectorLookAt(sector, &a3, &weaponPos, 0.0);
                    sithThing_Create(weapon->weaponParams.trailThing, &weaponPos, &camera, sectorLook, 0);
                    rdMatrix_TransformPoint34(&vertex_out, &vertex, &camera);
                    rdVector_Add3Acc(&weaponPos, &vertex_out);
                }
                while ( elementSize_ < (double)searchRes->distance );
            }
            if ( (searchRes->collideType & 0x20) == 0 )
                break;
            sector = searchRes->surface->adjoin->sector;
            searchRes = sithCollision_NextSearchResult();
            if ( !searchRes )
                goto LABEL_25;
        }
        if ( (weapon->weaponParams.typeflags & THING_TYPEFLAGS_4000) != 0 )
        {
            v22 = weapon->weaponParams.mindDamage;
            amount = amount - searchRes->distance * weapon->weaponParams.rate;
            if ( v22 > amount )
                amount = weapon->weaponParams.mindDamage;
        }
        if ( (searchRes->collideType & 1) != 0 )
        {
            sithThing_Damage(searchRes->receiver, weapon, amount, weapon->weaponParams.damageClass);
            if ( weapon->weaponParams.force != 0.0 )
            {
                receiveThing = searchRes->receiver;
                if ( receiveThing->moveType == SITH_MT_PHYSICS )
                {
                    tmp2.x = weapon->weaponParams.force * lookOrient.x;
                    tmp2.y = weapon->weaponParams.force * lookOrient.y;
                    tmp2.z = weapon->weaponParams.force * lookOrient.z;
                    sithPhysics_ThingApplyForce(receiveThing, &tmp2);
                }
            }
        }
        else if ( (searchRes->collideType & 2) != 0 )
        {
            sithSurface_SendDamageToThing(searchRes->surface, weapon, amount, weapon->weaponParams.damageClass);
        }
        explodeTemplate = weapon->weaponParams.explodeTemplate;
        if ( explodeTemplate )
        {
            tmp2.x = searchRes->distance * lookOrient.x + weapon->position.x;
            tmp2.y = searchRes->distance * lookOrient.y + weapon->position.y;
            tmp2.z = searchRes->distance * lookOrient.z + weapon->position.z;
            sithThing_Create(explodeTemplate, &tmp2, &rdroid_identMatrix34, sector, 0);
        }
    }
LABEL_25:
    sithCollision_SearchClose();
    if ( !searchRes
      && (weapon->weaponParams.typeflags & 0x8000u) != 0
      && weapon->weaponParams.trailThing
      && elementSize_ < (double)weapon->weaponParams.range )
    {
        do
        {
            rdVector_Sub3(&vertex_out, &weaponPos, &weapon->position);
            v32 = rdVector_Dot3(&vertex_out, &lookOrient);
            if ( v32 > weapon->weaponParams.range )
                break;

            a3.x = v32 * lookOrient.x + weapon->position.x;
            a3.y = v32 * lookOrient.y + weapon->position.y;
            a3.z = v32 * lookOrient.z + weapon->position.z;
            if (rdVector_Len3(&a3) <= 0.0) // TODO verify this
            {
                rot.x = _frand() * (weapon->weaponParams.trainRandAngle + weapon->weaponParams.trainRandAngle)
                      - weapon->weaponParams.trainRandAngle;
                rot.y = _frand() * (weapon->weaponParams.trainRandAngle + weapon->weaponParams.trainRandAngle)
                      - weapon->weaponParams.trainRandAngle;
                rot.z = _frand() * (weapon->weaponParams.trainRandAngle + weapon->weaponParams.trainRandAngle)
                      - weapon->weaponParams.trainRandAngle;
                rdMatrix_PreRotate34(&camera, &rot);
                if ( camera.lvec.x * lookOrient.x + camera.lvec.y * lookOrient.y + camera.lvec.z * lookOrient.z < 0.0 )
                    _memcpy(&camera, &weapon->lookOrientation, sizeof(camera));
            }
            else
            {
                rdVector_Sub3(&a1a, &a3, &weaponPos);
                rdVector_Normalize3Acc(&a1a);
                v43 = _frand() * 0.5;
                rdVector_Scale3Acc(&a1a, v43);
                v51 = 1.0 - v43;
                rdVector_Copy3(&tmp, &a1a);
                rdVector_MultAcc3(&tmp, &lookOrient, v51);
                rdVector_Normalize3Acc(&tmp);
                rdVector_ExtractAngle(&tmp, &rot);
                rdMatrix_BuildRotate34(&camera, &rot);
            }
            sectorLook_ = sithCollision_GetSectorLookAt(sector, &a3, &weaponPos, 0.0);
            sithThing_Create(weapon->weaponParams.trailThing, &weaponPos, &camera, sectorLook_, 0);
            rdMatrix_TransformPoint34(&vertex_out, &vertex, &camera);
            rdVector_Add3Acc(&weaponPos, &vertex_out);
        }
        while ( v32 < (double)weapon->weaponParams.range );
    }
    sithThing_Destroy(weapon);
}

int sithWeapon_LoadParams(stdConffileArg *arg, sithThing *thing, int param)
{
    int tmp;

    switch ( param )
    {
        case THINGPARAM_TYPEFLAGS:
            if ( _sscanf(arg->value, "%x", &tmp) != 1 )
                return 1;
            thing->weaponParams.typeflags = tmp;
            return 1;

        case THINGPARAM_DAMAGE:
            thing->weaponParams.damage = _atof(arg->value);
            return 1;

        case THINGPARAM_MINDDAMAGE:
            thing->weaponParams.mindDamage = _atof(arg->value);
            return 1;

        case THINGPARAM_DAMAGECLASS:
            if ( _sscanf(arg->value, "%x", &tmp) == 1 )
            {
                thing->weaponParams.damageClass = tmp;
            }
            return 1;
        case THINGPARAM_EXPLODE:
            thing->weaponParams.explodeTemplate = sithTemplate_GetEntryByName(arg->value);
            return 1;

        case THINGPARAM_FORCE:
            thing->weaponParams.force = _atof(arg->value);
            return 1;

        case THINGPARAM_RANGE:
            thing->weaponParams.range = _atof(arg->value);
            return 1;

        case THINGPARAM_RATE:
            thing->weaponParams.rate = _atof(arg->value);
            return 1;

        case THINGPARAM_ELEMENTSIZE:
            thing->weaponParams.elementSize = _atof(arg->value);
            return 1;

        case THINGPARAM_TRAILTHING:
            thing->weaponParams.trailThing = sithTemplate_GetEntryByName(arg->value);
            return 1;

        case THINGPARAM_TRAILCYLRADIUS:
            thing->weaponParams.trailCylRadius = _atof(arg->value);
            return 1;

        case THINGPARAM_TRAINRANDANGLE:
            thing->weaponParams.trainRandAngle = _atof(arg->value);
            return 1;

        case THINGPARAM_FLESHHIT:
            thing->weaponParams.fleshHitTemplate = sithTemplate_GetEntryByName(arg->value);
            return 1;

        default:
            return 0;
    }
}

sithThing* sithWeapon_Fire(sithThing *weapon, sithThing *projectile, rdVector3 *fireOffset, rdVector3 *aimError, sithSound *fireSound, int anim, float scale, int16_t scaleFlags, float a9)
{
    sithThing *spawned; // esi

    if ( fireSound )
        sithAIAwareness_AddEntry(weapon->sector, &weapon->position, 1, 4.0, weapon);

    spawned = sithWeapon_FireProjectile_0(weapon, projectile, fireOffset, aimError, fireSound, anim, scale, scaleFlags, a9);

    if ( spawned && sithCogVm_multiplayerFlags )
        sithDSSThing_SendFireProjectile(weapon, projectile, fireOffset, aimError, fireSound, anim, scale, scaleFlags, a9, spawned->thing_id, -1, 255);

    return spawned;
}

sithThing* sithWeapon_FireProjectile_0(sithThing *sender, sithThing *projectileTemplate, rdVector3 *fireOffset, rdVector3 *aimError, sithSound *fireSound, int anim, float scale, char scaleFlags, float a9)
{
    sithThing *v9; // esi
    double v12; // st6
    double v13; // st7
    double v14; // rt2
    double v17; // st7
    double v18; // st7
    sithCollisionSearchEntry *v19; // ebx
    sithThing *v20; // ebx
    rdVector3 a1; // [esp+10h] [ebp-48h] BYREF
    rdVector3 a5a; // [esp+1Ch] [ebp-3Ch] BYREF
    rdMatrix34 a3a; // [esp+28h] [ebp-30h] BYREF
    float a6a; // [esp+74h] [ebp+1Ch]
    float a6c; // [esp+74h] [ebp+1Ch]
    float a6; // [esp+74h] [ebp+1Ch]

    //return sithWeapon_FireProjectile_0_(sender, projectileTemplate, fireOffset, aimError, fireSound, anim, scale, scaleFlags, a9);

    v9 = 0;
    if ( !sender || !fireOffset )
        return 0;

    if ( projectileTemplate )
    {
        rdMatrix_BuildFromLook34(&a3a, fireOffset);
        v9 = sithThing_Create(projectileTemplate, &sender->position, &a3a, sender->sector, sender);

        if (!v9 )
            return 0;

        if ( (scaleFlags & 1) != 0 && v9->moveType == SITH_MT_PHYSICS) // Added: physics check
        {
            v9->physicsParams.vel.x = v9->physicsParams.vel.x * scale;
            v9->physicsParams.vel.y = v9->physicsParams.vel.y * scale;
            v9->physicsParams.vel.z = v9->physicsParams.vel.z * scale;
        }
        if ( (scaleFlags & 2) != 0 )
            v9->weaponParams.damage = v9->weaponParams.damage * scale;
        if ( (scaleFlags & 4) != 0 )
            v9->weaponParams.damage = scale * v9->weaponParams.damage;
        if ( (scaleFlags & 8) != 0 )
            v9->weaponParams.unk8 = scale * v9->weaponParams.unk8;
        v12 = aimError->z;
        v13 = aimError->y - sender->position.y;
        a1.x = aimError->x - sender->position.x;
        v14 = v12 - sender->position.z;
        a1.y = v13;
        a1.z = v14;
        if ( a1.x != 0.0 || a1.y != 0.0 || a1.z != 0.0 )
        {
            a6a = rdVector_Normalize3Acc(&a1);
            sithCollision_UpdateThingCollision(v9, &a1, a6a, 0);
        }
        if ( a9 > 0.02 )
        {
            sithPhysics_ThingTick(v9, a9);
            v17 = rdVector_Normalize3(&a5a, &v9->physicsParams.velocityMaybe);
            if ( v17 > 0.0 )
            {
                a6c = v17;
                sithCollision_UpdateThingCollision(v9, &a5a, a6c, v9->physicsParams.physflags);
            }
        }
        if ( !sithNet_isMulti && jkPlayer_setDiff && sender == g_localPlayerThing && (v9->weaponParams.typeflags & THING_TYPEFLAGS_IMMOBILE) != 0 )
        {
            v18 = rdVector_Normalize3(&a5a, &v9->physicsParams.vel) * 3.0;
            a6 = v18 >= 5.0 ? 5.0 : (float)v18;
            sithCollision_SearchRadiusForThings(v9->sector, v9, &v9->position, &a5a, a6, 0.0, 2);
            v19 = sithCollision_NextSearchResult();
            sithCollision_SearchClose();
            if ( v19 )
            {
                if ( (v19->collideType & 1) != 0 )
                {
                    v20 = v19->receiver;
                    if ( v20->thingtype == SITH_THING_ACTOR )
                        sithAI_SetActorFireTarget(v20->actor, 0x1000, v9); // aaaaaaaaa undefined
                }
            }
        }
        goto LABEL_31;
    }

LABEL_31:
    if ( fireSound )
        sithSoundSys_PlaySoundPosThing(fireSound, sender, 1.0, 1.0, 4.0, 0x180);
    if ( anim >= 0 )
    {
        if ( sender->animclass )
            sithPuppet_PlayMode(sender, anim, 0);
    }

    return v9;
}

void sithWeapon_SetTimeLeft(sithThing *weapon, sithThing* a2, float timeLeft)
{
    unsigned int v3; // eax
    
    // TODO: ??? why is timeLeft unused

    if ( (weapon->weaponParams.typeflags & 0x200) != 0 && timeLeft > 1.0 )
    {
        if ( !weapon->lifeLeftMs || weapon->lifeLeftMs > 250 )
            weapon->lifeLeftMs = 250;
    }
}

int sithWeapon_Collide(sithThing *physicsThing, sithThing *collidedThing, sithCollisionSearchEntry *a4, int a5)
{
    int v4; // eax
    int result; // eax
    int v6; // ecx
    int v7; // eax
    double v8; // st7
    double v9; // st5
    double v10; // st6
    double v11; // st6
    double v12; // st4
    double v13; // st7
    double v14; // st7
    int v16; // eax
    int v17; // eax
    sithThing *v19; // ebx
    sithThing *v20; // eax
    sithThing *v21; // edi
    int v22; // eax
    sithThing *fleshHitTemplate; // edi
    sithThing *v24; // ebx
    sithThing *v25; // eax
    sithThing *explodeTemplate; // edi
    sithThing *v27; // ebx
    sithThing *v28; // eax
    uint32_t v30; // eax
    rdVector3 v31; // [esp+10h] [ebp-Ch]

    if ( (physicsThing->weaponParams.typeflags & THING_TYPEFLAGS_1000) != 0 )
    {
        physicsThing->weaponParams.typeflags &= ~0x1000;
        physicsThing->weaponParams.typeflags |= 0x100;
        physicsThing->collideSize = 0.0;
        physicsThing->lifeLeftMs = 550;
        sithSoundClass_ThingPlaySoundclass4(physicsThing, SITH_SC_ACTIVATE);
        return 0;
    }
    v6 = physicsThing->weaponParams.typeflags & 0x400;
    if ( (physicsThing->weaponParams.typeflags & 0x400) != 0 && (collidedThing->thingflags & SITH_TF_4) != 0
      || collidedThing->thingtype == SITH_THING_COG && (physicsThing->weaponParams.typeflags & THING_TYPEFLAGS_CANTSHOOTUNDERWATER) != 0 && physicsThing->weaponParams.field_18 < 2u )
    {
        v7 = physicsThing->weaponParams.field_18;
        physicsThing->weaponParams.field_18 = v7 + 1;
        if ( (unsigned int)v7 < 6 )
        {
            v31 = physicsThing->physicsParams.vel;
            result = sithCollision_DebrisDebrisCollide(physicsThing, collidedThing, a4, 0);
            if ( result )
            {
                v8 = (a4->field_14.x * v31.x + a4->field_14.y * v31.y + a4->field_14.z * v31.z) * -2.0;
                if ( a5 )
                    v8 = -v8;
                v9 = a4->field_14.y * v8 + v31.y;
                v10 = a4->field_14.z * v8 + v31.z;
                physicsThing->physicsParams.vel.x = a4->field_14.x * v8 + v31.x;
                physicsThing->physicsParams.vel.y = v9;
                physicsThing->physicsParams.vel.z = v10;
                rdVector_Normalize3(&physicsThing->lookOrientation.lvec, &physicsThing->physicsParams.vel);
                v11 = physicsThing->lookOrientation.lvec.x;
                v12 = physicsThing->lookOrientation.lvec.y;
                v13 = physicsThing->lookOrientation.lvec.z * 0.0;
                physicsThing->lookOrientation.rvec.x = v12 * 1.0 - v13;
                physicsThing->lookOrientation.rvec.y = v13 - v11 * 1.0;
                physicsThing->lookOrientation.rvec.z = v11 * 0.0 - v12 * 0.0;
                rdVector_Normalize3Acc(&physicsThing->lookOrientation.rvec);
                physicsThing->lookOrientation.uvec.x = physicsThing->lookOrientation.rvec.y * physicsThing->lookOrientation.lvec.z
                                                     - physicsThing->lookOrientation.rvec.z * physicsThing->lookOrientation.lvec.y;
                v14 = physicsThing->lookOrientation.lvec.y * physicsThing->lookOrientation.rvec.x;
                physicsThing->lookOrientation.uvec.y = physicsThing->lookOrientation.rvec.z * physicsThing->lookOrientation.lvec.x
                                                     - physicsThing->lookOrientation.lvec.z * physicsThing->lookOrientation.rvec.x;
                physicsThing->lookOrientation.uvec.z = v14 - physicsThing->lookOrientation.rvec.y * physicsThing->lookOrientation.lvec.x;
                sithSoundClass_ThingPlaySoundclass(physicsThing, SITH_SC_DEFLECTED);
                physicsThing->weaponParams.typeflags &= ~1;
                result = 1;
            }
            return result;
        }
    }
    v16 = collidedThing->thingtype;
    if ( v16 != SITH_THING_ACTOR && v16 != SITH_THING_PLAYER )
    {
        if ( physicsThing->weaponParams.damage != 0.0 )
            sithThing_Damage(collidedThing, physicsThing, physicsThing->weaponParams.damage, physicsThing->weaponParams.damageClass);
        v17 = physicsThing->weaponParams.typeflags;
        if ( (v17 & 4) != 0 )
        {
            if ( !physicsThing->weaponParams.explodeTemplate )
                goto LABEL_53;
            v19 = sithThing_GetParent(physicsThing);
            v20 = sithThing_Create(physicsThing->weaponParams.explodeTemplate, &physicsThing->position, &rdroid_identMatrix34, physicsThing->sector, v19);
            v21 = v20;
            if ( !v20 )
                goto LABEL_53;
            if ( v19 == g_localPlayerThing )
            {
                sithAIAwareness_AddEntry(v20->sector, &v20->position, 0, 2.0, v19);
                if ( (physicsThing->thingflags & 0x100) != 0 )
                {
LABEL_52:
                    v21->thingflags |= 0x100;
                }
LABEL_53:
                sithThing_Destroy(physicsThing);
                return 1;
            }
LABEL_45:
            if ( (physicsThing->thingflags & 0x100) != 0 )
                goto LABEL_52;
            goto LABEL_53;
        }
        if ( (v17 & 0x80u) == 0 )
            return sithCollision_DebrisDebrisCollide(physicsThing, collidedThing, a4, a5);
        sithPhysics_ThingStop(physicsThing);
        sithSoundClass_ThingPauseSoundclass(physicsThing, PHYSFLAGS_GRAVITY);
        sithSoundClass_ThingPlaySoundclass4(physicsThing, SITH_SC_HITHARD);
        physicsThing->moveSize = 0.0;
        sithThing_AttachThing(physicsThing, collidedThing);
        sithPhysics_ThingSetLook(physicsThing, &a4->field_14, 0.0);
        goto LABEL_56;
    }
    if ( (collidedThing->weaponParams.typeflags & THING_TYPEFLAGS_ISBLOCKING) != 0
      && v6
      && (collidedThing->thingflags & (SITH_TF_DEAD|SITH_TF_WILLBEREMOVED)) == 0
      && (collidedThing != g_localPlayerThing || sithTime_curSeconds >= (double)sithWeapon_fireWait)
      && sithUnk4_thing_anim_blocked(physicsThing, collidedThing, a4) )
    {
        return 1;
    }
    if ( physicsThing->weaponParams.damage == 0.0 && (physicsThing->weaponParams.typeflags & 0x808) == 0 )
        return 0;
    result = sithCollision_DebrisDebrisCollide(physicsThing, collidedThing, a4, a5);
    if ( result )
    {
        if ( physicsThing->weaponParams.damage != 0.0 )
            sithThing_Damage(collidedThing, physicsThing, physicsThing->weaponParams.damage, physicsThing->weaponParams.damageClass);
        v22 = physicsThing->weaponParams.typeflags;
        if ( (v22 & THING_TYPEFLAGS_8) != 0 )
        {
            if ( (collidedThing->weaponParams.typeflags & THING_TYPEFLAGS_DROID) != 0 )
            {
                explodeTemplate = physicsThing->weaponParams.explodeTemplate;
                if ( !explodeTemplate )
                    goto LABEL_53;
                v27 = sithThing_GetParent(physicsThing);
                v28 = sithThing_Create(explodeTemplate, &physicsThing->position, &rdroid_identMatrix34, physicsThing->sector, v27);
                v21 = v28;
                if ( !v28 )
                    goto LABEL_53;
                if ( v27 == g_localPlayerThing )
                    sithAIAwareness_AddEntry(v28->sector, &v28->position, 0, 2.0, v27);
                if ( (physicsThing->thingflags & 0x100) == 0 )
                    goto LABEL_53;
                goto LABEL_52;
            }
            fleshHitTemplate = physicsThing->weaponParams.fleshHitTemplate;
            if ( !fleshHitTemplate )
                goto LABEL_53;
            v24 = sithThing_GetParent(physicsThing);
            v25 = sithThing_Create(fleshHitTemplate, &physicsThing->position, &rdroid_identMatrix34, physicsThing->sector, v24);
            v21 = v25;
            if ( !v25 )
                goto LABEL_53;
            if ( v24 == g_localPlayerThing )
                sithAIAwareness_AddEntry(v25->sector, &v25->position, 0, 2.0, v24);
            goto LABEL_45;
        }
        if ( (v22 & THING_TYPEFLAGS_BLIND) == 0 )
            return PHYSFLAGS_GRAVITY;
        sithPhysics_ThingStop(physicsThing);
        sithThing_AttachThing(physicsThing, collidedThing);
LABEL_56:
        v30 = physicsThing->physicsParams.physflags | PHYSFLAGS_GRAVITY;
        physicsThing->attach_flags |= ATTACHFLAGS_THING_RELATIVE;
        physicsThing->physicsParams.physflags = v30;
        return PHYSFLAGS_GRAVITY;
    }
    return result;
}

int sithWeapon_HitDebug(sithThing *thing, sithSurface *surface, sithCollisionSearchEntry *a3)
{
    int result; // eax
    rdMaterial *v4; // eax
    const char *v5; // eax
    int v6; // ecx
    int typeFlags; // eax
    unsigned int v8; // eax
    int v9; // eax
    char v10; // bl
    int v11; // eax
    double v12; // st6
    double v13; // st4
    double v14; // st7
    double v15; // st7
    double v16; // st7
    int v17; // eax
    sithThing *v18; // edi
    sithThing *v19; // ebx
    sithThing *v20; // eax
    sithThing *v21; // edi
    int v22; // eax

    if ( thing->moveType != SITH_MT_PHYSICS )
        return 0;
    if ( (g_debugmodeFlags & 0x40) != 0 )
    {
        v4 = surface->surfaceInfo.face.material;
        if ( v4 )
            v5 = v4->mat_fpath;
        else
            v5 = "none";
        _sprintf(std_genBuffer, "Weapon hit surface %d, sector %d, material '%s'.\n", surface->field_0, surface->parent_sector->id, v5);
        DebugConsole_Print(std_genBuffer);
    }
    v6 = surface->surfaceFlags;
    if ( (v6 & (SURFACEFLAGS_400|SURFACEFLAGS_200)) != 0 )
        goto LABEL_9;
    typeFlags = thing->weaponParams.typeflags;
    if ( ((typeFlags & 0x400) != 0 && (v6 & 0x4000) != 0 || (typeFlags & 0x80000) != 0 && thing->weaponParams.field_18 < 2u)
      && (v8 = thing->weaponParams.field_18, thing->weaponParams.field_18 = v8 + 1, v8 < 6) )
    {
        thing->physicsParams.physflags |= PHYSFLAGS_SURFACEBOUNCE;
        sithCollision_DefaultHitHandler(thing, surface, a3);
        if ( (thing->physicsParams.physflags & PHYSFLAGS_SURFACEBOUNCE) == 0 )
        {
            thing->physicsParams.physflags &= ~PHYSFLAGS_SURFACEBOUNCE;
        }
        rdVector_Normalize3(&thing->lookOrientation.lvec, &thing->physicsParams.vel);
        v12 = thing->lookOrientation.lvec.x;
        v13 = thing->lookOrientation.lvec.y;
        v14 = thing->lookOrientation.lvec.z * 0.0;
        thing->lookOrientation.rvec.x = v13 * 1.0 - v14;
        thing->lookOrientation.rvec.y = v14 - v12 * 1.0;
        thing->lookOrientation.rvec.z = v12 * 0.0 - v13 * 0.0;
        rdVector_Normalize3Acc(&thing->lookOrientation.rvec);
        v15 = thing->lookOrientation.rvec.z * thing->lookOrientation.lvec.x;
        thing->lookOrientation.uvec.x = thing->lookOrientation.rvec.y * thing->lookOrientation.lvec.z
                                      - thing->lookOrientation.rvec.z * thing->lookOrientation.lvec.y;
        thing->lookOrientation.uvec.y = v15 - thing->lookOrientation.lvec.z * thing->lookOrientation.rvec.x;
        v16 = thing->lookOrientation.lvec.y * thing->lookOrientation.rvec.x - thing->lookOrientation.rvec.y * thing->lookOrientation.lvec.x;
        thing->weaponParams.typeflags &= ~THING_TYPEFLAGS_1;
        thing->lookOrientation.uvec.z = v16;
        sithSoundClass_ThingPlaySoundclass(thing, SITH_SC_DEFLECTED);
        result = 1;
    }
    else
    {
        if ( thing->weaponParams.damage != 0.0 )
            sithSurface_SendDamageToThing(surface, thing, thing->weaponParams.damage, thing->weaponParams.damageClass);
        v17 = thing->weaponParams.typeflags;
        if ( (v17 & 4) != 0 )
        {
            v18 = thing->weaponParams.explodeTemplate;
            if ( v18 )
            {
                v19 = sithThing_GetParent(thing);
                v20 = sithThing_Create(v18, &thing->position, &rdroid_identMatrix34, thing->sector, v19);
                v21 = v20;
                if ( v20 )
                {
                    if ( v19 == g_localPlayerThing )
                        sithAIAwareness_AddEntry(v20->sector, &v20->position, 0, 2.0, v19);
                    if ( (thing->thingflags & SITH_TF_INVULN) != 0 )
                    {
                        v21->thingflags |= 0x100;
                    }
                }
            }
LABEL_9:
            sithThing_Destroy(thing);
            return 1;
        }
        if ( (v17 & 0x80u) == 0 )
        {
            result = sithCollision_DefaultHitHandler(thing, surface, a3);
        }
        else
        {
            sithCollision_DefaultHitHandler(thing, surface, a3);
            sithPhysics_ThingStop(thing);
            sithSoundClass_ThingPauseSoundclass(thing, SITH_SC_CREATE);
            thing->moveSize = 0.0;
            sithThing_AttachToSurface(thing, surface, 0);
            sithPhysics_ThingSetLook(thing, &surface->surfaceInfo.face.normal, 0.0);
            thing->physicsParams.physflags |= PHYSFLAGS_NOTHRUST;
            result = 1;
        }
    }
    return result;
}

void sithWeapon_Remove(sithThing *weapon)
{
    // This gets called for thermal detonators when they run out of lifetime
    if ( (weapon->weaponParams.typeflags & 0x100) != 0 )
    {
        sithWeapon_RemoveAndExplode(weapon, weapon->weaponParams.explodeTemplate);
    }
    else
    {
        sithThing_Destroy(weapon);
    }
}

void sithWeapon_RemoveAndExplode(sithThing *weapon, sithThing *explodeTemplate)
{
    if ( explodeTemplate )
    {
        sithThing* player = sithThing_GetParent(weapon);
        sithThing* spawned = sithThing_Create(explodeTemplate, &weapon->position, &rdroid_identMatrix34, weapon->sector, player);
        if ( spawned )
        {
            if ( player == g_localPlayerThing )
                sithAIAwareness_AddEntry(spawned->sector, &spawned->position, 0, 2.0, player);
            if ( (weapon->thingflags & 0x100) != 0 )
            {
                spawned->thingflags |= 0x100;
            }
        }
    }
    sithThing_Destroy(weapon);
}

void sithWeapon_InitializeEntry()
{
    sithWeapon_8BD0A0[0] = -1.0;
    sithWeapon_a8BD030[0] = 0;
    sithWeapon_8BD0A0[1] = -1.0;
    sithWeapon_8BD05C[1] = -1.0;
    sithWeapon_LastFireTimeSecs = -1.0;
    sithWeapon_fireWait = -1.0;
    sithWeapon_fireRate = -1.0;
    sithWeapon_a8BD030[1] = 0;
    sithWeapon_mountWait = 0.0;
    sithWeapon_8BD05C[0] = 0.0;
    sithWeapon_CurWeaponMode = -1;
    sithWeapon_8BD024 = -1;
}

void sithWeapon_ShutdownEntry()
{
    ;
}

int sithWeapon_SelectWeapon(sithThing *player, int binIdx, int a3)
{
    int v4; // edi
    sithCog *v5; // edx
    int v7; // ebp
    int v9; // esi
    int v10; // edi
    int v11; // eax
    int v12; // eax
    sithItemDescriptor *v13; // ebp
    sithCog *v14; // eax
    int v15; // esi
    int v18; // [esp-18h] [ebp-28h]
    int playera; // [esp+14h] [ebp+4h]

    v4 = sithInventory_GetCurWeapon(player);
    if ( binIdx == v4 || sithInventory_GetBinAmount(player, binIdx) == 0.0 || !sithInventory_GetAvailable(player, binIdx) || sithWeapon_8BD024 != -1 )
        return 0;
    v5 = sithInventory_GetBinByIdx(binIdx)->cog;
    if ( v5 )
    {
        v7 = 0;
        v9 = sithWeapon_bAutoSwitch & 2;
        v10 = sithWeapon_bMultiplayerAutoSwitch & 2;
        
        sithWeapon_bAutoSwitch &= ~2;
        
        sithWeapon_bMultiplayerAutoSwitch &= ~2u;
        if ( sithCog_SendMessageEx(v5, SITH_MESSAGE_AUTOSELECT, SENDERTYPE_SYSTEM, -1, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0) < 0.0 )
            v7 = 1;
        if ( v9 )
        {
            sithWeapon_bAutoSwitch |= 2;
        }
        if ( v10 )
        {
            sithWeapon_bMultiplayerAutoSwitch |= 2;
        }
        if (v7)
            return 0;
    }

    v13 = sithInventory_GetBinByIdx(v4);
    v14 = v13->cog;
    if ( v14 )
    {
        sithCog_SendMessage(v14, SITH_MESSAGE_DESELECTED, SENDERTYPE_SYSTEM, sithWeapon_8BD024, SENDERTYPE_THING, player->thingIdx, 0);
        for (int i = 0; i < 2; i++)
        {
            if (sithWeapon_8BD0A0[i] != -1.0 ) {
                sithCog_SendMessage(v13->cog, SITH_MESSAGE_DEACTIVATED, SENDERTYPE_SYSTEM, i, SENDERTYPE_THING, player->thingIdx, 0);
            }
        }
    }

    sithWeapon_8BD024 = binIdx;
    sithWeapon_senderIndex = a3 != 0;
    return 1;
}

void sithWeapon_SetMountWait(sithThing *a1, float mountWait)
{
    sithWeapon_mountWait = mountWait + sithTime_curSeconds;
}

void sithWeapon_SetFireWait(sithThing *weapon, float firewait)
{
    if ( firewait == -1.0 )
    {
        sithWeapon_fireWait = -1.0;
        sithWeapon_fireRate = -1.0;
    }
    else
    {
        sithWeapon_fireRate = firewait;
        sithWeapon_fireWait = firewait + sithTime_curSeconds;
    }
}

void sithWeapon_handle_inv_msgs(sithThing *player)
{
    sithItemDescriptor *v1; // eax
    sithCog *v2; // eax
    int v3; // eax
    sithItemDescriptor *v4; // eax
    sithCog *v5; // eax
    int v6; // eax
    sithItemDescriptor *v7; // eax
    sithCog *v8; // eax
    int v9; // [esp-18h] [ebp-1Ch]

    if ( sithWeapon_8BD024 == -1 || sithTime_curSeconds < (double)sithWeapon_mountWait )
    {
        // aaaaaaaaaaa ????? wtf is going on here
        if ( *(int*)&sithWeapon_8BD05C[0] == 1 && sithTime_curSeconds >= (double)sithWeapon_mountWait )
        {
            v3 = sithInventory_GetCurWeapon(player);
            v4 = sithInventory_GetBinByIdx(v3);
            if ( (v4->flags & ITEMINFO_WEAPON) != 0 && sithWeapon_CurWeaponMode != -1 )
            {
                v5 = v4->cog;
                if ( v5 )
                    sithCog_SendMessage(v5, SITH_MESSAGE_ACTIVATE, SENDERTYPE_SYSTEM, sithWeapon_CurWeaponMode, SENDERTYPE_THING, player->thingIdx, 0);
            }
            sithWeapon_8BD05C[0] = 0.0;
        }
        else if ( sithWeapon_CurWeaponMode != -1 && sithWeapon_fireRate > 0.0 && sithTime_curSeconds >= (double)sithWeapon_fireWait )
        {
            v6 = sithInventory_GetCurWeapon(player);
            v7 = sithInventory_GetBinByIdx(v6);
            if ( (v7->flags & ITEMINFO_WEAPON) != 0 )
            {
                v8 = v7->cog;
                if ( v8 )
                {
                    v9 = player->thingIdx;
                    sithWeapon_fireWait = sithWeapon_fireRate + sithTime_curSeconds;
                    sithCog_SendMessageEx(v8, SITH_MESSAGE_FIRE, SENDERTYPE_SYSTEM, sithWeapon_CurWeaponMode, SENDERTYPE_THING, v9, 0, 0.0, 0.0, 0.0, 0.0);
                }
            }
        }
    }
    else
    {
        v1 = sithInventory_GetBinByIdx(sithWeapon_8BD024);
        if ( (v1->flags & ITEMINFO_WEAPON) != 0 )
        {
            v2 = v1->cog;
            if ( v2 )
            {
                if ( sithWeapon_8BD024 != -1 )
                {
                    sithWeapon_LastFireTimeSecs = -1.0;
                    sithCog_SendMessage(v2, SITH_MESSAGE_SELECTED, SENDERTYPE_SYSTEM, sithWeapon_senderIndex, SENDERTYPE_THING, player->thingIdx, 0);
                    *(int*)&sithWeapon_8BD05C[0] = 1;
                }
            }
        }
        sithWeapon_8BD024 = -1;
    }
}

void sithWeapon_Activate(sithThing *weapon, sithCog *cogCtx, float fireRate, int mode)
{
    sithWeapon_fireRate = fireRate;
    sithWeapon_CurWeaponMode = mode;
    sithWeapon_8BD0A0[mode] = sithTime_curSeconds;

    if (sithWeapon_fireRate <= 0.0)
        sithWeapon_fireWait = -1.0;

    if ( sithWeapon_fireWait != -1.0 && sithTime_curSeconds >= (double)sithWeapon_fireWait )
    {
        if ( mode != -1 )
            sithCog_SendMessageEx(cogCtx, SITH_MESSAGE_FIRE, 1, mode, 3, weapon->thingIdx, 0, 0.0, 0.0, 0.0, 0.0);
        sithWeapon_fireWait = sithWeapon_fireRate + sithTime_curSeconds;
    }
}

float sithWeapon_Deactivate(sithThing *weapon, sithCog *cogCtx, int mode)
{
    int v3; // edx
    float result; // st7

    // Added: bounds check
    if (mode < 0 || mode >= 2)
        return 0.0;

    v3 = 0;
    if (sithWeapon_8BD0A0[mode] == -1.0 )
        result = 0.0;
    else
        result = sithTime_curSeconds - sithWeapon_8BD0A0[mode];

    sithWeapon_8BD0A0[mode] = -1.0;
    if ( sithWeapon_fireRate > 0.0 )
        sithWeapon_mountWait = sithWeapon_fireRate + sithTime_curSeconds;
    sithWeapon_LastFireTimeSecs = -1.0;
    sithWeapon_fireRate = 0.0;
    for (int i = 0; i < 2; i++)
    {
        if ( sithWeapon_a8BD030[i] == 1 )
            v3 = 1;
    }

    if ( !v3 )
        sithWeapon_CurWeaponMode = -1;
    return result;
}

int sithWeapon_AutoSelect(sithThing *player, int weapIdx)
{
    int v7; // [esp+10h] [ebp-4h]
    float a1a; // [esp+18h] [ebp+4h]

    sithInventory_GetCurWeapon(player);
    v7 = -1;
    a1a = -1.0;
    for (int i = 0; i < SITHBIN_NUMBINS; i++)
    {
        sithItemDescriptor* desc =  &sithInventory_aDescriptors[i];
        if (desc->flags & ITEMINFO_WEAPON)
        {
            if (desc->cog)
            {
                float v5 = sithCog_SendMessageEx(desc->cog, SITH_MESSAGE_AUTOSELECT, SENDERTYPE_SYSTEM, weapIdx, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0);
                if ( v5 > a1a )
                {
                    a1a = v5;
                    v7 = i;
                }
            }
        }
    }
    return v7;
}

int sithWeapon_HandleWeaponKeys(sithThing *player, float a2)
{
    float *v3; // edi
    int v4; // eax
    sithCog *v5; // eax
    int inputFunc; // edi
    int v11; // edi
    sithItemDescriptor *v12; // eax
    sithItemDescriptor *v13; // eax
    signed int v14; // eax
    int v15; // edi
    sithItemDescriptor *v16; // eax
    sithItemDescriptor *v17; // eax
    int v18; // eax
    sithItemDescriptor *v19; // ebx
    int v20; // eax
    int v21; // ebp
    int v22; // edi
    sithCog *v23; // eax
    sithCog *v24; // eax
    int v25; // [esp+10h] [ebp-8h]
    int v26; // [esp+14h] [ebp-4h]
    int readInput;
    
    //return sithWeapon_HandleWeaponKeys_(player, a2);

    if ( player->type != SITH_THING_PLAYER || (player->thingflags & SITH_TF_DEAD) != 0 )
        return 0;

    if ( (player->weaponParams.typeflags & THING_TYPEFLAGS_IMMOBILE) == 0 )
    {
        if ( sithTime_curSeconds < sithWeapon_mountWait )
            return 0;

        inputFunc = INPUT_FUNC_SELECT1;
        while ( 1 )
        {
            sithControl_ReadFunctionMap(inputFunc, &readInput);
            if ( readInput )
            {
                if ( sithWeapon_SelectWeapon(player, sithInventory_SelectWeaponFollowing(inputFunc - INPUT_FUNC_ACTIVATE), 0) )
                    break;
            }

            if ( ++inputFunc <= INPUT_FUNC_SELECT0 )
                continue;

            sithControl_ReadFunctionMap(INPUT_FUNC_NEXTWEAPON, &readInput);
            while (readInput--)
            {
                v11 = sithInventory_GetNumBinsWithFlag(player, sithInventory_GetCurWeapon(player), ITEMINFO_WEAPON);
                if ( v11 == -1 )
                    v11 = sithInventory_GetNumBinsWithFlag(player, 0, ITEMINFO_WEAPON);
                v12 = sithInventory_GetItemDesc(player, v11);
                if ( sithCog_SendMessageEx(v12->cog, SITH_MESSAGE_AUTOSELECT, 0, 0, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0) == -1.0 )
                {
                    do
                    {
                        v11 = sithInventory_GetNumBinsWithFlag(player, v11, ITEMINFO_WEAPON);
                        if ( v11 == -1 )
                            v11 = sithInventory_GetNumBinsWithFlag(player, 0, ITEMINFO_WEAPON);
                        v13 = sithInventory_GetItemDesc(player, v11);
                    }
                    while ( sithCog_SendMessageEx(v13->cog, SITH_MESSAGE_AUTOSELECT, 0, 0, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0) == -1.0 );
                }
                sithWeapon_SelectWeapon(player, v11, 0);
            }

            sithControl_ReadFunctionMap(INPUT_FUNC_PREVWEAPON, &readInput);
            while (readInput--)
            {
                v14 = sithInventory_GetCurWeapon(player);
                
                v15 = sithInventory_GetNumBinsWithFlagRev(player, v14, ITEMINFO_WEAPON);
                if ( v15 == -1 )
                    v15 = sithInventory_GetNumBinsWithFlagRev(player, 0, ITEMINFO_WEAPON);
                
                v16 = sithInventory_GetItemDesc(player, v15);
                if ( sithCog_SendMessageEx(v16->cog, SITH_MESSAGE_AUTOSELECT, 0, 0, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0) == -1.0 )
                {
                    do
                    {
                        v15 = sithInventory_GetNumBinsWithFlagRev(player, v15, ITEMINFO_WEAPON);
                        if ( v15 == -1 )
                            v15 = sithInventory_GetNumBinsWithFlagRev(player, 0, ITEMINFO_WEAPON);
                        v17 = sithInventory_GetItemDesc(player, v15);
                    }
                    while ( sithCog_SendMessageEx(v17->cog, SITH_MESSAGE_AUTOSELECT, 0, 0, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0) == -1.0 );
                }
                sithWeapon_SelectWeapon(player, v15, 0);
            }

            if ( sithWeapon_8BD024 != -1 )
                return 0;

            v18 = sithInventory_GetCurWeapon(player);
            v19 = sithInventory_GetBinByIdx(v18);
            v20 = INPUT_FUNC_FIRE2;
            v26 = INPUT_FUNC_FIRE2;
            v25 = 1;
            v21 = 0;
            while ( 1 )
            {
                v22 = v20 - 10;
                if ( sithControl_ReadFunctionMap(v20, &readInput) )
                {
                    if ( !sithWeapon_a8BD030[v25] )
                    {
                        sithWeapon_a8BD030[v25] = 1;
                        v23 = v19->cog;
                        if ( v23 )
                        {
                            if (sithWeapon_a8BD030[v21]) {
                                sithCog_SendMessage(v23, SITH_MESSAGE_DEACTIVATED, SENDERTYPE_SYSTEM, 1 - v22, SENDERTYPE_THING, player->thingIdx, 0);
                            }
                            sithCog_SendMessage(v19->cog, SITH_MESSAGE_ACTIVATE, SENDERTYPE_SYSTEM, v22, SENDERTYPE_THING, player->thingIdx, 0);
                        }
                    }
                }
                else if ( sithWeapon_a8BD030[v25] == 1 )
                {
                    sithWeapon_a8BD030[v25] = 0;
                    v24 = v19->cog;
                    if ( v24 )
                    {
                        sithCog_SendMessage(v24, SITH_MESSAGE_DEACTIVATED, SENDERTYPE_SYSTEM, v22, SENDERTYPE_THING, player->thingIdx, 0);
                        if (sithWeapon_a8BD030[v21])
                            sithCog_SendMessage(v19->cog, SITH_MESSAGE_ACTIVATE, SENDERTYPE_SYSTEM, 1 - v22, SENDERTYPE_THING, player->thingIdx, 0);
                    }
                }
                ++v21;
                --v26;
                --v25;
                if ( v21 > 1 )
                    break;
                v20 = v26;
            }
            return 0;
        }
        return 0;
    }

    for (int v2 = 0; v2 < 2; v2++)
    {
        if (sithWeapon_a8BD030[v2] == 1 )
        {
            sithWeapon_a8BD030[v2] = 0;
            v4 = sithInventory_GetCurWeapon(player);
            v5 = sithInventory_GetBinByIdx(v4)->cog;
            if ( v5 ) {
                sithCog_SendMessage(v5, SITH_MESSAGE_DEACTIVATED, SENDERTYPE_SYSTEM, v2, SENDERTYPE_THING, player->thingIdx, 0);
            }
        }
    }
    return 0;
}

void sithWeapon_ProjectileAutoAim(rdMatrix34 *out, sithThing *sender, rdMatrix34 *in, rdVector3 *fireOffset, float autoaimFov, float autoaimMaxDist)
{
    double v6; // st7
    double v7; // st6
    float v8; // edx
    unsigned int v9; // ebp
    unsigned int v10; // ebx
    sithThing **v11; // edi
    sithThing *v12; // eax
    sithThing *v13; // eax
    double v15; // st7
    rdVector3 v16; // [esp+0h] [ebp-58h] BYREF
    rdVector3 v17; // [esp+Ch] [ebp-4Ch] BYREF
    sithThing *thingList[16]; // [esp+18h] [ebp-40h] BYREF
    float a3a; // [esp+6Ch] [ebp+14h]
    int a4a; // [esp+70h] [ebp+18h]

    if ( autoaimFov == 0.0 && autoaimMaxDist == 0.0 )
        return;
    if ( jkPlayer_setDiff == 2 )
    {
        v6 = autoaimFov * g_flt_8BD054;
        v7 = autoaimMaxDist * g_flt_8BD054;
        autoaimFov = v6;
        autoaimMaxDist = v7;
    }
    else if ( !jkPlayer_setDiff )
    {
        v6 = autoaimFov * g_flt_8BD050;
        v7 = autoaimMaxDist * g_flt_8BD050;
        autoaimFov = v6;
        autoaimMaxDist = v7;
    }

    if ( sithCamera_currentCamera - sithCamera_cameras == 1 )
    {
        autoaimFov = autoaimFov * g_flt_8BD058;
        autoaimMaxDist = autoaimMaxDist * g_flt_8BD058;
    }
    _memcpy(out, in, sizeof(rdMatrix34));
    out->scale.x = fireOffset->x;
    v8 = fireOffset->z;
    out->scale.y = fireOffset->y;
    out->scale.z = v8;
    v9 = sithAI_FirstThingInView(sender->sector, out, autoaimFov, autoaimMaxDist, 16, thingList, 1028, g_flt_8BD044);
    if ( v9 )
    {
        v10 = 0;
        a4a = -1;
        a3a = -1.0;
        v11 = thingList;
        do
        {
            v12 = *v11;
            if ( *v11 != sender && (v12->actorParams.typeflags & THING_TYPEFLAGS_100000) == 0 )
            {
                if ( sithCollision_HasLos(sender, v12, 0) )
                {
                    v13 = *v11;
                    v16.x = v13->position.x - sender->position.x;
                    v16.y = v13->position.y - sender->position.y;
                    v16.z = v13->position.z - sender->position.z;
                    if (rdVector_Len3(&v16) > g_flt_8BD040)
                    {
                        v17 = out->lvec;
                        rdVector_Normalize3Acc(&v16);
                        rdVector_Normalize3Acc(&v17);
                        v15 = v16.x * v17.x + v16.y * v17.y + v16.z * v17.z;
                        if ( v15 < 0.0 )
                            v15 = -v15;
                        if ( a4a < 0 || v15 > a3a )
                        {
                            a3a = v15;
                            a4a = v10;
                        }
                    }
                }
            }
            ++v10;
            ++v11;
        }
        while ( v10 < v9 );
        if ( a4a >= 0 ) {
            rdMatrix_LookAt(out, fireOffset, &thingList[a4a]->position, 0.0);
            //jkHud_SetTarget(thingList[a4a]);
        }
    }
}

sithThing* sithWeapon_FireProjectile(sithThing *sender, sithThing *projectileTemplate, sithSound *fireSound, int mode, rdVector3 *fireOffset, rdVector3 *aimError, float scale, int16_t scaleFlags, float autoaimFov, float autoaimMaxDist)
{
    int thingtype; // eax
    double v13; // st7
    double v14; // st6
    double v15; // st7
    float v16; // esi
    sithThing *v17; // eax
    sithThing *result; // eax
    rdVector3 v19; // [esp+10h] [ebp-6Ch] BYREF
    rdMatrix34 v20; // [esp+1Ch] [ebp-60h] BYREF
    rdMatrix34 out; // [esp+4Ch] [ebp-30h] BYREF
    float a1a; // [esp+80h] [ebp+4h]
    sithThing *a1b; // [esp+80h] [ebp+4h]
    float a5a; // [esp+90h] [ebp+14h]

    thingtype = sender->type;
    _memcpy(&out, &sender->lookOrientation, sizeof(out));
    if ( thingtype == SITH_THING_ACTOR || thingtype == SITH_THING_PLAYER )
        rdMatrix_PreRotate34(&out, &sender->actorParams.eyePYR);
    if ( fireOffset->x == 0.0 && fireOffset->y == 0.0 && fireOffset->z == 0.0 )
    {
        *fireOffset = sender->position;
    }
    else
    {
        rdMatrix_TransformVector34Acc(fireOffset, &out);
        v13 = sender->position.y + fireOffset->y;
        v14 = sender->position.z + fireOffset->z;
        fireOffset->x = fireOffset->x + sender->position.x;
        fireOffset->y = v13;
        fireOffset->z = v14;
    }

    if ( (sithWeapon_bAutoAim & 1) != 0 && (scaleFlags & 0x20) != 0 && (!sithNet_isMulti || (scaleFlags & 0x40) != 0) )
        sithWeapon_ProjectileAutoAim(&v20, sender, &out, fireOffset, autoaimFov, autoaimMaxDist);
    else

        _memcpy(&v20, &out, sizeof(v20));
    if ( aimError->x != 0.0 || aimError->y != 0.0 || aimError->z != 0.0 )
        rdMatrix_PreRotate34(&v20, aimError);
    v19 = v20.lvec;
    if ( (scaleFlags & 0x10) == 0 )
    {
        sithWeapon_LastFireTimeSecs = -1.0;
LABEL_30:
        v16 = 0.0;
        goto LABEL_31;
    }
    a1a = 1.0;
    if ( sithWeapon_LastFireTimeSecs != -1.0 && sithWeapon_fireRate > 0.0 )
        a1a = (sithTime_curSeconds - sithWeapon_LastFireTimeSecs) / sithWeapon_fireRate - 1.0;
    sithWeapon_LastFireTimeSecs = sithTime_curSeconds;
    if ( a1a <= 1.0 )
        goto LABEL_30;
    do
    {
        v15 = a1a - 1.0;
        a1a = v15;
        a5a = v15 * sithWeapon_fireRate;
        v16 = a5a;
        v17 = sithWeapon_FireProjectile_0(sender, projectileTemplate, &v19, fireOffset, 0, mode, scale, scaleFlags, a5a);
        if ( v17 && sithCogVm_multiplayerFlags )
            sithDSSThing_SendFireProjectile(sender, projectileTemplate, &v19, fireOffset, 0, mode, scale, scaleFlags, a5a, v17->thing_id, -1, 255);
    }
    while ( a1a > 1.0 );
LABEL_31:
    if ( fireSound )
        sithAIAwareness_AddEntry(sender->sector, &sender->position, 1, 4.0, sender);
    result = sithWeapon_FireProjectile_0(sender, projectileTemplate, &v19, fireOffset, fireSound, mode, scale, scaleFlags, v16);
    a1b = result;
    if ( result )
    {
        if ( sithCogVm_multiplayerFlags )
        {
            sithDSSThing_SendFireProjectile(
                sender,
                projectileTemplate,
                &v19,
                fireOffset,
                fireSound,
                mode,
                scale,
                scaleFlags,
                v16,
                result->thing_id,
                -1,
                255);
            result = a1b;
        }
    }
    return result;
}

float sithWeapon_GetPriority(sithThing *player, int binIdx, int mode)
{
    double result; // st7
    sithCog *cog; // eax

    result = -1.0;
    if ( (sithInventory_aDescriptors[binIdx].flags & ITEMINFO_WEAPON) != 0 )
    {
        cog = sithInventory_aDescriptors[binIdx].cog;
        if ( cog )
            result = sithCog_SendMessageEx(cog, SITH_MESSAGE_AUTOSELECT, SENDERTYPE_SYSTEM, mode, SENDERTYPE_THING, player->thingIdx, 0, 0.0, 0.0, 0.0, 0.0);
    }
    return result;
}

int sithWeapon_GetCurWeaponMode()
{
    return sithWeapon_CurWeaponMode;
}

void sithWeapon_SyncPuppet(sithThing *player)
{
    int weapon; // eax
    sithItemDescriptor *itemDesc; // eax
    sithCog *cog; // eax

    weapon = sithInventory_GetCurWeapon(player);
    itemDesc = sithInventory_GetBinByIdx(weapon);
    if ( (itemDesc->flags & ITEMINFO_WEAPON) != 0 )
    {
        cog = itemDesc->cog;
        if ( cog )
        {
            if ( sithWeapon_CurWeaponMode != -1 ) {
                sithCog_SendMessage(cog, SITH_MESSAGE_DEACTIVATED, SENDERTYPE_SYSTEM, sithWeapon_CurWeaponMode, SENDERTYPE_THING, player->thingIdx, 0);
            }
        }
    }
}

int sithWeapon_WriteConf()
{
    return stdConffile_Printf("autoPickup %d\n", sithWeapon_bAutoPickup)
        && stdConffile_Printf("autoSwitch %d\n", sithWeapon_bAutoSwitch)
        && stdConffile_Printf("autoReload %d\n", sithWeapon_bAutoReload)
        && stdConffile_Printf("multiAutoPickup %d\n", sithWeapon_bMultiAutoPickup)
        && stdConffile_Printf("multiAutoSwitch %d\n", sithWeapon_bMultiplayerAutoSwitch)
        && stdConffile_Printf("multiAutoReload %d\n", sithWeapon_bMultiAutoReload)
        && stdConffile_Printf("autoAim %d\n", sithWeapon_bAutoAim);
}

int sithWeapon_ReadConf()
{
    return stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "autopickup")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bAutoPickup) == 1
        && stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "autoswitch")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bAutoSwitch) == 1
        && stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "autoreload")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bAutoReload) == 1
        && stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "multiautopickup")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bMultiAutoPickup) == 1
        && stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "multiautoswitch")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bMultiplayerAutoSwitch) == 1
        && stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "multiautoreload")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bMultiAutoReload) == 1
        && stdConffile_ReadArgs()
        && stdConffile_entry.numArgs
        && !_strcmp(stdConffile_entry.args[0].key, "autoaim")
        && _sscanf(stdConffile_entry.args[1].value, "%d", &sithWeapon_bAutoAim) == 1;
}

// TODO these functions are interesting
void sithWeapon_Syncunused1(){}
void sithWeapon_Syncunused2(){}

void sithWeapon_SetFireRate(sithThing *weapon, float fireRate)
{
    sithWeapon_fireRate = fireRate;
}
