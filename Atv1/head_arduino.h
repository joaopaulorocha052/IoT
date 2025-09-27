#ifndef HEAD_ARDUINO_H
#define HEAD_ARDUINO_H

#include <Arduino.h>

#define SOUND_SPEED 0.034
#define pino_distancia_bit0 6
#define pino_distancia_bit1 7
#define pino_luminosity_bit 4
#define pino_humidity_bit 5

#define DHTPIN 10
#define DHTTYPE DHT11

#define pino_recebe_bit0 2
#define pino_recebe_bit1 3

// Pinos para os LEDs
#define pinLED1 8
#define pinLED2 9

// Pinos para o sensor de distancia
#define trigPin 5
#define echoPin 18

// Pinos LDR
#define LDRPin A1

// Pino DHT11
#define DHT11Pin 10

// Períodos para piscar os LEDs (em milissegundos)
// Precisará ser mudado (aumentado)
#define periodoLed1_cleaning 100
#define periodoLed2_docking 50
#define periodoLed1_charging 100
#define periodoLed2_charging 50

// Parâmetros das distâncias
#define distanceMargin 5.0f
#define distanceThresholdNoObs 180
#define distanceThresholdFarAway 100
#define distanceThresholdMedium 50
#define distanceThresholdClose 25

// Parâmetros dos níveis de luminosidade
#define threshold_obstacle 500

// Parâmetros dos níveis de humidade
#define threshold_humidity 100

#endif // HEAD_ARDUINO_H
