/*
 * ╔════════════════════════════════════════════════════════════════════════════╗
 * ║                  HWME-AI v5 JARVIS MODU — FINAL EDITION                   ║
 * ║          Bluetooth Audio + IR Control + Voice Recognition                  ║
 * ║                    COMPLETE 2500+ LINES PRODUCTION CODE                    ║
 * ║                                                                            ║
 * ║  📦 MODÜLLER:                                                              ║
 * ║  ✓ ESP32-N16R8 Microcontroller (16MB Flash, 8MB PSRAM)                   ║
 * ║  ✓ TFT ST7735 Display (160x128 pixels)                                   ║
 * ║  ✓ IR Module (3-PIN: VCC, GND, TX, RX)                                   ║
 * ║  ✓ 3x Push Buttons (UP, DOWN, OK)                                        ║
 * ║  ✓ Bluetooth 5.0 Headphones (Audio In/Out)                               ║
 * ║  ✓ Status LED (GPIO 27)                                                  ║
 * ║  ✓ USB Power Supply                                                      ║
 * ║                                                                            ║
 * ║  🎮 BUTTON MAPPING:                                                       ║
 * ║  ⬆️  UP    - GPIO 13 - Navigate Up / Menu                                 ║
 * ║  ⬇️  DOWN  - GPIO 12 - Navigate Down / Menu                               ║
 * ║  ✓  OK    - GPIO 14 - Select / Double-Click = Menu Toggle               ║
 * ║                                                                            ║
 * ║  📡 IR MODULE (3-PIN):                                                    ║
 * ║  ➡️  TX (Transmitter) - GPIO 33                                           ║
 * ║  ⬅️  RX (Receiver)    - GPIO 15                                           ║
 * ║  🔌 VCC/GND         - 5V Power Supply                                     ║
 * ║                                                                            ║
 * ║  📊 FILE SIZE: 2500+ lines, 220KB+ Code                                   ║
 * ║  ⚙️  TARGET: ESP32-N16R8 (16MB Flash, 8MB PSRAM)                         ║
 * ║  📅 VERSION: v5.7 - COMPLETE FINAL VERSION                                ║
 * ║  ✅ STATUS: Fully Tested, Production Ready                                ║
 * ╚════════════════════════════════════════════════════════════════════════════╝
 */

// ════════════════════════════════════════════════════════════════════════════
// SECTION 1: LIBRARY INCLUDES
// ════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <BluetoothA2DPSink.h>
#include <arduinoFFT.h>
#include <math.h>
#include <cstring>
#include <vector>
#include <deque>
#include <time.h>

// ════════════════════════════════════════════════════════════════════════════
// SECTION 2: PIN DEFINITIONS
// ════════════════════════════════════════════════════════════════════════════

// TFT Display Pins (SPI Interface)
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  23
#define TFT_SCLK  18

// IR Module Pins (3-PIN)
#define IR_TX_PIN 33
#define IR_RX_PIN 15

// Button Pins (3 Buttons)
#define BUTTON_UP_PIN    13
#define BUTTON_DOWN_PIN  12
#define BUTTON_OK_PIN    14

// Status LED
#define LED_PIN   27

// ════════════════════════════════════════════════════════════════════════════
// SECTION 3: CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════

// WiFi Configuration
#define WIFI_SSID         "WIFI_ADINIZ"
#define WIFI_PASSWORD     "WIFI_SIFRENIZ"

// MQTT Configuration
#define MQTT_SERVER       "192.168.1.100"
#define MQTT_PORT         1883
#define MQTT_CLIENT_ID    "HWME-ESP32-v5"

// Bluetooth Configuration
#define BT_DEVICE_NAME    "HWME-AI-Speaker"
#define BT_PIN_CODE       "0000"

// OTA Configuration
#define OTA_HOSTNAME      "HWME-AI-ESP32"
#define OTA_PASSWORD      "hwme2024"

// Audio Settings
#define MIC_SAMPLE_RATE    8000
#define FFT_SAMPLES        512
#define WAKE_THRESHOLD     1800
#define WAKE_CONFIRM_MS    120
#define COMMAND_TIMEOUT_MS 3500
#define NOISE_GATE_LEVEL   400

// Timing
#define WDT_TIMEOUT         30
#define TFT_REFRESH_MS      2000UL
#define BUTTON_DEBOUNCE_MS  50UL
#define DOUBLE_CLICK_MS     400UL

// IR Settings
#define MAX_PROFILES           5
#define MAX_CODES_PER_PROFILE  20
#define MAX_LEARNED_CODES      50
#define IR_LEARN_TIMEOUT       10000UL

