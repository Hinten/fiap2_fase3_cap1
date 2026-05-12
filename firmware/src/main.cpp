// =============================================================================
// main.cpp — PARTE 1: Leitura de Sensores, Buffer Offline e Resiliência (Edge)
// =============================================================================
// PARTE 1 DO PROJETO CARDIOIA — Implementação principal
//
// Responsabilidades:
// 1. Ler DHT22 (temperatura + umidade) + Botão (BPM)
// 2. Manter buffer circular em RAM para resiliência offline
// 3. Sincronizar dados ao reconectar à nuvem
// 4. Integrar com Parte 2 (cloud_link.h) para MQTT publish
//
// Arquitetura de Buffer:
// - Buffer circular de 100 amostras (struct Reading) usando índices head/tail
// - Quando online: envia direto; se falhar ou offline, bufferiza
// - Quando reconecta: sincroniza buffer (marcando buffered=true) e limpa
// - Limite: ~8 minutos de dados a 5s/amostra (100 × 5s = 500s)
//
// Pinos Wokwi (conforme assets/wokwi_monitor_cardiaco.png):
// - DHT22: pino 4 (SDA típico, adaptado para DHT)
// - Botão: pino 5 (entrada com PULLUP para simular batidas)
// =============================================================================

#include <Arduino.h>
#include <DHT.h>
#include <cstring>

#include "cloud_link.h"
#include "secrets.h"

// --- Configuração de pinos (Wokwi) ------------------------------------------
#define DHT_PIN     4
#define DHT_TYPE    DHT22
#define BUTTON_PIN  5

DHT dht(DHT_PIN, DHT_TYPE);

// --- Buffer circular para resiliência offline --------------------------------
// Estrutura: fila FIFO simples com índices head (escrita) e tail (leitura)
// Quando cheio, próxima escrita sobrescreve a entrada mais antiga (FIFO)

#define BUFFER_SIZE 100

static Reading s_readingBuffer[BUFFER_SIZE];
static int s_bufferHead = 0;
static int s_bufferTail = 0;
static int s_bufferCount = 0;

// Adiciona uma leitura ao buffer. Se cheio, sobrescreve a mais antiga.
static void bufferAdd(const Reading& r) {
  s_readingBuffer[s_bufferHead] = r;
  s_bufferHead = (s_bufferHead + 1) % BUFFER_SIZE;

  if (s_bufferCount < BUFFER_SIZE) {
    s_bufferCount++;
  } else {
    // Buffer cheio — move tail para descartar a mais antiga
    s_bufferTail = (s_bufferTail + 1) % BUFFER_SIZE;
  }
}

// Remove e retorna a leitura mais antiga do buffer. Retorna false se vazio.
static bool bufferRemove(Reading& r) {
  if (s_bufferCount == 0) return false;

  r = s_readingBuffer[s_bufferTail];
  s_bufferTail = (s_bufferTail + 1) % BUFFER_SIZE;
  s_bufferCount--;
  return true;
}

// Retorna o número de leituras no buffer
static int bufferSize() {
  return s_bufferCount;
}

// --- Contagem de batidas para BPM -------------------------------------------
static volatile uint32_t s_beatCount = 0;
static int s_lastButtonState = HIGH;

// --- Timing de publicação ---------------------------------------------------
static const unsigned long PUBLISH_INTERVAL_MS = 5000;
static unsigned long s_lastPublishMs = 0;

// --- Estado de sincronização ------------------------------------------------
// Controla se estamos sincronizando buffer após reconexão
static bool s_syncMode = false;
static unsigned long s_lastConnectedMs = 0;

// =============================================================================
// SETUP — Inicialização de sensores e nuvem
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== CardioIA — Parte 1: Edge Computing & Resiliência ===\n");

  // Inicializa sensores
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  dht.begin();

  Serial.println("[main] Inicializando DHT22 e botão...");
  delay(1000);  // DHT22 precisa de tempo para estabilizar

  // Inicializa nuvem (Wi-Fi + MQTT)
  Serial.println("[main] Iniciando cloud_link (Wi-Fi + MQTT)...");
  if (!cloudBegin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("[main] cloudBegin retornou false — modo offline, tentará reconectar no loop");
  }

  Serial.println("[main] Setup completo. Entrando no loop principal.\n");
}

