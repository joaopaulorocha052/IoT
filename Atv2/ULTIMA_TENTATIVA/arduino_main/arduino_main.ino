// mega_hub_sensores.ino  (serve para Master e Slave)

#include <Wire.h>
#include <DHT.h>

#define I2C_ADDR 0x08  // Endereço I2C do Mega

// --- Pinos dos sensores ---
#define DHTPIN   2
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TRIG_PIN 3
#define ECHO_PIN 4
#define SOUND_SPEED 0.034f  // cm/us

// --- Variáveis com as últimas leituras ---
float lastTemp = NAN;    // °C
float lastHum  = NAN;    // %
float lastDist = NAN;    // cm

// Comando atual solicitado pelo mestre ('a', 'b', 'c')
volatile char currentCmd = 'a';

// Atualiza apenas qual sensor o mestre quer ler
void onI2CReceive(int numBytes) {
  while (Wire.available()) {
    char c = (char)Wire.read();
    if (c == 'a' || c == 'b' || c == 'c') {
      currentCmd = c;
    }
  }
}

// Envia o float correspondente ao comando atual
void onI2CRequest() {
  float value = NAN;
  switch (currentCmd) {
    case 'a': value = lastTemp; break;
    case 'b': value = lastHum;  break;
    case 'c': value = lastDist; break;
    default:  value = NAN;      break;
  }
  Wire.write((uint8_t*)&value, sizeof(float));
}

// Mede a distância em cm com o HC-SR04
float measureDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL); // timeout 30 ms
  if (duration == 0) {
    return NAN;
  }

  float distanceCm = (duration * SOUND_SPEED) / 2.0f;
  return distanceCm;
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();

  Wire.begin(I2C_ADDR);          // Mega como I2C Slave
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);

  Serial.begin(115200);          // (opcional, para debug local)
  Serial.println(F("Mega - Hub de sensores (T/U/D) pronto."));
}

void loop() {
  static unsigned long lastUpdateMs = 0;
  unsigned long now = millis();

  // Atualiza sensores periodicamente (ex: a cada 1000 ms)
  if (now - lastUpdateMs >= 1000UL) {
    lastUpdateMs = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float d = measureDistanceCm();

    lastTemp = t;
    lastHum  = h;
    lastDist = d;

    // Debug opcional
    Serial.print(F("T="));
    Serial.print(lastTemp);
    Serial.print(F("  U="));
    Serial.print(lastHum);
    Serial.print(F("  D="));
    Serial.println(lastDist);
  }

  // Nada mais aqui; leituras I2C são disparadas pelo mestre (ESP32)
}
