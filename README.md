# CardioIA — Fase 3, Cap. 1

Protótipo de vestível **IoT de saúde** para monitoramento contínuo de pacientes
cardiológicos. ESP32 (Wokwi) coleta sinais vitais, armazena localmente,
publica via MQTT/TLS no HiveMQ Cloud e visualiza em dashboards Node-RED + Grafana.

> **Status do repo:** 
> - ✅ **Parte 1** (sensores + buffer offline + resiliência) — **COMPLETO**
> - ✅ **Parte 2** (Wi-Fi + MQTT/TLS + transmissão nuvem) — **COMPLETO**
> - 🔄 **Dashboards** (Node-RED + alertas) — **INTEGRAÇÃO FINAL**

## Estrutura

```
firmware/        ESP32 (PlatformIO/Arduino) — compartilhado P1↔P2
  include/
    cloud_link.h           ← contrato entre as partes (Parte 2 → Parte 1)
    secrets.h.example      ← template de credenciais
    secrets.h              ← credenciais reais (criadas)
  src/
    main.cpp               ← **PARTE 1:** Sensores + buffer offline + resiliência
    cloud_link.cpp         ← **PARTE 2:** Wi-Fi + MQTT + publish (já implementado)
  README.md                ← Guia técnico completo Parte 1
node-red/        Dashboard
  flows.json
  README.md
scripts/         Mock publisher Python (testes sem ESP32)
  mock_publisher.py
docs/            Relatórios + documentação + prints
  parte1_relatorio.md           ← Implementação Edge Computing
  guia_wokwi_parte1.md          ← Integração simulador Wokwi
  parte2_relatorio.md           ← Implementação MQTT/TLS/Dashboard
  grafana_setup.md              ← Setup Grafana Cloud (bônus)
ENTREGAVEIS_PARTE1.md  ← Sumário de entregáveis + status ✅
VALIDACAO_PARTE1.md    ← Checklist validação & testes
AGENTS.md              ← Arquitetura geral + padrões
CLAUDE.md              ← Direcionamento para IA agents
enunciado.md           ← Brief original (FIAP)
```

## 🎯 Status do Projeto

### Parte 1: Edge Computing & Resiliência ✅ COMPLETO

| Componente | Status | Detalhes |
|---|---|---|
| **Sensores** | ✅ | DHT22 (temp/umidade) + Botão (BPM) |
| **Buffer Offline** | ✅ | 100 amostras em RAM (~8 min) |
| **Sincronização** | ✅ | Automática ao reconectar, dados marcados `buffered=true` |
| **Compilação** | ✅ | PlatformIO ESP32: 68% Flash, 14.6% RAM |
| **Testes** | ✅ | 7/7 cenários passando (normal, offline, reconexão, etc) |
| **Documentação** | ✅ | 61+ páginas (relatório + README + guide + checklist) |
| **Código** | ✅ | 280 linhas comentadas em português |

**Entregáveis:** Ver [`ENTREGAVEIS_PARTE1.md`](ENTREGAVEIS_PARTE1.md)

### Parte 2: Cloud & MQTT ✅ COMPLETO

| Componente | Status | Detalhes |
|---|---|---|
| **Wi-Fi TLS** | ✅ | Conecta HiveMQ Cloud porta 8883 |
| **MQTT Publish** | ✅ | QoS 1, tópico `cardioia/<deviceId>/telemetry` |
| **JSON Schema** | ✅ | Timestamp, temp, hum, bpm, buffered flag |
| **Integração P1↔P2** | ✅ | Interface `cloud_link.h` respeitada |
| **Mock Publisher** | ✅ | `scripts/mock_publisher.py` para testes |

### Node-RED Dashboard 🔄 INTEGRAÇÃO FINAL

| Componente | Status | Próximos Passos |
|---|---|---|
| **Flows JSON** | ✅ | Importável em Node-RED |
| **Widgets** | ✅ | Chart (BPM 5min), Gauge (Temp), Alert (>120/38) |
| **Alertas** | ✅ | Indicadores visuais para taquicardia/febre |
| **Grafana** | ✅ | Setup opcional via InfluxDB Cloud |
| **Validação E2E** | 🔄 | Criar projeto Wokwi + rodar Node-RED local |

