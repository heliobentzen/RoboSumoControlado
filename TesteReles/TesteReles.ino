// Teste dos relés — aciona um pino por vez e mostra no Serial Monitor (115200).
// Para cada pino, observe QUAL relé da placa reage e se ele desliga de verdade
// em cada um dos dois modos de "desligado".

const int pinos[] = {15, 33, 27, 12, 14};
const char* nomes[] = {"rele1 (GPIO 15)", "rele2 (GPIO 33)", "rele3 (GPIO 27)", "rele4 (GPIO 12)",
                       "rele5 (GPIO 14)"};
const int total = sizeof(pinos) / sizeof(pinos[0]);

void desligarTodos() {
    for (int i = 0; i < total; i++) {
        pinMode(pinos[i], OUTPUT);
        digitalWrite(pinos[i], HIGH);
    }
}

void setup() {
    desligarTodos();
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TESTE DOS RELES ===");
}

void loop() {
    for (int i = 0; i < total; i++) {
        int p = pinos[i];
        Serial.printf("\n>>> %s\n", nomes[i]);

        Serial.println("  DESLIGADO modo 1 (pino em HIGH, 3,3 V) ... 3 s");
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH);
        delay(3000);

        Serial.println("  LIGADO (pino em LOW) ... 2 s");
        digitalWrite(p, LOW);
        delay(2000);

        Serial.println("  DESLIGADO modo 2 (pino solto, open-drain) ... 3 s");
        pinMode(p, OUTPUT_OPEN_DRAIN);
        digitalWrite(p, HIGH);
        delay(3000);

        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH);
    }
    Serial.println("\n=== FIM DO CICLO, recomeçando em 5 s ===");
    delay(5000);
}
