# Mock Publisher — testes da Parte 2 sem ESP32

Gera telemetria fake e publica no broker HiveMQ Cloud nos mesmos tópicos
do firmware real. Use para validar Node-RED + Grafana antes/independente
do código da Parte 1.

## Setup (primeira vez)

```powershell
# A partir da pasta scripts/

# 1) cria virtualenv local
python -m venv .venv

# 2) ativa o venv (Windows PowerShell)
.\.venv\Scripts\Activate.ps1
# Se aparecer erro "a execução de scripts foi desabilitada", rode antes:
#   Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
# Para CMD (em vez de PowerShell):
#   .\.venv\Scripts\activate.bat
# Para Linux/macOS:
#   source .venv/bin/activate

# 3) instala paho-mqtt + python-dotenv
pip install -r requirements.txt

# 4) copia o template de credenciais e edita
Copy-Item .env.example .env
notepad .env
```

Com o venv ativo, o prompt mostra `(.venv)` no começo.

## Sessões futuras

```powershell
cd scripts
.\.venv\Scripts\Activate.ps1   # só reativa, sem reinstalar
python mock_publisher.py --scenario normal --duration 30
```

Se ver `ModuleNotFoundError: No module named 'paho'`, é sinal de que o
venv não está ativo (ou você nunca fez `pip install`).

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
