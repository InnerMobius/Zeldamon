#include "global.h"
#include "gflib.h"
#include "decompress.h"
#include "battle_interface.h"
#include "pokemon_icon.h"
#include "pokemon.h"
#include "money.h"
#include "item_menu_icons.h"
#include "graphics.h"
#include "overworld.h"
#include "field_message_box.h"
#include "quest_log.h"
#include "constants/quest_log.h"
#include "script.h"
#include "start_menu.h"
#include "item_menu.h"
#include "berry_pouch.h"
#include "overworld_hud.h"
#include "task.h"
#include "party_menu.h"
#include "constants/items.h"
#include "constants/species.h"

#define TAG_OVERWORLD_BALL_TILE 55062
#define TAG_OVERWORLD_BALL_PAL  55063
#define B_INTERFACE_GFX_BALL_PARTY_SUMMARY 66
#define B_INTERFACE_GFX_BALL_CAUGHT 70
#define B_INTERFACE_GFX_HP_BAR_GREEN 3
#define B_INTERFACE_GFX_HP_BAR_YELLOW 47
#define B_INTERFACE_GFX_HP_BAR_RED 56

// Sprite tag values used exclusively by the overworld HUD
#define TAG_HUD_POKEBALL_TILE       0xE500
#define TAG_HUD_POKEBALL_SMALL_TILE 0xE501
#define TAG_HUD_POKEBALL_PAL        0xE502
#define TAG_HUD_ITEM_TILE           0xE503
#define TAG_HUD_ITEM_PAL            0xE504
// Dedicated tags for the registered item icon
#define TAG_HUD_ITEM_ICON_TILE      0xE505
#define TAG_HUD_ITEM_ICON_PAL       0xE506

struct OverworldHud
{
    u8 taskId;
    u8 pokemonNameWindowId;
    u8 moneyWindowId;
    u8 buttonWindowId;
    u8 hpBarSpriteIds[6];
    u8 pokeballSpriteIds[PARTY_SIZE];
    u8 itemIconSpriteId;
    u16 registeredItemId;
        bool8 visible;
};

static EWRAM_DATA struct OverworldHud sOverworldHud = {0};

static void Task_OverworldHud(u8 taskId);
static void CreateHudSprites(void);
static void DestroyHudSprites(void);
static void UpdateHud(void);
static bool8 ShouldShowOverworldHud(void);
static void UpdateHpBar(void);
static void UpdatePartyBallIcons(void);

#define TAG_OW_HP_BAR_GREEN   0x5500
#define TAG_OW_HP_BAR_YELLOW  0x5501
#define TAG_OW_HP_BAR_RED     0x5502
#define TAG_OW_HP_BAR_PAL     0x5503

static const struct OamData sHpBarOamData = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0
};

