#include <PR/ultratypes.h>
#include <PR/gbi.h>

#include "audio/external.h"
#include "behavior_data.h"
#include "dialog_ids.h"
#include "engine/behavior_script.h"
#include "engine/graph_node.h"
#include "engine/math_util.h"
#include "file_select.h"
#include "game/area.h"
#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "game/object_helpers.h"
#include "game/object_list_processor.h"
#include "game/print.h"
#include "game/save_file.h"
#include "game/segment2.h"
#include "game/segment7.h"
#include "game/spawn_object.h"
#include "game/rumble_init.h"
#include "game/options_menu.h"
#include "sm64.h"

static char textEnteredNumbers[8] = "0000000";

/**
 * @file file_select.c
 * This file implements how the file select and it's menus render and function.
 * That includes button IDs rendered as object models, strings, hand cursor,
 * special menu messages and phases, button states and button clicked checks.
 */

// Amount of main menu buttons defined in the code called by spawn_object_rel_with_rot.
// See file_select.h for the names in MenuButtonTypes.
static struct Object *sMainMenuButtons[MENU_BUTTON_SEED_MAX];

// Used to defined yes/no fade colors after a file is selected in the erase menu.
// sYesNoColor[0]: YES | sYesNoColor[1]: NO
static u8 sYesNoColor[2];

// The button that is selected when it is clicked.
static s8 sSelectedButtonID = MENU_BUTTON_NONE;

// Whether we are on the main menu or one of the submenus.
static s8 sCurrentMenuLevel = MENU_LAYER_MAIN;

// 2D position of the cursor on the screen.
// sCursorPos[0]: X | sCursorPos[1]: Y
static f32 sCursorPos[] = {0, 0};

// Determines which graphic to use for the cursor.
static s16 sCursorClickingTimer = 0;

// Equal to sCursorPos if the cursor gets clicked, {-10000, -10000} otherwise.
static s16 sClickPos[] = {-10000, -10000};

// Used for determining which file has been selected during copying and erasing.
static s8 sSelectedFileIndex = -1;

// Whether to fade out text or not.
static s8 sFadeOutText = FALSE;

// The message currently being displayed at the top of a menu.
static s8 sStatusMessageID = 0;

// Used for text fading. The alpha value of text is calculated as
// gDialogTextAlpha - sTextFadeAlpha.
static u8 sTextFadeAlpha = 0;

// File select timer that keeps counting until it reaches 1000.
// Used to prevent buttons from being clickable as soon as a menu loads.
// Gets reset when you click an empty save, existing saves in copy and erase menus
// and when you click yes/no in the erase confirmation prompt.
static s16 sMainMenuTimer = 0;

// Sound mode menu buttonID, has different values compared to gSoundMode in audio.
// 0: gSoundMode = 0 (Stereo) | 1: gSoundMode = 3 (Mono) | 2: gSoundMode = 1 (Headset)
s8 sSoundMode = 0;

// Active language for EU arrays
// 0: English | 1: French | 2: German

// Tracks which button will be pressed in the erase confirmation prompt (yes/no).
static s8 sEraseYesNoHoverState = MENU_ERASE_HOVER_NONE;

// Used for the copy menu, defines if the game as all 4 save slots with data.
// if TRUE, it doesn't allow copying more files.
static s8 sAllFilesExist = FALSE;

// Defines the value of the save slot selected in the menu.
// Mario A: 1 | Mario B: 2 | Mario C: 3 | Mario D: 4
static s8 sSelectedFileNum = 0;

// Which coin score mode to use when scoring files. 0 for local
// coin high score, 1 for high score across all files.
static s8 sScoreFileCoinScoreMode = 0;

static s8 sSelectingAdventure = 0;

#ifdef MULTILANG
// Index of the selected language in the above array.
static s8 sSelectedLanguageIndex = LANGUAGE_ENGLISH;

// Whether to open the language menu when the game is booted.
static s8 sOpenLangSettings = FALSE;
#endif

// namespace Randomizer {
static const char textRandomOptions[] = "RANDOMIZE";
static const char textRandom[] = "RANDOM";
static const char textReset[] = "RESET";
static const char textOptions[] = "OPTIONS";
static const char textSeed[] = "SEED";

static const char textObjOptions[] = "OBJECT OPTIONS";
static const char textWarpOptions[] = "WARP OPTIONS";
static const char textAestheticOptions[] = "AESTHETIC OPTIONS";
static const char testPresets[] = "PRESETS";
static const char textGPMOptions[] = "GAMEPLAY MODE OPTIONS";

static const char textEnterSeed[] = "ENTER SEED";
static const char textOptionSelect[] = "SELECT OPTIONS";
static const char textSeedShouldBe[] = "SEED SHOULD BE BETWEEN 0 AND 9999999";

static const char textPlus[] = "◀";
static const char textMinus[] = "▶";

static const char textNext[] = "◀ L - PREV";
static const char textPrev[] = "NEXT - R ▶";

static const char textPreset1[] = "DEFAULT";
static const char textPreset2[] = "EXTREME";
static const char textPreset3[] = "NONSTOP";
static const char textPreset4[] = "SET PROGRESSION";
static const char textPreset5[] = "IRON MARIO LITE";
static const char textPreset6[] = "IRON MARIO 80";
static const char textPreset7[] = "IRON MARIO 130";
static const char textPreset1Desc[] = "The randomizer experience you know\nwith wacky colors and aesthetics!";
static const char textPreset2Desc[] = "A super challenging experience for the\nskilled players out there.";
static const char textPreset3Desc[] = "A more peaceful and beginner-friendly\nmode for those new to the game.";
static const char textPreset4Desc[] = "Randomize the game while keeping\nthe original game progression!";
static const char textPreset5Desc[] = "Iron Mario permadeath mode with\n50 star requirement to beat the game!\nIncludes 3 lives";
static const char textPreset6Desc[] = "Iron Mario permadeath mode with\n80 star requirement to beat the game!";
static const char textPreset7Desc[] = "Iron Mario permadeath mode with\n130 star requirement to beat the game!\nCaps are unlocked in Hidden Palace at 100 stars.";
static const char textPresetCustom[] = "CUSTOM";
static const char textUsePreset[] = "USE PRESET";

static const char *textsPresets[] = { textPreset1, textPreset2, textPreset3, textPreset4 /*, textPreset5, textPreset6, textPreset7*/};
static const char *textsPresetDescriptions[] = { textPreset1Desc, textPreset2Desc, textPreset3Desc, textPreset4Desc, textPreset5Desc, textPreset6Desc, textPreset7Desc};

static u8 OptionPage = 3;

#define textCountPresets (sizeof(textsPresets) / 4)

static const char *pages[] = { textAestheticOptions, textObjOptions, textWarpOptions, testPresets, textGPMOptions };
static const u32 pageCount = sizeof(pages) / 4;

#define COSMETIC_VARS_SET(i, val) \
{ \
    switch(i) { \
        case 2: Randomizer_gOptionsSettings.cosmetic.s.coinsOn = val; break; \
        case 3: Randomizer_gOptionsSettings.cosmetic.s.skyboxOn = val; \
    } \
}

#define COSMETIC_VARS_GET(i) \
((i) == 2 ? Randomizer_gOptionsSettings.cosmetic.s.coinsOn : \
(Randomizer_gOptionsSettings.cosmetic.s.skyboxOn))

#define WARPS_VARS_SET(i, val) \
{ \
    switch(i) { \
        case 1: Randomizer_gOptionsSettings.gameplay.s.randomLevelSpawn = val; break; \
        case 2: Randomizer_gOptionsSettings.gameplay.s.randomLevelWarp = val; break; \
        case 3: Randomizer_gOptionsSettings.gameplay.s.adjustedExits = val; break; \
    } \
}

#define WARPS_VARS_GET(i) \
((i) == 1 ? Randomizer_gOptionsSettings.gameplay.s.randomLevelSpawn : \
((i) == 2 ? Randomizer_gOptionsSettings.gameplay.s.randomLevelWarp : \
(Randomizer_gOptionsSettings.gameplay.s.adjustedExits)))

#define OBJECT_VARS_SET(i, val) \
{ \
    switch(i) { \
        case 1: Randomizer_gOptionsSettings.gameplay.s.objectRandomization = val; break; \
        case 2: Randomizer_gOptionsSettings.gameplay.s.randomizeStarSpawns = val; \
    } \
}

#define OBJECT_VARS_GET(i) \
((i) == 1 ? Randomizer_gOptionsSettings.gameplay.s.objectRandomization : \
(Randomizer_gOptionsSettings.gameplay.s.randomizeStarSpawns))
// }

/**
 * Yellow Background Menu Initial Action
 * Rotates the background at 180 grades and it's scale.
 * Although the scale is properly applied in the loop function.
 */
void beh_yellow_background_menu_init(void) {
    gCurrentObject->oFaceAngleYaw = 0x8000;
    gCurrentObject->oMenuButtonScale = 9.0f;
}

/**
 * Yellow Background Menu Loop Action
 * Properly scales the background in the main menu.
 */
void beh_yellow_background_menu_loop(void) {
    cur_obj_scale(9.0f);
}

/**
 * Check if a button was clicked.
 * depth = 200.0 for main menu, 22.0 for submenus.
 */
s32 check_clicked_button(s16 x, s16 y, f32 depth) {
    f32 a = 52.4213f;
    f32 newX = ((f32) x * 160.0f) / (a * depth);
    f32 newY = ((f32) y * 120.0f) / (a * 3 / 4 * depth);
    s16 maxX = newX + 25.0f;
    s16 minX = newX - 25.0f;
    s16 maxY = newY + 21.0f;
    s16 minY = newY - 21.0f;

    if (sClickPos[0] < maxX && minX < sClickPos[0] && sClickPos[1] < maxY && minY < sClickPos[1]) {
        return TRUE;
    }
    return FALSE;
}

/**
 * Grow from main menu, used by selecting files and menus.
 */
