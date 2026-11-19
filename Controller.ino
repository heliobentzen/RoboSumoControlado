#include <Bluepad32.h>
#include <esp_system.h>  // esp_reset_reason()

// =====================================================================
//  Robô Sumô Controlado — firmware da ESP32 (Bluepad32)
//
//  Placa alvo: ESP32 clássico (ex.: "ESP32 Dev Module" em esp32_bluepad32).
//  O ESP32-S3 não tem Bluetooth Clássico (controles de PS3/PS4/PS5/Switch
//  não pareiam) e não possui os GPIOs 27 e 33 livres.
// =====================================================================

// ----------------------------- AJUSTES -------------------------------

// Exibe no Serial Monitor os dados do controle a cada leitura.
// Deixe em 0 durante as lutas: imprimir tudo isso deixa o robô mais lento.
#define DEBUG_CONTROLE 1  // TEMPORÁRIO: diagnóstico do relé 3. Voltar para 0 antes das lutas.

// Zona morta do analógico (escala de -512 a 512). Evita que a folga do
// analógico em repouso movimente o robô.
#define ZONA_MORTA 100

// Ao inverter o sentido de um motor (frente -> trás), ele fica desligado
// por este tempo antes de girar ao contrário. Poupa os relés, mas atrasa a
// resposta. Em luta, resposta vale mais: 0 = inversão imediata.
// Se os contatos dos relés começarem a falhar, use algo entre 20 e 40.
#define TEMPO_REVERSAO_MS 0

// L1 faz o robô girar no próprio eixo por este tempo (meia-volta).
// Ajuste testando no seu robô até ele virar 180 graus.
#define TEMPO_MEIA_VOLTA_MS 400

// Para os motores se o controle ficar este tempo sem enviar dados.
// ATENÇÃO: controles de PS4/PS5 enviam dados o tempo todo, mas alguns
// (ex.: Xbox) só enviam quando algo muda. Com eles, segurar o analógico
// parado faria o robô parar sozinho. Por isso vem desativado (0).
// Com controle de PS4/PS5, use por exemplo 500.
#define TIMEOUT_SEM_DADOS_MS 0

// ------------------------------ PINOS --------------------------------
// ATENÇÃO: o GPIO 12 é um pino de "strapping" do ESP32. Se ele estiver em nível
// alto no momento em que a placa liga, ela pode não iniciar ou ficar reiniciando.
// Se isso acontecer com o módulo de relés conectado, mude o rele4 para o GPIO 26
// (e mova o fio do IN1 para o pino 26).
#define rele4 12  // IN1 do relé (módulo 1) — motor 2
#define rele3 27  // IN2 do relé (módulo 1) — motor 2
#define rele2 33  // IN1 do relé (módulo 2) — motor 1
#define rele1 15  // IN2 do relé (módulo 2) — motor 1
#define rele5 14  // relé auxiliar
#define PINO_13 13

// Os relés são acionados em nível BAIXO.
#define RELE_LIGADO LOW
#define RELE_DESLIGADO HIGH

// ------------------------------ D-PAD --------------------------------
#define FRENTE 1
#define TRAS 2
#define DIREITA 4
#define ESQUERDA 8
#define FRENTE_DIREITA 5
#define TRAS_DIREITA 6
#define FRENTE_ESQUERDA 9
#define TRAS_ESQUERDA 10

// ============================ MOTORES ================================
// Cada motor tem 3 estados: +1 (frente), 0 (parado) e -1 (trás).

struct Motor {
    uint8_t pinoFrente;     // relé que, ligado, faz o motor ir para frente
    uint8_t pinoTras;       // relé que, ligado, faz o motor ir para trás
    int8_t atual;           // sentido aplicado agora nos relés
    int8_t desejado;        // sentido pedido pelo controle
    unsigned long paradoEm; // instante em que o motor foi desligado
};

// Mesma lógica dos relés da versão anterior:
//   motor 1: rele2 ligado = frente, rele1 ligado = trás
//   motor 2: rele4 ligado = frente, rele3 ligado = trás
Motor motor1 = {rele2, rele1, 0, 0, 0};
Motor motor2 = {rele4, rele3, 0, 0, 0};

void escreverMotor(Motor& m, int8_t sentido) {
    digitalWrite(m.pinoFrente, sentido > 0 ? RELE_LIGADO : RELE_DESLIGADO);
    digitalWrite(m.pinoTras, sentido < 0 ? RELE_LIGADO : RELE_DESLIGADO);
    if (sentido == 0 && m.atual != 0) {
        m.paradoEm = millis();
    }
    m.atual = sentido;
}

