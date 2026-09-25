/* ==================================================================
   ####  PROJETO PARA ESP32-C3 MINI  ####
   (tambem chamado de ESP32-C3 SuperMini / C3 Mini)

   >>> ESTE CODIGO NAO SERVE PARA O ESP32 COMUM (WROOM/DevKit) <<<
   Os pinos sao diferentes. No ESP32-C3 Mini existem SO os GPIO
   0 a 10, 20 e 21. Os pinos 11 a 17 sao da memoria flash interna
   e os pinos 18 e 19 sao o USB - se voce usar 18/19 a placa fica
   conectando e desconectando sem parar.
   ------------------------------------------------------------------
   CONFIGURACAO OBRIGATORIA NA IDE (menu Ferramentas):
     Placa .................. ESP32C3 Dev Module
     USB CDC On Boot ........ Enabled     <-- nao esqueca deste
     Core (Gerenciador) ..... esp32 by Espressif  (2.x ou 3.x, ambos ok)
     Bibliotecas ............ Adafruit GFX + Adafruit SSD1306
   ------------------------------------------------------------------
   LIGACOES NO ESP32-C3 MINI:
     GPIO5  -> OLED SDA          GPIO0  -> LED carro VERMELHO
     GPIO6  -> OLED SCL          GPIO1  -> LED carro AMARELO
     GPIO7  -> Buzzer passivo    GPIO3  -> LED carro VERDE
     GPIO10 -> Botao (p/ GND)    GPIO4  -> LED pedestre VERMELHO
     3V3    -> OLED VCC          GPIO20 -> LED pedestre VERDE
     GND    -> OLED GND, buzzer, resistores dos LEDs (220-330 ohm)

   NAO use GPIO2, GPIO8 e GPIO9: sao pinos de boot do C3 Mini e a
   placa pode nao ligar. O GPIO8 tem o LED azul da placa.
   ------------------------------------------------------------------
   Modos: Pianinho | Desenho | Semaforo  (+ 1 modo secreto)

   COMO USAR O BOTAO (sempre igual, em qualquer tela):
     clique rapido (< 0,6 s)  -> passa para o proximo item
     segurar medio (> 0,6 s)  -> confirma / executa
     segurar longo (> 2,5 s)  -> voltar  (no MENU = segredo)
   ================================================================== */

// Travessa a compilacao se a placa selecionada nao for um ESP32-C3
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  #error "PLACA ERRADA! Selecione Ferramentas > Placa > ESP32 Arduino > ESP32C3 Dev Module. Este sketch e exclusivo para ESP32-C3 Mini."
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- PINOS - GPIOs DO ESP32-C3 MINI (mude aqui se precisar) ----------
// Atencao: sao os numeros de GPIO do C3 Mini, NAO os do ESP32 comum.
#define PIN_SDA        5    // OLED SDA
#define PIN_SCL        6    // OLED SCL
#define PIN_BOTAO     10    // botao -> GND  (usa pull-up interno)
#define PIN_BUZZER     7    // buzzer PASSIVO

#define LED_C_VERM     0    // semaforo carros - vermelho
#define LED_C_AMAR     1    // semaforo carros - amarelo
#define LED_C_VERDE    3    // semaforo carros - verde
#define LED_P_VERM     4    // semaforo pedestre - vermelho
#define LED_P_VERDE   20    // semaforo pedestre - verde

const uint8_t LEDS[5] = { LED_C_VERM, LED_C_AMAR, LED_C_VERDE, LED_P_VERM, LED_P_VERDE };


// ---- compatibilidade core ESP32 2.x / 3.x (buzzer) ----
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define BUZZER_INIT()   ledcAttach(PIN_BUZZER, 2000, 10)
  #define BUZZER_TOM(f)   ledcWriteTone(PIN_BUZZER, (f))
#else
  #define BUZZER_CANAL 0
  #define BUZZER_INIT()   do { ledcSetup(BUZZER_CANAL, 2000, 10); ledcAttachPin(PIN_BUZZER, BUZZER_CANAL); } while (0)
  #define BUZZER_TOM(f)   ledcWriteTone(BUZZER_CANAL, (f))
#endif

// ------------------ OLED ------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ------------------ BOTAO ------------------
#define T_DEBOUNCE    40UL
#define T_LONGO      600UL     // confirmar / executar
#define T_SEGREDO   2500UL     // voltar (ou segredo, no menu)

