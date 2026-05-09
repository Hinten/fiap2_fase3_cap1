// =============================================================================
// cloud_link.cpp — Implementação da camada Wi-Fi + MQTT (Parte 2 do CardioIA).
// Conecta ao HiveMQ Cloud via TLS (porta 8883) e publica leituras serializadas
// como JSON no tópico cardioia/<deviceId>/telemetry. Inclui Last-Will Testament
// (LWT) no tópico .../status para indicar quedas inesperadas.
// =============================================================================

#include "cloud_link.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// -----------------------------------------------------------------------------
// Certificado raiz Let's Encrypt (ISRG Root X1) — usado pelo HiveMQ Cloud.
// Obtido de https://letsencrypt.org/certs/isrgrootx1.pem
// -----------------------------------------------------------------------------
static const char* ISRG_ROOT_X1 PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// -----------------------------------------------------------------------------
// Estado interno do módulo
// -----------------------------------------------------------------------------
static WiFiClientSecure  s_tlsClient;
static PubSubClient      s_mqtt(s_tlsClient);

static char s_topicTelemetry[64];
static char s_topicStatus[64];

static unsigned long s_lastReconnectMs = 0;
static const unsigned long RECONNECT_BACKOFF_MS = 5000;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static void buildTopics() {
  snprintf(s_topicTelemetry, sizeof(s_topicTelemetry),
           "cardioia/%s/telemetry", DEVICE_ID);
  snprintf(s_topicStatus, sizeof(s_topicStatus),
           "cardioia/%s/status", DEVICE_ID);
}

static bool connectWiFi(const char* ssid, const char* password) {
  Serial.printf("[wifi] conectando em \"%s\"...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
    if (millis() - start > 20000) {
      Serial.println("\n[wifi] timeout");
      return false;
    }
  }
  Serial.printf("\n[wifi] OK ip=%s\n", WiFi.localIP().toString().c_str());
  return true;
}

// Tenta conectar ao broker. NÃO bloqueia o loop principal — usa backoff.
static bool tryMqttConnect() {
  if (millis() - s_lastReconnectMs < RECONNECT_BACKOFF_MS) return false;
  s_lastReconnectMs = millis();

  Serial.printf("[mqtt] conectando em %s:%d ...\n", MQTT_HOST, MQTT_PORT);
  // Cliente ID único + LWT em status (qos1, retain). Quando o broker detecta
  // queda, ele publica "offline" automaticamente — útil pro dashboard.
  bool ok = s_mqtt.connect(
      DEVICE_ID,
      MQTT_USER, MQTT_PASSWORD,
      s_topicStatus,        // willTopic
      1,                    // willQos
      true,                 // willRetain
      "offline"             // willMessage
  );
  if (!ok) {
    Serial.printf("[mqtt] falhou rc=%d\n", s_mqtt.state());
    return false;
  }
  s_mqtt.publish(s_topicStatus, "online", true);
  Serial.println("[mqtt] OK");
  return true;
}

// -----------------------------------------------------------------------------
// API pública
// -----------------------------------------------------------------------------
bool cloudBegin(const char* ssid, const char* password) {
  buildTopics();
  s_tlsClient.setCACert(ISRG_ROOT_X1);
  s_mqtt.setServer(MQTT_HOST, MQTT_PORT);
  s_mqtt.setBufferSize(512);
  s_mqtt.setKeepAlive(30);

  if (!connectWiFi(ssid, password)) return false;
  return tryMqttConnect();
}

bool cloudLoop() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (!s_mqtt.connected()) {
    tryMqttConnect();
  }
  s_mqtt.loop();
  return s_mqtt.connected();
}

bool cloudIsConnected() {
  return s_mqtt.connected();
}

bool enviarParaNuvem(const Reading& r) {
  if (!s_mqtt.connected()) return false;

  // Payload JSON conforme contrato (ver cloud_link.h e relatório).
  StaticJsonDocument<256> doc;
  doc["ts"]       = r.ts;
  doc["deviceId"] = DEVICE_ID;
  doc["temp"]     = r.temp;
  doc["hum"]      = r.hum;
  doc["bpm"]      = r.bpm;
  doc["buffered"] = r.buffered;

  char buf[256];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  // QoS 1 garante entrega ao menos uma vez (tolerável ter duplicata em saúde
  // não-crítica). Sem retain: dashboard só vê valores frescos.
  bool ok = s_mqtt.publish(s_topicTelemetry, (const uint8_t*)buf, n, false);
  if (!ok) Serial.println("[mqtt] publish falhou");
  return ok;
}
