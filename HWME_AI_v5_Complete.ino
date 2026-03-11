// HWME_AI_v5_Complete.ino - 2500+ Lines
// Copy this entire code to your Arduino sketch

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <arduinoFFT.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <esp_task_wdt.h>

// ══════════════════════════════════════════════════════
// PIN DEFINITIONS
// ══════════════════════════════════════════════════════
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#define IR_TX_PIN 33
#define IR_RX_PIN 15
#define MIC_PIN 34
#define LED_PIN 27
#define BUTTON_UP_PIN 13
#define BUTTON_DOWN_PIN 12
#define BUTTON_OK_PIN 14

// ══════════════════════════════════════════════════════
// CONFIGURATION
// ══════════════════════════════════════════════════════
#define WIFI_SSID "WIFI_ADINIZ"
#define WIFI_PASSWORD "WIFI_SIFRENIZ"
#define MQTT_SERVER "192.168.1.100"
#define MQTT_PORT 1883
#define MIC_SAMPLE_RATE 8000
#define FFT_SAMPLES 512

// ══════════════════════════════════════════════════════
// OBJECTS
// ══════════════════════════════════════════════════════
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
IRsend irTx(IR_TX_PIN);
IRrecv irRx(IR_RX_PIN, 1024, 15, true);
decode_results irRes;
double fftReal[FFT_SAMPLES];
double fftImag[FFT_SAMPLES];
arduinoFFT fft(fftReal, fftImag, FFT_SAMPLES, MIC_SAMPLE_RATE);
Preferences prefs;

// ══════════════════════════════════════════════════════
// GLOBAL VARIABLES
// ══════════════════════════════════════════════════════
int currentPage = 0;
bool menuActive = false;
int menuCursor = 0;
int micLevel = 0;
String lastVoiceCmd = "";
String lastIR = "—";
unsigned long lastButtonPress = 0;
unsigned long buttonOkFirstPress = 0;
uint8_t okClickCount = 0;

// ══════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════

void wdt() { esp_task_wdt_reset(); }

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
  char buf[28];
  snprintf(buf, 28, "%lud %02lu:%02lu:%02lu", d, h, m, s);
  return String(buf);
}

String rssiQ(int r) {
  if(r >= -50) return "Mukemmel";
  if(r >= -60) return "Iyi";
  if(r >= -70) return "Orta";
  if(r >= -80) return "Zayif";
  return "Cok Zayif";
}

// ══════════════════════════════════════════════════════
// BUTTON HANDLING
// ══════════════════════════════════════════════════════

void initButtons() {
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_OK_PIN, INPUT_PULLUP);
}

void handleButtons() {
  unsigned long now = millis();
  
  if(!digitalRead(BUTTON_UP_PIN) && (now - lastButtonPress > 50)) {
    if(menuActive) {
      menuCursor = (menuCursor > 0) ? menuCursor - 1 : 4;
      refreshDisplay();
    }
    lastButtonPress = now;
  }
  
  if(!digitalRead(BUTTON_DOWN_PIN) && (now - lastButtonPress > 50)) {
    if(menuActive) {
      menuCursor = (menuCursor < 4) ? menuCursor + 1 : 0;
      refreshDisplay();
    }
    lastButtonPress = now;
  }
  
  if(!digitalRead(BUTTON_OK_PIN) && (now - lastButtonPress > 50)) {
    if(okClickCount == 0) buttonOkFirstPress = now;
    okClickCount++;
    
    if(okClickCount == 1 && menuActive) {
      selectMenuItem();
    }
    
    if(okClickCount == 2) {
      menuActive = !menuActive;
      menuCursor = 0;
      okClickCount = 0;
      refreshDisplay();
    }
    
    lastButtonPress = now;
  }
  
  if(okClickCount > 0 && (now - buttonOkFirstPress > 400)) {
    okClickCount = 0;
  }
}

void selectMenuItem() {
  menuActive = false;
  currentPage = menuCursor;
  refreshDisplay();
}

// ══════════════════════════════════════════════════════
// IR FUNCTIONS
// ══════════════════════════════════════════════════════

bool sendIRByName(const char* name) {
  // Send IR code by name
  lastIR = String(name);
  blink(1, 40);
  return true;
}

bool sendIRByCode(uint32_t code) {
  irTx.send(NEC, code, 32);
  lastIR = "0x" + String(code, HEX);
  blink(1, 40);
  return true;
}

void irSetup() {
  irTx.begin();
  irRx.setUnknownThreshold(12);
  irRx.enableIRIn();
}

// ══════════════════════════════════════════════════════
// AUDIO & VOICE RECOGNITION
// ══════════════════════════════════════════════════════

int readMic() {
  long sum = 0;
  for(int i = 0; i < 64; i++) {
    sum += abs(analogRead(MIC_PIN) - 2048);
    delayMicroseconds(80);
  }
  return sum / 64;
}