enum { EV_NENHUM = 0, EV_CURTO, EV_LONGO, EV_SEGREDO };

bool          btnPress   = false;
unsigned long btnT0      = 0;
bool          btnIgnorar = false;

uint8_t btnLer() {
  bool agora = (digitalRead(PIN_BOTAO) == LOW);
  uint8_t ev = EV_NENHUM;

  if (agora && !btnPress) {
    btnPress = true;
    btnT0 = millis();
  } else if (!agora && btnPress) {
    unsigned long d = millis() - btnT0;
    btnPress = false;
    if (btnIgnorar) { btnIgnorar = false; return EV_NENHUM; }
    if      (d >= T_SEGREDO)  ev = EV_SEGREDO;
    else if (d >= T_LONGO)    ev = EV_LONGO;
    else if (d >= T_DEBOUNCE) ev = EV_CURTO;
  }
  return ev;
}

unsigned long btnSegurando() { return btnPress ? (millis() - btnT0) : 0; }
void btnDescartar() { if (btnPress) btnIgnorar = true; }   // ignora a proxima soltura

// ------------------ LEDS / SOM ------------------
void ledsApaga() { for (uint8_t i = 0; i < 5; i++) digitalWrite(LEDS[i], LOW); }

void ledPorNota(int f) {
  ledsApaga();
  if (f <= 0) return;
  uint8_t idx;
  if      (f < 300)  idx = 0;
  else if (f < 500)  idx = 1;
  else if (f < 800)  idx = 2;
  else if (f < 1500) idx = 3;
  else               idx = 4;
  digitalWrite(LEDS[idx], HIGH);
}

void silencio() { BUZZER_TOM(0); ledsApaga(); }

// espera "ms" observando o botao
// retorno: 0 = seguiu normal | 1 = parar o que esta tocando | 2 = sair do modo
uint8_t esperar(unsigned long ms) {
  unsigned long t = millis();
  while (millis() - t < ms) {
    uint8_t ev = btnLer();
    if (ev == EV_LONGO)   return 1;
    if (ev == EV_SEGREDO) return 2;
    if (btnSegurando() >= T_SEGREDO) { btnDescartar(); return 2; }
    delay(2);
  }
  return 0;
}

uint8_t tocarMusica(const int* notas, const int* tempos, int n) {
  for (int i = 0; i < n; i++) {
    int f = notas[i];
    if (f == 0) silencio();
    else { BUZZER_TOM(f); ledPorNota(f); }

    uint8_t r = esperar(tempos[i]);
    if (r) { silencio(); return r; }

    silencio();
    r = esperar(tempos[i] / 5);
    if (r) { silencio(); return r; }
  }
  silencio();
  return 0;
}

// ============================================================
//                          MUSICAS
// ============================================================
#define TAM(a) (sizeof(a) / sizeof(a[0]))

// ------------------------------------------------------------
// MARIO
// ------------------------------------------------------------
const int marioN[] = {
  2637, 2637, 0, 2637, 0, 2093, 2637, 0, 3136, 0, 0, 0, 1568, 0, 0, 0,
  2093, 0, 0, 1568, 0, 0, 1319, 0, 0, 1760, 0, 1976, 0, 1865, 1760, 0,
  1568, 2637, 3136, 1760, 0, 2794, 3136, 0, 2637, 0, 2093, 2349, 1976, 0, 0
};

const int marioT[] = {
  83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
  83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
  111, 111, 111, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83
};


// ------------------------------------------------------------
// MEGALOVANIA
// ------------------------------------------------------------
const int megaN[] = {
  294, 294, 587, 440, 415, 392, 349, 294, 349, 392,
  261, 261, 587, 440, 415, 392, 349, 294, 349, 392,
  247, 247, 587, 440, 415, 392, 349, 294, 349, 392,
  233, 233, 587, 440, 415, 392, 349, 294, 349, 392
};

const int megaT[] = {
  125, 125, 250, 375, 250, 250, 250, 125, 125, 125,
  125, 125, 250, 375, 250, 250, 250, 125, 125, 125,
  125, 125, 250, 375, 250, 250, 250, 125, 125, 125,
  125, 125, 250, 375, 250, 250, 250, 125, 125, 125
};


// ------------------------------------------------------------
// HARRY POTTER
// ------------------------------------------------------------
const int hpN[] = {
  494, 659, 784, 740, 659, 988, 880,
  740, 659, 784, 740, 622, 659, 494
};