void bhv_menu_button_growing_from_main_menu(struct Object *button) {
    if (button->oMenuButtonTimer < 16) {
        button->oFaceAngleYaw += 0x800;
    }
    if (button->oMenuButtonTimer < 8) {
        button->oFaceAnglePitch += 0x800;
    }
    if (button->oMenuButtonTimer >= 8 && button->oMenuButtonTimer < 16) {
        button->oFaceAnglePitch -= 0x800;
    }
    button->oParentRelativePosX -= button->oMenuButtonOrigPosX / 16.0f;
    button->oParentRelativePosY -= button->oMenuButtonOrigPosY / 16.0f;
    if (button->oPosZ < button->oMenuButtonOrigPosZ + 17800.0f) {
        button->oParentRelativePosZ += 1112.5f;
    }
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 16) {
        button->oParentRelativePosX = 0.0f;
        button->oParentRelativePosY = 0.0f;
        button->oMenuButtonState = MENU_BUTTON_STATE_FULLSCREEN;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * Shrink back to main menu, used to return back while inside menus.
 */
void bhv_menu_button_shrinking_to_main_menu(struct Object *button) {
    if (button->oMenuButtonTimer < 16) {
        button->oFaceAngleYaw -= 0x800;
    }
    if (button->oMenuButtonTimer < 8) {
        button->oFaceAnglePitch -= 0x800;
    }
    if (button->oMenuButtonTimer >= 8 && button->oMenuButtonTimer < 16) {
        button->oFaceAnglePitch += 0x800;
    }
    button->oParentRelativePosX += button->oMenuButtonOrigPosX / 16.0f;
    button->oParentRelativePosY += button->oMenuButtonOrigPosY / 16.0f;
    if (button->oPosZ > button->oMenuButtonOrigPosZ) {
        button->oParentRelativePosZ -= 1112.5f;
    }
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 16) {
        button->oParentRelativePosX = button->oMenuButtonOrigPosX;
        button->oParentRelativePosY = button->oMenuButtonOrigPosY;
        button->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * Grow from submenu, used by selecting a file in the score menu.
 */
void bhv_menu_button_growing_from_submenu(struct Object *button) {
    if (button->oMenuButtonTimer < 16) {
        button->oFaceAngleYaw += 0x800;
    }
    if (button->oMenuButtonTimer < 8) {
        button->oFaceAnglePitch += 0x800;
    }
    if (button->oMenuButtonTimer >= 8 && button->oMenuButtonTimer < 16) {
        button->oFaceAnglePitch -= 0x800;
    }
    button->oParentRelativePosX -= button->oMenuButtonOrigPosX / 16.0f;
    button->oParentRelativePosY -= button->oMenuButtonOrigPosY / 16.0f;
    button->oParentRelativePosZ -= 116.25f;
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 16) {
        button->oParentRelativePosX = 0.0f;
        button->oParentRelativePosY = 0.0f;
        button->oMenuButtonState = MENU_BUTTON_STATE_FULLSCREEN;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * Shrink back to submenu, used to return back while inside a score save menu.
 */
void bhv_menu_button_shrinking_to_submenu(struct Object *button) {
    if (button->oMenuButtonTimer < 16) {
        button->oFaceAngleYaw -= 0x800;
    }
    if (button->oMenuButtonTimer < 8) {
        button->oFaceAnglePitch -= 0x800;
    }
    if (button->oMenuButtonTimer >= 8 && button->oMenuButtonTimer < 16) {
        button->oFaceAnglePitch += 0x800;
    }
    button->oParentRelativePosX += button->oMenuButtonOrigPosX / 16.0f;
    button->oParentRelativePosY += button->oMenuButtonOrigPosY / 16.0f;
    if (button->oPosZ > button->oMenuButtonOrigPosZ) {
        button->oParentRelativePosZ += 116.25f;
    }
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 16) {
        button->oParentRelativePosX = button->oMenuButtonOrigPosX;
        button->oParentRelativePosY = button->oMenuButtonOrigPosY;
        button->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * A small increase and decrease in size.
 * Used by failed copy/erase/score operations and sound mode select.
 */
void bhv_menu_button_zoom_in_out(struct Object *button) {
    if (sCurrentMenuLevel == MENU_LAYER_MAIN) {
        if (button->oMenuButtonTimer < 4) {
            button->oParentRelativePosZ -= 20.0f;
        }
        if (button->oMenuButtonTimer >= 4) {
            button->oParentRelativePosZ += 20.0f;
        }
    } else {
        if (button->oMenuButtonTimer < 4) {
            button->oParentRelativePosZ += 20.0f;
        }
        if (button->oMenuButtonTimer >= 4) {
            button->oParentRelativePosZ -= 20.0f;
        }
    }
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 8) {
        button->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * A small temporary increase in size.
 * Used while selecting a target copy/erase file or yes/no erase confirmation prompt.
 */
void bhv_menu_button_zoom_in(struct Object *button) {
    button->oMenuButtonScale += 0.0022f;
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 10) {
        button->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * A small temporary decrease in size.
 * Used after selecting a target copy/erase file or
 * yes/no erase confirmation prompt to undo the zoom in.
 */
void bhv_menu_button_zoom_out(struct Object *button) {
    button->oMenuButtonScale -= 0.0022f;
    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 10) {
        button->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * Menu Buttons Menu Initial Action
 * Aligns menu buttons so they can stay in their original
 * positions when you choose a button.
 */
void bhv_menu_button_init(void) {
    gCurrentObject->oMenuButtonOrigPosX = gCurrentObject->oParentRelativePosX;
    gCurrentObject->oMenuButtonOrigPosY = gCurrentObject->oParentRelativePosY;
}

/**
 * Menu Buttons Menu Loop Action
 * Handles the functions of the button states and
 * object scale for each button.
 */
void bhv_menu_button_loop(void) {
    switch (gCurrentObject->oMenuButtonState) {
        case MENU_BUTTON_STATE_DEFAULT: // Button state
            gCurrentObject->oMenuButtonOrigPosZ = gCurrentObject->oPosZ;
            break;
        case MENU_BUTTON_STATE_GROWING: // Switching from button to menu state
            if (sCurrentMenuLevel == MENU_LAYER_MAIN) {
                bhv_menu_button_growing_from_main_menu(gCurrentObject);
            }
            if (sCurrentMenuLevel == MENU_LAYER_SUBMENU) {
                bhv_menu_button_growing_from_submenu(gCurrentObject); // Only used for score files
            }
            gDialogTextAlpha = 0;
            sCursorClickingTimer = 4;
            break;
        case MENU_BUTTON_STATE_FULLSCREEN: // Menu state
            break;
        case MENU_BUTTON_STATE_SHRINKING: // Switching from menu to button state
            if (sCurrentMenuLevel == MENU_LAYER_MAIN) {
                bhv_menu_button_shrinking_to_main_menu(gCurrentObject);
            }
            if (sCurrentMenuLevel == MENU_LAYER_SUBMENU) {
                bhv_menu_button_shrinking_to_submenu(gCurrentObject); // Only used for score files
            }
            gDialogTextAlpha = 0;
            sCursorClickingTimer = 4;
            break;
        case MENU_BUTTON_STATE_ZOOM_IN_OUT:
            bhv_menu_button_zoom_in_out(gCurrentObject);
            sCursorClickingTimer = 4;
            break;
        case MENU_BUTTON_STATE_ZOOM_IN:
            bhv_menu_button_zoom_in(gCurrentObject);
            sCursorClickingTimer = 4;
            break;
        case MENU_BUTTON_STATE_ZOOM_OUT:
            bhv_menu_button_zoom_out(gCurrentObject);
            sCursorClickingTimer = 4;
            break;
    }
    cur_obj_scale(gCurrentObject->oMenuButtonScale);
}

/**
 * Handles how to exit the score file menu using button states.
 */
void exit_score_file_to_score_menu(struct Object *scoreFileButton, s8 scoreButtonID) {
    // Begin exit
    if (scoreFileButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN
        && sCursorClickingTimer == 2) {
        play_sound(SOUND_MENU_CAMERA_ZOOM_OUT, gGlobalSoundSource);
#if ENABLE_RUMBLE
        queue_rumble_data(5, 80);
#endif
        scoreFileButton->oMenuButtonState = MENU_BUTTON_STATE_SHRINKING;
    }
    // End exit
    if (scoreFileButton->oMenuButtonState == MENU_BUTTON_STATE_DEFAULT) {
        sSelectedButtonID = scoreButtonID;
        if (sCurrentMenuLevel == MENU_LAYER_SUBMENU) {
            sCurrentMenuLevel = MENU_LAYER_MAIN;
        }
    }
}

static const Vec3s sSaveFileButtonPositions[] = {
    {  711, 311, -100 }, // SAVE_FILE_A
    { -166, 311, -100 }, // SAVE_FILE_B
    {  711,   0, -100 }, // SAVE_FILE_C
    { -166,   0, -100 }, // SAVE_FILE_D
};

#define SPAWN_FILE_SELECT_FILE_BUTTON(parent, saveFile)                                                 \
    spawn_object_rel_with_rot((parent),                                                                 \
    (save_file_exists(saveFile) ? MODEL_MAIN_MENU_MARIO_SAVE_BUTTON : MODEL_MAIN_MENU_MARIO_NEW_BUTTON),\
    bhvMenuButton,                                                                                      \
    sSaveFileButtonPositions[saveFile][0],                                                              \
    sSaveFileButtonPositions[saveFile][1],                                                              \
    sSaveFileButtonPositions[saveFile][2],                                                              \
    0x0, -0x8000, 0x0)

#define MENU_BUTTON_SCALE 0.11111111f

/**
 * Render buttons for the menu.
 * Also check if the save file exists to render a different Mario button.
 */
void render_menu_buttons(s32 selectedButtonID) {
    struct Object *button = sMainMenuButtons[selectedButtonID];
    // MENU_BUTTON_SCORE ->  7
    // MENU_BUTTON_COPY  -> 14
    // MENU_BUTTON_ERASE -> 21
    s32 idx = (selectedButtonID - 3) * 7;

    // File A
    sMainMenuButtons[idx + 0] = SPAWN_FILE_SELECT_FILE_BUTTON(button, SAVE_FILE_A);
    sMainMenuButtons[idx + 0]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // File B
    sMainMenuButtons[idx + 1] = SPAWN_FILE_SELECT_FILE_BUTTON(button, SAVE_FILE_B);
    sMainMenuButtons[idx + 1]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // File C
    sMainMenuButtons[idx + 2] = SPAWN_FILE_SELECT_FILE_BUTTON(button, SAVE_FILE_C);
    sMainMenuButtons[idx + 2]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // File D
    sMainMenuButtons[idx + 3] = SPAWN_FILE_SELECT_FILE_BUTTON(button, SAVE_FILE_D);
    sMainMenuButtons[idx + 3]->oMenuButtonScale = MENU_BUTTON_SCALE;

    // Return to main menu button
    sMainMenuButtons[idx + 4] =
        spawn_object_rel_with_rot(button, MODEL_MAIN_MENU_YELLOW_FILE_BUTTON,
                                  bhvMenuButton,  711, -388, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[idx + 4]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // Switch to copy menu button
    sMainMenuButtons[idx + 5] =
        spawn_object_rel_with_rot(button, selectedButtonID == MENU_BUTTON_SCORE ? MODEL_MAIN_MENU_BLUE_COPY_BUTTON : MODEL_MAIN_MENU_GREEN_SCORE_BUTTON,
                                  bhvMenuButton,    0, -388, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[idx + 5]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // Switch to erase menu button
    sMainMenuButtons[idx + 6] =
        spawn_object_rel_with_rot(button, selectedButtonID == MENU_BUTTON_ERASE ? MODEL_MAIN_MENU_BLUE_COPY_BUTTON : MODEL_MAIN_MENU_RED_ERASE_BUTTON,
                                  bhvMenuButton, -711, -388, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[idx + 6]->oMenuButtonScale = MENU_BUTTON_SCALE;
}

#define SCORE_TIMER 31
/**
 * In the score menu, checks if a button was clicked to play a sound, button state and other functions.
 */
void check_score_menu_clicked_buttons(struct Object *scoreButton) {
    if (scoreButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        s32 buttonID;
        // Configure score menu button group
        for (buttonID = MENU_BUTTON_SCORE_MIN; buttonID < MENU_BUTTON_SCORE_MAX; buttonID++) {
            s16 buttonX = sMainMenuButtons[buttonID]->oPosX;
            s16 buttonY = sMainMenuButtons[buttonID]->oPosY;

            if (check_clicked_button(buttonX, buttonY, 22.0f) == TRUE && sMainMenuTimer >= SCORE_TIMER) {
                // If menu button clicked, select it
                if (buttonID == MENU_BUTTON_SCORE_RETURN || buttonID == MENU_BUTTON_SCORE_COPY_FILE
                    || buttonID == MENU_BUTTON_SCORE_ERASE_FILE) {
                    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                    queue_rumble_data(5, 80);
#endif
                    sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
                    sSelectedButtonID = buttonID;
                }
                else { // Check if a save file is clicked
                    if (sMainMenuTimer >= SCORE_TIMER) {
                        // If clicked in a existing save file, select it too see it's score
                        if (save_file_exists(buttonID - MENU_BUTTON_SCORE_MIN) == TRUE) {
                            play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gGlobalSoundSource);
#if ENABLE_RUMBLE
                            queue_rumble_data(5, 80);
#endif
                            sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_GROWING;
                            sSelectedButtonID = buttonID;
                        }
                        else {
                            // If clicked in a non-existing save file, play buzz sound
                            play_sound(SOUND_MENU_CAMERA_BUZZ, gGlobalSoundSource);
#if ENABLE_RUMBLE
                            queue_rumble_data(5, 80);
#endif
                            sMainMenuButtons[buttonID]->oMenuButtonState =
                                MENU_BUTTON_STATE_ZOOM_IN_OUT;
                            if (sMainMenuTimer >= SCORE_TIMER) {
                                sFadeOutText = TRUE;
                                sMainMenuTimer = 0;
                            }
                        }
                    }
                }
                sCurrentMenuLevel = MENU_LAYER_SUBMENU;
                break;
            }
        }
    }
}

#undef SCORE_TIMER

#define BUZZ_TIMER 21

/**
 * Copy Menu phase actions that handles what to do when a file button is clicked.
 */
void copy_action_file_button(struct Object *copyButton, s32 copyFileButtonID) {
    switch (copyButton->oMenuButtonActionPhase) {
        case COPY_PHASE_MAIN: // Copy Menu Main Phase
            if (sAllFilesExist == TRUE) { // Don't enable copy if all save files exists
                return;
            }
            if (save_file_exists(copyFileButtonID - MENU_BUTTON_COPY_MIN) == TRUE) {
                // If clicked in a existing save file, ask where it wants to copy
                play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                queue_rumble_data(5, 80);
#endif
                sMainMenuButtons[copyFileButtonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN;
                sSelectedFileIndex = copyFileButtonID - MENU_BUTTON_COPY_MIN;
                copyButton->oMenuButtonActionPhase = COPY_PHASE_COPY_WHERE;
                sFadeOutText = TRUE;
                sMainMenuTimer = 0;
            } else {
                // If clicked in a non-existing save file, play buzz sound
                play_sound(SOUND_MENU_CAMERA_BUZZ, gGlobalSoundSource);
#if ENABLE_RUMBLE
                queue_rumble_data(5, 80);
#endif
                sMainMenuButtons[copyFileButtonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
                if (sMainMenuTimer >= BUZZ_TIMER) {
                    sFadeOutText = TRUE;
                    sMainMenuTimer = 0;
                }
            }
            break;
        case COPY_PHASE_COPY_WHERE: // Copy Menu "COPY IT TO WHERE?" Phase (after a file is selected)
            sMainMenuButtons[copyFileButtonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
            if (save_file_exists(copyFileButtonID - MENU_BUTTON_COPY_MIN) == FALSE) {
                // If clicked in a non-existing save file, copy the file
                play_sound(SOUND_MENU_STAR_SOUND, gGlobalSoundSource);
#if ENABLE_RUMBLE
                queue_rumble_data(5, 80);
#endif
                copyButton->oMenuButtonActionPhase = COPY_PHASE_COPY_COMPLETE;
                sFadeOutText = TRUE;
                sMainMenuTimer = 0;
                save_file_copy(sSelectedFileIndex, copyFileButtonID - MENU_BUTTON_COPY_MIN);
                sMainMenuButtons[copyFileButtonID]->header.gfx.sharedChild =
                    gLoadedGraphNodes[MODEL_MAIN_MENU_MARIO_SAVE_BUTTON_FADE];
                sMainMenuButtons[copyFileButtonID - MENU_BUTTON_COPY_MIN]->header.gfx.sharedChild =
                    gLoadedGraphNodes[MODEL_MAIN_MENU_MARIO_SAVE_BUTTON_FADE];
            } else {
                // If clicked in a existing save file, play buzz sound
                if (MENU_BUTTON_COPY_FILE_A + sSelectedFileIndex == copyFileButtonID) {
                    play_sound(SOUND_MENU_CAMERA_BUZZ, gGlobalSoundSource);
#if ENABLE_RUMBLE
                    queue_rumble_data(5, 80);
#endif
                    sMainMenuButtons[MENU_BUTTON_COPY_FILE_A + sSelectedFileIndex]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_OUT;
                    copyButton->oMenuButtonActionPhase = COPY_PHASE_MAIN;
                    sFadeOutText = TRUE;
                    return;
                }
                if (sMainMenuTimer >= BUZZ_TIMER) {
                    sFadeOutText = TRUE;
                    sMainMenuTimer = 0;
                }
            }
            break;
    }
}

#define ACTION_TIMER 30

/**
 * In the copy menu, checks if a button was clicked to play a sound, button state and other functions.
 */
void check_copy_menu_clicked_buttons(struct Object *copyButton) {
    if (copyButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        s32 buttonID;
        // Configure copy menu button group
        for (buttonID = MENU_BUTTON_COPY_MIN; buttonID < MENU_BUTTON_COPY_MAX; buttonID++) {
            s16 buttonX = sMainMenuButtons[buttonID]->oPosX;
            s16 buttonY = sMainMenuButtons[buttonID]->oPosY;

            if (check_clicked_button(buttonX, buttonY, 22.0f) == TRUE) {
                // If menu button clicked, select it
                if (buttonID == MENU_BUTTON_COPY_RETURN || buttonID == MENU_BUTTON_COPY_CHECK_SCORE
                    || buttonID == MENU_BUTTON_COPY_ERASE_FILE) {
                    if (copyButton->oMenuButtonActionPhase == COPY_PHASE_MAIN) {
                        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                        queue_rumble_data(5, 80);
#endif
                        sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
                        sSelectedButtonID = buttonID;
                    }
                }
                else {
                    // Check if a file button is clicked to play a copy action
                    if (sMainMenuButtons[buttonID]->oMenuButtonState == MENU_BUTTON_STATE_DEFAULT
                        && sMainMenuTimer >= ACTION_TIMER) {
                        copy_action_file_button(copyButton, buttonID);
                    }
                }
                sCurrentMenuLevel = MENU_LAYER_SUBMENU;
                break;
            }
        }

        // After copy is complete, return to main copy phase
        if (copyButton->oMenuButtonActionPhase == COPY_PHASE_COPY_COMPLETE
            && sMainMenuTimer > ACTION_TIMER) {
            copyButton->oMenuButtonActionPhase = COPY_PHASE_MAIN;
            sMainMenuButtons[MENU_BUTTON_COPY_MIN + sSelectedFileIndex]->oMenuButtonState =
                MENU_BUTTON_STATE_ZOOM_OUT;
        }
    }
}

/**
 * Erase Menu phase actions that handles what to do when a file button is clicked.
 */
void erase_action_file_button(struct Object *eraseButton, s32 eraseFileButtonID) {
    switch (eraseButton->oMenuButtonActionPhase) {
        case ERASE_PHASE_MAIN: // Erase Menu Main Phase
            if (save_file_exists(eraseFileButtonID - MENU_BUTTON_ERASE_MIN) == TRUE) {
                // If clicked in a existing save file, ask if it wants to delete it
                play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                queue_rumble_data(5, 80);
#endif
                sMainMenuButtons[eraseFileButtonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN;
                sSelectedFileIndex = eraseFileButtonID - MENU_BUTTON_ERASE_MIN;
                eraseButton->oMenuButtonActionPhase = ERASE_PHASE_PROMPT;
                sFadeOutText = TRUE;
                sMainMenuTimer = 0;
            } else {
                // If clicked in a non-existing save file, play buzz sound
                play_sound(SOUND_MENU_CAMERA_BUZZ, gGlobalSoundSource);
#if ENABLE_RUMBLE
                queue_rumble_data(5, 80);
#endif
                sMainMenuButtons[eraseFileButtonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;

                if (sMainMenuTimer >= BUZZ_TIMER) {
                    sFadeOutText = TRUE;
                    sMainMenuTimer = 0;
                }
            }
            break;
        case ERASE_PHASE_PROMPT: // Erase Menu "SURE? YES NO" Phase (after a file is selected)
            if (MENU_BUTTON_ERASE_MIN + sSelectedFileIndex == eraseFileButtonID) {
                // If clicked in a existing save file, play click sound and zoom out button
                // Note: The prompt functions are actually called when the ERASE_MSG_PROMPT
                // message is displayed with print_erase_menu_prompt
                play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                queue_rumble_data(5, 80);
#endif
                sMainMenuButtons[MENU_BUTTON_ERASE_MIN + sSelectedFileIndex]->oMenuButtonState =
                    MENU_BUTTON_STATE_ZOOM_OUT;
                eraseButton->oMenuButtonActionPhase = ERASE_PHASE_MAIN;
                sFadeOutText = TRUE;
            }
            break;
    }
}

#undef BUZZ_TIMER

/**
 * In the erase menu, checks if a button was clicked to play a sound, button state and other functions.
 */
void check_erase_menu_clicked_buttons(struct Object *eraseButton) {
    if (eraseButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        s32 buttonID;
        // Configure erase menu button group
        for (buttonID = MENU_BUTTON_ERASE_MIN; buttonID < MENU_BUTTON_ERASE_MAX; buttonID++) {
            s16 buttonX = sMainMenuButtons[buttonID]->oPosX;
            s16 buttonY = sMainMenuButtons[buttonID]->oPosY;

            if (check_clicked_button(buttonX, buttonY, 22.0f) == TRUE) {
                // If menu button clicked, select it
                if (buttonID == MENU_BUTTON_ERASE_RETURN || buttonID == MENU_BUTTON_ERASE_CHECK_SCORE
                    || buttonID == MENU_BUTTON_ERASE_COPY_FILE) {
                    if (eraseButton->oMenuButtonActionPhase == ERASE_PHASE_MAIN) {
                        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                        queue_rumble_data(5, 80);
#endif
                        sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
                        sSelectedButtonID = buttonID;
                    }
                }
                else {
                    // Check if a file button is clicked to play an erase action
                    if (sMainMenuTimer >= ACTION_TIMER) {
                        erase_action_file_button(eraseButton, buttonID);
                    }
                }
                sCurrentMenuLevel = MENU_LAYER_SUBMENU;
                break;
            }
        }
        // After erase is complete, return to main erase phase
        if (eraseButton->oMenuButtonActionPhase == ERASE_PHASE_MARIO_ERASED
            && sMainMenuTimer > ACTION_TIMER) {
            eraseButton->oMenuButtonActionPhase = ERASE_PHASE_MAIN;
            sMainMenuButtons[MENU_BUTTON_ERASE_MIN + sSelectedFileIndex]->oMenuButtonState =
                MENU_BUTTON_STATE_ZOOM_OUT;
        }
    }
}

#undef ACTION_TIMER

// namespace Randomizer
static u32 get_entered_seed(void) {
    return (textEnteredNumbers[0] - '0') * 1000000
         + (textEnteredNumbers[1] - '0') * 100000
         + (textEnteredNumbers[2] - '0') * 10000
         + (textEnteredNumbers[3] - '0') * 1000
         + (textEnteredNumbers[4] - '0') * 100
         + (textEnteredNumbers[5] - '0') * 10
         + (textEnteredNumbers[6] - '0');
}

static void seed_menu_create_buttons(struct Object *seedButton) {
    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_RETURN] =
        spawn_object_rel_with_rot(seedButton, 6, bhvMenuButton, -690, -400, -100, 0, -0x8000, 0);
    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_RETURN]->oMenuButtonScale = 0.11111111f;
    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_RESET] =
        spawn_object_rel_with_rot(seedButton, 12, bhvMenuButton, 690, -400, -100, 0, -0x8000, 0);
    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_RESET]->oMenuButtonScale = 0.11111111f;
    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_OPTIONS] =
        spawn_object_rel_with_rot(seedButton, 12, bhvMenuButton, 690, 0, -100, 0, -0x8000, 0);
    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_OPTIONS]->oMenuButtonScale = 0.11111111f;
}

extern u8 Randomizer_gOverwriteFileOptions;
extern u8 Randomizer_gOverwriteFileSeed;

static void seed_menu_check_clicked_buttons(struct Object *seedButton) {
    if (seedButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        s32 buttonId;
        for (buttonId = MENU_BUTTON_SEED_MIN; buttonId < MENU_BUTTON_SEED_MAX; buttonId++) {
            s16 buttonX = sMainMenuButtons[buttonId]->oPosX;
            s16 buttonY = sMainMenuButtons[buttonId]->oPosY;

            if (check_clicked_button(buttonX, buttonY, 22.0f) == TRUE) {
                if (seedButton->oMenuButtonActionPhase == 0) {
                    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
                    sMainMenuButtons[buttonId]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
                }
                if (buttonId == MENU_BUTTON_SELECT_SEED_RETURN) {
                    sCurrentMenuLevel = MENU_LAYER_SUBMENU;
                    sSelectedButtonID = buttonId;        
                    save_main_menu_data();
                    break;
                } else if (buttonId == MENU_BUTTON_SELECT_SEED_RESET) {
                    for (int i = 0; i < 7; i++) {
                        textEnteredNumbers[i] = '0';
                    }
                    Randomizer_gOverwriteFileSeed = TRUE;
                    Randomizer_gIsSetSeed = FALSE;
                } else {
                    sCurrentMenuLevel = MENU_LAYER_SUBMENU;
                    sSelectedButtonID = MENU_BUTTON_SELECT_SEED_OPTIONS;
                    for (buttonId = MENU_BUTTON_SELECT_SEED_RETURN;
                         buttonId < MENU_BUTTON_SELECT_SEED_OPTIONS; buttonId++) {
                        mark_obj_for_deletion(sMainMenuButtons[buttonId]);
                    }
                    sMainMenuButtons[MENU_BUTTON_SELECT_SEED_OPTIONS]->oMenuButtonState =
                        MENU_BUTTON_STATE_GROWING;
                    play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gGlobalSoundSource);
                }
            }
        }
    }
}

s32 check_clicked_text_width(s16 x, s16 y, UNUSED int ID, s32 xWidth) {
    s16 cursorX = sCursorPos[0] - (x - 165.f);
    s16 cursorY = sCursorPos[1] - (y - 110.0f);
    s16 maxX = xWidth;
    s16 minX = 0.f;
    s16 maxY = 8.0f;
    s16 minY = -8.0f;

    if (gPlayer1Controller->buttonPressed & A_BUTTON) {
        if (cursorX < maxX && minX < cursorX && cursorY < maxY && minY < cursorY) {
            play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
            return TRUE;
        }
    }
    return FALSE;
}

s32 check_clicked_text(s16 x, s16 y, int ID) {
    return check_clicked_text_width(x, y, ID, 30);
}

// generate a random number of either 0 or 1 based on the weights
static s32 randomize_weighted_2(s32 weight1, s32 weight2) {
    s32 random = random_u16() % (weight1 + weight2);
    if (random < weight1) {
        return 0;
    } else {
        return 1;
    }
}

// generate a random number of either 0, 1, or 2 based on the weights
s32 randomize_weighted_3(s32 weight1, s32 weight2, s32 weight3) {
    s32 random = random_u16() % (weight1 + weight2 + weight3);
    if (random < weight1) {
        return 0;
    } else if (random < weight1 + weight2) {
        return 1;
    } else {
        return 2;
    }
}

void randomize_options() {
    Randomizer_gOptionsSettings.gameplay.s.keepStructure = random_u16() % 2;

    // Use two different rng calls to get an approximate normal distribution
    Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement = (random_u16() % 6) + (random_u16() % 6) + 1; // 1-11 - cant be 0 or 119

    Randomizer_gOptionsSettings.gameplay.s.nonstopMode = randomize_weighted_3(2, 1, 1); // weight no nonstop higher
    //Randomizer_gOptionsSettings.gameplay.s.demonOn = random_u16() % 2; // demon not an option
    Randomizer_gOptionsSettings.gameplay.s.demonOn = 0;
    Randomizer_gOptionsSettings.gameplay.s.randomLevelSpawn = random_u16() % 2;
    Randomizer_gOptionsSettings.gameplay.s.randomLevelWarp = randomize_weighted_2(1, 3); // weight random warps higher
    Randomizer_gOptionsSettings.gameplay.s.adjustedExits = random_u16() % 2;
    Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts = randomize_weighted_3(2, 3, 1); // weight no requirements lower
    Randomizer_gOptionsSettings.gameplay.s.safeSpawns = random_u16() % 3;
    Randomizer_gOptionsSettings.gameplay.s.objectRandomization = randomize_weighted_2(1, 3); // weight only key objects lower
    Randomizer_gOptionsSettings.gameplay.s.randomizeStarSpawns = random_u16() % 2;

    Randomizer_gOptionsSettings.cosmetic.s.marioColors = random_u16() % 3;
    Randomizer_gOptionsSettings.cosmetic.s.musicOn = random_u16() % 2; // music off not option
    Randomizer_gOptionsSettings.cosmetic.s.skyboxOn = random_u16() % 2;
    Randomizer_gOptionsSettings.cosmetic.s.coinsOn = random_u16() % 2;
    Randomizer_gOptionsSettings.cosmetic.s.starColors = random_u16() % 4;

    if (!Randomizer_gOptionsSettings.gameplay.s.randomLevelWarp) {
        Randomizer_gOptionsSettings.gameplay.s.adjustedExits = 0;
    }

    Randomizer_gOverwriteFileOptions = TRUE;
    Randomizer_curPreset = -1;
}

static void seed_menu_options_check_clicked_buttons(UNUSED struct Object *seedButton) {
    if (check_clicked_text_width(240, 33, 0, 45)) {
        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
        sCurrentMenuLevel = MENU_LAYER_SUBMENU;
        sSelectedButtonID = MENU_BUTTON_SELECT_SEED_RETURN;
        save_main_menu_data();
        mark_obj_for_deletion(sMainMenuButtons[MENU_BUTTON_SELECT_SEED_OPTIONS]);
    }
    if (check_clicked_text_width(35,33,0,60)) {
        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
        randomize_options();
    }
}

#ifdef MULTILANG
    #define SOUND_BUTTON_Y 388
#else
    #define SOUND_BUTTON_Y 0
#endif

/**
 * Render buttons for the sound mode menu.
 */
void render_sound_mode_menu_buttons(struct Object *soundModeButton) {
#ifdef ENABLE_STEREO_HEADSET_EFFECTS
    // Stereo option button
    sMainMenuButtons[MENU_BUTTON_STEREO] = spawn_object_rel_with_rot(
        soundModeButton, MODEL_MAIN_MENU_GENERIC_BUTTON, bhvMenuButton,  533, SOUND_BUTTON_Y, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[MENU_BUTTON_STEREO]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // Mono option button
    sMainMenuButtons[MENU_BUTTON_MONO] = spawn_object_rel_with_rot(
        soundModeButton, MODEL_MAIN_MENU_GENERIC_BUTTON, bhvMenuButton,    0, SOUND_BUTTON_Y, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[MENU_BUTTON_MONO]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // Headset option button
    sMainMenuButtons[MENU_BUTTON_HEADSET] = spawn_object_rel_with_rot(
        soundModeButton, MODEL_MAIN_MENU_GENERIC_BUTTON, bhvMenuButton, -533, SOUND_BUTTON_Y, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[MENU_BUTTON_HEADSET]->oMenuButtonScale = MENU_BUTTON_SCALE;
#else
    // Stereo option button
    sMainMenuButtons[MENU_BUTTON_STEREO] = spawn_object_rel_with_rot(
        soundModeButton, MODEL_MAIN_MENU_GENERIC_BUTTON, bhvMenuButton,  355, SOUND_BUTTON_Y, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[MENU_BUTTON_STEREO]->oMenuButtonScale = MENU_BUTTON_SCALE;
    // Mono option button
    sMainMenuButtons[MENU_BUTTON_MONO] = spawn_object_rel_with_rot(
        soundModeButton, MODEL_MAIN_MENU_GENERIC_BUTTON, bhvMenuButton, -355, SOUND_BUTTON_Y, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[MENU_BUTTON_MONO]->oMenuButtonScale = MENU_BUTTON_SCALE;
#endif

#ifdef MULTILANG
    // Return button
    sMainMenuButtons[MENU_BUTTON_OPTION_RETURN] = spawn_object_rel_with_rot(
        soundModeButton, MODEL_MAIN_MENU_YELLOW_FILE_BUTTON, bhvMenuButton, 0, -533, -100, 0x0, -0x8000, 0x0);
    sMainMenuButtons[MENU_BUTTON_OPTION_RETURN]->oMenuButtonScale = MENU_BUTTON_SCALE;
#else
    // Zoom in current selection
    sMainMenuButtons[MENU_BUTTON_SOUND_OPTION_MIN + sSoundMode]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN;
#endif
}

/**
 * In the sound mode menu, checks if a button was clicked to change sound mode & button state.
 */
void check_sound_mode_menu_clicked_buttons(struct Object *soundModeButton) {
    if (soundModeButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        s32 buttonID;
        // Configure sound mode menu button group
        for (buttonID = MENU_BUTTON_OPTION_MIN; buttonID < MENU_BUTTON_OPTION_MAX; buttonID++) {
            s16 buttonX = sMainMenuButtons[buttonID]->oPosX;
            s16 buttonY = sMainMenuButtons[buttonID]->oPosY;

            if (check_clicked_button(buttonX, buttonY, 22.0f) == TRUE) {
                // If sound mode button clicked, select it and define sound mode
                // The check will always be true because of the group configured above (In JP & US)
                if (buttonID >= MENU_BUTTON_SOUND_OPTION_MIN && buttonID < MENU_BUTTON_SOUND_OPTION_MAX) {
                    if (soundModeButton->oMenuButtonActionPhase == SOUND_MODE_PHASE_MAIN) {
                        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
                        queue_rumble_data(5, 80);
#endif
                        sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
#ifndef MULTILANG
                        // Sound menu buttons don't return to Main Menu with multilang enabled
                        sSelectedButtonID = buttonID;
#endif
                        sSoundMode = buttonID - MENU_BUTTON_SOUND_OPTION_MIN;
                        save_file_set_sound_mode(sSoundMode);
                    }
                }
#ifdef MULTILANG
                // If neither of the buttons above are pressed, return to main menu
                if (buttonID == MENU_BUTTON_OPTION_RETURN) {
                    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
                    sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_ZOOM_IN_OUT;
                    sSelectedButtonID = buttonID;
                }
#endif
                sCurrentMenuLevel = MENU_LAYER_SUBMENU;

                break;
            }
        }
    }
}

extern struct SaveBuffer gSaveBuffer;

/**
 * Loads a save file selected after it goes into a full screen state
 * retuning sSelectedFileNum to a save value defined in fileNum.
 */
void load_main_menu_save_file(struct Object *fileButton, s32 fileNum) {
    if (fileButton->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        sSelectedFileNum = fileNum;
    
        if (!Randomizer_gIsSetSeed) {
            Randomizer_gGameSeed = (u32)(random_float() * 10000000);
        } else {
            Randomizer_gGameSeed = get_entered_seed();
        }

        if (sSelectingAdventure == 0)
        {
            sSelectingAdventure = (gSaveBuffer.menuData.optionsFlags & 0x40000000) ? 3 : 1;
            gSaveBuffer.menuData.optionsFlags |= 0x40000000;
        }
        
        Randomizer_init_randomizer(fileNum);
    }
}

/**
 * Clears a section of sMainMenuButtons.
 */
void delete_menu_button_objects(s16 minID, s16 maxID) {
    for (s16 buttonID = minID; buttonID < maxID; buttonID++) {
        obj_mark_for_deletion(sMainMenuButtons[buttonID]);
    }
}

/**
 * Hides buttons of corresponding button menu groups.
 */
void hide_submenu_buttons(s16 prevMenuButtonID) {
    switch (prevMenuButtonID) {
        case MENU_BUTTON_SCORE:      delete_menu_button_objects(MENU_BUTTON_SEED_MIN,   MENU_BUTTON_SEED_MAX  ); break;
        case MENU_BUTTON_COPY:       delete_menu_button_objects(MENU_BUTTON_COPY_MIN,   MENU_BUTTON_COPY_MAX  ); break;
        case MENU_BUTTON_ERASE:      delete_menu_button_objects(MENU_BUTTON_ERASE_MIN,  MENU_BUTTON_ERASE_MAX ); break;
        case MENU_BUTTON_SOUND_MODE: delete_menu_button_objects(MENU_BUTTON_OPTION_MIN, MENU_BUTTON_OPTION_MAX); break;
    }
}

/**
 * Returns from the previous menu back to the main menu using
 * the return button (or sound mode) as source button.
 */
void return_to_main_menu(s16 prevMenuButtonID, struct Object *sourceButton) {
    // If the source button is in default state and the previous menu in full screen,
    // play zoom out sound and shrink previous menu
    if (sourceButton->oMenuButtonState == MENU_BUTTON_STATE_DEFAULT
        && sMainMenuButtons[prevMenuButtonID]->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) {
        play_sound(SOUND_MENU_CAMERA_ZOOM_OUT, gGlobalSoundSource);
        sMainMenuButtons[prevMenuButtonID]->oMenuButtonState = MENU_BUTTON_STATE_SHRINKING;
        sCurrentMenuLevel = MENU_LAYER_MAIN;
    }
    // If the previous button is in default state, return back to the main menu
    if (sMainMenuButtons[prevMenuButtonID]->oMenuButtonState == MENU_BUTTON_STATE_DEFAULT) {
        sSelectedButtonID = MENU_BUTTON_NONE;
        hide_submenu_buttons(prevMenuButtonID);
    }
}

void load_menu_from_submenu(s16 prevMenuButtonID, s16 selectedButtonID, struct Object *sourceButton) {
    // If the source button is in default state and the previous menu in full screen,
    // play zoom out sound and shrink previous menu
    if ((sourceButton->oMenuButtonState == MENU_BUTTON_STATE_DEFAULT)
     && (sMainMenuButtons[prevMenuButtonID]->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN)) {
        play_sound(SOUND_MENU_CAMERA_ZOOM_OUT, gGlobalSoundSource);
        sMainMenuButtons[prevMenuButtonID]->oMenuButtonState = MENU_BUTTON_STATE_SHRINKING;
        sCurrentMenuLevel = MENU_LAYER_MAIN;
    }
    // If the previous button is in default state
    if (sMainMenuButtons[prevMenuButtonID]->oMenuButtonState == MENU_BUTTON_STATE_DEFAULT) {
        if (selectedButtonID != prevMenuButtonID) {
            hide_submenu_buttons(prevMenuButtonID);
        }
        // Play zoom in sound, select score menu and render it's buttons
        sSelectedButtonID = selectedButtonID;
        play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gGlobalSoundSource);
        sMainMenuButtons[selectedButtonID]->oMenuButtonState = MENU_BUTTON_STATE_GROWING;
        render_menu_buttons(selectedButtonID);
    }
}

// Loads score menu from the previous menu using "CHECK SCORE" as source button.
void load_score_menu_from_submenu(s16 prevMenuButtonID, struct Object *sourceButton) {
    load_menu_from_submenu(prevMenuButtonID, MENU_BUTTON_SCORE, sourceButton);
}

// Loads copy menu from the previous menu using "COPY FILE" as source button.
void load_copy_menu_from_submenu(s16 prevMenuButtonID, struct Object *sourceButton) {
    load_menu_from_submenu(prevMenuButtonID, MENU_BUTTON_COPY, sourceButton);
}

// Loads erase menu from the previous menu using "ERASE FILE" as source button.
void load_erase_menu_from_submenu(s16 prevMenuButtonID, struct Object *sourceButton) {
    load_menu_from_submenu(prevMenuButtonID, MENU_BUTTON_ERASE, sourceButton);
}


static const Vec3s sSaveFileButtonInitPositions[] = {
    { -6400, 2800, 0 }, // SAVE_FILE_A
    {  1500, 2800, 0 }, // SAVE_FILE_B
    { -6400,    0, 0 }, // SAVE_FILE_C
    {  1500,    0, 0 }, // SAVE_FILE_D
};

#define SPAWN_FILE_SELECT_FILE_BUTTON_INIT(saveFile)                                                                                            \
    spawn_object_rel_with_rot(o, (save_file_exists(saveFile) ? MODEL_MAIN_MENU_MARIO_SAVE_BUTTON_FADE : MODEL_MAIN_MENU_MARIO_NEW_BUTTON_FADE), \
                              bhvMenuButton,                                                                                                    \
                              sSaveFileButtonInitPositions[saveFile][0],                                                                        \
                              sSaveFileButtonInitPositions[saveFile][1],                                                                        \
                              sSaveFileButtonInitPositions[saveFile][2],                                                                        \
                              0x0, 0x0, 0x0)

/**
 * Menu Buttons Menu Manager Initial Action
 * Creates models of the buttons in the menu. For the Mario buttons it
 * checks if a save file exists to render an specific button model for it.
 * Unlike buttons on submenus, these are never hidden or recreated.
 */
void bhv_menu_button_manager_init(void) {
    // File A
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_A] = SPAWN_FILE_SELECT_FILE_BUTTON_INIT(SAVE_FILE_A);
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_A]->oMenuButtonScale = 1.0f;
    // File B
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_B] = SPAWN_FILE_SELECT_FILE_BUTTON_INIT(SAVE_FILE_B);
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_B]->oMenuButtonScale = 1.0f;
    // File C
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_C] = SPAWN_FILE_SELECT_FILE_BUTTON_INIT(SAVE_FILE_C);
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_C]->oMenuButtonScale = 1.0f;
    // File D
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_D] = SPAWN_FILE_SELECT_FILE_BUTTON_INIT(SAVE_FILE_D);
    sMainMenuButtons[MENU_BUTTON_PLAY_FILE_D]->oMenuButtonScale = 1.0f;
    // Score menu button
    sMainMenuButtons[MENU_BUTTON_SCORE] = spawn_object_rel_with_rot(
        gCurrentObject, MODEL_MAIN_MENU_GREEN_SCORE_BUTTON, bhvMenuButton, -6400, -3500, 0, 0, 0, 0);
    sMainMenuButtons[MENU_BUTTON_SCORE]->oMenuButtonScale = 1.0f;
    // Copy menu button
    sMainMenuButtons[MENU_BUTTON_COPY] =
        spawn_object_rel_with_rot(o, MODEL_MAIN_MENU_BLUE_COPY_BUTTON,
                                  bhvMenuButton, -2134, -3500, 0, 0x0, 0x0, 0x0);
    sMainMenuButtons[MENU_BUTTON_COPY]->oMenuButtonScale = 1.0f;
    // Erase menu button
    sMainMenuButtons[MENU_BUTTON_ERASE] =
        spawn_object_rel_with_rot(o, MODEL_MAIN_MENU_RED_ERASE_BUTTON,
                                  bhvMenuButton,  2134, -3500, 0, 0x0, 0x0, 0x0);
    sMainMenuButtons[MENU_BUTTON_ERASE]->oMenuButtonScale = 1.0f;
    // Sound mode menu button (Option Mode in EU)
    sMainMenuButtons[MENU_BUTTON_SOUND_MODE] =
        spawn_object_rel_with_rot(o, MODEL_MAIN_MENU_PURPLE_SOUND_BUTTON,
                                  bhvMenuButton,  6400, -3500, 0, 0x0, 0x0, 0x0);
    sMainMenuButtons[MENU_BUTTON_SOUND_MODE]->oMenuButtonScale = 1.0f;

    gDialogTextAlpha = 0;
}

/**
 * In the main menu, check if a button was clicked to play it's button growing state.
 * Also play a sound and/or render buttons depending of the button ID selected.
 */
void check_main_menu_clicked_buttons(void) {
    // Sound mode menu is handled separately because the button ID for it
    // is not grouped with the IDs of the other submenus.
    if (check_clicked_button(sMainMenuButtons[MENU_BUTTON_SOUND_MODE]->oPosX,
                                sMainMenuButtons[MENU_BUTTON_SOUND_MODE]->oPosY, 200.0f)) {
        sMainMenuButtons[MENU_BUTTON_SOUND_MODE]->oMenuButtonState = MENU_BUTTON_STATE_GROWING;
        sSelectedButtonID = MENU_BUTTON_SOUND_MODE;
    } else {
        // Main Menu buttons
        s8 buttonID;
        // Configure Main Menu button group
        for (buttonID = MENU_BUTTON_MAIN_MIN; buttonID < MENU_BUTTON_MAIN_MAX; buttonID++) {
            s16 buttonX = sMainMenuButtons[buttonID]->oPosX;
            s16 buttonY = sMainMenuButtons[buttonID]->oPosY;

            if (check_clicked_button(buttonX, buttonY, 200.0f)) {
                // If menu button clicked, select it
                sMainMenuButtons[buttonID]->oMenuButtonState = MENU_BUTTON_STATE_GROWING;
                sSelectedButtonID = buttonID;
                break;
            }
        }
    }

#ifdef MULTILANG
    // Open Options Menu if sOpenLangSettings is TRUE (It's TRUE when there's no saves)
    if (sOpenLangSettings && (sMainMenuTimer >= 5)) {
        sMainMenuButtons[MENU_BUTTON_SOUND_MODE]->oMenuButtonState = MENU_BUTTON_STATE_GROWING;
        sSelectedButtonID = MENU_BUTTON_SOUND_MODE;
        sOpenLangSettings = FALSE;
    }
#endif

    // Play sound of the save file clicked
    switch (sSelectedButtonID) {
        case MENU_BUTTON_PLAY_FILE_A:
        case MENU_BUTTON_PLAY_FILE_B:
        case MENU_BUTTON_PLAY_FILE_C:
        case MENU_BUTTON_PLAY_FILE_D:
            play_sound(SOUND_MENU_STAR_SOUND_OKEY_DOKEY, gGlobalSoundSource);
#if ENABLE_RUMBLE
            queue_rumble_data(60, 70);
            queue_rumble_decay(1);
#endif
            break;
        // Play sound of the button clicked and render buttons of that menu.
        case MENU_BUTTON_SCORE:
            play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gGlobalSoundSource);
            seed_menu_create_buttons(sMainMenuButtons[MENU_BUTTON_SCORE]);
            break;
        case MENU_BUTTON_COPY:
        case MENU_BUTTON_ERASE:
            play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gGlobalSoundSource);
#if ENABLE_RUMBLE
            queue_rumble_data(5, 80);
#endif
            render_menu_buttons(sSelectedButtonID);
            break;
        case MENU_BUTTON_SOUND_MODE:
            play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gGlobalSoundSource);
#if ENABLE_RUMBLE
            queue_rumble_data(5, 80);
#endif
            render_sound_mode_menu_buttons(sMainMenuButtons[MENU_BUTTON_SOUND_MODE]);
            break;
    }
}

/**
 * Menu Buttons Menu Manager Loop Action
 * Calls a menu function depending of the button chosen.
 * sSelectedButtonID is MENU_BUTTON_NONE when the file select
 * is loaded, and that checks what buttonID is clicked in the main menu.
 */
extern void save_main_menu_data(void);
extern void seq_player_play_sequence(u8 player, u8 seqId, u16 arg2);
void bhv_menu_button_manager_loop(void) {
#if 0
    static int music = 1;
    if (gPlayer1Controller->buttonPressed & R_JPAD)
    {
        music++;
        seq_player_play_sequence(0, music, 0);
    }
    if (gPlayer1Controller->buttonPressed & L_JPAD)
    {
        if (music)
            music--;

        seq_player_play_sequence(0, music, 0);
    }

    print_text_fmt_int(20, 20, "%d", music);
#endif
    switch (sSelectedButtonID) {
        case MENU_BUTTON_NONE: check_main_menu_clicked_buttons(); break;

        case MENU_BUTTON_PLAY_FILE_A: load_main_menu_save_file(sMainMenuButtons[MENU_BUTTON_PLAY_FILE_A], 1); break;
        case MENU_BUTTON_PLAY_FILE_B: load_main_menu_save_file(sMainMenuButtons[MENU_BUTTON_PLAY_FILE_B], 2); break;
        case MENU_BUTTON_PLAY_FILE_C: load_main_menu_save_file(sMainMenuButtons[MENU_BUTTON_PLAY_FILE_C], 3); break;
        case MENU_BUTTON_PLAY_FILE_D: load_main_menu_save_file(sMainMenuButtons[MENU_BUTTON_PLAY_FILE_D], 4); break;

        case MENU_BUTTON_SCORE: seed_menu_check_clicked_buttons(sMainMenuButtons[MENU_BUTTON_SCORE]); break;
        case MENU_BUTTON_COPY:  check_copy_menu_clicked_buttons (sMainMenuButtons[MENU_BUTTON_COPY ]); break;
        case MENU_BUTTON_ERASE: check_erase_menu_clicked_buttons(sMainMenuButtons[MENU_BUTTON_ERASE]); break;

        case MENU_BUTTON_SCORE_FILE_A: exit_score_file_to_score_menu(sMainMenuButtons[MENU_BUTTON_SCORE_FILE_A], MENU_BUTTON_SCORE); break;
        case MENU_BUTTON_SCORE_FILE_B: exit_score_file_to_score_menu(sMainMenuButtons[MENU_BUTTON_SCORE_FILE_B], MENU_BUTTON_SCORE); break;
        case MENU_BUTTON_SCORE_FILE_C: exit_score_file_to_score_menu(sMainMenuButtons[MENU_BUTTON_SCORE_FILE_C], MENU_BUTTON_SCORE); break;
        case MENU_BUTTON_SCORE_FILE_D: exit_score_file_to_score_menu(sMainMenuButtons[MENU_BUTTON_SCORE_FILE_D], MENU_BUTTON_SCORE); break;

        case MENU_BUTTON_SCORE_RETURN:     return_to_main_menu         (MENU_BUTTON_SCORE, sMainMenuButtons[MENU_BUTTON_SCORE_RETURN    ]); break;
        case MENU_BUTTON_SCORE_COPY_FILE:  load_copy_menu_from_submenu (MENU_BUTTON_SCORE, sMainMenuButtons[MENU_BUTTON_SCORE_COPY_FILE ]); break;
        case MENU_BUTTON_SCORE_ERASE_FILE: load_erase_menu_from_submenu(MENU_BUTTON_SCORE, sMainMenuButtons[MENU_BUTTON_SCORE_ERASE_FILE]); break;

        case MENU_BUTTON_COPY_FILE_A: break;
        case MENU_BUTTON_COPY_FILE_B: break;
        case MENU_BUTTON_COPY_FILE_C: break;
        case MENU_BUTTON_COPY_FILE_D: break;

        case MENU_BUTTON_COPY_RETURN:      return_to_main_menu         (MENU_BUTTON_COPY, sMainMenuButtons[MENU_BUTTON_COPY_RETURN     ]); break;
        case MENU_BUTTON_COPY_CHECK_SCORE: load_score_menu_from_submenu(MENU_BUTTON_COPY, sMainMenuButtons[MENU_BUTTON_COPY_CHECK_SCORE]); break;
        case MENU_BUTTON_COPY_ERASE_FILE:  load_erase_menu_from_submenu(MENU_BUTTON_COPY, sMainMenuButtons[MENU_BUTTON_COPY_ERASE_FILE ]); break;

        case MENU_BUTTON_ERASE_FILE_A: break;
        case MENU_BUTTON_ERASE_FILE_B: break;
        case MENU_BUTTON_ERASE_FILE_C: break;
        case MENU_BUTTON_ERASE_FILE_D: break;

        case MENU_BUTTON_ERASE_RETURN:      return_to_main_menu         (MENU_BUTTON_ERASE, sMainMenuButtons[MENU_BUTTON_ERASE_RETURN     ]); break;
        case MENU_BUTTON_ERASE_CHECK_SCORE: load_score_menu_from_submenu(MENU_BUTTON_ERASE, sMainMenuButtons[MENU_BUTTON_ERASE_CHECK_SCORE]); break;
        case MENU_BUTTON_ERASE_COPY_FILE:   load_copy_menu_from_submenu (MENU_BUTTON_ERASE, sMainMenuButtons[MENU_BUTTON_ERASE_COPY_FILE  ]); break;

        case MENU_BUTTON_SOUND_MODE: check_sound_mode_menu_clicked_buttons(sMainMenuButtons[MENU_BUTTON_SOUND_MODE]); break;

        case MENU_BUTTON_SELECT_SEED_RETURN:  return_to_main_menu(MENU_BUTTON_SCORE, sMainMenuButtons[MENU_BUTTON_SELECT_SEED_RETURN]); break;
        case MENU_BUTTON_SELECT_SEED_RESET:   break;
        case MENU_BUTTON_SELECT_SEED_OPTIONS: seed_menu_options_check_clicked_buttons(sMainMenuButtons[MENU_BUTTON_SELECT_SEED_OPTIONS]); break;

#ifdef MULTILANG
        case MENU_BUTTON_OPTION_RETURN: return_to_main_menu(MENU_BUTTON_SOUND_MODE, sMainMenuButtons[MENU_BUTTON_OPTION_RETURN]); break;
#endif
        // STEREO, MONO and HEADSET buttons are undefined so they can be selected without
        // exiting the Options menu, as a result they added a return button
        case MENU_BUTTON_STEREO:  return_to_main_menu(MENU_BUTTON_SOUND_MODE, sMainMenuButtons[MENU_BUTTON_STEREO ]); break;
        case MENU_BUTTON_MONO:    return_to_main_menu(MENU_BUTTON_SOUND_MODE, sMainMenuButtons[MENU_BUTTON_MONO   ]); break;
#ifdef ENABLE_STEREO_HEADSET_EFFECTS
        case MENU_BUTTON_HEADSET: return_to_main_menu(MENU_BUTTON_SOUND_MODE, sMainMenuButtons[MENU_BUTTON_HEADSET]); break;
#endif
    }

    sClickPos[0] = -10000;
    sClickPos[1] = -10000;
}

/**
 * Cursor function that handles button inputs.
 * If the cursor is clicked, sClickPos uses the same value as sCursorPos.
 */
void handle_cursor_button_input(void) {
    // If scoring a file, pressing A just changes the coin score mode.
    if (sSelectedButtonID == MENU_BUTTON_SCORE_FILE_A || sSelectedButtonID == MENU_BUTTON_SCORE_FILE_B
        || sSelectedButtonID == MENU_BUTTON_SCORE_FILE_C
        || sSelectedButtonID == MENU_BUTTON_SCORE_FILE_D) {
        if (gPlayer1Controller->buttonPressed & (B_BUTTON | START_BUTTON | Z_TRIG)) {
            sClickPos[0] = sCursorPos[0];
            sClickPos[1] = sCursorPos[1];
            sCursorClickingTimer = 1;
        } else if (gPlayer1Controller->buttonPressed & A_BUTTON) {
            sScoreFileCoinScoreMode = 1 - sScoreFileCoinScoreMode;
            play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
        }
    } else { // If cursor is clicked
        if (gPlayer1Controller->buttonPressed
            & (A_BUTTON | B_BUTTON | START_BUTTON)) {
            sClickPos[0] = sCursorPos[0];
            sClickPos[1] = sCursorPos[1];
            sCursorClickingTimer = 1;
        }
    }
}

/**
 * Cursor function that handles analog stick input and button presses with a function near the end.
 */
void handle_controller_cursor_input(void) {
    s16 rawStickX = gPlayer1Controller->rawStickX;
    s16 rawStickY = gPlayer1Controller->rawStickY;

    // Handle deadzone
    if (rawStickY > -2 && rawStickY < 2) {
        rawStickY = 0;
    }
    if (rawStickX > -2 && rawStickX < 2) {
        rawStickX = 0;
    }

//    // Move cursor
//    if (sSelectedButtonID == MENU_BUTTON_SCORE) {
//        sCursorPos[0] += rawStickX / 12;
//        sCursorPos[1] += rawStickY / 12;
//    } else {
        sCursorPos[0] += rawStickX / 8;
        sCursorPos[1] += rawStickY / 8;
//    }

    // Stop cursor from going offscreen
    if (sCursorPos[0] > 132.0f) {
        sCursorPos[0] = 132.0f;
    }
    if (sCursorPos[0] < -132.0f) {
        sCursorPos[0] = -132.0f;
    }

    if (sCursorPos[1] > 90.0f) {
        sCursorPos[1] = 90.0f;
    }
    if (sCursorPos[1] < -90.0f) {
        sCursorPos[1] = -90.0f;
    }

    if (sCursorClickingTimer == 0) {
        handle_cursor_button_input();
    }
}

/**
 * Prints the cursor (Mario Hand, different to the one in the Mario screen)
 * and loads it's controller inputs in handle_controller_cursor_input
 * to be usable on the file select.
 */
void print_menu_cursor(void) {
    handle_controller_cursor_input();
    create_dl_translation_matrix(MENU_MTX_PUSH, sCursorPos[0] + 160.0f - 5.0, sCursorPos[1] + 120.0f - 25.0, 0.0f);
    // Get the right graphic to use for the cursor.
    if (sCursorClickingTimer == 0) { // Idle
        gSPDisplayList(gDisplayListHead++, dl_menu_idle_hand);
    }
    if (sCursorClickingTimer != 0) { // Grabbing
        gSPDisplayList(gDisplayListHead++, dl_menu_grabbing_hand);
    }
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    if (sCursorClickingTimer != 0) {
        sCursorClickingTimer++; // This is a very strange way to implement a timer? It counts up and
                                // then resets to 0 instead of just counting down to 0.
        if (sCursorClickingTimer == 5) {
            sCursorClickingTimer = 0;
        }
    }
}

/**
 * Takes a number between 0 and 3 and formats the corresponding file letter A to D into a buffer.
 * If the language is set to Japanese, the letter is written in full-width digits.
 */
void string_format_file_letter(char *buf, const char *str, s32 fileIndex) {
    char letterBuf[4];
#ifdef ENABLE_JAPANESE
    if (gInGameLanguage == LANGUAGE_JAPANESE) {
        // The UTF-8 encoding of "Ａ" is 0xEF, 0xBC, 0xA1
        letterBuf[0] = 0xEF;
        letterBuf[1] = 0xBC;
        letterBuf[2] = 0xA1 + fileIndex;
        letterBuf[3] = '\0';
        sprintf(buf, str, letterBuf);
        return;
    }
#endif

    letterBuf[0] = 'A' + fileIndex;
    letterBuf[1] = '\0';
    sprintf(buf, str, letterBuf);
}

/**
 * Prints a hud string with text fade properties.
 */
void print_hud_lut_string_fade(s16 x, s16 y, const char *text, u32 alignment) {
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDialogTextAlpha -= sTextFadeAlpha;
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_hud_lut_string_aligned(x, y, text, alignment);
    gDialogTextAlpha += sTextFadeAlpha;
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
}

/**
 * Prints a generic white string with text fade properties.
 */
void print_generic_string_fade(s16 x, s16 y, const char *text, u32 alignment) {
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDialogTextAlpha -= sTextFadeAlpha;
    set_text_color(255, 255, 255);
    print_generic_string_aligned(x, y, text, alignment);
    gDialogTextAlpha += sTextFadeAlpha;
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

/**
 * Updates text fade at the top of a menu.
 */
s32 update_text_fade_out(void) {
    if (sFadeOutText == TRUE) {
        sTextFadeAlpha += 50;
        if (sTextFadeAlpha == 250) {
            sFadeOutText = FALSE;
            return TRUE;
        }
    } else {
        if (sTextFadeAlpha > 0) {
            sTextFadeAlpha -= 50;
        }
    }
    return FALSE;
}

LangArray textMarioA = DEFINE_LANGUAGE_ARRAY(
    "MARIO A",
    "MARIO A",
    "MARIO A",
    "マリオＡ",
    "MARIO A");

LangArray textMarioB = DEFINE_LANGUAGE_ARRAY(
    "MARIO B",
    "MARIO B",
    "MARIO B",
    "マリオＢ",
    "MARIO B");

LangArray textMarioC = DEFINE_LANGUAGE_ARRAY(
    "MARIO C",
    "MARIO C",
    "MARIO C",
    "マリオＣ",
    "MARIO C");

LangArray textMarioD = DEFINE_LANGUAGE_ARRAY(
    "MARIO D",
    "MARIO D",
    "MARIO D",
    "マリオＤ",
    "MARIO D");

// namespace Randomizer
#define MARIOTEXT_X1 (submenu ? 89 : 92)
#define MARIOTEXT_X2 (submenu ? 211 : 207)
#define MARIOTEXT_Y1 (submenu ? 62 : 65)
#define MARIOTEXT_Y2 105

#define SEEDTEXT_X1 (submenu ? 45 : 50)
#define SEEDTEXT_X2 (submenu ? 166 : 165)
#define SEEDTEXT_Y1 (submenu ? 52 : 56)
#define SEEDTEXT_Y2 (submenu ? 136 : 135)

extern struct SaveBuffer gSaveBuffer;

static void print_file_names_and_seeds(u32 submenu) {
    u32 i, xpos, ypos;
    // Print file names
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_menu_generic_string(MARIOTEXT_X1, MARIOTEXT_Y1, LANG_ARRAY(textMarioA));
    print_menu_generic_string(MARIOTEXT_X2, MARIOTEXT_Y1, LANG_ARRAY(textMarioB));
    print_menu_generic_string(MARIOTEXT_X1, MARIOTEXT_Y2, LANG_ARRAY(textMarioC));
    print_menu_generic_string(MARIOTEXT_X2, MARIOTEXT_Y2, LANG_ARRAY(textMarioD));
    // For each file, if it exists, print the seed
    for (i = 0; i < 4; i++) {
        if (save_file_exists(i)) {
            char seed[8];
            sprintf(seed, "%07d", gSaveBuffer.files[i][0].seed);
            xpos = i % 2 ? SEEDTEXT_X2 : SEEDTEXT_X1;
            ypos = i < 2 ? SEEDTEXT_Y1 : SEEDTEXT_Y2;
            print_menu_generic_string(xpos - 7, ypos, seed);
        }
    }
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
}
// }

/**
 * Prints the amount of stars of a save file.
 * If a save doesn't exist, print "NEW" instead.
 */
LangArray textNew = DEFINE_LANGUAGE_ARRAY(
    "NEW",
    "VIDE",
    "FREI",
    "NEW",
    "NUEVO");

void print_save_file_star_count(s8 fileIndex, s16 x, s16 y) {
    char starCountText[10];

    if (save_file_exists(fileIndex)) {
        s16 starCount = save_file_get_total_star_count(fileIndex,
                                                       COURSE_NUM_TO_INDEX(COURSE_MIN),
                                                       COURSE_NUM_TO_INDEX(COURSE_MAX));

        if (starCount < 100) {
            sprintf(starCountText, "★×%d", starCount);
        } else {
            sprintf(starCountText, "★%d", starCount);
        }
        print_hud_lut_string(x, y, starCountText);
    } else {
        // Print "new" text
        print_hud_lut_string(x, y, LANG_ARRAY(textNew));
    }
}

LangArray textSelectFile = DEFINE_LANGUAGE_ARRAY(
    "SELECT FILE",
    "CHOISIR  FICHIER",
    "WÄHLE SPIEL",
    "ファイルセレクト",
    "ELIGE ARCHIVO");

static const char textScore[] = "RANDO";
LangArray textCopy = DEFINE_LANGUAGE_ARRAY(
    "COPY",
    "COPIER",
    "KOPIEREN",
    "コピー",
    "COPIAR");

LangArray textErase = DEFINE_LANGUAGE_ARRAY(
    "ERASE",
    "EFFACER",
    "LÖSCHEN",
    "けす",
    "BORRAR");

LangArray textSoundModeStereo = DEFINE_LANGUAGE_ARRAY(
    "STEREO",
    "STÉRÉO",
    "STEREO",
    "ステレオ",
    "ESTÉREO");

LangArray textSoundModeMono = DEFINE_LANGUAGE_ARRAY(
    "MONO",
    "MONO",
    "MONO",
    "モノラル",
    "MONO");

LangArray textSoundModeHeadset = DEFINE_LANGUAGE_ARRAY(
    "HEADSET",
    "CASQUE",
    "PHONES",
    "ヘッドホン",
    "CASCOS");

LangArray *textSoundModes[] = {
    &textSoundModeStereo,
    &textSoundModeMono,
#ifdef ENABLE_STEREO_HEADSET_EFFECTS
    &textSoundModeHeadset,
#endif
};

#ifdef MULTILANG
LangArray textOption = DEFINE_LANGUAGE_ARRAY(
    "OPTION",
    "OPTION",
    "OPTIONEN",
    "オプション",
    "OPCIONES");
#endif

/**
 * Prints main menu strings that shows on the yellow background menu screen.
 *
 * In EU this function acts like "print_save_file_strings" because
 * print_main_lang_strings is first called to render the strings for the 4 buttons.
 * Same rule applies for score, copy and erase strings.
 */
void print_main_menu_strings(void) {
    // Print "SELECT FILE" text
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_hud_lut_string_aligned(SCREEN_CENTER_X, 35, LANG_ARRAY(textSelectFile), TEXT_ALIGN_CENTER);
    // Print file star counts
    print_save_file_star_count(SAVE_FILE_A, 92, 78);
    print_save_file_star_count(SAVE_FILE_B, 209, 78);
    print_save_file_star_count(SAVE_FILE_C, 92, 118);
    print_save_file_star_count(SAVE_FILE_D, 209, 118);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
    // Print menu names
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    set_text_color(255, 255, 255);
    print_generic_string_aligned(67, 39, LANG_ARRAY(textScore), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(130, 39, LANG_ARRAY(textCopy), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(191, 39, LANG_ARRAY(textErase), TEXT_ALIGN_CENTER);
#ifdef MULTILANG
    print_generic_string_aligned(253, 39, LANG_ARRAY(textOption), TEXT_ALIGN_CENTER);
#else
    print_generic_string_aligned(253, 39, LANG_ARRAY(*textSoundModes[sSoundMode]), TEXT_ALIGN_CENTER);
#endif
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    // Print file names
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_menu_generic_string(92, 65, LANG_ARRAY(textMarioA));
    print_menu_generic_string(207, 65, LANG_ARRAY(textMarioB));
    print_menu_generic_string(92, 105, LANG_ARRAY(textMarioC));
    print_menu_generic_string(207, 105, LANG_ARRAY(textMarioD));
    print_menu_generic_string(20, 215, "RANDO v0.3, Original by ArthurTilly, port by aglab2");
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
    
    print_file_names_and_seeds(FALSE);
}

static const char sChoose[] = "Choose your experience:";
static const char sCanChange1[] = "You can change any configs";
static const char sCanChange2[] = "in Pause Menu options.";

static const char sClassicTitle[] = "Classic Preset";
static const char sClassicDesc1[] = "Play N64 compatible original";
static const char sClassicDesc2[] = "Star Road with minimal changes.";

static const char sQoLTitle[] = "Quality of Life Preset";
static const char sQoLDesc1[] = "Adds modifications to levels that";
static const char sQoLDesc2[] = "improve gameplay experience.";

static const char sModernTitle[] = "Modern Preset";
static const char sModernDesc1[] = "Quality of Life preset with";
static const char sModernDesc2[] = "improvements to game mechanics.";

static int get_selected_adventure_mode()
{
    f32 x = sCursorPos[0];
    f32 y = sCursorPos[1];

    if (ABS(x) > 120.f)
    {
        return 0;
    }

     if (76.0f > y && y > 40.0f) {
        return 1;
    } else if (25.0f > y && y > -10.0f) {
        return 2;
    } else if (-22.0f > y && y > -58.0f) {
        return 3;
    }

    return 0;
}

static void print_select_adventure()
{
    int selectedMode = 0;
    if (gDialogTextAlpha == 250)
    {
        selectedMode = get_selected_adventure_mode();
    }

    if (selectedMode && gPlayer1Controller->buttonPressed & A_BUTTON)
    {
        play_sound(SOUND_MENU_STAR_SOUND_OKEY_DOKEY, gGlobalSoundSource);
        sSelectingAdventure = 2;
        set_preset(selectedMode - 1);
        save_file_save_all_config();
    }

    gSPDisplayList(gDisplayListHead++, dl_shade_screen_begin);

    int unselectedAlpha = gDialogTextAlpha * 2 / 3;
    int selectedAlpha   = gDialogTextAlpha * 5 / 6;

    gDPSetPrimColor(gDisplayListHead++, 0, 0, 0, 0, 0, selectedMode == 1 ? selectedAlpha : unselectedAlpha);
    gDPFillRectangle(gDisplayListHead++, 40, 50 - 5 , SCREEN_WIDTH - 40, 74  + 12);
    gDPSetPrimColor(gDisplayListHead++, 0, 0, 0, 0, 0, selectedMode == 2 ? selectedAlpha : unselectedAlpha);
    gDPFillRectangle(gDisplayListHead++, 40, 100 - 5, SCREEN_WIDTH - 40, 124 + 12);
    gDPSetPrimColor(gDisplayListHead++, 0, 0, 0, 0, 0, selectedMode == 3 ? selectedAlpha : unselectedAlpha);
    gDPFillRectangle(gDisplayListHead++, 40, 150 - 5, SCREEN_WIDTH - 40, 174 + 12);
    gSPDisplayList(gDisplayListHead++, dl_shade_screen_end);

    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);

    gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, gDialogTextAlpha);
    print_menu_generic_string_aligned(160, 20, sChoose, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 204, sCanChange1, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 214, sCanChange2, TEXT_ALIGN_CENTER);

    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_menu_generic_string_aligned(160, 50, sClassicTitle, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 64, sClassicDesc1, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 74, sClassicDesc2, TEXT_ALIGN_CENTER);

    print_menu_generic_string_aligned(160, 100, sQoLTitle, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 114, sQoLDesc1, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 124, sQoLDesc2, TEXT_ALIGN_CENTER);

    print_menu_generic_string_aligned(160, 150, sModernTitle, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 164, sModernDesc1, TEXT_ALIGN_CENTER);
    print_menu_generic_string_aligned(160, 174, sModernDesc2, TEXT_ALIGN_CENTER);

    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
}

LangArray textCheckFile = DEFINE_LANGUAGE_ARRAY(
    "CHECK FILE",
    "VOIR  SCORE",
    "VON WELCHEM SPIEL",
    "どのスコアをみる？",
    "VER ARCHIVO");

LangArray textNoSavedDataExists = DEFINE_LANGUAGE_ARRAY(
    "NO SAVED DATA EXISTS",
    "AUCUNE SAUVEGARDE DISPONIBLE",
    "KEIN SPIEL VORHANDEN",
    "ファイルにデータがありません",
    "NO HAY DATOS GUARDADOS");

/**
 * Defines IDs for the top message of the score menu and displays it if the ID is called in messageID.
 */
void score_menu_display_message(s8 messageID) {

    switch (messageID) {
        case SCORE_MSG_CHECK_FILE:
            print_hud_lut_string_fade(SCREEN_CENTER_X, 35, LANG_ARRAY(textCheckFile), TEXT_ALIGN_CENTER);
            break;
        case SCORE_MSG_NOSAVE_DATA:
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textNoSavedDataExists), TEXT_ALIGN_CENTER);
            break;
    }
}

