# Pendências para Nota Máxima — CardioIA

**Data:** 12/05/2026  
**Status do projeto:** Parte 1 ✅ | Parte 2 ✅ | Ir Além 1 ✅ | Ir Além 2 ✅

---

## 1. Link público do projeto Wokwi

**Impacto:** Entregável obrigatório da Parte 1 — pode custar pontos na rubrica de sensores.

**O que fazer:**
1. Acesse https://wokwi.com → New Project → ESP32
2. Cole os arquivos do `firmware/` no editor Wokwi:
   - `firmware/src/main.cpp`
   - `firmware/src/cloud_link.cpp`
   - `firmware/include/cloud_link.h`
   - `firmware/include/secrets.h` (com credenciais reais)
   - `firmware/platformio.ini`
3. Adicione os componentes no `diagram.json`:
   - DHT22 no pino GPIO 4
   - Botão no pino GPIO 5 (INPUT_PULLUP)
4. Pressione **Play ▶️** e confirme que o Serial Monitor exibe leituras
5. Clique em **Share** → copie o link público

**Onde atualizar:**
- `README.md` — seção "Como executar", passo 1
- `docs/parte1_relatorio.md` — seção 8 (Entregáveis)

Substituir o placeholder `https://wokwi.com/projects/XXXXXXXXXX` pelo link real.

---

## 2. Screenshots do dashboard Node-RED

**Impacto:** Entregável obrigatório da Parte 2 — o enunciado exige "prints evidenciando a aplicação".

**O que fazer:**

### Pré-requisitos
```powershell
# Terminal 1 — Node-RED
npm install -g --unsafe-perm node-red
node-red

# Importar flows.json no editor http://127.0.0.1:1880
# Instalar node-red-dashboard via Manage Palette
# Configurar nó MQTT com host/creds HiveMQ → Deploy
```

```powershell
# Terminal 2 — Mock publisher
cd scripts
copy .env.example .env   # preencher com creds HiveMQ
pip install -r requirements.txt
```

### Capturas necessárias

| Print | Comando | Salvar como |
|-------|---------|-------------|
| Operação normal (chart + gauge verdes) | `python mock_publisher.py --scenario normal --duration 30` | `assets/node_red_normal.png` |
| Alerta taquicardia (BPM > 120) | `python mock_publisher.py --scenario taquicardia --duration 15` | `assets/node_red_taquicardia.png` |
| Alerta febre (temp > 38 °C) | `python mock_publisher.py --scenario febre --duration 15` | `assets/node_red_febre.png` |

Abrir `http://127.0.0.1:1880/ui` após cada comando e tirar print mostrando o chart de BPM, o gauge de temperatura e o indicador de alerta.

As referências já estão inseridas em `docs/parte2_relatorio.md` (seção 3.4).

---

## 3. Nomes dos professores no README

**Impacto:** Apresentação do projeto.

No `README.md`, substituir:
```markdown
### Tutor(a)
- <a href="#">Nome do Tutor</a>
### Coordenador(a)
- <a href="#">Nome do Coordenador</a>
```
pelos nomes reais com os respectivos links do LinkedIn.

---

## 4. (Opcional) LinkedIn dos integrantes no README

No `README.md`, substituir os `href="#"` pelos perfis LinkedIn de cada integrante:
- Alice C. M. Assis — RM 566233
- Leonardo S. Souza — RM 563928
- Lucas B. Francelino — RM 561409
- Pedro L. T. Silva — RM 561644
- Vitor A. Bezerra — RM 563001

---

## 5. (Opcional) Vídeo YouTube para Ir Além 2

**Impacto:** Critério de avaliação do bônus "Ir Além 2".

O enunciado exige um vídeo de até 4 minutos, postado no YouTube como **"não listado"**, apresentando os resultados do notebook.

**O que fazer:**
1. Gravar apresentação do `ir_alem_2/cardiaia_ir_alem2.ipynb` (~4 min):
   - Mostrar os dados do MIT-BIH carregados
   - Comparar métricas LR × LIF (accuracy, F1)
   - Concluir sobre vantagens do modelo neuromórfico
2. Postar no YouTube como "não listado"
3. Adicionar o link em `ir_alem_2/README.md`

---

## Resumo de prioridade

| # | Pendência | Obrigatório? | Esforço |
|---|-----------|:---:|---------|
| 1 | Link Wokwi | ✅ Sim | ~10 min |
| 2 | Screenshots Node-RED | ✅ Sim | ~20 min |
| 3 | Nomes dos professores | — Apresentação | ~2 min |
| 4 | LinkedIn dos integrantes | — Apresentação | ~5 min |
| 5 | Vídeo YouTube Ir Além 2 | — Bônus | ~30 min |