const int hpT[] = {
  300, 450, 150, 300, 600, 300, 900,
  900, 450, 150, 300, 600, 300, 900
};


// ------------------------------------------------------------
// NEVER GONNA GIVE YOU UP
// ------------------------------------------------------------
const int rickN[] = {
  440, 494, 587, 494, 740, 740, 659, 440, 494,
  587, 494, 659, 659, 587, 554, 494, 440, 494, 587, 494, 587,
  659, 554, 494, 440, 440, 440, 659, 587, 440, 494, 587, 494,
  740, 740, 659, 440, 494, 587, 494, 880, 554, 587, 554, 494,
  440, 494, 587, 494, 587, 659, 554, 494, 440, 440, 659, 587,
  0, 0
};

const int rickT[] = {
  132, 132, 132, 132, 395, 395, 789, 132, 132,
  132, 132, 395, 395, 395, 132, 395, 132, 132, 132, 132, 526,
  263, 395, 132, 263, 263, 263, 526, 1053, 132, 132, 132, 132,
  395, 395, 789, 132, 132, 132, 132, 526, 263, 395, 132, 263,
  132, 132, 132, 132, 526, 263, 395, 132, 526, 263, 526, 1053,
  526, 263
};


// ------------------------------------------------------------
// TETRIS
// ------------------------------------------------------------
const int tetrisN[] = {
  659, 494, 523, 587, 523, 494, 440, 440,
  523, 659, 587, 523, 494, 494, 523, 587,
  659, 523, 440, 440, 0,

  587, 698, 880, 784, 698, 659, 523, 659,
  587, 523, 494, 494, 523, 587, 659, 523,
  440, 440
};

const int tetrisT[] = {
  250, 125, 125, 250, 125, 125, 250, 125,
  125, 250, 125, 125, 375, 125, 125, 250,
  250, 250, 375, 250, 125,

  250, 125, 250, 125, 125, 375, 125, 250,
  125, 125, 375, 125, 125, 250, 250, 250,
  375, 375
};


// ------------------------------------------------------------
// STAR WARS - IMPERIAL MARCH
// ------------------------------------------------------------
const int imperialN[] = {
  392, 392, 392, 311, 466, 392, 311, 466, 392,

  587, 587, 587, 622, 466, 370, 311, 466, 392,

  784, 392, 392, 784, 740, 698, 659, 622, 659,

  0, 415, 554, 523, 494, 466, 440, 466
};

const int imperialT[] = {
  500, 500, 500, 350, 150, 500, 350, 150, 800,

  500, 500, 500, 350, 150, 500, 350, 150, 800,

  500, 300, 150, 500, 300, 150, 150, 150, 300,

  100, 300, 500, 300, 150, 150, 150, 300
};


// ------------------------------------------------------------
// PIRATES OF THE CARIBBEAN
// ------------------------------------------------------------
const int piratesN[] = {
  330, 392, 440, 440, 0, 440, 494, 523, 523, 0,
  523, 587, 494, 494, 0, 440, 392, 440, 0, 330,
  392, 440, 440, 0, 440, 494, 523, 523, 0, 523,
  587, 494, 494, 0, 440, 392, 440, 0, 330, 392,
  440, 440, 0, 440, 523, 587, 587, 0, 587, 659,
  698, 698, 0, 659, 587, 659, 440, 0, 440, 494,
  523, 523, 0, 587, 659, 440, 0, 440, 523, 494,
  494, 0, 523, 440, 494, 0, 440, 440, 440, 494,
  523, 523, 0, 523, 587, 494, 494, 0, 440, 392,
  440, 0, 330, 392, 440, 440, 0, 440, 494, 523,
  523, 0, 523, 587, 494, 494, 0, 440, 392, 440,
  0, 330, 392, 440, 440, 0, 440, 523, 587, 587,
  0, 587, 659, 698, 698, 0, 659, 587, 659, 440,
  0, 440, 494, 523, 523, 0, 587, 659, 440, 0,
  440, 523, 494, 494, 0, 523, 440, 494, 0
};

