#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RoboEyes.h>

#define BUZZER_PIN_1 18
#define BUTTON_PIN 23
#define LED_PIN 19

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
FluxGarage::RoboEyes roboEyes;

int musicaSelecionada = 0;
const int totalMusicas = 4;

// Modo: false = seleção de música, true = olhos de robô
bool modoOlhos = false;

const char* nomesMusicas[] = {
  "Mario",
  "Megalovania",
  "Harry Potter",
  "Never Gonna"
};

void mostrarMusicaSelecionada() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Selecionada:");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(nomesMusicas[musicaSelecionada]);
  display.display();
}

void mostrarTocando() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Tocando:");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(nomesMusicas[musicaSelecionada]);
  display.display();
}

void tocarMusica(int* melodia, int* duracoes, int totalNotas) {
  mostrarTocando();

  for (int i = 0; i < totalNotas; i++) {
    int nota = melodia[i];

    if (nota == 0) {
      ledcWriteTone(BUZZER_PIN_1, 0);
      digitalWrite(LED_PIN, LOW);
    } else {
      ledcWriteTone(BUZZER_PIN_1, nota);
      digitalWrite(LED_PIN, HIGH);
    }

    delay(duracoes[i]);
    ledcWriteTone(BUZZER_PIN_1, 0);
    digitalWrite(LED_PIN, LOW);
    delay(duracoes[i] / 5);
  }
}

void entrarModoOlhos() {
  modoOlhos = true;
  display.clearDisplay();
  display.display();
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setMood(TIRED);
  roboEyes.setAutoblinker(ON, 3, 1);
  roboEyes.setIdleMode(ON, 2, 2);
}

void sairModoOlhos() {
  modoOlhos = false;
  roboEyes.setAutoblinker(OFF);
  roboEyes.setIdleMode(OFF);
  mostrarMusicaSelecionada();
}

// --- Músicas ---

void prepararMario() {
  int notas[] = {
    2637, 2637, 0, 2637, 0, 2093, 2637, 0, 3136, 0, 0, 0, 1568, 0, 0, 0,
    2093, 0, 0, 1568, 0, 0, 1319, 0, 0, 1760, 0, 1976, 0, 1865, 1760, 0,
    1568, 2637, 3136, 1760, 0, 2794, 3136, 0, 2637, 0, 2093, 2349, 1976, 0, 0
  };
  int tempos[] = {
    83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
    83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
    111, 111, 111, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83
  };
  tocarMusica(notas, tempos, sizeof(notas) / sizeof(notas[0]));
}

void prepararMegalovania() {
  int notas[] = {
    294, 294, 587, 440, 415, 392, 349, 294, 349, 392,
    261, 261, 587, 440, 415, 392, 349, 294, 349, 392,
    247, 247, 587, 440, 415, 392, 349, 294, 349, 392,
    233, 233, 587, 440, 415, 392, 349, 294, 349, 392
  };
  int tempos[] = {
    125, 125, 250, 375, 250, 250, 250, 125, 125, 125,
    125, 125, 250, 375, 250, 250, 250, 125, 125, 125,
    125, 125, 250, 375, 250, 250, 250, 125, 125, 125,
    125, 125, 250, 375, 250, 250, 250, 125, 125, 125
  };
  tocarMusica(notas, tempos, sizeof(notas) / sizeof(notas[0]));
}

void prepararHarryPotter() {
  int notas[] = {
    494, 659, 784, 740, 659, 988, 880, 740,
    659, 784, 740, 622, 659, 494
  };
  int tempos[] = {
    300, 450, 150, 300, 600, 300, 900, 900,
    450, 150, 300, 600, 300, 900
  };
  tocarMusica(notas, tempos, sizeof(notas) / sizeof(notas[0]));
}

void prepararNeverGonnaGiveYouUp() {
  int notas[] = {
    440, 494, 587, 494, 740, 740, 659, 440, 494,
    587, 494, 659, 659, 587, 554, 494, 440, 494, 587, 494, 587,
    659, 554, 494, 440, 440, 440, 659, 587, 440, 494, 587, 494,
    740, 740, 659, 440, 494, 587, 494, 880, 554, 587, 554, 494,
    440, 494, 587, 494, 587, 659, 554, 494, 440, 440, 659, 587,
    0, 0
  };
  int tempos[] = {
    132, 132, 132, 132, 395, 395, 789, 132, 132,
    132, 132, 395, 395, 395, 132, 395, 132, 132, 132, 132, 526,
    263, 395, 132, 263, 263, 263, 526, 1053, 132, 132, 132, 132,
    395, 395, 789, 132, 132, 132, 132, 526, 263, 395, 132, 263,
    132, 132, 132, 132, 526, 263, 395, 132, 526, 263, 526, 1053,
    526, 263
  };
  tocarMusica(notas, tempos, sizeof(notas) / sizeof(notas[0]));
}

// --- Leitura do botão ---

unsigned long lerTempoBotao(int pinBotao) {
  unsigned long tempoInicio = millis();
  while (digitalRead(pinBotao) == LOW) {
    delay(10);
  }
  return millis() - tempoInicio;
}

// --- Setup e Loop ---

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ledcAttach(BUZZER_PIN_1, 2000, 10);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }

  display.clearDisplay();
  display.display();

  mostrarMusicaSelecionada();
}

void loop() {
  if (modoOlhos) {
    roboEyes.update();

    if (digitalRead(BUTTON_PIN) == LOW) {
      unsigned long tempo = lerTempoBotao(BUTTON_PIN);
      if (tempo >= 800) {
        sairModoOlhos();
      }
    }

  } else {
    if (digitalRead(BUTTON_PIN) == LOW) {
      unsigned long tempo = lerTempoBotao(BUTTON_PIN);

      if (tempo >= 800) {
        // Toca a música e depois entra no modo olhos
        switch (musicaSelecionada) {
          case 0: prepararMario(); break;
          case 1: prepararMegalovania(); break;
          case 2: prepararHarryPotter(); break;
          case 3: prepararNeverGonnaGiveYouUp(); break;
        }
        entrarModoOlhos();

      } else if (tempo > 50) {
        musicaSelecionada = (musicaSelecionada + 1) % totalMusicas;
        mostrarMusicaSelecionada();

        digitalWrite(LED_PIN, HIGH);
        delay(120);
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
}