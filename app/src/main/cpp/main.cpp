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
#include "platform/AndroidPlatformServices.h"

struct RuntimeContext {
    Game *game{nullptr};
    AndroidPlatformServices *platform{nullptr};
};

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    auto *context = reinterpret_cast<RuntimeContext *>(app->userData);
    if (!context || !context->game || !context->platform) {
        return 0;
    }

    WindowInfo windowInfo = context->platform->getWindowInfo();
    if (windowInfo.width == 0 || windowInfo.height == 0) {
        return 0;
    }

    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);

    float normalizedX = (x / static_cast<float>(windowInfo.width)) * 2.0f - 1.0f;
    float normalizedY = (y / static_cast<float>(windowInfo.height)) * 2.0f - 1.0f;

    std::unique_ptr<InputCommand> command;
    GameState state = context->game->getCurrentStateType();

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            if (state == GameState::Playing) {
                command = std::make_unique<MoveShipCommand>(InputEventType::TouchDown, normalizedX, normalizedY);
            } else {
                InputEvent evt{InputEventType::TouchDown, normalizedX, normalizedY};
                context->game->handleInput(evt); //forwards to mainmenu state since its active first
                return 1; // skip enqueuing RestartGameCommand
//                command = std::make_unique<RestartGameCommand>();
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

    context->game->enqueueCommand(std::move(command));
    return 1;
}

static void handle_cmd(android_app *app, int32_t cmd) {
    auto *context = reinterpret_cast<RuntimeContext *>(app->userData);
    if (!context || !context->platform) {
        return;
    }
    context->platform->handleAppCommand(cmd);
}

void android_main(struct android_app *app) {
    AndroidPlatformServices platform(*app);
    std::unique_ptr<Game> game = std::make_unique<Game>(platform);
    RuntimeContext context{game.get(), &platform};
    app->userData = &context;
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
                context.game = nullptr;
                game.reset();
                app->userData = nullptr;
                return;
            }
        }

        GameTime::updateTime(platform);
        if (game) {
            game->update(GameTime::deltaTime);
            game->render();
        }
    }
}