#define SUBMENU_LEFT_BUTTON_X 62
#define SUBMENU_MIDDLE_BUTTON_X  160
#define SUBMENU_RIGHT_BUTTON_X 258

#define FADEOUT_TIMER 20

LangArray textReturn = DEFINE_LANGUAGE_ARRAY(
    "RETURN",
    "RETOUR",
    "ZURÜCK",
    "もどる",
    "VOLVER");

LangArray textCopyFileButton = DEFINE_LANGUAGE_ARRAY(
    "COPY FILE",
    "COPIER",
    "KOPIEREN",
    "ファイルコピー",
    "COPIAR ARCHIVO");

LangArray textEraseFileButton = DEFINE_LANGUAGE_ARRAY(
    "ERASE FILE",
    "EFFACER",
    "LÖSCHEN",
    "ファイルけす",
    "BORRAR ARCHIVO");

/**
 * Prints score menu strings that shows on the green background menu screen.
 */
void print_score_menu_strings(void) {

    // Update and print the message at the top of the menu.
    if (sMainMenuTimer == FADEOUT_TIMER) {
        sFadeOutText = TRUE;
    }
    if (update_text_fade_out()) {
        if (sStatusMessageID == SCORE_MSG_CHECK_FILE) {
            sStatusMessageID = SCORE_MSG_NOSAVE_DATA;
        } else {
            sStatusMessageID = SCORE_MSG_CHECK_FILE;
        }
    }
    // Print messageID called above
    score_menu_display_message(sStatusMessageID);

    // Print file star counts
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_save_file_star_count(SAVE_FILE_A, 90, 76);
    print_save_file_star_count(SAVE_FILE_B, 211, 76);
    print_save_file_star_count(SAVE_FILE_C, 90, 119);
    print_save_file_star_count(SAVE_FILE_D, 211, 119);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);

    // Print menu names
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    set_text_color(255, 255, 255);
    print_generic_string_aligned(SUBMENU_LEFT_BUTTON_X, 35, LANG_ARRAY(textReturn), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SUBMENU_MIDDLE_BUTTON_X, 35, LANG_ARRAY(textCopyFileButton), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SUBMENU_RIGHT_BUTTON_X, 35, LANG_ARRAY(textEraseFileButton), TEXT_ALIGN_CENTER);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);

    // Print file names
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_menu_generic_string(89, 62, LANG_ARRAY(textMarioA));
    print_menu_generic_string(211, 62, LANG_ARRAY(textMarioB));
    print_menu_generic_string(89, 105, LANG_ARRAY(textMarioC));
    print_menu_generic_string(211, 105, LANG_ARRAY(textMarioD));
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);

    print_file_names_and_seeds(TRUE);
}