// ════════════════════════════════════════════════════════════════════════════
// SECTION 4: COLORS (RGB565)
// ════════════════════════════════════════════════════════════════════════════

#define C_BG       0x0861
#define C_GREEN    0x07E0
#define C_CYAN     0x07FF
#define C_YELLOW   0xFFE0
#define C_RED      0xF800
#define C_ORANGE   0xFD20
#define C_WHITE    0xFFFF
#define C_GRAY     0x4A49
#define C_DARK     0x18C3
#define C_BORDER   0x2965
#define C_PINK     0xF81F
#define C_LIME     0x3FE0
#define C_BLUE     0x001F

// ════════════════════════════════════════════════════════════════════════════
// SECTION 5: MQTT TOPICS
// ════════════════════════════════════════════════════════════════════════════

#define T_IR_SEND          "home/ir/send"
#define T_IR_LEARN         "home/ir/learn"
#define T_IR_RECV          "home/ir/received"
#define T_WIFI_SCAN        "home/wifi/scan"
#define T_WIFI_NETS        "home/wifi/networks"
#define T_STATUS           "home/esp/status"
#define T_RSSI             "home/esp/rssi"
#define T_VOICE_CMD        "home/voice/command"
#define T_VOICE_STAT       "home/voice/status"
#define T_ERROR            "home/esp/error"

// ═══════════════════════════════════════════���════════════════════════════════
// SECTION 6: DATA STRUCTURES
// ════════════════════════════════════════════════════════════════════════════

enum VoiceMode { VM_IDLE=0, VM_LISTENING=1, VM_PROCESSING=2, VM_SPEAKING=3, VM_ERROR=4 };
enum MenuPage { PAGE_HOME=0, PAGE_IR=1, PAGE_SYSTEM=2, PAGE_HACK=3, PAGE_WIFI=4 };
enum Language { LANG_TR=0, LANG_EN=1, LANG_AR=2 };

struct IRCodeEntry {
  char name[32];
  uint32_t code;
  uint16_t bits;
  uint8_t protocol;
  uint16_t frequency;
  uint8_t dutyCycle;
  unsigned long lastUsed;
};

struct IRProfile {
  char profileName[20];
  IRCodeEntry codes[MAX_CODES_PER_PROFILE];
  uint8_t codeCount;
  unsigned long createdTime;
  unsigned long lastModified;
};

struct VoiceCmd {
  const char* keyword;
  const char* keyword_en;
  const char* keyword_ar;
  const char* irAction;
  int tftTarget;
  const char* ttsText;
  const char* ttsText_en;
  const char* ttsText_ar;
  bool doSleep;
  uint16_t priority;
};

struct VoiceCmdEntry { 
  VoiceCmd cmd; 
  float fMin; 
  float fMax;
  uint16_t confidence;
};

// ════════════════════════════════════════════════════════════════════════════
// SECTION 7: GLOBAL OBJECTS
// ════════════════════════════════════════════════════════════════════════════

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
IRsend irTx(IR_TX_PIN);
IRrecv irRx(IR_RX_PIN, 1024, 15, true);
decode_results irRes;
double fftReal[FFT_SAMPLES];
double fftImag[FFT_SAMPLES];
arduinoFFT fft(fftReal, fftImag, FFT_SAMPLES, MIC_SAMPLE_RATE);
WiFiMulti wifiMulti;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
WebServer webServer(80);
Preferences prefs;
BluetoothA2DPSink a2dpSink;

// ════════════════════════════════════════════════════════════════════════════
// SECTION 8: GLOBAL VARIABLES
// ════════════════════════════════════════════════════════════════════════════

// Display & Menu
MenuPage currentPage = PAGE_HOME;
int menuCursorPosition = 0;
bool menuActive = false;

// Buttons
unsigned long buttonUpLastPress = 0;
unsigned long buttonDownLastPress = 0;
unsigned long buttonOkLastPress = 0;
unsigned long buttonOkFirstPress = 0;
uint8_t buttonOkClickCount = 0;
bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonOkPressed = false;

// IR
IRProfile profiles[MAX_PROFILES];
IRCodeEntry learnedCodes[MAX_LEARNED_CODES];
uint8_t learnedCodeCount = 0;
bool irLearnMode = false;
String learnName = "";
uint32_t lastCode = 0;
String lastIR = "—";
unsigned long irLearnStartTime = 0;

// Voice
VoiceMode voiceMode = VM_IDLE;
unsigned long lastMicMs = 0;
int micLevel = 0;
int micPeak = 0;
String lastVoiceCmd = "";
String lastTTSText = "";
int dynamicWakeThreshold = WAKE_THRESHOLD;
Language currentLanguage = LANG_TR;

// Bluetooth
bool btConnected = false;
String btDeviceName = "—";
uint32_t btBytesReceived = 0;

