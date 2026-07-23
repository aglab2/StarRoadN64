// door.inc.c
#include "game/randomizer.h"
#include "libc/string.h"

struct DoorAction {
    u32 flag;
    ObjAction32 action;
};

static struct DoorAction sDoorActions[] = {
    { INT_STATUS_WARP_DOOR_PULLED, DOOR_ACT_WARP_PULLED },
    { INT_STATUS_WARP_DOOR_PUSHED, DOOR_ACT_WARP_PUSHED },
    { INT_STATUS_DOOR_PULLED,      DOOR_ACT_PULLED      },
    { INT_STATUS_DOOR_PUSHED,      DOOR_ACT_PUSHED      },
    { -1,                          DOOR_ACT_CLOSED      },
};

static s32 sDoorOpenSounds[] = { SOUND_GENERAL_OPEN_WOOD_DOOR, SOUND_GENERAL_OPEN_IRON_DOOR };

static s32 sDoorCloseSounds[] = { SOUND_GENERAL_CLOSE_WOOD_DOOR, SOUND_GENERAL_CLOSE_IRON_DOOR };

void door_animation_and_reset(s32 animIndex) {
    cur_obj_init_animation_with_sound(animIndex);
    if (cur_obj_check_if_near_animation_end()) {
        o->oAction = DOOR_ACT_CLOSED;
    }
}

void set_door_camera_event(void) {
    if (segmented_to_virtual(bhvDoor) == o->behavior) {
        gPlayerCameraState->cameraEvent = CAM_EVENT_DOOR;
    } else {
        gPlayerCameraState->cameraEvent = CAM_EVENT_DOOR_WARP;
    }
    gPlayerCameraState->usedObj = o;
}

void play_door_open_noise(void) {
    s32 isMetalDoor = cur_obj_has_model(MODEL_HMC_METAL_DOOR);
    if (o->oTimer == 0) {
        cur_obj_play_sound_2(sDoorOpenSounds[isMetalDoor]);
        gTimeStopState |= TIME_STOP_MARIO_OPENED_DOOR;
    }
    if (o->oTimer == 70) {
        cur_obj_play_sound_2(sDoorCloseSounds[isMetalDoor]);
    }
}

void play_warp_door_open_noise(void) {
    s32 isMetalDoor = cur_obj_has_model(MODEL_HMC_METAL_DOOR);
    if (o->oTimer == 30) {
        cur_obj_play_sound_2(sDoorCloseSounds[isMetalDoor]);
    }
}

void bhv_door_loop(void) {
    s32 index = 0;

    while (sDoorActions[index].flag != 0xFFFFFFFF) {
        if (cur_obj_clear_interact_status_flag(sDoorActions[index].flag)) {
            set_door_camera_event();
            cur_obj_change_action(sDoorActions[index].action);
        }
        index++;
    }

    switch (o->oAction) {
        case DOOR_ACT_CLOSED:
            cur_obj_init_animation_with_sound(DOOR_ANIM_CLOSED);
            load_object_collision_model();
            break;
        case DOOR_ACT_PULLED:
            door_animation_and_reset(DOOR_ANIM_PULLED);
            play_door_open_noise();
            break;
        case DOOR_ACT_PUSHED:
            door_animation_and_reset(DOOR_ANIM_PUSHED);
            play_door_open_noise();
            break;
        case DOOR_ACT_WARP_PULLED:
            door_animation_and_reset(DOOR_ANIM_WARP_PULLED);
            play_warp_door_open_noise();
            break;
        case DOOR_ACT_WARP_PUSHED:
            door_animation_and_reset(DOOR_ANIM_WARP_PUSHED);
            play_warp_door_open_noise();
            break;
    }

    bhv_door_rendering_loop();
}

#define door_seg3_texture_BASE door_seg3_texture_zero_star_door_sign_orig
extern Texture door_seg3_texture_zero_star_door_sign[];
extern Texture door_seg3_texture_one_star_door_sign[];
extern Texture door_seg3_texture_three_star_door_sign[];
extern Texture door_seg3_texture_zero_star_door_sign_orig[];
extern Texture door_seg3_texture_one_star_door_sign_orig[];
extern Texture door_seg3_texture_three_star_door_sign_orig[];

