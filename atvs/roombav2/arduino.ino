// =================================================================
// == CÓDIGO PARA O SUBSISTEMA SECUNDÁRIO (Arduino Mega)          ==
// =================================================================

// Pinos para receber o estado do ESP8266
const int pino_recebe_bit0 = 2;
const int pino_recebe_bit1 = 3;

// Pinos para os LEDs
const int pinLED1 = 8;
const int pinLED2 = 9;

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
  // Configura os pinos de leitura do estado como entrada
  pinMode(pino_recebe_bit0, INPUT);
  pinMode(pino_recebe_bit1, INPUT);

  // Configura os pinos dos LEDs como saída
  pinMode(pinLED1, OUTPUT);
  pinMode(pinLED2, OUTPUT);
}

void loop() {
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
}