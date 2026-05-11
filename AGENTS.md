# AGENTS.md — CardioIA Development Guidance

**Project**: CardioIA IoT health-monitoring prototype (FIAP Fase 3, Cap. 1)

Guidance for AI coding agents contributing to this repository. For project scope & deliverables see `CLAUDE.md` and `enunciado.md`.

---

## Architecture Overview

CardioIA flows data through **three connected layers**:

1. **Edge** (Firmware) — ESP32 (Wokwi simulator)
   - Reads DHT22 (temp/humidity) + button (BPM simulation)
   - **Offline buffer** for resilience (ring buffer in RAM, NOT SPIFFS — Wokwi is volatile)
   - Publishes JSON telemetry every 5 seconds via MQTT (when connected)

2. **Cloud Broker** — HiveMQ Cloud (Serverless cluster)
   - TLS port 8883, QoS 1, username/password auth
   - Topic: `cardioia/<deviceId>/telemetry`
   - Stores last message per device (retain flag for status, not telemetry)

3. **Fog/Dashboard** — Node-RED (local WebUI) + optional Grafana (cloud)
   - Subscribes to `cardioia/+/telemetry` (wildcard for all devices)
   - Real-time chart (BPM, 5-min window), gauge (temperature), alert indicator
   - Thresholds: BPM > 120 (tachycardia), Temp > 38°C (fever)

**Critical Design**: `firmware/include/cloud_link.h` is a **formal contract** between Part 1 (sensor logic, Parte 1 responsibility) and Part 2 (cloud transmit, Parte 2 responsibility). Part 1 calls only `cloudBegin()`, `cloudLoop()`, `enviarParaNuvem()`, `cloudIsConnected()`. Part 2 implements these as a drop-in library.

---

## Telemetry Schema

All published payloads follow this JSON structure:

```json
{
  "ts": 1715260800,
  "deviceId": "cardioia-01",
  "temp": 36.8,
  "hum": 58.2,
  "bpm": 78,
  "buffered": false
}
```

- **ts**: Unix epoch (s), or millis-since-boot if no RTC
- **deviceId**: Configured in `firmware/include/secrets.h` as `DEVICE_ID`
- **temp**, **hum**: float from DHT22
- **bpm**: int computed from button edge-count × (60000 / publish_interval_ms)
- **buffered**: `true` if recovered from offline buffer and re-transmitted

Topic: Always `cardioia/<deviceId>/telemetry` (built in `cloud_link.cpp:buildTopics()`).

---

## Part 1 ↔ Part 2 Integration

### For Part 1 Developer

Include `cloud_link.h` and `secrets.h`:

```cpp
#include "cloud_link.h"
#include "secrets.h"

void setup() {
    Serial.begin(115200);
    cloudBegin(WIFI_SSID, WIFI_PASSWORD);
    // ... your sensor init ...
}

void loop() {
    cloudLoop();  // Handles Wi-Fi reconnect, MQTT keep-alive.
    
    Reading r = lerSensores();  // YOUR FUNCTION: read DHT22 + button
    if (cloudIsConnected()) {
        if (!enviarParaNuvem(r)) {
            bufferLocal.push(r);  // YOUR LOGIC: buffer if publish failed
        }
        // Flush offline buffer when online
        while (!bufferLocal.empty() && cloudIsConnected()) {
            Reading b = bufferLocal.pop();
            b.buffered = true;
            if (!enviarParaNuvem(b)) { bufferLocal.push(b); break; }
        }
    } else {
        bufferLocal.push(r);  // YOUR LOGIC: buffer all readings offline
    }
}
```

### For Part 2 Developer (Already Implemented)

