#include <ultra64.h>
#include <string.h>

#include "sm64.h"
#include "engine/behavior_script.h"
#include "behavior_data.h"
#include "randomizer.h"
#include "engine/surface_collision.h"
#include "engine/surface_load.h"
#include "memory.h"
#include "area.h"
#include "camera.h"
#include "mario.h"
#include "object_list_processor.h"
#include "engine/math_util.h"
#include "ingame_menu.h"
#include "print.h"
#include "menu/file_select.h"
#include "save_file.h"
#include "buffers/buffers.h"
#include "segment2.h"
#include "game/emutest.h"

u32 Randomizer_gGameSeed = 7224515;

u8 Randomizer_gIsSetSeed = FALSE;

u8 Randomizer_gIgnoreCollisionDistance = FALSE; // hacky

u8 Randomizer_gNumDynamicAvoidancePoints = 0;
struct Randomizer_AvoidancePoint Randomizer_gDynamicAvoidancePoints[200];

s32 Randomizer_curPreset;
struct Randomizer_OptionsSettings Randomizer_gOptionsSettings;

#include "randomizer_data.h"

u8 Randomizer_gRandomSongs[] = { 2,3,4,5,6,7,8,9,12,13,14,17,19,24,25,26,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,47,49,51
    , 52 // sa dilapidated
    , 53 // bm64 blue resort
    , 54 // bm hero radical
    , 55 // kirby yogurt yard
    , 56 // waluigi pinball
    // , 57 // e101
    , 58 // tikal
    , 59 // kirby above the clouds
    , 60 // beach bowl galaxy
    , 61 // pit thwomp
    , 62 // pit holly jolly
    // , 63 free
    , 64 // smrpg ship
    , 65 // pm Forever Forest
    , 66 // mp Full of Danger
    , 67 // stickerbrush symphony
    , 68 // rayman
    , 69 // xnaut
    // , 70 free
    // , 71 free
    , 72 // galaxy rolling
    , 73 // good egg
    , 74 // mp ending
    // , 76 free
    , 77 // galaxy spacejunk
    , 78 // galaxy honey
    , 79 // galaxy battlerock
    , 80 // dkc2 hot head
    , 81 // tty glitz pit
    , 82 // mkdd dry dry desert
    , 83 // pm64 crystal palace
    // , 84 pm64 gusty gulch
    , 85 // ttyd battle theme
    , 86 // pm64 lavalava
    , 87 // pm64 freeze
    // , 88 rayman deep forest
    , 89 // rayman dream forest
    // , 90 rayman dark dare
    , 91 // smg freezeflame
    , 92 // smrpg marrymore
    , 93 // spm lineroad
    , 95 // sa ec2
    , 97 // sm spin dig galaxy
    , 98 // smg2 starshine galaxy
};

static u8 Randomizer_gRandomBossSongs[] = {
    57, /*e101*/
    15, /*mc*/
    46, /*b3 alt*/
    48, /*game over*/
    63, /*dkc2*/
    75, /*pit star hill*/
    84, /*pm64 gusty gulch*/
    94, /*yi title screen*/
    96, /*sa tornado*/
    99, /*bm64 altair*/
};

extern u8 Randomizer_get_random_boss_song()
{
    const int normalTotals = sizeof(Randomizer_gRandomSongs) / sizeof(Randomizer_gRandomSongs[0]) - 0x30;
    const int bossTotals = sizeof(Randomizer_gRandomBossSongs) / sizeof(Randomizer_gRandomBossSongs[0]);
    const int total = bossTotals + normalTotals;
    int which = random_u16_seeded(Randomizer_gGameSeed - (gCurrLevelNum * 0x100)) % total;
    if (which < bossTotals) {
        return Randomizer_gRandomBossSongs[which];
    } else {
        which -= bossTotals;
        return Randomizer_gRandomSongs[which + 0x30];
    }
}

struct SongDescription {
    const char* game;
    const char* song;
};

static const char sBM64[]               = "Bomberman 64";
static const char sBMH[]                = "Bomberman Hero";
static const char sChameleonTwist2[]    = "Chameleon Twist 2";
static const char sDKC2[]               = "Donkey Kong Country 2";
static const char sKA[]                 = "Kirby's Adventure";
static const char sKDL2[]               = "Kirby's Dream Land 2";
static const char sMKDS[]               = "Mario Kart DS";
static const char sMKDD[]               = "Mario Kart: Double Dash!!";
static const char sMP[]                 = "Mario Party";
static const char sMPDS[]               = "Mario Party DS";
static const char sM_LSS[]              = "Mario & Luigi: Superstar Saga";
static const char sM_LPIT[]             = "Mario & Luigi: Partners in Time";
static const char sMetroidPrime[]       = "Metroid Prime";
static const char sPM[]                 = "Paper Mario";
static const char sTTYD[]               = "Paper Mario: The Thousand-Year Door";
static const char sRayman[]             = "Rayman";
static const char sSAdvance[]           = "Sonic Advance";
static const char sSMG[]                = "Super Mario Galaxy";
static const char sSMG2[]               = "Super Mario Galaxy 2";
static const char sSMRPG[]              = "Super Mario RPG: Legend of the Seven Stars";
static const char sSuperMarioSunshine[] = "Super Mario Sunshine";
static const char sSMW[]                = "Super Mario World";
static const char sYI[]                 = "Super Mario World 2: Yoshi's Island";
static const char sSPM[]                = "Super Paper Mario";
static const char sSSB[]                = "Super Smash Bros.";
static const char sSSSV[]               = "Space Station Silicon Valley";
static const char sSA[]                 = "Sonic Adventure";
static const char sWW[]                 = "The Legend of Zelda: The Wind Waker";
static const char sK64[]                = "Kirby 64: The Crystal Shards";

static const struct SongDescription songDescriptions[] = {
    [2 ] = { sSMG            , "Buoy Base Galaxy" },
    [3 ] = { sBM64           , "Green Garden" },
    [4 ] = { sSMG            , "Battlerock Galaxy" },
    [5 ] = { sDKC2           , "In a Snow-Bound Land" },
    [6 ] = { sChameleonTwist2, "Toy Land (Stage 5)" },
    [7 ] = { sPM             , "Dry Dry Desert Trek" },
    [8 ] = { sSAdvance       , "Ice Mountain Zone (Act 1)" },
    [9 ] = { sMKDD           , "Circuit Theme" },
    [12] = { sSMG            , "Waltz of the Boos" },
    [13] = { sSMG            , "Rosalina in the Observatory" },
    [14] = { sSMG            , "Chase the Bunnies!" },
    [15] = { sSSB            , "Meta Crystal" },
    [17] = { sSMG2           , "Honeybloom Galaxy" },
    [19] = { sSMRPG          , "Nimbus Land" },
    [24] = { sSMRPG          , "Beware the Forest's Mushrooms" },
    [25] = { sSSSV           , "The Engine Room" },
    [26] = { sSMG            , "Grand Finale Galaxy" },
    [30] = { sMKDS           , "Desert Hills" },
    [31] = { sSA             , "Windy Hill" },
    [32] = { sSPM            , "The Overthere Stair" },
    [33] = { sSMG            , "Gateway Galaxy" },
    [34] = { sM_LSS          , "Come On, Again!" },
    [35] = { sKA             , "Rainbow Resort" },
    [36] = { sYI             , "Overworld" },
    [37] = { sDKC2           , "Bayou Boogie" },
    [38] = { sSMW            , "Castle & Fortress" },
    [39] = { sRayman         , "Harmony" },
    [40] = { sWW             , "Outset Island" },
    [41] = { sTTYD           , "X-Naut Fortress" },
    [42] = { sMP             , "Bowser's Magma Mountain" },
    [43] = { sMetroidPrime   , "Vs. Parasite Queen" },
    [44] = { sM_LPIT         , "Koopaseum" },
    [46] = { sKDL2           , "Dark Castle" },
    [47] = { sSMG2           , "Starship Mario 1" },
    [48] = { sTTYD           , "Sadness and Happiness" },
    [49] = { sSMG            , "Space Junk Galaxy" },
    [51] = { sKDL2           , "Dark Castle" },
    [52] = { sSA             , "Dilapidated Way ... for Casinopolis" },
    [53] = { sBM64           , "Blue Resort" },
    [54] = { sBMH            , "Hero Radical" },
    [55] = { sKA             , "Yogurt Yard" },
    [56] = { sMKDS           , "Waluigi Pinball" },
    [57] = { sSA             , "E101 MK2 Theme" },
    [58] = { sSA             , "Tikal Theme" },
    [59] = { sK64            , "Above the Clouds" },
    [60] = { sSMG            , "Beach Bowl Galaxy" },
    [61] = { sM_LPIT         , "Thwomp Ruins" },
    [62] = { sM_LPIT         , "Holli Jolli" },
    [63] = { sDKC2           , "Boss Bossanova" },
    [64] = { sSMRPG          , "Sunken Ship" },
    [65] = { sPM             , "Forever Forest" },
    [66] = { sMP             , "Full of Danger" },
    [67] = { sDKC2           , "Stickerbrush Symphony" },
    [68] = { sRayman         , "Harmony" },
    [69] = { sTTYD           , "X-Naut Fortress" },

    [72] = { sSMG            , "Star Ball" },
    [73] = { sSMG            , "Good Egg Galaxy" },
    [74] = { sMPDS           , "Mario Party Ending Theme" },
    [75] = { sM_LPIT         , "Star Hill" },
    [77] = { sSMG            , "Space Junk Galaxy" },
    [78] = { sSMG            , "Honeyhive Galaxy" },
    [79] = { sSMG            , "Battlerock Galaxy" },
    [80] = { sDKC2           , "Hot-Head Bop-a-Boo" },
    [81] = { sTTYD           , "Glitz Pit" },
    [82] = { sMKDD           , "Dry Dry Desert" },
    [83] = { sPM             , "Crystal Palace" },
    [84] = { sPM             , "Gusty Gulch" },
    [85] = { sTTYD           , "Battle Theme" },
    [86] = { sPM             , "Lavalava Island" },
    [87] = { sPM             , "Freeze Flame Core" },
    [89] = { sRayman         , "Dream Forest" },
    [91] = { sSMG            , "Freezeflame Galaxy" },
    [92] = { sSMRPG          , "Marrymore" },
    [93] = { sSPM            , "Lineland Road" },
    [94] = { sYI             , "Title Screen Theme" },
    [95] = { sSA             , "Emerald Coast 2" },
    [96] = { sSA             , "Tornado" },
    [97] = { sSMG2           , "Spin-Dig Galaxy" },
    [98] = { sSMG2           , "Starshine Beach Galaxy" },
    [99] = { sBM64           , "Altair Fight" },
};

