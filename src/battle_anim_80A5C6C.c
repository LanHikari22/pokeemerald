#include "global.h"
#include "constants/battle_anim.h"
#include "constants/species.h"
#include "battle.h"
#include "battle_anim.h"
#include "contest.h"
#include "data2.h"
#include "decompress.h"
#include "palette.h"
#include "pokemon_icon.h"
#include "sprite.h"
#include "task.h"
#include "trig.h"
#include "util.h"
#include "gpu_regs.h"
#include "bg.h"
#include "malloc.h"
#include "dma3.h"

#define GET_UNOWN_LETTER(personality) ((        \
      (((personality & 0x03000000) >> 24) << 6) \
    | (((personality & 0x00030000) >> 16) << 4) \
    | (((personality & 0x00000300) >> 8) << 2)  \
    | (((personality & 0x00000003) >> 0) << 0)  \
) % 28)

#define IS_DOUBLE_BATTLE() ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE))

extern const struct OamData gUnknown_0852497C;
extern const struct MonCoords gMonFrontPicCoords[];
extern const struct MonCoords gMonBackPicCoords[];
extern const u8 gEnemyMonElevation[];
extern const struct CompressedSpriteSheet gMonFrontPicTable[];
extern const union AffineAnimCmd *gUnknown_082FF6C0[];

// This file's functions.
void sub_80A64EC(struct Sprite *sprite);
void sub_80A653C(struct Sprite *sprite);
void InitAnimLinearTranslation(struct Sprite *sprite);
void sub_80A6FB4(struct Sprite *sprite);
void sub_80A6F98(struct Sprite *sprite);
void sub_80A7144(struct Sprite *sprite);
void sub_80A791C(struct Sprite *sprite);
void sub_80A8DFC(struct Sprite *sprite);
void sub_80A8E88(struct Sprite *sprite);
void sub_80A7E6C(u8 spriteId);
u16 sub_80A7F18(u8 spriteId);
void AnimTask_BlendMonInAndOutSetup(struct Task *task);
void sub_80A7AFC(u8 taskId);
void sub_80A8CAC(u8 taskId);
void AnimTask_BlendMonInAndOutStep(u8 taskId);
bool8 sub_80A7238(void);
void sub_80A8048(s16 *bottom, s16 *top, const void *ptr);
void *sub_80A8050(s16 bottom, s16 top);
u8 sub_80A82E4(u8 battlerId);
void sub_80A8D78(struct Task *task, u8 taskId);

// EWRAM vars
EWRAM_DATA static union AffineAnimCmd *gUnknown_02038444 = NULL;

// Const rom data
static const struct UCoords8 sBattlerCoords[][4] =
{
    {
        { 72, 80 },
        { 176, 40 },
        { 48, 40 },
        { 112, 80 },
    },
    {
        { 32, 80 },
        { 200, 40 },
        { 90, 88 },
        { 152, 32 },
    },
};

// One entry for each of the four Castform forms.
const struct MonCoords gCastformFrontSpriteCoords[] =
{
    { 0x44, 17 }, // NORMAL
    { 0x66, 9 }, // SUN
    { 0x46, 9 }, // RAIN
    { 0x86, 8 }, // HAIL
};

static const u8 sCastformElevations[] =
{
    13, // NORMAL
    14, // SUN
    13, // RAIN
    13, // HAIL
};

// Y position of the backsprite for each of the four Castform forms.
static const u8 sCastformBackSpriteYCoords[] =
{
    0, // NORMAL
    0, // SUN
    0, // RAIN
    0, // HAIL
};

static const struct SpriteTemplate sUnknown_08525F90[] =
{
    {
        .tileTag = 55125,
        .paletteTag = 55125,
        .oam = &gUnknown_0852497C,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    {
        .tileTag = 55126,
        .paletteTag = 55126,
        .oam = &gUnknown_0852497C,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    }
};

static const struct SpriteSheet sUnknown_08525FC0[] =
{
    { gMiscBlank_Gfx, 0x800, 55125, },
    { gMiscBlank_Gfx, 0x800, 55126, },
};

// code
u8 GetBattlerSpriteCoord(u8 battlerId, u8 attributeId)
{
    u8 retVal;
    u16 species;
    struct BattleSpriteInfo *spriteInfo;

    if (IsContest())
    {
        if (attributeId == BATTLER_COORD_3 && battlerId == 3)
            attributeId = BATTLER_COORD_Y;
    }

    switch (attributeId)
    {
    case BATTLER_COORD_X:
    case BATTLER_COORD_X_2:
        retVal = sBattlerCoords[IS_DOUBLE_BATTLE()][GetBattlerPosition(battlerId)].x;
        break;
    case BATTLER_COORD_Y:
        retVal = sBattlerCoords[IS_DOUBLE_BATTLE()][GetBattlerPosition(battlerId)].y;
        break;
    case BATTLER_COORD_3:
    case BATTLER_COORD_4:
    default:
        if (IsContest())
        {
            if (shared19348.unk4_0)
                species = shared19348.unk2;
            else
                species = shared19348.unk0;
        }
        else
        {
            if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
            {
                spriteInfo = gBattleSpritesDataPtr->battlerData;
                if (!spriteInfo[battlerId].transformSpecies)
                    species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
                else
                    species = spriteInfo[battlerId].transformSpecies;
            }
            else
            {
                spriteInfo = gBattleSpritesDataPtr->battlerData;
                if (!spriteInfo[battlerId].transformSpecies)
                    species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
                else
                    species = spriteInfo[battlerId].transformSpecies;
            }
        }
        if (attributeId == BATTLER_COORD_3)
            retVal = GetBattlerSpriteFinal_Y(battlerId, species, TRUE);
        else
            retVal = GetBattlerSpriteFinal_Y(battlerId, species, FALSE);
        break;
    }

    return retVal;
}

u8 GetBattlerYDelta(u8 battlerId, u16 species)
{
    u16 letter;
    u32 personality;
    struct BattleSpriteInfo *spriteInfo;
    u8 ret;
    u16 coordSpecies;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER || IsContest())
    {
        if (species == SPECIES_UNOWN)
        {
            if (IsContest())
            {
                if (shared19348.unk4_0)
                    personality = shared19348.unk10;
                else
                    personality = shared19348.unk8;
            }
            else
            {
                spriteInfo = gBattleSpritesDataPtr->battlerData;
                if (!spriteInfo[battlerId].transformSpecies)
                    personality = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
                else
                    personality = gTransformedPersonalities[battlerId];
            }
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                coordSpecies = species;
            else
                coordSpecies = letter + SPECIES_UNOWN_B - 1;
            ret = gMonBackPicCoords[coordSpecies].y_offset;
        }
        else if (species == SPECIES_CASTFORM)
        {
            ret = sCastformBackSpriteYCoords[gBattleMonForms[battlerId]];
        }
        else if (species > NUM_SPECIES)
        {
            ret = gMonBackPicCoords[0].y_offset;
        }
        else
        {
            ret = gMonBackPicCoords[species].y_offset;
        }
    }
    else
    {
        if (species == SPECIES_UNOWN)
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                personality = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
            else
                personality = gTransformedPersonalities[battlerId];
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                coordSpecies = species;
            else
                coordSpecies = letter + SPECIES_UNOWN_B - 1;
            ret = gMonFrontPicCoords[coordSpecies].y_offset;
        }
        else if (species == SPECIES_CASTFORM)
        {
            ret = gCastformFrontSpriteCoords[gBattleMonForms[battlerId]].y_offset;
        }
        else if (species > NUM_SPECIES)
        {
            ret = gMonFrontPicCoords[0].y_offset;
        }
        else
        {
            ret = gMonFrontPicCoords[species].y_offset;
        }
    }
    return ret;
}

u8 GetBattlerElevation(u8 battlerId, u16 species)
{
    u8 ret = 0;
    if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT)
    {
        if (!IsContest())
        {
            if (species == SPECIES_CASTFORM)
                ret = sCastformElevations[gBattleMonForms[battlerId]];
            else if (species > NUM_SPECIES)
                ret = gEnemyMonElevation[0];
            else
                ret = gEnemyMonElevation[species];
        }
    }
    return ret;
}

u8 GetBattlerSpriteFinal_Y(u8 battlerId, u16 species, bool8 a3)
{
    u16 offset;
    u8 y;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER || IsContest())
    {
        offset = GetBattlerYDelta(battlerId, species);
    }
    else
    {
        offset = GetBattlerYDelta(battlerId, species);
        offset -= GetBattlerElevation(battlerId, species);
    }
    y = offset + sBattlerCoords[IS_DOUBLE_BATTLE()][GetBattlerPosition(battlerId)].y;
    if (a3)
    {
        if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
            y += 8;
        if (y > 104)
            y = 104;
    }
    return y;
}

