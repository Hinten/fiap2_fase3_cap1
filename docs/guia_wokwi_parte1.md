# Guia de Integração: Firmware Parte 1 no Wokwi

## Visão Geral

Este guia explica como integrar o código compilado da Parte 1 do CardioIA no simulador Wokwi (https://wokwi.com).

---

## Opção 1: Importar Projeto Completo (Recomendado para iniciantes)

### Passo 1: Preparar Arquivos

```bash
cd firmware
pio run -e esp32dev  # Compila projeto
```

Arquivo gerado: `.pio/build/esp32dev/firmware.bin`

### Passo 2: Criar Projeto Wokwi

1. Acesse https://wokwi.com
2. Crie novo projeto → **ESP32 + Breadboard**
3. No painel esquerdo, crie arquivo `diagram.json`:

```json
{
  "version": 1,
  "author": "CardioIA Team",
  "title": "CardioIA — Parte 1 (Edge Computing)",
  "description": "Monitoramento cardíaco com buffer offline",
  "files": [
    "main.cpp",
    "cloud_link.cpp",
    "cloud_link.h",
    "secrets.h",
    "platformio.ini"
  ],
  "parts": [
    {
      "type": "wokwi-esp32-devkit-v1",
      "id": "esp32",
      "top": 0,
      "left": 0,
      "attrs": { "env": "esp32dev" }
    },
    {
      "type": "wokwi-dht22",
      "id": "dht",
      "top": 100,
      "left": 350,
      "attrs": { "temperature": "25" }
    },
    {
      "type": "wokwi-pushbutton",
      "id": "btn",
      "top": 250,
      "left": 350,
      "attrs": { "color": "red" }
    }
  ],
  "connections": [
    [
      "esp32:GND",
      "dht:GND",
      "black"
    ],
    [
      "esp32:3V3",
      "dht:VCC",
      "red"
    ],
    [
      "esp32:4",
      "dht:SDA",
      "yellow"
    ],
    [
      "esp32:5",
      "btn:1.pin",
      "green"
    ],
    [
      "esp32:GND",
      "btn:2.pin",
      "black"
    ]
  ]
}
```

### Passo 3: Upload de Código

1. Copie conteúdo de `firmware/src/main.cpp` → Editor Wokwi, arquivo `main.cpp`
2. Copie conteúdo de `firmware/src/cloud_link.cpp` → arquivo `cloud_link.cpp`
3. Copie conteúdo de `firmware/include/cloud_link.h` → arquivo `cloud_link.h`
4. Copie conteúdo de `firmware/include/secrets.h` → arquivo `secrets.h` (edite com suas credenciais)
5. Copie conteúdo de `firmware/platformio.ini` → arquivo `platformio.ini`

### Passo 4: Executar

1. Pressione Play (▶️) no Wokwi
2. Abra Serial Monitor (Ctrl+Shift+J)
3. Observe logs em tempo real

**Output esperado:**
```
=== CardioIA — Parte 1: Edge Computing & Resiliência ===

[main] Inicializando DHT22 e botão...
[main] Iniciando cloud_link (Wi-Fi + MQTT)...
[main] Setup completo. Entrando no loop principal.

[wifi] conectando em "Wokwi-GUEST"...
[wifi] OK ip=192.168.1.100
[mqtt] conectando em SEU-CLUSTER.s2.eu.hivemq.cloud:8883 ...
[mqtt] OK

[main] ✓ ts=10 temp=25.3°C hum=50.2% bpm=60 [ONLINE]
[main] ✓ ts=15 temp=25.4°C hum=50.1% bpm=72 [ONLINE]
```

---

## Opção 2: Usar Link Compartilhado (Se já existe projeto criado)

Se você tem um projeto Wokwi já criado com o CardioIA:

1. Compartilhe o link do projeto
2. Usuários abrem link e veem circuito + código pronto para rodar
3. Botão verde Play (▶️) para simular

**Exemplo de link:**
```
https://wokwi.com/projects/YOUR-PROJECT-ID
```

---

## Pinos: Mapeamento

| Componente | Pino ESP32 | Função | Wokwi |
|---|---|---|---|
| **DHT22** | GPIO 4 | SDA (Dados) | Vermelho = VCC, Amarelo = SDA, Preto = GND |
| **Botão** | GPIO 5 | Digital | Verde = clique (LOW), Preto = GND |
| **GND** | GND | Terra | Preto |
| **VCC** | 3V3 | Alimentação | Vermelho |

### Visual no Breadboard Wokwi

```
┌─────────────┐
│    ESP32    │
│ [3V3]  [GND]│───────▶ GND (Preto)
│ [ 4]        │───────▶ DHT22 SDA (Amarelo)
│ [ 5]        │───────▶ Botão (Verde)
└─────────────┘
     ║
     ║ WiFi Simulado
     ░░ (Wokwi-GUEST)
```

---

## Teste: Simulação de Online/Offline

### Simular Desconexão Wi-Fi

1. No Wokwi, pressione **Ctrl+Shift+M** para abrir menu simulador
2. Opção: **"Disconnect WiFi"** ou desativar Wi-Fi pelo botão
3. Observe Serial:
   ```
   [main] 📵 OFFLINE — buffering (ts=50, buffer=1/100)
   [main] 📵 OFFLINE — buffering (ts=55, buffer=2/100)
   ```

### Simular Reconexão Wi-Fi

1. Reativar Wi-Fi (menu ou botão)
2. Observe sincronização:
   ```
   [main] 🔄 Sincronizando buffer (2 leituras)...
   [main]   ✓ buffered ts=50 bpm=62
   [main]   ✓ buffered ts=55 bpm=64
   [main] Sincronização completa: 2/2 enviadas
   ```

---

## Teste: Cliques no Botão (BPM)

### Simular Batidas Cardíacas

1. No circuito Wokwi, localize o botão (vermelho)
2. Clique 5 vezes rápido (simular 5 batidas em ~5s)
3. Aguarde próxima publicação (5s)
4. Observe Serial:
   ```
   [main] ✓ ts=100 temp=25.3°C hum=50.2% bpm=60 [ONLINE]
   ```
   (5 cliques × 12 = 60 BPM)

### Interpretação

| BPM Calculado | Cliques/5s | Interpretação |
|---|---|---|
| 0 | 0 | Sem cliques (normal em repouso, buscando sinal) |
| 60 | 5 | Normal |
| 120+ | 10+ | Taquicardia (alerta no dashboard) |

---

## Teste: DHT22 (Temperatura)

### Alterar Temperatura Simulada

1. Clique no DHT22 no circuito
2. Painel direita mostra "Temperature: 25 °C"
3. Altere para 38 °C (simulando febre)
4. Observe Serial:
   ```
   [main] ✓ ts=150 temp=38.0°C hum=50.2% bpm=60 [ONLINE]
   ```

### Dashboard Node-RED

Se tiver Parte 2 rodando (Node-RED):
- Temperatura aparece no gauge em tempo real
- Se > 38°C: indicador de alerta muda para vermelho 🔴

---

## Troubleshooting: Problemas Comuns

### Serial Monitor Vazio

**Problema:** Nenhum output no Serial.

**Solução:**
1. Verificar baud rate: **115200** (canto inferior Serial do Wokwi)
2. Pressionar Play novamente
3. Verificar `Serial.begin(115200)` em `setup()`

### DHT NaN em Logs

**Problema:**
```
[main] ⚠ DHT22 retornou NaN, pulando publicação
```

**Solução:**
- Normal nos primeiros 1-2 segundos (DHT22 estabilizando)
- Aguardar 2s, próxima leitura OK
- Se persistir: verificar pino GPIO 4 no diagram.json

### Cannot Connect to MQTT Broker

**Problema:**
```
[mqtt] conectando em SEU-CLUSTER.s2.eu.hivemq.cloud:8883 ...
[mqtt] falhou rc=-4
```

**Solução:**
1. Verificar credenciais em `secrets.h`:
   - MQTT_HOST correto?
   - MQTT_USER / MQTT_PASSWORD corretos?
   - DEVICE_ID único?
2. Verificar Wi-Fi Wokwi: "Wokwi-GUEST" está ativado?
3. Testear broker com HiveMQ Web Client (https://www.hivemq.com/mqtt-websocket-client/)

### Botão Não Registra Cliques

**Problema:** Clico no botão mas BPM fica 0.

**Solução:**
1. Certificar pino 5 correto no diagram.json (GPIO 5)
2. INPUT_PULLUP ativo em `pinMode(BUTTON_PIN, INPUT_PULLUP)`
3. Aguardar ciclo de 5s para leitura publicar BPM

---

## Integração: Node-RED + Wokwi

### Setup Completo

```
┌──────────────┐
│   Wokwi      │
│ (ESP32+Fwm)  │─── MQTT TLS ───┬──────────────────┐
└──────────────┘                 │   HiveMQ Cloud   │
                                 │ (Broker Público) │
                                 └──┬───────────────┘
                                    │
                                    └─── MQTT Subscribe
                                         ↓
                                   ┌──────────────┐
                                   │  Node-RED    │
                                   │ (Dashboard)  │
                                   │ localhost:   │
                                   │   1880/ui    │
                                   └──────────────┘
```

### Como Rodar Junto

1. **Terminal 1:** Rodar Node-RED
   ```bash
   node-red
   ```

2. **Terminal 2:** Abrir Wokwi e pressionar Play

3. **Browser:** 
   - Wokwi serial: http://localhost:3000 (ou URL do Wokwi online)
   - Node-RED dashboard: http://localhost:1880/ui

4. **Resultado:**
   - Lê DHT22 + Botão em Wokwi
   - Publica JSON no HiveMQ
   - Node-RED subscreve e atualiza gráfico em tempo real

---

## Próximos Passos

### Passar para Produção (ESP32 Físico)

1. Substituir buffer RAM por **SPIFFS** (persistente)
   ```cpp
   #include <SPIFFS.h>
   // Inicializar: SPIFFS.begin(true);
   ```

2. Usar credenciais de **Wi-Fi real** (não Wokwi-GUEST)

3. Adicionar **RTC** (DS3231) para timestamp preciso

4. Implementar **compressão** (ZLib) para reduzir tráfego MQTT

---

## Referências

- **Wokwi Docs:** https://wokwi.com/docs/guides/esp32
- **HiveMQ Cloud:** https://www.hivemq.com/mqtt-cloud-broker/
- **Node-RED:** https://nodered.org
- **DHT22 Spec:** https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf

---

**Status:** ✅ Guia completo  
**Última atualização:** Maio/2026  
**Autores:** Equipe CardioIA

