# CardioIA — Fase 3, Cap. 1

Protótipo de vestível para monitoramento contínuo de pacientes
cardiológicos. ESP32 (Wokwi) coleta sinais vitais, publica via
MQTT/TLS no HiveMQ Cloud e visualiza em dashboards Node-RED + Grafana.

> **Status do repo:** Parte 2 (transmissão + dashboards) implementada.
> Parte 1 (sensores + buffer offline) é responsabilidade de outra
> pessoa do grupo e se encaixa via `firmware/include/cloud_link.h`.

## Estrutura

```
firmware/        ESP32 (PlatformIO/Arduino) — compartilhado P1↔P2
  include/
    cloud_link.h      ← contrato entre as partes
    secrets.h.example ← template de credenciais
  src/
    cloud_link.cpp    ← Wi-Fi + MQTT + publish (Parte 2)
    main.cpp          ← stub testável (Parte 1 substitui)
node-red/        Dashboard
  flows.json
  README.md
scripts/         Mock publisher Python (testes sem ESP32)
  mock_publisher.py
docs/            Relatórios + prints
  parte2_relatorio.md
enunciado.md     Brief original (FIAP)
```

## Setup rápido

### 1. Credenciais

```powershell
# Firmware
copy firmware\include\secrets.h.example firmware\include\secrets.h
# Editar secrets.h com SSID, senha Wi-Fi e creds do HiveMQ.

# Mock publisher
copy scripts\.env.example scripts\.env
# Editar .env com as mesmas creds do HiveMQ.
```

### 2. HiveMQ Cloud

Criar conta gratuita em https://www.hivemq.com/mqtt-cloud-broker/,
criar cluster Serverless, anotar host (`*.s2.eu.hivemq.cloud`) e
gerar usuário/senha. Porta 8883, TLS obrigatório.

### 3. Subir Node-RED

```powershell
npm install -g --unsafe-perm node-red
node-red
```

Acessar editor em http://127.0.0.1:1880, instalar `node-red-dashboard`
em **Manage palette → Install** e importar `node-red/flows.json`.
Editar o nó `HiveMQ Cloud` com seu host/credenciais e fazer Deploy.
Dashboard fica em http://127.0.0.1:1880/ui.

Detalhes em [`node-red/README.md`](node-red/README.md).

### 4. Wokwi

Subir um projeto Wokwi com ESP32 + DHT22 (GPIO 4) + botão (GPIO 5,
INPUT_PULLUP) e colar o conteúdo de `firmware/`. Rodar — o serial
deve mostrar conexão Wi-Fi → MQTT → publish a cada 5 segundos.

## Contrato Parte 1 ↔ Parte 2

A pessoa responsável pela Parte 1 inclui apenas
[`firmware/include/cloud_link.h`](firmware/include/cloud_link.h) e usa:

```cpp
#include "cloud_link.h"
#include "secrets.h"

void setup() {
    Serial.begin(115200);
    cloudBegin(WIFI_SSID, WIFI_PASSWORD);
    // ... inicializar sensores e buffer ...
}

void loop() {
    cloudLoop();

    Reading r = lerSensores();          // <- responsabilidade da Parte 1
    if (cloudIsConnected()) {
        if (!enviarParaNuvem(r)) {
            bufferLocal.push(r);        // <- responsabilidade da Parte 1
        }
        // flush do buffer:
        while (!bufferLocal.empty() && cloudIsConnected()) {
            Reading b = bufferLocal.pop();
            b.buffered = true;
            if (!enviarParaNuvem(b)) { bufferLocal.push(b); break; }
        }
    } else {
        bufferLocal.push(r);
    }
}
```

A Parte 2 garante apenas: o que entrar em `enviarParaNuvem()` chega
ao broker (com TLS, QoS 1) ou retorna `false` para a Parte 1 decidir.

### Schema JSON (publicado em `cardioia/<deviceId>/telemetry`)

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

## Testes

A Parte 2 é testável **sem precisar da Parte 1**, graças ao mock
publisher Python e ao inject node embutido no Node-RED.

### Por camada

| # | Camada | Comando | Esperado |
|---|--------|---------|----------|
| T1 | TLS HiveMQ | `mosquitto_pub -h <host> -p 8883 -u <u> -P <p> --cafile isrgx1.pem -t test -m hi` | publish OK |
| T2 | Schema | `python scripts/mock_publisher.py --scenario normal --duration 5` | 5 mensagens no HiveMQ Web Client |
| T3 | ESP32 → broker | Wokwi rodando + Web Client em `cardioia/#` | mensagens reais aparecem |
| T4 | Reconexão | Trocar senha errada → certa, recompilar | logs mostram retry e reconexão |
| T5 | Broker → Node-RED | mock + nó debug | payload parseado no debug pane |
| T6 | Widgets | abrir `/ui` durante T5 | chart, gauge atualizam |
| T7 | Alerta BPM | `--scenario taquicardia` | indicador acende ao cruzar 120 |
| T8 | Alerta Temp | `--scenario febre` | indicador acende em > 38°C |
| T9 | Alertas combinados | `--scenario tudo` | os dois alertas ativos |
| T10 | Robustez parser | `--scenario ruido` | dashboard não quebra com payload inválido |
| T11 | InfluxDB | após `--scenario normal --duration 30`, query Flux no Influx UI | ≥30 pontos |
| T12 | Grafana | painel com a query da T11 | série temporal recente |
| T13 | Flush offline | `--scenario offline-flush` | 10 pontos `buffered:true` em rajada |

### Cenários ponta-a-ponta (gerar prints pro relatório)

- **E2E-A — Caso feliz Wokwi**: ESP32 stub → todos os widgets ativos por 5 min.
- **E2E-B — Alerta ao vivo**: arrastar slider DHT22 → 39°C, capturar print do indicador vermelho.
- **E2E-C — Sync simulado**: `--scenario offline-flush` (prova o cabo P1↔P2 antes de receber código da P1).

### Checklist de aceitação

- [ ] **Envio MQTT (2 pt)** — T1, T2, T3.
- [ ] **Dashboard + alertas (2 pt)** — T6, T7, T8, T9.
- [ ] **Documentação (2 pt)** — `docs/parte2_relatorio.md` + prints + READMEs.
- [ ] **Bônus Grafana** — T11, T12.
- [ ] **Integração com Parte 1** — `cloud_link.h` finalizado, T13 passando.

## Links externos

- **Wokwi:** *(adicionar link após subir o projeto)*
- **HiveMQ Cloud Web Client:** https://console.hivemq.cloud/
- **Grafana Cloud público (opcional):** *(adicionar link do dashboard se houver)*

## Decisões e escopo

- **Sensor 2 = botão simulando BPM** (sugerido pelo enunciado; combina com alerta > 120 bpm).
- **SPIFFS pulado**: enunciado isenta esse critério no Wokwi (volátil).
- **Next.js / React frontend customizado**: tecnicamente compatível
  (assinaria broker via `wss://`), mas não é requisito do enunciado e
  acrescenta deploy de servidor Node persistente — ficou de fora para
  manter o escopo simples.

Detalhes técnicos completos em [`docs/parte2_relatorio.md`](docs/parte2_relatorio.md).
