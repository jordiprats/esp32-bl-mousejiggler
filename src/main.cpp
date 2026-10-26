#include <Arduino.h>
#include <BleMouse.h>

BleMouse bleMouse("MX Master 4", "Logitech", 100); 

void setup() {
  bleMouse.begin();
}

void loop() {
  if (bleMouse.isConnected()) {
    bleMouse.move(1, 0);
    delay(1000); 
    bleMouse.move(-1, 0);
    delay(1500);
    bleMouse.move(0, 1);
    delay(2000); 
    bleMouse.move(0, -1);
    delay(1000);
  } else {
    delay(5000);
  }
}