/*
 * ============================================================================
 * ARQUIVO DE CONFIGURAÇÃO - CARRINHO COM CONTROLE DE DISTÂNCIA
 * ============================================================================
 * Todos os parâmetros configuráveis do projeto estão centralizados aqui.
 * Modifique conforme necessário para seu hardware e ambiente.
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// CONFIGURAÇÕES DE REDE WIFI
// ============================================================================
#define WIFI_SSID           "Fernando WiFi"
#define WIFI_PASSWORD       "abracadabra2"
#define WIFI_TIMEOUT_MS     10000       // Tempo máximo para conectar (ms)
#define WIFI_RETRY_DELAY_MS 5000        // Intervalo entre tentativas de reconexão

// ============================================================================
// CONFIGURAÇÕES FLESPI MQTT
// ============================================================================
#define MQTT_BROKER         "mqtt.flespi.io"
#define MQTT_PORT           1883
#define MQTT_TOKEN          "2M3aWYXKgNvgCUlsycMgM5emxKcsWoOSnpj8fBm3vMIzVwBsVilDQ5lEWjKzRDxw"  // Token vai no campo username
#define MQTT_PASSWORD       ""                   // Flespi não usa password
#define MQTT_CLIENT_ID      "esp32_joaopaulo"

// ============================================================================
// TÓPICOS MQTT - TELEMETRIA (ESP32 -> Flespi)
// ============================================================================
#define TOPIC_TELEMETRIA_DISTANCIA   "/projeto-final/adas/telemetria/distancia"
#define TOPIC_TELEMETRIA_VELOCIDADE  "/projeto-final/adas/telemetria/velocidade"
#define TOPIC_TELEMETRIA_INCLINACAO  "/projeto-final/adas/telemetria/inclinacao"
#define TOPIC_TELEMETRIA_STATUS      "/projeto-final/adas/telemetria/status"

// ============================================================================
// TÓPICOS MQTT - COMANDOS (Flespi -> ESP32)
// ============================================================================
#define TOPIC_COMANDO_SETPOINT       "/projeto-final/adas/conf/distancia"
#define TOPIC_COMANDO_CONTROLE       "/projeto-final/adas/conf/estado"

// ============================================================================
// PINOS ESP32 - SENSOR ULTRASSÔNICO HC-SR04
// ============================================================================
#define PIN_TRIG            5
#define PIN_ECHO            18

// ============================================================================
// PINOS ESP32 - MPU-6050 (I2C)
// ============================================================================
#define PIN_SDA             21
#define PIN_SCL             22
#define MPU_ADDR            0x68        // Endereço I2C do MPU-6050

// ============================================================================
// PINOS ESP32 - DRIVER DE MOTOR L298N
// ============================================================================
// Motor A (Esquerdo)
#define PIN_IN1             25
#define PIN_IN2             26
#define PIN_ENA             32

// Motor B (Direito)
#define PIN_IN3             27
#define PIN_IN4             14
#define PIN_ENB             33

// ============================================================================
// PINO DO LED DE STATUS (Erro/Desconexão)
// ============================================================================
#define PIN_LED_STATUS      2           // LED embutido do ESP32

// ============================================================================
// CONFIGURAÇÕES PWM DOS MOTORES
// ============================================================================
#define PWM_FREQUENCY       5000        // Frequência PWM em Hz
#define PWM_RESOLUTION      8           // Resolução em bits (8 = 0-255)
#define PWM_CHANNEL_A       0           // Canal PWM para motor A
#define PWM_CHANNEL_B       1           // Canal PWM para motor B

// ============================================================================
// LIMITES DE VELOCIDADE (PWM)
// ============================================================================
#define PWM_MIN             0
#define PWM_MAX             255
#define PWM_MIN_MOVIMENTO   50          // PWM mínimo para o motor se mover

// ============================================================================
// PARÂMETROS DO SENSOR ULTRASSÔNICO
// ============================================================================
#define SOUND_SPEED         0.034       // Velocidade do som em cm/us
#define DISTANCIA_MAX       400.0       // Distância máxima válida (cm)
#define DISTANCIA_MIN       2.0         // Distância mínima válida (cm)
#define NUM_AMOSTRAS        10          // Número de amostras para média móvel

// ============================================================================
// PARÂMETROS DE CONTROLE
// ============================================================================
#define DISTANCIA_PADRAO    30.0        // Setpoint inicial (cm)
#define DISTANCIA_TOLERANCIA 2.0        // Tolerância para considerar "na posição" (cm)

// ============================================================================
// INTERVALOS DE TEMPO (ms)
// ============================================================================
#define INTERVALO_TELEMETRIA    500     // Frequência de envio de telemetria
#define INTERVALO_SENSOR        100     // Frequência de leitura dos sensores
#define INTERVALO_PID           100     // Frequência de atualização do PID
#define INTERVALO_MQTT_RECONNECT 5000   // Intervalo para tentar reconectar MQTT

// ============================================================================
// PARÂMETROS PID (AJUSTAR CONFORME CALIBRAÇÃO)
// ============================================================================
// Estes valores são iniciais e devem ser calibrados para seu carrinho
// Kp: Ganho proporcional - resposta ao erro atual
// Ki: Ganho integral - elimina erro em regime permanente
// Kd: Ganho derivativo - suaviza a resposta, reduz overshoot

#define PID_KP_INICIAL      2.0
#define PID_KI_INICIAL      0.5
#define PID_KD_INICIAL      1.0

// Limites do termo integral (anti-windup)
#define PID_INTEGRAL_MAX    100.0
#define PID_INTEGRAL_MIN    -100.0

#endif // CONFIG_H
