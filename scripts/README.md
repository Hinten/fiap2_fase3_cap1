# Mock Publisher — testes da Parte 2 sem ESP32

Gera telemetria fake e publica no broker HiveMQ Cloud nos mesmos tópicos
do firmware real. Use para validar Node-RED + Grafana antes/independente
do código da Parte 1.

## Setup

```powershell
# A partir da pasta scripts/
python -m venv .venv
.\.venv\Scripts\Activate.ps1     # Windows PowerShell
pip install -r requirements.txt

cp .env.example .env             # editar com creds do HiveMQ
```

## Cenários

| `--scenario`     | O que faz                                                |
|------------------|----------------------------------------------------------|
| `normal`         | bpm 70–85, temp 36.5–37.0 (caso feliz)                   |
| `taquicardia`    | bpm sobe 80 → 140 (testa alerta > 120 bpm)               |
| `febre`          | temp sobe 37 → 39 (testa alerta > 38 °C)                 |
| `tudo`           | os dois alertas ao mesmo tempo                           |
| `offline-flush`  | rajada de 10 mensagens com `buffered=true` (sync da P1)  |
| `ruido`          | 1% de payloads malformados (testa robustez do parser)    |

## Exemplos

```powershell
# 30 mensagens normais ao longo de 30s
python mock_publisher.py --scenario normal --duration 30

# Forçar alerta de bpm em ~30s
python mock_publisher.py --scenario taquicardia --duration 30

# Simular sync de buffer offline (P1) — manda 10 em rajada
python mock_publisher.py --scenario offline-flush

# Stream rápido pra estressar o dashboard (5 msg/s por 1 min)
python mock_publisher.py --scenario tudo --rate 5 --duration 60
```

## Verificação rápida

Após rodar qualquer cenário:

1. Abra **HiveMQ Cloud Web Client** e assine `cardioia/#` — deve ver o JSON.
2. Com Node-RED rodando: dashboard `/ui` deve atualizar; alertas devem
   acender nos cenários `taquicardia` / `febre` / `tudo`.
3. Se Grafana estiver configurado: o painel deve mostrar pontos novos
   em até ~10s (latência típica do InfluxDB Cloud).
