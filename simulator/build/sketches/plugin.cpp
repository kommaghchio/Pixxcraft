#include "Arduino.h"
void setup();
void loop();
#include "joined.ino"
extern "C" __declspec(dllexport) void sketch_setup() { setup(); }
extern "C" __declspec(dllexport) void sketch_loop() { loop(); }
