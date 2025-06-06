#include "global.h"
#include "gflib.h"
#include "decompress.h"
#include "pokemon_icon.h"
#include "money.h"
#include "item_menu_icons.h"
#include "overworld.h"
#include "overworld_hud.h"
#include "task.h"
#include "party_menu.h"
#include "constants/items.h"

struct OverworldHud
{
    u8 taskId;
    u8 pokemonNameWindowId;
    u8 moneyWindowId;
    u8 buttonWindowId;
    u8 hpBarSpriteId;
    u8 pokeballSpriteIds[PARTY_SIZE];
    u8 itemIconSpriteId;
};

static EWRAM_DATA struct OverworldHud sOverworldHud = {0};

static void Task_OverworldHud(u8 taskId);
static void CreateHudSprites(void);
static void DestroyHudSprites(void);
static void UpdateHud(void);

void CreateOverworldHud(void)
{

    static const struct WindowTemplate sNameWindow = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
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
    FillWindowPixelBuffer(sOverworldHud.buttonWindowId, PIXEL_FILL(1));

    sOverworldHud.taskId = CreateTask(Task_OverworldHud, 80);
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
    UpdateHud();
}

static void CreateHudSprites(void)
{
    u8 i;
    LoadCompressedSpriteSheet(&sSpriteSheet_MenuPokeballSmall);
    LoadCompressedSpritePalette(&sSpritePalette_MenuPokeball);

    for (i = 0; i < PARTY_SIZE; i++)
        sOverworldHud.pokeballSpriteIds[i] = CreateSprite(&sSpriteTemplate_MenuPokeballSmall, i * 8 + 8, 32, 0);

    if (gSaveBlock1Ptr->registeredItem != ITEM_NONE)
        sOverworldHud.itemIconSpriteId = AddItemIconObject(1000, 1000, gSaveBlock1Ptr->registeredItem);
    else
        sOverworldHud.itemIconSpriteId = SPRITE_NONE;

    sOverworldHud.hpBarSpriteId = CreateSprite(&sSpriteTemplate_MenuPokeball, 40, 16, 0); // placeholder
}

static void DestroyHudSprites(void)
{
    u8 i;
    if (sOverworldHud.itemIconSpriteId != SPRITE_NONE)
        DestroySpriteAndFreeResources(&gSprites[sOverworldHud.itemIconSpriteId]);

    for (i = 0; i < PARTY_SIZE; i++)
        if (sOverworldHud.pokeballSpriteIds[i] != SPRITE_NONE)
            DestroySpriteAndFreeResources(&gSprites[sOverworldHud.pokeballSpriteIds[i]]);

    if (sOverworldHud.hpBarSpriteId != SPRITE_NONE)
        DestroySpriteAndFreeResources(&gSprites[sOverworldHud.hpBarSpriteId]);
}

static void UpdateHud(void)
{
    struct Pokemon *mon = &gPlayerParty[0];
    u8 buf[POKEMON_NAME_LENGTH + 1];

    GetMonData(mon, MON_DATA_NICKNAME, buf);
    buf[POKEMON_NAME_LENGTH] = EOS;

    FillWindowPixelBuffer(sOverworldHud.pokemonNameWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized(sOverworldHud.pokemonNameWindowId, FONT_NORMAL, buf, 0, 0, 0, NULL);

    FillWindowPixelBuffer(sOverworldHud.moneyWindowId, PIXEL_FILL(1));
    ConvertIntToDecimalStringN(buf, GetMoney(&gSaveBlock1Ptr->money), STR_CONV_MODE_RIGHT_ALIGN, 6);
    AddTextPrinterParameterized(sOverworldHud.moneyWindowId, FONT_NORMAL, buf, 8, 0, 0, NULL);

    CopyWindowToVram(sOverworldHud.pokemonNameWindowId, COPYWIN_GFX);
    CopyWindowToVram(sOverworldHud.moneyWindowId, COPYWIN_GFX);
}
