# CardioIA — Fase 3, Cap. 1 / Parte 2
## Transmissão MQTT e Visualização em Dashboards

**Aluno:** Lucas Francelino
**Data:** 09/05/2026

---

## 1. Visão geral

A Parte 2 do projeto CardioIA cobre a camada Fog/Cloud do sistema de
monitoramento contínuo: pegar os sinais vitais já capturados pelo ESP32 na
Parte 1 e fazê-los subir para a nuvem via **MQTT**, sendo consumidos por
um dashboard em **Node-RED** (com gráfico, gauge e alertas) e, como
extensão opcional, replicados em **Grafana Cloud** via InfluxDB Cloud.

O sensor obrigatório é o **DHT22** (temperatura + umidade). Como segundo
sensor, escolhemos um **botão como simulador de batimentos cardíacos** —
cada toque conta como uma batida e o BPM é calculado por janela móvel
de 5 segundos. Os limites de alerta seguem a sugestão do enunciado:
**BPM > 120** (taquicardia) e **temperatura > 38 °C** (febre).

### Arquitetura

```
┌──────────────┐  WiFi (sim)   ┌────────────┐  MQTT/TLS  ┌──────────┐
│  ESP32 (P1)  │──────────────▶│ Wi-Fi flag │───────────▶│ HiveMQ   │
│ DHT22 + btn  │   (booleano)  │ + buffer   │   :8883    │  Cloud   │
└──────────────┘               └────────────┘            └────┬─────┘
                                                              │
                                                  ┌───────────┴──────────┐
                                                  ▼                      ▼
                                          ┌──────────────┐      ┌────────────────┐
                                          │  Node-RED    │      │ InfluxDB Cloud │
                                          │  Dashboard   │      │  → Grafana     │
                                          │ chart+gauge  │      │     Cloud      │
                                          │   +alerta    │      │                │
                                          └──────────────┘      └────────────────┘
```

---

## 2. Comunicação MQTT

### 2.1 Broker

Foi escolhido o **HiveMQ Cloud Serverless (free tier)** por três razões:

1. **TLS 1.2 nativo na porta 8883** — autenticação obrigatória por
   usuário/senha + criptografia ponto-a-ponto, requisito mínimo para
   dados de saúde.
2. **Plano gratuito** com até 100 conexões simultâneas e 10 GB/mês de
   tráfego — suficiente para um vestível enviando 1 mensagem a cada
   5 segundos (~17 280 mensagens/dia, ~5 KB/dia).
3. **Web Client integrado** — facilita inspeção e debug durante o
   desenvolvimento sem instalar ferramentas extras.

A conexão TLS é validada usando o certificado raiz **Let's Encrypt
ISRG Root X1**, que está embutido no firmware (`cloud_link.cpp` →
`ISRG_ROOT_X1`).

### 2.2 Estrutura de tópicos

| Tópico                              | Direção         | QoS | Retain | Conteúdo                               |
|-------------------------------------|-----------------|-----|--------|----------------------------------------|
| `cardioia/<deviceId>/telemetry`     | ESP32 → broker  | 1   | não    | Pacote JSON com leituras (ver 2.3).    |
| `cardioia/<deviceId>/status`        | ESP32 → broker  | 1   | sim    | `online` / `offline` (LWT).            |

A escolha de **QoS 1** (entrega ao menos uma vez) equilibra confiabilidade
com simplicidade — em saúde digital uma duplicata ocasional é tolerável,
enquanto perda silenciosa de dados não é. **Retain é desabilitado** em
`telemetry` porque o dashboard só deve mostrar valores frescos; `status`,
ao contrário, usa retain para que assinantes que conectem depois saibam
imediatamente se o dispositivo está online.

O **Last-Will Testament (LWT)** publica `offline` em `status` se o broker
detectar queda da conexão sem desconexão limpa. Isso permite que o
dashboard distinga "sem dados porque o paciente está bem" de "sem dados
porque o vestível caiu".

### 2.3 Formato do payload

Um único schema JSON serve tanto para leituras ao vivo quanto para o
flush do buffer offline da Parte 1:

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

O campo `buffered` permite ao consumidor distinguir entre leitura em
tempo real (`false`) e leitura recuperada do buffer offline (`true`),
útil para visualização e auditoria.

### 2.4 Contrato Parte 1 ↔ Parte 2

A integração entre as duas partes do projeto é feita por meio de um
header C++ compartilhado (`firmware/include/cloud_link.h`). A Parte 1 é
dona da leitura dos sensores e do buffer offline; a Parte 2 é dona da
conexão Wi-Fi e MQTT. O único ponto de acoplamento é a função
`enviarParaNuvem(const Reading& r)` que a Parte 1 chama quando precisa
publicar — sem precisar saber nada de MQTT.

Esse desacoplamento permite que cada lado seja desenvolvido, testado e
substituído de forma independente.

---

## 3. Dashboard Node-RED

### 3.1 Estrutura do flow

O flow (`node-red/flows.json`) tem três blocos principais:

1. **Ingestão** — `mqtt in (cardioia/+/telemetry)` → `parse JSON` →
   debug node lateral.
2. **Visualização** — duas saídas paralelas: `extract bpm` →
   `ui_chart` (linha de 5 min, faixa 40–180 bpm) e `extract temp` →
   `ui_gauge` (escala 30–42 °C, zonas verde/amarela/vermelha em
   37,5 / 38).
