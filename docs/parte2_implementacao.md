# Plano implementado — Parte 2 (CardioIA)

Checklist do que foi entregue. Use como conferência para validar a entrega
antes de testar end-to-end.

## Decisões de escopo (já alinhadas com você)

- [x] Visualização: **Node-RED + Grafana Cloud** (Grafana via InfluxDB Cloud, item bônus do enunciado).
- [x] Integração com Parte 1: **contrato definido** (`cloud_link.h` + payload JSON único + função `enviarParaNuvem()`). Nada combinado previamente — você passa este header pra outra pessoa.
- [x] Sensor 2: **botão como BPM** (sugerido pelo enunciado).
- [x] **Next.js / React fora do escopo** — pesquisa confirmou que é compatível tecnicamente, mas não pontua e adiciona deploy de servidor Node persistente.

## Arquivos criados

### Firmware ESP32 (`firmware/`)

- [x] `platformio.ini` — libs `PubSubClient`, `ArduinoJson`, `DHT sensor library`, `Adafruit Unified Sensor`.
- [x] `include/cloud_link.h` — contrato P1↔P2: `struct Reading` + `cloudBegin / cloudLoop / enviarParaNuvem / cloudIsConnected`.
- [x] `include/secrets.h.example` — template de credenciais (Wi-Fi + HiveMQ + DEVICE_ID).
- [x] `src/cloud_link.cpp` — implementação MQTT/TLS:
  - [x] Cert raiz Let's Encrypt ISRG X1 embutido em PROGMEM.
  - [x] `WiFiClientSecure` + `PubSubClient` (buffer 512, keepalive 30s).
  - [x] LWT em `cardioia/<id>/status` (`offline` se queda; publica `online` no connect).
  - [x] Reconexão não-bloqueante com backoff de 5s.
  - [x] Serialização JSON via ArduinoJson, QoS 1, retain=false.
- [x] `src/main.cpp` — stub testável:
  - [x] DHT22 em GPIO 4, botão em GPIO 5 (INPUT_PULLUP).
  - [x] Conta cliques numa janela de 5s e calcula BPM.
  - [x] Descarta leituras `NaN` do DHT22.
  - [x] Comentado deixando claro que é descartável (Parte 1 substitui).

### Mock publisher Python (`scripts/`)

- [x] `requirements.txt` — `paho-mqtt`, `python-dotenv`.
- [x] `.env.example` — template (broker, porta, user, senha, device).
- [x] `mock_publisher.py` — CLI `--scenario <nome> --rate <hz> --duration <s>`:
  - [x] Cenário `normal` (caso feliz).
  - [x] Cenário `taquicardia` (BPM 80→140).
  - [x] Cenário `febre` (temp 37→39).
  - [x] Cenário `tudo` (ambos os alertas).
  - [x] Cenário `offline-flush` (rajada de 10 com `buffered=true`, simula sync da P1).
  - [x] Cenário `ruido` (1% malformado, testa robustez).
  - [x] TLS automático via bundle de CAs do sistema (não precisa baixar PEM).
  - [x] QoS 1 com `wait_for_publish` para garantir delivery.
- [x] `README.md` — setup PowerShell + tabela de cenários + exemplos.
- [x] **Validado**: `python -m py_compile` passa.

### Dashboard Node-RED (`node-red/`)

- [x] `flows.json` — 18 nós, validado como JSON:
  - [x] `mqtt-broker` configurado para HiveMQ (TLS porta 8883).
  - [x] `mqtt in cardioia/+/telemetry` (QoS 1, parse auto).
  - [x] `parse JSON` → `debug` (sidebar) + 4 saídas paralelas.
  - [x] `extract bpm` → `ui_chart` BPM (linha, 5 min, eixo 40–180).
  - [x] `extract temp` → `ui_gauge` Temperatura (gage, 30–42, zonas em 37,5 / 38).
  - [x] `regra de alerta` (função JS): thresholds via `global.BPM_MAX/TEMP_MAX` (default 120 / 38), descarta payload malformado sem quebrar.
  - [x] `ui_text` Status — colorido (verde/vermelho) com mensagem dinâmica.
  - [x] `MOCK alerta` — inject node desabilitado, dispara payload de alerta sem MQTT (smoke test).
  - [x] `InfluxDB Cloud` — nó desabilitado por padrão (habilitar e configurar URL/token/bucket pra ativar Grafana).
