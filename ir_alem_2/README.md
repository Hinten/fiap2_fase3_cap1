# CardioIA — Ir Além 2: IA em séries temporais de saúde

Comparação entre **Regressão Logística** (baseline clássico) e uma **Spiking Neural Network (SNN) com neurônios LIF parametrizados (PLIF)** implementada em TensorFlow puro, aplicada à classificação de batimentos cardíacos do **MIT-BIH Arrhythmia Database**.

## Estrutura do repositório

```
.
├── cardioia_ir_alem2.ipynb    # notebook principal (rodável no Colab)
├── relatorio_comparativo.md   # relatório comparativo de 2 páginas
├── requirements.txt           # dependências
└── README.md                  # este arquivo
```

## Como rodar

### Opção A — Google Colab (recomendado)

1. Abra o notebook no Colab: `File → Upload notebook` → selecione `cardioia_ir_alem2.ipynb`.
2. `Runtime → Change runtime type → GPU` (reduz tempo de treino de ~10 min para ~1 min).
3. `Runtime → Run all`.

A primeira célula instala `wfdb` e `neurokit2`. As demais bibliotecas (TensorFlow, scikit-learn, scipy, pandas, matplotlib, seaborn) já vêm pré-instaladas no Colab.

### Opção B — local

```bash
python -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install -r requirements.txt
jupyter lab cardioia_ir_alem_2.ipynb
```

## O que o notebook faz

```
ECG bruto → bandpass 0.5–40 Hz + z-score → segmentação em batidas (180 amostras / R-peak)
        → [A] 12 features (estat. + FFT + gradiente) → LogisticRegression
        → [B] sinal bruto temporal               → SNN (PLIF + soft reset)
        → métricas comparativas + spike raster + matriz de confusão
```

1. **Dados**: baixa 5 registros do MIT-BIH via PhysioNet (`100`, `101`, `103`, `208`, `210`). Se o download falhar, usa ECG simulado via NeuroKit2 mantendo todo o pipeline.
2. **Pré-processamento**: filtro Butterworth bandpass (0.5–40 Hz, ordem 4) + normalização z-score por registro.
3. **Segmentação**: janela de 180 amostras (~0.5 s @ 360 Hz) centrada no R-peak anotado. Rótulos AAMI binários: Normal `{N, L, R, e, j}` e Anormal `{A, a, J, S, V, F, /, f, Q, E}`.
4. **Split**: 80% treino / 20% teste estratificado por classe.
5. **Caminho A — Regressão Logística**: extrai 12 features (média, desvio, min, max, skew, kurtosis, energia em 4 bandas FFT, média e máximo do gradiente) → `StandardScaler` → `LogisticRegression(class_weight='balanced')`.
6. **Caminho B — SNN (LIF v3)**: neurônios LIF implementados como `keras.layers.Layer` customizada com:
   - **Surrogate gradient** fast-sigmoid escalado (`sharpness=5`) para permitir backpropagation através da função Heaviside
   - **Soft reset** (`V ← V − θ·s`) preservando gradiente no BPTT
   - **Beta aprendível por neurônio** via sigmoid (Parametric LIF / PLIF)
   - Arquitetura: `Dense(64, relu) → LIF(128) → Dropout(0.3) → LIF(2) → reduce_sum → softmax`
   - Treino: Adam (lr=5e-4, clipnorm=1.0), EarlyStopping e ReduceLROnPlateau monitorando `val_accuracy`
7. **Comparação**: Accuracy, Precision, Recall, F1, ROC-AUC lado a lado + matrizes de confusão.
8. **Spike raster**: visualização da atividade da 1ª camada LIF (128 neurônios × 180 timesteps) + métrica de esparsidade (proxy de eficiência energética em hardware neuromórfico).

## Resultados obtidos

Split intra-patient 80/20, MIT-BIH (registros 100, 101, 103, 208, 210), seed=42:

| Métrica   | Logistic Regression | SNN (LIF v3) | Delta    |
|-----------|---------------------|--------------|----------|
| Accuracy  | **97.00%**          | 96.74%       | −0.26 pp |
| Precision | **92.95%**          | 92.52%       | −0.43 pp |
| Recall    | **84.71%**          | 83.18%       | −1.53 pp |
| F1-score  | **88.64%**          | 87.60%       | −1.04 pp |
| ROC-AUC   | **95.34%**          | 94.94%       | −0.40 pp |

A SNN ficou dentro de **1–2 pp da Regressão Logística em todas as métricas**, operando diretamente sobre o sinal bruto de 180 amostras sem nenhuma feature engineering manual. O treinamento convergiu em 19 épocas (pesos salvos na época 9 pelo EarlyStopping). Esparsidade de ativação da 1ª camada LIF: **11.43%**.

> **Atenção**: estes resultados usam split intra-patient. Em split inter-patient (DS1/DS2), que é o padrão clínico correto, esperam-se quedas de 10–20 pp por eliminação do vazamento de morfologia individual entre treino e teste.

## Arquitetura da SNN

```
Input (180, 1)
    │
Dense(64, relu)          ← projeção linear; ReLU garante correntes positivas para o LIF
    │
LIFDense(128, PLIF)      ← 128 neurônios LIF com β aprendível por neurônio
    │                       soft reset: V ← V − θ·s
Dropout(0.3)
    │
LIFDense(2, PLIF)        ← 2 neurônios de saída (Normal / Anormal)
    │
reduce_sum(axis=T)       ← contagem total de spikes por classe
    │
softmax                  ← probabilidades de classe

Total: 8.836 parâmetros treináveis
```

## Por que esta comparação importa

- **Regressão Logística** é o padrão de fato em wearables médicos certificados: treino em menos de 1 segundo, interpretável por coeficiente, baixa exigência de dados.
- **Spiking Neural Networks** são a aposta da indústria de IoT médica para wearables *always-on*. Em chips como Intel Loihi 2 ou BrainChip Akida, consomem 100–1000× menos energia que ANNs equivalentes porque processam apenas eventos (spikes), não tensores densos.

A pergunta real não é "qual é melhor?", mas **"qual é apropriada para o substrato computacional disponível?"**

## Referências

- Moody, G.B., Mark, R.G. (2001). *The impact of the MIT-BIH Arrhythmia Database*. IEEE EMB Mag.
- De Chazal, P. et al. (2004). *Automatic classification of heartbeats using ECG morphology and heartbeat interval features*. IEEE TBME.
- Neftci, E.O., Mostafa, H., Zenke, F. (2019). *Surrogate gradient learning in spiking neural networks*. IEEE Signal Processing Magazine.
- Fang, W. et al. (2021). *Incorporating learnable membrane time constant to enhance learning of spiking neural networks (PLIF)*. ICCV.
- Davies, M. et al. (2018). *Loihi: a neuromorphic manycore processor with on-chip learning*. IEEE Micro.
- Yan, Z. et al. (2024). *Energy-efficient SNNs for ECG classification on neuromorphic hardware*. Frontiers in Neuroscience.

## video
url: https://drive.google.com/file/d/1iOzqClnWFcnLlP6x7qsbWhA-Blt8e9dL/view?usp=sharing

## Licença

MIT — uso acadêmico do projeto CardioIA / FIAP.