LangArray textCopyFile = DEFINE_LANGUAGE_ARRAY(
    "COPY FILE",
    "COPIER  FICHIER",
    "SPIEL KOPIEREN",
    "ファイルコピーする",
    "COPIAR ARCHIVO");

LangArray textCopyItToWhere = DEFINE_LANGUAGE_ARRAY(
    "COPY IT TO WHERE?",
    "COPIER SUR?",
    "WOHIN KOPIEREN?",
    "どこにコピーしますか？",
    "¿COPIARLO A DÓNDE?");

LangArray textCopyCompleted = DEFINE_LANGUAGE_ARRAY(
    "COPYING COMPLETED",
    "COPIE ACHEVEÉ",
    "SPIEL KOPIERT",
    "コピーおわりました",
    "COPIA COMPLETADA");

LangArray textSavedDataExists = DEFINE_LANGUAGE_ARRAY(
    "SAVED DATA EXISTS",
    "SAVEGARDE EXISTANTE",
    "BEREITS BELEGT",
    "ファイルにデータがはいってます",
    "YA EXISTEN DATOS GUARDADOS");

LangArray textNoFileToCopyFrom = DEFINE_LANGUAGE_ARRAY(
    "NO EMPTY FILE",
    "AUCUN FICHIER VIDE",
    "KEIN PLATZ VORHANDEN",
    "からのファイルがありません",
    "NO HAY NINGÚN ARCHIVO VACÍO");

