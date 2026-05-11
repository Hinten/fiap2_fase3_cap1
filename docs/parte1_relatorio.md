# Relatório Parte 1: Armazenamento e Processamento Local (Edge Computing) - CardioIA

## Introdução

Este relatório descreve a implementação da Parte 1 do projeto CardioIA, focada no armazenamento e processamento local de dados em um dispositivo ESP32 simulado no Wokwi. O objetivo é demonstrar conceitos de Edge Computing em aplicações de monitoramento de saúde, garantindo resiliência offline e captura de sinais vitais simulados. A implementação foi integrada com a Parte 2 (MQTT) para formar o protótipo completo.

## Arquitetura e Componentes

### Sensores Utilizados
- **DHT22**: Sensor obrigatório para leitura de temperatura (°C) e umidade (%). Conta como um sensor, pois fornece duas medições relacionadas.
- **Botão (Push Button)**: Sensor opcional para simulação de batimentos cardíacos (BPM). Cada pressão no botão representa uma batida, convertida em BPM com base no intervalo de publicação.

### Simulação de Conectividade Wi-Fi
- Integrada com `cloudIsConnected()` da Parte 2, que verifica conexão MQTT/Wi-Fi no simulador Wokwi.
- Quando desconectado, leituras são bufferizadas; ao reconectar, buffer é sincronizado.

### Armazenamento Local e Resiliência Offline
- **Buffer circular**: Array de 100 leituras (`Reading` struct) para armazenamento temporário em RAM.
- **Estratégia de limite**: Máximo de 100 amostras, alinhado com monitoramento cardíaco contínuo (aproximadamente 8 minutos de dados a intervalos de 5 segundos). Quando cheio, sobrescreve as leituras mais antigas (FIFO).
- **Integração MQTT**: Leituras são enviadas via `enviarParaNuvem()` da Parte 2 quando conectadas.

## Fluxo de Funcionamento

### Setup
1. Inicialização do Serial (115200 baud) e DHT22.
2. Configuração do pino do botão como INPUT_PULLUP.
3. Chamada a `cloudBegin()` para iniciar Wi-Fi + MQTT.

### Loop Principal
1. **Amostragem contínua**: Verificação do estado do botão para contagem de batidas.
2. **Publicação periódica** (a cada 5 segundos):
   - Coleta de dados: timestamp (segundos desde boot), temperatura, umidade, BPM calculado.
   - Validação: Descarta leituras com NaN do DHT22.
   - **Lógica de conectividade**:
     - **Online (`cloudIsConnected() = true`)**:
       - Tenta enviar leitura atual via MQTT.
       - Se falhar, bufferiza.
       - Sincroniza buffer: envia leituras armazenadas (marcadas como `buffered = true`) e limpa o buffer.
     - **Offline (`cloudIsConnected() = false`)**:
       - Armazena leitura no buffer circular.
       - Continua operação normal, garantindo resiliência.

### Cálculo de BPM
- BPM = (batidas na janela × 60000) / intervalo_ms
- Janela = 5000 ms, então BPM = batidas × 12.
- Exemplo: 5 batidas em 5s = 60 BPM.

## Lógica de Resiliência Offline

A resiliência é crítica em aplicações médicas, onde interrupções de conectividade não podem resultar em perda de dados. A implementação segue estes princípios:

- **Coleta contínua**: O dispositivo continua lendo sensores independentemente do status de conectividade.
- **Bufferização inteligente**: Leituras são armazenadas em RAM quando offline, evitando perda.
- **Sincronização automática**: Ao reconectar, todas as leituras pendentes são enviadas antes das novas.
- **Limite de armazenamento**: 100 amostras previnem estouro de memória, priorizando dados recentes em cenários de longa desconexão.
- **Indicadores**: Campo `buffered` no JSON distingue dados em tempo real de recuperados.

### Cenário de Demonstração
1. Inicie com Wi-Fi/MQTT conectados: leituras são enviadas imediatamente.
2. Desconecte no Wokwi (desative Wi-Fi sim): leituras vão para buffer.
3. Aguarde algumas publicações: observe buffer_size crescendo.
4. Reconecte: buffer é esvaziado, dados marcados como buffered.

## Integração com Parte 2
- Usa `cloud_link.h` para MQTT TLS publishing to HiveMQ Cloud.
- Telemetria no tópico `cardioia/<deviceId>/telemetry` com JSON schema conforme AGENTS.md.
- Dashboard Node-RED assina wildcard `cardioia/+/telemetry` para visualização em tempo real.

## Considerações Técnicas

- **Limitações do simulador**: Buffer em RAM simula SPIFFS volátil; para produção, migrar para SPIFFS/microSD.
- **Desempenho**: Buffer circular O(1) para inserção/remoção, adequado para ESP32.
- **Segurança**: Dados em RAM são voláteis, mas simulam Edge Computing seguro (dados não saem do dispositivo até transmissão).
- **Escalabilidade**: Parte 2 implementa MQTT QoS 1 para at-least-once delivery.

## Conclusão

Esta implementação demonstra Edge Computing integrado com Cloud Broker e Fog Dashboard, formando o protótipo completo CardioIA. O buffer offline garante continuidade do monitoramento mesmo em condições adversas, enquanto a integração MQTT permite transmissão segura e visualização em tempo real. As extensões Ir Além 1 (REST + email) e Ir Além 2 (AI notebook) adicionam camadas de automação e análise avançada.

**Entregáveis**:
- Código C++ comentado em `firmware/src/main.cpp`.
- Dashboard Node-RED em `node-red/flows.json`.
- Scripts de teste em `scripts/`.
- Extensões opcionais em `ir_alem_1/` e `ir_alem_2/`.
- Este relatório.