// =============================================================================
// Polling simples do botão
// =============================================================================
// Cada transição HIGH→LOW conta como 1 batida.
// Funciona bem no Wokwi sem debounce sofisticado.
static void sampleButton() {
  int s = digitalRead(BUTTON_PIN);
  if (s == LOW && s_lastButtonState == HIGH) {
    s_beatCount++;
  }
  s_lastButtonState = s;
}

// =============================================================================
// Cálculo de BPM
// =============================================================================
// BPM = (batidas_na_janela × 60000) / intervalo_ms
// Com intervalo = 5000ms: BPM = batidas × 12
static int computeBpm(uint32_t beatsInWindow) {
  return (int)((beatsInWindow * 60000UL) / PUBLISH_INTERVAL_MS);
}

// =============================================================================
// Sincronização do buffer quando reconecta
// =============================================================================
// Envia todas as leituras armazenadas, marcadas como buffered=true
static void syncBuffer() {
  if (bufferSize() == 0) {
    Serial.printf("[main] Buffer vazio, nada para sincronizar\n");
    return;
  }

  Serial.printf("[main] 🔄 Sincronizando buffer (%d leituras)...\n", bufferSize());

  int sent = 0;
  Reading r;
  while (bufferRemove(r)) {
    r.buffered = true;  // Marca como leitura recuperada

    if (enviarParaNuvem(r)) {
      sent++;
      Serial.printf("[main]   ✓ buffered ts=%lu bpm=%d\n", (unsigned long)r.ts, r.bpm);
    } else {
      // Se falhar ao enviar, re-insere e para (não perde dados)
      bufferAdd(r);
      Serial.printf("[main]   ✗ falha ao enviar — parando sincronização por enquanto\n");
      break;
    }
  }

  Serial.printf("[main] Sincronização completa: %d/%d enviadas\n\n", sent, sent + bufferSize());
}

// =============================================================================
// LOOP PRINCIPAL — Coleta, buffer e publish
// =============================================================================
void loop() {
  // Mantém Wi-Fi e MQTT vivos (não-bloqueante)
  cloudLoop();

  // Amostra botão continuamente (estado para BPM)
  sampleButton();

  // Verifica timing para publicação
  unsigned long now = millis();
  if (now - s_lastPublishMs < PUBLISH_INTERVAL_MS) {
    return;
  }
  s_lastPublishMs = now;

  // Reseta contador de batidas para próxima janela
  uint32_t beats = s_beatCount;
  s_beatCount = 0;

  // =========================================================================
  // Lê sensores
  // =========================================================================
  Reading r;
  r.ts       = now / 1000;              // Segundos desde boot (sem RTC)
  r.temp     = dht.readTemperature();   // °C
  r.hum      = dht.readHumidity();      // %
  r.bpm      = computeBpm(beats);       // Batidas/min calculadas
  r.buffered = false;

  // Descarta leituras inválidas do DHT22 (comum nas primeiras tentativas)
  if (isnan(r.temp) || isnan(r.hum)) {
    Serial.println("[main] ⚠ DHT22 retornou NaN, pulando publicação");
    return;
  }

  // =========================================================================
  // Lógica de estatus de conectividade e decisão de publicação
  // =========================================================================
  bool isConnected = cloudIsConnected();

  if (isConnected) {
    s_lastConnectedMs = now;

    // Se estava offline e acabou de reconectar, sincroniza buffer
    if (s_syncMode) {
      s_syncMode = false;
      syncBuffer();
    }

    // Tenta publicar leitura atual
    if (enviarParaNuvem(r)) {
      Serial.printf("[main] ✓ ts=%lu temp=%.1f°C hum=%.1f%% bpm=%d [ONLINE]\n",
                    (unsigned long)r.ts, r.temp, r.hum, r.bpm);
    } else {
      // Falha na publicação — bufferiza para tentar depois
      bufferAdd(r);
      Serial.printf("[main] ✗ publish falhou — buffering (n=%d)\n", bufferSize());
    }
  } else {
    // ======= OFFLINE =======
    // Armazena leitura no buffer
    bufferAdd(r);

    Serial.printf("[main] 📵 OFFLINE — buffering (ts=%lu, buffer=%d/%d)\n",
                  (unsigned long)r.ts, bufferSize(), BUFFER_SIZE);

    // Se estava online e agora offline, marca para sincronizar quando reconectar
    if (!s_syncMode && s_lastConnectedMs > 0 && (now - s_lastConnectedMs > 5000)) {
      s_syncMode = true;
      Serial.println("[main] ⚠ Conexão perdida — sincronizará ao reconectar");
    }
  }
}