extern const Texture texture_hud_char_s[];

extern u8 castle_grounds_1__texture_0E02D810[];
extern u8 castle_inside_1__texture_0E006810[];
extern u8 castle_courtyard_1__texture_0E027410[];

static void blit(u16* dst, u8 dstX, u8 dstY, const u16* digit)
{
    for (int y = 0; y < 16; y++) {
        u16*       dstRow = dst   + (dstY + y) * 32 + dstX;
        const u16* srcRow = digit         + y  * 16;

        for (int x = 0; x < 16; x++) {
            u16*       pdstPixel = dstRow + x;
            const u16* psrcPixel = srcRow + x;

            u16 srcPixel = *psrcPixel;
            if (!(srcPixel & 1))
                continue; // transparent pixel, skip

            *pdstPixel = srcPixel;
        }
    }
}

static void blitStarCount(Texture* texture, s32 requiredStars) {
    u16* dst = segmented_to_virtual(texture);
    const u16* src = segmented_to_virtual(door_seg3_texture_BASE);
    const u16* digitTex = segmented_to_virtual(texture_hud_char_s);

    memcpy(dst, src, 32 * 32 * G_IM_SIZ_16b_BYTES);

    static const u8 sKernings[] = { 0 /*-1*/, 10, 9, 9, 10, 10, 10, 10, 10, 10, 10 };

    s8 digits[3];
    digits[0] = requiredStars % 10;
    digits[1] = requiredStars >= 10 ? (requiredStars / 10) % 10 : -1;
    digits[2] = requiredStars >= 100 ? (requiredStars / 100) % 10 : -1;

    u8 len = 0;
    for (int i = 0; i < 3; i++)
        len += sKernings[digits[i] + 1];

    // we need to start blitting texture from 9 pixels from top in the middle
    // each digit is 16x16 blitted on 32x32 texture, so we need to start from 16 - len/2 from left
    len /= 2;
    for (int i = 2; i >= 0; i--) {
        if (digits[i] < 0)
            continue;

        blit(dst, 16 - len, 9, digitTex + 16*16*digits[i]);
        len -= sKernings[digits[i] + 1];
    }
}

static void* get_baked_texture(void) {
    void* bakedTexture = NULL;
    if (gCurrLevelNum == LEVEL_CASTLE_GROUNDS)
        bakedTexture = castle_grounds_1__texture_0E02D810;
    if (gCurrLevelNum == LEVEL_CASTLE)
        bakedTexture = castle_inside_1__texture_0E006810;
    if (gCurrLevelNum == LEVEL_CASTLE_COURTYARD)
        bakedTexture = castle_courtyard_1__texture_0E027410;

    return bakedTexture ? segmented_to_virtual(bakedTexture) : NULL;
}