extern u8 sCurrentBackgroundMusicSeqId;
void Randomizer_print_cur_song()
{
    if (sCurrentBackgroundMusicSeqId >= sizeof(songDescriptions) / sizeof(*songDescriptions))
        return;

    char buf[200];
    const struct SongDescription* songDesc = &songDescriptions[sCurrentBackgroundMusicSeqId];
    if (!songDesc->game || !songDesc->song)
        return;

    sprintf(buf, "%s - %s", songDesc->game, songDesc->song);
    
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    print_generic_string_aligned(20, 195, buf, TEXT_ALIGN_LEFT);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

const u8 Randomizer_gRandomSongsCount = sizeof(Randomizer_gRandomSongs) / sizeof(Randomizer_gRandomSongs[0]);

struct Randomizer_nodeInfo Randomizer_gLevelWarps[] = {
    // Level        Area  0xF0  0xF1
    [ LEVEL_BBH ] = { LEVEL_CASTLE_GROUNDS, 1, 42, 43, }, // BBH
    [ LEVEL_CCM ] = { LEVEL_CASTLE_GROUNDS, 1, 27, 28, }, // CCM
    [ LEVEL_CASTLE ] = { LEVEL_CASTLE_GROUNDS, 1, 0, 245, }, // Inside Castle
    [ LEVEL_HMC ] = { LEVEL_CASTLE_COURTYARD, 1, 21, 22, }, // HMC
    [ LEVEL_SSL ] = { LEVEL_CASTLE_COURTYARD, 1, 5, 6, }, // SSL
    [ LEVEL_BOB ] = { LEVEL_CASTLE_GROUNDS, 1, 21, 22, }, // BoB
    [ LEVEL_SL ] = { LEVEL_CASTLE_COURTYARD, 1, 12, 13, }, // SL
    [ LEVEL_WDW ] = { LEVEL_CASTLE_COURTYARD, 1, 36, 37, }, // WDW
    [ LEVEL_JRB ] = { LEVEL_CASTLE_GROUNDS, 1, 29, 30, }, // JRB
    [ LEVEL_THI ] = { LEVEL_CASTLE_COURTYARD, 1, 42, 43, }, // THI
    [ LEVEL_TTC ] = { LEVEL_CASTLE, 1, 6, 7, }, // TTC
    [ LEVEL_RR ] = { LEVEL_CASTLE, 1, 9, 11, }, // RR
    [ LEVEL_CASTLE_GROUNDS ] = { LEVEL_CASTLE_GROUNDS, 1, 0, 201, }, // Castle Grounds
    [ LEVEL_BITDW ] = { LEVEL_CASTLE_GROUNDS, 1, 39, 40, }, // BitDW
    [ LEVEL_VCUTM ] = { LEVEL_CASTLE_GROUNDS, 1, 42, 43, }, // VCutM
    [ LEVEL_BITFS ] = { LEVEL_CASTLE_COURTYARD, 1, 27, 28, }, // BitFS
    [ LEVEL_SA ] = { LEVEL_CASTLE_COURTYARD, 0x01, 15, 16, }, // SA
    [ LEVEL_BITS ] = { LEVEL_CASTLE, 1, 3, 4, }, // BitS
    [ LEVEL_LLL ] = { LEVEL_CASTLE_COURTYARD, 1, 24, 25, }, // LLL
    [ LEVEL_DDD ] = { LEVEL_CASTLE_COURTYARD, 1, 18, 19, }, // DDD
    [ LEVEL_WF ] = { LEVEL_CASTLE_GROUNDS, 1, 36, 37, }, // WF
    [ LEVEL_ENDING ] = { LEVEL_CASTLE_GROUNDS, 1, 9, 201, }, // Cake
    [ LEVEL_CASTLE_COURTYARD ] = { LEVEL_CASTLE_GROUNDS, 1, 0, 201, }, // Castle Courtyard
    [ LEVEL_PSS ] = { LEVEL_CASTLE_GROUNDS, 1, 45, 46, }, // PSS
    [ LEVEL_COTMC ] = { LEVEL_CASTLE_GROUNDS, 1, 27, 28, }, // CotMC
    [ LEVEL_TOTWC ] = { LEVEL_CASTLE_COURTYARD, 1, 39, 40, }, // TotWC
    [ LEVEL_WMOTR ] = { LEVEL_CASTLE_GROUNDS, 1, 9, 201, }, // WMotR
    [ LEVEL_TTM ] = { LEVEL_CASTLE_COURTYARD, 1, 39, 40, }, // TTM
    [ LEVEL_BOWSER_1 ] = { LEVEL_CASTLE_GROUNDS, 1, 39, 12, LEVEL_BITDW }, // Bowser 1
    [ LEVEL_BOWSER_2 ] = { LEVEL_CASTLE_COURTYARD, 1, 27, 12, LEVEL_BITFS }, // Bowser 2
};

char *presetStrings[] = {
    "",
    "Extreme",
    "Nonstop",
    "Set Progression",
    "Iron Mario Lite",
    "Iron Mario 80",
    "Iron Mario 130"
};

s32 Randomizer_curPreset = 0;

struct Randomizer_OptionsSettings Randomizer_gPresets[] = {
    {{{0, /* pad */ 0, 0, 1, 0, 1, 0, 1, 1, 1, 1,  8, 0}}, {{1, 1, 1, 1, 1}}},
    {{{0, /* pad */ 0, 1, 1, 1, 1, 0, 2, 1, 1, 1, 10, 0}}, {{2, 1, 1, 1, 2}}},
    {{{0, /* pad */ 0, 0, 1, 0, 1, 1, 1, 1, 1, 1,  8, 0}}, {{1, 1, 1, 1, 1}}},
    {{{0, /* pad */ 0, 1, 1, 1, 1, 0, 1, 1, 1, 1,  8, 0}}, {{1, 1, 1, 1, 1}}},
    {{{0, /* pad */ 1, 1, 1, 1, 1, 0, 2, 1, 1, 1,  5, 0}}, {{1, 1, 1, 1, 1}}},
    {{{0, /* pad */ 1, 1, 1, 1, 1, 0, 2, 1, 1, 1,  8, 0}}, {{1, 1, 1, 1, 1}}},
    {{{0, /* pad */ 1, 1, 1, 1, 1, 0, 2, 1, 1, 1, 12, 0}}, {{1, 1, 1, 1, 1}}},
};

unsigned char textVersion2[] = { "Randomizer 0.1" };

static void print_generic_text_ascii_buf(s16 x, s16 y, const char *str) {
    print_generic_string_aligned(x, y+10, str, TEXT_ALIGN_LEFT);
}

void Randomizer_refreshPreset()
{
    for (size_t i = 0; i < ARRAY_COUNT(Randomizer_gPresets); i++) {
        if (Randomizer_gOptionsSettings.gameplay.w == Randomizer_gPresets[i].gameplay.w) {
            Randomizer_curPreset = i;
            return;
        }
    }

    Randomizer_curPreset = -1;
}

void Randomizer_print_seed_and_options_data(void) {
    char buf[20];
    s32 ypos = 4;
    u32 i;
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    
    sprintf(buf, "%s Seed", (Randomizer_gIsSetSeed ? "Set" : "Random"));
    print_generic_text_ascii_buf(20, ypos + 28, buf);
    sprintf(buf, "Seed: %07d", Randomizer_gGameSeed);
    print_generic_text_ascii_buf(20, ypos + 14, buf);
    
    for (i = 0; i < ARRAY_COUNT(Randomizer_gPresets); i++) {
        if (Randomizer_gOptionsSettings.gameplay.w == Randomizer_gPresets[i].gameplay.w) {
            if ('\0' != *presetStrings[i])
            {
                sprintf(buf, "Preset: %s", presetStrings[i]);
                print_generic_text_ascii_buf(20,ypos,buf);
            }
            goto presetFound; // don't kill me please
        }
    }
    
    sprintf(buf, "Settings ID: %d", Randomizer_gOptionsSettings.gameplay.w);
    print_generic_text_ascii_buf(20,ypos,buf);

presetFound:
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

static s32 find_floor_slipperiness(struct Surface *floor) {
    s32 floorClass = SURFACE_CLASS_DEFAULT;

    if (floor) {
        switch (floor->type) {
            case SURFACE_NOT_SLIPPERY:
            case SURFACE_HARD_NOT_SLIPPERY:
                floorClass = SURFACE_CLASS_NOT_SLIPPERY;
                break;

            case SURFACE_SLIPPERY:
            case SURFACE_NOISE_SLIPPERY:
            case SURFACE_HARD_SLIPPERY:
            case SURFACE_NO_CAM_COL_SLIPPERY:
                floorClass = SURFACE_CLASS_SLIPPERY;
                break;

            case SURFACE_VERY_SLIPPERY:
            case SURFACE_ICE:
            case SURFACE_HARD_VERY_SLIPPERY:
            case SURFACE_NOISE_VERY_SLIPPERY:
                floorClass = SURFACE_CLASS_VERY_SLIPPERY;
                break;
        }
    }

    return floorClass;
}

static u8 rando_floors_general(TerrainData type)
{
    switch (type)
    {
        case SURFACE_NULL:
        case SURFACE_DEFAULT:                  // Environment default
        case SURFACE_0004:                     // Unused, has no function and has parameters
        case SURFACE_VERY_SLIPPERY:            // Very slippery, mostly used for slides
        case SURFACE_SLIPPERY:                 // Slippery
        case SURFACE_NOT_SLIPPERY:             // Non-slippery, climbable
        case SURFACE_HARD:                     // Hard floor (Always has fall damage)
        case SURFACE_HARD_SLIPPERY:            // Hard and slippery (Always has fall damage)
        case SURFACE_HARD_VERY_SLIPPERY:       // Hard and very slippery (Always has fall damage)
        case SURFACE_HARD_NOT_SLIPPERY:        // Hard and Non-slippery (Always has fall damage)
        case SURFACE_ICE:                      // Slippery Ice, in snow levels and THI's water floor
        case SURFACE_HORIZONTAL_WIND:          // Horizontal wind, has parameters
        case SURFACE_FLOWING_WATER:            // Water (flowing), has parameters
        case SURFACE_MGR_MUSIC:                // Plays the Merry go round music, see handle_merry_go_round_music in bbh_merry_go_round.inc.c for more details
        case SURFACE_NOISE_DEFAULT:            // Default floor with noise
        case SURFACE_NOISE_SLIPPERY:           // Slippery floor with noise
        case SURFACE_NOISE_VERY_SLIPPERY:      // Very slippery floor with noise, used in CCM
        case SURFACE_NOISE_VERY_SLIPPERY_73:   // Very slippery floor with noise, unused
        case SURFACE_NOISE_VERY_SLIPPERY_74:   // Very slippery floor with noise, unused
        case SURFACE_CLOSE_CAMERA:             // Close camera
        case SURFACE_WATER:                    // Water, has no action, used on some waterboxes below
        case SURFACE_SHALLOW_QUICKSAND:        // Shallow Quicksand (depth of 10 units)
        case SURFACE_LOOK_UP_WARP:             // Look up and warp (Wing cap entrance)
        case SURFACE_TIMER_START:              // Timer start (Peach's secret slide)
        case SURFACE_TIMER_END:                // Timer stop (Peach's secret slide)
        case SURFACE_BOSS_FIGHT_CAMERA:        // Wide camera for BOB and WF bosses
        case SURFACE_CAMERA_FREE_ROAM:         // Free roam camera for THI and TTC
        case SURFACE_THI3_WALLKICK:            // Surface where there's a wall kick section in THI 3rd area, has no action defined
        case SURFACE_CAMERA_8_DIR:             // Surface that enables far camera for platforms, used in THI
        case SURFACE_CAMERA_MIDDLE:            // Surface camera that returns to the middle, used on the 4 pillars of SSL
        case SURFACE_CAMERA_ROTATE_RIGHT:      // Surface camera that rotates to the right (Bowser 1 & THI)
        case SURFACE_CAMERA_ROTATE_LEFT:       // Surface camera that rotates to the left (BOB & TTM)
        case SURFACE_NO_CAM_COLLISION:         // Surface with no cam collision flag
        case SURFACE_NO_CAM_COLLISION_77:      // Surface with no cam collision flag, unused
        case SURFACE_NO_CAM_COL_VERY_SLIPPERY: // Surface with no cam collision flag, very slippery with noise (THI)
        case SURFACE_NO_CAM_COL_SLIPPERY:      // Surface with no cam collision flag, slippery with noise (CCM, PSS and TTM slides)
        case SURFACE_TTM_VINES:                // TTM vines, has no action defined
        case SURFACE_SWITCH:                   // Surface with no cam collision flag, non-slippery with noise, used by switches and Dorrie
        case SURFACE_VANISH_CAP_WALLS:         // Vanish cap walls, pass through them with Vanish Cap
        case SURFACE_WALL_MISC:                // Used for some walls, Cannon to adjust the camera, and some objects like Warp Pipe
        case SURFACE_HANGABLE:                 // Ceiling that Mario can climb on
        case SURFACE_SLOW:                     // Slow down Mario, unused
            return 1;
        default:
            return 0;
    }
}

static u8 rando_floors_hard(TerrainData type)
{
    switch (type)
    {
        case SURFACE_NULL:
        case SURFACE_DEFAULT:                  // Environment default
        case SURFACE_0004:                     // Unused, has no function and has parameters
        case SURFACE_VERY_SLIPPERY:            // Very slippery, mostly used for slides
        case SURFACE_SLIPPERY:                 // Slippery
        case SURFACE_NOT_SLIPPERY:             // Non-slippery, climbable
        case SURFACE_HARD:                     // Hard floor (Always has fall damage)
        case SURFACE_HARD_SLIPPERY:            // Hard and slippery (Always has fall damage)
        case SURFACE_HARD_VERY_SLIPPERY:       // Hard and very slippery (Always has fall damage)
        case SURFACE_HARD_NOT_SLIPPERY:        // Hard and Non-slippery (Always has fall damage)
        case SURFACE_ICE:                      // Slippery Ice, in snow levels and THI's water floor
        case SURFACE_HORIZONTAL_WIND:          // Horizontal wind, has parameters
        case SURFACE_FLOWING_WATER:            // Water (flowing), has parameters
        case SURFACE_MGR_MUSIC:                // Plays the Merry go round music, see handle_merry_go_round_music in bbh_merry_go_round.inc.c for more details
        case SURFACE_NOISE_DEFAULT:            // Default floor with noise
        case SURFACE_NOISE_SLIPPERY:           // Slippery floor with noise
        case SURFACE_NOISE_VERY_SLIPPERY:      // Very slippery floor with noise, used in CCM
        case SURFACE_NOISE_VERY_SLIPPERY_73:   // Very slippery floor with noise, unused
        case SURFACE_NOISE_VERY_SLIPPERY_74:   // Very slippery floor with noise, unused
        case SURFACE_CLOSE_CAMERA:             // Close camera
        case SURFACE_WATER:                    // Water, has no action, used on some waterboxes below
        case SURFACE_SHALLOW_QUICKSAND:        // Shallow Quicksand (depth of 10 units)
        case SURFACE_LOOK_UP_WARP:             // Look up and warp (Wing cap entrance)
        case SURFACE_TIMER_START:              // Timer start (Peach's secret slide)
        case SURFACE_TIMER_END:                // Timer stop (Peach's secret slide)
        case SURFACE_BOSS_FIGHT_CAMERA:        // Wide camera for BOB and WF bosses
        case SURFACE_CAMERA_FREE_ROAM:         // Free roam camera for THI and TTC
        case SURFACE_THI3_WALLKICK:            // Surface where there's a wall kick section in THI 3rd area, has no action defined
        case SURFACE_CAMERA_8_DIR:             // Surface that enables far camera for platforms, used in THI
        case SURFACE_CAMERA_MIDDLE:            // Surface camera that returns to the middle, used on the 4 pillars of SSL
        case SURFACE_CAMERA_ROTATE_RIGHT:      // Surface camera that rotates to the right (Bowser 1 & THI)
        case SURFACE_CAMERA_ROTATE_LEFT:       // Surface camera that rotates to the left (BOB & TTM)
        case SURFACE_NO_CAM_COLLISION:         // Surface with no cam collision flag
        case SURFACE_NO_CAM_COLLISION_77:      // Surface with no cam collision flag, unused
        case SURFACE_NO_CAM_COL_VERY_SLIPPERY: // Surface with no cam collision flag, very slippery with noise (THI)
        case SURFACE_NO_CAM_COL_SLIPPERY:      // Surface with no cam collision flag, slippery with noise (CCM, PSS and TTM slides)
        case SURFACE_TTM_VINES:                // TTM vines, has no action defined
        case SURFACE_SWITCH:                   // Surface with no cam collision flag, non-slippery with noise, used by switches and Dorrie
        case SURFACE_VANISH_CAP_WALLS:         // Vanish cap walls, pass through them with Vanish Cap
        case SURFACE_WALL_MISC:                // Used for some walls, Cannon to adjust the camera, and some objects like Warp Pipe
        case SURFACE_HANGABLE:                 // Ceiling that Mario can climb on
        case SURFACE_SLOW:                     // Slow down Mario, unused

        case SURFACE_BURNING:                  // Lava / Frostbite (in SL), but is used mostly for Lava
        case SURFACE_DEEP_QUICKSAND:           // Quicksand (lethal, slow, depth of 160 units)
        case SURFACE_INSTANT_QUICKSAND:        // Quicksand (lethal, instant)
        case SURFACE_DEEP_MOVING_QUICKSAND:    // Moving quicksand (flowing, depth of 160 units)
        case SURFACE_SHALLOW_MOVING_QUICKSAND: // Moving quicksand (flowing, depth of 25 units)
        case SURFACE_QUICKSAND:                // Moving quicksand (60 units)
        case SURFACE_MOVING_QUICKSAND:         // Moving quicksand (flowing, depth of 60 units)
        case SURFACE_INSTANT_MOVING_QUICKSAND: // Quicksand (lethal, flowing)
            return 1;
        default:
            return 0;
    }
}

static u8 is_floor_safe(struct Surface *floor, u8 floorSafeLevel,
                        u32 randPosFlags) { // Checks if floor triangle can be spawned on
    s32 slipperiness;
    f32 norm;

    if (floor->flags & SURFACE_FLAG_DYNAMIC)
        return FALSE; // grounded objects / DDD objects can't spawn on platforms

    switch(Randomizer_gOptionsSettings.gameplay.s.safeSpawns){
        case Randomizer_SPAWN_SAFETY_SAFE:
            norm = 0.85f;
            break;
        case Randomizer_SPAWN_SAFETY_HARD:
            norm = 0.3f;
            break;
        default:
            switch (gCurrLevelNum)
            {
                case LEVEL_LLL:
                    norm = 0.83f;
                    break;
                case LEVEL_SL:
                    norm = 0.78f;
                    break;
                case LEVEL_THI:
                    norm = 0.891f;
                    break;
                case LEVEL_TTC:
                    norm = 0.71f;
                    break;
                default:
                    norm = 0.7f;
                    break;
            }
    }

    slipperiness = find_floor_slipperiness(floor);
    if (SURFACE_CLASS_NOT_SLIPPERY == slipperiness)
    {
        norm = 0.4f;
    }

    if ((floorSafeLevel == Randomizer_FLOOR_SAFETY_HIGH) || (randPosFlags & RAND_TYPE_SAFE)) {
        norm = 0.95f;
    }

    if (floor->normal.y <= norm) {
        return FALSE;
    }

    if ((randPosFlags & RAND_TYPE_SAFE) && (floorSafeLevel == Randomizer_FLOOR_SAFETY_HIGH)
        && ((slipperiness == SURFACE_CLASS_SLIPPERY) || (slipperiness == SURFACE_CLASS_VERY_SLIPPERY))) {

        // This code kills some spawns, assuming the most slippery case. This code would
        // probably be better to refactor based off slipperiness in general.
        if (floor->normal.y <= 0.99f) {
            return FALSE; // Don't spawn on slippery surfaces if you are a warp or safe object
        }
    }

    if (rando_floors_general(floor->type)) {
        return TRUE;
    }

    if ((Randomizer_gOptionsSettings.gameplay.s.safeSpawns == Randomizer_SPAWN_SAFETY_HARD) && (floorSafeLevel == Randomizer_FLOOR_SAFETY_LOW) && !(randPosFlags & RAND_TYPE_SAFE)) {
        if (rando_floors_hard(floor->type)) {
            return TRUE;
        }
    }

    return FALSE;
}

// Checks if near a specific avoidance point
static u32 check_avoidance_point(Vec3s pos, BehaviorScript* bhv, const struct Randomizer_AvoidancePoint *avoidancePoint) {
    const void *behavior = avoidancePoint->behavior;
        
    if(((avoidancePoint->safety == Randomizer_AVOIDANCE_SAFETY_ALL) 
    || ((avoidancePoint->safety == Randomizer_AVOIDANCE_SAFETY_MED) && (Randomizer_gOptionsSettings.gameplay.s.safeSpawns == Randomizer_SPAWN_SAFETY_SAFE))
    || ((avoidancePoint->safety == Randomizer_AVOIDANCE_SAFETY_HARD) && (Randomizer_gOptionsSettings.gameplay.s.safeSpawns != Randomizer_SPAWN_SAFETY_HARD)))){
    } else {
        return FALSE;
    }

    if (behavior != bhvStub && bhv != behavior) {
        return FALSE;
    }

    if ((sqr(pos[0] - avoidancePoint->pos[0]) + sqr(pos[2] - avoidancePoint->pos[2]) < sqr(avoidancePoint->radius))
        && (pos[1] > avoidancePoint->pos[1]) && (pos[1] < avoidancePoint->pos[1] + avoidancePoint->height)) {
        return TRUE;
    }
    return FALSE;
}

// Checks if near any avoidance point
static u32 is_in_avoidance_point(Vec3s pos, const struct Randomizer_AreaParams *areaParams, BehaviorScript* bhv) {
    const struct Randomizer_AvoidancePoint *avoidancePoint;

    for (u32 i = 0; i < areaParams->numAvoidancePoints; i++) {
        avoidancePoint = &(*areaParams->avoidancePoints)[i];
        if (check_avoidance_point(pos, bhv, avoidancePoint)) {
            return TRUE;
        }
    }
    for (u32 i = 0; i < Randomizer_gNumDynamicAvoidancePoints; i++) {
        if (check_avoidance_point(pos, bhv, &Randomizer_gDynamicAvoidancePoints[i])) {
            return TRUE;
        }
    }

    if (gCurrCourseNum == COURSE_JRB)
    {
        if ((-5267 < pos[0]) && (pos[0] < -4499) && (-3792 < pos[2]) && (pos[2] < -2640) && pos[1] < 1293) {
            return TRUE;
        }
    }

    return FALSE;
}

u32 Randomizer_raycast_wall_check(Vec3s pos) {
    s16 yaw = 0;
    struct Surface *surf;
    Vec3f checkPos;
    vec3_copy_y_off(checkPos, pos, 50.0f);
    Vec3f hitPos;

    for (u32 i = 0; i < 8; i++) {
        Vec3f rayDir;
        vec3f_set(rayDir, 0x10000 * sins(yaw), 0, 0x10000 * coss(yaw));

        find_surface_on_ray(checkPos, rayDir, &surf, hitPos,
            (RAYCAST_FIND_FLOOR | RAYCAST_FIND_WALL | RAYCAST_FIND_CEIL));

        if (surf != NULL) {
            f32 det = vec3f_dot(&surf->normal.x, rayDir);
            if (det > 0) {
                return FALSE;
            }
        }

        yaw += 0x2000;
    }
    return TRUE;
}

static void vec3s_resolve_wall_collisions(Vec3s pos, f32 radius) {
    Vec3f pos2;
    
    vec3s_to_vec3f(pos2, pos);
    f32_find_wall_collision(&pos2[0], &pos2[1], &pos2[2], 0.0f, radius);
    vec3f_to_vec3s(pos, pos2);
}

void Randomizer_create_dynamic_avoidance_point(Vec3f pos, f32 radius, f32 height, f32 downOffset) {
    struct Randomizer_AvoidancePoint *newPoint = &Randomizer_gDynamicAvoidancePoints[Randomizer_gNumDynamicAvoidancePoints];
    newPoint->pos[0] = pos[0];
    newPoint->pos[1] = pos[1] - downOffset;
    newPoint->pos[2] = pos[2];
    newPoint->radius = radius;
    newPoint->height = height;
    newPoint->safety = Randomizer_AVOIDANCE_SAFETY_ALL;
    newPoint->behavior = bhvStub;
    Randomizer_gNumDynamicAvoidancePoints++;
}

// #define DEBUG_FAIRNESS
#ifdef DEBUG_FAIRNESS
int gFailReasons[30] = {0};
#define LOG_FAIL(idx) do { gFailReasons[(idx) + 1]++; } while(0)
#else
#define LOG_FAIL(idx) do {} while(0)
#endif

extern const BehaviorScript bhvStarRoadGGGrave[];
static void Randomizer_get_safe_position_impl(const BehaviorScript* bhv, Vec3s pos, f32 minHeightRange, f32 maxHeightRange, tinymt32_t *randomState,
                       u8 floorSafeLevel, u32 randPosFlags) {
    const struct Randomizer_AreaParams *areaParams = &(*Randomizer_sLevelParams[gCurrLevelNum - 4])[gCurrAreaIndex - 1];
    f32 minX, maxX, minY, maxY, minZ, maxZ, minHeight, maxHeight, waterLevel, lowFloorHeight, cHeight,
        highFloorHeight;
    u32 objCanBeUnderwater;
    struct Surface *lowFloor, *ceil, *highFloor;

    f32 wallRadius = 50.f;
    if (randPosFlags & RAND_TYPE_SPAWN_FAR_FROM_WALLS) {
        wallRadius = 300.f;
    }

    if (gCurrLevelNum == LEVEL_BOWSER_2) {
        pos[0] = 0;
        pos[1] = 2310;
        pos[2] = 1935;
        return;
    }
    if (areaParams == NULL) {
        pos[0] = 0;
        pos[1] = 5000;
        pos[2] = 0;
        return;
    }

    if (Randomizer_gOptionsSettings.gameplay.s.nonstopMode == 1) {
        if ((bhv == bhvStar)
         || (bhv == bhvStarSpawnCoordinates)
         || (bhv == bhvHiddenRedCoinStar)
         || (bhv == bhvHiddenStar)) {
            floorSafeLevel = Randomizer_FLOOR_SAFETY_MEDIUM;
            randPosFlags |= RAND_TYPE_SAFE;
        }
    }

    while (TRUE) {
        minX = areaParams->minX;
        maxX = areaParams->maxX;
        minY = areaParams->minY;
        maxY = areaParams->maxY;
        minZ = areaParams->minZ;
        maxZ = areaParams->maxZ;

        if(bhvMario == bhv)
        {
            if (gCurrCourseNum == COURSE_HMC)
            {
                int zone = Randomizer_get_val_in_range_uniform(0, 3, randomState);
                switch (zone)
                {
                    case 0: minY = 1743;
                    break;
                    case 1: minY = -2577;
                    break;
                    case 2:
                    break;
                }
            }
            if (gCurrCourseNum == COURSE_CCM)
            {
                int zone = Randomizer_get_val_in_range_uniform(0, 3, randomState);
                switch (zone)
                {
                    case 0: minY = -441;
                    break;
                    case 1: minY = -4251;
                    break;
                    case 2:
                    break;
                }
            }
        }

        if (bhvHiddenStarTrigger == bhv && gCurrCourseNum == COURSE_RR)
        {
            minY = 0;
        }

        u32 dangerShiftedOverHighFloor = FALSE;

        // Generate random position
        pos[1] = Randomizer_get_val_in_range_uniform(minY, maxY, randomState);

        if (gCurrCourseNum == COURSE_HMC)
        {
            if (pos[1] < 1743)
            {
                minX = -3773;
                //maxX = areaParams->maxX;
                minZ = -7136;
                maxZ = 6126;
            }
            if (pos[1] < -2577)
            {
                minX = -2763;
                //maxX = areaParams->maxX;
                minZ = -2563;
                maxZ = 5054;
            }
        }

        if (gCurrCourseNum == COURSE_CCM)
        {
            if (pos[1] < -441)
            {
                minX = -6159;
                maxX = 5706;
                minZ = -4421;
                maxZ = 5572;
            }
            if (pos[1] < -4251)
            {
                maxZ = 6531;
                minZ = 3016;
                maxX = 4233;
                minX = 1755;
            }
        }

        if (gCurrCourseNum == COURSE_SSL)
        {
            if (pos[1] > 780)
            {
                minX = -2362;
                maxX = 2279;
                minZ = 1024;
                maxZ = 3924;
            }
            if (pos[1] < -1350)
            {
                minX = -2911;
                maxX = 604;
                minZ = 1436;
                maxZ = 4330;
            }
        }

        if (gCurrCourseNum == COURSE_RR)
        {
            if (pos[1] < -1980)
            {
                minX = -7576;
                maxX = 6152;
                minZ = -7096;
                maxZ = 3719;
            }

            if (pos[1] > 3255)
            {
                minX = -2609;
                maxX = -870;
                minZ = -6062;
                maxZ = -357;
            }

            if (pos[1] > 4164)
            {
                minX = -4336;
                maxX = 1980;
                minZ = -967;
                maxZ = 7471;
            }

            if (pos[1] > 6234)
            {
                minX = -8190;
                maxX = 5658;
                minZ = -601;
                maxZ = 7471;
            }
        }

        if (gCurrCourseNum == COURSE_PSS)
        {
            if (pos[1] > 1620)
            {
                minZ = 144;
                maxZ = 7183;   
            }
            if (pos[1] < -870)
            {
                maxZ = 3065;   
            }
        }

        if (gCurrCourseNum == COURSE_BOB)
        {
            if (pos[1] < -2400)
            {
                maxZ = 1148;
                maxX = 5587;
            }
            if (pos[1] > -120)
            {
                minZ = -255;
                minX = -5273;
            }
        }

        if (gCurrCourseNum == COURSE_TOTWC)
        {
            if (pos[1] > 2190)
            {
                minZ = -5140;
                maxZ = 3949;
                minX = -5572;
                maxX = -1787;
            }
        }

        // For courses like CCCoral special handling is used. We prioritize randomness within Y coordinates fairness.
        // Goal is to discover the spot where on a given height there is _possibly_ a floor instead of rejecting Y and rerolling it.
        int wantYFairness = gCurrCourseNum == COURSE_HMC || gCurrCourseNum == COURSE_RR;
        if (wantYFairness)
        {
            do
            {
                pos[0] = Randomizer_get_val_in_range_uniform(minX, maxX, randomState);
                pos[2] = Randomizer_get_val_in_range_uniform(minZ, maxZ, randomState);

                lowFloorHeight = find_floor(pos[0], pos[1] + 20, pos[2], &lowFloor);
                LOG_FAIL(-1); 
            }
            while (!lowFloor || lowFloor->type == SURFACE_DEATH_PLANE);
        }
        else
        {
            pos[0] = Randomizer_get_val_in_range_uniform(minX, maxX, randomState);
            pos[2] = Randomizer_get_val_in_range_uniform(minZ, maxZ, randomState);

            lowFloorHeight = find_floor(pos[0], pos[1] + 20, pos[2], &lowFloor);
            if (lowFloor == NULL) { LOG_FAIL(0); continue; }
        }

        int lowDiff = 800;
        if ((pos[1] - lowFloorHeight) > lowDiff) { LOG_FAIL(1); continue; }

        if (lowFloorHeight + 20 <= maxY) {
            pos[1] = lowFloorHeight + 20;
        }

        // Move out of any walls. This has to be done here because otherwise
        // there's the possibility of being pushed out of the wall into OoB or a ceiling
        vec3s_resolve_wall_collisions(
            pos, wallRadius);

        lowFloorHeight = find_floor(pos[0], pos[1], pos[2], &lowFloor);

        if ((pos[1] - lowFloorHeight) > lowDiff) { LOG_FAIL(2); continue; }

        pos[1] = lowFloorHeight;

        if (lowFloor == NULL) { LOG_FAIL(3); continue; }

        if (!is_floor_safe(lowFloor, floorSafeLevel, randPosFlags)) { LOG_FAIL(4); continue; }

        // Snap to ground and check if safe
        objCanBeUnderwater =
            (randPosFlags
                 & (RAND_TYPE_CAN_BE_UNDERWATER | RAND_TYPE_MUST_BE_UNDERWATER));
        waterLevel = find_water_level(pos[0], pos[2]);
        minHeight = pos[1] + minHeightRange;
        maxHeight = pos[1] + maxHeightRange;

        // Let objects spawn anywhere in water
        if (floorSafeLevel != Randomizer_FLOOR_SAFETY_HIGH
            || (randPosFlags & RAND_TYPE_MUST_BE_UNDERWATER)) {
            if ((objCanBeUnderwater && (waterLevel > maxHeight))
                || (randPosFlags & RAND_TYPE_MUST_BE_UNDERWATER))
                {
                    maxHeight = waterLevel;
                }
        }

        // For the start warp, always spawn above the water
        if ((bhv == bhvSpinAirborneWarp) && (waterLevel > pos[1])) {
            minHeight = waterLevel + minHeightRange;
            maxHeight = waterLevel + maxHeightRange;
        }

        if (gCurrCourseNum == COURSE_HMC || gCurrCourseNum == COURSE_JRB || (gCurrCourseNum == COURSE_CCM && pos[1] < -4000.f))
        {
            cHeight = find_ceil(pos[0], pos[1], pos[2], &ceil) - 100.f;
            if (cHeight < maxHeight) {
                maxHeight = cHeight;
            }
            if (maxHeight < minHeight) {
                minHeight = pos[1];
            }

            if (maxHeight < minHeight) {
                LOG_FAIL(14); continue;
            }
        }

#if 0
        // Prevent objects from spawning too high above water in BBH
        if ((gCurrCourseNum == COURSE_BBH) && (pos[1] < waterLevel) && (maxHeight > waterLevel))
            maxHeight = waterLevel + 100.f;
#endif

        // Check if max height has gone above the level bounds
        if (maxHeight > maxY) {
            maxHeight = maxY;
        }

        pos[1] = Randomizer_get_val_in_range_uniform(minHeight, maxHeight, randomState);

        if ((Randomizer_gOptionsSettings.gameplay.s.safeSpawns == Randomizer_SPAWN_SAFETY_HARD) &&
            (floorSafeLevel == Randomizer_FLOOR_SAFETY_LOW) && !(randPosFlags & RAND_TYPE_SAFE)) {
            Vec3f oldPos;
            vec3s_to_vec3f(oldPos, pos);
            pos[0] += Randomizer_get_val_in_range_uniform(-200, 200, randomState);
            pos[2] += Randomizer_get_val_in_range_uniform(-200, 200, randomState);

            vec3s_resolve_wall_collisions(
                pos, wallRadius);

            // Make sure it doesnt shift through a surface
            Vec3f rayDir;
            vec3s_to_vec3f(rayDir, pos);
            vec3f_sub(rayDir, oldPos);
            struct Surface *surf;
            Vec3f hitPos;
            find_surface_on_ray(oldPos, rayDir, &surf, hitPos,
            (RAYCAST_FIND_FLOOR | RAYCAST_FIND_WALL | RAYCAST_FIND_CEIL));

            if (surf != NULL) {
                LOG_FAIL(5); continue;
            }
            
            waterLevel = find_water_level(pos[0], pos[2]);

            lowFloorHeight = find_floor(pos[0], pos[1], pos[2], &lowFloor);
            if (lowFloor == NULL) {
                LOG_FAIL(6); continue;
            }
            if ((pos[1] - lowFloorHeight) > 800.f) {
                dangerShiftedOverHighFloor = TRUE;
            }
        }

        // Start checking if position is valid

        // Ceiling check
        cHeight = find_ceil(pos[0], lowFloorHeight + 80, pos[2], &ceil);

        if (pos[1] > cHeight - 100.f) { LOG_FAIL(7); continue; } // If in a ceiling, cancel spawn

        if (dangerShiftedOverHighFloor & (pos[1] > cHeight - 200.f)) { LOG_FAIL(8); continue; } // If no ground nearby and too close to the ceiling

        // Floor Check
        highFloorHeight = find_floor(pos[0], cHeight - 80, pos[2],
                                     &highFloor); // Find floor under object assuming 80 units of space

        if ((highFloorHeight > (pos[1] + 20))
            && ((highFloorHeight - pos[1])
                < 1500)) { LOG_FAIL(9); continue; } // If under floor and not large distance, deny height

        if ((pos[1] - highFloorHeight) < (minHeightRange - 50.f)) {
            LOG_FAIL(10); continue;
        }

        if (!objCanBeUnderwater && (waterLevel > pos[1])) { LOG_FAIL(11); continue; }

        if ((randPosFlags & RAND_TYPE_MUST_BE_UNDERWATER) && (waterLevel < pos[1])) { LOG_FAIL(12); continue; }

#if 0
        if (randPosFlags & RAND_TYPE_LIMITED_BBH_HMC_SPAWNS) {
            if ((gCurrCourseNum == COURSE_BBH) && (lowFloor->room == 9)) {
                continue;
            } else if ((gCurrCourseNum == COURSE_HMC) && (lowFloor->room == 8)) {
                continue;
            }
        }
#endif
        if (gCurrLevelNum == LEVEL_HMC)
        {
            f32 df = lowFloorHeight - 2073.f;
            if (absf(df) < 10.f)
            {
                // is covered by ground?
                {
                    struct Surface* t = NULL;
                    f32 height = find_floor(pos[0], 8000.f, pos[2], &t);
                    if (height - 2073.f > 10.f) { LOG_FAIL(13); continue; }
                }

                // raycast to the left to check if covered by wall - first find the rock location...
                Vec3f leftPos;
                Vec3f loc = { pos[0] , pos[1] + 10.f, pos[2] };
                {
                    Vec3f r1 = { -8000.f, pos[1] + 10.f, pos[2] };
                    Vec3f dir;
                    vec3_diff(dir, r1, loc);
                    struct Surface* surf = NULL;
                    find_surface_on_ray(loc, dir, &surf, leftPos, (RAYCAST_FIND_FLOOR | RAYCAST_FIND_WALL | RAYCAST_FIND_CEIL));
                }

                // ... and then raycast down from there to check if it's a wall we are hitting
                Vec3f hitPos;
                {
                    leftPos[0]++;
                    Vec3f dir;
                    vec3_diff(dir, loc, leftPos);
                    struct Surface* surf = NULL;
                    find_surface_on_ray(leftPos, dir, &surf, hitPos, (RAYCAST_FIND_FLOOR | RAYCAST_FIND_WALL | RAYCAST_FIND_CEIL));
                }

                f32 dx = hitPos[0] - loc[0];
                if (dx < -10.f) { LOG_FAIL(14); continue; }
            }

            // underwater objects must never be higher than ceiling, otherwise clamping will occur
            if (pos[1] < 2803.f)
            {
                struct Surface* surf = NULL;
                f32 height = find_ceil(pos[0], lowFloorHeight, pos[2], &surf);
                if (pos[1] > height)
                {
                    LOG_FAIL(14); continue;
                }
            }
        }

        if (gCurrLevelNum == LEVEL_THI)
        {
            if (lowFloor->type == SURFACE_BURNING)
            {
                // check if lava is covered by floor - do not spawn anything there
                struct Surface* t;
                gCollisionFlags |= COLLISION_FLAG_EXCLUDE_DYNAMIC;
                f32 h = find_ceil(pos[0], pos[1], pos[2], &t);
                h = find_floor(pos[0], h, pos[2], &t);
                if (pos[1] < h)
                {
                    LOG_FAIL(14); continue;
                }
            }
        }

        if (is_in_avoidance_point(pos, areaParams, bhv)) { LOG_FAIL(15); continue; }

        // Wall Check
        if (!Randomizer_raycast_wall_check(pos)) { LOG_FAIL(16); continue; }

        // Spawn avoidance point if needed
        if ((randPosFlags & RAND_TYPE_CREATE_AVOIDANCE_POINT) && (Randomizer_gNumDynamicAvoidancePoints < 200)) {
            Vec3f fpos;
            vec3_copy(fpos, pos);
            f32 radius = 100.f;
            f32 height = 200.f;
            if (bhv == bhvStarRoadGGGrave || bhv == bhvPushableMetalBox)
            {
                radius *= 3.f;
                height *= 1.5f;
            }
            Randomizer_create_dynamic_avoidance_point(fpos, radius, height, 50.f);
        }

        return;
    }
}

void Randomizer_get_safe_position(const BehaviorScript* bhv, Vec3s pos, f32 minHeightRange, f32 maxHeightRange, tinymt32_t *randomState,
                       u8 floorSafeLevel, u32 randPosFlags)
{
    if (randPosFlags & RAND_TYPE_HAS_SAFE_GROUND_AROUND)
    {
        int ok = 0;
        while (!ok)
        {
            Randomizer_get_safe_position_impl(bhv, pos, minHeightRange, maxHeightRange, randomState, floorSafeLevel, randPosFlags);
            f32 floorHeight;
            {
                struct Surface* floor = NULL;
                gCollisionFlags |= COLLISION_FLAG_EXCLUDE_DYNAMIC;
                floorHeight = find_floor(pos[0], pos[1] + 20.f, pos[2], &floor);
                if (!floor)
                {
                    ok = 0;
                    continue;
                }
            }

            ok = 1;
            for (int i = 0; i < 8; i++)
            {
                struct Surface* dfloor;
                const f32 r = bhv == bhvMrI ? 300.f : 60.f;
                f32 dx = r * sins(0x2000 * i);
                f32 dz = r * coss(0x2000 * i);
                gCollisionFlags |= COLLISION_FLAG_EXCLUDE_DYNAMIC;
                f32 dheight = find_floor(pos[0] + dx
                                    , pos[1] + 20.f
                                    , pos[2] + dz, &dfloor);
                if (!dfloor
                    || dfloor->type == SURFACE_BURNING
                    || dfloor->type == SURFACE_INSTANT_QUICKSAND
                    || dfloor->type == SURFACE_DEATH_PLANE)
                {
                    ok = 0;
                    break;
                }

                if (dheight + 100.f < floorHeight)
                {
                    ok = 0;
                    break;
                }
            }
        }
    }
    else
    {
        Randomizer_get_safe_position_impl(bhv, pos, minHeightRange, maxHeightRange, randomState, floorSafeLevel, randPosFlags);
    }
}

void Randomizer_get_safe_position_obj(const struct Object* obj, Vec3s pos, f32 minHeightRange, f32 maxHeightRange, tinymt32_t *randomState, u8 floorSafeLevel, u32 randPosFlags)
{
    if (obj->behavior == bhvCoinFormation)
    {
        s32 snapToGround = TRUE;
        int shape = obj->oBehParams2ndByte;

        switch (shape & COIN_FORMATION_BP_SHAPE_MASK) {
            case COIN_FORMATION_BP_SHAPE_HORIZONTAL_LINE:
                break;
            case COIN_FORMATION_BP_SHAPE_VERTICAL_LINE:
                snapToGround = FALSE;
                break;
            case COIN_FORMATION_BP_SHAPE_HORIZONTAL_RING:
                break;
            case COIN_FORMATION_BP_SHAPE_VERTICAL_RING:
                snapToGround = FALSE;
                break;
            case COIN_FORMATION_BP_SHAPE_ARROW:
                break;
        }

        if (snapToGround) {
            randPosFlags |= RAND_TYPE_GROUNDED;
        }
    }

    Randomizer_get_safe_position(obj->behavior, pos, minHeightRange, maxHeightRange, randomState, floorSafeLevel, randPosFlags);
}

// Only uniform if used for floats. [min, max)
f32 Randomizer_get_val_in_range_uniform(f32 min, f32 max, tinymt32_t *randomState) {
    if (min > max)
        return min;
    return (tinymt32_generate_float(randomState) * (max - min)) + min;
}

static u16 calulate_star_total(u32 level) {
    switch (Randomizer_gWarpDestinations[level]) {
        case LEVEL_SA:
        case LEVEL_PSS:
            return 2;
        
        case LEVEL_WMOTR:
            return 1;
        
        case LEVEL_BBH:
            return 7 + calulate_star_total(LEVEL_VCUTM);
        case LEVEL_CCM:
            return 7 + calulate_star_total(LEVEL_COTMC);
        case LEVEL_TTM:
            return 7 + calulate_star_total(LEVEL_TOTWC);

        case LEVEL_BITDW:
        case LEVEL_TOTWC:
        case LEVEL_COTMC:
        case LEVEL_BITFS:
        case LEVEL_VCUTM:
        case LEVEL_BITS:
        case LEVEL_ENDING:
            return 1;

        default:
            return 7;
    }
}

// Get the maximum requirement for a star door based on:
// BitS requirement
// If no keep structure, layer (0 for easily accessible doors, 1 for doors behind other doors)
// If keep structure, section (0 for lobby, 1 for basement, 2 for upstairs)
// Min factor (usually / 2 for important doors and / 3 for most )
// Maximum stars available at this point, if lower than regular max
static u8 get_star_requirement(u8 layer, u8 section, u8 maxAvailable, u8 factor, tinymt32_t *randomState) {
    u8 bitsStars = Randomizer_gStarDoorReqLUT[Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement];
    u8 maxStars = 0;
    u8 starReq;

    if (bitsStars == 0) bitsStars = 80;

    if (Randomizer_gOptionsSettings.gameplay.s.keepStructure) {
        switch (section) {
        case 0:
            maxStars = (u8)(bitsStars*0.2f); // lobby
            break;
        case 1:
            maxStars = (u8)(bitsStars*0.4f); // basement
            break;
        case 2:
            maxStars = (u8)(bitsStars*0.7f); // upstairs
        }
    } else {
        if (layer == 0) {
            maxStars = (u8)(bitsStars*0.35f); // layer 1
        } else {
            maxStars = (u8)(bitsStars*0.8f); // layer 2
        }
    }
    starReq = Randomizer_get_val_in_range_uniform(maxStars / factor, maxStars, randomState);
    return MIN(starReq, Randomizer_get_val_in_range_uniform(MAX(maxAvailable - 5, 0), maxAvailable, randomState));
}

static void randomize_star_doors() {
    tinymt32_t randomState;
    tinymt32_init(&randomState, Randomizer_gGameSeed);

    u16 starTotal = calulate_star_total(LEVEL_BOB) + calulate_star_total(LEVEL_JRB) + calulate_star_total(LEVEL_WF) + calulate_star_total(LEVEL_PSS);
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_CH] = get_star_requirement(0, 0, starTotal, 7, &randomState);
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_GG] = get_star_requirement(0, 0, starTotal, 6, &randomState);

    starTotal += calulate_star_total(LEVEL_CCM);
    starTotal += calulate_star_total(LEVEL_BBH);

    Randomizer_gRequiredStars[Randomizer_STAR_REQ_B1] = get_star_requirement(0, 0, starTotal, 3, &randomState);

    starTotal += calulate_star_total(LEVEL_BITDW);
    
    //Randomizer_gRequiredStars[Randomizer_STAR_REQ_K1] = get_star_requirement(1, 0, starTotal, 3, &randomState);

    starTotal += calulate_star_total(LEVEL_SSL);
    starTotal += calulate_star_total(LEVEL_SL);
    
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_MMM]      = get_star_requirement(0, 1, starTotal, 3, &randomState);
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_BASE]     = get_star_requirement(0, 1, starTotal, 4, &randomState);

    starTotal += calulate_star_total(LEVEL_DDD);

    starTotal += calulate_star_total(LEVEL_HMC);

    Randomizer_gRequiredStars[Randomizer_STAR_REQ_KC] = get_star_requirement(1, 1, starTotal, 3, &randomState);

    starTotal += calulate_star_total(LEVEL_LLL);
    
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_B2] = get_star_requirement(1, 1, starTotal, 2, &randomState);

    starTotal += calulate_star_total(LEVEL_BITFS);

    //Randomizer_gRequiredStars[Randomizer_STAR_REQ_K2] = get_star_requirement(1, 1, starTotal, 2, &randomState);

    starTotal += calulate_star_total(LEVEL_TTM);
    starTotal += calulate_star_total(LEVEL_THI);
    starTotal += calulate_star_total(LEVEL_WDW);
    
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_OW3] = get_star_requirement(0, 2, starTotal, 2, &randomState);

    if (Randomizer_gOptionsSettings.gameplay.s.keepStructure) {
        //Randomizer_gRequiredStars[Randomizer_STAR_REQ_K1] = 0;
        //Randomizer_gRequiredStars[Randomizer_STAR_REQ_K2] = 0;
    }
}

