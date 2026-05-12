# Parte 1: Edge Computing & Resiliência Offline — CardioIA

## Visão Geral

Este diretório contém a implementação da **Parte 1** do projeto CardioIA em um ESP32 (simulado no Wokwi). O objetivo é:

1. **Capturar sinais vitais** a partir de sensores (DHT22 + botão)
2. **Armazenar localmente** em buffer circular para resiliência offline  
3. **Sincronizar com a nuvem** quando conectado (integração com Parte 2)
4. **Demonstrar Edge Computing** em aplicações críticas de saúde

## Estrutura

```
firmware/
├── platformio.ini           # Configuração PlatformIO (dependências + targets)
├── include/
│   ├── cloud_link.h         # Interface Parte 1 ↔ Parte 2 (contrato)
│   ├── secrets.h.example    # Template de credenciais (copiar → secrets.h)
│   └── secrets.h            # NUNCA commitar (credenciais locais)
└── src/
    ├── main.cpp             # **PARTE 1**: Sensores + buffer offline
    ├── cloud_link.cpp       # PARTE 2: MQTT/TLS/Wi-Fi (já implementada)
    └── ai_analysis.ipynb    # (Opcional) Análise IA em Jupyter
```

## Sensores Utilizados

| Sensor | Pino | Medição | Observações |
|--------|------|---------|-------------|
| **DHT22** | GPIO 4 | Temp (°C) + Umidade (%) | Obrigatório, estabiliza em ~1s |
| **Botão** | GPIO 5 | BPM simulado | INPUT_PULLUP, transições LOW=beat |

### Cálculo de BPM

```
BPM = (batidas_em_5s × 60000) / 5000 = batidas × 12
Exemplo: 5 cliques em 5s = 60 BPM
```

## Buffer Offline (Ring Buffer circulante)

### Estrutura

```cpp
struct Reading {
  uint32_t ts;       // Timestamp (s)
  float temp;        // Temperatura (°C)
  float hum;         // Umidade (%)
  int bpm;           // Batimentos/min
  bool buffered;     // Flag: recuperado do buffer?
};
```

### Capacidade

- **Tamanho**: 100 amostras
- **Intervalo**: 5 segundos
- **Duração**: ~8 minutos de dados offline
- **Política**: FIFO (sobrescreve antigas se > 100)

### Fluxo Lógico

**ONLINE** (`cloudIsConnected() = true`):
1. Publica leitura atual vie MQTT
2. Se falhar, bufferiza para tentar depois
3. Sincroniza buffer (marca `buffered=true`, envia todos)

**OFFLINE** (`cloudIsConnected() = false`):
1. Armazena leitura no buffer circulante
2. Continua lendo sensores normalmente (resiliente)
3. Aguarda reconexão

## Instalação & Setup

### Pré-requisitos

- Python 3.8+
- pip (para instalar PlatformIO)
- VS Code com extensão PlatformIO (opcional, mas recomendado)

### Passo 1: Preparar credenciais

```bash
# Windows PowerShell (ou terminal equivalente)
cd firmware

# Copiar template de credenciais
Copy-Item include\secrets.h.example include\secrets.h

# Editar secrets.h com suas credenciais:
# - WIFI_SSID / WIFI_PASSWORD (para Wokwi: "Wokwi-GUEST" / "")
# - MQTT_HOST / MQTT_USER / MQTT_PASSWORD (HiveMQ Cloud)
# - DEVICE_ID (ex: "cardioia-01")
```

### Passo 2: Compilar com PlatformIO

```bash
# Instalar PlatformIO (primeira vez)
pip install platformio

# Compilar firmware
cd firmware
pio run -e esp32dev

# Saída esperada:
# Flash: [=======   ]  68.2% (used 894461 bytes from 1310720 bytes)
# ========================= [SUCCESS] ===
```

### Passo 3: Carregar no Wokwi

**Opção A: Via Editor Wokwi (recomendado)**