const int piratesT[] = {
  125, 125, 250, 125, 125, 125, 125, 250, 125, 125,
  125, 125, 250, 125, 125, 125, 125, 375, 125, 125,
  125, 250, 125, 125, 125, 125, 250, 125, 125, 125,
  125, 250, 125, 125, 125, 125, 375, 125, 125, 125,
  250, 125, 125, 125, 125, 250, 125, 125, 125, 125,
  250, 125, 125, 125, 125, 125, 250, 125, 125, 125,
  250, 125, 125, 250, 125, 250, 125, 125, 125, 250,
  125, 125, 125, 125, 375, 375, 250, 125, 125, 125,
  250, 125, 125, 125, 125, 250, 125, 125, 125, 125,
  375, 125, 125, 125, 250, 125, 125, 125, 125, 250,
  125, 125, 125, 125, 250, 125, 125, 125, 125, 375,
  125, 125, 125, 250, 125, 125, 125, 125, 250, 125,
  125, 125, 125, 250, 125, 125, 125, 125, 125, 250,
  125, 125, 125, 250, 125, 125, 250, 125, 250, 125,
  125, 125, 250, 125, 125, 125, 125, 375, 375
};


// ------------------------------------------------------------
// TAKE ON ME
// ------------------------------------------------------------
const int takeN[] = {
  740, 740, 587, 494, 0, 494, 0, 659,
  0, 659, 0, 659, 831, 831, 880, 988,
  880, 880, 880, 659, 0, 587, 0, 740,
  0, 740, 0, 740, 659, 659, 740, 659,

  740, 740, 587, 494, 0, 494, 0, 659,
  0, 659, 0, 659, 831, 831, 880, 988,
  880, 880, 880, 659, 0, 587, 0, 740,
  0, 740, 0, 740, 659, 659, 740, 659,

  740, 740, 587, 494, 0, 494, 0, 659,
  0, 659, 0, 659, 831, 831, 880, 988,
  880, 880, 880, 659, 0, 587, 0, 740,
  0, 740, 0, 740, 659, 659, 740, 659
};

const int takeT[] = {
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,

  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,

  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179,
  179, 179, 179, 179, 179, 179, 179, 179
};

// ------------------------------------------------------------
// NOKIA TUNE
// Tempo original: 180 BPM
// ------------------------------------------------------------

const int nokiaN[] = {
  659, 587, 370, 415,
  554, 494, 294, 330,
  494, 440, 277, 330,
  440
};

const int nokiaT[] = {
  139, 139, 278, 278,
  139, 139, 278, 278,
  139, 139, 278, 278,
  556
};

// ------------------------------------------------------------
// PINK PANTHER
// ------------------------------------------------------------
const int pinkN[] = {
  0, 0, 0, 311, 330, 0, 370, 392, 0, 311, 330,
  370, 392, 523, 494, 330, 392, 494, 466, 440, 392, 330,
  294, 330, 0, 0, 311, 330, 0, 370, 392, 0, 311,
  330, 370, 392, 523, 494, 392, 494, 659, 622, 587, 0,
  0, 311, 330, 0, 370, 392, 0, 311, 330, 370, 392,
  523, 494, 330, 392, 494, 466, 440, 392, 330, 294, 330,
  0, 0, 659, 587, 494, 440, 392, 330, 466, 440, 466,
  440, 466, 440, 466, 440, 392, 330, 294, 330, 330, 330
};

const int pinkT[] = {
  833, 417, 208, 208, 625, 208, 208, 625, 208, 208, 312,
  208, 312, 208, 312, 208, 312, 208, 833, 157, 157, 157,
  157, 833, 417, 208, 417, 625, 208, 208, 625, 208, 208,
  312, 208, 312, 208, 312, 208, 312, 208, 1667, 833, 417,
  208, 208, 625, 208, 208, 625, 208, 208, 312, 208, 312,
  208, 312, 208, 312, 208, 833, 157, 157, 157, 157, 625,
  417, 417, 312, 208, 312, 208, 312, 312, 104, 312, 104,
  312, 104, 312, 104, 312, 157, 157, 157, 104, 104, 833
};


// ------------------------------------------------------------
// MELODIA AUTORAL - MODO SECRETO
// ------------------------------------------------------------
const int paisN[] = {
  659, 784, 880, 784, 659, 587, 523,   0,
  587, 659, 784, 659, 523, 587, 659,   0,
  784, 880, 1047, 880, 784, 659, 587,  0,
  523, 659, 784, 1047, 988, 784, 880,  0,
  1047, 988, 880, 784, 880, 784, 659,  0,
  523, 659, 784, 1047, 1047, 784, 523, 0
};