u8 GetBattlerSpriteCoord2(u8 battlerId, u8 attributeId)
{
    u16 species;
    struct BattleSpriteInfo *spriteInfo;

    if (attributeId == BATTLER_COORD_3 || attributeId == BATTLER_COORD_4)
    {
        if (IsContest())
        {
            if (shared19348.unk4_0)
                species = shared19348.unk2;
            else
                species = shared19348.unk0;
        }
        else
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                species = gAnimBattlerSpecies[battlerId];
            else
                species = spriteInfo[battlerId].transformSpecies;
        }
        if (attributeId == BATTLER_COORD_3)
            return GetBattlerSpriteFinal_Y(battlerId, species, TRUE);
        else
            return GetBattlerSpriteFinal_Y(battlerId, species, FALSE);
    }
    else
    {
        return GetBattlerSpriteCoord(battlerId, attributeId);
    }
}

u8 GetBattlerSpriteDefault_Y(u8 battlerId)
{
    return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_4);
}

u8 GetSubstituteSpriteDefault_Y(u8 battlerId)
{
    u16 y;
    if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        y = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y) + 16;
    else
        y = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y) + 17;
    return y;
}

u8 GetBattlerYCoordWithElevation(u8 battlerId)
{
    u16 species;
    u8 y;
    struct BattleSpriteInfo *spriteInfo;

    y = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y);
    if (!IsContest())
    {
        if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
            else
                species = spriteInfo[battlerId].transformSpecies;
        }
        else
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
            else
                species = spriteInfo[battlerId].transformSpecies;
        }
        if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
            y -= GetBattlerElevation(battlerId, species);
    }
    return y;
}

u8 GetAnimBattlerSpriteId(u8 which)
{
    u8 *sprites;

    if (which == ANIM_ATTACKER)
    {
        if (IsBattlerSpritePresent(gBattleAnimAttacker))
        {
            sprites = gBattlerSpriteIds;
            return sprites[gBattleAnimAttacker];
        }
        else
        {
            return 0xff;
        }
    }
    else if (which == ANIM_TARGET)
    {
        if (IsBattlerSpritePresent(gBattleAnimTarget))
        {
            sprites = gBattlerSpriteIds;
            return sprites[gBattleAnimTarget];
        }
        else
        {
            return 0xff;
        }
    }
    else if (which == ANIM_ATK_PARTNER)
    {
        if (!IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
            return 0xff;
        else
            return gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)];
    }
    else
    {
        if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimTarget)))
            return gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimTarget)];
        else
            return 0xff;
    }
}

void StoreSpriteCallbackInData6(struct Sprite *sprite, void (*callback)(struct Sprite*))
{
    sprite->data[6] = (u32)(callback) & 0xffff;
    sprite->data[7] = (u32)(callback) >> 16;
}

void SetCallbackToStoredInData6(struct Sprite *sprite)
{
    u32 callback = (u16)sprite->data[6] | (sprite->data[7] << 16);
    sprite->callback = (void (*)(struct Sprite *))callback;
}

