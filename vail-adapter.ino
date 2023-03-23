// Distributed under the MIT license
// Please see https://github.com/nealey/vail-adapter/

// MIDIUSB - Version: Latest 
#include <MIDIUSB.h>
#include <Keyboard.h>
#include <Adafruit_FreeTouch.h>
#include "bounce2.h"
#include "touchbounce.h"
#include "adapter.h"
#include "keycodes.h"

#define DIT_PIN 2
#define DAH_PIN 1
#define KEY_PIN 0
#define QT_DIT_PIN A6
#define QT_DAH_PIN A7
#define QT_KEY_PIN A8
#define PIEZO 10
#define LED_ON false // Xiao inverts this logic for some reason
#define LED_OFF (!LED_ON)

#define TONE 3000

#define MILLISECOND 1
#define SECOND (1 * MILLISECOND)

#ifdef TIMEOUT_SILENCE
#include <UniversalTimer.h>
UniversalTimer timeoutSilence(TIMEOUT_SILENCE, delay);
#endif

VailAdapter adapter = VailAdapter(PIEZO);
bool trs = false; // true if a TRS plug is in a TRRS jack
uint16_t iambicDelay = 80 * MILLISECOND;
Bounce dit = Bounce();
Bounce dah = Bounce();
Bounce key = Bounce();
TouchBounce qt_dit = TouchBounce();
TouchBounce qt_dah = TouchBounce();
TouchBounce qt_key = TouchBounce();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  dit.attach(DIT_PIN, INPUT_PULLUP);
  dah.attach(DAH_PIN, INPUT_PULLUP);
  key.attach(KEY_PIN, INPUT_PULLUP);
  qt_dit.attach(QT_DIT_PIN);
  qt_dah.attach(QT_DAH_PIN);
  qt_key.attach(QT_KEY_PIN);

  Keyboard.begin();

  // To auto-sense a straight key in a TRRS jack,
  // we just check to see if DAH is closed. 
  // The sleeve on the straight key's TRS plug
  // will short the second ring to the sleeve.
  for (int i = 0; i < 16; i++) {
    delay(20);
    dah.update();
  }
  if (dah.read() == LOW) {
    trs = true;
    key = dit;
  }
}

// A reentrant doodad to blink out the letter V at startup
// After startup, display the status of the keyboard
#define HELLO_BITS 0b0000101010111000
void setLED() {
  static bool beepin = false;
  int beat = millis() / iambicDelay;
  bool on = adapter.KeyboardMode(); // If we're not in intro, display status of keyboard

  if (beat < 16) {
    on = HELLO_BITS & (1 << (15-beat));
    if (on != beepin) {
      if (on) {
        tone(PIEZO, TONE);
      } else {
        noTone(PIEZO);
      }
      beepin = on;
    }
  }
  
  digitalWrite(LED_BUILTIN, on?LED_ON:LED_OFF);
}

void loop() {
  unsigned now = millis();  
  midiEventPacket_t event = MidiUSB.read();

  setLED();
  adapter.Tick(now);

  if (event.header) {
    adapter.HandleMIDI(event);
  }

  // Monitor straight key pin
  if (key.update() || qt_key.update()) {
    bool pressed = !key.read() || qt_key.read();
    adapter.HandlePaddle(PADDLE_STRAIGHT, pressed);
  }

  // If we made dit = dah, we have a straight key on the dit pin,
  // so we skip other keys polling.
  if (trs) {
    return;
  }

  if (dit.update() || qt_dit.update()) {
    bool pressed = !dit.read() || qt_dit.read();
    adapter.HandlePaddle(PADDLE_DIT, pressed);
  }
  
  if (dah.update() || qt_dah.update()) {
    bool pressed = !dah.read() || qt_dah.read();
    adapter.HandlePaddle(PADDLE_DAH, pressed);
  }

#ifdef TIMEOUT_SILENCE
  if(timeoutSilence.check()==true){
    timeoutSilence.resetTimerValue();
    timeoutSilence.stop();
    Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(KEY_RIGHT_SHIFT);
    delay(10);
    Keyboard.press(KEY_END);
    delay(10);
    Keyboard.release(KEY_END);
    delay(10);

    /*
    delay(10);
    for(size_t i=0; i<SILENCE_AMOUNT; i++){
       Keyboard.press(KEY_UP_ARROW);
       delay(10);
       Keyboard.release(KEY_UP_ARROW);
       delay(15);
    }
    */
    Keyboard.release(KEY_RIGHT_SHIFT);
    Keyboard.release(KEY_LEFT_ALT);
  }
#endif
}
