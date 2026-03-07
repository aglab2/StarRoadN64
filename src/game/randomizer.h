#ifndef _RANDOMIZER_H
#define _RANDOMIZER_H

#include "seq_ids.h"
#include "engine/math_util.h"

enum Randomizer_AvoidanceSafety {
    Randomizer_AVOIDANCE_SAFETY_ALL,
    Randomizer_AVOIDANCE_SAFETY_MED,
    Randomizer_AVOIDANCE_SAFETY_HARD
};

struct Randomizer_AvoidancePoint {
    const uintptr_t *behavior;
    Vec3s pos;
    s16 radius;
    s16 height;
    u32 safety;
};

typedef const struct Randomizer_AvoidancePoint Randomizer_AvoidancePointArray[];

struct Randomizer_AreaParams {
    s16 minX;
    s16 maxX;
    s16 minY;
    s16 maxY;
    s16 minZ;
    s16 maxZ;

    u8 stub;
    u32 numAvoidancePoints;
    const Randomizer_AvoidancePointArray *avoidancePoints;
};

typedef const struct Randomizer_AreaParams Randomizer_AreaParamsArray[];

enum Randomizer_FloorSafeLevels {
    Randomizer_FLOOR_SAFETY_LOW, // Most hovering objects
    Randomizer_FLOOR_SAFETY_MEDIUM, // ! boxes, nonstop stars - won't spawn over dangerous surfaces
    Randomizer_FLOOR_SAFETY_HIGH // Grounded objects and start warp - won't spawn over slippery or steep floors
};

enum Randomizer_Safety {
    Randomizer_SPAWN_SAFETY_SAFE,
    Randomizer_SPAWN_SAFETY_DEFAULT,
    Randomizer_SPAWN_SAFETY_HARD
};

enum Randomizer_StarDoorReqIDs {
    Randomizer_STAR_REQ_CH,
    Randomizer_STAR_REQ_GG,
    Randomizer_STAR_REQ_B1,
    // Randomizer_STAR_REQ_K1,
    Randomizer_STAR_REQ_BASE,
    Randomizer_STAR_REQ_KC,
    Randomizer_STAR_REQ_MMM,
    Randomizer_STAR_REQ_B2,
    // Randomizer_STAR_REQ_K2,
    Randomizer_STAR_REQ_OW3,
    Randomizer_STAR_REQ_B3,
    Randomizer_STAR_REQ_MAX
};

#define RANDO_DOOR_PARAM(id, req) (((id) << 12) | (((req) + 1) << 8) )

struct Randomizer_nodeInfo {
    u8 level;
    u8 area;
    u8 f0;
    u8 f1;
    u8 f1levelOverride;
};

struct Randomizer_OptionsSettings {
    union {
        struct {
            u32 padding:13; // to make sure settings ids are small

            u32 ironMode:1;
            u32 keepStructure:1;
            u32 randomLevelWarp:1;
            u32 adjustedExits:2;
            u32 objectRandomization:1;
            u32 nonstopMode:2;
            u32 safeSpawns:2;
            u32 randomizeStarSpawns:1;
            u32 randomStarDoorCounts:2;
            u32 randomLevelSpawn:1;
            u32 starDoorRequirement:4;
            u32 demonOn:1;
        } s;
        u32 w;
    } gameplay;
    union {
        struct {
            u32 marioColors:2;
            u32 musicOn:2;
            u32 skyboxOn:1;
            u32 coinsOn:1;
            u32 starColors:2;
        } s;
        u32 w;
    } cosmetic;
};

enum Randomizer_StarColors {
    Randomizer_STAR_COLOR_OFF,
    Randomizer_STAR_COLOR_PER_STAR,
    Randomizer_STAR_COLOR_PER_LEVEL,
    Randomizer_STAR_COLOR_GLOBAL
};

extern s32 Randomizer_curPreset;
extern struct Randomizer_OptionsSettings Randomizer_gOptionsSettings;
extern struct Randomizer_OptionsSettings Randomizer_gPresets[];

extern struct Randomizer_nodeInfo Randomizer_gLevelWarps[];
extern u8 Randomizer_gRequiredStars[];
extern u32 Randomizer_gGameSeed;
extern u8 Randomizer_gIsSetSeed;
extern u8 Randomizer_gIgnoreCollisionDistance;
extern u8 Randomizer_gWarpDestinations[];
extern Randomizer_AreaParamsArray *Randomizer_sLevelParams[];
extern u8 Randomizer_gRandomSongs[];
extern const u8 Randomizer_gRandomSongsCount;
extern u8 Randomizer_gSkyboxIndex;
extern const u8 Randomizer_gStarDoorReqLUT[];
extern struct Randomizer_AvoidancePoint Randomizer_gDynamicAvoidancePoints[200];
extern u8 Randomizer_gNumDynamicAvoidancePoints;

extern void Randomizer_refreshPreset();
extern void Randomizer_create_dynamic_avoidance_point(Vec3f pos, f32 radius, f32 height, f32 downOffset);
extern u32 Randomizer_raycast_wall_check(Vec3s pos);
extern void Randomizer_get_safe_position(struct Object *, Vec3s, f32, f32, tinymt32_t *, u8, u32);
extern f32 Randomizer_get_val_in_range_uniform(f32, f32, tinymt32_t *);
extern void Randomizer_init_randomizer(s32 fileNum);
extern u8 Randomizer_get_nonrandom_level(u8 level);
extern u8 Randomizer_expected_mini_level_target(u8 currLevel);
extern void Randomizer_print_seed_and_options_data(void);
extern void Randomizer_set_mario_rando_colors(void);
extern void Randomizer_init_star_color(struct Object *star, s32 courseID, s32 starID);
extern void Randomizer_precalc_probability_tables();

#endif