void sub_80A62EC(struct Sprite *sprite)
{
    if (sprite->data[3])
    {
        sprite->pos2.x = Sin(sprite->data[0], sprite->data[1]);
        sprite->pos2.y = Cos(sprite->data[0], sprite->data[1]);
        sprite->data[0] += sprite->data[2];
        if (sprite->data[0] >= 0x100)
            sprite->data[0] -= 0x100;
        else if (sprite->data[0] < 0)
            sprite->data[0] += 0x100;
        sprite->data[3]--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void sub_80A634C(struct Sprite *sprite)
{
    if (sprite->data[3])
    {
        sprite->pos2.x = Sin(sprite->data[0], (sprite->data[5] >> 8) + sprite->data[1]);
        sprite->pos2.y = Cos(sprite->data[0], (sprite->data[5] >> 8) + sprite->data[1]);
        sprite->data[0] += sprite->data[2];
        sprite->data[5] += sprite->data[4];
        if (sprite->data[0] >= 0x100)
            sprite->data[0] -= 0x100;
        else if (sprite->data[0] < 0)
            sprite->data[0] += 0x100;
        sprite->data[3]--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void sub_80A63C8(struct Sprite *sprite)
{
    if (sprite->data[3])
    {
        sprite->pos2.x = Sin(sprite->data[0], sprite->data[1]);
        sprite->pos2.y = Cos(sprite->data[4], sprite->data[1]);
        sprite->data[0] += sprite->data[2];
        sprite->data[4] += sprite->data[5];
        if (sprite->data[0] >= 0x100)
            sprite->data[0] -= 0x100;
        else if (sprite->data[0] < 0)
            sprite->data[0] += 0x100;
        if (sprite->data[4] >= 0x100)
            sprite->data[4] -= 0x100;
        else if (sprite->data[4] < 0)
            sprite->data[4] += 0x100;
        sprite->data[3]--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void sub_80A6450(struct Sprite *sprite)
{
    if (sprite->data[3])
    {
        sprite->pos2.x = Sin(sprite->data[0], sprite->data[1]);
        sprite->pos2.y = Cos(sprite->data[0], sprite->data[4]);
        sprite->data[0] += sprite->data[2];
        if (sprite->data[0] >= 0x100)
            sprite->data[0] -= 0x100;
        else if (sprite->data[0] < 0)
            sprite->data[0] += 0x100;
        sprite->data[3]--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

// Simply waits until the sprite's data[0] hits zero.
// This is used to let sprite anims or affine anims to run for a designated
// duration.
void sub_80A64B0(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
        sprite->data[0]--;
    else
        SetCallbackToStoredInData6(sprite);
}

void sub_80A64D0(struct Sprite *sprite)
{
    sub_80A64EC(sprite);
    sprite->callback = sub_80A653C;
    sprite->callback(sprite);
}

void sub_80A64EC(struct Sprite *sprite)
{
    s16 old;
    int v1;

    if (sprite->data[1] > sprite->data[2])
        sprite->data[0] = -sprite->data[0];
    v1 = sprite->data[2] - sprite->data[1];
    old = sprite->data[0];
    sprite->data[0] = abs(v1 / sprite->data[0]);
    sprite->data[2] = (sprite->data[4] - sprite->data[3]) / sprite->data[0];
    sprite->data[1] = old;
}

void sub_80A653C(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        sprite->data[0]--;
        sprite->pos2.x += sprite->data[1];
        sprite->pos2.y += sprite->data[2];
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void sub_80A656C(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        sprite->data[0]--;
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        sprite->pos2.x = sprite->data[3] >> 8;
        sprite->pos2.y = sprite->data[4] >> 8;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void sub_80A65A8(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        sprite->data[0]--;
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        sprite->pos2.x = sprite->data[3] >> 8;
        sprite->pos2.y = sprite->data[4] >> 8;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
    UpdateMonIconFrame(sprite);
}

void sub_80A65EC(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x + sprite->pos2.x;
    sprite->data[3] = sprite->pos1.y + sprite->pos2.y;
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, 2);
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, 3);
    sprite->callback = sub_80A64D0;
}

void sub_80A6630(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        sprite->data[0]--;
        gSprites[sprite->data[3]].pos2.x += sprite->data[1];
        gSprites[sprite->data[3]].pos2.y += sprite->data[2];
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

// Same as sub_80A6630, but it operates on sub-pixel values
// to handle slower translations.
void sub_80A6680(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        sprite->data[0]--;
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        gSprites[sprite->data[5]].pos2.x = sprite->data[3] >> 8;
        gSprites[sprite->data[5]].pos2.y = sprite->data[4] >> 8;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void sub_80A66DC(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        sprite->data[0]--;
        sprite->pos2.x = sprite->data[2] >> 8;
        sprite->data[2] += sprite->data[1];
        sprite->pos2.y = sprite->data[4] >> 8;
        sprite->data[4] += sprite->data[3];
        if (sprite->data[0] % sprite->data[5] == 0)
        {
            if (sprite->data[5])
                sprite->invisible ^= 1;
        }
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void move_anim_8074EE0(struct Sprite *sprite)
{
    FreeSpriteOamMatrix(sprite);
    DestroyAnimSprite(sprite);
}

void sub_80A6760(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x + sprite->pos2.x;
    sprite->data[3] = sprite->pos1.y + sprite->pos2.y;
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimAttacker, 2);
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimAttacker, 3);
    sprite->callback = sub_80A64D0;
}

void sub_80A67A4(struct Sprite *sprite)
{
    ResetPaletteStructByUid(sprite->data[5]);
    move_anim_8074EE0(sprite);
}

void sub_80A67BC(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
        SetCallbackToStoredInData6(sprite);
}

void sub_80A67D8(struct Sprite *sprite)
{
    if (sprite->animEnded)
        SetCallbackToStoredInData6(sprite);
}

void sub_80A67F4(struct Sprite *sprite)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DestroyAnimSprite(sprite);
}

void sub_80A6814(u8 taskId)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DestroyAnimVisualTask(taskId);
}

void sub_80A6838(struct Sprite *sprite)
{
    sprite->pos1.x = GetBattlerSpriteCoord(gBattleAnimAttacker, 2);
    sprite->pos1.y = GetBattlerSpriteCoord(gBattleAnimAttacker, 3);
}

void sub_80A6864(struct Sprite *sprite, s16 a2)
{
    u16 v1 = GetBattlerSpriteCoord(gBattleAnimAttacker, 0);
    u16 v2 = GetBattlerSpriteCoord(gBattleAnimTarget, 0);

    if (v1 > v2)
    {
        sprite->pos1.x -= a2;
    }
    else if (v1 < v2)
    {
        sprite->pos1.x += a2;
    }
    else
    {
        if (GetBattlerSide(gBattleAnimAttacker) != 0)
            sprite->pos1.x -= a2;
        else
            sprite->pos1.x += a2;
    }
}

void sub_80A68D4(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    InitAnimLinearTranslation(sprite);
    sprite->data[6] = 0x8000 / sprite->data[0];
    sprite->data[7] = 0;
}

bool8 TranslateAnimArc(struct Sprite *sprite)
{
    if (TranslateAnimLinear(sprite))
        return TRUE;
    sprite->data[7] += sprite->data[6];
    sprite->pos2.y += Sin((u8)(sprite->data[7] >> 8), sprite->data[5]);
    return FALSE;
}

bool8 sub_80A6934(struct Sprite *sprite)
{
    if (TranslateAnimLinear(sprite))
        return TRUE;
    sprite->data[7] += sprite->data[6];
    sprite->pos2.x += Sin((u8)(sprite->data[7] >> 8), sprite->data[5]);
    return FALSE;
}

void oamt_add_pos2_onto_pos1(struct Sprite *sprite)
{
    sprite->pos1.x += sprite->pos2.x;
    sprite->pos1.y += sprite->pos2.y;
    sprite->pos2.x = 0;
    sprite->pos2.y = 0;
}

void sub_80A6980(struct Sprite *sprite, bool8 a2)
{
    if (!a2)
    {
        sprite->pos1.x = GetBattlerSpriteCoord2(gBattleAnimTarget, 0);
        sprite->pos1.y = GetBattlerSpriteCoord2(gBattleAnimTarget, 1);
    }
    sub_80A6864(sprite, gBattleAnimArgs[0]);
    sprite->pos1.y += gBattleAnimArgs[1];
}

void sub_80A69CC(struct Sprite *sprite, u8 a2)
{
    if (!a2)
    {
        sprite->pos1.x = GetBattlerSpriteCoord2(gBattleAnimAttacker, 0);
        sprite->pos1.y = GetBattlerSpriteCoord2(gBattleAnimAttacker, 1);
    }
    else
    {
        sprite->pos1.x = GetBattlerSpriteCoord2(gBattleAnimAttacker, 2);
        sprite->pos1.y = GetBattlerSpriteCoord2(gBattleAnimAttacker, 3);
    }
    sub_80A6864(sprite, gBattleAnimArgs[0]);
    sprite->pos1.y += gBattleAnimArgs[1];
}

u8 GetBattlerSide(u8 battlerId)
{
    return GET_BATTLER_SIDE2(battlerId);
}

u8 GetBattlerPosition(u8 battlerId)
{
    return GET_BATTLER_POSITION(battlerId);
}

u8 GetBattlerAtPosition(u8 position)
{
    u8 i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (gBattlerPositions[i] == position)
            break;
    }
    return i;
}

bool8 IsBattlerSpritePresent(u8 battlerId)
{
    if (IsContest())
    {
        if (gBattleAnimAttacker == battlerId)
            return TRUE;
        else if (gBattleAnimTarget == battlerId)
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        if (gBattlerPositions[battlerId] == 0xff)
        {
            return FALSE;
        }
        else if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        {
            if (GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_HP) != 0)
                return TRUE;
        }
        else
        {
            if (GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_HP) != 0)
                return TRUE;
        }
    }
    return FALSE;
}

bool8 IsDoubleBattle(void)
{
    return IS_DOUBLE_BATTLE();
}

void sub_80A6B30(struct UnknownAnimStruct2 *unk)
{
    if (IsContest())
    {
        unk->bgTiles = gUnknown_0202305C;
        unk->unk4 = (u16 *)gUnknown_02023060;
        unk->unk8 = 0xe;
        unk->bgId = 1;
        unk->tilesOffset = 0;
        unk->unkC = 0;
    }
    else
    {
        unk->bgTiles = gUnknown_0202305C;
        unk->unk4 = (u16 *)gUnknown_02023060;
        unk->unk8 = 0x8;
        unk->bgId = 1;
        unk->tilesOffset = 0x200;
        unk->unkC = 0;
    }
}

void sub_80A6B90(struct UnknownAnimStruct2 *unk, u32 arg1)
{
    if (IsContest())
    {
        unk->bgTiles = gUnknown_0202305C;
        unk->unk4 = (u16 *)gUnknown_02023060;
        unk->unk8 = 0xe;
        unk->bgId = 1;
        unk->tilesOffset = 0;
        unk->unkC = 0;
    }
    else if (arg1 == 1)
    {
        sub_80A6B30(unk);
    }
    else
    {
        unk->bgTiles = gUnknown_0202305C;
        unk->unk4 = (u16 *)gUnknown_02023060;
        unk->unk8 = 0x9;
        unk->bgId = 2;
        unk->tilesOffset = 0x300;
        unk->unkC = 0;
    }
}

void sub_80A6BFC(struct UnknownAnimStruct2 *unk)
{
    unk->bgTiles = gUnknown_0202305C;
    unk->unk4 = (u16 *)gUnknown_02023060;
    if (IsContest())
    {
        unk->unk8 = 0xe;
        unk->bgId = 1;
        unk->tilesOffset = 0;
        unk->unkC = 0;
    }
    else if (sub_80A8364(gBattleAnimAttacker) == 1)
    {
        unk->unk8 = 8;
        unk->bgId = 1;
        unk->tilesOffset = 0x200;
        unk->unkC = 0;
    }
    else
    {
        unk->unk8 = 0x9;
        unk->bgId = 2;
        unk->tilesOffset = 0x300;
        unk->unkC = 0;
    }
}

void sub_80A6C68(u32 arg0)
{
    struct UnknownAnimStruct2 unkStruct;

    sub_80A6B90(&unkStruct, arg0);
    CpuFill32(0, unkStruct.bgTiles, 0x2000);
    LoadBgTiles(unkStruct.bgId, unkStruct.bgTiles, 0x2000, unkStruct.tilesOffset);
    FillBgTilemapBufferRect(unkStruct.bgId, 0, 0, 0, 0x20, 0x40, 0x11);
    CopyBgTilemapBufferToVram(unkStruct.bgId);
}

void sub_80A6CC0(u32 bgId, void *src, u32 tilesOffset)
{
    CpuFill32(0, gUnknown_0202305C, 0x2000);
    LZDecompressWram(src, gUnknown_0202305C);
    LoadBgTiles(bgId, gUnknown_0202305C, 0x2000, tilesOffset);
}

void sub_80A6D10(u32 bgId, const void *src)
{
    FillBgTilemapBufferRect(bgId, 0, 0, 0, 0x20, 0x40, 0x11);
    CopyToBgTilemapBuffer(bgId, src, 0, 0);
}

void sub_80A6D48(u32 bgId, const void *src)
{
    sub_80A6D10(bgId, src);
    CopyBgTilemapBufferToVram(bgId);
}

void sub_80A6D60(struct UnknownAnimStruct2 *unk, const void *src, u32 arg2)
{
    sub_80A6D10(unk->bgId, src);
    if (IsContest() == TRUE)
        sub_80A4720(unk->unk8, unk->unk4, 0, arg2);
    CopyBgTilemapBufferToVram(unk->bgId);
}

u8 sub_80A6D94(void)
{
    if (IsContest())
        return 1;
    else
        return 2;
}

void sub_80A6DAC(bool8 arg0)
{
    if (!arg0 || IsContest())
    {
        SetAnimBgAttribute(3, BG_ANIM_SCREEN_SIZE, 0);
        SetAnimBgAttribute(3, BG_ANIM_AREA_OVERFLOW_MODE, 1);
    }
    else
    {
        SetAnimBgAttribute(3, BG_ANIM_SCREEN_SIZE, 1);
        SetAnimBgAttribute(3, BG_ANIM_AREA_OVERFLOW_MODE, 0);
    }
}

void sub_80A6DEC(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    sub_80A6E14(sprite);
    sprite->callback = sub_80A65A8;
    sprite->callback(sprite);
}

void sub_80A6E14(struct Sprite *sprite)
{
    s16 x = (sprite->data[2] - sprite->data[1]) << 8;
    s16 y = (sprite->data[4] - sprite->data[3]) << 8;
    sprite->data[1] = x / sprite->data[0];
    sprite->data[2] = y / sprite->data[0];
    sprite->data[4] = 0;
    sprite->data[3] = 0;
}

void InitAnimLinearTranslation(struct Sprite *sprite)
{
    int x = sprite->data[2] - sprite->data[1];
    int y = sprite->data[4] - sprite->data[3];
    bool8 movingLeft = x < 0;
    bool8 movingUp = y < 0;
    u16 xDelta = abs(x) << 8;
    u16 yDelta = abs(y) << 8;

    xDelta = xDelta / sprite->data[0];
    yDelta = yDelta / sprite->data[0];

    if (movingLeft)
        xDelta |= 1;
    else
        xDelta &= ~1;

    if (movingUp)
        yDelta |= 1;
    else
        yDelta &= ~1;

    sprite->data[1] = xDelta;
    sprite->data[2] = yDelta;
    sprite->data[4] = 0;
    sprite->data[3] = 0;
}

void sub_80A6EEC(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    InitAnimLinearTranslation(sprite);
    sprite->callback = sub_80A6F98;
    sprite->callback(sprite);
}

void sub_80A6F14(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    InitAnimLinearTranslation(sprite);
    sprite->callback = sub_80A6FB4;
    sprite->callback(sprite);
}

bool8 TranslateAnimLinear(struct Sprite *sprite)
{
    u16 v1, v2, x, y;

    if (!sprite->data[0])
        return TRUE;

    v1 = sprite->data[1];
    v2 = sprite->data[2];
    x = sprite->data[3];
    y = sprite->data[4];
    x += v1;
    y += v2;

    if (v1 & 1)
        sprite->pos2.x = -(x >> 8);
    else
        sprite->pos2.x = x >> 8;

    if (v2 & 1)
        sprite->pos2.y = -(y >> 8);
    else
        sprite->pos2.y = y >> 8;

    sprite->data[3] = x;
    sprite->data[4] = y;
    sprite->data[0]--;
    return FALSE;
}

void sub_80A6F98(struct Sprite *sprite)
{
    if (TranslateAnimLinear(sprite))
        SetCallbackToStoredInData6(sprite);
}

void sub_80A6FB4(struct Sprite *sprite)
{
    sub_8039E9C(sprite);
    if (TranslateAnimLinear(sprite))
        SetCallbackToStoredInData6(sprite);
}

void sub_80A6FD4(struct Sprite *sprite)
{
    int v1 = abs(sprite->data[2] - sprite->data[1]) << 8;
    sprite->data[0] = v1 / sprite->data[0];
    InitAnimLinearTranslation(sprite);
}

void sub_80A7000(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    sub_80A6FD4(sprite);
    sprite->callback = sub_80A6F98;
    sprite->callback(sprite);
}

void sub_80A7028(struct Sprite *sprite)
{
    int x = sprite->data[2] - sprite->data[1];
    int y = sprite->data[4] - sprite->data[3];
    bool8 x_sign = x < 0;
    bool8 y_sign = y < 0;
    u16 x2 = abs(x) << 4;
    u16 y2 = abs(y) << 4;

    x2 /= sprite->data[0];
    y2 /= sprite->data[0];

    if (x_sign)
        x2 |= 1;
    else
        x2 &= ~1;

    if (y_sign)
        y2 |= 1;
    else
        y2 &= ~1;

    sprite->data[1] = x2;
    sprite->data[2] = y2;
    sprite->data[4] = 0;
    sprite->data[3] = 0;
}

void sub_80A70C0(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    sub_80A7028(sprite);
    sprite->callback = sub_80A7144;
    sprite->callback(sprite);
}

bool8 sub_80A70E8(struct Sprite *sprite)
{
    u16 v1, v2, x, y;

    if (!sprite->data[0])
        return TRUE;

    v1 = sprite->data[1];
    v2 = sprite->data[2];
    x = sprite->data[3];
    y = sprite->data[4];
    x += v1;
    y += v2;

    if (v1 & 1)
        sprite->pos2.x = -(x >> 4);
    else
        sprite->pos2.x = x >> 4;

    if (v2 & 1)
        sprite->pos2.y = -(y >> 4);
    else
        sprite->pos2.y = y >> 4;

    sprite->data[3] = x;
    sprite->data[4] = y;
    sprite->data[0]--;
    return FALSE;
}

void sub_80A7144(struct Sprite *sprite)
{
    if (sub_80A70E8(sprite))
        SetCallbackToStoredInData6(sprite);
}

void sub_80A7160(struct Sprite *sprite)
{
    int v1 = abs(sprite->data[2] - sprite->data[1]) << 4;
    sprite->data[0] = v1 / sprite->data[0];
    sub_80A7028(sprite);
}

void sub_80A718C(struct Sprite *sprite)
{
    sprite->data[1] = sprite->pos1.x;
    sprite->data[3] = sprite->pos1.y;
    sub_80A7160(sprite);
    sprite->callback = sub_80A7144;
    sprite->callback(sprite);
}

void obj_id_set_rotscale(u8 spriteId, s16 xScale, s16 yScale, u16 rotation)
{
    int i;
    struct ObjAffineSrcData src;
    struct OamMatrix matrix;

    src.xScale = xScale;
    src.yScale = yScale;
    src.rotation = rotation;
    if (sub_80A7238())
        src.xScale = -src.xScale;
    i = gSprites[spriteId].oam.matrixNum;
    ObjAffineSet(&src, &matrix, 1, 2);
    gOamMatrices[i].a = matrix.a;
    gOamMatrices[i].b = matrix.b;
    gOamMatrices[i].c = matrix.c;
    gOamMatrices[i].d = matrix.d;
}

bool8 sub_80A7238(void)
{
    if (IsContest())
    {
        if (gSprites[GetAnimBattlerSpriteId(ANIM_ATTACKER)].data[2] == SPECIES_UNOWN)
            return FALSE;
        else
            return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void sub_80A7270(u8 spriteId, u8 objMode)
{
    u8 battlerId = gSprites[spriteId].data[0];

    if (IsContest() || IsBattlerSpriteVisible(battlerId))
        gSprites[spriteId].invisible = FALSE;
    gSprites[spriteId].oam.objMode = objMode;
    gSprites[spriteId].affineAnimPaused = TRUE;
    if (!IsContest() && !gSprites[spriteId].oam.affineMode)
        gSprites[spriteId].oam.matrixNum = gBattleSpritesDataPtr->healthBoxesData[battlerId].field_6;
    gSprites[spriteId].oam.affineMode = 3;
    CalcCenterToCornerVec(&gSprites[spriteId], gSprites[spriteId].oam.shape, gSprites[spriteId].oam.size, gSprites[spriteId].oam.affineMode);
}

void sub_80A7344(u8 spriteId)
{
    obj_id_set_rotscale(spriteId, 0x100, 0x100, 0);
    gSprites[spriteId].oam.affineMode = 1;
    gSprites[spriteId].oam.objMode = 0;
    gSprites[spriteId].affineAnimPaused = FALSE;
    CalcCenterToCornerVec(&gSprites[spriteId], gSprites[spriteId].oam.shape, gSprites[spriteId].oam.size, gSprites[spriteId].oam.affineMode);
}

void sub_80A73A0(u8 spriteId)
{
    u16 matrix = gSprites[spriteId].oam.matrixNum;
    s16 c = gOamMatrices[matrix].c;

    if (c < 0)
        c = -c;
    gSprites[spriteId].pos2.y = c >> 3;
}

// related to obj_id_set_rotscale
void sub_80A73E0(struct Sprite *sprite, bool8 a2, s16 xScale, s16 yScale, u16 rotation)
{
    int i;
    struct ObjAffineSrcData src;
    struct OamMatrix matrix;

    if (sprite->oam.affineMode & 1)
    {
        sprite->affineAnimPaused = TRUE;
        if (a2)
            CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
        src.xScale = xScale;
        src.yScale = yScale;
        src.rotation = rotation;
        if (sub_80A7238())
            src.xScale = -src.xScale;
        i = sprite->oam.matrixNum;
        ObjAffineSet(&src, &matrix, 1, 2);
        gOamMatrices[i].a = matrix.a;
        gOamMatrices[i].b = matrix.b;
        gOamMatrices[i].c = matrix.c;
        gOamMatrices[i].d = matrix.d;
    }
}

void sub_80A749C(struct Sprite *sprite)
{
    sub_80A73E0(sprite, TRUE, 0x100, 0x100, 0);
    sprite->affineAnimPaused = FALSE;
    CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
}

static u16 ArcTan2_(s16 a, s16 b)
{
    return ArcTan2(a, b);
}

u16 ArcTan2Neg(s16 a, s16 b)
{
    u16 var = ArcTan2_(a, b);
    return -var;
}

void sub_80A750C(u16 a1, bool8 a2)
{
    int i;
    struct PlttData *c;
    struct PlttData *c2;
    u16 average;

    a1 *= 0x10;

    if (!a2)
    {
        for (i = 0; i < 0x10; i++)
        {
            c = (struct PlttData *)&gPlttBufferUnfaded[a1 + i];
            average = c->r + c->g + c->b;
            average /= 3;

            c2 = (struct PlttData *)&gPlttBufferFaded[a1 + i];
            c2->r = average;
            c2->g = average;
            c2->b = average;
        }
    }
    else
    {
        CpuCopy32(&gPlttBufferUnfaded[a1], &gPlttBufferFaded[a1], 0x20);
    }
}

u32 sub_80A75AC(u8 a1, u8 a2, u8 a3, u8 a4, u8 a5, u8 a6, u8 a7)
{
    u32 var = 0;
    u32 shift;

    if (a1)
    {
        if (!IsContest())
            var = 0xe;
        else
            var = 1 << sub_80A6D94();
    }
    if (a2)
    {
        shift = gBattleAnimAttacker + 16;
        var |= 1 << shift;
    }
    if (a3) {
        shift = gBattleAnimTarget + 16;
        var |= 1 << shift;
    }
    if (a4)
    {
        if (IsBattlerSpriteVisible(gBattleAnimAttacker ^ 2))
        {
            shift = (gBattleAnimAttacker ^ 2) + 16;
            var |= 1 << shift;
        }
    }
    if (a5)
    {
        if (IsBattlerSpriteVisible(gBattleAnimTarget ^ 2))
        {
            shift = (gBattleAnimTarget ^ 2) + 16;
            var |= 1 << shift;
        }
    }
    if (a6)
    {
        if (!IsContest())
            var |= 0x100;
        else
            var |= 0x4000;
    }
    if (a7)
    {
        if (!IsContest())
            var |= 0x200;
    }
    return var;
}

u32 sub_80A76C4(u8 a1, u8 a2, u8 a3, u8 a4)
{
    u32 var = 0;
    u32 shift;

    if (IsContest())
    {
        if (a1)
        {
            var |= 1 << 18;
            return var;
        }
    }
    else
    {
        if (a1)
        {
            if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)))
            {
                var |= 1 << (GetBattlerAtPosition(B_POSITION_PLAYER_LEFT) + 16);
            }
        }
        if (a2)
        {
            if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)))
            {
                shift = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT) + 16;
                var |= 1 << shift;
            }
        }
        if (a3)
        {
            if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)))
            {
                shift = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT) + 16;
                var |= 1 << shift;
            }
        }
        if (a4)
        {
            if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)))
            {
                shift = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT) + 16;
                var |= 1 << shift;
            }
        }
    }
    return var;
}