- Never modify `firmware/include/cloud_link.h` signature
- Implement the four functions in `firmware/src/cloud_link.cpp`
- **enviarParaNuvem** returns `false` if publish fails (no retry — Part 1 decides)
- **cloudLoop** uses non-blocking backoff (5s) to avoid starving the main loop
- Certificate is embedded (ISRG Root X1 for Let's Encrypt)
- Credentials live in `firmware/include/secrets.h` (file is `.gitignore`'d)

### Testing Without Real Hardware

Use `scripts/mock_publisher.py` to inject JSON directly into broker:

```powershell
# Setup (one time)
cd scripts
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
Copy-Item .env.example .env
# Edit .env with HiveMQ host/user/password

# Run a scenario
python mock_publisher.py --scenario normal --duration 30
```

Scenarios: `normal`, `taquicardia`, `febre`, `tudo`, `offline-flush`, `ruido`.

---

## Firmware Development Workflow

### Setup

```powershell
# Windows PowerShell
copy firmware\include\secrets.h.example firmware\include\secrets.h
# Edit with:
#   WIFI_SSID, WIFI_PASSWORD (for Wokwi wifi simulation flag)
#   MQTT_HOST (e.g., "xxxx.s2.eu.hivemq.cloud")
#   MQTT_USER, MQTT_PASSWORD (HiveMQ creds)
#   DEVICE_ID (e.g., "cardioia-01")
```

### Compile (PlatformIO)

```powershell
pio run -e esp32dev        # Compile
pio run -e esp32dev -t upload   # Upload to board (or paste into Wokwi editor)
pio device monitor         # Watch Serial output (115200 baud)
```

### Key Files

| File | Purpose |
|------|---------|
| `firmware/platformio.ini` | ESP32 dependencies (PubSubClient, ArduinoJson, DHT) + build flags |
| `firmware/include/cloud_link.h` | Contract interface (unmodifiable once both parts exist) |
| `firmware/src/cloud_link.cpp` | TLS/MQTT implementation (Part 2) |
| `firmware/src/main.cpp` | Stub for Part 2 testing; **Part 1 will replace this** |

### Serial Output Interpretation

```
[main] ts=1715260800 temp=36.8 hum=58.2 bpm=78 publish=OK
[mqtt] conectando em xxxx.s2.eu.hivemq.cloud:8883 ...
[mqtt] OK
[wifi] conectando em "MyWiFi"...
[wifi] OK ip=192.168.1.100
```

- Timestamps: `[prefix]` indicates module (`main`, `wifi`, `mqtt`)
- **Important**: To verify cloud delivery, check HiveMQ Web Client (`cardioia/#` subscription)

---

## Dashboard (Node-RED) Workflow

### Quick Start

```powershell
npm install -g --unsafe-perm node-red   # One time
node-red

# In browser:
# - Editor: http://127.0.0.1:1880
# - Dashboard: http://127.0.0.1:1880/ui  ← THIS is what users see
```

⚠️ **Common error**: Staring at the editor (1880) expecting a chart. Charts live at `/ui` only.

### Importing Flows

1. Editor → menu → **Import** → select `node-red/flows.json`
2. Edit `HiveMQ Cloud` node:
   - Server: your cluster host (e.g., `xxxx.s2.eu.hivemq.cloud`)
   - Port: 8883, **Use TLS** = ✓
   - Security: username/password
3. Deploy (top right)

### Modifying Alerts

Thresholds (`BPM_MAX=120`, `TEMP_MAX=38`) live in:
- `context global` (Settings → Context Data in editor), OR
- Node-RED flow's `regra de alerta` function node (JS snippet)

To adjust at runtime: create an `inject → change` node that sets `global.BPM_MAX`, `global.TEMP_MAX` on startup.

### Smoke Test

```powershell
# Without any external input, test the parser:
# (In editor) Enable node "MOCK alerta" (blue triangle left of node)
# Deploy
# Click the node — dashboard should show fake alert
```

---

## Optional Extensions

### IR ALÉM 1 — REST API + Email Alerts

Located in `ir_alem_1/`. Provides:
- **Docker Compose** orchestration (api service + monitor service)
- Flask REST endpoints: `/health`, `/patients/<id>/vitals`, `/patients/<id>/alerts`
- Monitor queries vitals periodically, fires SMTP emails if thresholds crossed

**Setup**: Copy `.env.example` to `.env`, configure SMTP/thresholds, then `docker-compose up`.

### IR ALÉM 2 — AI Time Series Analysis

Located in `ir_alem_2/`. Notebook comparing:
- Logistic regression (baseline)
- LIF/FHN neuromorphic model (advanced)

**Output**: `cardioia_ir_alem2.ipynb` + `relatorio_comparativo.md` + YouTube link.

---

## Testing Strategy (Grading Checklist)

| Test | Command | Expected | Weight |
|------|---------|----------|--------|
| T1 | `mosquitto_pub` direct to HiveMQ | publish ✓ | TLS setup |
| T2 | `mock_publisher.py --scenario normal --duration 5` | 5 msgs in Web Client | Schema |
| T3 | Wokwi running + ESP32 logging | telemetry appears | ESP32 integration |
| T4 | Reconnection test (bad creds → correct) | logs show retry | Resilience |
| T5 | Debug node in Node-RED | JSON parsed correctly | Parser robustness |
| T6 | `/ui` during T2 or T3 | chart/gauge update | Dashboard |
| T7 | `--scenario taquicardia` | alert indicator red | BPM alert |
| T8 | `--scenario febre` | alert indicator red | Temp alert |
| T9 | `--scenario offline-flush` | 10 msgs buffered=true | Edge buffering |
| T10 | `--scenario ruido` | dashboard doesn't crash | Error handling |
| T11 (bonus) | Grafana query after 30s normal | ≥30 data points | InfluxDB integration |
| T12 (bonus) | Grafana panel with T11 query | time series chart | Visualization |

---

## Project Conventions

- **Language**: Portuguese (pt-BR) in code comments, docs, and reports
- **File structure**: Flat layout (`firmware/`, `node-red/`, `scripts/`, `ir_alem_1/`, `ir_alem_2/`, `docs/`)
- **Naming**: CamelCase for C++ classes/functions, snake_case for Python
- **MQTT QoS**: Always 1 (at-least-once, idempotent on dashboard)
- **SPIFFS**: **Not used**. Wokwi SPIFFS is volatile → use in-memory ring buffer + Serial dumps instead
- **Credentials**: Never commit secrets. Use `secrets.h.example` + `.env.example` templates
- **Logging**: Prefix all Serial prints with `[module]` for traceability (e.g., `[mqtt]`, `[wifi]`)

---

## Cross-Component Communication Patterns

| From | To | Protocol | Topic / Endpoint | Format |
|------|---------|----------|-----|--------|
| ESP32 (Part 1 code) | cloud_link lib | Function call | N/A | `Reading` struct |
| cloud_link lib | HiveMQ Broker | MQTT TLS | `cardioia/<deviceId>/telemetry` | JSON (serialized) |
| HiveMQ Broker | Node-RED | MQTT subscribe | `cardioia/+/telemetry` | JSON (parsed) |
| Node-RED dashboard | User | HTTP WebUI | `/ui` realtime update | HTML/Canvas (chart.js) |
| (Optional) Node-RED | InfluxDB Cloud | HTTP POST | `/write` | Line protocol |
| (Optional) InfluxDB | Grafana | Flux query | HTTP API | Time series |
| (Optional) REST API | Monitor | HTTP | `/patients/<id>/vitals` | JSON vitals |
| (Optional) Monitor | SMTP server | SMTP TLS | 587/465 | Email |

---

## Debugging Tips

1. **"publish=FAIL" in Serial** → Check `cloudIsConnected()`. If false, MQTT handshake may be blocking (backoff running). Wait 5s + retry.

2. **"DHT NaN" warnings** → Normal in first ~2s. DHT22 needs time to stabilize. Main loop skips null readings.

3. **Node-RED chart not updating** → Remember: dashboard is at **`/ui`**, not `/`. Also check **HiveMQ Web Client** to verify broker is receiving JSON.

4. **"rc=-4" in MQTT logs** → Connection lost. WiFi may have dropped. Check WiFi simulation flag & credentials in `secrets.h`.

5. **Wokwi project won't run** → Ensure PlatformIO.ini dependencies are in Wokwi project. Copy `firmware/` contents (src + include) directly into Wokwi editor. Don't try to link directories.

6. **"MQTT buffer overflow"** → Increase `setBufferSize()` in `cloudBegin()`. Default 256 bytes may be too small for large JSON payloads + metadata.

---

## References

- **MQTT Broker**: HiveMQ Cloud (https://www.hivemq.com/mqtt-cloud-broker/) — free tier adequate
- **Simulator**: Wokwi (https://wokwi.com) — no account required for basic ESP32
- **Dashboard**: Node-RED (https://nodered.org) + `node-red-dashboard` plugin
- **Grafana** (bonus): Public example at https://hinten.grafana.net/public-dashboards/1fc63ea0bb144546ac549fc34fb3b542
- **Certificates**: ISRG Root X1 embedded in code; valid until 2035