/**
 * Defines IDs for the top message of the copy menu and displays it if the ID is called in messageID.
 */
void copy_menu_display_message(s8 messageID) {

    switch (messageID) {
        case COPY_MSG_MAIN_TEXT:
            if (sAllFilesExist) {
                print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textNoFileToCopyFrom), TEXT_ALIGN_CENTER);
            } else {
                print_hud_lut_string_fade(SCREEN_CENTER_X, 35, LANG_ARRAY(textCopyFile), TEXT_ALIGN_CENTER);
            }
            break;
        case COPY_MSG_COPY_WHERE:
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textCopyItToWhere), TEXT_ALIGN_CENTER);
            break;
        case COPY_MSG_NOSAVE_EXISTS:
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textNoSavedDataExists), TEXT_ALIGN_CENTER);
            break;
        case COPY_MSG_COPY_COMPLETE:
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textCopyCompleted), TEXT_ALIGN_CENTER);
            break;
        case COPY_MSG_SAVE_EXISTS:
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textSavedDataExists), TEXT_ALIGN_CENTER);
            break;
    }
}

/**
 * Updates messageIDs of the copy menu depending of the copy phase value defined.
 */
void copy_menu_update_message(void) {
    switch (sMainMenuButtons[MENU_BUTTON_COPY]->oMenuButtonActionPhase) {
        case COPY_PHASE_MAIN:
            if (sMainMenuTimer == FADEOUT_TIMER) {
                sFadeOutText = TRUE;
            }
            if (update_text_fade_out() == TRUE) {
                if (sStatusMessageID == COPY_MSG_MAIN_TEXT) {
                    sStatusMessageID = COPY_MSG_NOSAVE_EXISTS;
                } else {
                    sStatusMessageID = COPY_MSG_MAIN_TEXT;
                }
            }
            break;
        case COPY_PHASE_COPY_WHERE:
            if (sMainMenuTimer == FADEOUT_TIMER
                && sStatusMessageID == COPY_MSG_SAVE_EXISTS) {
                sFadeOutText = TRUE;
            }
            if (update_text_fade_out() == TRUE) {
                if (sStatusMessageID != COPY_MSG_COPY_WHERE) {
                    sStatusMessageID = COPY_MSG_COPY_WHERE;
                } else {
                    sStatusMessageID = COPY_MSG_SAVE_EXISTS;
                }
            }
            break;
        case COPY_PHASE_COPY_COMPLETE:
            if (sMainMenuTimer == FADEOUT_TIMER) {
                sFadeOutText = TRUE;
            }
            if (update_text_fade_out() == TRUE) {
                if (sStatusMessageID != COPY_MSG_COPY_COMPLETE) {
                    sStatusMessageID = COPY_MSG_COPY_COMPLETE;
                } else {
                    sStatusMessageID = COPY_MSG_MAIN_TEXT;
                }
            }
            break;
    }
}