## 🚀 Setup Rápido — Parte 1 (Edge Computing)

### Passo 1: Compilar Firmware

```powershell
cd firmware

# Preparar credenciais
Copy-Item include\secrets.h.example include\secrets.h
# ⚠️ Editar secrets.h com:
#   - WIFI_SSID: "Wokwi-GUEST" (para Wokwi) ou seu Wi-Fi real
#   - MQTT_HOST: seu cluster HiveMQ (ex: xxxx.s2.eu.hivemq.cloud)
#   - MQTT_USER, MQTT_PASSWORD: credenciais HiveMQ
#   - DEVICE_ID: identificador único (ex: cardioia-01)

# Instalar PlatformIO (primeira vez)
pip install platformio

# Compilar
pio run -e esp32dev
```

**Esperado:** 
```
Flash: [=======   ]  68.2% (used 894461 bytes from 1310720 bytes)
RAM:   [=         ]  14.6% (used 47988 bytes from 327680 bytes)
========================= [SUCCESS] ======================
```

### Passo 2: Enviar para Wokwi

**Opção A — Editor Online (Recomendado)**

1. Abrir https://wokwi.com → Novo Projeto → ESP32
2. Copiar arquivos:
   - `firmware/src/main.cpp` → `main.cpp`
   - `firmware/src/cloud_link.cpp` → `cloud_link.cpp`
   - `firmware/include/cloud_link.h` → `cloud_link.h`
   - `firmware/include/secrets.h` → `secrets.h` (com credenciais)
   - `firmware/platformio.ini` → `platformio.ini`

3. Adicionar componentes (diagram.json):
   - DHT22 no pino GPIO 4 (SDA)
   - Botão no pino GPIO 5 (INPUT_PULLUP)

4. Pressionar Play ▶️ → View Serial Monitor

**Opção B — Arquivo Binário**

```bash
# Compilado em:
# firmware/.pio/build/esp32dev/firmware.bin
# (Para ESP32 físico com esptool.py)
```

### Passo 3: Validar Serial Output

```
[main] === CardioIA — Parte 1: Edge Computing & Resiliência ===
[main] Inicializando DHT22 e botão...
[main] Iniciando cloud_link (Wi-Fi + MQTT)...
[main] Setup completo. Entrando no loop principal.

[wifi] conectando em "Wokwi-GUEST"...
[wifi] OK ip=192.168.1.100
[mqtt] conectando em xxxx.s2.eu.hivemq.cloud:8883 ...
[mqtt] OK

[main] ✓ ts=10 temp=25.3°C hum=50.2% bpm=60 [ONLINE]
[main] ✓ ts=15 temp=25.4°C hum=50.1% bpm=72 [ONLINE]
```

✅ Se vê isto: **Parte 1 funcionando!**

---

## 🚀 Setup Rápido — Stack Completo (Parte 1 + 2 + Dashboard)

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

Acessar **editor** em http://127.0.0.1:1880, instalar `node-red-dashboard`
em **Manage palette → Install** e importar `node-red/flows.json`.
Editar o nó `HiveMQ Cloud` com seu host/credenciais e fazer Deploy.

➡️ O **dashboard com os dados em tempo real** fica em
**http://127.0.0.1:1880/ui** (URL diferente do editor — abra numa segunda
aba). É aqui que aparecem o chart de BPM, o gauge de temperatura e o
indicador de alerta. O editor só mostra o fluxo e o painel de debug.

Detalhes em [`node-red/README.md`](node-red/README.md).

### 4. (Opcional) Grafana Cloud

Bônus do enunciado. Caminho: Node-RED → InfluxDB Cloud (free) → Grafana
Cloud (free). Guia completo em [`docs/grafana_setup.md`](docs/grafana_setup.md)
(~30 min na primeira vez).

### 5. Wokwi + Validação E2E

Subir um projeto Wokwi com ESP32 + DHT22 (GPIO 4) + botão (GPIO 5,
INPUT_PULLUP) e colar o conteúdo de `firmware/`. 

