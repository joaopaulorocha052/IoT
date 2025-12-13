# Projeto Final - Carrinho com Controle de Distância Adaptativo

Projeto da disciplina de Internet das Coisas (IoT) do curso de Engenharia de Computação da Universidade Federal de São Paulo (UNIFESP).

## 📋 Ideia Principal

Desenvolver um sistema de **Controle de Cruzeiro Adaptativo**, capaz de manter uma distância segura de objetos em sua frente. A telemetria de dados do carrinho mantém os dados de distância à frente, inclinação e velocidade do carrinho, sendo exibidos em um cliente externo (Flespi). Além disso, a distância será definida também por meio da interface pelo usuário.

## 🔧 Componentes de Hardware

| Componente | Descrição |
|------------|-----------|
| ESP32 | Microcontrolador principal |
| HC-SR04 | Sensor ultrassônico para medir distância |
| MPU-6050 | Giroscópio/Acelerômetro para telemetria de inclinação |
| L298N | Driver para controle de 2 motores DC |
| 2x Motor DC | Motores do carrinho |
| LED | Indicador de status/erro |

## 📁 Estrutura do Projeto

```
Projeto Final/
├── README.md
├── src/
│   └── main/
│       ├── main.ino           # Arquivo principal
│       ├── config.h           # Configurações parametrizáveis
│       ├── sensors.h          # Header dos sensores
│       ├── sensors.cpp        # Implementação dos sensores
│       ├── motor_control.h    # Header do controle de motores
│       ├── motor_control.cpp  # Implementação do controle + PID
│       ├── mqtt_handler.h     # Header da comunicação MQTT
│       └── mqtt_handler.cpp   # Implementação MQTT
└── tests/
    ├── distance/              # Teste do sensor ultrassônico
    ├── gyroscope/             # Teste do MPU-6050
    └── on_car/                # Teste do driver de motor L298N
```

## ⚙️ Configuração

Edite o arquivo `src/main/config.h` para configurar:

### WiFi
```cpp
#define WIFI_SSID           "SEU_SSID"
#define WIFI_PASSWORD       "SUA_SENHA"
```

### Flespi MQTT
```cpp
#define MQTT_BROKER         "mqtt.flespi.io"
#define MQTT_PORT           1883
#define MQTT_TOKEN          "SEU_TOKEN_FLESPI"
```

### Pinos do ESP32
```cpp
// Sensor Ultrassônico
#define PIN_TRIG            5
#define PIN_ECHO            18

// MPU-6050 (I2C)
#define PIN_SDA             21
#define PIN_SCL             22

// Motor Driver L298N
#define PIN_IN1             25
#define PIN_IN2             26
#define PIN_ENA             32
#define PIN_IN3             27
#define PIN_IN4             14
#define PIN_ENB             33

// LED de Status
#define PIN_LED_STATUS      2
```

### Parâmetros PID (Calibração)
```cpp
#define PID_KP_INICIAL      2.0
#define PID_KI_INICIAL      0.5
#define PID_KD_INICIAL      1.0
```

## 📡 Tópicos MQTT

### Telemetria (ESP32 → Flespi)

| Tópico | Payload | Descrição |
|--------|---------|-----------|
| `carrinho/telemetria/distancia` | `42.5` | Distância atual em cm |
| `carrinho/telemetria/velocidade` | `180` | Velocidade PWM (-255 a 255) |
| `carrinho/telemetria/inclinacao` | `{"ax":..., "gx":...}` | Dados do IMU em JSON |
| `carrinho/telemetria/status` | `"ativo"` ou `"inativo"` | Estado do controle |

### Comandos (Flespi → ESP32)

| Tópico | Payload | Descrição |
|--------|---------|-----------|
| `carrinho/comando/setpoint` | `30.0` | Define distância desejada (cm) |
| `carrinho/comando/controle` | `"on"` ou `"off"` | Ativa/Desativa controle |

