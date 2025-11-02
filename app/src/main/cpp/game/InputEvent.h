#pragma once

enum class InputEventType {
    TouchDown,
    TouchMove,
    TouchUp,
    TouchCancel
};

struct InputEvent {
    InputEventType type{InputEventType::TouchDown};
    float normalizedX{0.0f};
    float normalizedY{0.0f};
};
