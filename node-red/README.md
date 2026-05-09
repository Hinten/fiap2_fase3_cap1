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

Acesse http://127.0.0.1:1880 (editor) e http://127.0.0.1:1880/ui (dashboard).

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
| `InfluxDB Cloud` (desabilitado) | Escreve `vitals` no bucket. Habilitar e configurar URL/token para enviar dados ao Grafana Cloud. |

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

```powershell
# Em outro terminal, com .env configurado:
cd ..\scripts
python mock_publisher.py --scenario tudo --duration 60
```

Você deve ver pontos chegando no chart, gauge subindo, e o indicador
piscando entre OK e ALERTA conforme as séries cruzam os limites.
