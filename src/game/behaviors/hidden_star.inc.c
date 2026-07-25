// hidden_star.inc.c

void bhv_hidden_star_init(void) {
    s16 remainingTriggers = count_objects_with_behavior(bhvHiddenStarTrigger);
    if (remainingTriggers == 0) {
        struct Object *starObj = spawn_object_abs_with_rot(o, 0, MODEL_STAR, bhvStar, o->oPosX, o->oPosY, o->oPosZ, 0, 0, 0);
        starObj->oBehParams = o->oBehParams;
        o->activeFlags = ACTIVE_FLAG_DEACTIVATED;
    }

    o->oHiddenStarTriggerCounter = 5 - remainingTriggers;
}

void bhv_hidden_star_loop(void) {
    switch (o->oAction) {
        case 0:
            if (o->oHiddenStarTriggerCounter == 5) {
                o->oAction = 1;
            }
            break;

        case 1:
            if (o->oTimer > 2) {
                spawn_red_coin_cutscene_star(o->oPosX, o->oPosY, o->oPosZ);
                spawn_mist_particles();
                o->activeFlags = ACTIVE_FLAG_DEACTIVATED;
            }
            break;
    }
}

void bhv_sparkle_circle(void)
{
    if (gGlobalTimer % 5 == 0) {
        int count = 3;
        for (int i = 0; i < count; i++)
        {
            struct Object *sparkle = spawn_object(o, MODEL_SPARKLES, bhvCoinSparkles);
            int angle = gGlobalTimer * 630 + i * (0x10000 / count);
            sparkle->oPosX += 100 * sins(angle);
            sparkle->oPosY -= 35;
            sparkle->oPosZ += 100 * coss(angle);
        }
    }
}

extern const BehaviorScript bhvCheckMark[];
void bhv_hidden_star_trigger_loop(void) {
    if (gGlobalTimer % 5 == 0) {
        struct Object *sparkle = spawn_object(o, (gCurrCourseNum == COURSE_LLL || gCurrCourseNum == COURSE_BOB) ? MODEL_SPARKLES_ALT : MODEL_SPARKLES, bhvCoinSparkles);
        sparkle->oPosX += random_float() * 100 - 50;
        sparkle->oPosY += random_float() * 100 - 50;
        sparkle->oPosZ += random_float() * 100 - 50;
    }

    if (obj_check_if_collided_with_object(o, gMarioObject)) {
        struct Object *hiddenStar = cur_obj_nearest_object_with_behavior(bhvHiddenStar);

        if (hiddenStar != NULL) {
            hiddenStar->oHiddenStarTriggerCounter++;

            if (hiddenStar->oHiddenStarTriggerCounter != 5) {
                spawn_orange_number(hiddenStar->oHiddenStarTriggerCounter, 0, 0, 0);
            }

            play_sound(SOUND_MENU_COLLECT_SECRET + (((u8) hiddenStar->oHiddenStarTriggerCounter - 1) << 16), gGlobalSoundSource);
        }

        o->activeFlags = ACTIVE_FLAG_DEACTIVATED;
        if (configFasterObjects)
        {
            if (0)
            {
                struct Object* checkmark = spawn_object(o, MODEL_CHECKMARK, bhvCheckMark);
                checkmark->oPosY += 150.f;
            }
        }
    }
}

void bhv_bowser_course_red_coin_star_init(void) {
    if (o->oBehParams2ndByte != 0) {
        o->oHiddenStarTriggerTotal = o->oBehParams2ndByte;
        o->oHiddenStarTriggerCounter = gRedCoinsCollected;
    }
    else {
        s16 numRedCoinsRemaining = count_objects_with_behavior(bhvRedCoin);
        o->oHiddenStarTriggerTotal = numRedCoinsRemaining + gRedCoinsCollected;
        o->oHiddenStarTriggerCounter = o->oHiddenStarTriggerTotal - numRedCoinsRemaining;
    }
}

void bhv_bowser_course_red_coin_star_loop(void) {
    gRedCoinsCollected = o->oHiddenStarTriggerCounter;

    switch (o->oAction) {
        case 0:
            if (o->oHiddenStarTriggerCounter == o->oHiddenStarTriggerTotal) {
                o->oAction = 1;
            }
            break;

        case 1:
            if (o->oTimer > 2) {
                spawn_no_exit_star(o->oPosX, o->oPosY, o->oPosZ);
                spawn_mist_particles();
                o->activeFlags = ACTIVE_FLAG_DEACTIVATED;
            }
            break;
    }
}
