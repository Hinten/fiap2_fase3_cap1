// =============================================================================
// cloud_link.h — Contrato Parte 1 ↔ Parte 2 do CardioIA
// =============================================================================
// A Parte 1 (leitura de sensores + buffer offline) inclui este header e usa
// SOMENTE as funções abaixo para enviar dados pra nuvem. A implementação
// MQTT/TLS/Wi-Fi vive em cloud_link.cpp (Parte 2). Assim cada lado evolui
// independente.
// =============================================================================

#ifndef CARDIOIA_CLOUD_LINK_H
#define CARDIOIA_CLOUD_LINK_H

#include <Arduino.h>

// Pacote único de leitura. Mesmo formato vai ao vivo OU vindo do buffer
// offline da Parte 1 (campo `buffered` distingue os dois casos).
struct Reading {
  uint32_t ts;        // epoch s (preferido) OU millis() se ESP32 sem RTC
  float    temp;      // °C — DHT22
  float    hum;       // %  — DHT22
  int      bpm;       // batidas/min calculadas a partir do botão
  bool     buffered;  // true = leitura recuperada do buffer offline
};

// Inicializa Wi-Fi + MQTT (TLS porta 8883). Retorna true em sucesso.
// Chamar uma vez no setup().
bool cloudBegin(const char* ssid, const char* password);

// Mantém a conexão MQTT viva. Chamar a cada iteração do loop().
// Retorna true se está conectado ao broker.
bool cloudLoop();

// Publica uma leitura no tópico cardioia/<deviceId>/telemetry.
// Retorna false se falhar (Parte 1 deve manter a leitura no buffer).
bool enviarParaNuvem(const Reading& r);

// Consulta rápida — Parte 1 usa pra decidir entre publicar ou bufferizar.
bool cloudIsConnected();

#endif  // CARDIOIA_CLOUD_LINK_H