3. **Alerta** — função JavaScript que avalia `bpm > 120 || temp > 38`,
   gera texto explicativo e pinta o widget de status em vermelho ou
   verde. A função descarta payloads malformados sem quebrar o flow,
   o que é essencial para tolerar mensagens corrompidas vindas do
   ESP32 ou do mock publisher de testes.

Os limites são parametrizados via **context global** do Node-RED
(`global.BPM_MAX`, `global.TEMP_MAX`), permitindo ajuste sem redeploy.

### 3.2 Regras de alerta — justificativa clínica

- **BPM > 120**: corresponde a taquicardia em adulto em repouso
  (referência: faixa normal 60–100 bpm; acima de 100 = taquicardia,
  acima de 120 = sinal clinicamente relevante mesmo sob stress moderado).
- **Temperatura > 38 °C**: limiar consensual para definição de febre.

Os limites são intencionalmente conservadores — em produção real,
seriam personalizados por paciente em coordenação com a equipe médica.

### 3.3 Robustez e testabilidade

O flow inclui dois recursos para validação rápida:

- **Inject node "MOCK alerta"** (desabilitado por padrão) — dispara um
  payload de alerta direto na entrada do parser, sem precisar do
  broker. Útil para smoke test após qualquer mudança no layout.
- **Mock publisher Python** (`scripts/mock_publisher.py`) — gera fluxos
  realistas de telemetria com seis cenários (normal, taquicardia,
  febre, tudo, offline-flush, ruido). Permite validar a stack inteira
  HiveMQ → Node-RED → Grafana **sem precisar do firmware da Parte 1**.

---

## 4. Bônus — Grafana Cloud

A integração com Grafana Cloud foi **implementada** pelo caminho mais
direto e gratuito: **Node-RED escreve em InfluxDB Cloud (free) → Grafana
Cloud consome InfluxDB como datasource**.

| Por que não usar o plugin MQTT direto do Grafana? | É **pago** (marketplace partner). O caminho via InfluxDB usa apenas free tiers. |

### 4.1 Pipeline implementado

```
ESP32/Mock ──MQTT/TLS──▶ HiveMQ ──▶ Node-RED ──┐
                                                ├─▶ Dashboard Node-RED (já mostrado em §3)
                                                │
                                                └─▶ InfluxDB Cloud (bucket "cardioia",
                                                       measurement "vitals", fields:
                                                       temp, hum, bpm) ──▶ Grafana Cloud
```

No `flows.json` o nó `InfluxDB Cloud (opcional)` ficou habilitado depois
de configurado com URL do cluster, organização, bucket `cardioia` e
token de escrita. A query Flux usada nos painéis é:

```flux
from(bucket: "cardioia")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "vitals")
  |> filter(fn: (r) => r._field == "bpm" or r._field == "temp")
```

### 4.2 Resultado

O dashboard público está disponível em
**https://hinten.grafana.net/public-dashboards/1fc63ea0bb144546ac549fc34fb3b542**
e exibe os mesmos dados que o painel Node-RED, com a vantagem de
agregação e histórico nativos do Grafana.

![Dashboard Grafana CardioIA](../assets/grafana_dashboard.png)

A latência típica entre publicação no broker e ponto visível no Grafana
é de **< 10 segundos** (limitada principalmente pelo flush do InfluxDB
Cloud Serverless free tier).

### 4.3 Limitação de embedding

Grafana **Cloud** não permite habilitar `allow_embedding`, ou seja, não
é possível incorporar o dashboard via `<iframe>` em sites externos. A
alternativa adotada foi o **public dashboard** (URL acima) — qualquer
pessoa com o link abre direto, sem login.

### 4.4 Documentação de setup

O passo-a-passo completo (criar conta InfluxDB, gerar token, configurar
Node-RED, criar conta Grafana, adicionar datasource, montar painéis,
tornar dashboard público) está em [`grafana_setup.md`](grafana_setup.md).

---

## 5. Segurança e privacidade

Embora seja um protótipo educacional, o projeto adota práticas
mínimas de segurança coerentes com dados de saúde:

- **TLS obrigatório** em todas as conexões (broker MQTT e InfluxDB).
- **Credenciais fora do versionamento**: arquivos `secrets.h` e `.env`
  estão no `.gitignore`; o repositório só tem `secrets.h.example`.
- **Cliente ID determinístico** (`DEVICE_ID`) — em produção real,
  cada vestível teria credenciais únicas (princípio do menor
  privilégio); aqui simplificamos para um único par.
- **Tópicos por dispositivo** (`cardioia/<deviceId>/...`) — base
  natural para ACLs no broker quando escalar.

---

## 6. Conclusão

A Parte 2 entrega o ciclo completo Edge → Fog → Cloud do CardioIA:
o ESP32 publica via MQTT/TLS no HiveMQ Cloud; o Node-RED visualiza em
tempo real com chart, gauge e alerta de threshold; e o Grafana Cloud
fornece visualização avançada como bônus.

A maior decisão de design foi o **contrato mínimo entre as duas partes
do projeto** (`cloud_link.h` + payload JSON único): permite que a
Parte 1 e a Parte 2 evoluam em paralelo, com testes isolados em
ambos os lados graças ao mock publisher e ao stub `main.cpp`.

A documentação de testes (seção `Testes` no README do projeto e nos
READMEs específicos de `scripts/` e `node-red/`) descreve como validar
cada camada e cada cenário, mapeando explicitamente os critérios de
avaliação do enunciado.