u8 sub_80A77AC(u8 a1)
{
    return a1;
}

u8 sub_80A77B4(u8 position)
{
    return GetBattlerAtPosition(position);
}

void sub_80A77C8(struct Sprite *sprite)
{
    bool8 var;

    if (!sprite->data[0])
    {
        if (!gBattleAnimArgs[3])
            var = TRUE;
        else
            var = FALSE;
        if (!gBattleAnimArgs[2])
            sub_80A69CC(sprite, var);
        else
            sub_80A6980(sprite, var);
        sprite->data[0]++;

    }
    else if (sprite->animEnded || sprite->affineAnimEnded)
    {
        move_anim_8074EE0(sprite);
    }
}

// Linearly translates a sprite to a target position on the
// other mon's sprite.
// arg 0: initial x offset
// arg 1: initial y offset
// arg 2: target x offset
// arg 3: target y offset
// arg 4: duration
// arg 5: lower 8 bits = location on attacking mon, upper 8 bits = location on target mon pick to target
void TranslateAnimSpriteToTargetMonLocation(struct Sprite *sprite)
{
    bool8 v1;
    u8 attributeId;

    if (!(gBattleAnimArgs[5] & 0xff00))
        v1 = TRUE;
    else
        v1 = FALSE;

    if (!(gBattleAnimArgs[5] & 0xff))
        attributeId = BATTLER_COORD_3;
    else
        attributeId = BATTLER_COORD_Y;

    sub_80A69CC(sprite, v1);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
        gBattleAnimArgs[2] = -gBattleAnimArgs[2];

    sprite->data[0] = gBattleAnimArgs[4];
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2) + gBattleAnimArgs[2];
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, attributeId) + gBattleAnimArgs[3];
    sprite->callback = sub_80A6EEC;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

