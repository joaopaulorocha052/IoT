#include "DHT.h"

// =================================================================
// == CÓDIGO PARA O SUBSISTEMA SECUNDÁRIO (Arduino Mega)          ==
// =================================================================  
#define DHTPIN 10
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define SOUND_SPEED 0.034
// Pinos para receber o estado do ESP8266
const int pino_recebe_bit0 = 2;
const int pino_recebe_bit1 = 3;

// Pinos para os LEDs
const int pinLED1 = 8;
const int pinLED2 = 9;

// Pinos para o sensor de distancia 
const int trigPin = 5;
const int echoPin = 18;

// Pinos LDR
const int LDRPin = A1;

// Períodos para piscar os LEDs (em milissegundos)
const int periodoLed1_cleaning = 100;
const int periodoLed2_docking = 50;
const int periodoLed1_charging = 100;
const int periodoLed2_charging = 50;

// Variáveis para controle de tempo dos LEDs
unsigned long ultimoMillisLed1 = 0;
unsigned long ultimoMillisLed2 = 0;
int estadoLed1 = LOW;
int estadoLed2 = LOW;

long duration;
float distanceCm;
float distaceAcumulator = 0;
int distanceIterator = 0;
float average;

int distanceMargin = 5;
int distanceThresholdNoObs = 180;
int distanceThresholdFarAway = 100;
int distanceThresholdMedium = 50;
int distanceThresholdClose = 25;

void enviarEstado(int estado, int pin0, int pin1) {
  // Converte o número do estado (0-3) para 2 bits
  int bit0 = estado % 2;      // Se estado for 0 ou 2, bit0=0. Se for 1 ou 3, bit0=1.
  int bit1 = estado / 2;      // Se estado for 0 ou 1, bit1=0. Se for 2 ou 3, bit1=1.

  digitalWrite(pin0, bit0);
  digitalWrite(pin1, bit1);
}

void sendDistance(){
  int state;
  if(fabs(average - distanceThresholdNoObs) > distanceMargin){
    state = 0;
  }
  else if(fabs(average - distanceThresholdFarAway) > distanceMargin){
    state = 1;
  }
  else if(fabs(average - distanceThresholdMedium) > distanceMargin){
    state = 2;
  }
  else if(fabs(average - distanceThresholdClose) > distanceMargin){
    state = 3;
  }

  enviarEstado(state, 6, 7); // 11 e 12 livres

}
 
float get_distance(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  
  return duration * SOUND_SPEED/2;
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

void setup() {
  Serial.begin(115200);

  dht.begin();
  // Configura os pinos de leitura do estado como entrada
  pinMode(pino_recebe_bit0, INPUT);
  pinMode(pino_recebe_bit1, INPUT);

  // Configura os pinos dos LEDs como saída
  pinMode(pinLED1, OUTPUT);
  pinMode(pinLED2, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(LDRPin, INPUT);
}

void averageDistance(){

  if(distanceIterator < 10){
    distanceIterator++;
    distaceAcumulator += get_distance();
  } 
  else {
    Serial.println(distaceAcumulator/distanceIterator);
    average = distaceAcumulator/distanceIterator;
    distanceIterator = 0;
    distaceAcumulator = 0;
 
  }
  
}
void loop() {
  averageDistance();
  // PARTE 1: Ler os pinos para descobrir o estado atual
  int bit0 = digitalRead(pino_recebe_bit0);
  int bit1 = digitalRead(pino_recebe_bit1);

  // Reconstrói o número do estado a partir dos bits lidos
  int estado_recebido = bit0 + (bit1 * 2);

  int LDRinput = analogRead(LDRPin);
  
//  Serial.println("Leitura do LDR:");
//  Serial.println(LDRinput);
    float humidade = dht.readHumidity();
    float tempC = dht.readTemperature();
    Serial.println("Leitura de humidade");
    Serial.println(humidade);
  
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
}