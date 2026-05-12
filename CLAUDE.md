# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project context

FIAP coursework — **Fase 3, Cap. 1**: the *CardioIA* IoT health-monitoring prototype. The assignment brief is in `enunciado.md`. The project is fully implemented.

**Grading rubric (2 pts each):** sensor reads, offline resilience, MQTT integration, dashboard + alerts, documentation. Keep changes balanced across all five axes.

## Build & run commands

### Firmware (ESP32 / PlatformIO)
```bash
cd firmware
pio run -e esp32dev           # compilar
pio run -e esp32dev -t upload # upload para ESP32 físico (opcional)
# Serial monitor: baud 115200
```
Expected output: `Flash: 68% / RAM: 14.6% / [SUCCESS]`

### Mock publisher (Python — testa Node-RED sem ESP32)
```bash
cd scripts
cp .env.example .env   # editar com creds HiveMQ
pip install -r requirements.txt
python mock_publisher.py --scenario normal --duration 30
# Cenários: normal | taquicardia | febre | tudo | offline-flush | ruido
```

### Node-RED
```bash
npm install -g --unsafe-perm node-red
node-red
# Editor: http://127.0.0.1:1880  |  Dashboard: http://127.0.0.1:1880/ui
```
Importar `node-red/flows.json` → configurar nó MQTT com host HiveMQ → Deploy.

### Ir Além 1 (Flask API + monitor e-mail)
```bash
cd ir_alem_1
docker compose up          # recomendado
# ou: python api/api_server.py + python monitor/monitor.py
```

### Ir Além 2 (Notebook IA)
```bash
cd ir_alem_2
pip install -r requirements.txt
jupyter notebook cardioia_ir_alem2.ipynb
```

## Credenciais (nunca commitar)

- `firmware/include/secrets.h` — copiar de `secrets.h.example`, preencher `WIFI_SSID`, `MQTT_HOST`, `MQTT_USER`, `MQTT_PASSWORD`, `DEVICE_ID`
- `scripts/.env` — copiar de `.env.example`, mesmas credenciais HiveMQ
- Ambos estão no `.gitignore`

## Arquitetura

### Fluxo de dados

```
ESP32 (Wokwi)
  │  DHT22 + Botão → Reading struct
  │  Buffer RAM 100 amostras (se offline)
  ↓ MQTT/TLS :8883
HiveMQ Cloud
  ↓ MQTT Subscribe
Node-RED  →  /ui  (chart BPM + gauge temp + alerta)
          →  InfluxDB Cloud  →  Grafana Cloud (bônus)
```

### Contrato Parte 1 ↔ Parte 2

`firmware/include/cloud_link.h` é o único ponto de acoplamento entre as partes. A Parte 1 (`main.cpp`) usa apenas:
- `cloudBegin()` — setup Wi-Fi + MQTT
- `cloudLoop()` — keep-alive, chamar a cada iteração do `loop()`
- `cloudIsConnected()` — decide publicar ou bufferizar
- `enviarParaNuvem(Reading)` — retorna `false` se falhar; Parte 1 deve guardar no buffer

A Parte 2 (`cloud_link.cpp`) implementa Wi-Fi, TLS e MQTT sem que a Parte 1 precise saber detalhes.

### JSON payload (tópico `cardioia/<deviceId>/telemetry`)
```json
{ "ts": 1715260800, "deviceId": "cardioia-01",
  "temp": 36.8, "hum": 58.2, "bpm": 78, "buffered": false }
```
O campo `buffered: true` indica leitura recuperada do buffer offline.

### Buffer offline

Ring buffer FIFO de 100 amostras em RAM (`main.cpp`). Capacidade ~8 min a 5 s/amostra. **Não usar SPIFFS** — é volátil no Wokwi/PlatformIO (limitação do simulador aceita pelo enunciado).

### Alertas Node-RED

Limiares configuráveis via context global (`global.BPM_MAX = 120`, `global.TEMP_MAX = 38`). A função JavaScript de alerta descarta payloads malformados sem quebrar o flow.

## Idioma

Código, comentários e documentação em **português (pt-BR)**, conforme exigência do enunciado.