void sub_80A78AC(struct Sprite *sprite)
{
    sub_80A69CC(sprite, 1);
    if (GetBattlerSide(gBattleAnimAttacker))
        gBattleAnimArgs[2] = -gBattleAnimArgs[2];
    sprite->data[0] = gBattleAnimArgs[4];
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, 2) + gBattleAnimArgs[2];
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, 3) + gBattleAnimArgs[3];
    sprite->data[5] = gBattleAnimArgs[5];
    sub_80A68D4(sprite);
    sprite->callback = sub_80A791C;
}

void sub_80A791C(struct Sprite *sprite)
{
    if (TranslateAnimArc(sprite))
        DestroyAnimSprite(sprite);
}

void sub_80A7938(struct Sprite *sprite)
{
    bool8 r4;
    u8 battlerId, attributeId;

    if (!gBattleAnimArgs[6])
    {
        r4 = TRUE;
        attributeId = BATTLER_COORD_3;
    }
    else
    {
        r4 = FALSE;
        attributeId = BATTLER_COORD_Y;
    }
    if (!gBattleAnimArgs[5])
    {
        sub_80A69CC(sprite, r4);
        battlerId = gBattleAnimAttacker;
    }
    else
    {
        sub_80A6980(sprite, r4);
        battlerId = gBattleAnimTarget;
    }
    if (GetBattlerSide(gBattleAnimAttacker))
        gBattleAnimArgs[2] = -gBattleAnimArgs[2];
    sub_80A6980(sprite, r4);
    sprite->data[0] = gBattleAnimArgs[4];
    sprite->data[2] = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_X_2) + gBattleAnimArgs[2];
    sprite->data[4] = GetBattlerSpriteCoord(battlerId, attributeId) + gBattleAnimArgs[3];
    sprite->callback = sub_80A6EEC;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

s16 duplicate_obj_of_side_rel2move_in_transparent_mode(u8 whichBattler)
{
    u16 i;
    u8 spriteId = GetAnimBattlerSpriteId(whichBattler);

    if (spriteId != 0xff)
    {
        for (i = 0; i < MAX_SPRITES; i++)
        {
            if (!gSprites[i].inUse)
            {
                gSprites[i] = gSprites[spriteId];
                gSprites[i].oam.objMode = 1;
                gSprites[i].invisible = FALSE;
                return i;
            }
        }
    }
    return -1;
}

void obj_delete_but_dont_free_vram(struct Sprite *sprite)
{
    sprite->usingSheet = TRUE;
    DestroySprite(sprite);
}

void sub_80A7A74(u8 taskId)
{
    s16 v1 = 0;
    s16 v2 = 0;

    if (gBattleAnimArgs[2] > gBattleAnimArgs[0])
        v2 = 1;
    if (gBattleAnimArgs[2] < gBattleAnimArgs[0])
        v2 = -1;
    if (gBattleAnimArgs[3] > gBattleAnimArgs[1])
        v1 = 1;
    if (gBattleAnimArgs[3] < gBattleAnimArgs[1])
        v1 = -1;

    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = gBattleAnimArgs[4];
    gTasks[taskId].data[2] = 0;
    gTasks[taskId].data[3] = gBattleAnimArgs[0];
    gTasks[taskId].data[4] = gBattleAnimArgs[1];
    gTasks[taskId].data[5] = v2;
    gTasks[taskId].data[6] = v1;
    gTasks[taskId].data[7] = gBattleAnimArgs[2];
    gTasks[taskId].data[8] = gBattleAnimArgs[3];
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gBattleAnimArgs[0], gBattleAnimArgs[1]));
    gTasks[taskId].func = sub_80A7AFC;
}

