#include <Arduino.h>
#include "frfd.h"

FRFD* frfd;

void setup() {
    // Initialize FRFD
    frfd = new FRFD();

    if (!frfd->begin()) {
        Serial.println("FRFD initialization failed!");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("\nFRFD ready for operations");
    Serial.println("Available commands:");
    Serial.println("  triage  - Run triage mode");
    Serial.println("  collect - Run collection mode");
    Serial.println("  contain - Run containment mode");
    Serial.println("  analyze - Run analysis mode");
    Serial.println("  status  - Show status and chain of custody");
    Serial.println("  os:<windows|linux|macos> - Set detected OS");
}

void loop() {
    frfd->loop();
    delay(10);
}