## 🚗 Funcionamento

1. **Leitura de Sensores**: O ESP32 lê continuamente a distância (HC-SR04) e inclinação (MPU-6050)

2. **Controle PID**: Calcula a velocidade necessária para manter a distância desejada
   - Erro positivo → Carrinho muito longe → Acelera para frente
   - Erro negativo → Carrinho muito perto → Vai para trás
   - Erro ≈ 0 → Para (dentro da tolerância)

3. **Telemetria**: Envia dados via MQTT para o Flespi periodicamente

4. **Comandos**: Recebe setpoint e controle on/off via MQTT

5. **Segurança**: Se perder conexão WiFi/MQTT, o carrinho **para imediatamente** e o LED acende

## 📊 Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                         FLESPI                               │
│                    (Broker MQTT)                             │
│  ┌─────────────────┐          ┌─────────────────────────┐   │
│  │   TELEMETRIA    │          │       COMANDOS          │   │
│  │ - Distância     │          │ - Setpoint              │   │
│  │ - Velocidade    │          │ - Controle on/off       │   │
│  │ - Inclinação    │          └─────────────────────────┘   │
│  │ - Status        │                                         │
│  └─────────────────┘                                         │
└─────────────────────────────────────────────────────────────┘
              ▲                           │
              │ WiFi (hotspot celular)    │
              │                           ▼
┌─────────────────────────────────────────────────────────────┐
│                         ESP32                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │  HC-SR04     │  │   MPU-6050   │  │  Controlador PID │   │
│  │  (Distância) │  │ (Inclinação) │  │  (Kp, Ki, Kd)    │   │
│  └──────────────┘  └──────────────┘  └──────────────────┘   │
│                                              │               │
│                                              ▼               │
│                           ┌──────────────────────────┐       │
│                           │         L298N            │       │
│                           │      Motor Driver        │       │
│                           └──────────────────────────┘       │
│                                    │       │                 │
│                               Motor A   Motor B              │
└─────────────────────────────────────────────────────────────┘
```

## 🔌 Conexões

### Sensor Ultrassônico HC-SR04
| HC-SR04 | ESP32 |
|---------|-------|
| VCC | 5V |
| GND | GND |
| TRIG | GPIO 5 |
| ECHO | GPIO 18 |

### MPU-6050
| MPU-6050 | ESP32 |
|----------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Driver L298N
| L298N | ESP32 |
|-------|-------|
| IN1 | GPIO 25 |
| IN2 | GPIO 26 |
| IN3 | GPIO 27 |
| IN4 | GPIO 14 |
| ENA | GPIO 32 |
| ENB | GPIO 33 |

## 📚 Bibliotecas Necessárias

- `WiFi.h` (incluída no ESP32)
- `Wire.h` (incluída no ESP32)
- `PubSubClient` (instalar via Arduino IDE)

## 🚀 Como Usar

1. Instale a biblioteca `PubSubClient` no Arduino IDE
2. Configure as credenciais WiFi e Flespi em `config.h`
3. Ajuste os pinos conforme seu hardware
4. Faça upload para o ESP32
5. No Flespi, envie comandos para os tópicos:
   - `carrinho/comando/setpoint` → distância desejada
   - `carrinho/comando/controle` → `"on"` para ativar

## 🔧 Calibração do PID

Os valores iniciais do PID são:
- **Kp = 2.0**: Resposta proporcional ao erro
- **Ki = 0.5**: Elimina erro em regime permanente
- **Kd = 1.0**: Suaviza a resposta

Para calibrar, ajuste as variáveis globais no `main.ino`:
```cpp
float Kp = 2.0;  // Aumentar = resposta mais agressiva
float Ki = 0.5;  // Aumentar = corrige erro acumulado mais rápido
float Kd = 1.0;  // Aumentar = mais amortecimento
```

## 📝 Licença

Projeto acadêmico - UNIFESP 2025

