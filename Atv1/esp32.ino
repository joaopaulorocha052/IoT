#include "esp32.h"

int aplicarNivelBateria(int valor) {
  valor = constrain(valor, 0, 255);
  dacWrite(pino_led_bateria, valor);
  return valor;
}

// Nova função para enviar o estado atual via GPIOs
void enviarEstado(int estado) {
  // Converte o número do estado (0-3) para 2 bits
  int bit0 = estado % 2;      // Se estado for 0 ou 2, bit0=0. Se for 1 ou 3, bit0=1.
  int bit1 = estado / 2;      // Se estado for 0 ou 1, bit1=0. Se for 2 ou 3, bit1=1.

  digitalWrite(pino_estado_bit0, bit0);
  digitalWrite(pino_estado_bit1, bit1);
}

/* Talvez seja preciso modificar o default desses inputs*/
void receiveDistance(){
  int distance_bit1 = digitalRead(pino_distancia_bit1);
  int distance_bit2 = digitalRead(pino_distancia_bit2);

  if(distance_bit1 == 1 && distance_bit2 == 1){
    Serial.println("Distância: Muito Perto");
  } else if(distance_bit1 == 0 && distance_bit2 == 1){
    Serial.println("Distância: Médio");
  } else if(distance_bit1 == 1 && distance_bit2 == 0){
    Serial.println("Distância: Longe");
  } else {
    Serial.println("Distância: Sem Obstrução");
  }
}

void receiveHumidity(){
  int humidity_bit = digitalRead(pino_humidity_bit);

  if(humidity_bit == 1){
    Serial.println("Humidade: Alta");
  } else {
    Serial.println("Humidade: Normal");
  }
}

void receiveLuminosity(){
  int light_bit = digitalRead(pino_luminosity_bit);

  if(light_bit == 1){
    Serial.println("Luminosidade: Alta");
  } else {
    Serial.println("Luminosidade: Baixa");
  }
}

void receiveData(){

  if ((millis() - tempo_leitura >= 1000)) {
    tempo_leitura = millis();
    receiveDistance();
    receiveHumidity();
    receiveLuminosity();
  }

}

void setup() {
  Serial.begin(115200);

  pinMode(pino_estado_bit0, OUTPUT);
  pinMode(pino_estado_bit1, OUTPUT);
  pinMode(pino_led_bateria, OUTPUT);
  pinMode(pino_luminosity_bit, INPUT);
  pinMode(pino_humidity_bit, INPUT);
  pinMode(pino_distancia_bit1, INPUT);
  pinMode(pino_distancia_bit2, INPUT);

  tempoEntradaEstado = millis();
  ultimoFadeOut = tempoEntradaEstado;
  ultimoFadeIn = tempoEntradaEstado;
  dacNivelFadeOut = aplicarNivelBateria(255);
  dacNivelFadeIn = 0;
  enviarEstado(estado_maquina);

  Serial.println("Subsistema Principal Iniciado.");
  Serial.println("Envie 'a' para iniciar a limpeza.");
}

void loop() {
  int estado_anterior = estado_maquina;

  // PARTE 1: Verificação das regras de transição (lógica principal)
  char input_serial = 0;
  if (Serial.available()) {
    input_serial = Serial.read();
  }

  receiveData();

  switch (estado_maquina) {
    case 0: // Transições a partir de Idle
      dacNivelFadeOut = aplicarNivelBateria(255);
      dacNivelFadeIn = 0;
      if (input_serial == 'a') {
        estado_maquina = 1;
        tempoEntradaEstado = millis();
        dacNivelFadeOut = 255;
        Serial.println("Estado: Cleaning");
      }
      break;

    case 1: // Transições a partir de Cleaning
      
      if(millis() - ultimoFadeOut >= intervalo_fade_bateria) {
        ultimoFadeOut = millis();
        dacNivelFadeOut = aplicarNivelBateria(dacNivelFadeOut - fade_step);
      }
      if ((millis() - tempoEntradaEstado >= cleaning_para_docking_delay) || (input_serial == 'b')) {
        estado_maquina = 2;
        tempoEntradaEstado = millis();
        Serial.println("Estado: Docking");
        dacNivelFadeOut = 255;
        aplicarNivelBateria(0);
      }
      break;

    case 2: // Transições a partir de Docking
      aplicarNivelBateria(0);
      if (millis() - tempoEntradaEstado >= docking_para_charging_delay) {
        estado_maquina = 3;
        tempoEntradaEstado = millis();
        ultimoFadeIn = tempoEntradaEstado;
        dacNivelFadeIn = 0;
        Serial.println("Estado: Charging");
      }
      break;

    case 3: // Transições a partir de Charging
      // O diagrama mais recente indica que 'a' vai para Cleaning
      if(millis() - ultimoFadeIn >= intervalo_fade_bateria) {
        ultimoFadeIn = millis();
        dacNivelFadeIn = aplicarNivelBateria(dacNivelFadeIn + fade_step);
      }
      if (input_serial == 'a') {
        estado_maquina = 1;
        tempoEntradaEstado = millis();
        dacNivelFadeIn = 0;
        dacNivelFadeOut = 255;
        Serial.println("Estado: Cleaning (comando manual 'a' recebido)");
      } else if (millis() - tempoEntradaEstado >= charging_para_idle_delay) {
        estado_maquina = 0; // Timeout leva para Idle
        Serial.println("Estado: Idle (timeout atingido)");
        dacNivelFadeIn = 0;
      }
      break;
  }

  // PARTE 2: Envia o estado para o subsistema secundário se houver mudança
  if (estado_maquina != estado_anterior) {
    enviarEstado(estado_maquina);
  }
}