const int paisT[] = {
  160, 160, 160, 240, 160, 160, 320, 120,
  160, 160, 160, 240, 160, 160, 320, 120,
  160, 160, 160, 240, 160, 160, 320, 120,
  160, 160, 160, 240, 200, 200, 360, 120,
  160, 160, 160, 240, 160, 160, 320, 120,
  200, 200, 200, 260, 400, 200, 500, 200
};


// ============================================================
//                       LISTA DE MUSICAS
// ============================================================

struct Musica {
  const char* nome;
  const int* n;
  const int* t;
  int qtd;
};

const Musica MUSICAS[] = {
  { "Mario",        marioN,    marioT,    (int)TAM(marioN)    },
  { "Megalovania",  megaN,     megaT,     (int)TAM(megaN)     },
  { "Harry P.",     hpN,       hpT,       (int)TAM(hpN)       },
  { "Never Gonna",  rickN,     rickT,     (int)TAM(rickN)     },

  { "Tetris",       tetrisN,   tetrisT,   (int)TAM(tetrisN)   },
  { "Imperial",     imperialN, imperialT, (int)TAM(imperialN) },
  { "Pirates",      piratesN,  piratesT,  (int)TAM(piratesN)  },
  { "Take On Me",   takeN,     takeT,     (int)TAM(takeN)     },
  { "Pink Panther", pinkN,     pinkT,     (int)TAM(pinkN)     },
  { "Nokia", nokiaN, nokiaT, (int)TAM(nokiaN) }
};

const int TOTAL_MUSICAS = (int)TAM(MUSICAS);

// ============================================================
//                        TELAS AUXILIARES
// ============================================================
void barraSegurando(unsigned long limite) {
  unsigned long h = btnSegurando();
  if (h < 80) return;
  if (h > limite) h = limite;
  int w = (int)((h * 124UL) / limite);
  display.drawRect(2, 58, 124, 5, SSD1306_WHITE);
  display.fillRect(2, 58, w, 5, SSD1306_WHITE);
}

// ============================================================
//                            MENU
// ============================================================
const char* ITENS[] = { "Pianinho", "Desenho", "Semaforo" };
const int TOTAL_ITENS = 3;

void telaMenu(int sel) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(31, 0);
  display.print("== MENU ==");

  for (int i = 0; i < TOTAL_ITENS; i++) {
    int y = 15 + i * 14;
    if (i == sel) {
      display.fillRoundRect(2, y - 3, 124, 13, 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(10, y);
    display.print(ITENS[i]);
  }
  display.setTextColor(SSD1306_WHITE);
  barraSegurando(T_LONGO);          // a barra para no "confirmar": o segredo continua escondido
  display.display();
}

// ============================================================
//                       MODO 1: PIANINHO
// ============================================================
void telaPiano(int sel, bool tocando) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(tocando ? "Tocando:" : "Selecionada:");
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print(MUSICAS[sel].nome);
  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print("clique=trocar");
  display.setCursor(0, 52);
  display.print("segure=tocar/voltar");
  if (!tocando) barraSegurando(T_SEGREDO);
  display.display();
}

void modoPianinho() {
  int sel = 0;
  bool redesenhar = true;
  bool tinhaBarra = false;

  while (true) {
    uint8_t ev = btnLer();

    if (ev == EV_CURTO) {
      sel = (sel + 1) % TOTAL_MUSICAS;
      redesenhar = true;
      digitalWrite(LED_C_VERDE, HIGH); delay(80); digitalWrite(LED_C_VERDE, LOW);
    } else if (ev == EV_LONGO) {
      telaPiano(sel, true);
      uint8_t r = tocarMusica(MUSICAS[sel].n, MUSICAS[sel].t, MUSICAS[sel].qtd);
      silencio();
      if (r == 2) return;
      redesenhar = true;
    } else if (ev == EV_SEGREDO) {
      return;
    }

    bool temBarra = (btnSegurando() > 80);
    if (redesenhar || temBarra || tinhaBarra) {
      telaPiano(sel, false);
      redesenhar = false;
      tinhaBarra = temBarra;
    }
    delay(2);
  }
}

// ============================================================
//                       MODO 2: DESENHO
// ============================================================
void desRosto(bool piscando) {
  display.drawRoundRect(20, 2, 88, 60, 8, SSD1306_WHITE);
  if (piscando) {
    display.fillRoundRect(40, 28, 18, 4, 2, SSD1306_WHITE);
    display.fillRoundRect(70, 28, 18, 4, 2, SSD1306_WHITE);
  } else {
    display.fillRoundRect(40, 20, 18, 20, 4, SSD1306_WHITE);
    display.fillRoundRect(70, 20, 18, 20, 4, SSD1306_WHITE);
  }
  display.fillRoundRect(57, 45, 15, 3, 1, SSD1306_WHITE);
}

