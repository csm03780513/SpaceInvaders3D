#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/input.h>

#include <memory>

#include "GameTime.h"
#include "game/Game.h"
#include "game/InputEvent.h"

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    auto *game = reinterpret_cast<Game *>(app->userData);
    if (!game || !app->window) {
        return 0;
    }

    InputEvent inputEvent{};
    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);
    int32_t width = ANativeWindow_getWidth(app->window);
    int32_t height = ANativeWindow_getHeight(app->window);
    if (width == 0 || height == 0) {
        return 0;
    }

    inputEvent.normalizedX = (x / static_cast<float>(width)) * 2.0f - 1.0f;
    inputEvent.normalizedY = (y / static_cast<float>(height)) * 2.0f - 1.0f;

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            inputEvent.type = InputEventType::TouchDown;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            inputEvent.type = InputEventType::TouchMove;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            inputEvent.type = InputEventType::TouchUp;
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            inputEvent.type = InputEventType::TouchCancel;
            break;
        default:
            return 0;
    }

    return game->handleInput(inputEvent) ? 1 : 0;
}

static void handle_cmd(android_app *app, int32_t cmd) {
    auto *game = reinterpret_cast<Game *>(app->userData);
    if (!game) {
        return;
    }
    game->handleCmd(cmd);
}

void android_main(struct android_app *app) {
    std::unique_ptr<Game> game = std::make_unique<Game>(app);
    app->userData = game.get();
    app->onInputEvent = handle_input;
    app->onAppCmd = handle_cmd;

    while (true) {
        int events = 0;
        android_poll_source *source = nullptr;
        while (ALooper_pollOnce(0, nullptr, &events, reinterpret_cast<void **>(&source)) >= 0) {
            if (source) {
                source->process(app, source);
            }
            if (app->destroyRequested) {
                game.reset();
                app->userData = nullptr;
                return;
            }
        }

        Time::updateTime();
        if (game) {
            game->update(Time::deltaTime);
            game->render();
        }
    }
}