static void init_required_stars() {
    u32 i;
    switch (Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts) {
        case 1:
            randomize_star_doors();
            break;
        case 0:
            for (i = 0; i < sizeof(Randomizer_gRequiredStars); i++) {
                Randomizer_gRequiredStars[i] = sDefaultStarReqs[i];
            }
            break;
        case 2:
            for (i = 0; i < sizeof(Randomizer_gRequiredStars); i++) {
                Randomizer_gRequiredStars[i] = 0;
            }
            break;
    }
    Randomizer_gRequiredStars[Randomizer_STAR_REQ_B3] = Randomizer_gStarDoorReqLUT[Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement]; // Final Bowser Door is special.
}

static u8 pick_random_u8(const u8* arr, size_t arrSize, tinymt32_t *randomState)
{
    u8 index = Randomizer_get_val_in_range_uniform(0, arrSize, randomState);
    return arr[index];
}

static void shuffle_warp_pool(const u8* warpPool, size_t warpPoolSize, tinymt32_t *randomState)
{
    for (int i = warpPoolSize - 1; i > 0; i--) {
        u8 i1 = warpPool[i];
        u8 i2 = pick_random_u8(warpPool, i + 1, randomState);

        u8 tmp = Randomizer_gWarpDestinations[i1];
        Randomizer_gWarpDestinations[i1] = Randomizer_gWarpDestinations[i2];
        Randomizer_gWarpDestinations[i2] = tmp;
    }
}