void bhv_door_init(void) {
    bhv_warp_init();
    const f32 checkDist = 200.0f;

    f32 x = o->oPosX;
    f32 y = o->oPosY;
    f32 z = o->oPosZ;

    o->oDoorSelfRoom = get_room_at_pos(x, y, z);

    x = o->oPosX + (sins(o->oMoveAngleYaw) *  checkDist);
    z = o->oPosZ + (coss(o->oMoveAngleYaw) *  checkDist);

    o->oDoorForwardRoom = get_room_at_pos(x, y, z);

    x = o->oPosX + (sins(o->oMoveAngleYaw) * -checkDist);
    z = o->oPosZ + (coss(o->oMoveAngleYaw) * -checkDist);

    o->oDoorBackwardRoom = get_room_at_pos(x, y, z);

    if (
        // Ensure the room number is in bounds.
        o->oDoorSelfRoom > 0 && o->oDoorSelfRoom < ARRAY_COUNT(gDoorAdjacentRooms)
        // Only set gDoorAdjacentRooms for transition rooms.
        && o->oDoorSelfRoom    != o->oDoorForwardRoom
        && o->oDoorSelfRoom    != o->oDoorBackwardRoom
        && o->oDoorForwardRoom != o->oDoorBackwardRoom
    ) {
        gDoorAdjacentRooms[o->oDoorSelfRoom].forwardRoom  = o->oDoorForwardRoom;
        gDoorAdjacentRooms[o->oDoorSelfRoom].backwardRoom = o->oDoorBackwardRoom;
    }
    
    int bparam1 = ((o->oBehParams >> 24) & 0xFF);
    int bparam3 = ((o->oBehParams >> 8) & 0xFF);
    int requiredStars = 0;
    if ((bparam3 != 0) && (bparam1 < 0xFE)) {
        int starsIdx = (bparam3 & 0xf) - 1;
        requiredStars = Randomizer_gRequiredStars[starsIdx];
        o->oBehParams = (requiredStars << 24) + (o->oBehParams & 0x00FFFFFF);
    }

    // Check model id
    if ((!Randomizer_gOptionsSettings.gameplay.s.keepStructure) && o->behavior == bhvDoorWarp && o->header.gfx.sharedChild) {
        o->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_CASTLE_CASTLE_DOOR];
    }

    if (bparam3)
    {
        int idx = bparam3 >> 4;
        static Texture* sDoorTextures[] = {
            door_seg3_texture_one_star_door_sign,
            door_seg3_texture_three_star_door_sign,
            door_seg3_texture_zero_star_door_sign,
        };

        static Texture* sDoorTexturesOrigs[] = {
            door_seg3_texture_one_star_door_sign_orig,
            door_seg3_texture_three_star_door_sign_orig,
            door_seg3_texture_zero_star_door_sign_orig,
        };

        if (Randomizer_gOptionsSettings.gameplay.s.randomStarDoorCounts)
        {
            if (idx)
            {
                // not bowser course - pick a door sign with the correct star count and blit it to the door texture
                idx--;
                static const u8 sDoorIds[] = {
                    MODEL_CASTLE_DOOR_1_STAR,
                    MODEL_CASTLE_DOOR_3_STARS,
                    MODEL_CASTLE_DOOR_0_STARS,
                };

                blitStarCount(sDoorTextures[idx], requiredStars);
                o->header.gfx.sharedChild = gLoadedGraphNodes[sDoorIds[idx]];
            }
            else
            {
                // bowser course has star counts baked directly in the model
                blitStarCount((void*) 0x80725F80, requiredStars);

                void* bakedTexture = get_baked_texture();
                // baked texture is annoying because it is flipped vertically...
                if (bakedTexture)
                {
                    for (int x = 0; x < 32; x++) {
                        for (int y = 0; y < 32; y++) {
                            const u16* srcPixel = ((u16*)0x80725F80) + (y * 32) + x;
                            u16*       dstPixel = ((u16*)bakedTexture) + ((31 - y) * 32) + x;

                            *dstPixel = *srcPixel;
                        }
                    }
                }
            }
        }
        else
        {
            if (idx)
            {
                idx--;
                memcpy(segmented_to_virtual(sDoorTextures[idx])
                    , segmented_to_virtual(sDoorTexturesOrigs[idx])
                    , 32 * 32 * G_IM_SIZ_16b_BYTES);
            }
            // conveniently in alternative case no patching is needed because models are reloaded from cartridge
        }
    }
}

void bhv_door_rendering_loop(void) {
    struct TransitionRoomData* transitionRoom = &gDoorAdjacentRooms[gMarioCurrentRoom];

    o->oDoorIsRendering = (
        gMarioCurrentRoom            == 0                    || // Mario is in the "global" room.
        gMarioCurrentRoom            == o->oDoorSelfRoom     || // Mario is in the same room as the door.
        gMarioCurrentRoom            == o->oDoorForwardRoom  || // Mario is in the door's  forward room.
        gMarioCurrentRoom            == o->oDoorBackwardRoom || // Mario is in the door's backward room.
        transitionRoom->forwardRoom  == o->oDoorForwardRoom  || // The transition room's  forward room is in the same room as this door's  forward room.
        transitionRoom->forwardRoom  == o->oDoorBackwardRoom || // The transition room's  forward room is in the same room as this door's backward room.
        transitionRoom->backwardRoom == o->oDoorForwardRoom  || // The transition room's backward room is in the same room as this door's  forward room.
        transitionRoom->backwardRoom == o->oDoorBackwardRoom    // The transition room's backward room is in the same room as this door's backward room.
    );

    COND_BIT(o->oDoorIsRendering, o->header.gfx.node.flags, GRAPH_RENDER_ACTIVE);
}
