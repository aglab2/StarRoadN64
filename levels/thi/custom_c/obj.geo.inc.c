extern Gfx DL_geo_bbh_0005F8_0x701fa50[];
const GeoLayout bhv_star_road_fff_inv_pyramid[]= {
    GEO_CULLING_RADIUS(300),
    GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_OPAQUE,DL_geo_bbh_0005F8_0x701fa50),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

extern Gfx DL_geo_bbh_0005C8_0x701f570[];
const GeoLayout bhv_star_road_fff_wall_platform[]= {
    GEO_CULLING_RADIUS(600),
    GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_OPAQUE,DL_geo_bbh_0005C8_0x701f570),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

extern Gfx DL_geo_bbh_0005B0_0x701f1e8[];
const GeoLayout bhv_star_road_fff_lava[]= {
    GEO_CULLING_RADIUS(700),
    GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_OPAQUE,DL_geo_bbh_0005B0_0x701f1e8),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

extern Gfx DL_geo_bbh_000628_0x701ff98[];
const GeoLayout bhv_star_road_fff_pillar[]= {
    GEO_CULLING_RADIUS(600),
    GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_ALPHA,DL_geo_bbh_000628_0x701ff98),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

extern Gfx DL_geo_bbh_000640_0x600d668[];
const GeoLayout bhv_star_road_fff_sink[]= {
    GEO_CULLING_RADIUS(2300),
    GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(1,DL_geo_bbh_000640_0x600d668),
    GEO_CLOSE_NODE(),
    GEO_END(),
};
