// =============================================================================
// main.cpp — STUB DA PARTE 2 (descartável)
// =============================================================================
// Este arquivo existe para que a Parte 2 possa testar a stack
// ESP32 → HiveMQ → Node-RED → Grafana sem depender da Parte 1.
//
// Ele lê o DHT22, conta cliques no botão (BPM cru), e chama
// enviarParaNuvem() periodicamente. A Parte 1 vai SUBSTITUIR este arquivo
// pelo código real (com buffer offline + simulação de Wi-Fi), mantendo
// as chamadas a cloudBegin() / cloudLoop() / enviarParaNuvem().
// =============================================================================

#include <Arduino.h>
#include <DHT.h>

#include "cloud_link.h"
#include "secrets.h"

// --- pinos do Wokwi ---------------------------------------------------------
#define DHT_PIN     4
#define DHT_TYPE    DHT22
#define BUTTON_PIN  5

DHT dht(DHT_PIN, DHT_TYPE);

// --- contagem de batidas para BPM -------------------------------------------
static volatile uint32_t s_beatCount = 0;
static int s_lastButtonState = HIGH;

static const unsigned long PUBLISH_INTERVAL_MS = 5000;
static unsigned long s_lastPublishMs = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[main] CardioIA stub — Parte 2");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  dht.begin();

  if (!cloudBegin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("[main] cloudBegin falhou — seguindo offline e tentando reconectar no loop");
  }
}

// Polling simples do botão: cada transição HIGH→LOW conta como 1 batida.
// Funciona bem no Wokwi sem debounce sofisticado.
static void sampleButton() {
  int s = digitalRead(BUTTON_PIN);
  if (s == LOW && s_lastButtonState == HIGH) {
    s_beatCount++;
  }
  s_lastButtonState = s;
}

// BPM = batidas na janela × (60 / janela_em_segundos).
// Janela = PUBLISH_INTERVAL_MS, então fator = 60000 / janela.
static int computeBpm(uint32_t beatsInWindow) {
  return (int)((beatsInWindow * 60000UL) / PUBLISH_INTERVAL_MS);
}

void loop() {
  cloudLoop();
  sampleButton();

  unsigned long now = millis();
  if (now - s_lastPublishMs < PUBLISH_INTERVAL_MS) return;
  s_lastPublishMs = now;

  uint32_t beats = s_beatCount;
  s_beatCount = 0;

  Reading r;
  r.ts       = now / 1000;            // sem RTC — usa segundos desde boot
  r.temp     = dht.readTemperature();
  r.hum      = dht.readHumidity();
  r.bpm      = computeBpm(beats);
  r.buffered = false;

  // DHT22 às vezes retorna NaN nas primeiras leituras — descarta.
  if (isnan(r.temp) || isnan(r.hum)) {
    Serial.println("[main] DHT NaN, pulando publicação");
    return;
  }

  bool ok = enviarParaNuvem(r);
  Serial.printf("[main] ts=%lu temp=%.1f hum=%.1f bpm=%d publish=%s\n",
                (unsigned long)r.ts, r.temp, r.hum, r.bpm, ok ? "OK" : "FAIL");
}