void desCoracao(int fase) {
  int cx = 64, cy = 26, r = 14 + fase;
  display.fillCircle(cx - r / 2, cy, r / 2 + 2, SSD1306_WHITE);
  display.fillCircle(cx + r / 2, cy, r / 2 + 2, SSD1306_WHITE);
  display.fillTriangle(cx - r - 1, cy + 2, cx + r + 1, cy + 2, cx, cy + r + 10, SSD1306_WHITE);
}

void desCasa() {
  display.drawRect(34, 28, 60, 32, SSD1306_WHITE);
  display.drawTriangle(28, 28, 100, 28, 64, 4, SSD1306_WHITE);
  display.drawRect(56, 42, 16, 18, SSD1306_WHITE);   // porta
  display.drawRect(40, 34, 12, 12, SSD1306_WHITE);   // janela
  display.drawRect(78, 34, 12, 12, SSD1306_WHITE);
  display.fillCircle(69, 51, 1, SSD1306_WHITE);      // macaneta
}

void desSmiley(int fase) {
  int cx = 64, cy = 32;
  display.drawCircle(cx, cy, 28, SSD1306_WHITE);
  display.fillCircle(cx - 10, cy - 8, 3, SSD1306_WHITE);
  display.fillCircle(cx + 10, cy - 8, 3, SSD1306_WHITE);
  for (int a = 205; a <= 335; a += 4) {
    float rad = a * 3.14159265 / 180.0;
    display.drawPixel(cx + (int)(cos(rad) * 16), cy - (int)(sin(rad) * 16), SSD1306_WHITE);
  }
  if (fase) {  // brilhinhos
    display.drawPixel(cx - 24, cy - 22, SSD1306_WHITE);
    display.drawPixel(cx + 24, cy - 22, SSD1306_WHITE);
  }
}

void modoDesenho() {
  int qual = 0;
  const int TOTAL_DES = 4;
  unsigned long tAnim = millis();
  int fase = 0;

  while (true) {
    uint8_t ev = btnLer();
    if (ev == EV_CURTO) qual = (qual + 1) % TOTAL_DES;
    else if (ev == EV_LONGO || ev == EV_SEGREDO) return;

    if (millis() - tAnim > 400) { tAnim = millis(); fase = (fase + 1) % 8; }

    display.clearDisplay();
    switch (qual) {
      case 0: desRosto(fase == 0); break;            // pisca de vez em quando
      case 1: desCoracao((fase % 2) ? 2 : 0); break; // batendo
      case 2: desCasa(); break;
      case 3: desSmiley(fase % 2); break;
    }
    display.display();
    delay(30);
  }
}

// ============================================================
//                       MODO 3: SEMAFORO
// ============================================================
enum { S_CARRO_VERDE, S_CARRO_AMAR, S_PED_VERDE, S_PED_PISCA, S_TUDO_VERM };

#define T_MIN_VERDE   5000UL   // tempo minimo de verde para os carros
#define T_AMARELO     2500UL
#define T_PED_VERDE   6000UL   // travessia tranquila (bipe lento)
#define T_PED_PISCA   4000UL   // tempo acabando (bipe rapido + piscando)
#define T_TUDO_VERM   1000UL

void bipe(unsigned long periodo, unsigned long duracao, int freq) {
  if ((millis() % periodo) < duracao) BUZZER_TOM(freq);
  else                                BUZZER_TOM(0);
}

