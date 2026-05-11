#include "plugin.h"
#include "CFileLoader.h"
#include "CSprite2d.h"
#include "CTimer.h"
#include "common.h"
#include "..\includes\ShapeHelper.h"

#ifdef GTASA
#include "CAnimManager.h"
#include "CStreaming.h"
#include "CTaskSimpleRunNamedAnim.h"
#endif

using namespace plugin;

/*
* Unified Watch Plugin for GTA:VC and GTA:SA
* Features:
* - Rotated analog clock hands using ShapeHelper
* - Game time synchronization
* - Day of week display (SA only)
* - Check watch animation (SA only)
* - Configurable via watch.ini
*/

class Watch {
    RwTexDictionary* watchTxd = nullptr;
    CSprite2d bg, hourHand, minHand, dayTex;
    int delayTime = 0;

public:
    Watch() {
        Events::initRwEvent += [&] {
            ShapeHelper::InitSinCosTable();

#ifdef GTASA
            watchTxd = CFileLoader::LoadTexDictionary(GAME_PATH("models\\watch_sa.txd"));
#else
            watchTxd = CFileLoader::LoadTexDictionary(GAME_PATH("models\\watch.txd"));
#endif
            if (watchTxd) {
                hourHand.m_pTexture = RwTexDictionaryFindNamedTexture(watchTxd, "hr");
                minHand.m_pTexture = RwTexDictionaryFindNamedTexture(watchTxd, "min");
                bg.m_pTexture = RwTexDictionaryFindNamedTexture(watchTxd, "watch");
            }
        };

        Events::drawingEvent += [&] {
            if (!bg.m_pTexture) return;

            config_file conf(PLUGIN_PATH("watch.ini"));
            int alwaysOn = conf["AlwaysON"].asInt(1);
            
            bool shouldDraw = (alwaysOn == 1);
            if (alwaysOn == 2) {
                if (KeyPressed('Q')) {
                    delayTime = CTimer::m_snTimeInMilliseconds + 3000;
#ifdef GTASA
                    RaiseWatchAnim();
#endif
                }
                shouldDraw = (CTimer::m_snTimeInMilliseconds < delayTime);
            }

            if (shouldDraw) {
                unsigned char hr = 0, min = 0, day = 0;
#ifdef GTASA
                hr = patch::GetUChar(0xB70153, false);
                min = patch::GetUChar(0xB70152, false);
                day = patch::GetUChar(0xB7014E, false);
#else
                hr = patch::GetUChar(0xA10B6B, false);
                min = patch::GetUChar(0xA10B92, false);
#endif

                float wl = conf["WatchScaleLeft"].asFloat(304.0f);
                float wr = conf["WatchScaleRight"].asFloat(86.0f);
                float wt = conf["WatchScaleTop"].asFloat(430.0f);
                float wb = conf["WatchScaleBottom"].asFloat(20.0f);

                // Draw background
                bg.Draw(SCREEN_RECT_BOTTOM_RIGHT(wl, wt, wr, wb), CRGBA(255, 255, 255, 255));

#ifdef GTASA
                // Draw day of week
                const char* week[7] = { "sun", "mon" ,"tue","wed","thu","fri","sat" };
                if (day >= 1 && day <= 7) {
                    dayTex.m_pTexture = RwTexDictionaryFindNamedTexture(watchTxd, week[day - 1]);
                    if (dayTex.m_pTexture) {
                        float dl = conf["DayScaleLeft"].asFloat(180.0f);
                        float dr = conf["DayScaleRight"].asFloat(145.0f);
                        float dt = conf["DayScaleTop"].asFloat(236.0f);
                        float db = conf["DayScaleBottom"].asFloat(213.0f);
                        dayTex.Draw(SCREEN_RECT_BOTTOM_RIGHT(dl, dt, dr, db), CRGBA(255, 255, 255, 255));
                    }
                }
#endif

                // Calculate hand rotations (in degrees)
                float minAngle = static_cast<float>(min) * 6.0f;
                float hrAngle = (static_cast<float>(hr % 12) * 30.0f) + (static_cast<float>(min) / 60.0f * 30.0f);

                // Hand offsets (from config or defaults)
                float hb = static_cast<float>(conf["HrNeedleLeftRight"].asInt(27));
                float hl = static_cast<float>(conf["HrNeedleUpDown"].asInt(54));
                float mb = static_cast<float>(conf["MinNeedleLeftRight"].asInt(27));
                float ml = static_cast<float>(conf["MinNeedleUpDown"].asInt(54));

                // Drawing hands with ShapeHelper
                // Original code used SCREEN_COORD_RIGHT/BOTTOM with p1..p8.
                // We'll use a fixed size (e.g. 75x75) and center it based on offsets.
                float size = 75.0f;
                
                // Hour Hand
                CRect hrRect(SCREEN_COORD_RIGHT(175.0f + hb + size), SCREEN_COORD_BOTTOM(175.0f + hl + size),
                             SCREEN_COORD_RIGHT(175.0f + hb - size), SCREEN_COORD_BOTTOM(175.0f + hl - size));
                float hrCenterX = (hrRect.left + hrRect.right) * 0.5f;
                float hrCenterY = (hrRect.top + hrRect.bottom) * 0.5f;
                ShapeHelper::DrawRotatedTexturedRectangle(hrRect, hrCenterX, hrCenterY, hrAngle, CRGBA(255, 255, 255, 255), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, &hourHand);

                // Minute Hand
                CRect minRect(SCREEN_COORD_RIGHT(175.0f + mb + size), SCREEN_COORD_BOTTOM(175.0f + ml + size),
                              SCREEN_COORD_RIGHT(175.0f + mb - size), SCREEN_COORD_BOTTOM(175.0f + ml - size));
                float minCenterX = (minRect.left + minRect.right) * 0.5f;
                float minCenterY = (minRect.top + minRect.bottom) * 0.5f;
                ShapeHelper::DrawRotatedTexturedRectangle(minRect, minCenterX, minCenterY, minAngle, CRGBA(255, 255, 255, 255), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, &minHand);
            }
        };

        Events::shutdownRwEvent += [&] {
            if (watchTxd) {
                RwTexDictionaryDestroy(watchTxd);
                watchTxd = nullptr;
            }
            hourHand.m_pTexture = nullptr;
            minHand.m_pTexture = nullptr;
            bg.m_pTexture = nullptr;
            dayTex.m_pTexture = nullptr;
        };
    }

#ifdef GTASA
    void RaiseWatchAnim() {
        CPlayerPed* player = FindPlayerPed();
        if (player) {
            const char* animName = "COPLOOK_WATCH";
            const char* blockName = "COP_AMBIENT";
            
            CAnimBlock* block = CAnimManager::GetAnimationBlock(blockName);
            if (block && !block->bLoaded) {
                int index = CAnimManager::GetAnimationBlockIndex(blockName);
                CStreaming::RequestModel(25575 + index, MISSION_REQUIRED | PRIORITY_REQUEST);
                CStreaming::LoadAllRequestedModels(true);
            }

            if (block && block->bLoaded) {
                unsigned short flags = ANIMATION_UNLOCK_LAST_FRAME | ANIMATION_PARTIAL | ANIMATION_TRANLSATE_X | ANIMATION_TRANLSATE_Y;
                CTaskSimpleRunNamedAnim* task = new CTaskSimpleRunNamedAnim(animName, blockName, flags, 1.0f, -1, false, true, false, false);
                player->m_pIntelligence->m_TaskMgr.SetTask(task, TASK_PRIMARY_PRIMARY, true);
            }
        }
    }
#endif
} watchPlugin;
