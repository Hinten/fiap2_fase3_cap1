# Ir Além 1 — REST API + Alertas por E-mail

Extensão bônus do projeto CardioIA: um servidor Flask que expõe uma API REST com sinais vitais simulados e um monitor Python que consome essa API, detecta riscos clínicos e envia alertas automatizados por e-mail.

## Arquitetura

```
api_server.py  ←── GET /patients/<id>/vitals ───  monitor.py
(Flask REST)                                       (verifica thresholds)
                                                         │
                                                         ▼
                                                   E-mail de alerta
                                                   (SMTP / Gmail)
```

## Estrutura

```
ir_alem_1/
  api/
    api_server.py      Servidor Flask com rotas de sinais vitais e alertas
  monitor/
    monitor.py         Loop de monitoramento: consome API, detecta risco, envia e-mail
  shared/
    config.py          Configurações (URL da API, thresholds, credenciais e-mail)
  docker-compose.yml   Sobe API + monitor em containers
  requirements.txt     Dependências Python (flask, requests)
```

## Como executar

### Opção A — Docker Compose (recomendado)

```bash
cd ir_alem_1

# Configurar variáveis de ambiente
cp ../scripts/.env.example .env
# Editar .env:
#   SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASSWORD
#   EMAIL_FROM, EMAIL_TO
#   SCENARIO: normal | tachycardia | fever | hypoxia | no_movement | critical

docker compose up
```

O `api_server` sobe em `http://localhost:5000` e o `monitor` começa a consultar imediatamente.

### Opção B — Sem Docker

```bash
cd ir_alem_1
pip install -r requirements.txt

# Terminal 1 — API
python api/api_server.py

# Terminal 2 — Monitor
python monitor/monitor.py
```

## Rotas da API

| Método | Rota | Descrição |
|--------|------|-----------|
| `GET` | `/health` | Health check |
| `GET` | `/patients/<id>/vitals` | Sinais vitais simulados (JSON) |
| `POST` | `/patients/<id>/alerts` | Registra um alerta |
| `GET` | `/alerts` | Lista todos os alertas registrados |

## Thresholds de risco

| Parâmetro | Limite | Condição de alerta |
|-----------|--------|-------------------|
| `heart_rate` | > 100 bpm | Taquicardia |
| `temperature` | > 38.0 °C | Febre |
| `spo2` | < 92 % | Hipóxia |
| `seconds_without_movement` | > 300 s | Ausência de movimento |

## Cenários de simulação

Configurável via variável `SCENARIO` no `.env`:

- `normal` — sinais dentro dos limites normais
- `tachycardia` — frequência cardíaca elevada (105–140 bpm)
- `fever` — temperatura acima de 38 °C
- `hypoxia` — SpO2 baixo (80–91%)
- `no_movement` — sem movimento por mais de 5 minutos
- `critical` — múltiplos parâmetros críticos simultâneos