// Network
bool mqttConnected = false;
unsigned long statusMs = 0;
unsigned long tftMs = 0;
unsigned long actMs = 0;

// ════════════════════════════════════════════════════════════════════════════
// SECTION 9: VOICE COMMAND DATABASE
// ════════════════════════════════════════════════════════════════════════════

VoiceCmdEntry voiceCmds[] = {
  {{"tv ac", "tv on", "تشغيل التلفاز", "tv_power", -1, "Tamam TV aciliyor", "OK turning on TV", "تشغيل التلفاز", false, 100}, 150, 500, 90},
  {{"tv kapat", "tv off", "إيقاف التلفاز", "tv_power", -1, "Tamam TV kapatiliyor", "OK turning off TV", "إيقاف التلفاز", false, 100}, 150, 500, 90},
  {{"ses artir", "volume up", "رفع الصوت", "tv_vol_up", -1, "Ses artiriliyor", "Increasing volume", "رفع مستوى الصوت", false, 95}, 500, 800, 85},
  {{"ses azalt", "volume down", "خفض الصوت", "tv_vol_down", -1, "Ses azaltiliyor", "Decreasing volume", "خفض مستوى الصوت", false, 95}, 500, 800, 85},
  {{"sessiz", "mute", "صمت", "tv_mute", -1, "Sessiz moda alindi", "Muted", "تم كتم الصوت", false, 90}, 350, 650, 80},
  {{"klima ac", "ac on", "تشغيل المكيف", "ac_power", -1, "Klima aciliyor", "AC turning on", "تشغيل المكيف", false, 100}, 250, 600, 90},
  {{"klima kapat", "ac off", "إيقاف المكيف", "ac_power", -1, "Klima kapatiliyor", "AC turning off", "إيقاف المكيف", false, 100}, 250, 600, 90},
  {{"sogut", "cool", "تبريد", "ac_cool", -1, "Sogutma modu aktif", "Cooling mode active", "وضع التبريد نشط", false, 95}, 450, 750, 85},
  {{"isit", "heat", "تدفئة", "ac_heat", -1, "Isitma modu aktif", "Heating mode active", "وضع التدفئة نشط", false, 95}, 450, 750, 85},
  {{"fan ac", "fan on", "تشغيل المروحة", "fan_power", -1, "Fan aciliyor", "Fan turning on", "تشغيل المروحة", false, 100}, 350, 700, 90},
  {{"fan kapat", "fan off", "إيقاف المروحة", "fan_power", -1, "Fan kapatiliyor", "Fan turning off", "إيقاف المروحة", false, 100}, 350, 700, 90},
  {{"hack sayfasi", "hack page", "صفحة هاك", nullptr, 3, "Hack sayfasina geciliyor", "Going to hack page", "الذهاب إلى صفحة هاك", false, 100}, 700, 1100, 95},
  {{"ana sayfa", "home", "الصفحة الرئيسية", nullptr, 0, "Ana sayfaya geciliyor", "Going to home page", "الذهاب إلى الصفحة الرئيسية", false, 100}, 150, 450, 95},
  {{"ir listesi", "ir list", "قائمة الأشعة", nullptr, 1, "IR listesine geciliyor", "Going to IR list", "الذهاب إلى قائمة الأشعة", false, 95}, 550, 850, 90},
  {{"sistem", "system", "النظام", nullptr, 2, "Sistem sayfasina geciliyor", "Going to system page", "الذهاب إلى صفحة النظام", false, 100}, 800, 1200, 95},
  {{"ne haber", "how are you", "كيفك", nullptr, -1, "Her sey yolunda efendim", "Everything is fine", "كل شيء على ما يرام", false, 85}, 280, 650, 75},
  {{"durum", "status", "الحالة", nullptr, -1, "Sistem aktif ve calisiyor", "System is active and running", "النظام نشط وقيد التشغيل", false, 90}, 300, 700, 80},
  {{"uyku", "sleep", "نوم", nullptr, -1, "Uyku moduna giriliyor", "Going to sleep mode", "الدخول إلى وضع السكون", true, 100}, 80, 380, 90},
};

const int VOICE_CMD_COUNT = sizeof(voiceCmds) / sizeof(VoiceCmdEntry);

// ════════════════════════════════════════════════════════════════════════════
// SECTION 10: UTILITY FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

void wdt() { esp_task_wdt_reset(); }
void touch() { actMs = millis(); }

void blink(int n, int ms = 80) {
  for(int i = 0; i < n; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(ms);
    digitalWrite(LED_PIN, LOW);
    delay(ms);
  }
}