static void shuffle_u8(u8* arr, size_t arrSize, tinymt32_t *randomState)
{
    for (int i = arrSize - 1; i > 0; i--) {
        u8 i1 = i;
        u8 i2 = Randomizer_get_val_in_range_uniform(0, i + 1, randomState);

        u8 tmp = arr[i1];
        arr[i1] = arr[i2];
        arr[i2] = tmp;
    }
}

u8 Randomizer_get_nonrandom_level(u8 currLevel)
{
    for (int i = 0; i < ARRAY_COUNT(Randomizer_gWarpDestinations); i++) {
        if (Randomizer_gWarpDestinations[i] == currLevel) {
            return i;
        }
    }

    return 0;
}

u8 Randomizer_expected_mini_level_target(u8 currLevel)
{
    if (currLevel == LEVEL_BOWSER_1)
        return LEVEL_BITDW;
    if (currLevel == LEVEL_BOWSER_2)
        return LEVEL_BITFS;
    if (currLevel == LEVEL_VCUTM)
        return LEVEL_BBH;
    if (currLevel == LEVEL_COTMC)
        return LEVEL_CCM;
    if (currLevel == LEVEL_TOTWC)
        return LEVEL_TTM;

    return currLevel;
}

static int arr_have(const u8* arr, size_t arrSize, u8 val)
{
    for (size_t i = 0; i < arrSize; i++) {
        if (arr[i] == val) {
            return 1;
        }
    }
    return 0;
}