void sub_80A7AFC(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (++task->data[0] > task->data[1])
    {
        task->data[0] = 0;
        if (++task->data[2] & 1)
        {
            if (task->data[3] != task->data[7])
                task->data[3] += task->data[5];
        }
        else
        {
            if (task->data[4] != task->data[8])
                task->data[4] += task->data[6];
        }
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(task->data[3], task->data[4]));
        if (task->data[3] == task->data[7] && task->data[4] == task->data[8])
        {
            DestroyAnimVisualTask(taskId);
            return;
        }
    }
}

// Linearly blends a mon's sprite colors with a target color with increasing
// strength, and then blends out to the original color.
// arg 0: anim bank
// arg 1: blend color
// arg 2: target blend coefficient
// arg 3: initial delay
// arg 4: number of times to blend in and out
void AnimTask_BlendMonInAndOut(u8 task)
{
    u8 spriteId = GetAnimBattlerSpriteId(gBattleAnimArgs[0]);
    if (spriteId == 0xff)
    {
        DestroyAnimVisualTask(task);
        return;
    }
    gTasks[task].data[0] = (gSprites[spriteId].oam.paletteNum * 0x10) + 0x101;
    AnimTask_BlendMonInAndOutSetup(&gTasks[task]);
}

void AnimTask_BlendMonInAndOutSetup(struct Task *task)
{
    task->data[1] = gBattleAnimArgs[1];
    task->data[2] = 0;
    task->data[3] = gBattleAnimArgs[2];
    task->data[4] = 0;
    task->data[5] = gBattleAnimArgs[3];
    task->data[6] = 0;
    task->data[7] = gBattleAnimArgs[4];
    task->func = AnimTask_BlendMonInAndOutStep;
}

void AnimTask_BlendMonInAndOutStep(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (++task->data[4] >= task->data[5])
    {
        task->data[4] = 0;
        if (!task->data[6])
        {
            task->data[2]++;
            BlendPalette(task->data[0], 15, task->data[2], task->data[1]);
            if (task->data[2] == task->data[3])
                task->data[6] = 1;
        }
        else
        {
            task->data[2]--;
            BlendPalette(task->data[0], 15, task->data[2], task->data[1]);
            if (!task->data[2])
            {
                if (--task->data[7])
                {
                    task->data[4] = 0;
                    task->data[6] = 0;
                }
                else
                {
                    DestroyAnimVisualTask(taskId);
                    return;
                }
            }
        }
    }
}

void sub_80A7CB4(u8 task)
{
    u8 palette = IndexOfSpritePaletteTag(gBattleAnimArgs[0]);

    if (palette == 0xff)
    {
        DestroyAnimVisualTask(task);
        return;
    }
    gTasks[task].data[0] = (palette * 0x10) + 0x101;
    AnimTask_BlendMonInAndOutSetup(&gTasks[task]);
}

void sub_80A7CFC(struct Task *task, u8 a2, const void *a3)
{
    task->data[7] = 0;
    task->data[8] = 0;
    task->data[9] = 0;
    task->data[15] = a2;
    task->data[10] = 0x100;
    task->data[11] = 0x100;
    task->data[12] = 0;
    sub_80A8048(&task->data[13], &task->data[14], a3);
    sub_80A7270(a2, 0);
}

bool8 sub_80A7D34(struct Task *task)
{
    gUnknown_02038444 = sub_80A8050(task->data[13], task->data[14]) + (task->data[7] << 3);
    switch (gUnknown_02038444->type)
    {
    default:
        if (!gUnknown_02038444->frame.duration)
        {
            task->data[10] = gUnknown_02038444->frame.xScale;
            task->data[11] = gUnknown_02038444->frame.yScale;
            task->data[12] = gUnknown_02038444->frame.rotation;
            task->data[7]++;
            gUnknown_02038444++;
        }
        task->data[10] += gUnknown_02038444->frame.xScale;
        task->data[11] += gUnknown_02038444->frame.yScale;
        task->data[12] += gUnknown_02038444->frame.rotation;
        obj_id_set_rotscale(task->data[15], task->data[10], task->data[11], task->data[12]);
        sub_80A7E6C(task->data[15]);
        if (++task->data[8] >= gUnknown_02038444->frame.duration)
        {
            task->data[8] = 0;
            task->data[7]++;
        }
        break;
    case AFFINEANIMCMDTYPE_JUMP:
        task->data[7] = gUnknown_02038444->jump.target;
        break;
    case AFFINEANIMCMDTYPE_LOOP:
        if (gUnknown_02038444->loop.count)
        {
            if (task->data[9])
            {
                if (!--task->data[9])
                {
                    task->data[7]++;
                    break;
                }
            }
            else
            {
                task->data[9] = gUnknown_02038444->loop.count;
            }
            if (!task->data[7])
            {
                break;
            }
            for (;;)
            {
                task->data[7]--;
                gUnknown_02038444--;
                if (gUnknown_02038444->type == AFFINEANIMCMDTYPE_LOOP)
                {
                    task->data[7]++;
                    return TRUE;
                }
                if (!task->data[7])
                    return TRUE;
            }
        }
        task->data[7]++;
        break;
    case AFFINEANIMCMDTYPE_END:
        gSprites[task->data[15]].pos2.y = 0;
        sub_80A7344(task->data[15]);
        return FALSE;
    }

    return TRUE;
}

void sub_80A7E6C(u8 spriteId)
{
    int var = 0x40 - sub_80A7F18(spriteId) * 2;
    u16 matrix = gSprites[spriteId].oam.matrixNum;
    int var2 = (var << 8) / gOamMatrices[matrix].d;

    if (var2 > 0x80)
        var2 = 0x80;
    gSprites[spriteId].pos2.y = (var - var2) / 2;
}

void sub_80A7EC0(u8 spriteId, u8 spriteId2)
{
    int var = 0x40 - sub_80A7F18(spriteId2) * 2;
    u16 matrix = gSprites[spriteId].oam.matrixNum;
    int var2 = (var << 8) / gOamMatrices[matrix].d;

    if (var2 > 0x80)
        var2 = 0x80;
    gSprites[spriteId].pos2.y = (var - var2) / 2;
}

u16 sub_80A7F18(u8 spriteId)
{
    struct BattleSpriteInfo *spriteInfo;
    u8 battlerId = gSprites[spriteId].data[0];
    u16 species;
    u16 i;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        if (gBattlerSpriteIds[i] == spriteId)
        {
            if (IsContest())
            {
                species = shared19348.unk0;
                return gMonBackPicCoords[species].y_offset;
            }
            else
            {
                if (GetBattlerSide(i) == B_SIDE_PLAYER)
                {
                    spriteInfo = gBattleSpritesDataPtr->battlerData;
                    if (!spriteInfo[battlerId].transformSpecies)
                        species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[i]], MON_DATA_SPECIES);
                    else
                        species = spriteInfo[battlerId].transformSpecies;

                    if (species == SPECIES_CASTFORM)
                        return sCastformBackSpriteYCoords[gBattleMonForms[battlerId]];
                    else
                        return gMonBackPicCoords[species].y_offset;
                }
                else
                {
                    spriteInfo = gBattleSpritesDataPtr->battlerData;
                    if (!spriteInfo[battlerId].transformSpecies)
                        species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[i]], MON_DATA_SPECIES);
                    else
                        species = spriteInfo[battlerId].transformSpecies;

                    if (species == SPECIES_CASTFORM)
                        return sCastformElevations[gBattleMonForms[battlerId]];
                    else
                        return gMonFrontPicCoords[species].y_offset;
                }
            }
        }
    }
    return 0x40;
}

void sub_80A8048(s16 *bottom, s16 *top, const void *ptr)
{
    *bottom = ((intptr_t) ptr) & 0xffff;
    *top = (((intptr_t) ptr) >> 16) & 0xffff;
}

void *sub_80A8050(s16 bottom, s16 top)
{
    return (void *)((u16)bottom | ((u16)top << 16));
}

void sub_80A805C(struct Task *task, u8 a2, s16 a3, s16 a4, s16 a5, s16 a6, u16 a7)
{
    task->data[8] = a7;
    task->data[15] = a2; // spriteId
    task->data[9] = a3;
    task->data[10] = a4;
    task->data[13] = a5;
    task->data[14] = a6;
    task->data[11] = (a5 - a3) / a7;
    task->data[12] = (a6 - a4) / a7;
}