// Aplica o sentido desejado respeitando o tempo de reversão.
// É chamada a cada volta do loop, então não trava o programa.
void atualizarMotor(Motor& m) {
    if (m.desejado == m.atual) {
        return;
    }
    if (m.desejado == 0) {
        escreverMotor(m, 0);
        return;
    }
    if (m.atual != 0) {
        // Inversão direta: desliga primeiro e espera o tempo de reversão.
        escreverMotor(m, 0);
        if (TEMPO_REVERSAO_MS > 0) {
            return;
        }
    }
    if (millis() - m.paradoEm >= TEMPO_REVERSAO_MS) {
        escreverMotor(m, m.desejado);
    }
}

void mover(int8_t sentidoMotor1, int8_t sentidoMotor2) {
    motor1.desejado = sentidoMotor1;
    motor2.desejado = sentidoMotor2;
    atualizarMotor(motor1);
    atualizarMotor(motor2);
}

// Parada imediata (sem esperar nada).
void stop() {
    motor1.desejado = 0;
    motor2.desejado = 0;
    escreverMotor(motor1, 0);
    escreverMotor(motor2, 0);
}

void movimento_frente()          { mover(+1, +1); }
void movimento_tras()            { mover(-1, -1); }
void movimento_esquerda()        { mover(-1, +1); }
void movimento_direita()         { mover(+1, -1); }
void movimento_frente_esquerda() { mover(+1,  0); }
void movimento_frente_direita()  { mover( 0, +1); }
void movimento_tras_esquerda()   { mover( 0, -1); }
void movimento_tras_direita()    { mover(-1,  0); }

// ======================= COMANDOS DO CONTROLE ========================

void controleRoboSetas(int xy) {
    switch (xy) {
        case FRENTE:          movimento_frente();          break;
        case TRAS:            movimento_tras();            break;
        case ESQUERDA:        movimento_esquerda();        break;
        case DIREITA:         movimento_direita();         break;
        case FRENTE_ESQUERDA: movimento_frente_esquerda(); break;
        case FRENTE_DIREITA:  movimento_frente_direita();  break;
        case TRAS_ESQUERDA:   movimento_tras_esquerda();   break;
        case TRAS_DIREITA:    movimento_tras_direita();    break;
        default:              stop();                      break;  // combinação inválida
    }
}

// Analógico como na versão original: frente/trás têm prioridade e sempre usam
// os DOIS motores (força total no empurrão). O giro só acontece com o eixo Y
// solto. Sem diagonais aqui: elas desligam um motor e, com o dedo levemente
// torto, o robô perdia metade da força. Diagonais ficam no D-pad.
// Sentido do eixo X igual ao original: X negativo = direita.
void controleRoboJoystick(int eixoX, int eixoY) {
    if (eixoY <= -ZONA_MORTA) {
        movimento_frente();
    } else if (eixoY >= ZONA_MORTA) {
        movimento_tras();
    } else if (eixoX <= -ZONA_MORTA) {
        movimento_direita();
    } else if (eixoX >= ZONA_MORTA) {
        movimento_esquerda();
    } else {
        stop();  // analógico solto
    }
}

// O D-pad tem prioridade; o analógico só é usado quando nenhuma seta está
// pressionada. Assim os dois não "brigam" pelo comando dos motores.
void controlarMovimento(ControllerPtr ctl) {
    int xy = ctl->dpad();
    if (xy != 0) {
        controleRoboSetas(xy);
    } else {
        controleRoboJoystick(ctl->axisRX(), ctl->axisY());
    }
}

void dumpGamepad(ControllerPtr ctl) {
#if DEBUG_CONTROLE
    Serial.printf("dpad: 0x%02x, botões: 0x%04x, eixo L: %4d, %4d, eixo R: %4d, %4d, motores: %d %d\n",
                  ctl->dpad(), ctl->buttons(), ctl->axisX(), ctl->axisY(), ctl->axisRX(), ctl->axisRY(),
                  motor1.atual, motor2.atual);
#endif
}

// Estado da meia-volta (L1).
bool meiaVoltaAtiva = false;
unsigned long meiaVoltaInicio = 0;

ControllerPtr controle = nullptr;
unsigned long ultimoDado = 0;

void processGamepad(ControllerPtr ctl) {
    // R1 alterna o relé auxiliar só no instante em que é apertado (borda de subida).
    static bool r1Anterior = false;
    static bool estadoRele = false;
    bool r1Agora = ctl->r1();
    if (r1Agora && !r1Anterior) {
        estadoRele = !estadoRele;
        digitalWrite(rele5, estadoRele ? HIGH : LOW);
    }
    r1Anterior = r1Agora;

    static bool l1Anterior = false;
    bool l1Agora = ctl->l1();
    bool l1Apertado = l1Agora && !l1Anterior;
    l1Anterior = l1Agora;

    // Prioridades: L2 (freio) > meia-volta em andamento > L1 > R2 > D-pad > analógico.
    if (ctl->l2()) {
        meiaVoltaAtiva = false;
        stop();
    } else if (meiaVoltaAtiva) {
        // Deixa a meia-volta terminar; o loop() encerra no tempo certo.
    } else if (l1Apertado) {
        meiaVoltaAtiva = true;
        meiaVoltaInicio = millis();
        movimento_esquerda();  // gira no próprio eixo
    } else if (ctl->r2()) {
        movimento_frente();    // investida: frente total, ignora D-pad e analógico
    } else {
        controlarMovimento(ctl);
    }

    dumpGamepad(ctl);
}

