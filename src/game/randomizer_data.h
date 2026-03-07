#pragma once

#include "randomizer_data.h"
#include "randomizer.h"

u8 Randomizer_gRequiredStars[Randomizer_STAR_REQ_MAX];

static const u8 sWarpPool0[] = { 
    LEVEL_BBH,
    LEVEL_CCM,
    LEVEL_HMC,
    LEVEL_SSL,
    LEVEL_BOB,
    LEVEL_SL,
    LEVEL_WDW,
    LEVEL_JRB,
    LEVEL_THI,
    LEVEL_TTC,
    LEVEL_RR,
    LEVEL_SA,
    LEVEL_LLL,
    LEVEL_DDD,
    LEVEL_WF,
    LEVEL_PSS,
    // LEVEL_WMOTR,
    LEVEL_TTM,
    LEVEL_BITDW,
    LEVEL_BITFS,
};

static const u8 sWarpPool1[] = {
    LEVEL_COTMC,
    LEVEL_TOTWC,
    LEVEL_VCUTM,
    LEVEL_BOWSER_1,
    LEVEL_BOWSER_2,
};

static const u8 sWarpsPreB1[] = {
    LEVEL_BBH,
    LEVEL_CCM,
    LEVEL_BOB,
    LEVEL_JRB,
    LEVEL_WF,
    LEVEL_PSS,
};

static const u8 sWarpsPreB2[] = {
    LEVEL_HMC,
    LEVEL_SSL,
    LEVEL_SL,
    LEVEL_SA,
    LEVEL_LLL,
    LEVEL_DDD,
};

#if 0
static const u8 sWarpsPreB3[] = {
    LEVEL_WDW,
    LEVEL_THI,
    LEVEL_TTC,
    LEVEL_RR,
    LEVEL_TTM,
};
#endif

static const u8 gWarpDestinationsStatic[] = {
    [LEVEL_BBH] = LEVEL_BBH,
    [LEVEL_CCM] = LEVEL_CCM,
    [LEVEL_HMC] = LEVEL_HMC,
    [LEVEL_SSL] = LEVEL_SSL,
    [LEVEL_BOB] = LEVEL_BOB,
    [LEVEL_SL] = LEVEL_SL,
    [LEVEL_WDW] = LEVEL_WDW,
    [LEVEL_JRB] = LEVEL_JRB,
    [LEVEL_THI] = LEVEL_THI,
    [LEVEL_TTC] = LEVEL_TTC,
    [LEVEL_RR] = LEVEL_RR,
    [LEVEL_BITDW] = LEVEL_BITDW,
    [LEVEL_VCUTM] = LEVEL_VCUTM,
    [LEVEL_BITFS] = LEVEL_BITFS,
    [LEVEL_BOWSER_1] = LEVEL_BOWSER_1,
    [LEVEL_BOWSER_2] = LEVEL_BOWSER_2,
    [LEVEL_SA] = LEVEL_SA,
    [LEVEL_LLL] = LEVEL_LLL,
    [LEVEL_DDD] = LEVEL_DDD,
    [LEVEL_WF] = LEVEL_WF,
    [LEVEL_PSS] = LEVEL_PSS,
    [LEVEL_COTMC] = LEVEL_COTMC,
    [LEVEL_TOTWC] = LEVEL_TOTWC,
    // [LEVEL_WMOTR] = LEVEL_WMOTR,
    [LEVEL_TTM] = LEVEL_TTM,
};

// translates dest level to randomized dest level.
u8 Randomizer_gWarpDestinations[sizeof(gWarpDestinationsStatic)];

static const u8 sDefaultStarReqs[] = {
    [ Randomizer_STAR_REQ_CH ] = 8,
    [ Randomizer_STAR_REQ_GG ] = 8,
    [ Randomizer_STAR_REQ_B1 ] = 20,
    // [ Randomizer_STAR_REQ_K1 ] = 0,
    [ Randomizer_STAR_REQ_BASE ] = 0,
    [ Randomizer_STAR_REQ_KC ] = 30,
    [ Randomizer_STAR_REQ_MMM ] = 30,
    [ Randomizer_STAR_REQ_B2 ] = 40,
    // [ Randomizer_STAR_REQ_K2 ] = 0,
    [ Randomizer_STAR_REQ_OW3 ] = 65,
    [ Randomizer_STAR_REQ_B3 ] = 80,
}; 