LangArray textViewScore = DEFINE_LANGUAGE_ARRAY(
    "CHECK SCORE",
    "SCORE",
    "LEISTUNG",
    "スコアをみる",
    "VER RÉCORDS");

/**
 * Prints copy menu strings that shows on the blue background menu screen.
 */
void print_copy_menu_strings(void) {

    // Update and print the message at the top of the menu.
    copy_menu_update_message();
    // Print messageID called inside a copy_menu_update_message case
    copy_menu_display_message(sStatusMessageID);
    // Print file star counts
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_save_file_star_count(SAVE_FILE_A, 90, 76);
    print_save_file_star_count(SAVE_FILE_B, 211, 76);
    print_save_file_star_count(SAVE_FILE_C, 90, 119);
    print_save_file_star_count(SAVE_FILE_D, 211, 119);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
    // Print menu names
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    set_text_color(255, 255, 255);
    print_generic_string_aligned(SUBMENU_LEFT_BUTTON_X, 35, LANG_ARRAY(textReturn), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SUBMENU_MIDDLE_BUTTON_X, 35, LANG_ARRAY(textViewScore), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SUBMENU_RIGHT_BUTTON_X, 35, LANG_ARRAY(textEraseFileButton), TEXT_ALIGN_CENTER);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    // Print file names
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_menu_generic_string(89, 62, LANG_ARRAY(textMarioA));
    print_menu_generic_string(211, 62, LANG_ARRAY(textMarioB));
    print_menu_generic_string(89, 105, LANG_ARRAY(textMarioC));
    print_menu_generic_string(211, 105, LANG_ARRAY(textMarioD));
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);

    print_file_names_and_seeds(TRUE);
}

LangArray textYes = DEFINE_LANGUAGE_ARRAY(
    "YES",
    "OUI",
    "JA",
    "はい",
    "SÍ");

LangArray textNo = DEFINE_LANGUAGE_ARRAY(
    "NO",
    "NON",
    "NEIN",
    "いいえ",
    "NO");

/**
 * Prints the "YES NO" prompt and checks if one of the prompts are hovered to do it's functions.
 */
void print_erase_menu_prompt(s16 x, s16 y) {
    s16 colorFade = gGlobalTimer << 12;

    s16 cursorX = sCursorPos[0] + x + 70.f;
    s16 cursorY = sCursorPos[1] + 120.0f;

    if (cursorX < 169 && cursorX >= 140 &&
        cursorY < 210 && cursorY >= 191) {
        // Fade "YES" string color but keep "NO" gray
        sYesNoColor[0] = sins(colorFade) * 50.0f + 205.0f;
        sYesNoColor[1] = 150;
        sEraseYesNoHoverState = MENU_ERASE_HOVER_YES;
    } else if (cursorX < 218 && cursorX >= 189
        && cursorY < 210 && cursorY >= 191) {
        // Fade "NO" string color but keep "YES" gray
        sYesNoColor[0] = 150;
        sYesNoColor[1] = sins(colorFade) * 50.0f + 205.0f;
        sEraseYesNoHoverState = MENU_ERASE_HOVER_NO;
    } else {
        // Don't fade both strings and keep them gray
        sYesNoColor[0] = 150;
        sYesNoColor[1] = 150;
        sEraseYesNoHoverState = MENU_ERASE_HOVER_NONE;
    }
    // If the cursor is clicked...
    if (sCursorClickingTimer == 2) {
        // ..and is hovering "YES", delete file
        if (sEraseYesNoHoverState == MENU_ERASE_HOVER_YES) {
            play_sound(SOUND_MARIO_WAAAOOOW, gGlobalSoundSource);
#if ENABLE_RUMBLE
            queue_rumble_data(5, 80);
#endif
            sMainMenuButtons[MENU_BUTTON_ERASE]->oMenuButtonActionPhase = ERASE_PHASE_MARIO_ERASED;
            sFadeOutText = TRUE;
            sMainMenuTimer = 0;
            save_file_erase(sSelectedFileIndex);
            sMainMenuButtons[MENU_BUTTON_ERASE_MIN + sSelectedFileIndex]->header.gfx.sharedChild =
                gLoadedGraphNodes[MODEL_MAIN_MENU_MARIO_NEW_BUTTON_FADE];
            sMainMenuButtons[sSelectedFileIndex]->header.gfx.sharedChild =
                gLoadedGraphNodes[MODEL_MAIN_MENU_MARIO_NEW_BUTTON_FADE];
            sEraseYesNoHoverState = MENU_ERASE_HOVER_NONE;
            // ..and is hovering "NO", return back to main phase
        } else if (sEraseYesNoHoverState == MENU_ERASE_HOVER_NO) {
            play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
#if ENABLE_RUMBLE
            queue_rumble_data(5, 80);
#endif
            sMainMenuButtons[MENU_BUTTON_ERASE_MIN + sSelectedFileIndex]->oMenuButtonState =
                MENU_BUTTON_STATE_ZOOM_OUT;
            sMainMenuButtons[MENU_BUTTON_ERASE]->oMenuButtonActionPhase = ERASE_PHASE_MAIN;
            sFadeOutText = TRUE;
            sMainMenuTimer = 0;
            sEraseYesNoHoverState = MENU_ERASE_HOVER_NONE;
        }
    }

    // Print "YES NO" strings
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    set_text_color(sYesNoColor[0], sYesNoColor[0], sYesNoColor[0]);
    print_generic_string(x + 56, y, LANG_ARRAY(textYes));
    set_text_color(sYesNoColor[1], sYesNoColor[1], sYesNoColor[1]);
    print_generic_string(x + 98, y, LANG_ARRAY(textNo));
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

LangArray textEraseFile = DEFINE_LANGUAGE_ARRAY(
    "ERASE FILE",
    "EFFACER  FICHIER",
    "SPIEL LÖSCHEN",
    "ファイルけす",
    "BORRAR ARCHIVO");

LangArray textSure = DEFINE_LANGUAGE_ARRAY(
    "SURE?",
    "OK?",
    "SICHER?",
    "ほんと？",
    "¿SEGURO?");

LangArray textMarioXJustErased = DEFINE_LANGUAGE_ARRAY(
    "MARIO %s JUST ERASED",
    "MARIO %s EFFACÉ",
    "MARIO %s GELÖSCHT",
    "マリオ%sをけしました",
    "MARIO %s ELIMINADO");

/**
 * Defines IDs for the top message of the erase menu and displays it if the ID is called in messageID.
 */
void erase_menu_display_message(s8 messageID) {
    char str[50];
    switch (messageID) {
        case ERASE_MSG_MAIN_TEXT:
            print_hud_lut_string_fade(SCREEN_CENTER_X, 35, LANG_ARRAY(textEraseFile), TEXT_ALIGN_CENTER);
            break;
        case ERASE_MSG_PROMPT:
            print_generic_string_fade(90, 190, LANG_ARRAY(textSure), TEXT_ALIGN_LEFT);
            print_erase_menu_prompt(90, 190); // YES NO, has functions for it too
            break;
        case ERASE_MSG_NOSAVE_EXISTS:
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textNoSavedDataExists), TEXT_ALIGN_CENTER);
            break;
        case ERASE_MSG_MARIO_ERASED:
            string_format_file_letter(str, LANG_ARRAY(textMarioXJustErased), sSelectedFileIndex);
            print_generic_string_fade(SCREEN_CENTER_X, 190, str, TEXT_ALIGN_CENTER);
            break;
        case ERASE_MSG_SAVE_EXISTS: // unused
            print_generic_string_fade(SCREEN_CENTER_X, 190, LANG_ARRAY(textSavedDataExists), TEXT_ALIGN_CENTER);
            break;
    }
}

/**
 * Updates messageIDs of the erase menu depending of the erase phase value defined.
 */
void erase_menu_update_message(void) {
    switch (sMainMenuButtons[MENU_BUTTON_ERASE]->oMenuButtonActionPhase) {
        case ERASE_PHASE_MAIN:
            if (sMainMenuTimer == FADEOUT_TIMER
                && sStatusMessageID == ERASE_MSG_NOSAVE_EXISTS) {
                sFadeOutText = TRUE;
            }
            if (update_text_fade_out() == TRUE) {
                if (sStatusMessageID == ERASE_MSG_MAIN_TEXT) {
                    sStatusMessageID = ERASE_MSG_NOSAVE_EXISTS;
                } else {
                    sStatusMessageID = ERASE_MSG_MAIN_TEXT;
                }
            }
            break;
        case ERASE_PHASE_PROMPT:
            if (update_text_fade_out() == TRUE) {
                if (sStatusMessageID != ERASE_MSG_PROMPT) {
                    sStatusMessageID = ERASE_MSG_PROMPT;
                }
                sCursorPos[0] = 43.0f;
                sCursorPos[1] = 80.0f;
            }
            break;
        case ERASE_PHASE_MARIO_ERASED:
            if (sMainMenuTimer == FADEOUT_TIMER) {
                sFadeOutText = TRUE;
            }
            if (update_text_fade_out() == TRUE) {
                if (sStatusMessageID != ERASE_MSG_MARIO_ERASED) {
                    sStatusMessageID = ERASE_MSG_MARIO_ERASED;
                } else {
                    sStatusMessageID = ERASE_MSG_MAIN_TEXT;
                }
            }
            break;
    }
}

/**
 * Prints erase menu strings that shows on the red background menu screen.
 */
void print_erase_menu_strings(void) {

    // Update and print the message at the top of the menu.
    erase_menu_update_message();

    // Print messageID called inside a erase_menu_update_message case
    erase_menu_display_message(sStatusMessageID);

    // Print file star counts
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_save_file_star_count(SAVE_FILE_A, 90, 76);
    print_save_file_star_count(SAVE_FILE_B, 211, 76);
    print_save_file_star_count(SAVE_FILE_C, 90, 119);
    print_save_file_star_count(SAVE_FILE_D, 211, 119);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);

    // Print menu names
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    set_text_color(255, 255, 255);
    print_generic_string_aligned(SUBMENU_LEFT_BUTTON_X, 35, LANG_ARRAY(textReturn), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SUBMENU_MIDDLE_BUTTON_X, 35, LANG_ARRAY(textViewScore), TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SUBMENU_RIGHT_BUTTON_X, 35, LANG_ARRAY(textCopyFileButton), TEXT_ALIGN_CENTER);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);

    // Print file names
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_menu_generic_string(89, 62, LANG_ARRAY(textMarioA));
    print_menu_generic_string(211, 62, LANG_ARRAY(textMarioB));
    print_menu_generic_string(89, 105, LANG_ARRAY(textMarioC));
    print_menu_generic_string(211, 105, LANG_ARRAY(textMarioD));
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);

    print_file_names_and_seeds(TRUE);
}

static s16 sSeedSelectCharPositions[10][2] = { { 0, -60 }, { -30, 30 }, { 0, 30 }, { 30, 30 },
                                               { -30, 0 }, { 0, 0 },    { 30, 0 }, { -30, -30 },
                                               { 0, -30 }, { 30, -30 } };

static void seed_menu_get_clicked_numbers(void) {
    s16 cursorX = sCursorPos[0] + 160.0f;
    s16 cursorY = sCursorPos[1] + 115.0f;
    int i, j;
    if (sCursorClickingTimer == 2) {
        for (i = 0; i < 10; i++) {
            if ((cursorX < (175 + sSeedSelectCharPositions[i][0]))
                && (cursorX > (145 + sSeedSelectCharPositions[i][0]))
                && (cursorY < (105 + sSeedSelectCharPositions[i][1]))
                && (cursorY > (75 + sSeedSelectCharPositions[i][1]))) {
                Randomizer_gIsSetSeed = TRUE;
                Randomizer_gOverwriteFileSeed = TRUE;
                if ((textEnteredNumbers[0] != '0') || ((get_entered_seed() * 10 + i) > 9999999))
                    play_sound(SOUND_MENU_CAMERA_BUZZ, gGlobalSoundSource);
                else {
                    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
                    for (j = 0; j < 6; j++) {
                        textEnteredNumbers[j] = textEnteredNumbers[j + 1];
                    }
                    textEnteredNumbers[6] = i + '0';
                }
            }
        }
    }
}

static void draw_select_seed_menu(void) {
    seed_menu_get_clicked_numbers();
    // Display "SOUND SELECT" text
     gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
     gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_hud_lut_string(97, 35, textEnterSeed);
    // Display mode names
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_generic_string(55, 170, textSeedShouldBe);

    print_generic_string(237, 33, textReturn);
    print_generic_string(50, 33, textReset);
    print_generic_string(44, 87, textOptions);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    if (!Randomizer_gIsSetSeed)
        print_hud_lut_string(125, 80, textRandom);
    else {
        print_hud_lut_string(130 - 10, 80, textEnteredNumbers);
    }

    for (u32 i = 0; i < 10; i++) {
        char textSeedInput[] = "0";
        textSeedInput[0] += i;
        print_hud_lut_string(152 + sSeedSelectCharPositions[i][0],
                             142 - sSeedSelectCharPositions[i][1], textSeedInput);
    }

    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
}

extern void seq_player_fade_to_target_volume(s32 player, s32 fadeDuration, u8 targetVolume);

static void applyPreset() {
    Randomizer_curPreset = (Randomizer_curPreset + textCountPresets) % textCountPresets;
    
    Randomizer_gOptionsSettings = Randomizer_gPresets[Randomizer_curPreset];
    seq_player_fade_to_target_volume(SEQ_PLAYER_LEVEL, 20, 65);
    play_sound(SOUND_MENU_STAR_SOUND, gGlobalSoundSource);
}

#define MENUHEIGHT 17

#define OPTIONS_Y(line) (160 - MENUHEIGHT * (line))

void options_page_print_options(u32 textCount, const char *textList[]) {
    u32 i;
    
    gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, gDialogTextAlpha);
    for (i = 0; i < textCount; i++) {
        print_generic_string(23 + 1, OPTIONS_Y(i) - 1, textList[i]);
    }
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    for (i = 0; i < textCount; i++) {
        print_generic_string(23, OPTIONS_Y(i), textList[i]);
    }
}

void options_page_print_two(u32 currentSelected, s16 y, s16 x1, s16 x2, const char *str1, const char *str2) {
    u8 rgbVal = (currentSelected ? 40 : 255);
    gDPSetEnvColor(gDisplayListHead++, rgbVal, rgbVal, rgbVal, gDialogTextAlpha);
    print_generic_string(x1, y, str1);
    
    rgbVal = (255+40) - rgbVal;
    gDPSetEnvColor(gDisplayListHead++, rgbVal, rgbVal, rgbVal, gDialogTextAlpha);
    print_generic_string(x2, y, str2);
}

void options_page_print_on_off(u32 isOn, s16 y, s16 x1, s16 x2) {
    options_page_print_two(isOn, y, x1, x2, "OFF", "ON");
}