static void fixup_warps(u8 lvl, const u8* restrictions, size_t restrictionsSize, tinymt32_t *randomState) {
    lvl = Randomizer_get_nonrandom_level(lvl);
    lvl = Randomizer_expected_mini_level_target(lvl);
    lvl = Randomizer_get_nonrandom_level(lvl);

    if (!arr_have(restrictions, restrictionsSize, lvl)) {
        u8 newLvl = pick_random_u8(restrictions, restrictionsSize, randomState);
        u8 tmp = Randomizer_gWarpDestinations[newLvl];
        Randomizer_gWarpDestinations[newLvl] = Randomizer_gWarpDestinations[lvl];
        Randomizer_gWarpDestinations[lvl] = tmp;
    }
}

static void init_warp_scramble() {
    tinymt32_t randomState;
    tinymt32_init(&randomState, Randomizer_gGameSeed);
    for (int i = 0; i < ARRAY_COUNT(Randomizer_gWarpDestinations); i++)
        Randomizer_gWarpDestinations[i] = gWarpDestinationsStatic[i];

    shuffle_warp_pool(sWarpPool0, ARRAY_SIZE(sWarpPool0), &randomState);
    shuffle_warp_pool(sWarpPool1, ARRAY_SIZE(sWarpPool1), &randomState);
    if (Randomizer_gOptionsSettings.gameplay.s.keepStructure)
    {
        fixup_warps(LEVEL_BOWSER_1, sWarpsPreB1, ARRAY_SIZE(sWarpsPreB1), &randomState);
        fixup_warps(LEVEL_BOWSER_2, sWarpsPreB2, ARRAY_SIZE(sWarpsPreB2), &randomState);
    }
}