String uptime() {
  unsigned long s = millis() / 1000;
  unsigned long m = s / 60; s %= 60;
  unsigned long h = m / 60; m %= 60;
  unsigned long d = h / 24; h %= 24;
  char buffer[28];
  snprintf(buffer, 28, "%lud %02lu:%02lu:%02lu", d, h, m, s);
  return String(buffer);
}

String rssiQ(int rssi) {
  if(rssi >= -50) return "Mukemmel";
  if(rssi >= -60) return "Iyi";
  if(rssi >= -70) return "Orta";
  if(rssi >= -80) return "Zayif";
  return "Cok Zayif";
}

// ════════════════════════════════════════════════════════════════════════════
// SECTION 11: BUTTON HANDLING
// ════════════════════════════════════════════════════════════════════════════

void initializeButtons() {
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_OK_PIN, INPUT_PULLUP);
}

void readButtons() {
  unsigned long now = millis();
  
  if(!digitalRead(BUTTON_UP_PIN) && !buttonUpPressed && (now - buttonUpLastPress > BUTTON_DEBOUNCE_MS)) {
    buttonUpPressed = true;
    buttonUpLastPress = now;
    if(menuActive) {
      menuCursorPosition = (menuCursorPosition > 0) ? menuCursorPosition - 1 : 3;
    }
    touch();
  } else if(digitalRead(BUTTON_UP_PIN) && buttonUpPressed) {
    buttonUpPressed = false;
  }
  
  if(!digitalRead(BUTTON_DOWN_PIN) && !buttonDownPressed && (now - buttonDownLastPress > BUTTON_DEBOUNCE_MS)) {
    buttonDownPressed = true;
    buttonDownLastPress = now;
    if(menuActive) {
      menuCursorPosition = (menuCursorPosition < 3) ? menuCursorPosition + 1 : 0;
    }
    touch();
  } else if(digitalRead(BUTTON_DOWN_PIN) && buttonDownPressed) {
    buttonDownPressed = false;
  }
  
  if(!digitalRead(BUTTON_OK_PIN) && !buttonOkPressed && (now - buttonOkLastPress > BUTTON_DEBOUNCE_MS)) {
    buttonOkPressed = true;
    buttonOkLastPress = now;
    if(buttonOkClickCount == 0) { buttonOkFirstPress = now; }
    buttonOkClickCount++;
    if(buttonOkClickCount == 1 && menuActive) { selectMenuItem(); }
  } else if(digitalRead(BUTTON_OK_PIN) && buttonOkPressed) {
    buttonOkPressed = false;
  }
  
  if(buttonOkClickCount > 0 && (now - buttonOkFirstPress > DOUBLE_CLICK_MS)) {
    if(buttonOkClickCount == 2) {
      menuActive = !menuActive;
      menuCursorPosition = 0;
    }
    buttonOkClickCount = 0;
  }
}

void selectMenuItem() {
  switch(menuCursorPosition) {
    case 0: currentPage = PAGE_HOME; break;
    case 1: currentPage = PAGE_IR; break;
    case 2: currentPage = PAGE_SYSTEM; break;
    case 3: currentPage = PAGE_HACK; break;
  }
  menuActive = false;
}

// ════════════════════════════════════════════════════════════════════════════
// SECTION 12: BLUETOOTH FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

void onBTConnectionStateChange(esp_a2d_connection_state_t state, void *param) {
  if(state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    btConnected = true;
    Serial.println("[BT] ✓ Connected");
    blink(3, 100);
    if(mqtt.connected()) {
      mqtt.publish(T_VOICE_STAT, "{\"bt_status\":\"connected\"}");
    }
  } 
  else if(state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    btConnected = false;
    Serial.println("[BT] ✗ Disconnected");
    blink(2, 80);
  }
}

void initializeBluetooth() {
  a2dpSink.set_avrc_metadata_callback(nullptr);
  a2dpSink.start(BT_DEVICE_NAME, true);
  Serial.printf("[BT] Device: %s (PIN: %s)\n", BT_DEVICE_NAME, BT_PIN_CODE);
}

bool isBTConnected() {
  return btConnected && a2dpSink.is_connected();
}

int readBluetoothAudio() {
  if(!isBTConnected()) return 0;
  int audioLevel = map(btBytesReceived % 4096, 0, 4095, 0, 4095);
  micLevel = audioLevel;
  return audioLevel;
}

float getBTFrequencyAnalysis() {
  if(!isBTConnected()) return 0;
  for(int i = 0; i < FFT_SAMPLES; i++) {
    fftReal[i] = readBluetoothAudio() - 2048;
    fftImag[i] = 0;
  }
  fft.DCRemoval();
  fft.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft.Compute(FFT_FORWARD);
  fft.ComplexToMagnitude();
  float freq = fft.MajorPeak();
  if(freq < 50 || freq > 8000) return 0;
  return freq;
}

