# Robô Sumô Controlado por Bluetooth

Projeto de um robô da modalidade sumô controlado por um gamepad compatível com **Bluepad32**. A ESP32 recebe os comandos por Bluetooth e aciona os dois motores por meio de módulos de relé.

<p>
  <img src="./robos.jpeg" alt="Robô sumô construído" width="600">
</p>

## Objetivo

Um robô sumô tenta empurrar o adversário para fora do dohyo, a arena circular utilizada nas competições. Dependendo da categoria, as regras podem limitar peso, tamanho e tecnologias permitidas.

Este projeto utiliza controle remoto via Bluetooth. Futuramente, o firmware poderá ser expandido para incluir sensores, programação autônoma e controle proporcional dos motores.

## Componentes

| Quantidade | Componente |
| ---: | --- |
| 1 | ESP32 compatível com Bluepad32 |
| 1 | Módulo relé de 4 canais, 5 V |
| 1 | Módulo relé auxiliar de 1 canal, 5 V |
| 2 | Motores de vidro elétrico |
| 1 | Bateria LiPo 11,1 V |
| 1 | Case para bateria 18650 |
| 3 | Conectores XT60 macho/fêmea |
| 1 | Controle remoto compatível com Bluepad32 |
| 1 | Barra roscada de 8 mm |
| 2 | Rolamentos 628 2RS |
| 2 | Barras de pinos fêmea |
| 1 kg | Filamento para impressão 3D |

## Arquivos do Projeto

| Arquivo | Descrição |
| --- | --- |
| [Controller.ino](./Controller.ino) | Firmware principal da ESP32 |
| [robos.jpeg](./robos.jpeg) | Foto do robô montado |
| [Diagrama_conexao.jpg](./Diagrama_conexao.jpg) | Diagrama de ligação |
| [Chassi Remoto.stl](./Chassi%20Remoto.stl) | Modelo do chassi |

## Configuração da Arduino IDE

1. Instale a [Arduino IDE](https://www.arduino.cc/en/software).
2. Em **Arquivo > Preferências**, adicione esta URL ao campo **URLs adicionais para gerenciadores de placas**:

   ```text
   https://dl.espressif.com/dl/package_esp32_index.json
   ```

3. Instale o pacote **ESP32 by Espressif Systems** em **Ferramentas > Placa > Gerenciador de placas**.
4. Adicione o pacote Bluepad32 ao campo de URLs adicionais:

   ```text
   https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
   ```

5. Instale a placa **esp32_bluepad32** em **Ferramentas > Placa > Gerenciador de placas**.
6. Selecione o modelo correspondente da sua ESP32 em **Ferramentas > Placa**.
7. Abra `Controller.ino`, selecione a porta serial e faça o upload.

> **Importante:** selecione uma placa da plataforma **ESP32 + Bluepad32 Arduino**. Não use a biblioteca **Bluepad32 for NINA-W10 boards**: ela é destinada a placas Arduino com módulo NINA e possui uma API diferente. Se essa biblioteca estiver instalada e a compilação acusar métodos ausentes em `BP32` ou `Controller`, remova-a pelo Gerenciador de Bibliotecas e selecione a plataforma correta no Gerenciador de Placas.

## Conexão dos Relés

| Função | Pino da ESP32 |
| --- | ---: |
| Motor 1 - entrada 1 | GPIO 15 |
| Motor 1 - entrada 2 | GPIO 33 |
| Motor 2 - entrada 1 | GPIO 27 |
| Motor 2 - entrada 2 | GPIO 12 |
| Relé auxiliar | GPIO 14 |

> Os sinais ativos e a lógica de acionamento podem depender do módulo de relé utilizado. Confirme a polaridade e faça o primeiro teste com os motores desligados da alimentação principal.

## Controles

| Entrada | Ação |
| --- | --- |
| D-pad | Frente, trás, esquerda, direita e movimentos diagonais |
| Analógicos (Y do esquerdo, X do direito) | Frente/trás com os dois motores; giro só com o Y solto |
| Analógicos na posição neutra | Para os motores |
| R2 (segurar) | Investida: frente total, ignora D-pad e analógicos |
| L1 | Meia-volta: gira no próprio eixo por `TEMPO_MEIA_VOLTA_MS` |
| L2 | Parada de emergência (cancela tudo, inclusive a meia-volta) |
| R1 | Alterna o estado do relé auxiliar |

Prioridade dos comandos: L2 > meia-volta > L1 > R2 > D-pad > analógicos. No analógico, frente/trás sempre têm prioridade e usam os dois motores, para que um desvio leve do dedo não desligue um motor no meio do empurrão. As diagonais ficam só no D-pad. Ao conectar, o controle vibra e acende o LED (nos modelos que suportam). Apenas um controle pilota o robô: um segundo controle é recusado.

## Ajustes no Firmware

No início de `Controller.ino`:

| Ajuste | Padrão | Função |
| --- | ---: | --- |
| `ZONA_MORTA` | 100 | Folga do analógico ignorada (escala de -512 a 512) |
| `TEMPO_MEIA_VOLTA_MS` | 400 | Duração do giro do L1. Ajuste até o robô virar 180° |
| `TEMPO_REVERSAO_MS` | 0 | Pausa antes de inverter o sentido de um motor. 0 = resposta imediata (luta); 20–40 poupa os relés |
| `TIMEOUT_SEM_DADOS_MS` | 0 (desligado) | Para os motores se o controle ficar sem enviar dados. Use 500 com controles de PS4/PS5; não use com controles que só enviam dados quando algo muda (ex.: Xbox) |
| `DEBUG_CONTROLE` | 0 | Mostra os dados do controle no Serial Monitor |

## Segurança do Firmware

- A desconexão do controle para os motores imediatamente.
- A posição neutra do joystick para os motores.
- O botão L2 permite uma parada manual imediata.
- Opcional: breve parada nas inversões de sentido para poupar os relés (`TEMPO_REVERSAO_MS`).
- Opcional: parada automática quando o controle fica sem enviar dados (`TIMEOUT_SEM_DADOS_MS`).
- Use carregadores apropriados e desconecte a alimentação antes de mexer nas ligações.

## Diagrama de Ligação

<p>
  <img src="./Diagrama_conexao.jpg" alt="Diagrama de ligação do projeto" width="600">
</p>

## Melhorias Planejadas

- Controle proporcional dos motores por PWM.
- Substituição dos relés por uma ponte H.
- Uso de sensores infravermelhos ou ultrassônicos.
- Modo totalmente autônomo.
- Velocidade configurável pelo firmware.
- Indicadores visuais de conexão e status.

## Créditos

- [Hélio Bentzen](https://github.com/heliobentzen) — desenvolvimento e manutenção do projeto.
- [Ricardo Quesada](https://github.com/ricardoquesada/bluepad32) — biblioteca Bluepad32.
- [Abraão Lacerda](https://www.youtube.com/watch?v=K_rIaBKQcCk) — referência utilizada no projeto.
- **GitHub Copilot usando Copilot SDK + Claude Code no VS Code** — melhorias de firmware, segurança, organização do código e documentação.
