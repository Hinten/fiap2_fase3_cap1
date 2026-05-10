# Relatório Técnico: Inteligência Artificial em Séries Temporais de Saúde (CardioIA)
### Desafio Ir Além 2 - Fase 3: Monitoramento Contínuo e IA Neuromórfica

---

## 1. Introdução
O monitoramento contínuo de sinais vitais é um pilar fundamental da saúde digital moderna. No projeto **CardioIA**, o objetivo central é o desenvolvimento de uma solução de monitoramento cardiológico capaz de operar em dispositivos de borda (*Edge Computing*), como o ESP32.

Este relatório detalha a implementação e a comparação de dois modelos de Inteligência Artificial para a detecção de taquicardia: um modelo estatístico clássico (**Regressão Logística**) e um modelo de inspiração biológica (**Leaky Integrate-and-Fire - LIF**). A análise foca não apenas na precisão diagnóstica, mas também na eficiência computacional e na viabilidade técnica de rodar esses algoritmos em ambientes com recursos de hardware limitados, onde latência e consumo de energia são críticos.

---

## 2. Metodologia e Desenvolvimento Técnico

### 2.1 Aquisição e Tratamento de Dados (DSP)
Para garantir que a IA fosse testada em cenários realistas, evitou-se o uso de sinais sintéticos puros. Utilizamos a base de dados **MIT-BIH Arrhythmia Database**, injetando artefatos de ruído típicos de sensores vestíveis (*wearables*).

O pipeline de Processamento Digital de Sinais (**DSP**) implementado seguiu as seguintes etapas:

*   **Filtragem de Frequência:** Aplicação de filtros para remover a interferência da rede elétrica (60Hz) e flutuações de linha de base causadas pela respiração do paciente.
*   **Detecção de Picos R:** Utilização da biblioteca *NeuroKit2* para identificar os complexos QRS.
*   **Cálculo de HRV (Variabilidade Cardíaca):** Em vez de utilizar apenas a frequência cardíaca média, extraímos o **RMSSD** (*Root Mean Square of Successive Differences*). Esta métrica é o padrão-ouro na cardiologia para avaliar a variabilidade entre batimentos sucessivos, sendo um indicador precoce de estresse fisiológico e arritmias.

### 2.2 Arquitetura dos Modelos de IA

#### A. Regressão Logística (LR)
A Regressão Logística foi utilizada como *baseline*. Ela opera sobre vetores de características (*features*) pré-calculadas. O modelo aprende uma função logística que define a probabilidade de uma janela de 3 segundos de ECG pertencer à classe "Normal" ou "Taquicardia". Embora eficiente em computadores, ela depende de uma etapa prévia de agregação de dados.

#### B. Modelo Neuromórfico Leaky Integrate-and-Fire (LIF)
O LIF é um modelo de rede neural de terceira geração (*Spiking Neural Networks*). Diferente de redes tradicionais, o LIF mimetiza a biofísica de um neurônio:

1.  **Integrate:** Ele acumula o potencial elétrico do sinal de ECG recebido.
2.  **Leaky:** O potencial acumulado "vaza" com o tempo (decaimento exponencial), impedindo que ruídos aleatórios causem disparos.
3.  **Fire:** Quando o potencial atinge um limiar ($V_{threshold}$), o neurônio emite um *spike* (alerta) e reseta seu estado.
4.  **Período Refratário:** Implementamos uma pausa obrigatória após cada disparo para evitar o efeito de "repique" causado por oscilações rápidas no sinal.

---

## 3. Resultados e Análise Comparativa

Os modelos foram submetidos a um conjunto de teste com 30% dos dados, mantendo a proporcionalidade entre as classes (estratificação).

### 3.1 Tabela de Métricas Consolidadas

| Métrica | Regressão Logística (LR) | IA Neuromórfica (LIF) |
| :--- | :--- | :--- |
| **Acurácia Global** | 99% | 88% |
| **Sensibilidade (Recall)** | 100% | 91% |
| **Precisão (Classe 1)** | 98% | 85% |
| **Latência por Amostra (Py)** | 0.000004 s | 0.000256 s |
| **Uso de Memória RAM** | **Alto** (exige Buffer) | **Baixíssimo** (Ponto a Ponto) |

### 3.2 Discussão dos Resultados
Os dados revelam que, embora a Regressão Logística possua uma acurácia estatística superior em ambiente simulado, ela apresenta uma rigidez arquitetônica: ela precisa da "foto" completa do sinal (buffer) para decidir.

O modelo **LIF**, por outro lado, apresentou um **Recall de 91%**. Em aplicações médicas, o *Recall* é a métrica mais valiosa, pois indica a capacidade do sistema de não ignorar um evento de risco (falso negativo). O erro do LIF tendeu para "falsos positivos", o que, em um sistema de triagem, é preferível a deixar uma taquicardia passar despercebida.

---

## 4. Análise de Viabilidade no Edge (ESP32)

A verdadeira vantagem do modelo LIF reside na sua implementação em hardware de baixo recurso:

*   **Eficiência de Memória:** Enquanto a LR exige armazenar janelas de sinal em buffers de memória RAM para calcular médias e desvios, o LIF opera de forma *stateless* em relação ao buffer. Ele só precisa manter uma única variável na memória: o potencial atual da membrana.
*   **Processamento em Tempo Real:** O LIF processa o sinal amostra por amostra. No momento em que o coração acelera, o neurônio dispara o alerta. Na LR, o sistema precisaria esperar o fim da janela de tempo para processar o lote.
*   **Consumo de Energia:** A computação neuromórfica é baseada em eventos (*event-driven*). O sistema pode permanecer em estado de baixo consumo e "acordar" a transmissão via MQTT apenas quando o neurônio atingir o limite de disparos.

---

## 5. Conclusão

A investigação demonstrou que a integração de conceitos de neurociência computacional com IoT médica oferece um caminho promissor para o projeto **CardioIA**. A Regressão Logística permanece uma ferramenta poderosa para análise de dados históricos em *Cloud*, mas para o monitoramento vestível em tempo real, o modelo **LIF Neuromórfico** é a escolha técnica superior.

A robustez demonstrada pelo LIF, mesmo sob condições de ruído, aliada à sua baixíssima pegada de memória, permite que o sistema CardioIA ofereça segurança clínica e autonomia de bateria, transformando o ESP32 em um sentinela inteligente capaz de salvar vidas através da detecção precoce de anomalias cardiológicas.

---

## 🔗 Referências e Entregáveis

*   **Código Fonte:** `main.py` (Implementação em Python/NeuroKit2).
*   **Notebook:** Disponível no repositório GitHub.
*   **Vídeo Demonstrativo:** [Link do YouTube - Não Listado]
*   **Base de Dados:** Simulada com parâmetros reais MIT-BIH.

---
**Análise de Desempenho gerada em:** 2026-05-09  
**Responsável:** Equipe CardioIA