u8 sub_80A80C8(struct Task *task)
{
    if (!task->data[8])
        return 0;

    if (--task->data[8] != 0)
    {
        task->data[9] += task->data[11];
        task->data[10] += task->data[12];
    }
    else
    {
        task->data[9] = task->data[13];
        task->data[10] = task->data[14];
    }
    obj_id_set_rotscale(task->data[15], task->data[9], task->data[10], 0);
    if (task->data[8])
        sub_80A7E6C(task->data[15]);
    else
        gSprites[task->data[15]].pos2.y = 0;
    return task->data[8];
}

void AnimTask_GetFrustrationPowerLevel(u8 taskId)
{
    u16 powerLevel;

    if (gAnimFriendship <= 30)
        powerLevel = 0;
    else if (gAnimFriendship <= 100)
        powerLevel = 1;
    else if (gAnimFriendship <= 200)
        powerLevel = 2;
    else
        powerLevel = 3;
    gBattleAnimArgs[7] = powerLevel;
    DestroyAnimVisualTask(taskId);
}

void sub_80A8174(u8 priority)
{
    if (IsBattlerSpriteVisible(gBattleAnimTarget))
        gSprites[gBattlerSpriteIds[gBattleAnimTarget]].oam.priority = priority;
    if (IsBattlerSpriteVisible(gBattleAnimAttacker))
        gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].oam.priority = priority;
    if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimTarget)))
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimTarget)]].oam.priority = priority;
    if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].oam.priority = priority;
}

void sub_80A8278(void)
{
    int i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (IsBattlerSpriteVisible(i))
        {
            gSprites[gBattlerSpriteIds[i]].subpriority = sub_80A82E4(i);
            gSprites[gBattlerSpriteIds[i]].oam.priority = 2;
        }
    }
}

u8 sub_80A82E4(u8 battlerId)
{
    u8 position;
    u8 ret;

    if (IsContest())
    {
        if (battlerId == 2)
            return 30;
        else
            return 40;
    }
    else
    {
        position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_PLAYER_LEFT)
            ret = 30;
        else if (position == B_POSITION_PLAYER_RIGHT)
            ret = 20;
        else if (position == B_POSITION_OPPONENT_LEFT)
            ret = 40;
        else
            ret = 50;
    }
    return ret;
}

u8 sub_80A8328(u8 battlerId)
{
    u8 position = GetBattlerPosition(battlerId);

    if (IsContest())
        return 2;
    else if (position == B_POSITION_PLAYER_LEFT || position == B_POSITION_OPPONENT_RIGHT)
        return GetAnimBgAttribute(2, BG_ANIM_PRIORITY);
    else
        return GetAnimBgAttribute(1, BG_ANIM_PRIORITY);
}

u8 sub_80A8364(u8 battlerId)
{
    if (!IsContest())
    {
        u8 position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_PLAYER_LEFT || position == B_POSITION_OPPONENT_RIGHT)
            return 2;
        else
            return 1;
    }
    return 1;
}

u8 sub_80A8394(u16 species, bool8 isBackpic, u8 a3, s16 x, s16 y, u8 subpriority, u32 personality, u32 trainerId, u32 battlerId, u32 a10)
{
    u8 spriteId;
    u16 sheet = LoadSpriteSheet(&sUnknown_08525FC0[a3]);
    u16 palette = AllocSpritePalette(sUnknown_08525F90[a3].paletteTag);

    if (gMonSpritesGfxPtr != NULL && gMonSpritesGfxPtr->field_17C == NULL)
        gMonSpritesGfxPtr->field_17C = AllocZeroed(0x2000);
    if (!isBackpic)
    {
        LoadCompressedPalette(GetFrontSpritePalFromSpeciesAndPersonality(species, trainerId, personality), (palette * 0x10) + 0x100, 0x20);
        if (a10 == 1 || sub_80688F8(5, battlerId) == 1 || gBattleSpritesDataPtr->battlerData[battlerId].transformSpecies != 0)
            LoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species],
                                                gMonSpritesGfxPtr->field_17C,
                                                species,
                                                personality,
                                                TRUE);
        else
            LoadSpecialPokePic_2(&gMonFrontPicTable[species],
                                 gMonSpritesGfxPtr->field_17C,
                                 species,
                                 personality,
                                 TRUE);
    }
    else
    {
        LoadCompressedPalette(GetFrontSpritePalFromSpeciesAndPersonality(species, trainerId, personality), (palette * 0x10) + 0x100, 0x20);
        if (a10 == 1 || sub_80688F8(5, battlerId) == 1 || gBattleSpritesDataPtr->battlerData[battlerId].transformSpecies != 0)
            LoadSpecialPokePic_DontHandleDeoxys(&gMonBackPicTable[species],
                                                gMonSpritesGfxPtr->field_17C,
                                                species,
                                                personality,
                                                FALSE);
        else
            LoadSpecialPokePic_2(&gMonBackPicTable[species],
                                 gMonSpritesGfxPtr->field_17C,
                                 species,
                                 personality,
                                 FALSE);
    }

    RequestDma3Copy(gMonSpritesGfxPtr->field_17C, (void *)(OBJ_VRAM0 + (sheet * 0x20)), 0x800, 1);
    FREE_AND_SET_NULL(gMonSpritesGfxPtr->field_17C);

    if (!isBackpic)
        spriteId = CreateSprite(&sUnknown_08525F90[a3], x, y + gMonFrontPicCoords[species].y_offset, subpriority);
    else
        spriteId = CreateSprite(&sUnknown_08525F90[a3], x, y + gMonBackPicCoords[species].y_offset, subpriority);

    if (IsContest())
    {
        gSprites[spriteId].affineAnims = gUnknown_082FF6C0;
        StartSpriteAffineAnim(&gSprites[spriteId], 0);
    }
    return spriteId;
}

void sub_80A8610(struct Sprite *sprite)
{
    DestroySpriteAndFreeResources(sprite);
}

s16 sub_80A861C(u8 battlerId, u8 a2)
{
    u16 species;
    u32 personality;
    u16 letter;
    u16 var;
    int ret;
    const struct MonCoords *coords;
    struct BattleSpriteInfo *spriteInfo;

    if (IsContest())
    {
        if (shared19348.unk4_0)
        {
            species = shared19348.unk2;
            personality = shared19348.unk10;
        }
        else
        {
            species = shared19348.unk0;
            personality = shared19348.unk8;
        }
        if (species == SPECIES_UNOWN)
        {
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                var = SPECIES_UNOWN;
            else
                var = letter + SPECIES_UNOWN_B - 1;
            coords = &gMonBackPicCoords[var];
        }
        else if (species == SPECIES_CASTFORM)
        {
            coords = &gCastformFrontSpriteCoords[gBattleMonForms[battlerId]];
        }
        else if (species <= SPECIES_EGG)
        {
            coords = &gMonBackPicCoords[species];
        }
        else
        {
            coords = &gMonBackPicCoords[0];
        }
    }
    else
    {
        if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
            {
                species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
                personality = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
            }
            else
            {
                species = spriteInfo[battlerId].transformSpecies;
                personality = gTransformedPersonalities[battlerId];
            }
            if (species == SPECIES_UNOWN)
            {
                letter = GET_UNOWN_LETTER(personality);
                if (!letter)
                    var = SPECIES_UNOWN;
                else
                    var = letter + SPECIES_UNOWN_B - 1;
                coords = &gMonBackPicCoords[var];
            }
            else if (species > SPECIES_EGG)
            {
                coords = &gMonBackPicCoords[0];
            }
            else
            {
                coords = &gMonBackPicCoords[species];
            }
        }
        else
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
            {
                species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
                personality = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
            }
            else
            {
                species = spriteInfo[battlerId].transformSpecies;
                personality = gTransformedPersonalities[battlerId];
            }
            if (species == SPECIES_UNOWN)
            {
                letter = GET_UNOWN_LETTER(personality);
                if (!letter)
                    var = SPECIES_UNOWN;
                else
                    var = letter + SPECIES_UNOWN_B - 1;
                coords = &gMonFrontPicCoords[var];
            }
            else if (species == SPECIES_CASTFORM)
            {
                coords = &gCastformFrontSpriteCoords[gBattleMonForms[battlerId]];
            }
            else if (species > SPECIES_EGG)
            {
                coords = &gMonFrontPicCoords[0];
            }
            else
            {
                coords = &gMonFrontPicCoords[species];
            }
        }
    }

    switch (a2)
    {
    case 0:
        return (coords->coords & 0xf) * 8;
    case 1:
        return (coords->coords >> 4) * 8;
    case 4:
        return GetBattlerSpriteCoord(battlerId, 2) - ((coords->coords >> 4) * 4);
    case 5:
        return GetBattlerSpriteCoord(battlerId, 2) + ((coords->coords >> 4) * 4);
    case 2:
        return GetBattlerSpriteCoord(battlerId, 3) - ((coords->coords & 0xf) * 4);
    case 3:
        return GetBattlerSpriteCoord(battlerId, 3) + ((coords->coords & 0xf) * 4);
    case 6:
        ret = GetBattlerSpriteCoord(battlerId, 1) + 0x1f;
        return ret - coords->y_offset;
    default:
        return 0;
    }
}