void telaSemaforo(uint8_t estado, unsigned long dt, bool pedido) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // poste dos carros
  display.drawRoundRect(6, 4, 22, 54, 4, SSD1306_WHITE);
  bool carroVerm  = (estado == S_PED_VERDE || estado == S_PED_PISCA || estado == S_TUDO_VERM);
  bool carroAmar  = (estado == S_CARRO_AMAR);
  bool carroVerde = (estado == S_CARRO_VERDE);
  if (carroVerm)  display.fillCircle(17, 15, 6, SSD1306_WHITE); else display.drawCircle(17, 15, 6, SSD1306_WHITE);
  if (carroAmar)  display.fillCircle(17, 31, 6, SSD1306_WHITE); else display.drawCircle(17, 31, 6, SSD1306_WHITE);
  if (carroVerde) display.fillCircle(17, 47, 6, SSD1306_WHITE); else display.drawCircle(17, 47, 6, SSD1306_WHITE);

  // poste do pedestre
  display.drawRoundRect(36, 12, 22, 38, 4, SSD1306_WHITE);
  bool pedVerde = (estado == S_PED_VERDE) ||
                  (estado == S_PED_PISCA && ((millis() / 200) % 2));
  if (!pedVerde) display.fillCircle(47, 23, 6, SSD1306_WHITE); else display.drawCircle(47, 23, 6, SSD1306_WHITE);
  if (pedVerde)  display.fillCircle(47, 39, 6, SSD1306_WHITE); else display.drawCircle(47, 39, 6, SSD1306_WHITE);

  // textos
  display.setCursor(66, 4);
  switch (estado) {
    case S_CARRO_VERDE: display.print("Carros"); display.setCursor(66, 14); display.print("passando"); break;
    case S_CARRO_AMAR:  display.print("Atencao"); break;
    case S_PED_VERDE:   display.print("Pedestre"); display.setCursor(66, 14); display.print("atravessa"); break;
    case S_PED_PISCA:   display.print("Acabando"); display.setCursor(66, 14); display.print("corra!"); break;
    case S_TUDO_VERM:   display.print("Aguarde"); break;
  }

  display.setCursor(66, 30);
  if (estado == S_CARRO_VERDE) {
    display.print(pedido ? "Pedido OK" : "Clique p/");
    if (!pedido) { display.setCursor(66, 40); display.print("atravessar"); }
  } else {
    unsigned long total = (estado == S_CARRO_AMAR) ? T_AMARELO :
                          (estado == S_PED_VERDE)  ? T_PED_VERDE :
                          (estado == S_PED_PISCA)  ? T_PED_PISCA : T_TUDO_VERM;
    long resta = (long)total - (long)dt;
    if (resta < 0) resta = 0;
    display.print("Tempo: ");
    display.print((resta / 1000) + 1);
    display.print("s");
  }

  display.setCursor(66, 54);
  display.print("segure=sair");
  display.display();
}

void modoSemaforo() {
  uint8_t estado = S_CARRO_VERDE;
  unsigned long tEstado = millis();
  unsigned long tDesenho = 0;
  bool pedido = false;
  ledsApaga();

  while (true) {
    uint8_t ev = btnLer();
    if (ev == EV_LONGO || ev == EV_SEGREDO) { silencio(); return; }
    if (ev == EV_CURTO) pedido = true;

    unsigned long dt = millis() - tEstado;

    switch (estado) {
      case S_CARRO_VERDE:
        digitalWrite(LED_C_VERM, LOW);
        digitalWrite(LED_C_AMAR, LOW);
        digitalWrite(LED_C_VERDE, HIGH);
        digitalWrite(LED_P_VERM, HIGH);
        digitalWrite(LED_P_VERDE, LOW);
        BUZZER_TOM(0);
        if (pedido && dt >= T_MIN_VERDE) { estado = S_CARRO_AMAR; tEstado = millis(); }
        break;

      case S_CARRO_AMAR:
        digitalWrite(LED_C_VERDE, LOW);
        digitalWrite(LED_C_AMAR, HIGH);
        if (dt >= T_AMARELO) { estado = S_PED_VERDE; tEstado = millis(); pedido = false; }
        break;

      case S_PED_VERDE:
        digitalWrite(LED_C_AMAR, LOW);
        digitalWrite(LED_C_VERM, HIGH);
        digitalWrite(LED_P_VERM, LOW);
        digitalWrite(LED_P_VERDE, HIGH);
        bipe(1000, 60, 1800);                       // bipe calmo
        if (dt >= T_PED_VERDE) { estado = S_PED_PISCA; tEstado = millis(); }
        break;

      case S_PED_PISCA:
        digitalWrite(LED_P_VERDE, ((millis() / 200) % 2) ? HIGH : LOW);
        bipe(250, 90, 2600);                        // tempo acabando
        if (dt >= T_PED_PISCA) { estado = S_TUDO_VERM; tEstado = millis(); BUZZER_TOM(0); }
        break;

      case S_TUDO_VERM:
        digitalWrite(LED_P_VERDE, LOW);
        digitalWrite(LED_P_VERM, HIGH);
        digitalWrite(LED_C_VERM, HIGH);
        BUZZER_TOM(0);
        if (dt >= T_TUDO_VERM) { estado = S_CARRO_VERDE; tEstado = millis(); }
        break;
    }

    if (millis() - tDesenho > 120) { tDesenho = millis(); telaSemaforo(estado, dt, pedido); }
    delay(2);
  }
}