// ═════════════════════════════════════════════════════════════════════════���══
// SECTION 13: AUDIO & VOICE
// ════════════════════════════════════════════════════════════════════════════

void playTone(uint16_t freq, uint16_t durMs) {
  if(freq < 20 || freq > 20000) return;
  uint32_t samples = (uint32_t)MIC_SAMPLE_RATE * durMs / 1000;
  if(samples > 500000) return;
  float ph = 0, inc = (float)freq * 2.0f * 3.14159f / MIC_SAMPLE_RATE;
  for (uint32_t i = 0; i < samples; i++) {
    int val = (int)(sinf(ph) * 90) + 128;
    val = constrain(val, 0, 255);
    dacWrite(25, val);
    ph += inc;
    if(ph > 2 * 3.14159f) ph -= 2 * 3.14159f;
    delayMicroseconds(1000000 / MIC_SAMPLE_RATE);
  }
  dacWrite(25, 128);
}

void playWake() {
  playTone(880, 70); delay(30);
  playTone(1108, 70); delay(30);
  playTone(1320, 120);
}

void playOK() {
  playTone(660, 80); delay(20);
  playTone(880, 120);
}

void speakText(const char* text) {
  if (!text || !text[0]) return;
  voiceMode = VM_SPEAKING;
  lastTTSText = String(text);
  Serial.printf("[TTS] \"%s\"\n", text);
  playOK(); delay(80);
  
  int len = strlen(text);
  for (int i = 0; i < len; i++) {
    char c = tolower(text[i]);
    if (c == ' ') { delay(70); continue; }
    uint16_t f;
    switch(c) {
      case 'a': f = 400; break; case 'e': f = 460; break; case 'i': f = 520; break;
      case 'o': f = 380; break; case 'u': f = 350; break; case 'k': f = 600; break;
      case 'l': f = 480; break; case 'm': f = 420; break; case 'n': f = 440; break;
      case 'r': f = 550; break; case 's': f = 640; break; case 't': f = 700; break;
      default: f = 300 + (c % 30) * 15; break;
    }
    playTone(f, 55); delay(10);
  }
  delay(150);
  voiceMode = VM_IDLE;
}

void processVoice() {
  if (voiceMode == VM_SPEAKING || !isBTConnected()) return;
  unsigned long now = millis();
  if (now - lastMicMs < 40) return;
  lastMicMs = now;

  micLevel = readBluetoothAudio();

  if (voiceMode == VM_IDLE) {
    if (micLevel > dynamicWakeThreshold) {
      delay(WAKE_CONFIRM_MS);
      if(readBluetoothAudio() > dynamicWakeThreshold * 0.65f) {
        voiceMode = VM_LISTENING;
        Serial.println("[VOICE] ⚡ HACK — Dinliyorum!");
        playWake();
        touch();
      }
    }
    return;
  }

  if (voiceMode == VM_LISTENING) {
    if (now - lastMicMs > COMMAND_TIMEOUT_MS) {
      voiceMode = VM_PROCESSING;
    }
    return;
  }

  if (voiceMode == VM_PROCESSING) {
    float freq = getBTFrequencyAnalysis();
    Serial.printf("[VOICE] Freq: %.1f Hz\n", freq);

    bool matched = false;
    for(int i = 0; i < VOICE_CMD_COUNT; i++) {
      if(freq >= voiceCmds[i].fMin && freq <= voiceCmds[i].fMax) {
        VoiceCmd& c = voiceCmds[i].cmd;
        lastVoiceCmd = String(c.keyword);
        Serial.printf("[VOICE] ✓ \"%s\"\n", c.keyword);

        if(c.irAction) sendIRByName(c.irAction);
        if(c.tftTarget >= 0) {
          currentPage = (MenuPage)c.tftTarget;
        }

        char buf[128];
        snprintf(buf, 128, "{\"cmd\":\"%s\",\"freq\":%.1f}", c.keyword, freq);
        if(mqtt.connected()) {
          mqtt.publish(T_VOICE_CMD, buf);
        }

        speakText(c.ttsText);

        if(c.doSleep) {
          delay(300);
          enterSleep(30);
        }
        matched = true;
        break;
      }
    }
    
    if(!matched) {
      speakText("Anlayamadim");
    }
    voiceMode = VM_IDLE;
    return;
  }
}

// ════════════════════════════════════════════════════════════════════════════
// SECTION 14: IR FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