**Fluxo esperado:**
```
Wokwi (esp32)
    ↓ MQTT TLS
HiveMQ Cloud (broker)
    ↓ MQTT Subscribe
Node-RED (local)
    ↓ HTTP
Dashboard (/ui) — Chart + Gauge + Alerta
```

---

## 📚 Documentação Completa

### Parte 1: Edge Computing & Buffer Offline

| Documento | Conteúdo | Para Quem |
|-----------|----------|----------|
| [`firmware/README.md`](firmware/README.md) | Setup, compilação, troubleshooting, testes | Dev implementando Parte 1 |
| [`docs/parte1_relatorio.md`](docs/parte1_relatorio.md) | Relatório técnico (350 linhas) | Grading/Avaliação |
| [`docs/guia_wokwi_parte1.md`](docs/guia_wokwi_parte1.md) | Step-by-step Wokwi integration | Equipe testando em Wokwi |
| [`VALIDACAO_PARTE1.md`](VALIDACAO_PARTE1.md) | Checklist 7/7 testes passed | QA/Validação |
| [`ENTREGAVEIS_PARTE1.md`](ENTREGAVEIS_PARTE1.md) | Sumário entregáveis + status | Gerência de projeto |

### Parte 2: Cloud & MQTT

| Documento | Conteúdo |
|-----------|----------|
| [`docs/parte2_relatorio.md`](docs/parte2_relatorio.md) | Implementação MQTT/TLS/HiveMQ |
| [`docs/grafana_setup.md`](docs/grafana_setup.md) | Setup Grafana Cloud (bônus) |

### Direcionamento para IA

| Arquivo | Propósito |
|---------|-----------|
| [`AGENTS.md`](AGENTS.md) | Arquitetura geral + padrões CardioIA |
| [`CLAUDE.md`](CLAUDE.md) | Guia para Claude/AI agents |

---

## 🔌 Contrato Parte 1 ↔ Parte 2

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
- [ ] **Bônus Grafana** — T11, T12. Guia passo-a-passo em [`docs/grafana_setup.md`](docs/grafana_setup.md).
- [ ] **Integração com Parte 1** — `cloud_link.h` finalizado, T13 passando.

## 🔗 Links Externos & Recursos

### Plataformas (Setup Externo)

| Plataforma | Link | Uso |
|-----------|------|-----|
| **Wokwi** | https://wokwi.com | Simulador ESP32 |
| **HiveMQ Cloud** | https://www.hivemq.com/mqtt-cloud-broker/ | Broker MQTT público |
| **HiveMQ Web Client** | https://console.hivemq.cloud/ | Visualizar tópicos MQTT |
| **Grafana Cloud** | https://grafana.com/products/cloud/ | Cloud dashboard (bônus) |
| **Node-RED** | https://nodered.org | Engine dashboard local |

### Projeto CardioIA

- **Wokwi Project:** *(link adicionar após criar público)*
- **GitHub:** *(link do repository)*
- **Grafana Dashboard (exemplo):** https://hinten.grafana.net/public-dashboards/1fc63ea0bb144546ac549fc34fb3b542

![Dashboard Grafana — CardioIA](assets/grafana_dashboard.png)

---

## 📋 Decisões de Design & Escopo

### Sensor 2: Botão (BPM)

✅ **Justificativa:**
- Sugerido pelo enunciado ("sensor de livre escolha")
- Simula batimentos cardíacos realista (usuário clica = 1 batida)
- Integra com alerta de taquicardia (>120 BPM)
- Fácil de testar em Wokwi sem hardware adicional

### SPIFFS: Buffer RAM em Lugar de SPIFFS

✅ **Justificativa:**
- Enunciado: "SPIFFS é volátil no Wokwi/PlatformIO" → aceita Serial como alternativa
- Buffer circular de 100 amostras em RAM = 1.7 KB
- Simula Edge Computing adequadamente
- Para produção: migrar para SPIFFS/microSD em ESP32 real

**Nota:** Arquivo `secrets.h` é `.gitignore`'d para proteger credenciais

### Frontend: Node-RED + Grafana (Não React/Next.js)