static void init_random_songs()
{
    // sort array Randomizer_gRandomSongs by using counting sort
    uint8_t arr[256] = {0};
    for (size_t i = 0; i < ARRAY_SIZE(Randomizer_gRandomSongs); i++) {
        arr[Randomizer_gRandomSongs[i]] = 1;
    }
    size_t index = 0;
    for (size_t i = 0; i < 256; i++) {
        if (arr[i])
        {
            Randomizer_gRandomSongs[index++] = i;
        }
    }
    
    tinymt32_t randomState;
    tinymt32_init(&randomState, Randomizer_gGameSeed * 42);
    shuffle_u8(Randomizer_gRandomSongs, ARRAY_SIZE(Randomizer_gRandomSongs), &randomState);
}

extern void save_main_menu_data(void);
void Randomizer_init_randomizer(s32 fileNum) {
    save_main_menu_data();
    save_file_set_seed_and_options(fileNum);
    init_warp_scramble();
    init_random_songs();
    init_required_stars();
}

extern u8 Randomizer_gOverwriteFileOptions;
extern u8 Randomizer_gOverwriteFileSeed;

s32 Randomizer_init_randomizer_test(s32, s32 v)
{
    Randomizer_gOverwriteFileOptions = 1;
    Randomizer_gOverwriteFileSeed = 1;
    Randomizer_gOptionsSettings = Randomizer_gPresets[1];
    Randomizer_init_randomizer(v);
    return v;
}

