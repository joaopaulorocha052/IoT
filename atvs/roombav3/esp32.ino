// =================================================================
// == CÓDIGO PARA O SUBSISTEMA PRINCIPAL (ESP8266 / NodeMCU)      ==
// =================================================================

// Pinos para comunicar o estado para o Arduino (2 bits)
const int pino_estado_bit0 = D5; // Envia o bit 0 do estado
const int pino_estado_bit1 = D6; // Envia o bit 1 do estado
const int pino_led_bateria = D25; // Saida pra o indicador da bateria

// Tempos para transições de estado automáticas (em milissegundos)
const unsigned long cleaning_para_docking_delay = 2000;
const unsigned long docking_para_charging_delay = 2000;
const unsigned long charging_para_idle_delay = 2000;
const unsigned long intervalo_fade_bateria = 10;

// Variáveis de estado
int estado_maquina = 0; // 0: Idle, 1: Cleaning, 2: Docking, 3: Charging

// Variável para controle de tempo das transições de estado
unsigned long tempoEntradaEstado = 0;
unsigned long ultimoFadeOut = 0;
unsigned long ultimoFadeIn = 0;

int dacNivelFadeOut = 255;
int dacNivelFadeIn = 0;

// Nova função para enviar o estado atual via GPIOs
void enviarEstado(int estado) {
  // Converte o número do estado (0-3) para 2 bits
  int bit0 = estado % 2;      // Se estado for 0 ou 2, bit0=0. Se for 1 ou 3, bit0=1.
  int bit1 = estado / 2;      // Se estado for 0 ou 1, bit1=0. Se for 2 ou 3, bit1=1.

  digitalWrite(pino_estado_bit0, bit0);
  digitalWrite(pino_estado_bit1, bit1);
}

void setup() {
  Serial.begin(115200);

  pinMode(pino_estado_bit0, OUTPUT);
  pinMode(pino_estado_bit1, OUTPUT);

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

  switch (estado_maquina) {
    case 0: // Transições a partir de Idle
      dacWrite(pino_led_bateria, 255);
      if (input_serial == 'a') {
        estado_maquina = 1;
        tempoEntradaEstado = millis();
        Serial.println("Estado: Cleaning");
      }
      break;

    case 1: // Transições a partir de Cleaning
      
      if(millis() - ultimoFadeOut >= intervalo_fade_bateria) {
        dacNivelFadeOut = dacNivelFadeOut - 10;
        dacWrite(pino_led_bateria, dacNivelFadeOut);
        ultimoFadeOut = millis();
      }
      if ((millis() - tempoEntradaEstado >= cleaning_para_docking_delay) || (input_serial == 'b')) {
        estado_maquina = 2;
        tempoEntradaEstado = millis();
        Serial.println("Estado: Docking");
        dacNivelFadeOut = 255;
      }
      break;

    case 2: // Transições a partir de Docking
      dacWrite(pino_led_bateria, 0);
      if (millis() - tempoEntradaEstado >= docking_para_charging_delay) {
        estado_maquina = 3;
        tempoEntradaEstado = millis();
        Serial.println("Estado: Charging");
      }
      break;

    case 3: // Transições a partir de Charging
      // O diagrama mais recente indica que 'a' vai para Cleaning
      if(millis() - ultimoFadeIn >= intervalo_fade_bateria) {
        dacNivelFadeIn = dacNivelFadeIn + 10;
        dacWrite(pino_led_bateria, dacNivelFadeIn);
        ultimoFadeIn = millis();
      }
      if (input_serial == 'a') {
        estado_maquina = 1;
        tempoEntradaEstado = millis();
        dacNivelFadeIn = 0;
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