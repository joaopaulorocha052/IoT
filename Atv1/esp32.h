#ifndef ESP32_H
#define ESP32_H

// Pinos para comunicar o estado para o Arduino (2 bits)
#define pino_estado_bit0 5 // Envia o bit 0 do estado
#define pino_estado_bit1 6 // Envia o bit 1 do estado
#define pino_led_bateria 25 // Saida pra o indicador da bateria
#define pino_luminosity_bit 3
#define pino_humidity_bit 2
#define pino_distancia_bit1 6
#define pino_distancia_bit2 7


// Tempos para transições de estado automáticas (em milissegundos)
// Aumentar um pouco depois que fizermos os testes
unsigned long cleaning_para_docking_delay = 2000;
unsigned long docking_para_charging_delay = 2000;
unsigned long charging_para_idle_delay = 2000;
unsigned long intervalo_fade_bateria = 10;
int fade_step = 10;

// Variáveis de estado
int estado_maquina = 0; // 0: Idle, 1: Cleaning, 2: Docking, 3: Charging
int estado_anterior = 0;

// Variável para controle de tempo das transições de estado
unsigned long tempoEntradaEstado = 0;
unsigned long ultimoFadeOut = 0;
unsigned long ultimoFadeIn = 0;

// Variavel para controle da exibição de Dados
unsigned long tempo_leitura = 0;

int dacNivelFadeOut = 255;
int dacNivelFadeIn = 0;

#endif // ESP32_H