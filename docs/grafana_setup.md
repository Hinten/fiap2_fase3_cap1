# Guia — Grafana Cloud (bônus do enunciado)

Visualização opcional do CardioIA em Grafana Cloud. O caminho mais simples
e 100% gratuito é:

```
ESP32/Mock ──MQTT──▶ HiveMQ ──▶ Node-RED ──▶ InfluxDB Cloud ──▶ Grafana Cloud
                                  (já feito)        (este guia)
```

Por que **InfluxDB no meio?** O plugin MQTT nativo do Grafana é pago. O
InfluxDB Cloud Serverless tem free tier generoso (suficiente pro projeto)
e o Grafana Cloud tem datasource InfluxDB built-in.

> **Tempo estimado:** ~30 min na primeira vez.

---

## Etapa 1 — InfluxDB Cloud (Serverless free)

### 1.1 Criar conta

1. Acesse https://cloud2.influxdata.com/signup
2. Cadastre-se (pode usar Google/GitHub).
3. Quando perguntar o **provider** e a **região**, escolha qualquer um — **anote os dois**, vão entrar na URL. Recomendo `AWS us-east-1` (mais rápido).
4. No final do onboarding, escolha o plano **Free** (Pay As You Go aparece como default — clique em "Free Plan").

### 1.2 Criar o bucket

Um *bucket* é o "banco de dados" no InfluxDB.

1. Menu lateral → **Load Data → Buckets**.
2. Clique **Create Bucket** (canto superior direito).
3. Nome: `cardioia`
4. Retention: **30 days** (max do free tier).
5. **Create**.

### 1.3 Gerar API Token

1. Menu lateral → **Load Data → API Tokens**.
2. **Generate API Token → Custom API Token**.
3. **Description**: `node-red-write`
4. Em **Buckets**, ache `cardioia` e marque **Write** (não precisa Read pra escrita).
5. **Generate**.
6. **COPIE O TOKEN AGORA** — ele só aparece uma vez. Cole num arquivo temporário; você vai usá-lo nas etapas 2 e 4.

### 1.4 Anotar dados que você vai precisar

Vá em **Organization Name (canto superior esquerdo) → Settings**. Anote:

| Campo                     | Onde achar                          | Exemplo                                              |
|---------------------------|-------------------------------------|------------------------------------------------------|
| **Cluster URL**           | Topo da tela About                  | `https://us-east-1-1.aws.cloud2.influxdata.com`      |
| **Organization (Org ID e Name)** | About → Organization Name + ID | `cardioia-org` / `0a1b2c3d4e5f6789`                  |
| **Bucket**                | (definido na 1.2)                   | `cardioia`                                           |
| **Token**                 | (gerado na 1.3)                     | `Lq...` (longo)                                      |

---

## Etapa 2 — Configurar o nó InfluxDB no Node-RED

O `flows.json` já tem o nó preparado, só desabilitado. Vamos habilitar e
preencher as credenciais.

