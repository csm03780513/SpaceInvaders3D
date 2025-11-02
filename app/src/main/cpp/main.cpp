#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/input.h>

#include <memory>

#include "GameTime.h"
#include "game/Game.h"
#include "game/InputEvent.h"
#include "input/InputCommand.h"
#include "input/FireCommand.h"
#include "input/MoveShipCommand.h"
#include "input/RestartGameCommand.h"

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    auto *game = reinterpret_cast<Game *>(app->userData);
    if (!game || !app->window) {
        return 0;
    }

    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);
    int32_t width = ANativeWindow_getWidth(app->window);
    int32_t height = ANativeWindow_getHeight(app->window);
    if (width == 0 || height == 0) {
        return 0;
    }

    float normalizedX = (x / static_cast<float>(width)) * 2.0f - 1.0f;
    float normalizedY = (y / static_cast<float>(height)) * 2.0f - 1.0f;

    std::unique_ptr<InputCommand> command;
    GameState state = game->getCurrentStateType();

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            if (state == GameState::Playing) {
                command = std::make_unique<MoveShipCommand>(InputEventType::TouchDown, normalizedX, normalizedY);
            } else {
                command = std::make_unique<RestartGameCommand>();
            }
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            if (state == GameState::Playing) {
                command = std::make_unique<MoveShipCommand>(InputEventType::TouchMove, normalizedX, normalizedY);
            }
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            if (state == GameState::Playing) {
                command = std::make_unique<FireCommand>(InputEventType::TouchUp, normalizedX, normalizedY);
            }
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            if (state == GameState::Playing) {
                command = std::make_unique<FireCommand>(InputEventType::TouchCancel, normalizedX, normalizedY);
            }
            break;
        default:
            break;
    }

    if (!command) {
        return 0;
    }

    game->enqueueCommand(std::move(command));
    return 1;
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

        GameTime::updateTime();
        if (game) {
            game->update(GameTime::deltaTime);
            game->render();
        }
    }
}