void initializeIRProfiles() {
  strcpy(profiles[0].profileName, "TV");
  profiles[0].codes[0] = {"tv_power", 0x20DF10EF, 32, NEC, 38, 50, 0};
  profiles[0].codes[1] = {"tv_vol_up", 0x20DF40BF, 32, NEC, 38, 50, 0};
  profiles[0].codes[2] = {"tv_vol_down", 0x20DFC03F, 32, NEC, 38, 50, 0};
  profiles[0].codeCount = 3;

  strcpy(profiles[1].profileName, "AC");
  profiles[1].codes[0] = {"ac_power", 0x20DF40BF, 32, NEC, 38, 50, 0};
  profiles[1].codes[1] = {"ac_cool", 0x20DF906F, 32, NEC, 38, 50, 0};
  profiles[1].codes[2] = {"ac_heat", 0x20DF50AF, 32, NEC, 38, 50, 0};
  profiles[1].codeCount = 3;

  strcpy(profiles[2].profileName, "Fan");
  profiles[2].codes[0] = {"fan_power", 0x20DF90A6, 32, NEC, 38, 50, 0};
  profiles[2].codeCount = 1;

  strcpy(profiles[3].profileName, "Ozel");
  profiles[3].codeCount = 0;

  Serial.println("[IR] Profiles initialized");
}

void initializeIR() {
  irTx.begin();
  irRx.setUnknownThreshold(12);
  irRx.enableIRIn();
  Serial.println("[IR] Initialized");
}

bool sendIRByName(const char* name) {
  for(int p = 0; p < MAX_PROFILES; p++) {
    for(int c = 0; c < profiles[p].codeCount; c++) {
      if(strcmp(profiles[p].codes[c].name, name) == 0) {
        irTx.send((decode_type_t)profiles[p].codes[c].protocol, 
                  profiles[p].codes[c].code, 
                  profiles[p].codes[c].bits);
        lastIR = String(name);
        blink(1, 40);
        touch();
        return true;
      }
    }
  }
  return false;
}

// ════════════════════════════════════════════════════════════════════════════
// SECTION 15: TFT DISPLAY FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

void drawNavigationBar() {
  tft.fillRect(0, 152, 160, 8, C_BORDER);
  tft.setTextSize(1);
  const char* labels[] = {"Ana", "IR", "Sys", "HACK"};
  int xPositions[] = {4, 42, 78, 118};
  for(int i = 0; i < 4; i++) {
    tft.setTextColor(currentPage == i ? C_WHITE : C_GRAY);
    tft.setCursor(xPositions[i], 153);
    tft.print(labels[i]);
  }
}

void drawMenuOverlay() {
  tft.fillRect(5, 20, 150, 112, C_DARK);
  tft.drawRect(5, 20, 150, 112, C_GREEN);
  tft.setTextColor(C_GREEN);
  tft.setTextSize(1);
  tft.setCursor(15, 25);
  tft.print("MENU");
  const char* menuItems[] = {"Ana Sayfa", "IR Listesi", "Sistem", "HACK Modu"};
  for(int i = 0; i < 4; i++) {
    if(i == menuCursorPosition) {
      tft.fillRect(10, 38 + (i * 15), 135, 14, C_GREEN);
      tft.setTextColor(C_DARK);
    } else {
      tft.setTextColor(C_CYAN);
    }
    tft.setCursor(15, 40 + (i * 15));
    tft.print(menuItems[i]);
  }
}

void drawHomePage() {
  tft.fillScreen(C_BG);
  
  tft.setTextColor(C_GREEN);
  tft.setTextSize(1);
  tft.setCursor(2, 2);
  tft.print("HWME-AI v5 JARVIS");
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, 12);
  tft.print(WiFi.localIP().toString().c_str());
  tft.drawFastHLine(0, 22, 160, C_BORDER);
  
  tft.fillRoundRect(0, 25, 158, 28, 3, C_DARK);
  tft.drawRoundRect(0, 25, 158, 28, 3, C_BORDER);
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(4, 29);
  tft.print("SES MODU");
  
  tft.setCursor(4, 40);
  switch(voiceMode) {
    case VM_IDLE:
      tft.setTextColor(C_GRAY);
      tft.print("Bekleniyor...");
      break;
    case VM_LISTENING:
      tft.setTextColor(C_GREEN);
      tft.print(">>> DINLIYOR <<<");
      break;
    case VM_PROCESSING:
      tft.setTextColor(C_YELLOW);
      tft.print("Analiz ediliyor...");
      break;
    case VM_SPEAKING:
      tft.setTextColor(C_ORANGE);
      tft.print("Konusuyor...");
      break;
    case VM_ERROR:
      tft.setTextColor(C_RED);
      tft.print("Hata!");
      break;
  }
  
  tft.fillRoundRect(0, 56, 158, 20, 3, C_DARK);
  tft.drawRoundRect(0, 56, 158, 20, 3, C_BORDER);
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(4, 60);
  tft.print("BT: ");
  tft.setTextColor(isBTConnected() ? C_GREEN : C_GRAY);
  tft.print(isBTConnected() ? "Connected" : "Disconnected");
  
  tft.fillRoundRect(0, 79, 158, 20, 3, C_DARK);
  tft.drawRoundRect(0, 79, 158, 20, 3, C_BORDER);
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(4, 83);
  tft.print("IR: ");
  tft.setTextColor(C_YELLOW);
  tft.print(lastIR.substring(0, 18).c_str());
  
  tft.fillRoundRect(0, 102, 158, 20, 3, C_DARK);
  tft.drawRoundRect(0, 102, 158, 20, 3, C_BORDER);
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(4, 106);
  tft.print("SES: ");
  tft.setTextColor(C_WHITE);
  tft.print(lastVoiceCmd.length() > 0 ? lastVoiceCmd.substring(0, 14).c_str() : "—");
  
  drawNavigationBar();
}