// stolen from stackoverflow
f32 hue_to_rgb(f32 p, f32 q, f32 t) {
    if (t < 0.f)
        t += 1.f;
    if (t > 1.f)
        t -= 1.f;

    if (t < 1/6.f)
        return p + (q - p) * 6 * t;
    if (t < 1/2.f)
        return q;
    if (t < 2/3.f)
        return p + (q - p) * (2/3.f - t) * 6;
    return p;
}

void hsl_to_rgb(u8 h, u8 s, u8 l, u8 *RGB) {
    f32 r,g,b;
    f32 hf = h / 255.0f;
    f32 sf = s / 255.0f;
    f32 lf = l / 255.0f;

    if (s == 0.f) {
        r = g = b = lf;
    } else {
        f32 q = (lf < 1/2.f) ? (lf * (1 + sf)) : (lf + sf - lf * sf);
        f32 p = 2 * lf - q;
        r = hue_to_rgb(p, q, hf + 1/3.f);
        g = hue_to_rgb(p, q, hf);
        b = hue_to_rgb(p, q, hf - 1/3.f);
    }

    RGB[0] = r * 255;
    RGB[1] = g * 255;
    RGB[2] = b * 255;
}

void get_random_color(u8 *RGB, tinymt32_t *randomState) {
    u32 rand = tinymt32_generate_u32(randomState);
    hsl_to_rgb(rand & 0xFF,
               0xFF,
               ((rand >> 8) & 0x7F) + ((rand >> 16) & 0x7F),
               RGB);
}

