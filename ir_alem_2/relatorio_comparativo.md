# Relatório Comparativo — CardioIA: Ir Além 2
**Classificação de Arritmias Cardíacas: Regressão Logística vs Spiking Neural Network**

Pedro Tostes Silva | FIAP | 2025

---

## 1. Introdução

A classificação automática de batimentos cardíacos em sinais de eletrocardiograma (ECG) é uma das tarefas mais estudadas em aprendizado de máquina aplicado à saúde. Dispositivos vestíveis de monitoramento contínuo exigem modelos com alta acurácia, baixo consumo energético e, idealmente, capacidade de operar em microcontroladores com recursos limitados.

Este relatório compara dois paradigmas distintos para essa tarefa: a **Regressão Logística** (LR), classificador linear amplamente adotado em wearables certificados, e uma **Spiking Neural Network** (SNN) com neurônios Leaky Integrate-and-Fire (LIF) parametrizados, representando a fronteira de pesquisa em computação neuromórfica. Ambos foram aplicados ao MIT-BIH Arrhythmia Database, dataset de referência com anotações batida-a-batida de ECG de 48 pacientes.

---

## 2. Metodologia

### 2.1 Dataset e pré-processamento

Foram utilizados 5 registros do MIT-BIH Arrhythmia Database (IDs: 100, 101, 103, 208, 210), totalizando **11.820 batidas** segmentadas: 10.184 normais (86,2%) e 1.636 anormais (13,8%). O desbalanceamento é representativo da prevalência real de arritmias.

O pipeline de pré-processamento consistiu em: (1) filtro Butterworth bandpass 0,5–40 Hz (fase zero, ordem 4) para remover *baseline wander* e ruído elétrico; (2) normalização z-score por registro; (3) segmentação em janelas de 180 amostras (~0,5 s a 360 Hz) centradas no R-peak anotado. Os rótulos seguem a convenção AAMI binária: Normal `{N, L, R, e, j}` e Anormal `{A, a, J, S, V, F, /, f, Q, E}`. O split foi estratificado 80/20 (treino/teste).

### 2.2 Modelo A — Regressão Logística

Foram extraídas **12 features** por batida: média, desvio padrão, mínimo, máximo, assimetria (*skewness*), curtose, energia espectral em quatro bandas via FFT, e média e máximo do valor absoluto do gradiente temporal. As features foram normalizadas com `StandardScaler` e o modelo foi treinado com `class_weight='balanced'` para compensar o desbalanceamento.

### 2.3 Modelo B — Spiking Neural Network (LIF v3)

A SNN foi implementada como camada Keras customizada (`LIFDense`) seguindo a dinâmica discreta do neurônio Leaky Integrate-and-Fire:

$$V_{t+1} = \beta_i \cdot V_t + W \cdot x_t + b \quad;\quad s_t = \mathbf{1}[V_{t+1} > \theta] \quad;\quad V_{t+1} \leftarrow V_{t+1} - \theta \cdot s_t$$

Três contribuições técnicas distinguem esta implementação do modelo LIF básico:

- **Surrogate gradient escalado** (sharpness = 5): substitui a derivada da Heaviside por $5/(1+5|V-\theta|)^2$, concentrando o gradiente próximo ao limiar para estabilidade de treinamento.
- **Soft reset** ($V \leftarrow V - \theta \cdot s$): em vez de zerar a membrana após o spike, subtrai o limiar, preservando o gradiente no backpropagation through time (BPTT).
- **Beta aprendível por neurônio** (Parametric LIF / PLIF): cada neurônio aprende sua constante de decaimento $\beta_i \in (0,1)$ via sigmoid, adaptando sua janela de integração temporal.

A arquitetura final é `Dense(64, ReLU) → LIF(128, PLIF) → Dropout(0,3) → LIF(2, PLIF) → reduce_sum → softmax`, totalizando **8.836 parâmetros treináveis**. O treinamento usou Adam (lr = 5×10⁻⁴, clipnorm = 1,0), class_weight moderado `{0: 1,0; 1: √6,22 ≈ 2,49}`, e callbacks de EarlyStopping e ReduceLROnPlateau monitorando `val_accuracy`.

---

## 3. Resultados

O treinamento da SNN convergiu em 19 épocas, com os pesos ótimos salvos na época 9 (val_accuracy = 97,82%). A Tabela 1 apresenta as métricas no conjunto de teste (n = 2.364 batidas).

**Tabela 1 — Comparação de métricas no conjunto de teste (split intra-patient 80/20)**

| Métrica   | Regressão Logística | SNN (LIF v3) | Delta     |
|-----------|---------------------|--------------|-----------|
| Accuracy  | **97,00%**          | 96,74%       | −0,26 pp  |
| Precision | **92,95%**          | 92,52%       | −0,43 pp  |
| Recall    | **84,71%**          | 83,18%       | −1,53 pp  |
| F1-score  | **88,64%**          | 87,60%       | −1,04 pp  |
| ROC-AUC   | **95,34%**          | 94,94%       | −0,40 pp  |

