#ifndef CONFIG_H
#define CONFIG_H


// CONFIGURAÇÕES DE REDE WIFI
#define WIFI_SSID           "wifi"
#define WIFI_PASSWORD       "senha"
#define WIFI_TIMEOUT_MS     10000       // Tempo máximo para conectar (ms)
#define WIFI_RETRY_DELAY_MS 5000        // Intervalo entre tentativas de reconexão


// CONFIGURAÇÕES FLESPI MQTT
#define MQTT_BROKER         "mqtt.flespi.io"
#define MQTT_PORT           1883
#define MQTT_TOKEN          "token"         // Token vai no campo username
#define MQTT_PASSWORD       ""              // Flespi não usa password
#define MQTT_CLIENT_ID      "client"


// TÓPICOS MQTT - TELEMETRIA (ESP32 -> Flespi)
#define TOPIC_TELEMETRIA_DISTANCIA              "/projeto-final/adas/telemetria/distancia"
#define TOPIC_TELEMETRIA_VELOCIDADE             "/projeto-final/adas/telemetria/velocidade"
#define TOPIC_TELEMETRIA_INCLINACAO             "/projeto-final/adas/telemetria/inclinacao"
#define TOPIC_TELEMETRIA_STATUS                 "/projeto-final/adas/telemetria/status"
#define TOPIC_TELEMETRIA_BATERIA                "/projeto-final/adas/telemetria/bateria"
#define TOPIC_TELEMETRIA_THRESHOLD_DENTRO       "/projeto-final/adas/telemetria/dentro_threshold"
#define TOPIC_TELEMETRIA_THRESHOLD_FORA         "/projeto-final/adas/telemetria/fora_threshold"
#define TOPIC_TELEMETRIA_ACCEL                  "/projeto-final/adas/telemetria/aceleracao"
#define TOPIC_TELEMETRIA_TOUCH                  "/projeto-final/adas/telemetria/touch"

// TÓPICOS MQTT - COMANDOS (Flespi -> ESP32)
#define TOPIC_COMANDO_SETPOINT                  "/projeto-final/adas/conf/distancia"
#define TOPIC_COMANDO_CONTROLE                  "/projeto-final/adas/conf/estado"


// PINOS ESP32 - SENSOR ULTRASSÔNICO HC-SR04
#define PIN_TRIG            5
#define PIN_ECHO            18


// PINOS ESP32 - BATERIA
#define PIN_BATERIA        15          // Pino ADC para leitura da bateria (GPIO34)


// CONFIGURAÇÕES DA BATERIA; lógica legada pois calculamos a porcentagem direto no broker MQTT
// Configuração do divisor de tensão (R1 = resistor superior, R2 = resistor inferior)
// Tensão_bateria = Tensão_ADC * (R1 + R2) / R2
#define BATERIA_R1          2000.0    // 2kΩ (resistor superior)
#define BATERIA_R2          1000.0     // 1kΩ (resistor inferior)
                                       // Divisor: ~3.0 (permite medir até ~13.2V)

// Limites de tensão da bateria (ajustar conforme tipo de bateria)
// Valores típicos para bateria 2S LiPo (7.4V nominal):
#define BATERIA_TENSAO_MAX  8.0        // Tensão de 100% (totalmente carregada)
#define BATERIA_TENSAO_MIN  6.0        // Tensão de 0% (descarregada)

// Limiares de alerta
#define BATERIA_LIMIAR_BAIXO    30.0   // % - Aviso de bateria baixa
#define BATERIA_LIMIAR_CRITICO  15.0   // % - Bateria crítica

// Número de amostras para média móvel
#define BATERIA_NUM_AMOSTRAS    20     // Mais amostras = leitura mais estável
// ================ FIM CONFIGURAÇÕES DA BATERIA ================

#define PIN_TOUCH_SENSOR 15

// PINOS ESP32 - MPU-6050 (I2C)
#define PIN_SDA             21
#define PIN_SCL             22
#define MPU_ADDR            0x68        // Endereço I2C do MPU-6050


// PINOS ESP32 - DRIVER DE MOTOR L298N
// Motor A (Esquerdo)
#define PIN_IN1             25
#define PIN_IN2             26
#define PIN_ENA             32
// Motor B (Direito)
#define PIN_IN3             27
#define PIN_IN4             14
#define PIN_ENB             33


// PINO DO LED DE STATUS (Erro/Desconexão)
#define PIN_LED_STATUS      2           // LED embutido do ESP32

// PINO DO LED DE STATUS (Erro/Desconexão)
#define PIN_LED_INSIDE_THRESHOLD      4           // LED embutido do ESP32

// CONFIGURAÇÕES PWM DOS MOTORES
#define PWM_FREQUENCY       5000        // Frequência PWM em Hz
#define PWM_RESOLUTION      8           // Resolução em bits (8 = 0-255)
#define PWM_CHANNEL_A       0           // Canal PWM para motor A
#define PWM_CHANNEL_B       1           // Canal PWM para motor B

// LIMITES DE VELOCIDADE (PWM)
#define PWM_MIN             0
#define PWM_MAX             255
#define PWM_MIN_MOVIMENTO   50          // PWM mínimo para o motor se mover

// PARÂMETROS DO SENSOR ULTRASSÔNICO
#define SOUND_SPEED         0.034       // Velocidade do som em cm/us
#define DISTANCIA_MAX       400.0       // Distância máxima válida (cm)
#define DISTANCIA_MIN       2.0         // Distância mínima válida (cm)
#define NUM_AMOSTRAS        10          // Número de amostras para média móvel

// PARÂMETROS DE CONTROLE
#define DISTANCIA_PADRAO    30.0        // Setpoint inicial (cm)
#define DISTANCIA_TOLERANCIA 2.0        // Tolerância para considerar "na posição" (cm)
#define INSIDE_THRESHOLD    4.0         // Limiar para LED indicar "dentro da zona"

// INTERVALOS DE TEMPO (ms)
#define INTERVALO_TELEMETRIA    500     // Frequência de envio de telemetria
#define INTERVALO_SENSOR        100     // Frequência de leitura dos sensores
#define INTERVALO_PID           100     // Frequência de atualização do PID
#define INTERVALO_MQTT_RECONNECT 5000   // Intervalo para tentar reconectar MQTT

// PARÂMETROS PID
// IMPORTANTE! Estes valores foram calibrados empiricamente para o sistema
// Kp: Ganho proporcional - resposta ao erro atual
// Ki: Ganho integral - elimina erro em regime permanente
// Kd: Ganho derivativo - suaviza a resposta, reduz overshoot
#define PID_KP_INICIAL      2.0
#define PID_KI_INICIAL      0.5
#define PID_KD_INICIAL      1.0

// Limites do termo integral (anti-windup); usado apenas se Ki > 0
#define PID_INTEGRAL_MAX    100.0
#define PID_INTEGRAL_MIN    -100.0

#endif // CONFIG_H
