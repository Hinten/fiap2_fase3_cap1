# Pendências para Nota Máxima — CardioIA

**Data:** 12/05/2026  
**Status do projeto:** Parte 1 ✅ | Parte 2 ✅ | Ir Além 1 ✅ | Ir Além 2 ✅

---

## ⚠️ PRIORIDADE MÁXIMA — Screenshots do dashboard Node-RED

> **OBRIGATÓRIO para os 2 pontos da rubrica "Dashboard funcional e alertas automáticos".**  
> O enunciado exige explicitamente: *"Prints evidenciando a aplicação ou export do dashboard no Node-RED"*.  
> **Sem esses prints o projeto pode perder 2 pontos inteiros (de 10).**

### O que precisa ser entregue

Três capturas de tela do dashboard em `http://127.0.0.1:1880/ui`, salvas como:

| Print | Arquivo esperado |
|-------|-----------------|
| Dashboard em operação normal | `assets/node_red_normal.png` |
| Dashboard com alerta de taquicardia (BPM > 120) | `assets/node_red_taquicardia.png` |
| Dashboard com alerta de febre (temp > 38 °C) | `assets/node_red_febre.png` |

### Passo a passo

**Terminal 1 — subir o Node-RED:**
```powershell
npm install -g --unsafe-perm node-red
node-red
```
1. Acessar `http://127.0.0.1:1880`
2. Instalar `node-red-dashboard` em **Manage Palette → Install**
3. Importar `node-red/flows.json`
4. Configurar o nó MQTT com host e credenciais HiveMQ → **Deploy**

**Terminal 2 — gerar dados e capturar:**
```powershell
cd scripts
copy .env.example .env   # preencher com credenciais HiveMQ
pip install -r requirements.txt

# Print 1: operação normal
python mock_publisher.py --scenario normal --duration 30
# → abrir http://127.0.0.1:1880/ui → tirar print → salvar como assets/node_red_normal.png

# Print 2: alerta de taquicardia
python mock_publisher.py --scenario taquicardia --duration 15
# → abrir http://127.0.0.1:1880/ui → tirar print com alerta vermelho → salvar como assets/node_red_taquicardia.png

# Print 3: alerta de febre
python mock_publisher.py --scenario febre --duration 15
# → abrir http://127.0.0.1:1880/ui → tirar print com gauge vermelho → salvar como assets/node_red_febre.png
```

**Após salvar os arquivos em `assets/`**, os prints já estão referenciados automaticamente em `docs/parte2_relatorio.md` (seção 3.4). Commitar e fazer push para o PR.

---

## 1. ~~Link público do projeto Wokwi~~ ✅ CONCLUÍDO

Link atualizado: https://wokwi.com/projects/463837603097937921  
Já aplicado em `README.md` e `docs/parte1_relatorio.md`.

---

## 2. Nomes dos professores no README

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

## 3. (Opcional) LinkedIn dos integrantes no README

No `README.md`, substituir os `href="#"` pelos perfis LinkedIn de cada integrante:
- Alice C. M. Assis — RM 566233
- Leonardo S. Souza — RM 563928
- Lucas B. Francelino — RM 561409
- Pedro L. T. Silva — RM 561644
- Vitor A. Bezerra — RM 563001

---

## 4. (Opcional) Vídeo YouTube para Ir Além 2

O enunciado exige um vídeo de até 4 minutos, postado no YouTube como **"não listado"**, apresentando os resultados do notebook.

1. Gravar apresentação do `ir_alem_2/cardiaia_ir_alem2.ipynb` (~4 min):
   - Mostrar os dados do MIT-BIH carregados
   - Comparar métricas LR × LIF (accuracy, F1)
   - Concluir sobre vantagens do modelo neuromórfico
2. Postar no YouTube como "não listado"
3. Adicionar o link em `ir_alem_2/README.md`

---

## Resumo de prioridade

| # | Pendência | Impacto | Esforço |
|---|-----------|---------|---------|
| ⚠️ | **Screenshots Node-RED** | **−2 pts se faltar** | ~20 min |
| ~~1~~ | ~~Link Wokwi~~ | ~~Obrigatório~~ | ✅ Feito |
| 2 | Nomes dos professores | Apresentação | ~2 min |
| 3 | LinkedIn dos integrantes | Apresentação | ~5 min |
| 4 | Vídeo YouTube Ir Além 2 | Bônus | ~30 min |
