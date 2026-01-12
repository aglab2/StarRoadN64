#include "game/area.h"
#include "game/level_update.h"
#include "game/save_file.h"
#include "game/object_list_processor.h"

extern void seq_player_play_sequence(u8 player, u8 seqId, u16 arg2);

/*
ADDIU SP, SP, $FFE0
SW RA, $0014 (SP)
LUI A0, $8034
LH A0, $B21A (A0)  -- gMarioStates+0xAA gMarioStates.numStars
ADDIU A1, R0, $0082=130
BNE A1, A0, $000C6FF8
ADDIU A0, R0, $0000
ADDIU A1, R0, $002F
JAL $00320544 -- seq_player_play_sequence
ADDIU A2, R0, $0000
LW RA, $0014 (SP)
ADDIU SP, SP, $0020
JR RA
NOP
*/
void bhv_star_road_ow_music()
{
    if (gMarioStates->numStars == 130) {
        seq_player_play_sequence(0, 0x2F, 0);
    }
}

/*
ADDIU SP, SP, $FFE8
SW RA, $0014 (SP)
LUI A0, $8033
LH A0, $DDF4 (A0) ; gCurrSaveFileNum
OR A1, R0, R0
ADDIU A2, R0, $0018
JAL $0027A010 ; save_file_get_total_star_count
ADDIU A0, A0, $FFFF
SLTI AT, V0, $0001
BNEZ AT, $000CC328
NOP
LUI T6, $8036
LW T6, $1160 (T6) ; gCurrentObject
SH R0, $0074 (T6) ; ->activeFlags
cc328: BEQ R0, R0, $000CC330
NOP
cc330: LW RA, $0014 (SP)
ADDIU SP, SP, $0018
JR RA
NOP
*/

void bhv_star_road_yoshi_init()
{
    int totalStars;
    totalStars = save_file_get_total_star_count(gCurrSaveFileNum - 1, 0, 0x18);
    if (totalStars >= 1) {
        gCurrentObject->activeFlags = 0;
    }

    return;
}

Gfx *geo_star_road_p(s32 callContext, struct GraphNode *node, UNUSED Mat4 mtx)
{
    if (callContext == GEO_CONTEXT_RENDER) {
        struct GraphNodeGenerated *genNode = (struct GraphNodeGenerated *) node;
        struct GraphNodeDisplayList *graphNode = (struct GraphNodeDisplayList *) node->next;

        int active = gMarioStates->pos[1] < 0;
        if (active) {
            graphNode->node.flags |= GRAPH_RENDER_ACTIVE;
        } else {
            graphNode->node.flags &= ~GRAPH_RENDER_ACTIVE;
        }
    }

    return NULL;
}

Gfx *geo_star_road_n(s32 callContext, struct GraphNode *node, UNUSED Mat4 mtx)
{
    if (callContext == GEO_CONTEXT_RENDER) {
        struct GraphNodeGenerated *genNode = (struct GraphNodeGenerated *) node;
        struct GraphNodeDisplayList *graphNode = (struct GraphNodeDisplayList *) node->next;

        int active = gMarioStates->pos[1] > 0;
        if (active) {
            graphNode->node.flags |= GRAPH_RENDER_ACTIVE;
        } else {
            graphNode->node.flags &= ~GRAPH_RENDER_ACTIVE;
        }
    }

    return NULL;
}