// ============================================================
//                   MODO SECRETO: DIA DOS PAIS
// ============================================================
const char* PAGINAS[][3] = {
  { "FELIZ",      "DIA DOS",    "PAIS!"        },
  { "Obrigado",   "por tudo,",  "pai!"         },
  { "Voce e meu", "heroi de",   "todos os dias"},
  { "Com amor,",  "do seu",     "filho <3"     }
};
const int TOTAL_PAGINAS = 4;

void coracaoPequeno(int x, int y) {
  display.fillCircle(x - 2, y, 2, SSD1306_WHITE);
  display.fillCircle(x + 2, y, 2, SSD1306_WHITE);
  display.fillTriangle(x - 4, y + 1, x + 4, y + 1, x, y + 5, SSD1306_WHITE);
}

void telaPais(int pagina, int fase) {
  display.clearDisplay();
  display.drawRoundRect(0, 0, 128, 64, 6, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  for (int i = 0; i < 3; i++) {
    const char* txt = PAGINAS[pagina][i];
    int larg = strlen(txt) * 6;
    display.setCursor((128 - larg) / 2, 16 + i * 12);
    display.print(txt);
  }
  // coracoes subindo
  for (int i = 0; i < 4; i++) {
    int x = 12 + i * 34;
    int y = 56 - ((fase * 3 + i * 9) % 46);
    coracaoPequeno(x, y);
  }
  display.display();
}

void modoSegredo() {
  int pagina = 0, fase = 0;
  unsigned long tPag = millis(), tDes = 0;
  uint8_t r = 0;

  int total = (int)TAM(paisN);
  for (int i = 0; i < total; i++) {
    int f = paisN[i];
    if (f == 0) silencio();
    else { BUZZER_TOM(f); ledPorNota(f); }

    if (millis() - tPag > 2400) { tPag = millis(); pagina = (pagina + 1) % TOTAL_PAGINAS; }
    if (millis() - tDes > 120)  { tDes = millis(); fase++; telaPais(pagina, fase); }

    r = esperar(paisT[i]);
    if (r) break;
    silencio();
    r = esperar(paisT[i] / 6);
    if (r) break;
  }
  silencio();

  if (r != 2) {                       // musica terminou: deixa a mensagem na tela
    telaPais(0, fase);
    display.setCursor(24, 54);
    display.print("aperte p/ voltar");
    display.display();
    while (btnLer() == EV_NENHUM) delay(5);
  }
  silencio();
}

// ============================================================
//                       SETUP / LOOP
// ============================================================
void setup() {
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  for (uint8_t i = 0; i < 5; i++) { pinMode(LEDS[i], OUTPUT); digitalWrite(LEDS[i], LOW); }

  BUZZER_INIT();
  BUZZER_TOM(0);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // OLED nao respondeu: pisca o LED vermelho dos carros para avisar
    while (true) {
      digitalWrite(LED_C_VERM, HIGH); delay(200);
      digitalWrite(LED_C_VERM, LOW);  delay(200);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(31, 18);
  display.print("ESP32-C3");
  display.setCursor(43, 30);
  display.print("MINI");
  display.setCursor(25, 46);
  display.print("Iniciando...");
  display.display();
  delay(900);
}

int selMenu = 0;

void loop() {
  uint8_t ev = btnLer();
  static bool redesenhar = true;
  static bool tinhaBarra = false;

  if (ev == EV_CURTO) {
    selMenu = (selMenu + 1) % TOTAL_ITENS;
    redesenhar = true;
    digitalWrite(LED_C_VERDE, HIGH); delay(80); digitalWrite(LED_C_VERDE, LOW);
  } else if (ev == EV_LONGO) {
    if      (selMenu == 0) modoPianinho();
    else if (selMenu == 1) modoDesenho();
    else                   modoSemaforo();
    silencio();
    redesenhar = true;
  } else if (ev == EV_SEGREDO) {      // <<< segredo do menu
    modoSegredo();
    redesenhar = true;
  }

  bool temBarra = (btnSegurando() > 80);
  if (redesenhar || temBarra || tinhaBarra) {
    telaMenu(selMenu);
    redesenhar = false;
    tinhaBarra = temBarra;
  }
  delay(2);
}
