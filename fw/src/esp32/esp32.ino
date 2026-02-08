#include "ESP_I2S.h"
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

const uint8_t RX1PIN = 16;
const uint8_t TX1PIN = 17;

const uint8_t I2S_SCK = 5;       // Audio data bit clock
const uint8_t I2S_WS = 25;       // Audio data left and right clock
const uint8_t I2S_SDOUT = 26;    // ESP32 audio data output (to speakers)
I2SClass i2s;

BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
    Serial1.begin(9600, SERIAL_8N1, RX1PIN, TX1PIN);

    Serial.begin(115200);
    Serial.println("START");
    i2s.setPins(I2S_SCK, I2S_WS, I2S_SDOUT);
    if (!i2s.begin(I2S_MODE_STD, 44100, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
      Serial.println("Failed to initialize I2S!");
      while (1); // do nothing
    }

    Serial.println("START BT");
    a2dp_sink.start("Picoclock");
}

void loop() {
  //delay(20000);
  while(Serial1.available()) {
    delay(50);
    String in = Serial1.readString();
    in.trim();
    if (in == "START") {
      Serial.println("Starting...");
      a2dp_sink.start("Picoclock");
      Serial1.println("OK");
    } else if (in == "END") {
      Serial.println("Ending...");
      a2dp_sink.end();
      Serial1.println("OK");
    } else {
      Serial.println("Received:"+in);
      Serial1.println("UNKNOWN");
    }
  }
}