const u8 Randomizer_gStarDoorReqLUT[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 118};

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

// Use bhvStub for avoidance points for all objects.
// Use bhv<OBJECT> for points that only those OBJECTs should avoid.
static const struct Randomizer_AvoidancePoint mmmAvoidancePoints[] = {
    {bhvStub, {573, -2493, -1339}, 1500.0f, 3500.0f, Randomizer_AVOIDANCE_SAFETY_ALL} // Arph bottom
};

static const struct Randomizer_AvoidancePoint micrAvoidancePoints[] = {
    {bhvStub, {-898, -2910, -4833}, 2400.0f, 2900.0f, Randomizer_AVOIDANCE_SAFETY_ALL} // Snow pile in bottom
};

static const struct Randomizer_AvoidancePoint ccAvoidancePoints[] = {
    {bhvStub, {923, -1785, 1529}, 300.0f, 200.0f, Randomizer_AVOIDANCE_SAFETY_ALL} // Snow pile in bottom
};

static const struct Randomizer_AvoidancePoint ccandyAvoidancePoints[] = {
    {bhvStub, {580, 1803, 5703}, 1500.0f, 1700.0f, Randomizer_AVOIDANCE_SAFETY_HARD}, // target
    {bhvStub, {-4527, -3297, 1661}, 1100.0f, 500.0f, Randomizer_AVOIDANCE_SAFETY_HARD}, // top of the coin
    {bhvStub, {-5134, 890, -1687}, 1000.0f, 1000.0f, Randomizer_AVOIDANCE_SAFETY_HARD}, // on roof for coin
    {bhvStub, {-5134, 890, -1687}, 1000.0f, 1000.0f, Randomizer_AVOIDANCE_SAFETY_HARD}, // on roof for coin
};

static const struct Randomizer_AvoidancePoint b1fAvoidancePoints[] = {
    {bhvStub, {3258, 465, -2816}, 700.0f, 700.0f, Randomizer_AVOIDANCE_SAFETY_ALL} // top of the roof
};


// {Xmin, Xmax, Ymin, Ymax, Zmin, Zmax, 0, 0, NULL}

// Main courses
static const struct Randomizer_AreaParams bobParams[] = {
    {-7356, 7474, -3498, 2350, -8000, 6700, 0, 0, NULL}
};
static const struct Randomizer_AreaParams wfParams[] = {
    {-6000, 8191, -2166, 3564, -7200, 7000, 0, 0, NULL}
};
static const struct Randomizer_AreaParams jrbParams[] = {
    {-8192, 8191, -2546, 3900, -7600, 8191, 0, 0, NULL}
};
static const struct Randomizer_AreaParams ccmParams[] = {
    {-7146, 7194, -5650, 5090, -6624, 7637, 0, 0, NULL}
};
static const struct Randomizer_AreaParams bbhParams[] = {
    {-8192, 7266, -1977, 3000, -7124, 6318, 0, 0, NULL}
};
static const struct Randomizer_AreaParams hmcParams[] = {
    {-7133, 8191, -7363, 3929, -8192, 8191, 0, 0, NULL}
};
static const struct Randomizer_AreaParams lllParams[] = {
    {-7603, 6600, -3979, 5183, -7900, 6342, 0, 0, NULL}
};
static const struct Randomizer_AreaParams sslParams[] = {
    {-5462, 6806, -5250, 3960, -6956, 5105, 0, 0, NULL}
};
static const struct Randomizer_AreaParams dddParams[] = {
    {-6236, 7014, -2460, 2640, -7920, 5893, 0, ARRAY_SIZE(mmmAvoidancePoints), &mmmAvoidancePoints}
};
static const struct Randomizer_AreaParams slParams[] = {
    {-7284, 6957, -870, 4350, -7280, 5781, 0, ARRAY_SIZE(micrAvoidancePoints), &micrAvoidancePoints}
};
static const struct Randomizer_AreaParams wdwParams[] = {
    {-5515, 7069, -4521, 4524, -6966, 6610, 0, ARRAY_SIZE(ccandyAvoidancePoints), &ccandyAvoidancePoints}
};
static const struct Randomizer_AreaParams ttmParams[] = {
    {-7319, 7456, -1230, 5064, -8182, 7468, 0, 0, NULL}
};
static const struct Randomizer_AreaParams thiParams[] = {
    {-7839, 7380, -3530, 3104, -6877, 8013, 0, 0, NULL}
};
static const struct Randomizer_AreaParams ttcParams[] = {
    {-5834, 6167, -1524, 3785, -7578, 5612, 0, 0, NULL}
};
static const struct Randomizer_AreaParams rrParams[] = {
    {-7883, 7637, -6624, 11416, -7451, 7450, 0, 0, NULL}
};