A Regressão Logística superou a SNN em todas as métricas, com diferenças de 0,26 a 1,53 pp. A diferença em F1 — a métrica mais relevante em dados desbalanceados — foi de apenas **1,04 pp**. Ambos os modelos apresentaram comportamento semelhante na matriz de confusão, com a maior concentração de erros em falsos negativos (batidas anormais classificadas como normais).

A SNN apresentou esparsidade de ativação de **11,43%** na primeira camada LIF (2.633 spikes de um total possível de 23.040 por batida), característica fundamental para eficiência energética em hardware neuromórfico.

---

## 4. Discussão

### 4.1 Interpretação dos resultados

O resultado mais relevante não é a diferença de 1 pp entre os modelos, mas o fato de que a SNN atingiu desempenho competitivo **sem nenhuma feature engineering manual**. A Regressão Logística depende de 12 features cuidadosamente escolhidas por especialistas de domínio (estatísticas temporais, espectro de frequências, morfologia do gradiente), enquanto a SNN aprende representações diretamente do sinal bruto de 180 amostras.

Do ponto de vista de custo-benefício em GPU convencional, a LR é claramente superior: treina em menos de 1 segundo, gera modelos com 13 parâmetros interpretáveis por coeficiente, e é auditável para fins regulatórios (FDA, ANVISA). A SNN requer ~1 minuto de GPU para convergir e suas representações internas (padrões de spikes) são de difícil interpretação.

### 4.2 Contexto de aplicação: a pergunta correta

A comparação direta em GPU não captura a vantagem fundamental das SNNs. Em chips neuromórficos dedicados — Intel Loihi 2 ou BrainChip Akida —, a comunicação *event-driven* via spikes processa apenas os 11,43% de eventos ativos, enquanto os 88,57% de silêncio não geram consumo computacional. Estimativas da literatura indicam redução de 100 a 1.000 vezes no consumo energético em relação a ANNs densas equivalentes rodando em processadores convencionais. Para wearables cardíacos *always-on* com bateria de semanas, essa diferença é decisiva.

A pergunta correta, portanto, não é "qual modelo é melhor?", mas **"qual é apropriado para o substrato computacional do produto final?"**: Cortex-M convencional com restrições regulatórias → Regressão Logística; co-processador neuromórfico em smartwatch médico → SNN.

### 4.3 Limitações

Este experimento usa **split intra-patient**: batidas de treino e teste provêm dos mesmos pacientes. Isso introduz viés otimista, pois morfologias individuais de ECG (forma do QRS, amplitude da onda T) são altamente específicas por paciente e "vazam" do treino para o teste. O protocolo clínico correto (De Chazal et al., 2004) exige split inter-patient (DS1 treino / DS2 teste), onde quedas de 10–20 pp em F1 são típicas. Os resultados aqui reportados representam um limite superior de desempenho, não uma estimativa de generalização clínica.

---

## 5. Conclusão

A Spiking Neural Network implementada com PLIF, soft reset e surrogate gradient escalado demonstrou ser competitiva com a Regressão Logística na classificação de arritmias do MIT-BIH, ficando dentro de 1–2 pp em todas as métricas, sem extração manual de features. A Regressão Logística permanece superior para deployments em microcontroladores convencionais e contextos regulatórios. A SNN se justifica como evolução natural quando o hardware-alvo for neuromórfico, cenário em que sua esparsidade de 11,43% se traduz em vantagem energética concreta.

Próximos passos incluem validação com split inter-patient, expansão para 5 classes AAMI e quantização para estimativa real de consumo em Loihi/Akida.

---

## Referências

- Moody, G.B., Mark, R.G. (2001). The impact of the MIT-BIH Arrhythmia Database. *IEEE Engineering in Medicine and Biology Magazine*, 20(3), 45–50.
- De Chazal, P., O'Dwyer, M., Reilly, R.B. (2004). Automatic classification of heartbeats using ECG morphology and heartbeat interval features. *IEEE Transactions on Biomedical Engineering*, 51(7), 1196–1206.
- Neftci, E.O., Mostafa, H., Zenke, F. (2019). Surrogate gradient learning in spiking neural networks. *IEEE Signal Processing Magazine*, 36(6), 51–63.
- Fang, W., Yu, Z., Chen, Y., Huang, T., Masquelier, T., Tian, Y. (2021). Incorporating learnable membrane time constant to enhance learning of spiking neural networks. *ICCV 2021*.
- Davies, M. et al. (2018). Loihi: a neuromorphic manycore processor with on-chip learning. *IEEE Micro*, 38(1), 82–99.
