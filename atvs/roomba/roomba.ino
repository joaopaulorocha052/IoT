// Pinos para os LEDs no NodeMCU
const int pinLED1 = 27;
const int pinLED2 = 32;

// Períodos para piscar os LEDs
const int periodoLed1_cleaning = 100;
const int periodoLed2_docking = 50;
const int periodoLed1_charging = 100;
const int periodoLed2_charging = 50;

// Tempos para transições de estado automáticas
const unsigned long cleaning_para_docking_delay = 2000;
const unsigned long docking_para_charging_delay = 2000;
const unsigned long charging_para_idle_delay = 2000; // Usei o mostrado na atividade 4.2, a Roomba Subsistemas

int estado_maquina = 0; // 0: Idle, 1: Cleaning, 2: Docking, 3: Charging

// Variáveis para controle de tempo dos LEDs (independentes)
unsigned long ultimoMillisLed1 = 0;
unsigned long ultimoMillisLed2 = 0;
int estadoLed1 = LOW;
int estadoLed2 = LOW;

// Variável para controle de tempo das transições de estado
unsigned long tempoEntradaEstado = 0;

// --- Funções de Estado (Ações) ---
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
  pinMode(pinLED1, OUTPUT);
  pinMode(pinLED2, OUTPUT);
  digitalWrite(pinLED1, LOW);
  digitalWrite(pinLED2, LOW);
  
  Serial.println("Robô Aspirador Iniciado. Estado: Idle");
  Serial.println("Envie 'a' para iniciar a limpeza.");
}

void loop() {
  // PARTE 1: Execução das ações do estado atual
  switch (estado_maquina) {
    case 0: idle(); break;
    case 1: cleaning(); break;
    case 2: docking(); break;
    case 3: charging(); break;
  }

  // PARTE 2: Verificação das regras de transição
  char input_serial = 0;
  if(Serial.available()){
    input_serial = Serial.read();
  }

  switch (estado_maquina) {
    case 0: // Transições a partir de Idle
      if (input_serial == 'a') {
        estado_maquina = 1;
        tempoEntradaEstado = millis();
        Serial.println("Estado: Cleaning");
      }
      break;

    case 1: // Transições a partir de Cleaning
      if ((millis() - tempoEntradaEstado >= cleaning_para_docking_delay) || (input_serial == 'b')) {
        estado_maquina = 2;
        tempoEntradaEstado = millis();
        Serial.println("Estado: Docking");
      }
      break;

    case 2: // Transições a partir de Docking
      if (millis() - tempoEntradaEstado >= docking_para_charging_delay) {
        estado_maquina = 3;
        tempoEntradaEstado = millis(); // O timer para o estado Charging começa aqui
        Serial.println("Estado: Charging");
      }
      break;

    case 3: // Transições a partir de Charging
      if ((input_serial == 'a') || (millis() - tempoEntradaEstado >= charging_para_idle_delay)) {
        if (input_serial == 'a') {
            Serial.println("Estado: Cleaning (comando manual 'a' recebido)");
            estado_maquina = 1;

        } else {
            Serial.println("Estado: Idle (timeout de 5 segundos atingido)");
            estado_maquina = 0;
        }
      }
      break;
  }
}
