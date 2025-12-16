// mega_hub_sensores.ino  (serve para Master e Slave)

#include <Wire.h>
#include <DHT.h>

#define I2C_ADDR 0x09  // Endereço I2C do Mega (evitar 1-7, são reservados)

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
volatile bool cmdReceived = false;

// Atualiza apenas qual sensor o mestre quer ler
void onI2CReceive(int numBytes) {
  while (Wire.available()) {
    char c = (char)Wire.read();
    if (c == 'a' || c == 'b' || c == 'c') {
      currentCmd = c;
      cmdReceived = true;
    }
  }
}

// Envia o float correspondente ao comando atual
void onI2CRequest() {
  float value = NAN;
  char cmd = currentCmd;
  
  switch (cmd) {
    case 'a': value = lastTemp; break;
    case 'b': value = lastHum;  break;
    case 'c': value = lastDist; break;
    default:  value = NAN;      break;
  }
  
  // Envia os 4 bytes do float
  uint8_t buf[4];
  memcpy(buf, &value, sizeof(float));
  Wire.write(buf, 4);
}

// Mede a distância em cm com o HC-SR04
float measureDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 50000UL); // timeout 50 ms
  if (duration == 0) {
    return NAN;
  }

  float distanceCm = (duration * SOUND_SPEED) / 2.0f;
  return distanceCm;
}

void setup() {
  Serial.begin(115200);          // Inicializa Serial primeiro
  delay(2000);                   // Aguarda estabilização
  
  Serial.println(F("\n================================="));
  Serial.println(F("Arduino - Hub de Sensores I2C"));
  Serial.println(F("================================="));
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println(F("[OK] Pinos HC-SR04 configurados"));

  dht.begin();
  Serial.println(F("[OK] Sensor DHT11 inicializado"));

  Wire.begin(I2C_ADDR);          // Mega como I2C Slave
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);
  
  Serial.print(F("[OK] I2C Slave inicializado no endereco 0x"));
  Serial.println(I2C_ADDR, HEX);
  Serial.println(F("\nAguardando requisicoes I2C do Master..."));
  Serial.println(F("=================================\n"));
}

void loop() {
  static unsigned long lastUpdateMs = 0;
  unsigned long now = millis();

  // Atualiza sensores periodicamente (ex: a cada 500 ms)
  if (now - lastUpdateMs >= 500UL) {
    lastUpdateMs = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float d = measureDistanceCm();

    lastTemp = t;
    lastHum  = h;
    lastDist = d;

    // Debug - mostra leituras e comandos I2C recebidos
    Serial.print(F("T="));
    Serial.print(lastTemp);
    Serial.print(F("  U="));
    Serial.print(lastHum);
    Serial.print(F("  D="));
    Serial.print(lastDist);
    
    if (cmdReceived) {
      Serial.print(F("  [I2C cmd='"));
      Serial.print(currentCmd);
      Serial.print(F("']"));
      cmdReceived = false;
    }
    Serial.println();
  }

  // Nada mais aqui; leituras I2C são disparadas pelo mestre (ESP32)
}