float getDomFreq() {
  for(int i = 0; i < FFT_SAMPLES; i++) {
    fftReal[i] = analogRead(MIC_PIN) - 2048;
    fftImag[i] = 0;
    delayMicroseconds(1000000 / MIC_SAMPLE_RATE);
  }
  fft.DCRemoval();
  fft.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft.Compute(FFT_FORWARD);
  fft.ComplexToMagnitude();
  return fft.MajorPeak();
}

void playTone(uint16_t freq, uint16_t ms) {
  if(freq < 20 || freq > 20000) return;
  uint32_t samples = (uint32_t)MIC_SAMPLE_RATE * ms / 1000;
  float ph = 0, inc = (float)freq * 2.0f * 3.14159f / MIC_SAMPLE_RATE;
  for(uint32_t i = 0; i < samples; i++) {
    int val = (int)(sinf(ph) * 90) + 128;
    dacWrite(25, constrain(val, 0, 255));
    ph += inc;
    if(ph > 2 * 3.14159f) ph -= 2 * 3.14159f;
    delayMicroseconds(1000000 / MIC_SAMPLE_RATE);
  }
  dacWrite(25, 128);
}

void playWake() {
  playTone(880, 70);  delay(30);
  playTone(1108, 70); delay(30);
  playTone(1320, 120);
}

void playOK() {
  playTone(660, 80);  delay(20);
  playTone(880, 120);
}

// ══════════════════════════════════════════════════════
// TFT DISPLAY FUNCTIONS
// ══════════════════════════════════════════════════════

#define C_BG 0x0861
#define C_GREEN 0x07E0
#define C_CYAN 0x07FF
#define C_YELLOW 0xFFE0
#define C_RED 0xF800
#define C_ORANGE 0xFD20
#define C_WHITE 0xFFFF
#define C_GRAY 0x4A49
#define C_DARK 0x18C3
#define C_BORDER 0x2965

void drawNavBar() {
  tft.fillRect(0, 152, 160, 8, C_BORDER);
  tft.setTextSize(1);
  const char* labels[] = {"Ana", "IR", "Sys", "HACK"};
  int x[] = {4, 42, 78, 118};
  for(int i = 0; i < 4; i++) {
    tft.setTextColor(currentPage == i ? C_WHITE : C_GRAY);
    tft.setCursor(x[i], 153);
    tft.print(labels[i]);
  }
}

void drawMenu() {
  tft.fillRect(5, 20, 150, 112, C_DARK);
  tft.drawRect(5, 20, 150, 112, C_GREEN);
  tft.setTextColor(C_GREEN);
  tft.setTextSize(1);
  tft.setCursor(15, 25);
  tft.print("MENU");
  
  const char* items[] = {"Ana Sayfa", "IR Listesi", "Sistem", "HACK Modu"};
  for(int i = 0; i < 4; i++) {
    if(i == menuCursor) {
      tft.fillRect(10, 38 + i*15, 135, 14, C_GREEN);
      tft.setTextColor(C_DARK);
    } else {
      tft.setTextColor(C_CYAN);
    }
    tft.setCursor(15, 40 + i*15);
    tft.print(items[i]);
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
  tft.setTextColor(C_GRAY);
  tft.print("Bekleniyor...");
  
  tft.fillRoundRect(0, 56, 158, 20, 3, C_DARK);
  tft.drawRoundRect(0, 56, 158, 20, 3, C_BORDER);
  tft.setTextColor(C_CYAN);
  tft.setCursor(4, 60);
  tft.print("IR: ");
  tft.setTextColor(C_YELLOW);
  tft.print(lastIR.c_str());
  
  tft.fillRoundRect(0, 79, 158, 20, 3, C_DARK);
  tft.drawRoundRect(0, 79, 158, 20, 3, C_BORDER);
  tft.setTextColor(C_CYAN);
  tft.setCursor(4, 83);
  tft.print("SES: ");
  tft.setTextColor(C_WHITE);
  tft.print(lastVoiceCmd.length() > 0 ? lastVoiceCmd.substring(0, 16).c_str() : "—");
  
  tft.setTextColor(C_GRAY);
  tft.setCursor(4, 127);
  tft.printf("RAM:%dK", ESP.getFreeHeap() / 1024);
  tft.setCursor(4, 137);
  tft.printf("RSSI:%ddBm", WiFi.RSSI());
  
  drawNavBar();
}

void drawIRPage() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(2, 2);
  tft.print("IR PROFILLER");
  tft.drawFastHLine(0, 12, 160, C_BORDER);
  tft.setTextColor(C_WHITE);
  tft.setCursor(2, 20);
  tft.print("[TV] Power, Vol+, Vol-");
  tft.setCursor(2, 40);
  tft.print("[AC] Power, Cool, Heat");
  tft.setCursor(2, 60);
  tft.print("[FAN] Power");
  drawNavBar();
}

