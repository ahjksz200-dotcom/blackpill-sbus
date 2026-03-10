#define PPM_PIN PA0
#define CHANNELS 8

volatile uint16_t ppm[CHANNELS];
volatile uint8_t channel = 0;

volatile uint32_t lastTime = 0;

void ppmISR() {

  uint32_t now = micros();
  uint32_t diff = now - lastTime;
  lastTime = now;

  if (diff > 3000) { 
    // Sync pulse
    channel = 0;
  }
  else {
    if (channel < CHANNELS) {
      ppm[channel] = diff;
      channel++;
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(PPM_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(PPM_PIN),
    ppmISR,
    RISING
  );
}

void loop() {

  static uint32_t t = 0;

  if (millis() - t > 200) {

    t = millis();

    Serial.print("PPM: ");

    for (int i = 0; i < CHANNELS; i++) {

      Serial.print(ppm[i]);
      Serial.print(" ");
    }

    Serial.println();
  }
}