1. No editor (http://127.0.0.1:1880), localize o nó cinza **`InfluxDB Cloud (opcional)`** (canto inferior direito do flow).
2. **Duplo-clique** nele.
3. No campo `Server`, clique no lápis ✏️ ao lado de `InfluxDB Cloud`.
4. Preencha:
   - **Version**: `2.0`
   - **URL**: a Cluster URL da etapa 1.4 (ex: `https://us-east-1-1.aws.cloud2.influxdata.com`)
   - **Token**: cole o token da etapa 1.3
   - **Organization**: o **Name** da org (ex: `cardioia-org`)
   - **Default Bucket**: `cardioia`
5. **Update** → **Done** (ainda não fecha o nó-pai).
6. De volta ao nó `InfluxDB Cloud (opcional)`:
   - **Measurement**: `vitals`
   - **Org**: `cardioia-org` (mesmo nome de cima)
   - **Bucket**: `cardioia`
   - **Time Precision**: `Seconds (s)`
7. **Done**.

### 2.1 Habilitar o nó (importante!)

O nó está **desabilitado por padrão** (aparece esmaecido). Pra ligar:

- Clique no nó pra selecioná-lo.
- Menu superior → **Edit → Enable selected node(s)** (ou tecla `Shift+Ctrl+E`).
- Alternativa: clique no triângulo azul à esquerda do nó.

O nó deve ficar com cor cheia (não esmaecida).

### 2.2 Deploy + smoke test

1. **Deploy** (canto superior direito).
2. Rodar o mock publisher:
   ```powershell
   cd scripts
   .\.venv\Scripts\Activate.ps1
   python mock_publisher.py --scenario normal --duration 30
   ```
3. No Node-RED editor, abra o painel **Debug** (ícone 🐛). Não deve aparecer nenhum erro vermelho do nó InfluxDB. Se aparecer `unauthorized`, refaça a etapa 2 — token ou org provavelmente errados.

### 2.3 Conferir que os dados chegaram no InfluxDB

1. Volte ao InfluxDB Cloud no navegador.
2. Menu lateral → **Data Explorer**.
3. Em **From**, escolha bucket `cardioia`.
4. Em **Filter**, selecione `_measurement = vitals`.
5. Em **Filter** (segundo filtro), selecione `_field = bpm`.
6. Mude **Past 1h** pra **Past 5m**.
7. Clique **Submit**.

Se aparecer uma série temporal, **etapa 1+2 OK**. Se aparecer "no data",
verifique:
- O nó InfluxDB está habilitado e foi feito Deploy?
- O mock publisher rodou DEPOIS do Deploy?
- O token é de **Write** no bucket certo?

---

## Etapa 3 — Grafana Cloud

### 3.1 Criar conta

1. Acesse https://grafana.com/auth/sign-up/create-user
2. Cadastre-se. Escolha o plano **Free Forever**.
3. No onboarding, escolha um nome pra sua *stack* (ex: `cardioia`). Ele vira a URL: `cardioia.grafana.net`.

### 3.2 Adicionar InfluxDB como datasource

1. Na home do Grafana → menu lateral esquerdo → **Connections → Data sources** (ou direto: `https://<sua-stack>.grafana.net/connections/datasources`).
2. **+ Add new data source** → busque **InfluxDB** → clique.
3. Preencha:

   | Campo                  | Valor                                                       |
   |------------------------|-------------------------------------------------------------|
   | **Name**               | `InfluxDB CardioIA`                                         |
   | **Query language**     | **Flux**                                                    |
   | **URL**                | mesma Cluster URL da etapa 1.4                              |
   | **Basic auth**         | desligado                                                   |
   | **Organization**       | nome da org                                                 |
   | **Token**              | mesmo token (ou gere outro com permissão Read também)       |
   | **Default Bucket**     | `cardioia`                                                  |

4. Role até o final → **Save & Test**. Deve aparecer "datasource is working" em verde. Se der erro, o token provavelmente não tem permissão de **Read** — volte na etapa 1.3 e gere um token com Read também (ou um token "All Access" pro free tier).

### 3.3 Criar dashboard

1. Menu lateral → **Dashboards** → **New** → **New dashboard**.
2. **+ Add visualization** → escolha o datasource `InfluxDB CardioIA`.
3. No painel inferior **Query**, cole esta query Flux:

   ```flux
   from(bucket: "cardioia")
     |> range(start: -15m)
     |> filter(fn: (r) => r._measurement == "vitals")
     |> filter(fn: (r) => r._field == "bpm")
   ```

4. **Refresh dashboard** (ícone de seta circular no canto superior direito do painel). Deve aparecer uma série temporal de BPM.
5. Painel da direita:
   - **Title**: `BPM (batidas/min)`
   - **Visualization**: `Time series` (default)
   - **Standard options → Min**: `40`, **Max**: `180`
6. Clique **Apply** (canto superior direito).

### 3.4 Adicionar painel de temperatura

1. No dashboard → **Add → Visualization**.
2. Mesmo datasource. Query:

   ```flux
   from(bucket: "cardioia")
     |> range(start: -15m)
     |> filter(fn: (r) => r._measurement == "vitals")
     |> filter(fn: (r) => r._field == "temp")
     |> last()
   ```

3. **Visualization**: `Stat`.
4. **Title**: `Temperatura atual (°C)`.
5. **Standard options → Unit**: `Celsius (°C)`.
6. **Thresholds**: adicione threshold em `38` cor vermelha (vai pintar quando passar de febre).
7. **Apply**.

### 3.5 Adicionar gráfico de febre/taquicardia (bônus visual)

Repita 3.3 com a query abaixo pra ter um painel duplo:

```flux
from(bucket: "cardioia")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "vitals")
  |> filter(fn: (r) => r._field == "bpm" or r._field == "temp")
```

Configure como **Time series** com 2 séries no eixo. Bom pra screenshot.

### 3.6 Salvar e tirar screenshot

1. Ícone de disquete 💾 no topo → **Save dashboard** → nome `CardioIA — Sinais Vitais`.
2. Rode `python mock_publisher.py --scenario tudo --duration 90` em outro terminal.
3. Aguarde os pontos aparecerem (latência típica < 10s).
4. **Screenshot**: tecla `Print Screen` (ou `Win+Shift+S`) e salvar em `docs/parte2_prints/grafana_dashboard.png`.

---

## Etapa 4 — (Opcional) Tornar o dashboard público

Pra colocar um link no relatório que o professor consiga abrir sem login:

1. Dashboard aberto → ícone de **compartilhamento** (corrente) no topo.
2. Aba **Public dashboard** → **Generate public URL**.
3. Habilite **Annotations**: opcional.
4. Confirme. Copie a URL gerada (algo como `https://cardioia.grafana.net/public-dashboards/<hash>`).
5. Cole no `README.md` raiz, na seção **Links externos**.

> **Limitação Grafana Cloud**: você pode tornar o dashboard público, mas
> NÃO pode embutir em iframe num site externo (`allow_embedding` não é
> liberado no plano Cloud). Pro relatório, screenshot + URL pública é o
> caminho mais simples.

---

## Troubleshooting

| Sintoma                                                     | Causa provável                                                       |
|-------------------------------------------------------------|----------------------------------------------------------------------|
| Nó InfluxDB com erro `401 unauthorized`                     | Token errado ou não tem permissão Write no bucket.                   |
| Nó InfluxDB com erro `404 bucket not found`                 | Nome do bucket ou da org diferente do que está na URL/configuração.  |
| Nó InfluxDB silencioso, mas Data Explorer fica vazio         | Nó **desabilitado**? Olhe se está esmaecido no editor.               |
| Grafana **Save & Test** falha com `not found`               | URL com `https://` faltando ou com `/` extra no final.               |
| Grafana datasource OK, mas painel "No data"                  | Bucket / measurement / field digitados errados na query Flux.        |
| Latência alta (> 1 min)                                     | Free tier do InfluxDB Cloud Serverless tem flush em batches; normal até ~10–30s. |

---

## Mapeamento pro relatório (bônus)

Quando você terminar este guia, pode preencher essas duas linhas no
checklist do `docs/parte2_implementacao.md`:

- [x] Bônus — InfluxDB write (T11) verificado em Data Explorer.
- [x] Bônus — Grafana datasource + painel (T12) capturado em `docs/parte2_prints/grafana_dashboard.png`.

E adicionar o link no `README.md` raiz na seção **Links externos**:

```md
- **Grafana Cloud público:** https://cardioia.grafana.net/public-dashboards/<hash>
```