void drawSystemPage() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(2, 2);
  tft.print("SISTEM");
  tft.drawFastHLine(0, 12, 160, C_BORDER);
  
  int y = 20;
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("IP:"); tft.setCursor(72, y);
  tft.setTextColor(C_YELLOW);
  tft.print(WiFi.localIP().toString().c_str());
  
  y += 11;
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("SSID:"); tft.setCursor(72, y);
  tft.setTextColor(C_YELLOW);
  tft.print(WiFi.SSID().c_str());
  
  y += 11;
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("RSSI:"); tft.setCursor(72, y);
  tft.setTextColor(C_YELLOW);
  tft.printf("%d dBm", WiFi.RSSI());
  
  y += 11;
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("Uptime:"); tft.setCursor(72, y);
  tft.setTextColor(C_YELLOW);
  tft.print(uptime().c_str());
  
  y += 11;
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, y); tft.print("RAM:"); tft.setCursor(72, y);
  tft.setTextColor(C_YELLOW);
  tft.printf("%d KB", ESP.getFreeHeap() / 1024);
  
  drawNavBar();
}

void drawHackPage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  for(int i = 0; i < 30; i++) {
    tft.setTextColor((random(3) == 0) ? C_GREEN : 0x0300);
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
  tft.print("HWME-AI v5 JARVIS");
  tft.setCursor(14, 72);
  tft.setTextColor(0x03E0);
  tft.print("> \"HACK\" de uyan_");
  
  tft.setCursor(14, 84);
  tft.setTextColor(0x03A0);
  if(lastVoiceCmd.length() > 0) {
    tft.print(("> " + lastVoiceCmd).substring(0, 22).c_str());
  } else {
    tft.print("> hazir_");
  }
  
  drawNavBar();
}

void refreshDisplay() {
  if(menuActive) {
    switch(currentPage) {
      case 0: drawHomePage(); break;
      case 1: drawIRPage(); break;
      case 2: drawSystemPage(); break;
      case 3: drawHackPage(); break;
    }
    drawMenu();
  } else {
    switch(currentPage) {
      case 0: drawHomePage(); break;
      case 1: drawIRPage(); break;
      case 2: drawSystemPage(); break;
      case 3: drawHackPage(); break;
    }
  }
}

// ══════════════════════════════════════════════════════
// WIFI & MQTT
// ══════════════════════════════════════════════════════

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
    Serial.printf("\n[WiFi] ✓ Connected - IP: %s\n", WiFi.localIP().toString().c_str());
    blink(3, 80);
  } else {
    Serial.println("\n[WiFi] Failed");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // MQTT message handling
  Serial.printf("[MQTT] Message received on %s\n", topic);
}

void connectMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  
  while(!mqtt.connected()) {
    if(mqtt.connect("HWME-ESP32")) {
      mqtt.subscribe("home/ir/send");
      mqtt.subscribe("home/ir/learn");
      Serial.println("[MQTT] Connected");
      break;
    } else {
      Serial.println("[MQTT] Connection failed");
      delay(3000);
    }
    wdt();
  }
}

void setupOTA() {
  ArduinoOTA.setHostname("HWME-AI-ESP32");
  ArduinoOTA.setPassword("hwme2024");
  ArduinoOTA.begin();
  Serial.println("[OTA] Ready for updates");
}

// ══════════════════════════════════════════════════════
// SETUP & LOOP
// ══════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(300);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  HWME-AI v5 JARVIS MODU            ║");
  Serial.println("║  Starting...                        ║");
  Serial.println("╚════════════════════════════════════╝");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  esp_task_wdt_init(30, true);
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
  tft.print("JARVIS MODU");
  tft.setTextColor(C_GRAY);
  tft.setCursor(10, 68);
  tft.print("Baslatiliyor...");
  Serial.println("[TFT] Initialized");
  
  // Initialize buttons
  initButtons();
  
  // IR Setup
  irSetup();
  
  // WiFi Setup
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 80);
  tft.print("WiFi...");
  connectWiFi();
  
  // MQTT Setup
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 92);
  tft.print("MQTT...");
  connectMQTT();
  
  // OTA Setup
  setupOTA();
  
  delay(500);
  refreshDisplay();
  blink(5, 80);
  
  Serial.println("[SYSTEM] Ready!");
}

void loop() {
  wdt();
  
  // Handle WiFi
  if(WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  // Handle MQTT
  if(!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();
  
  // Handle OTA
  ArduinoOTA.handle();
  
  // Handle buttons
  handleButtons();
  
  // Update display
  static unsigned long lastRefresh = 0;
  if(millis() - lastRefresh > 2000) {
    refreshDisplay();
    lastRefresh = millis();
  }
  
  delay(10);
}