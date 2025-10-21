#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <android/input.h>
#include "Renderer.h"
#include "Time.h"
#include <stdexcept>
#include <atomic>


#define LOGI(...) __android_log_print(ANDROID_LOG_ERROR, "Vulkan", __VA_ARGS__)
Renderer *g_renderer = nullptr; // global pointer
volatile bool g_pendingRestart = false;

std::atomic<bool> g_touchActive{false};
std::atomic<float> g_touchX{0.0f};
std::atomic<float> g_touchY{0.0f};

void set_ship_x(float x, float y, bool fireBullet);


static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_MOVE ||
            action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            int32_t width = ANativeWindow_getWidth(app->window);
            int32_t height = ANativeWindow_getHeight(app->window);
            // Convert X to normalized device coordinate [-1, 1]
            float ndcX = (x / (float) width) * 2.0f - 1.0f;
            float ndcY = (y / (float) height) * 2.0f - 1.0f;
            g_touchX.store(ndcX);
            g_touchY.store(ndcY);
            if (g_renderer && g_renderer->gameState == GameState::Playing) {
                g_touchActive.store(true);
                // Move ship immediately so visual stays in sync with finger
                set_ship_x(ndcX, ndcY, false);
            } else if (g_renderer && g_renderer->gameState != GameState::Playing) {
                // TAP = RESTART GAME when game over/won
                if (action == AMOTION_EVENT_ACTION_DOWN) {
                    g_pendingRestart = true; // Set a flag to restart in the updateExplosionParticles loop
                }
            }
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP ||
                   action == AMOTION_EVENT_ACTION_CANCEL) {
            if (AMotionEvent_getPointerCount(event) <= 1) {
                g_touchActive.store(false);
            }
        }
        return 1; // Handled touch events
    }
    // For all other events (including volume keys), return 0 (not handled)
    return 0;
}

void set_ship_x(float x, float y, bool fireBullet) {
    if (g_renderer) {
        g_renderer->shipX_ = x;
        g_renderer->shipY_ = y - 0.12f;
        if (fireBullet) {
            g_renderer->spawnBullet(BulletType::Ship, {x, y - 0.12f});
        }
    }
}



void handle_cmd(android_app *app, int32_t cmd) {

    switch (cmd) {
        case APP_CMD_CONFIG_CHANGED:
            LOGI("Config changed (orientation)");

            break;
        case APP_CMD_WINDOW_RESIZED:
            LOGI("Window resized");

            break;
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr) {
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("Window terminated");
            break;
        case APP_CMD_LOST_FOCUS:
            g_renderer->stopAudioPlayer();
            LOGI("Lost focus");
            break;
        case APP_CMD_GAINED_FOCUS:
            g_renderer->resumeAudioPlayer();
            break;
        case APP_CMD_DESTROY:
            LOGI("APP GETTING DESTROYED:::");
            if (g_renderer) {
                delete g_renderer;
                g_renderer = nullptr;
            }
            break;
        default:
            break;
    }
}


void android_main(struct android_app *app) {
    Renderer *renderer = nullptr;
    std::shared_ptr<Time> time = std::make_shared<Time>();
    app->onInputEvent = handle_input;
    app->onAppCmd = handle_cmd;

    while (true) {
        int events;
        android_poll_source *source;
        while (ALooper_pollOnce(0, nullptr, &events, (void **) &source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                if (renderer) {
                    delete renderer;
                    renderer = nullptr;
                    g_renderer = nullptr;
                }
                return;
            }

            // Only create Renderer once window is valid
            if (!renderer && app->window) {
                LOGI("app here:=>");
                try {
                    renderer = new Renderer(app);
                    g_renderer = renderer;
                } catch (const std::exception &e) {
                    LOGI("Renderer initialization failed: %s", e.what());
                    return;
                }
            }
        }
        Time::updateTime();
        if (g_pendingRestart) {
            g_renderer->restartGame();
            g_pendingRestart = false;
        }

        if (g_touchActive.load() && renderer && renderer->gameState == GameState::Playing) {
            float touchX = g_touchX.load();
            float touchY = g_touchY.load();
            set_ship_x(touchX, touchY, true);
        }

        if (renderer) renderer->drawFrame();
    }
}