void SetAverageBattlerPositions(u8 battlerId, bool8 a2, s16 *x, s16 *y)
{
    u8 v1, v2;
    s16 v3, v4;
    s16 v5, v6;

    if (!a2)
    {
        v1 = 0;
        v2 = 1;
    }
    else
    {
        v1 = 2;
        v2 = 3;
    }
    v3 = GetBattlerSpriteCoord(battlerId, v1);
    v4 = GetBattlerSpriteCoord(battlerId, v2);
    if (IsDoubleBattle() && !IsContest())
    {
        v5 = GetBattlerSpriteCoord(BATTLE_PARTNER(battlerId), v1);
        v6 = GetBattlerSpriteCoord(BATTLE_PARTNER(battlerId), v2);
    }
    else
    {
        v5 = v3;
        v6 = v4;
    }
    *x = (v3 + v5) / 2;
    *y = (v4 + v6) / 2;
}

u8 sub_80A89C8(int battlerId, u8 spriteId, int species)
{
    u8 newSpriteId = CreateInvisibleSpriteWithCallback(SpriteCallbackDummy);
    gSprites[newSpriteId] = gSprites[spriteId];
    gSprites[newSpriteId].usingSheet = TRUE;
    gSprites[newSpriteId].oam.priority = 0;
    gSprites[newSpriteId].oam.objMode = 2;
    gSprites[newSpriteId].oam.tileNum = gSprites[spriteId].oam.tileNum;
    gSprites[newSpriteId].callback = SpriteCallbackDummy;
    return newSpriteId;
}

void sub_80A8A6C(struct Sprite *sprite)
{
    sub_80A6838(sprite);
    if (GetBattlerSide(gBattleAnimAttacker))
    {
        sprite->pos1.x -= gBattleAnimArgs[0];
        gBattleAnimArgs[3] = -gBattleAnimArgs[3];
        sprite->hFlip = TRUE;
    }
    else
    {
        sprite->pos1.x += gBattleAnimArgs[0];
    }
    sprite->pos1.y += gBattleAnimArgs[1];
    sprite->data[0] = gBattleAnimArgs[2];
    sprite->data[1] = gBattleAnimArgs[3];
    sprite->data[3] = gBattleAnimArgs[4];
    sprite->data[5] = gBattleAnimArgs[5];
    StoreSpriteCallbackInData6(sprite, move_anim_8074EE0);
    sprite->callback = sub_80A66DC;
}

void sub_80A8AEC(struct Sprite *sprite)
{
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        sprite->pos1.x -= gBattleAnimArgs[0];
        gBattleAnimArgs[3] *= -1;
    }
    else
    {
        sprite->pos1.x += gBattleAnimArgs[0];
    }
    sprite->pos1.y += gBattleAnimArgs[1];
    sprite->data[0] = gBattleAnimArgs[2];
    sprite->data[1] = gBattleAnimArgs[3];
    sprite->data[3] = gBattleAnimArgs[4];
    sprite->data[5] = gBattleAnimArgs[5];
    StartSpriteAnim(sprite, gBattleAnimArgs[6]);
    StoreSpriteCallbackInData6(sprite, move_anim_8074EE0);
    sprite->callback = sub_80A66DC;
}

void sub_80A8B64(struct Sprite *sprite)
{
    sub_80A6838(sprite);
    if (GetBattlerSide(gBattleAnimAttacker))
        sprite->pos1.x -= gBattleAnimArgs[0];
    else
        sprite->pos1.x += gBattleAnimArgs[0];
    sprite->pos1.y += gBattleAnimArgs[1];
    sprite->callback = sub_80A67D8;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

void sub_80A8BC4(u8 taskId)
{
    u16 src;
    u16 dest;
    struct Task *task = &gTasks[taskId];

    task->data[0] = GetAnimBattlerSpriteId(ANIM_ATTACKER);
    task->data[1] = ((GetBattlerSide(gBattleAnimAttacker)) != B_SIDE_PLAYER) ? -8 : 8;
    task->data[2] = 0;
    task->data[3] = 0;
    gSprites[task->data[0]].pos2.x -= task->data[0];
    task->data[4] = AllocSpritePalette(10097);
    task->data[5] = 0;

    dest = (task->data[4] + 0x10) * 0x10;
    src = (gSprites[task->data[0]].oam.paletteNum + 0x10) * 0x10;
    task->data[6] = sub_80A82E4(gBattleAnimAttacker);
    if (task->data[6] == 20 || task->data[6] == 40)
        task->data[6] = 2;
    else
        task->data[6] = 3;
    CpuCopy32(&gPlttBufferUnfaded[src], &gPlttBufferFaded[dest], 0x20);
    BlendPalette(dest, 16, gBattleAnimArgs[1], gBattleAnimArgs[0]);
    task->func = sub_80A8CAC;
}

void sub_80A8CAC(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    switch (task->data[2])
    {
    case 0:
        sub_80A8D78(task, taskId);
        gSprites[task->data[0]].pos2.x += task->data[1];
        if (++task->data[3] == 5)
        {
            task->data[3]--;
            task->data[2]++;
        }
        break;
    case 1:
        sub_80A8D78(task, taskId);
        gSprites[task->data[0]].pos2.x -= task->data[1];
        if (--task->data[3] == 0)
        {
            gSprites[task->data[0]].pos2.x = 0;
            task->data[2]++;
        }
        break;
    case 2:
        if (!task->data[5])
        {
            FreeSpritePaletteByTag(ANIM_TAG_BENT_SPOON);
            DestroyAnimVisualTask(taskId);
        }
        break;
    }
}

void sub_80A8D78(struct Task *task, u8 taskId)
{
    s16 spriteId = duplicate_obj_of_side_rel2move_in_transparent_mode(0);
    if (spriteId >= 0)
    {
        gSprites[spriteId].oam.priority = task->data[6];
        gSprites[spriteId].oam.paletteNum = task->data[4];
        gSprites[spriteId].data[0] = 8;
        gSprites[spriteId].data[1] = taskId;
        gSprites[spriteId].data[2] = spriteId;
        gSprites[spriteId].pos2.x = gSprites[task->data[0]].pos2.x;
        gSprites[spriteId].callback = sub_80A8DFC;
        task->data[5]++;
    }
}

void sub_80A8DFC(struct Sprite *sprite)
{
    if (--sprite->data[0] == 0)
    {
        gTasks[sprite->data[1]].data[5]--;
        obj_delete_but_dont_free_vram(sprite);
    }
}

void sub_80A8E30(struct Sprite *sprite)
{
    sprite->pos1.x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->pos1.y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_3);
    if (!GetBattlerSide(gBattleAnimAttacker))
        sprite->data[0] = 5;
    else
        sprite->data[0] = -10;
    sprite->data[1] = -40;
    sprite->callback = sub_80A8E88;
}

void sub_80A8E88(struct Sprite *sprite)
{
    sprite->data[2] += sprite->data[0];
    sprite->data[3] += sprite->data[1];
    sprite->pos2.x = sprite->data[2] / 10;
    sprite->pos2.y = sprite->data[3] / 10;
    if (sprite->data[1] < -20)
        sprite->data[1]++;
    if (sprite->pos1.y + sprite->pos2.y < -32)
        DestroyAnimSprite(sprite);
}

void sub_80A8EE4(struct Sprite *sprite)
{
    int x;
    sprite->data[0] = gBattleAnimArgs[2];
    sprite->data[2] = sprite->pos1.x + gBattleAnimArgs[4];
    sprite->data[4] = sprite->pos1.y + gBattleAnimArgs[5];
    if (!GetBattlerSide(gBattleAnimTarget))
    {
        x = (u16)gBattleAnimArgs[4] + 30;
        sprite->pos1.x += x;
        sprite->pos1.y = gBattleAnimArgs[5] - 20;
    }
    else
    {
        x = (u16)gBattleAnimArgs[4] - 30;
        sprite->pos1.x += x;
        sprite->pos1.y = gBattleAnimArgs[5] - 80;
    }
    sprite->callback = sub_80A6EEC;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}
