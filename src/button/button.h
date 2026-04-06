#pragma once

#include <Arduino.h>

enum class ButtonEvent { NONE, SHORT_PRESS, LONG_PRESS };

// Debounced button polling on BUTTON_PIN (defined via -D build flag).
// Short press  (<  LONG_PRESS_MS release) → SHORT_PRESS.
// Long press   (>= LONG_PRESS_MS release) → LONG_PRESS.
// Call poll() every loop iteration; it returns an event at most once per press.
class Button {
public:
    // Configure BUTTON_PIN as INPUT_PULLUP.
    void begin();

    // Sample pin state; returns event on button release or NONE.
    ButtonEvent poll();

private:
    bool     _lastReading{true};     // HIGH = not pressed (pullup)
    bool     _pressed{false};
    uint32_t _lastChangeTime{0};
    uint32_t _pressTime{0};

    static constexpr uint32_t DEBOUNCE_MS   = 50;
    static constexpr uint32_t LONG_PRESS_MS = 3000;
    static constexpr uint32_t MIN_PRESS_MS  = 50;
};
