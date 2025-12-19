#include <Wire.h>
#include <DHT.h>

#define I2C_ADDR 1

#define DHTPIN         4
#define DHTTYPE        DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TRIG_PIN       3
#define ECHO_PIN       4

#define SOUND_SPEED 0.034

volatile char lastCmd = 0;
volatile bool cmdPending = false;

float lastValue = NAN;     // último valor medido para o comando atual
unsigned long lastMeasureMs = 0;

float measureTemperatureC() {
  // DHT11: pode retornar NAN nas primeiras leituras; repetir se necessário.
  float t = dht.readTemperature(); // °C
  return t;
}

float measureHumidity() {
  float h = dht.readHumidity(); // %
  return h;
}

float measureDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration = pulseIn(ECHO_PIN, HIGH);
    unsigned long distanceCm = duration * SOUND_SPEED/2;

    return distanceCm;
}

void onI2CReceive(int numBytes) {
  while (Wire.available()) {
    char c = (char)Wire.read();
    lastCmd = c;
    cmdPending = true; // sinaliza para o loop fazer a medição
  }
}

void onI2CRequest() {
  // Apenas envia o último valor calculado (sem bloquear com leituras demoradas)
  float valueToSend = lastValue;
  // Opcional: se ainda não houve medida, envie NaN; o mestre pode tratar.
  Wire.write((uint8_t*)&valueToSend, sizeof(float));
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  dht.begin();

  Wire.begin(I2C_ADDR);
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);

  // Não é necessário Serial no escravo, mas útil para debug:
  Serial.begin(115200);
  Serial.println(F("Arduino I2C slave iniciado."));
}

void loop() {
  // Se chegou um novo comando, faça a medição aqui (fora de interrupção)
  if (cmdPending) {
    float v = NAN;
    switch (lastCmd) {
      case 'a': v = measureTemperatureC(); break;
      case 'b': v = measureHumidity();     break;
      case 'c': v = measureDistanceCm();   break;
      default:  v = NAN;                   break;
    }
    lastValue = v;
    lastMeasureMs = millis();
    cmdPending = false;
  }

  // DHT11 tem taxa baixa; evite leituras muito frequentes.
  // O loop pode ficar leve:
  delay(5);
}