- [x] `README.md` — instruções de instalação Node-RED + plugins + import + smoke tests.

### Documentação (`docs/`)

- [x] `parte2_relatorio.md` — relatório ≥ 2 páginas em pt-BR:
  - [x] Página 1: arquitetura, broker (justificativa HiveMQ), tópicos, QoS, schema JSON, contrato P1↔P2.
  - [x] Página 2: Node-RED (estrutura), regras de alerta com **justificativa clínica** (taquicardia, febre), bônus Grafana via InfluxDB, segurança (TLS + creds fora do git).
- [x] `parte2_implementacao.md` — este arquivo.

### Raiz

- [x] `.gitignore` — protege `secrets.h`, `.env`, `*.pem`, `.pio/`, `__pycache__/`, etc.
- [x] `README.md` — visão geral, setup rápido, contrato P1↔P2 com exemplo de uso, **matriz de testes T1–T13**, **cenários E2E-A/B/C**, **checklist de aceitação** mapeado aos 6 pt do enunciado.

## Cobertura dos critérios de avaliação (enunciado)

| Critério                             | Pontos | Onde está coberto |
|--------------------------------------|--------|-------------------|
| Leitura de sensores                  | 2      | (Parte 1) — `main.cpp` stub é só pra teste da P2 |
| Resiliência offline                  | 2      | (Parte 1) — contrato pronto via `cloud_link.h` + cenário `offline-flush` valida o cabo |
| **Envio MQTT + broker**              | **2**  | `cloud_link.cpp` (TLS, LWT, QoS 1) + relatório §2 |
| **Dashboard funcional + alertas**    | **2**  | `flows.json` (chart + gauge + alerta) + relatório §3 |
| **Documentação**                     | **2**  | `docs/parte2_relatorio.md` + `README.md` + READMEs por pasta |
| Bônus — Grafana                      | extra  | ✅ implementado: nó InfluxDB no flow + dashboard público em [hinten.grafana.net/public-dashboards/1fc63ea0bb144546ac549fc34fb3b542](https://hinten.grafana.net/public-dashboards/1fc63ea0bb144546ac549fc34fb3b542) + screenshot em `assets/grafana_dashboard.png` + relatório §4 |

## Próximos passos manuais (você)

1. [ ] Criar conta HiveMQ Cloud Serverless e cluster.
2. [ ] `copy firmware\include\secrets.h.example firmware\include\secrets.h` e preencher.
3. [ ] `copy scripts\.env.example scripts\.env` e preencher.
4. [ ] Subir projeto Wokwi: ESP32 + DHT22 (GPIO 4) + botão (GPIO 5). Colar conteúdo de `firmware/`. Anotar link no README raiz.
5. [ ] `npm install -g node-red`, rodar `node-red`, instalar `node-red-dashboard`, importar `flows.json`, configurar nó HiveMQ, deploy.
6. [ ] Smoke test: `python scripts\mock_publisher.py --scenario tudo --duration 60` com dashboard aberto. Capturar prints (E2E-A, E2E-B).
7. [x] (Bônus) Criar InfluxDB Cloud + Grafana Cloud, habilitar nó `InfluxDB` no flow, importar dashboard. ✅ — dashboard público: https://hinten.grafana.net/public-dashboards/1fc63ea0bb144546ac549fc34fb3b542
8. [ ] Combinar com a pessoa da Parte 1: passar `cloud_link.h` + schema JSON + exemplo de uso do README.

## Como testar (resumo)

```powershell
# 1. Smoke test Node-RED sem broker
#    -> habilitar nó "MOCK alerta" no flows.json e clicar nele

# 2. Smoke test broker + Node-RED
cd scripts
python mock_publisher.py --scenario normal --duration 30

# 3. Forçar alerta de BPM
python mock_publisher.py --scenario taquicardia --duration 30

# 4. Forçar alerta de temp
python mock_publisher.py --scenario febre --duration 30

# 5. Simular sync do buffer da P1
python mock_publisher.py --scenario offline-flush

# 6. Stress test
python mock_publisher.py --scenario tudo --rate 5 --duration 60
```

Matriz completa T1–T13 está no `README.md` raiz.