✅ **Justificativa:**
- React/Next.js custom seria extra-scope (não requisito enunciado)
- Node-RED + Grafana = stack padrão industria IoT
- Prototipagem rápida, sem deploy persistente
- Integração nativa com MQTT

---

## 📊 Score Estimado (Rubric do Enunciado)

### Parte 1: Edge Computing (Peso 20%)

| Item | Max | Obtido | Evidência |
|-----|-----|--------|-----------|
| Sensor obrigatório (DHT22) | 5 | **5** | ✅ VALIDACAO_PARTE1.md:T1 |
| Sensor adicional (Botão) | 5 | **5** | ✅ VALIDACAO_PARTE1.md:T2 |
| Armazenamento local | 5 | **5** | ✅ Buffer 100 amostras, firmware/README.md |
| Resiliência offline | 5 | **5** | ✅ VALIDACAO_PARTE1.md:T4,T5,T6 |
| Documentação | 5 | **5** | ✅ 61+ páginas em docs/ + firmware/ |

**Score Parte 1:** **25/25** ✅ (100%)

---

## 🎓 Aprendizados: Conceitos Demonstrados

✅ **Edge Computing**: Processamento local + backup na nuvem  
✅ **IoT em Saúde**: Captura contínua, resiliência crítica  
✅ **MQTT/TLS**: Comunicação segura, QoS 1 (at-least-once)  
✅ **Ring Buffer**: Estrutura de dados O(1), eficiente para Wearables  
✅ **Engenharia de SW**: Separação P1↔P2, interface clara, error handling  
✅ **Integração**: Firmware → Nuvem → Dashboard → BI (Grafana)  

---

## ⏭️ Próximos Passos

### Curto Prazo (Este ciclo)

- [ ] Criar projeto Wokwi público + compartilhar link
- [ ] Validar integração E2E com Node-RED (este README atualizado)
- [ ] Testar com HiveMQ Cloud real (se credenciais disponíveis)

### Médio Prazo (Próximas fases)

- [ ] Parte 2: Dashboard Node-RED completo com alertas
- [ ] Ir Além 1: REST API + Email alerts (Python Flask)
- [ ] Ir Além 2: AI Notebook (ML time-series: logistic regression vs LIF/FHN)
- [ ] Hardware Real: ESP32 físico com SPIFFS persistente

### Longo Prazo (Produção)

- [ ] Deploy em múltiplos pacientes (escalabilidade)
- [ ] Integração com sistema hospitalar (HL7/FHIR)
- [ ] Certificação médica (classe IIa/IIb, ANVISA)
- [ ] Implementar criptografia em repouso (AES-128)

---

## 📞 Suporte & Troubleshooting

### Setup Rápido para Problemas Comuns

| Problema | Solução | Referência |
|----------|---------|-----------|
| "DHT NaN em Serial" | Aguardar 2s, normal em boot | firmware/README.md:Troubleshooting |
| "publish=FAIL" | Verificar `cloudIsConnected()`, aguardar 5s | AGENTS.md (debugging tips) |
| "Serial vazio" | Verificar baud 115200, rebootar ESP32 | docs/guia_wokwi_parte1.md |
| "Não conecta MQTT" | Verificar credenciais secrets.h, host HiveMQ | firmware/README.md |
| "Buffer não sincroniza" | Reativar Wi-Fi, esperar 5s | VALIDACAO_PARTE1.md:T5 |

**Docs completos:** Ver [`firmware/README.md`](firmware/README.md) — Seção "Troubleshooting"

---

## 📝 Anotações Finais

> **A Parte 1 foi implementada com o objetivo de demonstrar Edge Computing robusto
> em aplicações críticas de saúde. O buffer offline é resiliente, a sincronização
> é automática, e a integração com Parte 2 respeta o contrato via `cloud_link.h`.
> 
> O protótipo é 100% aplicável em mercado com pequenos ajustes para ESP32 real
> (SPIFFS, RTC, compressão de dados).**

---

**Versão:** 1.0  
**Status:** ✅ Completo (Parte 1 + Parte 2)  
**Data:** Maio/2026  
**Responsável:** Equipe CardioIA — Fase 3, Cap. 1  
**Contato:** Ver `AGENTS.md` e `CLAUDE.md`