void options_page_print_three(u32 currentSelected, s16 y, 
    s16 x1, s16 x2, s16 x3, const char *str1, const char *str2, const char *str3) {

    u8 rgbVal = (currentSelected == 0 ? 255 : 40);
    gDPSetEnvColor(gDisplayListHead++, rgbVal, rgbVal, rgbVal, gDialogTextAlpha);
    print_generic_string(x1, y, str1);
    
    rgbVal = (currentSelected == 1 ? 255 : 40);
    gDPSetEnvColor(gDisplayListHead++, rgbVal, rgbVal, rgbVal, gDialogTextAlpha);
    print_generic_string(x2, y, str2);
    
    rgbVal = (currentSelected == 2 ? 255 : 40);
    gDPSetEnvColor(gDisplayListHead++, rgbVal, rgbVal, rgbVal, gDialogTextAlpha);
    print_generic_string(x3, y, str3);
}

static const char *textsCosmetic[] = {
    "MARIO COLORS",
    "STAR COLORS",
    "COIN COLORS",
    "RANDOM SKYBOXES",
    "RANDOM MUSIC",
};
#define textCountCosmetics (sizeof(textsCosmetic) / 4)

static void page_cosmetics() {
    u32 i;
    
    if (check_clicked_text(180, OPTIONS_Y(0), 0)){
        Randomizer_gOptionsSettings.cosmetic.s.marioColors = 0;
    }
    if (check_clicked_text(222, OPTIONS_Y(0), 0)){
        Randomizer_gOptionsSettings.cosmetic.s.marioColors = 1;
    }

    s32 temp = Randomizer_gOptionsSettings.cosmetic.s.starColors;
    if (check_clicked_text(236, OPTIONS_Y(1), 0)) {
        temp++;
    } else if (check_clicked_text(159, OPTIONS_Y(1), 0)) {
        temp--;
    }
    temp = (temp + 4) % 4;
    Randomizer_gOptionsSettings.cosmetic.s.starColors = temp;

    if (check_clicked_text(171, OPTIONS_Y(4), 1)){
        Randomizer_gOptionsSettings.cosmetic.s.musicOn = 0;
        seq_player_fade_to_target_volume(SEQ_PLAYER_LEVEL, 20, 65);
    }
    if (check_clicked_text(206, OPTIONS_Y(4), 1)){
        Randomizer_gOptionsSettings.cosmetic.s.musicOn = 1;
        seq_player_fade_to_target_volume(SEQ_PLAYER_LEVEL, 20, 65);
    }
    if (check_clicked_text_width(240, OPTIONS_Y(4), 1, 45)){
        Randomizer_gOptionsSettings.cosmetic.s.musicOn = 2;
        seq_player_fade_to_target_volume(SEQ_PLAYER_LEVEL, 20, 0);
    }
    
    for (i = 2; i < 4; i++) {
        if (check_clicked_text(180, OPTIONS_Y(i), i)) {
            COSMETIC_VARS_SET(i, 0)
        } else if (check_clicked_text(222, OPTIONS_Y(i), i)) {
            COSMETIC_VARS_SET(i, 1)
        }
    }
}

static const char *textsObjects[] = {
    "SPAWN DIFFICULTY",
    "OBJECT TYPES",
    "RANDOMIZE STAR SPAWNS"
};
#define textCountObjects (sizeof(textsObjects) / 4)

static void page_objects() {
    u32 i;
    if (check_clicked_text(156, OPTIONS_Y(0), 0)){
        Randomizer_gOptionsSettings.gameplay.s.safeSpawns = Randomizer_SPAWN_SAFETY_SAFE;
    }
    if (check_clicked_text_width(191, OPTIONS_Y(0), 0, 45)){
        Randomizer_gOptionsSettings.gameplay.s.safeSpawns = Randomizer_SPAWN_SAFETY_DEFAULT;
    }
    if (check_clicked_text_width(240, OPTIONS_Y(0), 0, 45)){
        Randomizer_gOptionsSettings.gameplay.s.safeSpawns = Randomizer_SPAWN_SAFETY_HARD;
    }

    for (i = 1; i < textCountObjects; i++) {
        if (check_clicked_text(180, OPTIONS_Y(i), i)) {
            OBJECT_VARS_SET(i, 0);
        } else if (check_clicked_text(222, OPTIONS_Y(i), i)) {
            OBJECT_VARS_SET(i, 1);
        }
    }
}

static const char *textsModes[] = {
    "KEEP STRUCTURE",
    "NONSTOP MODE",
    "GREEN DEMON MODE",
};

#define textCountModes ((sizeof(textsModes) - 4) / 4)

static void page_modes() {
    if (check_clicked_text(180, OPTIONS_Y(0), 0)) {
        Randomizer_gOptionsSettings.gameplay.s.keepStructure = 0;
    } else if (check_clicked_text(222, OPTIONS_Y(0), 0)) {
        Randomizer_gOptionsSettings.gameplay.s.keepStructure = 1;
    }

    if (check_clicked_text(166, OPTIONS_Y(1), 0)){
        Randomizer_gOptionsSettings.gameplay.s.nonstopMode = 0;
    }
    if (check_clicked_text(197, OPTIONS_Y(1), 0)){
        Randomizer_gOptionsSettings.gameplay.s.nonstopMode = 1;
    }
    if (check_clicked_text_width(231, OPTIONS_Y(1), 0, 45)){
        Randomizer_gOptionsSettings.gameplay.s.nonstopMode = 2;
    }

    if (check_clicked_text(180, OPTIONS_Y(2), 0)) {
        Randomizer_gOptionsSettings.gameplay.s.demonOn = 0;
    } else if (check_clicked_text(222, OPTIONS_Y(2), 0)) {
        Randomizer_gOptionsSettings.gameplay.s.demonOn = 1;
    }
}

static void page_presets() {
    if (check_clicked_text(280, OPTIONS_Y(1) + 10, 0)) {
        Randomizer_curPreset++;
        applyPreset();
    } else if (check_clicked_text(170, OPTIONS_Y(1) + 10, 0)) {
        Randomizer_curPreset--;
        applyPreset();
    }
}

static const char *textsWarps[] = {
    "B3 STARS NEEDED",
    "RANDOMIZE LEVEL SPAWN",
    "RANDOMIZE LEVEL WARPS",
    "ADJUST WARP EXITS",
    "RANDOMIZE STAR DOORS"
};

#define textCountWarps (sizeof(textsWarps) / 4)

static void page_warps() {
    u32 i;
    s32 temp = Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement;
    if (check_clicked_text(222, OPTIONS_Y(0), 0)) {
        temp++;
    } else if (check_clicked_text(175, OPTIONS_Y(0), 0)) {
        temp--;
    }
    temp = (temp + 13) % 13;
    Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement = temp;
    
    if (check_clicked_text(170, OPTIONS_Y(4), 0)){
        Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts = 0; // ON
    }
    if (check_clicked_text(202, OPTIONS_Y(4), 0)){
        Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts = 1; // OFF

    }
    if (check_clicked_text(230, OPTIONS_Y(4), 0)){
        Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts = 2; // No requirements
    }
    for (i = 1; i < textCountWarps-1; i++) {
        // If level warps are off, cant change adjusted exits
        if ((i == 3) && (!Randomizer_gOptionsSettings.gameplay.s.randomLevelWarp)) continue;
        if (check_clicked_text(180, OPTIONS_Y(i), i)) {
            WARPS_VARS_SET(i, 0)
            // If level warps set to off, disable adjusted exits
            if (i == 2) {
                Randomizer_gOptionsSettings.gameplay.s.adjustedExits = 0;
            }
        } else if (check_clicked_text(222, OPTIONS_Y(i), i)) {
            WARPS_VARS_SET(i, 1)
        }
    }
}

struct InfoDisplay {
    char *text;
    u32 width;
    u32 height;
};

s32 infoAlpha = 0;
s32 prevInfoDisplay = -1;

void display_box(u32 x, u32 y, u32 width, u32 height) {
    gDPPipeSync(gDisplayListHead++);
    gDPSetCombineMode(gDisplayListHead++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gDisplayListHead++, 0, 0, 0, 0, 0, (infoAlpha / 255.f * 180));
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPFillRectangle(gDisplayListHead++, x, y, x + width, y + height);
}

void handle_info_display(const struct InfoDisplay displays[], s32 count) {
    s32 i;
    s16 cursorX = sCursorPos[0] + 165.f;
    s16 cursorY = sCursorPos[1] + 110.0f;
    u32 displaying = FALSE;
    if (gDialogTextAlpha < 250) return;
    for (i = 0; i < count; i++) {
        // Check if cursor is hovering over text
        if (cursorX > 10 && cursorX < 130 && cursorY >= (152 - MENUHEIGHT * i) && cursorY <= (152 - MENUHEIGHT * i + 16)) {
            if (prevInfoDisplay != i) {
                infoAlpha = 0;
                prevInfoDisplay = i;
            } else if ((gPlayer1Controller->rawStickX != 0) || (gPlayer1Controller->rawStickY != 0)) {
                infoAlpha -= 40;
                if (infoAlpha < 0) infoAlpha = 0;
            } else if (infoAlpha < 255) {
                infoAlpha += 20;
                if (infoAlpha > 255) infoAlpha = 255;
            }

            gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
            display_box(cursorX + 20.f, SCREEN_HEIGHT - (cursorY + 18.f), displays[i].width, displays[i].height * 16 + 5);\
            gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
            gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, infoAlpha);
            print_generic_string(cursorX + 25.f, cursorY, displays[i].text);
            displaying = TRUE;
        }
    }
    if (!displaying) {
        infoAlpha = 0;
        prevInfoDisplay = -1;
    }
}


struct InfoDisplay aestheticInfo[] = {
    {"\
Randomize the colors of Mario's\n\
model. Selecting CLOTHES will\n\
keep his hair and skin their\n\
regular color.", 170, 4},

    {"\
Randomize the color of stars.\n\
Star colors can be unique for\n\
every star, tied to the level,\n\
or fixed for the whole game.", 157, 4},

    {"\
Randomize the color of yellow,\n\
red and blue coins. Coin colors\n\
will always be relatively\n\
distinct from each other.", 162, 4},

    {"\
Randomize the skybox displayed\n\
in the background of each level.", 168, 2},

    {"\
Randomize the music that plays\n\
within each level, or during\n\
events. Selecting MUTED will\n\
play no music at all.", 168, 4},
};

char *textsStarSettings[] = {
    "OFF",
    "PER STAR",
    "PER LEVEL",
    "GLOBAL"
};

u8 textsStarSettingsX[] = {
    205,
    191,
    189,
    197
};

void page_cosmetics_print() {
    u32 i;
    
    options_page_print_options(textCountCosmetics, textsCosmetic);
    
    options_page_print_on_off(Randomizer_gOptionsSettings.cosmetic.s.marioColors, OPTIONS_Y(0), 190, 222);

    options_page_print_three(Randomizer_gOptionsSettings.cosmetic.s.musicOn, OPTIONS_Y(4),
        171, 206, 240, "OFF", "ON", "MUTED");

    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_generic_string(174, OPTIONS_Y(1), textPlus);
    print_generic_string(textsStarSettingsX[Randomizer_gOptionsSettings.cosmetic.s.starColors], OPTIONS_Y(1),
                             textsStarSettings[Randomizer_gOptionsSettings.cosmetic.s.starColors]);
    print_generic_string(244, OPTIONS_Y(1), textMinus);

    for (i = 2; i < 4; i++) {
        options_page_print_on_off(COSMETIC_VARS_GET(i), OPTIONS_Y(i), 190, 222);
    }

    handle_info_display(aestheticInfo, textCountCosmetics);
    
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
}

struct InfoDisplay objectInfo[] = {
    {"\
Controls the average difficulty of\n\
object placements, including factors\n\
like height and ground steepness.", 188, 3},
    {"\
Controls which kinds of objects are\n\
randomized. If set to KEY, only\n\
objects that are directly required\n\
to obtain stars are randomized.", 184, 4},
    {"\
Whether stars that are spawned\n\
by other objects have their\n\
positions randomized or not.", 170, 3},
};

void page_objects_print() {
    options_page_print_options(textCountObjects, textsObjects);
    
    options_page_print_three(Randomizer_gOptionsSettings.gameplay.s.safeSpawns, OPTIONS_Y(0),
        156, 191, 240, "SAFE", "NORMAL", "DANGER");

    options_page_print_two(Randomizer_gOptionsSettings.gameplay.s.objectRandomization, OPTIONS_Y(1),
        190, 222, "KEY", "ALL");

    options_page_print_on_off(OBJECT_VARS_GET(2), OPTIONS_Y(2), 190, 222);

    handle_info_display(objectInfo, textCountObjects);
    
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
}

static const struct InfoDisplay modeInfo[] = {
    {"\
Determines the main gameplay structure\n\
of the playthrough. If set to ON, keys\n\
will be required to progress and levels\n\
will be separated into the three main\n\
areas. Otherwise, key doors will be\n\
disabled and levels will be fully random.", 212, 6},

    {"\
With Nonstop mode, collecting a\n\
star won't take you out of the level.\n\
If SAVE is chosen, stars will bring\n\
up a save prompt when collected,\n\
otherwise they will be collected\n\
instantly.", 190, 6},

    {"\
Enables Green Demon mode, where a\n\
1up will chase you through all\n\
levels and will kill you on contact.", 182, 3},
};

void page_modes_print() {
    options_page_print_options(textCountModes, textsModes);

    options_page_print_on_off(Randomizer_gOptionsSettings.gameplay.s.keepStructure, OPTIONS_Y(0), 190, 222);
    //options_page_print_on_off(Randomizer_gOptionsSettings.gameplay.s.demonOn, OPTIONS_Y(2), 190, 222);
    
    options_page_print_three(Randomizer_gOptionsSettings.gameplay.s.nonstopMode, OPTIONS_Y(1),
        166, 197, 231, "OFF", "SAVE", "NOSAVE");

    handle_info_display(modeInfo, textCountModes);

    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
}

void page_presets_print() {
    u32 i = 0;
    for (i = 0; i < 2; i++) {
        u32 color = i * 255; // 0 on first run, 255 on second
        u32 offset = 1 - i; // 1 on first run, 0 on second
        gDPSetEnvColor(gDisplayListHead++, color, color, color, gDialogTextAlpha);

        print_generic_string(28 + offset, OPTIONS_Y(1) + 10 - offset, textUsePreset);
        if (Randomizer_curPreset >= 0) {
            print_generic_string_aligned(230 + offset, OPTIONS_Y(1) + 10 - offset, textsPresets[Randomizer_curPreset], TEXT_ALIGN_CENTER);
            print_generic_string(40 + offset, OPTIONS_Y(4) + 10 - offset, textsPresetDescriptions[Randomizer_curPreset]);
        } else {
            print_generic_string(210 + offset, OPTIONS_Y(1) + 10 - offset, textPresetCustom);
        }
    }

    print_generic_string(170, OPTIONS_Y(1) + 10, textPlus);
    print_generic_string(280, OPTIONS_Y(1) + 10, textMinus);

    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
}

static const struct InfoDisplay warpInfo[] = {
    {"\
Sets the number of stars\n\
required to enter Bowser\n\
in the Sky.", 137, 3},

    {"\
Determines if Mario's starting\n\
position is randomized whenever\n\
he enters a level.", 166, 3},

    {"\
Determines if the level entrances\n\
lead to random levels, or if they\n\
lead to their original level.", 173, 3},

    {"\
Adjusted exits will return Mario\n\
outside of the painting he just\n\
entered, rather than the painting\n\
of the level he was just in.", 175, 4},

    {"\
Whether to randomize the star\n\
requirements for accessing new\n\
levels. Setting to NONE will\n\
remove all star requirements.", 166, 4},
};

void page_warps_print() {
    u32 i;
    char strNumStars[4];
    
    options_page_print_options(textCountWarps, textsWarps);

    sprintf(strNumStars, "%d", Randomizer_gStarDoorReqLUT[Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement]);
    print_generic_string(190, OPTIONS_Y(0), textPlus);
    if (Randomizer_gStarDoorReqLUT[Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement] < 10) {
        print_generic_string(211, OPTIONS_Y(0), strNumStars);
    } else if (Randomizer_gStarDoorReqLUT[Randomizer_gOptionsSettings.gameplay.s.starDoorRequirement] < 100) {
        print_generic_string(207, OPTIONS_Y(0), strNumStars);
    } else {
        print_generic_string(203, OPTIONS_Y(0), strNumStars);
    }
    print_generic_string(230, OPTIONS_Y(0), textMinus);


    options_page_print_three(Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts, OPTIONS_Y(4),
        170, 202, 230, "OFF", "ON", "NONE");

    for (i = 1; i < textCountWarps-1; i++) {
        // If level warps are off, adjusted exits is greyed out
        if ((i == 3) && (!Randomizer_gOptionsSettings.gameplay.s.randomLevelWarp)) {
                gDPSetEnvColor(gDisplayListHead++, 96, 96, 96, gDialogTextAlpha);
                print_generic_string(190, OPTIONS_Y(i), "OFF");
                gDPSetEnvColor(gDisplayListHead++, 96, 96, 96, gDialogTextAlpha);
                print_generic_string(222, OPTIONS_Y(i), "ON");
        } else {
            options_page_print_on_off(WARPS_VARS_GET(i), OPTIONS_Y(i), 190, 222);
        }
    }

    handle_info_display(warpInfo, textCountWarps);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
}