void Randomizer_init_star_color(struct Object *star, s32 courseID, s32 starID) {
    s32 index;
    switch (Randomizer_gOptionsSettings.cosmetic.s.starColors) {
        case Randomizer_STAR_COLOR_OFF:
            star->oStarColor = 0xFFFF29;
            return;
        case Randomizer_STAR_COLOR_PER_STAR:
            index = courseID * 8 + starID;
            break;
        case Randomizer_STAR_COLOR_PER_LEVEL:
            index = courseID;
            break;
        case Randomizer_STAR_COLOR_GLOBAL:
            index = 0;
            break;
    }
    tinymt32_t randomState;
    tinymt32_init(&randomState, index * 0x20000 + Randomizer_gGameSeed);

    u8 RGB[3];
    get_random_color(RGB, &randomState);
    star->oStarColor = (RGB[0] << 16) | (RGB[1] << 8) | RGB[2];
}

static void set_mario_light(void* dl, u8 r, u8 g, u8 b) {
    u8* walker = (u8*)dl;
    while (*walker != G_SETPRIMCOLOR)
    {
        walker += 8;
    }

    walker += 4;
    walker[0] = r;
    walker[1] = g;
    walker[2] = b;
}

struct Color
{
    u8 r;
    u8 g;
    u8 b;
};

struct Color set_mario_light_random(void* dl1, void* dl2, tinymt32_t *randomState) {
    u8 RGB[3];
    get_random_color(RGB, randomState);
    u8 r = RGB[0];
    u8 g = RGB[1];
    u8 b = RGB[2];
    set_mario_light(dl1, r, g, b);
    set_mario_light(dl2, r, g, b);
    struct Color color = {r, g, b};
    return color;
}

extern Gfx mat_mario_blue[];
extern Gfx mat_mario_button_layer1[];

extern Gfx mat_mario_red[];
extern Gfx mat_mario_logo_layer1[];

extern Gfx mat_mario_red_dark[];

// 4 vertex colors each
extern Vtx coin_seg3_vertex_yellow[];
extern Vtx coin_seg3_vertex_yellow_r[];
extern Vtx coin_seg3_vertex_red[];
extern Vtx coin_seg3_vertex_red_r[];
extern Vtx coin_seg3_vertex_blue[];
extern Vtx coin_seg3_vertex_blue_r[];

static void set_coin_color(u8 r, u8 g, u8 b, Vtx *d) {
    Vtx *a = segmented_to_virtual(d);
    u32 i;
    for (i = 0; i < 4; i++) {
        a[0].v.cn[0] = r;
        a[1].v.cn[0] = r;
        a[2].v.cn[0] = r;
        a[3].v.cn[0] = r;
        a[0].v.cn[1] = g;
        a[1].v.cn[1] = g;
        a[2].v.cn[1] = g;
        a[3].v.cn[1] = g;
        a[0].v.cn[2] = b;
        a[1].v.cn[2] = b;
        a[2].v.cn[2] = b;
        a[3].v.cn[2] = b;
    }
}

f32 RMSE(u8 r1, u8 r2, u8 g1, u8 g2, u8 b1, u8 b2) {
    f32 r, g, b;
    r = r1 - r2;
    g = g1 - g2;
    b = b1 - b2;
    return sqrtf(r * r + g * g + b * b);
}

#define MINDIFF 140.f
void Randomizer_set_mario_rando_colors(void) {
    tinymt32_t randomState;

    if (Randomizer_gOptionsSettings.cosmetic.s.marioColors) {
        {
            tinymt32_init(&randomState, Randomizer_gGameSeed);

            set_mario_light_random(segmented_to_virtual(&mat_mario_blue), segmented_to_virtual(&mat_mario_button_layer1), &randomState);
            struct Color hatColor = set_mario_light_random(segmented_to_virtual(&mat_mario_red), segmented_to_virtual(&mat_mario_logo_layer1), &randomState);
            u8* underhat = (u8*) segmented_to_virtual(mat_mario_red_dark);
            u8* color = underhat + 8*8;
            color[4] = hatColor.r / 2.5f;
            color[5] = hatColor.g / 2.5f;
            color[6] = hatColor.b / 2.5f;
        }
    }

    if (Randomizer_gOptionsSettings.cosmetic.s.coinsOn) {
        u8 yellows[3];
        u8 reds[3];
        u8 blues[3];
        tinymt32_init(&randomState, Randomizer_gGameSeed + 1);

        get_random_color(yellows, &randomState);
        set_coin_color(yellows[0], yellows[1], yellows[2], coin_seg3_vertex_yellow);
        set_coin_color(yellows[0], yellows[1], yellows[2], coin_seg3_vertex_yellow_r);

        get_random_color(reds, &randomState);
        while (RMSE(yellows[0], reds[0], yellows[1], reds[1], yellows[2], reds[2]) < MINDIFF) {
            get_random_color(reds, &randomState);
        }
        set_coin_color(reds[0], reds[1], reds[2], coin_seg3_vertex_red);
        set_coin_color(reds[0], reds[1], reds[2], coin_seg3_vertex_red_r);

        get_random_color(blues, &randomState);
        while ((RMSE(yellows[0], reds[0], yellows[1], reds[1], yellows[2], reds[2]) < MINDIFF)
               || (RMSE(yellows[0], blues[0], yellows[1], blues[1], yellows[2], blues[2]) < MINDIFF)
               || (RMSE(reds[0], blues[0], reds[1], blues[1], reds[2], blues[2]) < MINDIFF)) {
            get_random_color(blues, &randomState);
        }
        set_coin_color(blues[0], blues[1], blues[2], coin_seg3_vertex_blue);
        set_coin_color(blues[0], blues[1], blues[2], coin_seg3_vertex_blue_r);
    }
}
