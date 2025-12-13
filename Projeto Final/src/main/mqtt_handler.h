/*
 * ============================================================================
 * MÓDULO DE COMUNICAÇÃO MQTT
 * ============================================================================
 * Gerencia a conexão WiFi e comunicação MQTT com o broker Flespi.
 * ============================================================================
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================================
// ENUMERAÇÕES
// ============================================================================

enum EstadoConexao {
    DESCONECTADO,
    CONECTANDO_WIFI,
    WIFI_CONECTADO,
    CONECTANDO_MQTT,
    CONECTADO,
    ERRO_CONEXAO
};

// ============================================================================
// CALLBACK PARA COMANDOS RECEBIDOS
// ============================================================================

// Tipos de callback para processar mensagens recebidas
typedef void (*CallbackSetpoint)(float novoSetpoint);
typedef void (*CallbackControle)(bool ativar);

// ============================================================================
// CLASSE DO GERENCIADOR MQTT
// ============================================================================

class GerenciadorMQTT {
public:
    GerenciadorMQTT();
    
    void inicializar();
    void loop();
    
    // Conexão
    bool conectarWiFi();
    bool conectarMQTT();
    bool reconectar();
    void desconectar();
    
    // Status
    bool isConectado();
    bool isWiFiConectado();
    bool isMQTTConectado();
    EstadoConexao getEstado();
    
    // Publicação de telemetria
    bool publicarDistancia(float distancia);
    bool publicarVelocidade(int velocidade);
    bool publicarInclinacao(String jsonInclinacao);
    bool publicarStatus(String status);
    
    // Registro de callbacks para comandos recebidos
    void setCallbackSetpoint(CallbackSetpoint callback);
    void setCallbackControle(CallbackControle callback);

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    EstadoConexao _estado;
    unsigned long _ultimaTentativaReconexao;
    
    CallbackSetpoint _callbackSetpoint;
    CallbackControle _callbackControle;
    
    void subscribeTopicos();
    static void callbackMQTT(char* topic, byte* payload, unsigned int length);
    
    // Instância estática para callback
    static GerenciadorMQTT* _instancia;
};

#endif // MQTT_HANDLER_H
