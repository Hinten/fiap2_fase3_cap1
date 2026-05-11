// =============================================================================
// main.cpp — Parte 1: Leitura de sensores + buffer offline (Edge Computing)
// =============================================================================
// Esta implementação lê DHT22 (temperatura/umidade) e botão (BPM simulado),
// armazena leituras em buffer circular na RAM para resiliência offline.
// Usa cloud_link.h para transmissão MQTT quando conectado (Parte 2).
// Estratégia de armazenamento: buffer circular com máximo de 100 amostras.
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

// --- estrutura de leitura ---------------------------------------------------
struct Reading {
    uint32_t ts;        // segundos desde boot
    float    temp;      // °C — DHT22
    float    hum;       // %  — DHT22
    int      bpm;       // batidas/min calculadas a partir do botão
    bool     buffered;  // true se recuperada do buffer offline
};

// --- buffer circular para resiliência offline -------------------------------
const int MAX_BUFFER = 100;
Reading buffer[MAX_BUFFER];
int bufferHead = 0;
int bufferTail = 0;
int bufferSize = 0;

// --- funções do buffer -----------------------------------------------------
void pushBuffer(Reading r) {
    buffer[bufferHead] = r;
    bufferHead = (bufferHead + 1) % MAX_BUFFER;
    if (bufferSize < MAX_BUFFER) {
        bufferSize++;
    } else {
        // sobrescreve o mais antigo
        bufferTail = (bufferTail + 1) % MAX_BUFFER;
    }
}

Reading popBuffer() {
    if (bufferSize > 0) {
        Reading r = buffer[bufferTail];
        bufferTail = (bufferTail + 1) % MAX_BUFFER;
        bufferSize--;
        return r;
    }
    // não deveria acontecer
    Reading empty = {0, 0.0, 0.0, 0, false};
    return empty;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[main] CardioIA Parte 1 — Edge Computing + MQTT Parte 2");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    dht.begin();

    if (!cloudBegin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("[main] cloudBegin falhou — seguindo offline");
    }
}

static void sampleButton() {
    int s = digitalRead(BUTTON_PIN);
    if (s == LOW && s_lastButtonState == HIGH) {
        s_beatCount++;
    }
    s_lastButtonState = s;
}

static int computeBpm(uint32_t beatsInWindow) {
    return (int)((beatsInWindow * 60000UL) / PUBLISH_INTERVAL_MS);
}

void loop() {
    cloudLoop();  // Mantém MQTT vivo
    sampleButton();

    unsigned long now = millis();
    if (now - s_lastPublishMs < PUBLISH_INTERVAL_MS) return;
    s_lastPublishMs = now;

    uint32_t beats = s_beatCount;
    s_beatCount = 0;

    Reading r;
    r.ts       = now / 1000;
    r.temp     = dht.readTemperature();
    r.hum      = dht.readHumidity();
    r.bpm      = computeBpm(beats);
    r.buffered = false;

    if (isnan(r.temp) || isnan(r.hum)) {
        Serial.println("[main] DHT NaN, pulando leitura");
        return;
    }

    Serial.printf("[main] ts=%lu temp=%.1f hum=%.1f bpm=%d connected=%s buffer_size=%d\n",
                 (unsigned long)r.ts, r.temp, r.hum, r.bpm, cloudIsConnected() ? "YES" : "NO", bufferSize);

    if (cloudIsConnected()) {
        // Tenta enviar leitura atual
        if (!enviarParaNuvem(r)) {
            pushBuffer(r);  // Falhou, bufferiza
        }
        // Sincroniza buffer offline
        while (bufferSize > 0 && cloudIsConnected()) {
            Reading b = popBuffer();
            b.buffered = true;
            if (!enviarParaNuvem(b)) {
                pushBuffer(b);  // Falhou novamente, re-bufferiza
                break;
            }
        }
    } else {
        // Offline: bufferiza tudo
        pushBuffer(r);
        Serial.println("[main] Leitura armazenada no buffer offline");
    }
}