// Secret courses
static const struct Randomizer_AreaParams pssParams[] = {
    {-6191, 6539, -2302, 4725, -6828, 6896, 0, 0, NULL}
};
static const struct Randomizer_AreaParams saParams[] = {
    {-6262, 6626, -6010, 4106, -6933, 5629, 0, 0, NULL}
};
static const struct Randomizer_AreaParams wmotrParams[] = {
    {-6694, 5039, -1385, 6180, -4827, 6362, 0, 0, NULL}
};
static const struct Randomizer_AreaParams totwcParams[] = {
    {-6070, 5392, -2348, 7470, -5409, 6683, 0, 0, NULL}
};
static const struct Randomizer_AreaParams cotmcParams[] = {
    {-6762, 5286, 4400, 10920, -3896, 5550, 0, 0, NULL}
};
static const struct Randomizer_AreaParams vcutmParams[] = {
    {-6065, 7353, -789, 1821, -3404, 2498, 0, 0, NULL}
};
static const struct Randomizer_AreaParams bitdwParams[] = {
    {-6693, 6223, -940, 2570, -7165, 6440, 0, 0, NULL}
};
static const struct Randomizer_AreaParams bitfsParams[] = {
    {-6378, 4236, -815, 1200, -5222, 5037, 0, 0, NULL}
};
static const struct Randomizer_AreaParams bitsParams[] = {
    {-6007, 7033, -2679, 4463, -6114, 7823, 0, 0, NULL}
};

// Castle areas
static const struct Randomizer_AreaParams cgParams[] = {
    {-7956, 8191, -1798, 6841, -7460, 7913, 0, 0, NULL}
};
static const struct Randomizer_AreaParams ccParams[] = {
    {-3878, 8191, -3986, 4380, -3423, 7664, 0, ARRAY_SIZE(ccAvoidancePoints), &ccAvoidancePoints}
};
static const struct Randomizer_AreaParams icParams[] = {
    {-6292, 4951, -436, 1800, -7893, 8191, 0, 0, NULL}
};

static const struct Randomizer_AreaParams b1fParams[] = {
    {-5149, 5180, -1006, 434, -5619, 5745, 0, ARRAY_SIZE(b1fAvoidancePoints), &b1fAvoidancePoints}
};
static const struct Randomizer_AreaParams b2fParams[] = {
    {-5149, 5180, -1006, 434, -5619, 5745, 0, 0, NULL}
};

static const struct Randomizer_AreaParams endParams[] = {
    {-8192,-2044, -1166, 2429, -6084, 7820, 0, 0, NULL}
};

const Randomizer_AreaParamsArray *Randomizer_sLevelParams[] = {
    &bbhParams,
    &ccmParams,
    &icParams,
    &hmcParams,
    &sslParams,
    &bobParams,
    &slParams,
    &wdwParams,
    &jrbParams,
    &thiParams,
    &ttcParams,
    &rrParams,
    &cgParams,
    &bitdwParams,
    &vcutmParams,
    &bitfsParams,
    &saParams,
    &bitsParams,
    &lllParams,
    &dddParams,
    &wfParams,
    &endParams,
    &ccParams,
    &pssParams,
    &cotmcParams,
    &totwcParams,
    &b1fParams,
    &wmotrParams,
    NULL,
    &b2fParams, //b2
    NULL,
    NULL,
    &ttmParams
};
