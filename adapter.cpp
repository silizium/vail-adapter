#include <Arduino.h>
#include <Keyboard.h>
#include <MIDIUSB.h>
#include <pitchToNote.h>
#include <cstddef>
#include "keyers.h"
#include "adapter.h"
#include "polybuzzer.h"
#include "keycodes.h"
#include <UniversalTimer.h>
extern UniversalTimer timeoutSilence;

#define MILLISECOND 1
#define SECOND (1000 * MILLISECOND)

VailAdapter::VailAdapter(unsigned int PiezoPin) {
    this->buzzer = new PolyBuzzer(PiezoPin);
}

bool VailAdapter::KeyboardMode() {
    return this->keyboardMode;
}

// Send a MIDI Key Event
void VailAdapter::midiKey(uint8_t key, bool down) {
    midiEventPacket_t event = {uint8_t(down?9:8), uint8_t(down?0x90:0x80), key, 0x7f};
    MidiUSB.sendMIDI(event);
    MidiUSB.flush();
}

// Send a keyboard key event
void VailAdapter::keyboardKey(uint8_t key, bool down) {
#ifdef TIMEOUT_SILENCE
    if(timeoutSilence.isRunning()==false){
       timeoutSilence.start();
       Keyboard.press(KEY_LEFT_ALT);
       Keyboard.press(KEY_RIGHT_SHIFT);
       delay(10);
       Keyboard.press(KEY_END);
       delay(10);
       Keyboard.release(KEY_END);
       delay(10);
       Keyboard.release(KEY_RIGHT_SHIFT);
       Keyboard.release(KEY_LEFT_ALT);
    }else{
       timeoutSilence.resetTimerValue(); 
    }
#endif
    if (down) {
        Keyboard.press(key);
    } else {
        Keyboard.release(key);
        
    }
}

// Begin transmitting
void VailAdapter::BeginTx() {
    this->buzzer->Note(0, this->txNote);
    if (this->keyboardMode) {
        this->keyboardKey(STRAIGHT_KEYBOARD_KEY, true);
    } else {
        this->midiKey(0, true);
        this->midiKey(MIDI_KEY, true);
    }
}

// Stop transmitting
void VailAdapter::EndTx() {
    this->buzzer->NoTone(0);
    if (this->keyboardMode) {
        this->keyboardKey(STRAIGHT_KEYBOARD_KEY, false);
    } else {
        this->midiKey(0, false);
        this->midiKey(MIDI_KEY, false);
    }
}

// Handle a paddle being pressed.
//
// The caller needs to debounce keys and deal with keys wired in parallel.
void VailAdapter::HandlePaddle(Paddle paddle, bool pressed) {
    switch (paddle) {
    case PADDLE_STRAIGHT:
        if (pressed) {
            this->BeginTx();
        } else {
            this->EndTx();
        }
        return;
    case PADDLE_DIT:
        if (this->keyer) {
            this->keyer->Key(paddle, pressed);
        } else if (this->keyboardMode) {
            this->keyboardKey(DIT_KEYBOARD_KEY, pressed);
        } else {
            this->midiKey(1, pressed);
        }
        break;
    case PADDLE_DAH:
        if (this->keyer) {
            this->keyer->Key(paddle, pressed);
        } else if (this->keyboardMode) {
            this->keyboardKey(DAH_KEYBOARD_KEY, pressed);
        } else {
            this->midiKey(2, pressed);
        }
        break;
    }
}

// Handle a MIDI event.
//
// We act as a MIDI 
void VailAdapter::HandleMIDI(midiEventPacket_t event) {
    uint16_t msg = (event.byte1 << 8) | (event.byte2 << 0);
    switch (event.byte1) {
    case 0xB0: // Controller Change
        switch (event.byte2) {
            case 0: // turn keyboard mode on/off
                this->keyboardMode = (event.byte3 > 0x3f);
                MidiUSB.sendMIDI(event); // Send it back to acknowledge
                break;
            case 1: // set dit duration (0-254) *2ms
                this->ditDuration = event.byte3 * 2 * MILLISECOND;
                if (this->keyer) {
                    this->keyer->SetDitDuration(this->ditDuration);
                }
                break;
            case 2: // set tx note
                this->txNote = event.byte3;
                break;
        }
        break;
    case 0xC0: // Program Change
        if (this->keyer) {
            this->keyer->Release();
        }
        this->keyer = GetKeyerByNumber(event.byte2, this);
        this->keyer->SetDitDuration(this->ditDuration);
        break;
    case 0x80: // Note off
        this->buzzer->NoTone(1);
        break;
    case 0x90: // Note on
        this->buzzer->Note(1, event.byte2);
        break;
    }
}

void VailAdapter::Tick(unsigned millis) {
    if (this->keyer) {
        this->keyer->Tick(millis);
    }
}