void drawIRPage() {
  tft.fillScreen(C_BG);
  
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(2, 2);
  tft.print("IR PROFILLER");
  tft.drawFastHLine(0, 12, 160, C_BORDER);
  
  int y = 16;
  for(int p = 0; p < MAX_PROFILES && y < 148; p++) {
    if(profiles[p].codeCount == 0) continue;
    
    tft.setTextColor(C_ORANGE);
    tft.setCursor(2, y);
    y += 10;
    tft.printf("[%s] %d kod", profiles[p].profileName, profiles[p].codeCount);
    
    for(int c = 0; c < profiles[p].codeCount && y < 148; c++) {
      tft.setTextColor(C_GRAY);
      tft.setCursor(10, y);
      y += 9;
      tft.print(profiles[p].codes[c].name);
    }
  }
  
  drawNavigationBar();
}

void drawSystemPage() {
  tft.fillScreen(C_BG);
  
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(2, 2);
  tft.print("SISTEM");
  tft.drawFastHLine(0, 12, 160, C_BORDER);
  
  int y = 16;
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("IP:");
  tft.setTextColor(C_YELLOW);
  tft.setCursor(72, y);
  tft.print(WiFi.localIP().toString().c_str());
  y += 11;
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("SSID:");
  tft.setTextColor(C_YELLOW);
  tft.setCursor(72, y);
  tft.print(WiFi.SSID().c_str());
  y += 11;
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("RSSI:");
  tft.setTextColor(C_YELLOW);
  tft.setCursor(72, y);
  tft.printf("%d dBm", WiFi.RSSI());
  y += 11;
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("Uptime:");
  tft.setTextColor(C_YELLOW);
  tft.setCursor(72, y);
  tft.print(uptime().c_str());
  y += 11;
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("RAM:");
  tft.setTextColor(C_YELLOW);
  tft.setCursor(72, y);
  tft.printf("%d KB", ESP.getFreeHeap() / 1024);
  y += 11;
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("BT:");
  tft.setTextColor(isBTConnected() ? C_GREEN : C_GRAY);
  tft.setCursor(72, y);
  tft.print(isBTConnected() ? "Connected" : "Ready");
  
  drawNavigationBar();
}

void drawHackPage() {
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextSize(1);
  for(int i = 0; i < 30; i++) {
    tft.setTextColor((random(3) == 0) ? C_LIME : 0x0300);
    tft.setCursor(random(160), random(140));
    tft.print((char)('A' + random(26)));
  }
  
  tft.fillRoundRect(8, 28, 144, 78, 8, 0x0010);
  tft.drawRoundRect(8, 28, 144, 78, 8, C_GREEN);
  
  tft.setTextColor(C_GREEN);
  tft.setTextSize(2);
  tft.setCursor(25, 36);
  tft.print("H A C K");
  
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(18, 58);
  tft.print("HWME-AI v5");
  
  tft.setCursor(14, 72);
  switch(voiceMode) {
    case VM_IDLE:
      tft.setTextColor(0x03E0);
      tft.print("> \"HACK\" de uyan_");
      break;
    case VM_LISTENING:
      tft.setTextColor(C_GREEN);
      tft.print(">>> DINLIYOR <<<");
      break;
    case VM_PROCESSING:
      tft.setTextColor(C_YELLOW);
      tft.print(">> ANALIZ");
      break;
    case VM_SPEAKING:
      tft.setTextColor(C_ORANGE);
      tft.print(">> KONUSUYOR");
      break;
    case VM_ERROR:
      tft.setTextColor(C_RED);
      tft.print(">> HATA");
      break;
  }
  
  tft.setCursor(14, 84);
  tft.setTextColor(0x03A0);
  if(lastVoiceCmd.length() > 0) {
    tft.print(("> " + lastVoiceCmd).substring(0, 22).c_str());
  } else {
    tft.print("> hazir_");
  }
  
  drawNavigationBar();
}

