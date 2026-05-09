# Dashboard Node-RED — CardioIA

`flows.json` contém o flow completo: assinatura MQTT, parse, gráfico de BPM,
gauge de temperatura, indicador de alerta e (opcional) escrita no InfluxDB
Cloud para alimentar o Grafana.

## Pré-requisitos

```powershell
# Instalar Node-RED globalmente (uma vez)
npm install -g --unsafe-perm node-red

# Subir
node-red
# ou: npx node-red
```

Duas URLs diferentes — não confunda:

| URL                              | O que é                                                       |
|----------------------------------|---------------------------------------------------------------|
| http://127.0.0.1:1880            | **Editor** do Node-RED — onde você arrasta nós e configura.   |
| **http://127.0.0.1:1880/ui**     | **Dashboard** — chart, gauge e indicador de alerta. **É AQUI que os dados aparecem em tempo real.** |

⚠️ Erro comum: ficar olhando o editor esperando ver gráfico. O editor só mostra a topologia + o painel de debug. **Abra `/ui` numa segunda aba** quando rodar o mock publisher ou o ESP32.

### Plugins necessários

Pelo editor, em **Manage palette → Install**, adicione:

- `node-red-dashboard` — necessário para chart, gauge e text widgets.
- `node-red-contrib-influxdb` — **opcional**, só se quiser o ramo Grafana.

## Importar o flow

1. Editor → menu canto superior direito → **Import** → **select a file to import**
2. Escolha `flows.json` deste diretório.
3. Antes do **Deploy**, edite o nó `HiveMQ Cloud`:
   - **Server**: host do seu cluster (`xxxx.s2.eu.hivemq.cloud`)
   - **Port**: 8883, marcar **Use TLS**
   - **Security tab**: usuário/senha do HiveMQ.
4. Clique em **Deploy** (canto superior direito).

## Componentes do flow

| Nó                          | Função                                                            |
|----------------------------|--------------------------------------------------------------------|
| `mqtt in cardioia/+/telemetry` | Assina toda telemetria do CardioIA (qualquer deviceId).        |
| `parse JSON`               | Converte string em objeto.                                         |
| `telemetria recebida`      | Debug node — janela lateral mostra payload bruto.                  |
| `extract bpm` → `BPM`      | Extrai `payload.bpm` e plota no chart (janela 5 min, 40–180 bpm).  |
| `extract temp` → `Temperatura` | Extrai `payload.temp` e mostra no gauge (zona vermelha > 38°C).|
| `regra de alerta` → `Status` | Função JS aplica thresholds (BPM > 120, Temp > 38) e pinta verde/vermelho. |
| `MOCK alerta` (desabilitado) | Inject node que dispara um payload de alerta SEM precisar do MQTT. Habilitar para smoke test rápido do dashboard. |
| `InfluxDB Cloud` (desabilitado) | Escreve `vitals` no bucket. Habilitar e configurar URL/token para enviar dados ao Grafana Cloud. **Guia passo-a-passo:** [`docs/grafana_setup.md`](../docs/grafana_setup.md). |

## Ajustar limites

Os limites de alerta moram no **context global**. Você pode alterá-los pelo
editor (Settings → Context Data) ou criar um nó `inject → change` que setem
`global.BPM_MAX` / `global.TEMP_MAX` no startup. Por padrão (fallback no
código): **BPM_MAX = 120**, **TEMP_MAX = 38**.

## Smoke test sem MQTT

1. Habilite o nó `MOCK alerta` (clique no triângulo azul à esquerda do nó).
2. **Deploy**.
3. Clique no botão do nó (lado esquerdo). Veja:
   - Chart pula para 135 bpm.
   - Gauge vai pra 38.5 °C (zona amarela/vermelha).
   - Status fica vermelho com a mensagem do alerta.

## Smoke test com mock publisher

O script Python que gera telemetria fake mora em `../scripts`. Antes de
rodar pela primeira vez, prepare o virtualenv e as credenciais. Detalhes
completos em [`../scripts/README.md`](../scripts/README.md).

### Setup (uma vez)

```powershell
# A partir da raiz do projeto:
cd scripts

# 1) cria virtualenv local (não polui o Python global)
python -m venv .venv

# 2) ativa o venv (PowerShell)
.\.venv\Scripts\Activate.ps1
# Se der erro de "execução de scripts desabilitada", rode antes:
#   Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
# Em CMD (não PowerShell) o comando de ativação seria:
#   .\.venv\Scripts\activate.bat

# 3) instala dependências (paho-mqtt, python-dotenv)
pip install -r requirements.txt

# 4) copia o template de credenciais e edite com seus dados HiveMQ
Copy-Item .env.example .env
notepad .env
```

Quando o venv está ativo, o prompt do PowerShell mostra `(.venv)` no início.

### Rodar

```powershell
# Com o venv ativo e .env preenchido:
python mock_publisher.py --scenario tudo --duration 60
```

**Antes de rodar, abra http://127.0.0.1:1880/ui em outra aba** — é nessa
URL que você vai ver os dados em tempo real. O editor (`:1880`) só mostra
o fluxo e o painel de debug; o gráfico/gauge/alerta vivem só no `/ui`.

Você deve ver pontos chegando no chart, gauge subindo, e o indicador
piscando entre OK e ALERTA conforme as séries cruzam os limites.

### Sessões futuras

Se você fechar o terminal e voltar depois, basta reativar o venv (não
precisa reinstalar nada):

```powershell
cd scripts
.\.venv\Scripts\Activate.ps1
python mock_publisher.py --scenario normal --duration 30
```
