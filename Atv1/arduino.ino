#include <Arduino.h>
#include "DHT.h"
#include "head_arduino.h"

DHT dht(DHTPIN, DHTTYPE);

#define DISTANCE_SAMPLE_WINDOW 10

unsigned long ultimoMillisLed1 = 0;
unsigned long ultimoMillisLed2 = 0;
int estadoLed1 = LOW;
int estadoLed2 = LOW;

long duration = 0;
float distanceAccumulator = 0.0f;
int distanceIterator = 0;
float average = 0.0f;
int lastDistanceState = -1;
int lastLuminosityState = -1;
int lastHumidityState = -1;



void enviarEstado(int estado, int pin0, int pin1) {
  // Converte o número do estado (0-3) para 2 bits
  int bit0 = estado % 2;      // Se estado for 0 ou 2, bit0=0. Se for 1 ou 3, bit0=1.
  int bit1 = estado / 2;      // Se estado for 0 ou 1, bit1=0. Se for 2 ou 3, bit1=1.

  digitalWrite(pin0, bit0);
  digitalWrite(pin1, bit1);
}

/* Distância */
int resolveDistanceState(float measuredDistance) {
  if (measuredDistance >= distanceThresholdNoObs - distanceMargin) {
    return 0b00;
  }
  if (measuredDistance >= distanceThresholdFarAway - distanceMargin) {
    return 0b01;
  }
  if (measuredDistance >= distanceThresholdMedium - distanceMargin) {
    return 0b10;
  }
  return 0b11;
}

void sendDistance() {
  const int state = resolveDistanceState(average);
  if (state == lastDistanceState) {
    return;
  }

  lastDistanceState = state;
  enviarEstado(state, pino_distancia_bit0, pino_distancia_bit1); // 11 e 12 livres
}

float getDistance(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  
  return duration * SOUND_SPEED/2;
}


/* Luminosidade */
int resolveLuminosityState(int ldr_value) {
  return (ldr_value >= threshold_obstacle) ? 0b1 : 0b0;
}

void sendLight() {
  int ldr_value = analogRead(LDRPin);
  int state = resolveLuminosityState(ldr_value);
  if(state == lastLuminosityState){
    return;
  }
  digitalWrite(pino_luminosity_bit, state);
}

/* Humidade */
int resolveHumidityState(float humidity_value){
  return (humidity_value >= threshold_humidity) ? 0b1 : 0b0;
}

void sendHumidity(){
  float humidity_value = dht.readHumidity();
  int state = resolveHumidityState(humidity_value);
  if(state == lastHumidityState){
    return;
  }
  digitalWrite(pino_humidity_bit, state);
}


// Funções de estado (APENAS AÇÕES DOS LEDS)
void idle() {
  digitalWrite(pinLED1, LOW);
  digitalWrite(pinLED2, LOW);
}

void cleaning() {
  unsigned long currentMillis = millis();
  if (currentMillis - ultimoMillisLed1 >= periodoLed1_cleaning) {
    ultimoMillisLed1 = currentMillis;
    estadoLed1 = !estadoLed1;
    digitalWrite(pinLED1, estadoLed1);
  }
  digitalWrite(pinLED2, LOW);
}

void docking() {
  unsigned long currentMillis = millis();
  digitalWrite(pinLED1, LOW);
  if (currentMillis - ultimoMillisLed2 >= periodoLed2_docking) {
    ultimoMillisLed2 = currentMillis;
    estadoLed2 = !estadoLed2;
    digitalWrite(pinLED2, estadoLed2);
  }
}

void charging() {
  unsigned long currentMillis = millis();
  if (currentMillis - ultimoMillisLed1 >= periodoLed1_charging) {
    ultimoMillisLed1 = currentMillis;
    estadoLed1 = !estadoLed1;
    digitalWrite(pinLED1, estadoLed1);
  }
  if (currentMillis - ultimoMillisLed2 >= periodoLed2_charging) {
    ultimoMillisLed2 = currentMillis;
    estadoLed2 = !estadoLed2;
    digitalWrite(pinLED2, estadoLed2);
  }
}

void averageDistance(){

  const float measurement = getDistance();

  if (measurement <= 0) {
    return;
  }

  distanceAccumulator += measurement;
  distanceIterator++;

  if (distanceIterator < DISTANCE_SAMPLE_WINDOW) {
    return;
  }

  average = distanceAccumulator / distanceIterator;
  distanceIterator = 0;
  distanceAccumulator = 0.0f;

  Serial.println(average);
  sendDistance(average);
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  // Configura os pinos de leitura do estado como entrada
  pinMode(pino_recebe_bit0, INPUT);
  pinMode(pino_recebe_bit1, INPUT);

  pinMode(pino_distancia_bit0, OUTPUT);
  pinMode(pino_distancia_bit1, OUTPUT);

  // Configura os pinos dos LEDs como saída
  pinMode(pinLED1, OUTPUT);
  pinMode(pinLED2, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(LDRPin, INPUT);

  pinMode(DHT11Pin, INPUT);
  pinMode(pino_luminosity_bit, INPUT);;
  pinMod
  pinMode(pino_humidity_bit, INPUT);

  
  sendDistance(distanceThresholdNoObs);
}

void loop() {
  int estado_anterior = estado_recebido;
  averageDistance();
  // PARTE 1: Ler os pinos para descobrir o estado atual
  int bit0 = digitalRead(pino_recebe_bit0);
  int bit1 = digitalRead(pino_recebe_bit1);

  // Reconstrói o número do estado a partir dos bits lidos
  int estado_recebido = bit0 + (bit1 * 2);
  
  // PARTE 2: Executar a ação do LED correspondente ao estado recebido
  switch (estado_recebido) {
    case 0:
      idle();
      break;
    case 1:
      cleaning();
      break;
    case 2:
      docking();
      break;
    case 3:
      charging();
      break;
  }

  if(estado_recebido != estado_anterior){
    sendDistance();
    sendHumidity();
    sendLight();
  }
}