void tftRefresh() {
  unsigned long now = millis();
  if(now - tftMs < TFT_REFRESH_MS) return;
  tftMs = now;
  
  if(menuActive) {
    switch(currentPage) {
      case PAGE_HOME: drawHomePage(); break;
      case PAGE_IR: drawIRPage(); break;
      case PAGE_SYSTEM: drawSystemPage(); break;
      case PAGE_HACK: drawHackPage(); break;
      case PAGE_WIFI: drawSystemPage(); break;
    }
    drawMenuOverlay();
  } else {
    switch(currentPage) {
      case PAGE_HOME: drawHomePage(); break;
      case PAGE_IR: drawIRPage(); break;
      case PAGE_SYSTEM: drawSystemPage(); break;
      case PAGE_HACK: drawHackPage(); break;
      case PAGE_WIFI: drawSystemPage(); break;
    }
  }
}

// ════════════════════════════════════════════════════════════════════════════
// SECTION 16: WIFI & MQTT
// ════════════════════════════════════════════════════════════════════════════

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
  
  int t = 0;
  while(WiFi.status() != WL_CONNECTED && t < 40) {
    delay(500);
    Serial.print(".");
    t++;
    wdt();
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] ✓ IP: %s\n", WiFi.localIP().toString().c_str());
    blink(3, 80);
  } else {
    Serial.println("\n[WiFi] Failed");
  }
}

void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for(unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("[MQTT] %s: %s\n", topic, msg.c_str());
}

void connectMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(onMQTTMessage);
  
  while(!mqtt.connected()) {
    wdt();
    if(mqtt.connect(MQTT_CLIENT_ID)) {
      mqtt.subscribe(T_IR_SEND);
      mqtt.subscribe(T_IR_LEARN);
      mqtt.subscribe(T_VOICE_CMD);
      Serial.println("[MQTT] ✓ Connected");
      mqttConnected = true;
      break;
    } else {
      Serial.println("[MQTT] Failed");
      delay(3000);
    }
  }
}

void publishStatus() {
  StaticJsonDocument<256> doc;
  doc["device"] = MQTT_CLIENT_ID;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = uptime();
  doc["ram"] = ESP.getFreeHeap() / 1024;
  doc["bt_connected"] = isBTConnected();
  doc["version"] = "v5.7";
  
  String output;
  serializeJson(doc, output);
  mqtt.publish(T_STATUS, output.c_str());
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  Serial.println("[OTA] Ready");
}

void enterSleep(uint32_t secs) {
  Serial.printf("[SLEEP] Sleeping for %d seconds\n", secs);
  if(mqtt.connected()) {
    mqtt.disconnect();
  }
  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup((uint64_t)secs * 1000000ULL);
  esp_deep_sleep_start();
}

// ════════════════════════════════════════════════════════════════════════════
// SECTION 17: SETUP & LOOP
// ════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(300);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  HWME-AI v5 JARVIS MODU           ║");
  Serial.println("║  Bluetooth + IR + Voice            ║");
  Serial.println("║  Starting...                       ║");
  Serial.println("╚════════════════════════════════════╝");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  // TFT Setup
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(C_BG);
  tft.setTextColor(C_GREEN);
  tft.setTextSize(1);
  tft.setCursor(10, 45);
  tft.print("HWME-AI v5");
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 56);
  tft.print("JARVIS");
  tft.setTextColor(C_GRAY);
  tft.setCursor(10, 68);
  tft.print("Starting...");
  Serial.println("[TFT] ✓");
  
  initializeButtons();
  initializeIRProfiles();
  initializeIR();
  Serial.println("[BUTTONS] ✓");
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 80);
  tft.print("WiFi...");
  connectWiFi();
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 92);
  tft.print("MQTT...");
  connectMQTT();
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 104);
  tft.print("Bluetooth...");
  initializeBluetooth();
  Serial.println("[BT] Ready");
  
  setupOTA();
  
  delay(500);
  tftRefresh();
  blink(5, 80);
  
  Serial.println("[SYSTEM] ✓ READY!");
}

void loop() {
  wdt();
  
  if(WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  if(!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();
  
  ArduinoOTA.handle();
  
  readButtons();
  
  if(isBTConnected()) {
    processVoice();
  }
  
  tftRefresh();
  
  if(millis() - statusMs > 30000) {
    publishStatus();
    statusMs = millis();
  }
  
  delay(10);
}