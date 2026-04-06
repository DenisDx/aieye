#include "button.h"

// Configure BUTTON_PIN as input with pull-up; active-low (press = LOW).
void Button::begin() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// Returns ButtonEvent on release; NONE otherwise.
// Debounce: state must be stable for DEBOUNCE_MS before it is considered.
ButtonEvent Button::poll() {
    bool reading = digitalRead(BUTTON_PIN);
    uint32_t now = millis();

    if (reading != _lastReading) {
        _lastChangeTime = now;
        _lastReading    = reading;
        return ButtonEvent::NONE;
    }

    if ((now - _lastChangeTime) < DEBOUNCE_MS) return ButtonEvent::NONE;

    // --- stable state ---
    if (!_pressed && reading == LOW) {
        // Falling edge: button down
        _pressed   = true;
        _pressTime = now;
    } else if (_pressed && reading == HIGH) {
        // Rising edge: button released
        _pressed = false;
        uint32_t dur = now - _pressTime;
        if (dur >= LONG_PRESS_MS) return ButtonEvent::LONG_PRESS;
        if (dur >= MIN_PRESS_MS)  return ButtonEvent::SHORT_PRESS;
    }

    return ButtonEvent::NONE;
}