// Chamada no loop(): encerra a meia-volta e retoma o comando atual do
// controle (ex.: R2 segurado), sem esperar o próximo pacote do controle.
void atualizarMeiaVolta() {
    if (meiaVoltaAtiva && millis() - meiaVoltaInicio >= TEMPO_MEIA_VOLTA_MS) {
        meiaVoltaAtiva = false;
        if (controle && controle->isConnected()) {
            processGamepad(controle);
        } else {
            stop();
        }
    }
}

// ============================ CONEXÃO ================================
// Só um controle pilota o robô. Um segundo controle que tente se conectar
// é recusado, para que dois controles não disputem os motores.

void onConnectedController(ControllerPtr ctl) {
    if (controle != nullptr) {
        Serial.println("Segundo controle recusado: o robô já tem um controle conectado.");
        ctl->disconnect();
        return;
    }
    controle = ctl;
    ultimoDado = millis();
    Serial.printf("Controle conectado: %s\n", ctl->getModelName().c_str());

    // Avisa no próprio controle que a conexão deu certo (quando ele suporta).
    ctl->setColorLED(0, 255, 0);
    ctl->setPlayerLEDs(0x01);
    ctl->playDualRumble(0, 200, 0x80, 0x40);
}

void onDisconnectedController(ControllerPtr ctl) {
    if (ctl != controle) {
        return;  // era o segundo controle recusado
    }
    // Segurança: se o controle desconectar (bateria, distância), os motores param.
    meiaVoltaAtiva = false;
    stop();
    controle = nullptr;
    Serial.println("Controle desconectado: motores parados.");
}

void processControllers() {
    if (controle && controle->isConnected() && controle->hasData() && controle->isGamepad()) {
        ultimoDado = millis();
        processGamepad(controle);
    }
}

void verificarTimeout() {
#if TIMEOUT_SEM_DADOS_MS > 0
    bool motoresLigados = motor1.desejado != 0 || motor2.desejado != 0;
    if (controle && motoresLigados && millis() - ultimoDado > TIMEOUT_SEM_DADOS_MS) {
        meiaVoltaAtiva = false;
        stop();
        Serial.println("Sem dados do controle: motores parados.");
    }
#endif
}

// ============================ ARDUINO ================================

void setup() {
    // Relés começam desligados antes de qualquer outra coisa.
    pinMode(rele1, OUTPUT);
    pinMode(rele2, OUTPUT);
    pinMode(rele3, OUTPUT);
    pinMode(rele4, OUTPUT);
    pinMode(rele5, OUTPUT);
    pinMode(PINO_13, OUTPUT);
    stop();
    digitalWrite(rele5, LOW);
    digitalWrite(PINO_13, HIGH);

    Serial.begin(115200);
    // Motivo do último reinício. Se aparecer "BROWNOUT" ou se esta mensagem
    // se repetir sem parar, a placa está reiniciando (problema de alimentação
    // ou do GPIO 12), e não um erro de lógica do firmware.
    const char* motivos[] = {"DESCONHECIDO", "LIGOU", "EXTERNO", "SOFTWARE", "PANIC (travou)",
                             "WATCHDOG INT", "WATCHDOG TASK", "WATCHDOG", "DEEP SLEEP", "BROWNOUT", "SDIO"};
    int motivo = (int)esp_reset_reason();
    Serial.printf("\n=== INICIANDO === motivo do reinicio: %s\n",
                  motivo < (int)(sizeof(motivos) / sizeof(motivos[0])) ? motivos[motivo] : "OUTRO");
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    BP32.setup(&onConnectedController, &onDisconnectedController);

    // Se o controle tiver problemas para reconectar, descomente a linha abaixo,
    // grave, pareie de novo e depois comente outra vez.
    // BP32.forgetBluetoothKeys();

    // Sem isso, controles de PS4/PS5 criam um segundo dispositivo (touchpad como mouse).
    BP32.enableVirtualDevice(false);
}

void loop() {
    if (BP32.update()) {
        processControllers();
    }
    atualizarMeiaVolta();
    verificarTimeout();

    // Conclui inversões de sentido que estavam aguardando o tempo de reversão.
    atualizarMotor(motor1);
    atualizarMotor(motor2);

    // Cede 1 ms para alimentar o watchdog: o mínimo possível, para o robô
    // reagir ao controle o mais rápido que der.
    delay(1);
}