static void draw_select_seed_menu_option(void) {
    u8 bottomOptionColor = sins((u16)(gGlobalTimer*0x800))*40 + 215;
    char buf[30];
    struct Randomizer_OptionsSettings oldSettings = Randomizer_gOptionsSettings;
    switch (OptionPage) {
        case 0:
            page_cosmetics();
            break;
        case 1:
            page_objects();
            break;
        case 2:
            page_warps();
            break;
        case 3:
            page_presets();
            break;
        case 4:
            page_modes();
            break;
    }
    // Check if options have been modified
    if ((oldSettings.gameplay.w != Randomizer_gOptionsSettings.gameplay.w) || (oldSettings.cosmetic.w != Randomizer_gOptionsSettings.cosmetic.w)) {
        Randomizer_gOverwriteFileOptions = TRUE;
        if (OptionPage != 3) {
            Randomizer_curPreset = -1;
        }
    }
    if (gPlayer1Controller->buttonPressed & R_TRIG) {
        OptionPage--;
        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
        prevInfoDisplay = -1;
    } else if (gPlayer1Controller->buttonPressed & (Z_TRIG | L_TRIG)) {
        OptionPage++;
        play_sound(SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
        prevInfoDisplay = -1;
    }
    OptionPage += pageCount;
    OptionPage = OptionPage % pageCount;
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    print_hud_lut_string(23, 35, pages[OptionPage]);

    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);

    gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, gDialogTextAlpha);
    print_generic_string(240+1, 33-1, textReturn);
    print_generic_string(35+1,33-1,textRandomOptions);
    sprintf(buf, "Settings ID: %d", Randomizer_gOptionsSettings.gameplay.w);
    print_generic_string(10,9,buf);
    gDPSetEnvColor(gDisplayListHead++, bottomOptionColor, bottomOptionColor, bottomOptionColor, gDialogTextAlpha);
    print_generic_string(240, 33, textReturn);
    print_generic_string(35, 33, textRandomOptions);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    sprintf(buf, "Settings ID: %d", Randomizer_gOptionsSettings.gameplay.w);
    print_generic_string(9,10,buf);

    switch (OptionPage) {
        case 0:
            page_cosmetics_print();
            break;
        case 1:
            page_objects_print();
            break;
        case 2:
            page_warps_print();
            break;
        case 3:
            page_presets_print();
            break;
        case 4:
            page_modes_print();
            break;
    }

    print_generic_string(25, 214, textNext);
    print_generic_string(237, 214, textPrev);

    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

LangArray textSoundSelect = DEFINE_LANGUAGE_ARRAY(
    "SOUND SELECT",
    "SELECTION SON",
    "WÄHLE SOUND",
    "サウンドセレクト",
    "MODO DE SONIDO");

#ifdef MULTILANG
LangArray textLanguageSelect = DEFINE_LANGUAGE_ARRAY(
    "LANGUAGE SELECT",
    "SELECTION LANGUE",
    "WÄHLE SPRACHE",
    "ランゲージセレクト",
    "IDIOMA");

LangArray textLanguage = DEFINE_LANGUAGE_ARRAY(
    "ENGLISH",
    "FRANÇAIS",
    "DEUTSCH",
    "にほんご",
    "ESPAÑOL");

#define SOUND_LABEL_Y 141
#define LANGUAGE_SELECT_Y 80
#else
#define SOUND_LABEL_Y 87
#endif

#ifdef ENABLE_STEREO_HEADSET_EFFECTS
#define OPTION_LABEL_SPACING 74
#else
#define OPTION_LABEL_SPACING 99
#endif

#define OPTION_LABEL_START_X (SCREEN_CENTER_X - ((ARRAY_COUNT(textSoundModes) - 1) * OPTION_LABEL_SPACING / 2))

/**
 * Prints sound mode menu strings that shows on the purple background menu screen.
 *
 * With multilang, this function acts like "print_option_mode_menu_strings" because of languages.
 */
void print_sound_mode_menu_strings(void) {
    s32 mode;
    s32 textX;

    // Print "SOUND SELECT" text
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);

    print_hud_lut_string(47, 32, LANG_ARRAY(textSoundSelect));
#ifdef MULTILANG
    print_hud_lut_string(47, 110, LANG_ARRAY(textLanguageSelect));
#endif

    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);

    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);

    // Print sound mode names
    for (mode = 0, textX = OPTION_LABEL_START_X; mode < ARRAY_COUNT(textSoundModes); textX += OPTION_LABEL_SPACING, mode++) {
        if (mode == sSoundMode) {
            set_text_color(255, 255, 255);
        } else {
            set_text_color(0, 0, 0);
        }
        print_generic_string_aligned(textX, SOUND_LABEL_Y, LANG_ARRAY(*textSoundModes[mode]), TEXT_ALIGN_CENTER);
    }

#ifdef MULTILANG
    // Handle changing the selected language
    if (sCursorClickingTimer == 2) {
        s16 cursorX = sCursorPos[0] + SCREEN_CENTER_X;
        s16 cursorY = sCursorPos[1] + SCREEN_CENTER_Y;

        s8 oldSelectedLanguageIndex = sSelectedLanguageIndex;

        if (cursorY < LANGUAGE_SELECT_Y + 20 && cursorY >= LANGUAGE_SELECT_Y) {
            if (cursorX < SCREEN_CENTER_X - 40 && cursorX >= SCREEN_CENTER_X - 60) {
                sSelectedLanguageIndex--;
            } else if (cursorX < SCREEN_CENTER_X + 60 && cursorX >= SCREEN_CENTER_X + 40) {
                sSelectedLanguageIndex++;
            }
            // Update language if the language has been changed
            if (sSelectedLanguageIndex != oldSelectedLanguageIndex) {
                play_sound(SOUND_MENU_CHANGE_SELECT, gGlobalSoundSource);
                sSelectedLanguageIndex = (sSelectedLanguageIndex + LANGUAGE_COUNT) % LANGUAGE_COUNT;
                multilang_set_language(gDefinedLanguages[sSelectedLanguageIndex]);
            }
        }
    }

    set_text_color(255, 255, 255);
    // Print current language
    print_generic_string_aligned(SCREEN_CENTER_X,      LANGUAGE_SELECT_Y, textLanguage[gInGameLanguage], TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SCREEN_CENTER_X - 50, LANGUAGE_SELECT_Y, "◀", TEXT_ALIGN_CENTER);
    print_generic_string_aligned(SCREEN_CENTER_X + 50, LANGUAGE_SELECT_Y, "▶", TEXT_ALIGN_CENTER);

    // Print return text
    print_generic_string(184, 29, LANG_ARRAY(textReturn));
#endif

    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

/**
 * Prints castle secret stars collected in a score menu save file.
 */
void print_score_file_castle_secret_stars(s8 fileIndex, s16 x, s16 y) {
    char secretStarsText[20];
    char secretStarsNum[8];
    // Print number of castle secret stars
    format_int_to_string(secretStarsNum, save_file_get_total_star_count(fileIndex,
                                                                  COURSE_NUM_TO_INDEX(COURSE_BONUS_STAGES),
                                                                  COURSE_NUM_TO_INDEX(COURSE_MAX)));
    sprintf(secretStarsText, "★×%s", secretStarsNum);
    print_menu_generic_string(x, y, secretStarsText);
}

LangArray text4Dashes = DEFINE_LANGUAGE_ARRAY(
    "----",
    "----",
    "----",
    "ーーーー",
    "----");

LangArray textMarioFace = DEFINE_LANGUAGE_ARRAY(
    "{}%s",
    "{}%s",
    "{}%s",
    "マリオ%s",
    "{}%s");

/**
 * Prints course coins collected in a score menu save file.
 */
void print_score_file_course_coin_score(s8 fileIndex, s16 courseIndex, s16 x, s16 y) {
    char str[20];
    char coinScoreText[10];
    u8 stars = save_file_get_star_flags(fileIndex, courseIndex);

    // MYSCORE
    if (sScoreFileCoinScoreMode == 0) {
        // Print coin score
        // format_int_to_string(coinScoreText, save_file_get_course_coin_score(fileIndex, courseIndex));
        // sprintf(str, "✪×%s", coinScoreText);
        // print_menu_generic_string(x + 25, y, str);
        // If collected, print 100 coin star
        if (stars & STAR_FLAG_ACT_100_COINS) {
            print_menu_generic_string(x + 70, y, "★");
        }
    }
    // HISCORE
    else {
        u16 coinScoreFile;
        // Print coin highscore
        format_int_to_string(coinScoreText, (u16) save_file_get_max_coin_score(courseIndex) & 0xFFFF);
        sprintf(str, "✪×%s", coinScoreText);
        print_menu_generic_string(x + 18, y, str);
        // Print coin highscore file
        coinScoreFile = (save_file_get_max_coin_score(courseIndex) >> 16) & 0xFFFF;
        if (coinScoreFile == 0) {
            print_menu_generic_string(x + 60, y, LANG_ARRAY(text4Dashes));
        } else {
            string_format_file_letter(str, LANG_ARRAY(textMarioFace), coinScoreFile - 1);
            print_menu_generic_string(x + 60, y, str);
        }
    }
}

/**
 * Prints stars collected in a score menu save file.
 */
void print_score_file_star_score(s8 fileIndex, s16 courseIndex, s16 x, s16 y) {
    s16 i = 0;
    char starScoreText[30];
    char *entries[6];
    u8 stars = save_file_get_star_flags(fileIndex, courseIndex);
    s8 starCount = save_file_get_course_star_count(fileIndex, courseIndex);
    // Don't count 100 coin star
    if (stars & STAR_FLAG_ACT_100_COINS) {
        starCount--;
    }
    // Add 1 star character for every star collected
    for (i = 0; i < starCount; i++) {
        entries[i] = "★";
    }
    for (i = starCount; i < 6; i++) {
        entries[i] = "";
    }
    sprintf(starScoreText, "%s%s%s%s%s%s", entries[0], entries[1], entries[2], entries[3], entries[4], entries[5]);
    print_menu_generic_string(x, y, starScoreText);
}

LangArray textScoreMenuMarioX = DEFINE_LANGUAGE_ARRAY(
    "MARIO %c",
    "MARIO %c",
    "MARIO %c",
    "マリオ %c",
    "MARIO %c");

LangArray textHiScore = DEFINE_LANGUAGE_ARRAY(
    "HI SCORE",
    "MEILLEUR SCORE",
    "BESTLEISTUNG",
    "ハイスコア",
    "RÉCORDS");

extern LangArray textMyScore;

/**
 * Prints save file score strings that shows when a save file is chosen inside the score menu.
 */
void print_save_file_scores(s8 fileIndex) {
    u32 i;
    char str[20];
    char fileLetter;

#ifndef MULTILANG
    const char **levelNameTable = segmented_to_virtual(seg2_course_name_table);
#else
    const char ***levelNameLanguageTable = segmented_to_virtual(course_strings_language_table);
    const char **levelNameTable = segmented_to_virtual(levelNameLanguageTable[gInGameLanguage]);
#endif

    // Print file name at top
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);
    fileLetter = 'A' + fileIndex;
    sprintf(str, LANG_ARRAY(textScoreMenuMarioX), fileLetter);
    print_hud_lut_string(25, 15, str);

    // Print save file star count at top
    print_save_file_star_count(fileIndex, 124, 15);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
    // Print course scores
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gDialogTextAlpha);

    for ((i = 0); (i < COURSE_STAGES_MAX); (i++)) {
        s32 lineY = 35 + (12 * i);
        format_int_to_string(str, i + 1);
        print_menu_generic_string(41, lineY, segmented_to_virtual(levelNameTable[i]));
        print_menu_generic_string_aligned(37, lineY, str, TEXT_ALIGN_RIGHT);
        print_score_file_star_score(       fileIndex, i, 171, lineY);
        print_score_file_course_coin_score(fileIndex, i, 213, lineY);
    }

    // Print castle secret stars text
    print_menu_generic_string(41, 215, segmented_to_virtual(levelNameTable[25]));
    // Print castle secret stars score
    print_score_file_castle_secret_stars(fileIndex, 171, 215);

    // Print current coin score mode
    if (sScoreFileCoinScoreMode == 0) {
        print_menu_generic_string_aligned(262, 24, LANG_ARRAY(textMyScore), TEXT_ALIGN_CENTER);
    } else {
        print_menu_generic_string_aligned(262, 24, LANG_ARRAY(textHiScore), TEXT_ALIGN_CENTER);
    }

    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
}

/**
 * Prints file select strings depending on the menu selected.
 * Also checks if all saves exists and defines text and main menu timers.
 */
void print_file_select_strings(void) {
    random_float();
    create_dl_ortho_matrix();

    if (sSelectingAdventure == 1 || sSelectingAdventure == 2)
        print_select_adventure();

    switch (sSelectedButtonID) {
        case MENU_BUTTON_NONE:         print_main_menu_strings();                               break;
        case MENU_BUTTON_SCORE:        draw_select_seed_menu();                                 break;
        case MENU_BUTTON_COPY:         print_copy_menu_strings();                               break;
        case MENU_BUTTON_ERASE:        print_erase_menu_strings();                              break;
        case MENU_BUTTON_SCORE_FILE_A: print_save_file_scores(SAVE_FILE_A); break;
        case MENU_BUTTON_SCORE_FILE_B: print_save_file_scores(SAVE_FILE_B); break;
        case MENU_BUTTON_SCORE_FILE_C: print_save_file_scores(SAVE_FILE_C); break;
        case MENU_BUTTON_SCORE_FILE_D: print_save_file_scores(SAVE_FILE_D); break;
        case MENU_BUTTON_SOUND_MODE:   print_sound_mode_menu_strings();     break;
        case MENU_BUTTON_SELECT_SEED_OPTIONS: draw_select_seed_menu_option(); break;
    }
    // If all 4 save file exists, define true to sAllFilesExist to prevent more copies in copy menu
    if (save_file_exists(SAVE_FILE_A) == TRUE && save_file_exists(SAVE_FILE_B) == TRUE &&
        save_file_exists(SAVE_FILE_C) == TRUE && save_file_exists(SAVE_FILE_D) == TRUE) {
        sAllFilesExist = TRUE;
    } else {
        sAllFilesExist = FALSE;
    }
    // Timers for menu alpha text and the main menu itself
    if (gDialogTextAlpha < 250) {
        gDialogTextAlpha += 10;
    }
    if (sMainMenuTimer < 1000) {
        sMainMenuTimer++;
    }
}

/**
 * Geo function that prints file select strings and the cursor.
 */
Gfx *geo_file_select_strings_and_menu_cursor(s32 callContext, UNUSED struct GraphNode *node, UNUSED Mat4 mtx) {
    if (callContext == GEO_CONTEXT_RENDER) {
        print_file_select_strings();
        print_menu_cursor();
    }
    return NULL;
}

/**
 * Initiates file select values after Mario Screen.
 * Relocates cursor position of the last save if the game goes back to the Mario Screen
 * either completing a course choosing "SAVE & QUIT" or having a game over.
 */
extern void set_vi_mode(int enabled);
s32 lvl_init_menu_values_and_cursor_pos(UNUSED s32 arg, UNUSED s32 unused) {
    set_vi_mode(6);
    sSelectedButtonID = MENU_BUTTON_NONE;
    sCurrentMenuLevel = MENU_LAYER_MAIN;
    gDialogTextAlpha = 0;
    // Place the cursor over the save file that was being played.
    // gCurrSaveFileNum is 1 by default when the game boots, as such
    // the cursor will point on Mario A save file.
    switch (gCurrSaveFileNum) {
        case SAVE_FILE_NUM_A: sCursorPos[0] = -94.0f; sCursorPos[1] = 46.0f; break;
        case SAVE_FILE_NUM_B: sCursorPos[0] =  24.0f; sCursorPos[1] = 46.0f; break;
        case SAVE_FILE_NUM_C: sCursorPos[0] = -94.0f; sCursorPos[1] =  5.0f; break;
        case SAVE_FILE_NUM_D: sCursorPos[0] =  24.0f; sCursorPos[1] =  5.0f; break;
    }
    sClickPos[0] = -10000;
    sClickPos[1] = -10000;
    sCursorClickingTimer = 0;
    sSelectedFileNum = 0;
    sSelectedFileIndex = MENU_BUTTON_NONE;
    sFadeOutText = FALSE;
    sStatusMessageID = 0;
    sTextFadeAlpha = 0;
    sMainMenuTimer = 0;
    sEraseYesNoHoverState = MENU_ERASE_HOVER_NONE;
    sSoundMode = save_file_get_sound_mode();
    Randomizer_gOverwriteFileOptions = FALSE;
    Randomizer_gOverwriteFileSeed = FALSE;
    // Randomizer_curPreset = 0;
    // applyPreset();
#ifdef MULTILANG
    sSelectedLanguageIndex = get_language_index(gInGameLanguage);

    for (u32 fileNum = 0; fileNum < NUM_SAVE_FILES; fileNum++) {
        if (save_file_exists(fileNum) == TRUE) {
            sOpenLangSettings = FALSE;
            break;
        } else {
            sOpenLangSettings = TRUE;
        }
    }
#endif
    gCurrLevelNum = LEVEL_UNKNOWN_1;
    return 0;
}

/**
 * Updates file select menu button objects so they can be interacted.
 * When a save file is selected, it returns fileNum value
 * defined in load_main_menu_save_file.
 */
s32 lvl_update_obj_and_load_file_selected(UNUSED s32 arg, UNUSED s32 unused) {
    area_update_objects();
    return sSelectingAdventure == 1 ? 0 : sSelectedFileNum;
}

STATIC_ASSERT(SOUND_MODE_COUNT == MENU_BUTTON_SOUND_OPTION_MAX - MENU_BUTTON_SOUND_OPTION_MIN, "Mismatch between number of sound modes in audio code and file select!");