static const union AnimCmd sHpBarAnim_0[] = {
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_1[] = {
    ANIMCMD_FRAME(1, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_2[] = {
    ANIMCMD_FRAME(2, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_3[] = {
    ANIMCMD_FRAME(3, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_4[] = {
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_5[] = {
    ANIMCMD_FRAME(5, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_6[] = {
    ANIMCMD_FRAME(6, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_7[] = {
    ANIMCMD_FRAME(7, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_8[] = {
    ANIMCMD_FRAME(8, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_9[] = {
    ANIMCMD_FRAME(9, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_10[] = {
    ANIMCMD_FRAME(10, 0),
    ANIMCMD_END
};
static const union AnimCmd sHpBarAnim_11[] = {
    ANIMCMD_FRAME(11, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sHpBarAnimTable[] = {
    sHpBarAnim_0,
    sHpBarAnim_1,
    sHpBarAnim_2,
    sHpBarAnim_3,
    sHpBarAnim_4,
    sHpBarAnim_5,
    sHpBarAnim_6,
    sHpBarAnim_7,
    sHpBarAnim_8,
    sHpBarAnim_9,
    sHpBarAnim_10,
    sHpBarAnim_11
};

static const struct SpriteSheet sHpBarSpriteSheets[] = {
    {gBattleInterface_Gfx[B_INTERFACE_GFX_HP_BAR_GREEN], 12 * TILE_SIZE_4BPP, TAG_OW_HP_BAR_GREEN},
    {gBattleInterface_Gfx[B_INTERFACE_GFX_HP_BAR_YELLOW], 12 * TILE_SIZE_4BPP, TAG_OW_HP_BAR_YELLOW},
    {gBattleInterface_Gfx[B_INTERFACE_GFX_HP_BAR_RED], 12 * TILE_SIZE_4BPP, TAG_OW_HP_BAR_RED},
    {NULL, 0, 0},
};

static const struct SpritePalette sHpBarSpritePalette = {gBattleInterface_Healthbar_Pal, TAG_OW_HP_BAR_PAL};

static const struct SpriteTemplate sHpBarSpriteTemplate = {
    .tileTag = TAG_OW_HP_BAR_GREEN,
    .paletteTag = TAG_OW_HP_BAR_PAL,
    .oam = &sHpBarOamData,
    .anims = sHpBarAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct OamData sOamData_OverworldBall = {
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 0,
};

static const union AnimCmd sAnim_OverworldBall[] = {
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_OverworldBall[] = {
    sAnim_OverworldBall
};

static const struct SpriteSheet sSpriteSheet_OverworldBall = {
    gBattleInterface_Gfx + B_INTERFACE_GFX_BALL_PARTY_SUMMARY,
    4 * TILE_SIZE_4BPP,
    TAG_OVERWORLD_BALL_TILE
};

static const struct SpritePalette sSpritePalette_OverworldBall = {
    gBattleInterface_Healthbar_Pal,
    TAG_OVERWORLD_BALL_PAL
};

static const struct SpriteTemplate sSpriteTemplate_OverworldBall = {
    .tileTag = TAG_OVERWORLD_BALL_TILE,
    .paletteTag = TAG_OVERWORLD_BALL_PAL,
    .oam = &sOamData_OverworldBall,
    .anims = sSpriteAnimTable_OverworldBall,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

void CreateOverworldHud(void)
{

    static const struct WindowTemplate sNameWindow = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 2,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x1F0
    };
    static const struct WindowTemplate sMoneyWindow = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 17,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x210
    };
    static const struct WindowTemplate sButtonWindow = {
        .bg = 0,
        .tilemapLeft = 25,
        .tilemapTop = 1,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x230
    };

    if (FuncIsActiveTask(Task_OverworldHud))
        return;

    sOverworldHud.pokemonNameWindowId = AddWindow(&sNameWindow);
    sOverworldHud.moneyWindowId = AddWindow(&sMoneyWindow);
    sOverworldHud.buttonWindowId = AddWindow(&sButtonWindow);

    PutWindowTilemap(sOverworldHud.pokemonNameWindowId);
    PutWindowTilemap(sOverworldHud.moneyWindowId);
    PutWindowTilemap(sOverworldHud.buttonWindowId);

    FillWindowPixelBuffer(sOverworldHud.pokemonNameWindowId, PIXEL_FILL(1));
    FillWindowPixelBuffer(sOverworldHud.moneyWindowId, PIXEL_FILL(1));
    FillWindowPixelBuffer(sOverworldHud.buttonWindowId, PIXEL_FILL(0));

    sOverworldHud.taskId = CreateTask(Task_OverworldHud, 80);
	sOverworldHud.visible = TRUE;
    CreateHudSprites();
    CopyWindowToVram(sOverworldHud.pokemonNameWindowId, COPYWIN_FULL);
    CopyWindowToVram(sOverworldHud.moneyWindowId, COPYWIN_FULL);
    CopyWindowToVram(sOverworldHud.buttonWindowId, COPYWIN_FULL);
}

void DestroyOverworldHud(void)
{
    if (FuncIsActiveTask(Task_OverworldHud))
    {
        DestroyTask(sOverworldHud.taskId);
        DestroyHudSprites();
        ClearWindowTilemap(sOverworldHud.pokemonNameWindowId);
        ClearWindowTilemap(sOverworldHud.moneyWindowId);
        ClearWindowTilemap(sOverworldHud.buttonWindowId);
        RemoveWindow(sOverworldHud.pokemonNameWindowId);
        RemoveWindow(sOverworldHud.moneyWindowId);
        RemoveWindow(sOverworldHud.buttonWindowId);
        memset(&sOverworldHud, 0, sizeof(sOverworldHud));
    }
}

static void Task_OverworldHud(u8 taskId)
{
    u8 i;
	struct Pokemon *mon = &gPlayerParty[0];
    u16 species = GetMonData(mon, MON_DATA_SPECIES);

    if (ShouldShowOverworldHud() && species != SPECIES_NONE)
    {
        sOverworldHud.visible = TRUE;

        PutWindowTilemap(sOverworldHud.pokemonNameWindowId);
        PutWindowTilemap(sOverworldHud.moneyWindowId);
        PutWindowTilemap(sOverworldHud.buttonWindowId);

        for (i = 0; i < PARTY_SIZE; i++)
            if (sOverworldHud.pokeballSpriteIds[i] != SPRITE_NONE)
                gSprites[sOverworldHud.pokeballSpriteIds[i]].invisible = FALSE;

        if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
            gSprites[sOverworldHud.itemIconSpriteId].invisible = FALSE;

        for (i = 0; i < 6; i++)
            if (sOverworldHud.hpBarSpriteIds[i] != SPRITE_NONE)
                gSprites[sOverworldHud.hpBarSpriteIds[i]].invisible = FALSE;

        UpdateHud();
    }
    else
    {
        sOverworldHud.visible = FALSE;

        ClearWindowTilemap(sOverworldHud.pokemonNameWindowId);
        ClearWindowTilemap(sOverworldHud.moneyWindowId);
        ClearWindowTilemap(sOverworldHud.buttonWindowId);

        for (i = 0; i < PARTY_SIZE; i++)
            if (sOverworldHud.pokeballSpriteIds[i] != SPRITE_NONE)
                gSprites[sOverworldHud.pokeballSpriteIds[i]].invisible = TRUE;

        if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
            gSprites[sOverworldHud.itemIconSpriteId].invisible = TRUE;

        for (i = 0; i < 6; i++)
            if (sOverworldHud.hpBarSpriteIds[i] != SPRITE_NONE)
                gSprites[sOverworldHud.hpBarSpriteIds[i]].invisible = TRUE;
    }
}
	
static void CreateHudSprites(void)
{
    u8 i;
    LoadSpriteSheet(&sSpriteSheet_OverworldBall);
    LoadSpritePalette(&sSpritePalette_OverworldBall);
	LoadSpriteSheets(sHpBarSpriteSheets);
    LoadSpritePalette(&sHpBarSpritePalette);

    for (i = 0; i < PARTY_SIZE; i++)
        sOverworldHud.pokeballSpriteIds[i] = CreateSprite(&sSpriteTemplate_OverworldBall, i * 8 + 8, 38, 0);

    if (gSaveBlock1Ptr->registeredItem != ITEM_NONE)
    {
        sOverworldHud.itemIconSpriteId = AddItemIconObject(TAG_HUD_ITEM_ICON_TILE, TAG_HUD_ITEM_ICON_PAL, gSaveBlock1Ptr->registeredItem);
        if (sOverworldHud.itemIconSpriteId != MAX_SPRITES)
        {
            struct Sprite *icon = &gSprites[sOverworldHud.itemIconSpriteId];
            icon->x = 216;
            icon->y = 18;
        }
        else
        {
            sOverworldHud.itemIconSpriteId = SPRITE_NONE;
        }

        sOverworldHud.registeredItemId = gSaveBlock1Ptr->registeredItem;
    }	
    else
    {
        sOverworldHud.itemIconSpriteId = SPRITE_NONE;
        sOverworldHud.registeredItemId = ITEM_NONE;
    }

    for (i = 0; i < 6; i++)
        sOverworldHud.hpBarSpriteIds[i] = CreateSprite(&sHpBarSpriteTemplate, 8 + i * 8, 8, 0);
	
    UpdatePartyBallIcons();
}

static void DestroyHudSprites(void)
{
    u8 i;
    if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
    {
        DestroySprite(&gSprites[sOverworldHud.itemIconSpriteId]);
        FreeSpriteTilesByTag(TAG_HUD_ITEM_ICON_TILE);
        FreeSpritePaletteByTag(TAG_HUD_ITEM_ICON_PAL);
		sOverworldHud.registeredItemId = ITEM_NONE;
    }

    for (i = 0; i < PARTY_SIZE; i++)
        if (sOverworldHud.pokeballSpriteIds[i] != SPRITE_NONE)
            DestroySpriteAndFreeResources(&gSprites[sOverworldHud.pokeballSpriteIds[i]]);

    for (i = 0; i < 6; i++)
        if (sOverworldHud.hpBarSpriteIds[i] != SPRITE_NONE)
            DestroySpriteAndFreeResources(&gSprites[sOverworldHud.hpBarSpriteIds[i]]);

    FreeSpriteTilesByTag(TAG_OW_HP_BAR_GREEN);
    FreeSpriteTilesByTag(TAG_OW_HP_BAR_YELLOW);
    FreeSpriteTilesByTag(TAG_OW_HP_BAR_RED);
    FreeSpritePaletteByTag(TAG_OW_HP_BAR_PAL);
}

bool8 CanShowOverworldHud(void)
{
    return ShouldShowOverworldHud();
}

static void UpdateHud(void)
{
    struct Pokemon *mon = &gPlayerParty[0];
    u8 buf[POKEMON_NAME_LENGTH + 1];
    u16 species;
    u8 i;

    if (!sOverworldHud.visible)
        return;

    species = GetMonData(mon, MON_DATA_SPECIES);

    if (gSaveBlock1Ptr->registeredItem != sOverworldHud.registeredItemId)
    {
        if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
        {
            DestroySprite(&gSprites[sOverworldHud.itemIconSpriteId]);
            FreeSpriteTilesByTag(TAG_HUD_ITEM_ICON_TILE);
            FreeSpritePaletteByTag(TAG_HUD_ITEM_ICON_PAL);
            sOverworldHud.itemIconSpriteId = SPRITE_NONE;
        }

        if (gSaveBlock1Ptr->registeredItem != ITEM_NONE)
        {
            sOverworldHud.itemIconSpriteId = AddItemIconObject(TAG_HUD_ITEM_ICON_TILE, TAG_HUD_ITEM_ICON_PAL, gSaveBlock1Ptr->registeredItem);
            if (sOverworldHud.itemIconSpriteId != MAX_SPRITES)
            {
                struct Sprite *icon = &gSprites[sOverworldHud.itemIconSpriteId];
                icon->x = 216;
                icon->y = 18;
            }
            else
            {
                sOverworldHud.itemIconSpriteId = SPRITE_NONE;
            }
        }

        sOverworldHud.registeredItemId = gSaveBlock1Ptr->registeredItem;
    }
	
    PutWindowTilemap(sOverworldHud.moneyWindowId);

    FillWindowPixelBuffer(sOverworldHud.moneyWindowId, PIXEL_FILL(1));
    ConvertIntToDecimalStringN(buf, GetMoney(&gSaveBlock1Ptr->money), STR_CONV_MODE_RIGHT_ALIGN, 6);
    AddTextPrinterParameterized(sOverworldHud.moneyWindowId, FONT_NORMAL, buf, 8, 0, 0, NULL);
    CopyWindowToVram(sOverworldHud.moneyWindowId, COPYWIN_GFX);

    if (species == SPECIES_NONE)
    {
        ClearWindowTilemap(sOverworldHud.pokemonNameWindowId);
        ClearWindowTilemap(sOverworldHud.buttonWindowId);
        CopyWindowToVram(sOverworldHud.pokemonNameWindowId, COPYWIN_MAP);
        CopyWindowToVram(sOverworldHud.buttonWindowId, COPYWIN_MAP);

        if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
            gSprites[sOverworldHud.itemIconSpriteId].invisible = TRUE;
        for (i = 0; i < PARTY_SIZE; i++)
            if (sOverworldHud.pokeballSpriteIds[i] != SPRITE_NONE)
                gSprites[sOverworldHud.pokeballSpriteIds[i]].invisible = TRUE;
        for (i = 0; i < 6; i++)
            if (sOverworldHud.hpBarSpriteIds[i] != SPRITE_NONE)
                gSprites[sOverworldHud.hpBarSpriteIds[i]].invisible = TRUE;

        return;
    }


    PutWindowTilemap(sOverworldHud.pokemonNameWindowId);
    PutWindowTilemap(sOverworldHud.moneyWindowId);
    PutWindowTilemap(sOverworldHud.buttonWindowId);

    if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
        gSprites[sOverworldHud.itemIconSpriteId].invisible = FALSE;
    for (i = 0; i < PARTY_SIZE; i++)
        if (sOverworldHud.pokeballSpriteIds[i] != SPRITE_NONE)
            gSprites[sOverworldHud.pokeballSpriteIds[i]].invisible = FALSE;
    for (i = 0; i < 6; i++)
        if (sOverworldHud.hpBarSpriteIds[i] != SPRITE_NONE)
            gSprites[sOverworldHud.hpBarSpriteIds[i]].invisible = FALSE;

    GetMonData(mon, MON_DATA_NICKNAME, buf);
    buf[POKEMON_NAME_LENGTH] = EOS;

    FillWindowPixelBuffer(sOverworldHud.pokemonNameWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized(sOverworldHud.pokemonNameWindowId, FONT_NORMAL, buf, 0, 0, 0, NULL);

    UpdatePartyBallIcons();
	UpdateHpBar();
}

static void UpdateHpBar(void)
{
    struct Pokemon *mon = &gPlayerParty[0];
    u32 curHp = GetMonData(mon, MON_DATA_HP);
    u32 maxHp = GetMonData(mon, MON_DATA_MAX_HP);
    u8 numWholeHpBarTiles = 0;
    u8 animNum;
    s64 pointsPerTile;
    s64 totalPoints;
    u8 i;
    u16 tileTag;

    switch (GetHPBarLevel(curHp, maxHp))
    {
    case HP_BAR_YELLOW:
        tileTag = TAG_OW_HP_BAR_YELLOW;
        break;
    case HP_BAR_RED:
        tileTag = TAG_OW_HP_BAR_RED;
        break;
    default:
        tileTag = TAG_OW_HP_BAR_GREEN;
        break;
    }

    for (i = 0; i < 6; i++)
        gSprites[sOverworldHud.hpBarSpriteIds[i]].oam.tileNum = GetSpriteTileStartByTag(tileTag);

    if (curHp == maxHp)
    {
        for (i = 0; i < 6; i++)
            StartSpriteAnim(&gSprites[sOverworldHud.hpBarSpriteIds[i]], 8);
    }
    else
    {
        pointsPerTile = (maxHp << 2) / 6;
        totalPoints = (curHp << 2);

        while (totalPoints > pointsPerTile)
        {
            totalPoints -= pointsPerTile;
            numWholeHpBarTiles++;
        }

        for (i = 0; i < numWholeHpBarTiles; i++)
            StartSpriteAnim(&gSprites[sOverworldHud.hpBarSpriteIds[i]], 8);

        animNum = (totalPoints * 6) / pointsPerTile;
        StartSpriteAnim(&gSprites[sOverworldHud.hpBarSpriteIds[numWholeHpBarTiles]], animNum);

        for (i = numWholeHpBarTiles + 1; i < 6; i++)
            StartSpriteAnim(&gSprites[sOverworldHud.hpBarSpriteIds[i]], 0);
    }
}

static void UpdatePartyBallIcons(void)
{
    u8 i;
    u16 baseTile = GetSpriteTileStartByTag(TAG_OVERWORLD_BALL_TILE);

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
        if (sOverworldHud.pokeballSpriteIds[i] == SPRITE_NONE)
            continue;

        if (species == SPECIES_NONE)
        {
            gSprites[sOverworldHud.pokeballSpriteIds[i]].oam.tileNum = baseTile + 1;
        }
        else if (GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
        {
            gSprites[sOverworldHud.pokeballSpriteIds[i]].oam.tileNum = baseTile + 2;
        }
        else if (GetMonData(&gPlayerParty[i], MON_DATA_HP) == 0)
        {
            gSprites[sOverworldHud.pokeballSpriteIds[i]].oam.tileNum = baseTile + 3;
        }
        else
        {
            gSprites[sOverworldHud.pokeballSpriteIds[i]].oam.tileNum = baseTile;
        }
    }
}

static bool8 ShouldShowOverworldHud(void)
{
    if (ArePlayerFieldControlsLocked())
        return FALSE;

    if (GetFieldMessageBoxType() != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;

    if (FuncIsActiveTask(Task_StartMenuHandleInput))
        return FALSE;

    if (gBagMenuState.bagOpen)
        return FALSE;

    if (FuncIsActiveTask(Task_ReturnToBagFromContextMenu))
        return FALSE;

    if (FuncIsActiveTask(Task_BerryPouch_DestroyDialogueWindowAndRefreshListMenu))
        return FALSE;

    if (FuncIsActiveTask(Task_HandleChooseMonInput))
        return FALSE;

    if (gQuestLogState == QL_STATE_PLAYBACK)
        return FALSE;

    return TRUE;
}
