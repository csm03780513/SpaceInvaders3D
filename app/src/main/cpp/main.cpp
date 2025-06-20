#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <android/input.h>
#include "Renderer.h"


#define LOGI(...) __android_log_print(ANDROID_LOG_ERROR, "Vulkan", __VA_ARGS__)
Renderer *g_renderer = nullptr; // global pointer
volatile bool g_pendingRestart = false;

void set_ship_x(float x, float y);


static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_DOWN ||
            AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_MOVE) {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            int32_t width = ANativeWindow_getWidth(app->window);
            int32_t height = ANativeWindow_getHeight(app->window);
            // Convert X to normalized device coordinate [-1, 1]
            float ndcX = (x / (float) width) * 2.0f - 1.0f;
            float ndcY = (y / (float) height) * 1.0f - 1.0f;
            if (g_renderer && g_renderer->gameState == GameState::Playing) {
                // Move ship as before
                if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_DOWN ||
                    AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_MOVE) {
                    set_ship_x(ndcX,ndcY);
                }
                // Shoot bullet
                if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_DOWN) {
                    g_renderer->spawnBullet();
                }
            } else if (g_renderer && g_renderer->gameState != GameState::Playing) {
                // TAP = RESTART GAME when game over/won
                if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_DOWN) {
                    g_pendingRestart = true; // Set a flag to restart in the updateExplosionParticles loop
                }
            }
        }
    }
    return 1; // Event handled
}

void set_ship_x(float x, float y) {
    if (g_renderer) {
        g_renderer->shipX_ = x;
        g_renderer->shipY_ = y;
    }
}

void android_main(struct android_app *app) {
    Renderer *renderer = nullptr;
    app->onInputEvent = handle_input;

    while (true) {
        int events;
        android_poll_source *source;
        while (ALooper_pollOnce(0, nullptr, &events, (void **) &source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) return;

            // Only create Renderer once window is valid
            if (!renderer && app->window) {
                LOGI("app here:=>");
                renderer = new Renderer(app);
                g_renderer = renderer;
            }
        }
        if (g_pendingRestart) {
            g_renderer->restartGame();
            g_pendingRestart = false;
        }

        if (renderer) renderer->drawFrame();
    }
}

