# Estação Meteorológica com Arduino e Ethernet

Este projeto implementa a leitura de dados de uma estação meteorológica por meio de um Arduino, realiza o processamento básico dessas informações e as envia para um servidor via Ethernet em formato JSON para consumo por uma aplicação externa.

## Visão geral

O código foi desenvolvido para receber dados de uma estação meteorológica sem fio através de um receptor RF conectado ao Arduino.  
Os dados recebidos são interpretados por uma classe `WeatherSensor` e, sempre que um novo pacote é adquirido, as informações são exibidas no Serial Monitor e enviadas para uma API HTTP em um servidor remoto.

## Funcionalidades

- Leitura de sinais RF no pino digital `2` (constante `RF_IN`).  
- Utilização de um objeto `WeatherSensor` para decodificar os dados da estação (temperatura, umidade, vento, chuva, bateria, etc.). 
- Contagem de intervalos entre pulsos via Timer1 e interrupção (`ISR(TIMER1_COMPA_vect)`), usada como base para decodificação do protocolo do sensor.  
- Exibição dos dados meteorológicos no Serial Monitor, incluindo:
  - Tempo de intervalo entre pacotes.
  - Quantidade de pacotes recebidos.
  - ID do sensor.
  - Temperatura.
  - Umidade.
  - Direção do vento (string).
  - Velocidade do vento em km/h.
  - Chuva acumulada.
  - Nível da bateria.  
- Envio dos dados para um servidor remoto via `EthernetClient` usando requisição HTTP `POST` com corpo em JSON.

## Hardware e conexões

- Placa Arduino compatível com Timer1 (ex.: Arduino Uno). 
- Módulo/receptor RF conectado ao pino digital `2` (`RF_IN`).  
- Shield ou módulo Ethernet compatível com a biblioteca `Ethernet.h`. 
- Estação meteorológica sem fio compatível com a classe `WeatherSensor` utilizada no código.

> Observação: o código assume a existência da classe `WeatherSensor` e das bibliotecas necessárias (Ethernet, SPI, ArduinoJson, etc.), que devem estar disponíveis no projeto.

## Estrutura do código

### Classe `WeatherStation`

A classe `WeatherStation` encapsula a lógica principal do sistema.

- `setup()`:
  - Inicializa o Timer1 para geração de interrupções periódicas.
  - Inicializa a comunicação serial em 115200 bps.
  - Configura o pino `RF_IN` como entrada.
  - Habilita interrupções globais (`sei()`).
  - Inicializa a interface Ethernet com o MAC address definido e mostra o resultado no Serial.

- `loop()`:
  - Verifica se há um novo intervalo disponível (`weather._interval`).
  - Quando há dados, chama `processWeatherData()` e zera o flag `_interval`.

- `initializeTimer()`:
  - Configura os registradores do Timer1 para operar com interrupções periódicas no compare match (`TIMER1_COMPA_vect`).

- `processWeatherData()`:
  - Chama `weather.Receiver(weather.interval)` para decodificar o sinal.
  - Se um pacote válido for adquirido (`weather.acquired()`), atualiza estatísticas, imprime os dados no Serial e envia os dados para o servidor.

- `updateWeatherStats()`:
  - Atualiza:
    - `weather.now` com `millis()`.
    - `weather.spacing` (diferença entre o tempo atual e o anterior).
    - `weather.old` com o tempo atual.
    - `weather.packet_count` (contador de pacotes).
    - `weather.average_interval` (intervalo médio).

- `printWeatherData()`:
  - Imprime no Serial todas as métricas lidas do sensor (intervalo, pacotes, ID, temperatura, umidade, direção e velocidade do vento, chuva, bateria).

- `sendWeatherDataToServer()`:
  - Abre conexão TCP com o servidor definido em `IPAddress server(138, 118, 76, 228)` na porta `6097`.  
  - Monta um `StaticJsonDocument<200>` com os campos:
    - `spacing`
    - `packet_count`
    - `sensor_id`
    - `temperature`
    - `humidity`
    - `wind_speed_kmh`
    - `rainfall`
    - `battery`.  
  - Envia uma requisição HTTP `POST /temp` com cabeçalhos básicos e o JSON no corpo. 
  - Encerra a conexão com `client.stop()`.

### Interrupção do Timer1

A rotina `ISR(TIMER1_COMPA_vect)` faz a leitura de nível lógico do pino `RF_IN` em alta frequência para medir a duração dos pulsos do sinal RF.

- Incrementa um contador (`count`) sempre que o pino está em nível alto.[file:31]  
- Quando o sinal cai (transição de HIGH para LOW), armazena o valor de `count` em `weather.interval`, seta `weather._interval = 1` e zera o contador.
- Esse mecanismo fornece a base de tempo para o método `weather.Receiver()` decodificar os dados da estação.

## Fluxo de funcionamento

1. A estação meteorológica envia dados por RF para o receptor conectado ao Arduino.
2. A interrupção de Timer1 mede os intervalos dos pulsos de RF e atualiza `weather.interval` sempre que um pacote é identificado.
3. No loop principal, quando `_interval` é marcado, o código chama `weather.Receiver()` para tentar decodificar o pacote. 
4. Se a decodificação for bem-sucedida, as estatísticas são atualizadas, os dados são mostrados no Serial e enviados ao servidor via HTTP POST em JSON. 
5. Uma aplicação no servidor pode consumir esse JSON para armazenar, exibir ou processar as informações da estação meteorológica.

## Como usar

1. Ajuste o IP do servidor e rota (atualmente `POST /temp` para `138.118.76.228:6097`) conforme sua infraestrutura. 
2. Certifique-se de que a biblioteca `WeatherSensor` e as demais dependências (Ethernet, SPI, ArduinoJson, etc.) estejam instaladas no ambiente do Arduino IDE.  
3. Faça o upload do código para a placa Arduino. 
4. Abra o Serial Monitor a 115200 bps para acompanhar os dados recebidos e o status de conexão com o servidor.  
5. Verifique no servidor se os dados JSON estão chegando corretamente e integre com sua aplicação (dashboard, API, banco de dados, etc.).

## Objetivo do projeto

Este código foi utilizado em uma placa Arduino para receber informações de uma estação meteorológica e enviá-las para um sistema externo via rede, permitindo monitoramento remoto e integração com aplicações web ou APIs.
