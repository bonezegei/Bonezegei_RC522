#include <Bonezegei_RC522.h>

#define SS_PIN  17
#define RST_PIN 5

Bonezegei_RC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Initializing Bonezegei RC522...");
  if (rfid.begin()) {
    Serial.println("RC522 Ready!");
  } else {
    Serial.println("RC522 Initialization Failed!");
  }
}

void loop() {
  if (rfid.readCard()) {
    Serial.print("Card UID: ");
    Serial.println(rfid.getUIDString());
    rfid.halt();
    rfid.stopCrypto();
    delay(1000);
  }
}