1. Abra [wokwi.com](https://wokwi.com) e crie novo projeto ESP32
2. Adicione componentes ao `diagram.json`:
   - DHT22 no pino 4
   - Botão no pino 5 com pull-up
3. Copie conteúdo de `firmware/src/` e `firmware/include/` para o editor Wokwi
4. Copie `platformio.ini` (lib_deps) para `wokwi.toml`

**Opção B: Download do firmware compilado**

```bash
# Arquivo compilado em:
# firmware/.pio/build/esp32dev/firmware.bin
# firmware/.pio/build/esp32dev/bootloader.bin

# Usar esptool.py para carregar em ESP32 físico (se aplicável)
```

### Passo 4: Monitorar Serial

```bash
# Ver logs em tempo real
pio device monitor -e esp32dev --baud 115200

# Saída esperada:
# === CardioIA — Parte 1: Edge Computing & Resiliência ===
# [main] ✓ ts=10 temp=25.3°C hum=50.2% bpm=60 [ONLINE]
# [main] 📵 OFFLINE — buffering (ts=15, buffer=1/100)
# [main] 🔄 Sincronizando buffer (5 leituras)...
```

## Cenários de Teste

### Teste 1: Leitura Normal (online o tempo todo)

```bash
# Simular Wi-Fi ativado no Wokwi
# → Observar leituras publicadas a cada 5s
# → Serial mostra: [ONLINE] publish=OK
```

### Teste 2: Desconexão e Reconexão

```bash
# 1. Deixar online por 10s (publica normalmente)
# 2. Desativar Wi-Fi no Wokwi (simular offline)
# 3. Observar buffer crescer: buffer=1/100, buffer=2/100, ...
# 4. Reativar Wi-Fi
# 5. Verificar sincronização: buffered=true aparece no Serial

Serial:
[main] 📵 OFFLINE — buffering (ts=50, buffer=5/100)
[main] 🔄 Sincronizando buffer (5 leituras)...
[main]   ✓ buffered ts=30 bpm=65
[main]   ✓ buffered ts=35 bpm=70
[main] Sincronização completa: 5/5 enviadas
```

### Teste 3: Buffer Cheio (> 100 amostras)

```bash
# 1. Desconectar por > 8 minutos
# 2. Buffer fica cheio (100 amostras)
# 3. Novas leituras sobrescrevem as antigas
# 4. Reconectar: últimas 100 são sincronizadas (perda aceitável em saúde não-crítica)

Serial:
[main] 📵 OFFLINE — buffering (ts=500, buffer=100/100)
# → Próximas leituras sobrescrevem índices antigos
```

## Integração com Parte 2

### Interface `cloud_link.h`

A Parte 1 **sempre** usa estas 4 funções:

```cpp
// Setup inicial (chama uma vez)
bool cloudBegin(const char* ssid, const char* password);

// Loop principal (chama a cada iteração)
bool cloudLoop();

// Publica uma leitura (Parte 1 verifica retorno)
bool enviarParaNuvem(const Reading& r);

// Consulta conectividade (decide se publica ou bufferiza)
bool cloudIsConnected();
```

### Schema JSON (tópico `cardioia/<deviceId>/telemetry`)

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

- **ts**: Timestamp Unix (s) — ou millis() se sem RTC
- **buffered**: `true` = recuperado do buffer offline
- **QoS**: 1 (at-least-once, tolerável em saúde)
- **Retain**: `false` (apenas dados frescos no dashboard)

## Logs & Debug

### Prefixos de Módulo

```
[main]   → Lógica Parte 1 (sensores, buffer, sync)
[wifi]   → Conectividade Wi-Fi (Parte 2)
[mqtt]   → Broker MQTT (Parte 2)
[dht]    → Confirmações do sensor
```

### Mensagens Comuns

| Mensagem | Significado |
|----------|-------------|
| `✓ ts=10 [ONLINE]` | Leitura enviada com sucesso online |
| `✗ publish falhou` | MQTT falhou, leitura bufferizada |
| `📵 OFFLINE` | Sem conexão, armazenando localmente |
| `🔄 Sincronizando` | Reconectado, flushing buffer |
| `⚠ DHT NaN` | DHT22 instável, descartando leitura |

## Troubleshooting

| Problema | Solução |
|----------|---------|
| **Compilação falha** | Verificar `platformio.ini`, rodar `pio lib list`, limpar `.pio/` |
| **DHT retorna NaN** | Normal nos primeiros 1-2s. Aumentar `delay(1000)` em setup() |
| **"rc=-4" no MQTT** | Wi-Fi desconectou. Verificar SSID/senha em `secrets.h` |
| **Buffer não sincroniza** | Verificar `cloudIsConnected()` retorna true. Aguardar 5s |
| **Serial vazio** | Bottar ESP32, verificar baud rate (115200), check USB cabo |

## Referências

- **DHT22 Datasheet**: Precisão ±0.5°C, período mín 2s entre leituras
- **Wokwi Docs**: https://wokwi.com/docs/guides/esp32
- **MQTT QoS 1**: Garantia at-least-once (mensagens podem duplicar)
- **Edge Computing**: Processa localmente, sincroniza com nuvem quando possível

## Próximos Passos

- Substituir buffer RAM por **SPIFFS** em ESP32 físico (persistência)
- Adicionar **compressão** de dados para reduzir banda
- Implementar **criptografia TLS 1.3** (já suportada via `cloud_link.cpp`)
- Integrar com **Node-RED** para visualização em tempo real (ver `node-red/`)

---

**Status**: ✅ Parte 1 completa e integrada com Parte 2 (MQTT)  
**Última atualização**: 2026-05  
**Responsável**: Equipe CardioIA (Fase 3, Cap. 1)

