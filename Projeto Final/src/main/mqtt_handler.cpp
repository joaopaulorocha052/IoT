/*
 * ============================================================================
 * IMPLEMENTAÇÃO DO MÓDULO DE COMUNICAÇÃO MQTT
 * ============================================================================
 */

#include "mqtt_handler.h"
#include "config.h"

// Instância estática para uso no callback
GerenciadorMQTT* GerenciadorMQTT::_instancia = nullptr;

// ============================================================================
// IMPLEMENTAÇÃO
// ============================================================================

GerenciadorMQTT::GerenciadorMQTT() : _mqttClient(_wifiClient) {
    _estado = DESCONECTADO;
    _ultimaTentativaReconexao = 0;
    _callbackSetpoint = nullptr;
    _callbackControle = nullptr;
    _instancia = this;
}

void GerenciadorMQTT::inicializar() {
    // Configura servidor MQTT
    _mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    _mqttClient.setCallback(callbackMQTT);
    
    Serial.println("[MQTT] Gerenciador MQTT inicializado");
    Serial.print("[MQTT] Broker: ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_PORT);
}

void GerenciadorMQTT::loop() {
    // Verifica conexão WiFi
    if (!isWiFiConectado()) {
        _estado = DESCONECTADO;
        return;
    }
    
    // Verifica conexão MQTT
    if (!isMQTTConectado()) {
        unsigned long agora = millis();
        if (agora - _ultimaTentativaReconexao >= INTERVALO_MQTT_RECONNECT) {
            _ultimaTentativaReconexao = agora;
            reconectar();
        }
        return;
    }
    
    // Processa mensagens MQTT
    _mqttClient.loop();
}

bool GerenciadorMQTT::conectarWiFi() {
    Serial.println("[WIFI] Conectando ao WiFi...");
    Serial.print("[WIFI] SSID: ");
    Serial.println(WIFI_SSID);
    
    _estado = CONECTANDO_WIFI;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - inicio > WIFI_TIMEOUT_MS) {
            Serial.println("\n[WIFI] Falha na conexão - Timeout");
            _estado = ERRO_CONEXAO;
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.println("[WIFI] Conectado!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    _estado = WIFI_CONECTADO;
    return true;
}

bool GerenciadorMQTT::conectarMQTT() {
    if (!isWiFiConectado()) {
        Serial.println("[MQTT] WiFi não conectado");
        return false;
    }
    
    Serial.println("[MQTT] Conectando ao broker MQTT...");
    _estado = CONECTANDO_MQTT;
    
    // Flespi usa token no campo username, password vazio
    if (_mqttClient.connect(MQTT_CLIENT_ID, MQTT_TOKEN, MQTT_PASSWORD)) {
        Serial.println("[MQTT] Conectado ao broker!");
        _estado = CONECTADO;
        
        // Subscribe nos tópicos de comando
        subscribeTopicos();
        
        // Publica status inicial
        publicarStatus("online");
        
        return true;
    }
    
    Serial.print("[MQTT] Falha na conexão. Código: ");
    Serial.println(_mqttClient.state());
    _estado = ERRO_CONEXAO;
    return false;
}

bool GerenciadorMQTT::reconectar() {
    if (!isWiFiConectado()) {
        return conectarWiFi() && conectarMQTT();
    }
    
    if (!isMQTTConectado()) {
        return conectarMQTT();
    }
    
    return true;
}

void GerenciadorMQTT::desconectar() {
    publicarStatus("offline");
    _mqttClient.disconnect();
    WiFi.disconnect();
    _estado = DESCONECTADO;
    Serial.println("[MQTT] Desconectado");
}

bool GerenciadorMQTT::isConectado() {
    return isWiFiConectado() && isMQTTConectado();
}

bool GerenciadorMQTT::isWiFiConectado() {
    return WiFi.status() == WL_CONNECTED;
}

bool GerenciadorMQTT::isMQTTConectado() {
    return _mqttClient.connected();
}

EstadoConexao GerenciadorMQTT::getEstado() {
    return _estado;
}

void GerenciadorMQTT::subscribeTopicos() {
    // Subscribe no tópico de setpoint
    if (_mqttClient.subscribe(TOPIC_COMANDO_SETPOINT)) {
        Serial.print("[MQTT] Subscribed: ");
        Serial.println(TOPIC_COMANDO_SETPOINT);
    }
    
    // Subscribe no tópico de controle
    if (_mqttClient.subscribe(TOPIC_COMANDO_CONTROLE)) {
        Serial.print("[MQTT] Subscribed: ");
        Serial.println(TOPIC_COMANDO_CONTROLE);
    }
}

// ============================================================================
// PUBLICAÇÃO DE TELEMETRIA
// ============================================================================

bool GerenciadorMQTT::publicarDistancia(float distancia) {
    if (!isConectado()) return false;
    
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", distancia);
    
    return _mqttClient.publish(TOPIC_TELEMETRIA_DISTANCIA, payload);
}

bool GerenciadorMQTT::publicarVelocidade(int velocidade) {
    if (!isConectado()) return false;
    
    char payload[8];
    snprintf(payload, sizeof(payload), "%d", velocidade);
    
    return _mqttClient.publish(TOPIC_TELEMETRIA_VELOCIDADE, payload);
}

bool GerenciadorMQTT::publicarInclinacao(String jsonInclinacao) {
    if (!isConectado()) return false;
    
    return _mqttClient.publish(TOPIC_TELEMETRIA_INCLINACAO, jsonInclinacao.c_str());
}

bool GerenciadorMQTT::publicarStatus(String status) {
    if (!_mqttClient.connected()) return false;
    
    return _mqttClient.publish(TOPIC_TELEMETRIA_STATUS, status.c_str());
}

// ============================================================================
// CALLBACKS
// ============================================================================

void GerenciadorMQTT::setCallbackSetpoint(CallbackSetpoint callback) {
    _callbackSetpoint = callback;
}

void GerenciadorMQTT::setCallbackControle(CallbackControle callback) {
    _callbackControle = callback;
}

void GerenciadorMQTT::callbackMQTT(char* topic, byte* payload, unsigned int length) {
    if (_instancia == nullptr) return;
    
    // Converte payload para string
    char mensagem[length + 1];
    memcpy(mensagem, payload, length);
    mensagem[length] = '\0';
    
    Serial.print("[MQTT] Mensagem recebida em ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(mensagem);
    
    // Processa tópico de setpoint
    if (strcmp(topic, TOPIC_COMANDO_SETPOINT) == 0) {
        float novoSetpoint = atof(mensagem);
        if (_instancia->_callbackSetpoint != nullptr && novoSetpoint > 0) {
            _instancia->_callbackSetpoint(novoSetpoint);
        }
    }
    
    // Processa tópico de controle
    else if (strcmp(topic, TOPIC_COMANDO_CONTROLE) == 0) {
        bool ativar = (strcmp(mensagem, "on") == 0 || 
                       strcmp(mensagem, "1") == 0 ||
                       strcmp(mensagem, "true") == 0);
        
        if (_instancia->_callbackControle != nullptr) {
            _instancia->_callbackControle(ativar);
        }
    }
}
