// /*
//  * ╔══════════════════════════════════════════════════════╗
//  * ║        ESP32 Digital Multimeter - RTOS Edition       ║
//  * ║  Modes: Frequency | Resistor | Capacitor | Diode    ║
//  * ║         LED Test  | Voltage                          ║
//  * ║  OLED 0.96" SSD1306 | WiFi WebUI | Push Button      ║
//  * ╠══════════════════════════════════════════════════════╣
//  * ║  WIRING:                                             ║
//  * ║  OLED SDA      --> GPIO 21                           ║
//  * ║  OLED SCL      --> GPIO 22                           ║
//  * ║  OLED VCC      --> 3.3V                              ║
//  * ║  OLED GND      --> GND                               ║
//  * ║                                                      ║
//  * ║  SIGNAL IN     --> GPIO 34  (Frequency)              ║
//  * ║  RESISTOR PIN  --> GPIO 35  (+ 10kΩ to 3.3V)        ║
//  * ║  CAPACITOR PIN --> GPIO 32                           ║
//  * ║  DIODE ANODE   --> GPIO 25  (+ 1kΩ series)          ║
//  * ║  DIODE CATHODE --> GND                               ║
//  * ║  LED ANODE     --> GPIO 26  (+ 330Ω series)         ║
//  * ║  VOLTAGE IN    --> GPIO 33  (voltage divider /2)     ║
//  * ║                                                      ║
//  * ║  PUSH BUTTON   --> GPIO 0 (boot btn) or GPIO 27     ║
//  * ║  (1x press = next mode, hold 3s = confirm/reset)    ║
//  * ╚══════════════════════════════════════════════════════╝
//  * 
//  *  Libraries needed:
//  *  - Adafruit SSD1306
//  *  - Adafruit GFX
//  *  - WiFi (built-in ESP32)
//  *  - AsyncTCP + ESPAsyncWebServer
//  */

// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// #include <WiFi.h>
// #include <WebServer.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <freertos/semphr.h>
// #include <math.h>

// // Forward declaration for Arduino auto-prototype issue
// struct MeasData;

// void drawOLED_Frequency(MeasData &d);
// void drawOLED_Resistor(MeasData &d);
// void drawOLED_Capacitor(MeasData &d);
// void drawOLED_Diode(MeasData &d);
// void drawOLED_LED(MeasData &d);
// void drawOLED_Voltage(MeasData &d);

// // ═══════════════════════════════════════════
// //  CONFIG
// // ═══════════════════════════════════════════
// #define WIFI_SSID       "Tester"
// #define WIFI_PASS       "12341234"

// #define OLED_WIDTH      128
// #define OLED_HEIGHT      64
// #define OLED_RESET       -1
// #define OLED_ADDR       0x3C

// // ── Pins ──
// #define PIN_FREQ_IN      27   // Frequency input (interrupt capable)
// #define PIN_RESIST       35   // Resistor measurement (ADC)
// #define PIN_CAP          32   // Capacitor measurement
// #define PIN_CAP_CHARGE   33   // Cap charge pin (digital out)
// #define PIN_DIODE        25   // Diode test output
// #define PIN_DIODE_READ   36   // Diode voltage read (ADC)
// #define PIN_LED          26   // LED test output
// #define PIN_LED_READ     39   // LED forward voltage read
// #define PIN_VOLTAGE      18   // Voltage measurement (ADC, divider)
// #define PIN_BUTTON        0   // Push button (GPIO0 = BOOT button)

// // ── Resistor divider ref ──
// #define RESIST_REF      10000.0f  // 10kΩ pull-up
// #define ADC_VREF         3.3f
// #define ADC_MAX         4095.0f
// #define VIN_DIVIDER      2.0f    // Voltage divider ratio for voltage mode

// // ═══════════════════════════════════════════
// //  GLOBALS
// // ═══════════════════════════════════════════
// Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
// WebServer server(80);
// SemaphoreHandle_t dataMutex;

// // ── Modes ──
// typedef enum {
//   MODE_FREQUENCY = 0,
//   MODE_RESISTOR,
//   MODE_CAPACITOR,
//   MODE_DIODE,
//   MODE_LED,
//   MODE_VOLTAGE,
//   MODE_COUNT
// } DeviceMode;

// const char* modeNames[] = {
//   "FREQUENCY", "RESISTOR", "CAPACITOR",
//   "DIODE", "LED TEST", "VOLTAGE"
// };
// const char* modeIcons[] = { "Hz", "Ω", "F", "D|", "LED", "V" };

// volatile DeviceMode currentMode   = MODE_FREQUENCY;
// volatile bool       modeChanged   = false;

// // ── Button state ──
// volatile unsigned long btnPressTime  = 0;
// volatile bool          btnPressed    = false;
// volatile bool          shortPress    = false;
// volatile bool          longPress     = false;

// // ── Measurement data (shared, mutex protected) ──
// struct MeasData {
//   // Frequency
//   float   freq        = 0;
//   float   freqMin     = 9999999;
//   float   freqMax     = 0;
//   float   period_us   = 0;

//   // Resistor
//   float   resistance  = 0;  // Ohms

//   // Capacitor
//   float   capacitance = 0;  // uF

//   // Diode
//   float   diodeVf     = 0;  // Forward voltage
//   bool    diodeOk     = false;
//   String  diodeType   = "---";

//   // LED
//   float   ledVf       = 0;
//   bool    ledOn       = false;
//   String  ledColor    = "---";

//   // Voltage
//   float   voltage     = 0;

//   // WiFi
//   String  ip          = "---";
//   bool    wifiConn    = false;
//   int     clients     = 0;
// } data;

// // ── Frequency ISR ──
// volatile unsigned long freqPulseCount = 0;
// void IRAM_ATTR freqISR() { freqPulseCount++; }

// // ═══════════════════════════════════════════
// //  HELPERS
// // ═══════════════════════════════════════════
// String fmtFreq(float f) {
//   if (f <= 0)          return "No Signal";
//   if (f >= 1000000.0f) return String(f/1000000.0f, 3) + " MHz";
//   if (f >= 1000.0f)    return String(f/1000.0f, 3)    + " kHz";
//   return String(f, 2) + " Hz";
// }

// String fmtResist(float r) {
//   if (r <= 0 || r > 9000000) return "Open";
//   if (r < 100)        return String(r, 1) + " Ω";
//   if (r < 1000)       return String(r, 0) + " Ω";
//   if (r < 1000000)    return String(r/1000.0f, 2) + " kΩ";
//   return String(r/1000000.0f, 2) + " MΩ";
// }

// String fmtCap(float c) {
//   if (c <= 0)         return "No Cap";
//   if (c < 1.0f)       return String(c*1000.0f, 1) + " nF";
//   if (c < 1000.0f)    return String(c, 2) + " uF";
//   return String(c/1000.0f, 2) + " mF";
// }

// String fmtVoltage(float v) {
//   if (v < 0.01f) return "0.00 V";
//   return String(v, 3) + " V";
// }

// float readADC(int pin, int samples = 16) {
//   long sum = 0;
//   for (int i = 0; i < samples; i++) {
//     sum += analogRead(pin);
//     delayMicroseconds(100);
//   }
//   return (sum / (float)samples) * (ADC_VREF / ADC_MAX);
// }

// // ═══════════════════════════════════════════
// //  OLED DRAW TASK
// // ═══════════════════════════════════════════
// void drawBar(int x, int y, int w, int h, float pct) {
//   oled.drawRect(x, y, w, h, SSD1306_WHITE);
//   int fill = (int)(pct * (w - 2));
//   if (fill > 0) oled.fillRect(x+1, y+1, fill, h-2, SSD1306_WHITE);
// }

// void drawOLED_Frequency(MeasData &d) {
//   // Title
//   oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(2, 2);  oled.print("~ FREQUENCY METER");
//   oled.setTextColor(SSD1306_WHITE);

//   // Main value
//   String fStr = fmtFreq(d.freq);
//   oled.setTextSize(d.freq >= 1000 ? 1 : 2);
//   if (d.freq <= 0) { oled.setTextSize(1); }
//   int16_t x1,y1; uint16_t tw,th;
//   oled.getTextBounds(fStr,0,0,&x1,&y1,&tw,&th);
//   oled.setCursor((128-tw)/2, 14);
//   oled.setTextSize(2);
//   if (tw > 110) oled.setTextSize(1);
//   oled.print(fStr);

//   // Period
//   oled.setTextSize(1);
//   oled.setCursor(0, 36);
//   if (d.freq > 0) {
//     String p = d.period_us >= 1000 ?
//       "T:" + String(d.period_us/1000.0f,2)+"ms" :
//       "T:" + String(d.period_us,1)+"us";
//     oled.print(p);
//   } else {
//     oled.print("T: ---");
//   }

//   // Min/Max
//   oled.setCursor(0, 46);
//   oled.print("Lo:"); oled.print(d.freqMin > 9000000 ? "---" : fmtFreq(d.freqMin));
//   oled.setCursor(0, 56);
//   oled.print("Hi:"); oled.print(fmtFreq(d.freqMax));

//   // Signal dot blink
//   static bool blink = false; blink = !blink;
//   if (d.freq > 0 && blink)
//     oled.fillCircle(123, 5, 3, SSD1306_BLACK);
// }

// void drawOLED_Resistor(MeasData &d) {
//   oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(8, 2); oled.print("RESISTOR TEST");
//   oled.setTextColor(SSD1306_WHITE);

//   String rStr = fmtResist(d.resistance);
//   oled.setTextSize(2);
//   int16_t x1,y1; uint16_t tw,th;
//   oled.getTextBounds(rStr,0,0,&x1,&y1,&tw,&th);
//   oled.setCursor((128-tw)/2, 15);
//   oled.print(rStr);

//   // Range indicator
//   oled.setTextSize(1);
//   oled.setCursor(0, 36);
//   float r = d.resistance;
//   if (r < 100)        oled.print("Range: < 100 Ohm");
//   else if (r < 10000) oled.print("Range: 100-10k Ohm");
//   else if (r < 1e6)   oled.print("Range: 10k-1M Ohm");
//   else                oled.print("Range: > 1M Ohm");

//   // Bar
//   float pct = min(1.0f, (float)log10(max(1.0f,r)) / 7.0f);
//   drawBar(0, 47, 128, 8, pct);

//   oled.setCursor(0, 57);
//   oled.print("Ref: 10k | GPIO35");
// }

// void drawOLED_Capacitor(MeasData &d) {
//   oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(5, 2); oled.print("CAPACITOR TEST");
//   oled.setTextColor(SSD1306_WHITE);

//   String cStr = fmtCap(d.capacitance);
//   oled.setTextSize(2);
//   int16_t x1,y1; uint16_t tw,th;
//   oled.getTextBounds(cStr,0,0,&x1,&y1,&tw,&th);
//   oled.setCursor((128-tw)/2, 15);
//   oled.print(cStr);

//   oled.setTextSize(1);
//   oled.setCursor(0, 36);
//   float c = d.capacitance;
//   if (c <= 0)       oled.print("No capacitor");
//   else if (c < 1)   oled.print("Ceramic/Film type");
//   else if (c < 100) oled.print("Electrolytic type");
//   else              oled.print("Large electrolytic");

//   float pct = min(1.0f, (float)log10(max(0.001f,c)+1) / 4.0f);
//   drawBar(0, 47, 128, 8, pct);
//   oled.setCursor(0, 57);
//   oled.print("GPIO32/33 | RC method");
// }

// void drawOLED_Diode(MeasData &d) {
//   oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(14, 2); oled.print("DIODE TEST");
//   oled.setTextColor(SSD1306_WHITE);

//   oled.setTextSize(2);
//   String vStr = d.diodeOk ? fmtVoltage(d.diodeVf) : "Open/Bad";
//   int16_t x1,y1; uint16_t tw,th;
//   oled.getTextBounds(vStr,0,0,&x1,&y1,&tw,&th);
//   oled.setCursor((128-tw)/2, 14);
//   oled.print(vStr);

//   oled.setTextSize(1);
//   oled.setCursor(0, 34);
//   oled.print("Type: "); oled.print(d.diodeType);

//   oled.setCursor(0, 44);
//   if (d.diodeOk) {
//     oled.print("Status: GOOD ✓");
//   } else {
//     oled.print("Status: NO DIODE");
//   }

//   // Diode symbol
//   oled.drawLine(90, 44, 100, 44, SSD1306_WHITE);
//   oled.fillTriangle(100,40, 100,48, 108,44, SSD1306_WHITE);
//   oled.drawLine(108,40, 108,48, SSD1306_WHITE);
//   oled.drawLine(108,44, 118,44, SSD1306_WHITE);

//   oled.setCursor(0, 56);
//   oled.print("A:GPIO25 K:GND+1kOhm");
// }

// void drawOLED_LED(MeasData &d) {
//   oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(20, 2); oled.print("LED TEST");
//   oled.setTextColor(SSD1306_WHITE);

//   oled.setTextSize(2);
//   String vStr = d.ledOn ? fmtVoltage(d.ledVf) : "No LED";
//   int16_t x1,y1; uint16_t tw,th;
//   oled.getTextBounds(vStr,0,0,&x1,&y1,&tw,&th);
//   oled.setCursor((128-tw)/2, 14);
//   oled.print(vStr);

//   oled.setTextSize(1);
//   oled.setCursor(0, 34);
//   oled.print("Color: "); oled.print(d.ledColor);

//   oled.setCursor(0, 44);
//   if (d.ledOn) {
//     oled.print("LED is WORKING ✓");
//   } else {
//     oled.print("No LED detected");
//   }

//   // LED symbol (circle with rays)
//   oled.drawCircle(110, 44, 7, SSD1306_WHITE);
//   if (d.ledOn) {
//     oled.drawLine(115,36, 120,30, SSD1306_WHITE);
//     oled.drawLine(117,40, 123,37, SSD1306_WHITE);
//   }

//   oled.setCursor(0, 56);
//   oled.print("A:GPIO26 K:GND+330Ohm");
// }

// void drawOLED_Voltage(MeasData &d) {
//   oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(8, 2); oled.print("VOLTAGE METER");
//   oled.setTextColor(SSD1306_WHITE);

//   String vStr = fmtVoltage(d.voltage);
//   oled.setTextSize(2);
//   int16_t x1,y1; uint16_t tw,th;
//   oled.getTextBounds(vStr,0,0,&x1,&y1,&tw,&th);
//   oled.setCursor((128-tw)/2, 15);
//   oled.print(vStr);

//   oled.setTextSize(1);
//   oled.setCursor(0, 36);
//   float v = d.voltage;
//   if (v < 0.1)       oled.print("Range: ~0V");
//   else if (v < 1.0)  oled.print("Range: mV level");
//   else if (v < 3.3)  oled.print("Range: Low voltage");
//   else               oled.print("Range: Normal");

//   float pct = min(1.0f, d.voltage / 6.6f);
//   drawBar(0, 47, 128, 8, pct);

//   oled.setCursor(0, 57);
//   oled.print("Max:6.6V | GPIO27/Div2");
// }

// void drawModeSelect() {
//   oled.clearDisplay();
//   oled.fillRect(0, 0, 128, 64, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(20, 3); oled.print("SELECT MODE");
//   oled.fillRect(0,12,128,1,SSD1306_BLACK);

//   for (int i = 0; i < MODE_COUNT; i++) {
//     int row = i % 3;
//     int col = i / 3;
//     int x = col * 64 + 2;
//     int y = 15 + row * 16;
//     if ((int)currentMode == i) {
//       oled.fillRect(x-1, y-1, 62, 14, SSD1306_BLACK);
//       oled.setTextColor(SSD1306_WHITE);
//     } else {
//       oled.setTextColor(SSD1306_BLACK);
//     }
//     oled.setCursor(x+2, y+2);
//     oled.print(modeIcons[i]);
//     oled.setCursor(x+18, y+2);
//     oled.print(modeNames[i]);
//     oled.setTextColor(SSD1306_BLACK);
//   }
//   oled.display();
// }

// // ═══════════════════════════════════════════
// //  OLED TASK
// // ═══════════════════════════════════════════
// void taskOLED(void *pvParam) {
//   static unsigned long lastUpdate = 0;
//   while (true) {
//     if (millis() - lastUpdate < 150) { vTaskDelay(10 / portTICK_PERIOD_MS); continue; }
//     lastUpdate = millis();

//     MeasData d;
//     if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//       d = data;
//       xSemaphoreGive(dataMutex);
//     }

//     oled.clearDisplay();

//     // Mode indicator dots (bottom right, tiny)
//     for (int i = 0; i < MODE_COUNT; i++) {
//       int x = 128 - (MODE_COUNT - i) * 5;
//       if (i == (int)currentMode)
//         oled.fillRect(x, 61, 3, 3, SSD1306_WHITE);
//       else
//         oled.drawRect(x, 61, 3, 3, SSD1306_WHITE);
//     }

//     // WiFi indicator
//     if (d.wifiConn) {
//       oled.fillRect(0, 61, 3, 3, SSD1306_WHITE);
//     }

//     switch (currentMode) {
//       case MODE_FREQUENCY: drawOLED_Frequency(d); break;
//       case MODE_RESISTOR:  drawOLED_Resistor(d);  break;
//       case MODE_CAPACITOR: drawOLED_Capacitor(d); break;
//       case MODE_DIODE:     drawOLED_Diode(d);     break;
//       case MODE_LED:       drawOLED_LED(d);        break;
//       case MODE_VOLTAGE:   drawOLED_Voltage(d);   break;
//       default: break;
//     }

//     oled.display();
//     vTaskDelay(10 / portTICK_PERIOD_MS);
//   }
// }

// // ═══════════════════════════════════════════
// //  MEASUREMENT TASK
// // ═══════════════════════════════════════════
// // ── Frequency ──
// void measureFrequency() {
//   static unsigned long lastTime = 0;
//   unsigned long now = millis();
//   if (now - lastTime < 1000) return;

//   noInterrupts();
//   unsigned long cnt = freqPulseCount;
//   freqPulseCount = 0;
//   interrupts();

//   float elapsed = (now - lastTime) / 1000.0f;
//   lastTime = now;

//   float f = cnt / elapsed;
//   if (f < 5.0f) f = 0;

//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//     data.freq = f;
//     if (f > 0) {
//       data.period_us = 1000000.0f / f;
//       if (f < data.freqMin) data.freqMin = f;
//       if (f > data.freqMax) data.freqMax = f;
//     }
//     xSemaphoreGive(dataMutex);
//   }
// }

// // ── Resistor ──
// void measureResistor() {
//   float v = readADC(PIN_RESIST, 32);
//   float r = 0;
//   if (v > 0.05f && v < (ADC_VREF - 0.05f)) {
//     r = RESIST_REF * v / (ADC_VREF - v);
//   }
//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//     data.resistance = r;
//     xSemaphoreGive(dataMutex);
//   }
// }

// // ── Capacitor (RC time constant method) ──
// void measureCapacitor() {
//   const float R_charge = 10000.0f; // 10kΩ
//   const float V_thresh = ADC_VREF * 0.632f;
//   const float thresh_adc = (V_thresh / ADC_VREF) * ADC_MAX;

//   // Discharge
//   pinMode(PIN_CAP_CHARGE, OUTPUT);
//   digitalWrite(PIN_CAP_CHARGE, LOW);
//   delay(300);

//   // Charge and time
//   unsigned long t0 = micros();
//   digitalWrite(PIN_CAP_CHARGE, HIGH);

//   unsigned long timeout = 2000000UL;
//   while (analogRead(PIN_CAP) < thresh_adc) {
//     if (micros() - t0 > timeout) break;
//   }
//   unsigned long t1 = micros();
//   digitalWrite(PIN_CAP_CHARGE, LOW);
//   pinMode(PIN_CAP_CHARGE, INPUT);

//   float tau = (t1 - t0) / 1000000.0f;
//   float cap_uF = (tau / R_charge) * 1000000.0f;
//   if (cap_uF < 0.001f || tau >= timeout/1000000.0f) cap_uF = 0;

//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//     data.capacitance = cap_uF;
//     xSemaphoreGive(dataMutex);
//   }
// }

// // ── Diode ──
// void measureDiode() {
//   pinMode(PIN_DIODE, OUTPUT);
//   digitalWrite(PIN_DIODE, HIGH);
//   delay(10);

//   float v = readADC(PIN_DIODE_READ, 32);
//   // Vf = 3.3 - V_measured (across 1kΩ, we measure drop)
//   // Actually Vf = measured voltage at anode side
//   float vf = v;
//   bool ok = (vf > 0.2f && vf < 1.2f);

//   String dtype = "---";
//   if (ok) {
//     if (vf < 0.4f)      dtype = "Schottky";
//     else if (vf < 0.75f) dtype = "Silicon";
//     else if (vf < 1.0f)  dtype = "Germanium?";
//     else                 dtype = "LED/Zener";
//   }

//   digitalWrite(PIN_DIODE, LOW);

//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//     data.diodeVf   = vf;
//     data.diodeOk   = ok;
//     data.diodeType = dtype;
//     xSemaphoreGive(dataMutex);
//   }
// }

// // ── LED ──
// void measureLED() {
//   pinMode(PIN_LED, OUTPUT);
//   digitalWrite(PIN_LED, HIGH);
//   delay(20);

//   float v = readADC(PIN_LED_READ, 32);
//   bool on = (v > 1.5f && v < 4.0f);

//   String color = "Unknown";
//   if (on) {
//     if (v < 1.8f)      color = "Infrared";
//     else if (v < 2.1f) color = "Red";
//     else if (v < 2.2f) color = "Yellow";
//     else if (v < 2.4f) color = "Green";
//     else if (v < 3.0f) color = "Blue/White";
//     else               color = "UV";
//   }

//   digitalWrite(PIN_LED, LOW);

//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//     data.ledVf    = v;
//     data.ledOn    = on;
//     data.ledColor = color;
//     xSemaphoreGive(dataMutex);
//   }
// }

// // ── Voltage ──
// void measureVoltage() {
//   float v = readADC(PIN_VOLTAGE, 32);
//   float actual = v * VIN_DIVIDER;

//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//     data.voltage = actual;
//     xSemaphoreGive(dataMutex);
//   }
// }

// void taskMeasure(void *pvParam) {
//   while (true) {
//     switch (currentMode) {
//       case MODE_FREQUENCY: measureFrequency(); vTaskDelay(50/portTICK_PERIOD_MS);  break;
//       case MODE_RESISTOR:  measureResistor();  vTaskDelay(200/portTICK_PERIOD_MS); break;
//       case MODE_CAPACITOR: measureCapacitor(); vTaskDelay(500/portTICK_PERIOD_MS); break;
//       case MODE_DIODE:     measureDiode();     vTaskDelay(300/portTICK_PERIOD_MS); break;
//       case MODE_LED:       measureLED();       vTaskDelay(300/portTICK_PERIOD_MS); break;
//       case MODE_VOLTAGE:   measureVoltage();   vTaskDelay(100/portTICK_PERIOD_MS); break;
//       default: vTaskDelay(100/portTICK_PERIOD_MS); break;
//     }
//   }
// }

// // ═══════════════════════════════════════════
// //  BUTTON TASK
// // ═══════════════════════════════════════════
// void taskButton(void *pvParam) {
//   static unsigned long pressStart = 0;
//   static bool wasPressed = false;
//   static bool longFired  = false;

//   while (true) {
//     bool btnDown = (digitalRead(PIN_BUTTON) == LOW);

//     if (btnDown && !wasPressed) {
//       pressStart = millis();
//       wasPressed = true;
//       longFired  = false;
//     }

//     if (wasPressed && btnDown) {
//       if (!longFired && millis() - pressStart >= 3000) {
//         longFired = true;
//         // Long press: reset min/max for frequency
//         if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//           data.freqMin = 9999999;
//           data.freqMax = 0;
//           xSemaphoreGive(dataMutex);
//         }
//         // Show confirmation on OLED
//         oled.clearDisplay();
//         oled.setTextColor(SSD1306_WHITE);
//         oled.setTextSize(1);
//         oled.setCursor(20, 20); oled.print("Min/Max Reset!");
//         oled.setCursor(15, 35); oled.print("Hold: Reset Stats");
//         oled.display();
//         delay(1000);
//         Serial.println("[BTN] Long press - Stats reset");
//       }
//     }

//     if (!btnDown && wasPressed) {
//       unsigned long held = millis() - pressStart;
//       if (!longFired && held > 50 && held < 3000) {
//         // Short press: next mode
//         currentMode = (DeviceMode)(((int)currentMode + 1) % MODE_COUNT);
//         Serial.print("[BTN] Mode -> "); Serial.println(modeNames[(int)currentMode]);

//         // Show mode selection briefly
//         oled.clearDisplay();
//         oled.setTextColor(SSD1306_WHITE);
//         oled.fillRect(0,0,128,12,SSD1306_WHITE);
//         oled.setTextColor(SSD1306_BLACK);
//         oled.setTextSize(1);
//         oled.setCursor(5,2); oled.print("MODE CHANGED");
//         oled.setTextColor(SSD1306_WHITE);
//         oled.setTextSize(2);
//         oled.setCursor(5,20); oled.print(modeIcons[(int)currentMode]);
//         oled.setCursor(30,20); oled.print(modeNames[(int)currentMode]);
//         oled.display();
//         delay(800);
//       }
//       wasPressed = false;
//       longFired  = false;
//     }

//     vTaskDelay(20 / portTICK_PERIOD_MS);
//   }
// }

// // ═══════════════════════════════════════════
// //  SERIAL PRINT TASK
// // ═══════════════════════════════════════════
// void taskSerial(void *pvParam) {
//   while (true) {
//     MeasData d;
//     if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//       d = data;
//       xSemaphoreGive(dataMutex);
//     }

//     Serial.print("["); Serial.print(modeNames[(int)currentMode]); Serial.print("] ");
//     switch (currentMode) {
//       case MODE_FREQUENCY:
//         Serial.printf("Freq: %-12s Period: %-10s Min: %-12s Max: %s\n",
//           fmtFreq(d.freq).c_str(),
//           d.freq>0 ? (String(d.period_us,1)+"us").c_str() : "---",
//           d.freqMin>9000000 ? "---" : fmtFreq(d.freqMin).c_str(),
//           fmtFreq(d.freqMax).c_str());
//         break;
//       case MODE_RESISTOR:
//         Serial.printf("R: %s\n", fmtResist(d.resistance).c_str());
//         break;
//       case MODE_CAPACITOR:
//         Serial.printf("C: %s\n", fmtCap(d.capacitance).c_str());
//         break;
//       case MODE_DIODE:
//         Serial.printf("Diode Vf: %.3fV  Type: %s  OK: %s\n",
//           d.diodeVf, d.diodeType.c_str(), d.diodeOk?"YES":"NO");
//         break;
//       case MODE_LED:
//         Serial.printf("LED Vf: %.3fV  Color: %s  ON: %s\n",
//           d.ledVf, d.ledColor.c_str(), d.ledOn?"YES":"NO");
//         break;
//       case MODE_VOLTAGE:
//         Serial.printf("Voltage: %s\n", fmtVoltage(d.voltage).c_str());
//         break;
//     }

//     vTaskDelay(1000 / portTICK_PERIOD_MS);
//   }
// }

// // ═══════════════════════════════════════════
// //  WEB SERVER
// // ═══════════════════════════════════════════
// // (HTML is served from PROGMEM to save RAM)
// const char INDEX_HTML[] PROGMEM = R"rawhtml(
// <!DOCTYPE html><html lang="en"><head>
// <meta charset="UTF-8">
// <meta name="viewport" content="width=device-width,initial-scale=1">
// <title>DigiMeter Pro</title>
// <style>
//   :root{--bg:#0a0e1a;--card:#111827;--border:#1e2d45;--accent:#00d4ff;--accent2:#7c3aed;--green:#10b981;--red:#ef4444;--yellow:#f59e0b;--text:#e2e8f0;--sub:#64748b}
//   *{box-sizing:border-box;margin:0;padding:0}
//   body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh;padding:0 0 80px}
//   header{background:linear-gradient(135deg,#0a0e1a 0%,#111827 100%);border-bottom:1px solid var(--border);padding:16px 20px;display:flex;align-items:center;gap:12px;position:sticky;top:0;z-index:100;backdrop-filter:blur(10px)}
//   .logo{width:36px;height:36px;background:linear-gradient(135deg,var(--accent),var(--accent2));border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:18px;font-weight:900;color:#fff}
//   h1{font-size:18px;font-weight:700;letter-spacing:.5px}
//   .badge{margin-left:auto;background:var(--green);color:#fff;font-size:11px;padding:3px 10px;border-radius:20px;font-weight:600}
//   .badge.off{background:var(--red)}
//   main{padding:16px;max-width:480px;margin:0 auto}
//   .mode-tabs{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:16px}
//   .tab{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:10px 6px;text-align:center;cursor:pointer;transition:all .2s;font-size:12px;font-weight:600;color:var(--sub)}
//   .tab.active{background:linear-gradient(135deg,rgba(0,212,255,.15),rgba(124,58,237,.15));border-color:var(--accent);color:var(--accent)}
//   .tab .icon{font-size:20px;display:block;margin-bottom:4px}
//   .card{background:var(--card);border:1px solid var(--border);border-radius:16px;padding:20px;margin-bottom:14px}
//   .card-title{font-size:11px;font-weight:700;color:var(--sub);letter-spacing:1px;margin-bottom:12px;text-transform:uppercase}
//   .big-val{font-size:42px;font-weight:800;color:var(--accent);letter-spacing:-1px;line-height:1;font-variant-numeric:tabular-nums}
//   .big-val.green{color:var(--green)}
//   .big-val.red{color:var(--red)}
//   .unit{font-size:16px;color:var(--sub);font-weight:400;margin-left:4px}
//   .stats-row{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:14px}
//   .stat{background:rgba(255,255,255,.03);border:1px solid var(--border);border-radius:10px;padding:10px 12px}
//   .stat-label{font-size:10px;color:var(--sub);font-weight:600;text-transform:uppercase;margin-bottom:4px}
//   .stat-val{font-size:16px;font-weight:700;color:var(--text);font-variant-numeric:tabular-nums}
//   .bar-wrap{margin-top:14px}
//   .bar-label{font-size:11px;color:var(--sub);margin-bottom:6px;display:flex;justify-content:space-between}
//   .bar-bg{background:rgba(255,255,255,.06);border-radius:6px;height:8px;overflow:hidden}
//   .bar-fill{height:100%;border-radius:6px;background:linear-gradient(90deg,var(--accent),var(--accent2));transition:width .4s ease}
//   .status-chip{display:inline-flex;align-items:center;gap:6px;padding:6px 14px;border-radius:20px;font-size:13px;font-weight:600;margin-top:10px}
//   .status-chip.ok{background:rgba(16,185,129,.15);border:1px solid var(--green);color:var(--green)}
//   .status-chip.bad{background:rgba(239,68,68,.15);border:1px solid var(--red);color:var(--red)}
//   .dot{width:8px;height:8px;border-radius:50%;background:currentColor}
//   .wifi-card{display:flex;align-items:center;gap:12px}
//   .wifi-icon{font-size:28px}
//   .wifi-info{flex:1}
//   .wifi-ip{font-size:15px;font-weight:700;font-family:monospace;color:var(--accent)}
//   .wifi-sub{font-size:11px;color:var(--sub);margin-top:2px}
//   .btn-row{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:14px}
//   .btn{border:none;border-radius:10px;padding:12px;font-size:13px;font-weight:700;cursor:pointer;transition:all .15s}
//   .btn-primary{background:linear-gradient(135deg,var(--accent),var(--accent2));color:#fff}
//   .btn-ghost{background:rgba(255,255,255,.06);border:1px solid var(--border);color:var(--text)}
//   .btn:active{transform:scale(.97)}
//   .pulse{animation:pulse 2s infinite}
//   @keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
//   footer{text-align:center;font-size:11px;color:var(--sub);padding:20px;margin-top:8px}
//   .update-dot{width:6px;height:6px;border-radius:50%;background:var(--green);display:inline-block;margin-right:4px;animation:pulse 1s infinite}
// </style>
// </head>
// <body>
// <header>
//   <div class="logo">⚡</div>
//   <div>
//     <h1>DigiMeter Pro</h1>
//     <div style="font-size:11px;color:var(--sub)">ESP32 Digital Multimeter</div>
//   </div>
//   <div class="badge" id="wifiBadge">LIVE</div>
// </header>
// <main>
//   <div class="mode-tabs">
//     <div class="tab active" onclick="setMode(0)" id="tab0"><span class="icon">〜</span>Freq</div>
//     <div class="tab" onclick="setMode(1)" id="tab1"><span class="icon">Ω</span>Resist</div>
//     <div class="tab" onclick="setMode(2)" id="tab2"><span class="icon">⊡</span>Cap</div>
//     <div class="tab" onclick="setMode(3)" id="tab3"><span class="icon">◁</span>Diode</div>
//     <div class="tab" onclick="setMode(4)" id="tab4"><span class="icon">💡</span>LED</div>
//     <div class="tab" onclick="setMode(5)" id="tab5"><span class="icon">⚡</span>Volt</div>
//   </div>

//   <div id="panelFreq" class="panel">
//     <div class="card">
//       <div class="card-title"><span class="update-dot"></span>Frequency</div>
//       <div class="big-val" id="freqVal">---</div>
//       <div class="stats-row">
//         <div class="stat"><div class="stat-label">Period</div><div class="stat-val" id="periodVal">---</div></div>
//         <div class="stat"><div class="stat-label">Min</div><div class="stat-val" id="freqMin">---</div></div>
//         <div class="stat"><div class="stat-label">Max</div><div class="stat-val" id="freqMax">---</div></div>
//         <div class="stat"><div class="stat-label">Signal</div><div class="stat-val" id="sigStatus">---</div></div>
//       </div>
//     </div>
//     <div class="btn-row">
//       <button class="btn btn-primary" onclick="resetStats()">Reset Min/Max</button>
//       <button class="btn btn-ghost" onclick="fetch('/api/data').then(r=>r.json()).then(update)">Refresh</button>
//     </div>
//   </div>

//   <div id="panelResist" class="panel" style="display:none">
//     <div class="card">
//       <div class="card-title"><span class="update-dot"></span>Resistance</div>
//       <div class="big-val green" id="resistVal">---</div>
//       <div class="bar-wrap">
//         <div class="bar-label"><span>0Ω</span><span>1MΩ</span></div>
//         <div class="bar-bg"><div class="bar-fill" id="resistBar" style="width:0%"></div></div>
//       </div>
//     </div>
//   </div>

//   <div id="panelCap" class="panel" style="display:none">
//     <div class="card">
//       <div class="card-title"><span class="update-dot"></span>Capacitance</div>
//       <div class="big-val" style="color:var(--yellow)" id="capVal">---</div>
//       <div class="bar-wrap">
//         <div class="bar-label"><span>0</span><span>1000µF</span></div>
//         <div class="bar-bg"><div class="bar-fill" id="capBar" style="width:0%;background:linear-gradient(90deg,var(--yellow),var(--accent2))"></div></div>
//       </div>
//     </div>
//   </div>

//   <div id="panelDiode" class="panel" style="display:none">
//     <div class="card">
//       <div class="card-title"><span class="update-dot"></span>Diode Test</div>
//       <div class="big-val" id="diodeVf">---</div>
//       <div id="diodeStatus" class="status-chip bad"><div class="dot"></div>No Diode</div>
//       <div class="stats-row" style="margin-top:14px">
//         <div class="stat"><div class="stat-label">Type</div><div class="stat-val" id="diodeType">---</div></div>
//         <div class="stat"><div class="stat-label">Status</div><div class="stat-val" id="diodeOk">---</div></div>
//       </div>
//     </div>
//   </div>

//   <div id="panelLed" class="panel" style="display:none">
//     <div class="card">
//       <div class="card-title"><span class="update-dot"></span>LED Test</div>
//       <div class="big-val" id="ledVf">---</div>
//       <div id="ledStatus" class="status-chip bad"><div class="dot"></div>No LED</div>
//       <div class="stats-row" style="margin-top:14px">
//         <div class="stat"><div class="stat-label">Color</div><div class="stat-val" id="ledColor">---</div></div>
//         <div class="stat"><div class="stat-label">Forward V</div><div class="stat-val" id="ledFwd">---</div></div>
//       </div>
//     </div>
//   </div>

//   <div id="panelVolt" class="panel" style="display:none">
//     <div class="card">
//       <div class="card-title"><span class="update-dot"></span>Voltage</div>
//       <div class="big-val" style="color:var(--yellow)" id="voltVal">---</div>
//       <div class="bar-wrap">
//         <div class="bar-label"><span>0V</span><span>6.6V</span></div>
//         <div class="bar-bg"><div class="bar-fill" id="voltBar" style="width:0%;background:linear-gradient(90deg,var(--green),var(--yellow),var(--red))"></div></div>
//       </div>
//     </div>
//   </div>

//   <div class="card">
//     <div class="wifi-card">
//       <div class="wifi-icon">📡</div>
//       <div class="wifi-info">
//         <div class="wifi-ip" id="wifiIP">Connecting...</div>
//         <div class="wifi-sub">WiFi AP: <b>ESP32-DigiMeter</b> | Pass: <b>12341234</b></div>
//       </div>
//     </div>
//   </div>

//   <footer><span class="update-dot"></span>Auto-refresh every 800ms &nbsp;|&nbsp; ESP32 DigiMeter Pro</footer>
// </main>
// <script>
// let activeMode = 0;
// const panels = ['panelFreq','panelResist','panelCap','panelDiode','panelLed','panelVolt'];

// function setMode(m) {
//   activeMode = m;
//   panels.forEach((p,i) => {
//     document.getElementById(p).style.display = i===m ? 'block' : 'none';
//     document.getElementById('tab'+i).classList.toggle('active', i===m);
//   });
//   fetch('/api/mode?m='+m);
// }

// function fmt(v, unit) {
//   if (v === undefined || v === null) return '---';
//   return v.toFixed(v >= 100 ? 0 : v >= 10 ? 1 : 2) + '<span class="unit">' + unit + '</span>';
// }

// function fmtFreq(f) {
//   if (!f || f < 5) return '<span style="color:var(--sub)">No Signal</span>';
//   if (f >= 1e6) return (f/1e6).toFixed(3) + '<span class="unit">MHz</span>';
//   if (f >= 1e3) return (f/1e3).toFixed(3) + '<span class="unit">kHz</span>';
//   return f.toFixed(1) + '<span class="unit">Hz</span>';
// }

// function fmtR(r) {
//   if (!r || r > 9e6) return '<span style="color:var(--sub)">Open</span>';
//   if (r < 1000) return r.toFixed(0) + '<span class="unit">Ω</span>';
//   if (r < 1e6)  return (r/1000).toFixed(2) + '<span class="unit">kΩ</span>';
//   return (r/1e6).toFixed(2) + '<span class="unit">MΩ</span>';
// }

// function fmtC(c) {
//   if (!c || c < 0.001) return '<span style="color:var(--sub)">No Cap</span>';
//   if (c < 1)    return (c*1000).toFixed(1) + '<span class="unit">nF</span>';
//   if (c < 1000) return c.toFixed(2) + '<span class="unit">µF</span>';
//   return (c/1000).toFixed(2) + '<span class="unit">mF</span>';
// }

// function update(d) {
//   // Frequency
//   document.getElementById('freqVal').innerHTML = fmtFreq(d.freq);
//   document.getElementById('periodVal').textContent = d.freq > 5 ?
//     (d.period_us >= 1000 ? (d.period_us/1000).toFixed(2)+'ms' : d.period_us.toFixed(1)+'µs') : '---';
//   document.getElementById('freqMin').textContent = d.freqMin > 9e6 ? '---' :
//     (d.freqMin >= 1000 ? (d.freqMin/1000).toFixed(1)+'kHz' : d.freqMin.toFixed(0)+'Hz');
//   document.getElementById('freqMax').textContent = d.freqMax > 0 ?
//     (d.freqMax >= 1000 ? (d.freqMax/1000).toFixed(1)+'kHz' : d.freqMax.toFixed(0)+'Hz') : '---';
//   document.getElementById('sigStatus').textContent = d.freq > 5 ? '✓ Active' : '✗ None';

//   // Resistor
//   document.getElementById('resistVal').innerHTML = fmtR(d.resistance);
//   document.getElementById('resistBar').style.width = Math.min(100, Math.log10(Math.max(1,d.resistance))/7*100) + '%';

//   // Capacitor
//   document.getElementById('capVal').innerHTML = fmtC(d.capacitance);
//   document.getElementById('capBar').style.width = Math.min(100, d.capacitance/10) + '%';

//   // Diode
//   document.getElementById('diodeVf').innerHTML = d.diodeOk ?
//     d.diodeVf.toFixed(3)+'<span class="unit">V</span>' :
//     '<span style="color:var(--sub)">Open</span>';
//   const ds = document.getElementById('diodeStatus');
//   ds.className = 'status-chip ' + (d.diodeOk ? 'ok' : 'bad');
//   ds.innerHTML = '<div class="dot"></div>' + (d.diodeOk ? 'Good Diode' : 'No Diode');
//   document.getElementById('diodeType').textContent = d.diodeType || '---';
//   document.getElementById('diodeOk').textContent = d.diodeOk ? 'GOOD ✓' : 'FAIL ✗';

//   // LED
//   document.getElementById('ledVf').innerHTML = d.ledOn ?
//     d.ledVf.toFixed(3)+'<span class="unit">V</span>' :
//     '<span style="color:var(--sub)">No LED</span>';
//   const ls = document.getElementById('ledStatus');
//   ls.className = 'status-chip ' + (d.ledOn ? 'ok' : 'bad');
//   ls.innerHTML = '<div class="dot"></div>' + (d.ledOn ? 'LED Working' : 'No LED');
//   document.getElementById('ledColor').textContent = d.ledColor || '---';
//   document.getElementById('ledFwd').textContent = d.ledOn ? d.ledVf.toFixed(3)+'V' : '---';

//   // Voltage
//   document.getElementById('voltVal').innerHTML = d.voltage.toFixed(3)+'<span class="unit">V</span>';
//   document.getElementById('voltBar').style.width = Math.min(100, d.voltage/6.6*100) + '%';

//   // WiFi
//   document.getElementById('wifiIP').textContent = d.ip || '---';
// }

// function resetStats() {
//   fetch('/api/reset').then(()=>console.log('reset'));
// }

// function poll() {
//   fetch('/api/data').then(r=>r.json()).then(d => {
//     update(d);
//     document.getElementById('wifiBadge').textContent = 'LIVE';
//     document.getElementById('wifiBadge').className = 'badge';
//     document.getElementById('wifiIP').textContent = d.ip;
//   }).catch(() => {
//     document.getElementById('wifiBadge').textContent = 'OFFLINE';
//     document.getElementById('wifiBadge').className = 'badge off';
//   });
// }

// setInterval(poll, 800);
// poll();
// </script>
// </body></html>
// )rawhtml";

// void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

// void handleAPI() {
//   MeasData d;
//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//     d = data;
//     xSemaphoreGive(dataMutex);
//   }
//   String json = "{";
//   json += "\"mode\":"     + String((int)currentMode)      + ",";
//   json += "\"freq\":"     + String(d.freq, 2)             + ",";
//   json += "\"freqMin\":"  + String(d.freqMin > 9e6 ? 0 : d.freqMin, 2) + ",";
//   json += "\"freqMax\":"  + String(d.freqMax, 2)          + ",";
//   json += "\"period_us\":" + String(d.period_us, 2)       + ",";
//   json += "\"resistance\":" + String(d.resistance, 1)     + ",";
//   json += "\"capacitance\":" + String(d.capacitance, 4)   + ",";
//   json += "\"diodeVf\":"  + String(d.diodeVf, 3)          + ",";
//   json += "\"diodeOk\":"  + String(d.diodeOk ? "true":"false") + ",";
//   json += "\"diodeType\":\"" + d.diodeType               + "\",";
//   json += "\"ledVf\":"    + String(d.ledVf, 3)            + ",";
//   json += "\"ledOn\":"    + String(d.ledOn ? "true":"false") + ",";
//   json += "\"ledColor\":\"" + d.ledColor                  + "\",";
//   json += "\"voltage\":"  + String(d.voltage, 3)          + ",";
//   json += "\"ip\":\""     + WiFi.softAPIP().toString()    + "\"";
//   json += "}";
//   server.send(200, "application/json", json);
// }

// void handleMode() {
//   if (server.hasArg("m")) {
//     int m = server.arg("m").toInt();
//     if (m >= 0 && m < MODE_COUNT) {
//       currentMode = (DeviceMode)m;
//       Serial.print("[WiFi] Mode set to "); Serial.println(modeNames[m]);
//     }
//   }
//   server.send(200, "text/plain", "OK");
// }

// void handleReset() {
//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//     data.freqMin = 9999999;
//     data.freqMax = 0;
//     xSemaphoreGive(dataMutex);
//   }
//   server.send(200, "text/plain", "OK");
// }

// void taskWiFi(void *pvParam) {
//   WiFi.softAP(WIFI_SSID, WIFI_PASS);
//   IPAddress ip = WiFi.softAPIP();

//   if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
//     data.ip = ip.toString();
//     data.wifiConn = true;
//     xSemaphoreGive(dataMutex);
//   }

//   Serial.print("[WiFi] AP: "); Serial.println(WIFI_SSID);
//   Serial.print("[WiFi] IP: "); Serial.println(ip);
//   Serial.print("[WiFi] Pass: "); Serial.println(WIFI_PASS);

//   server.on("/",          handleRoot);
//   server.on("/api/data",  handleAPI);
//   server.on("/api/mode",  handleMode);
//   server.on("/api/reset", handleReset);
//   server.begin();

//   while (true) {
//     server.handleClient();
//     vTaskDelay(5 / portTICK_PERIOD_MS);
//   }
// }

// // ═══════════════════════════════════════════
// //  SETUP
// // ═══════════════════════════════════════════
// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   Serial.println();
//   Serial.println("╔══════════════════════════════════╗");
//   Serial.println("║   ESP32 DigiMeter Pro - RTOS     ║");
//   Serial.println("╚══════════════════════════════════╝");

//   // ── Pin setup ──
//   pinMode(PIN_FREQ_IN,  INPUT);
//   pinMode(PIN_RESIST,   INPUT);
//   pinMode(PIN_CAP,      INPUT);
//   pinMode(PIN_DIODE,    OUTPUT); digitalWrite(PIN_DIODE, LOW);
//   pinMode(PIN_LED,      OUTPUT); digitalWrite(PIN_LED, LOW);
//   pinMode(PIN_VOLTAGE,  INPUT);
//   pinMode(PIN_BUTTON,   INPUT_PULLUP);

//   attachInterrupt(digitalPinToInterrupt(PIN_FREQ_IN), freqISR, RISING);

//   // ── OLED ──
//   Wire.begin(21, 22);
//   if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
//     Serial.println("[ERROR] OLED not found!");
//   }

//   // Splash screen
//   oled.clearDisplay();
//   oled.setTextColor(SSD1306_WHITE);
//   oled.fillRect(0, 0, 128, 14, SSD1306_WHITE);
//   oled.setTextColor(SSD1306_BLACK);
//   oled.setTextSize(1);
//   oled.setCursor(10, 3); oled.print("DigiMeter Pro v2.0");
//   oled.setTextColor(SSD1306_WHITE);
//   oled.setCursor(5, 18);  oled.print("ESP32 + OLED + WiFi");
//   oled.setCursor(5, 30);  oled.print("6 Measurement Modes");
//   oled.setCursor(5, 42);  oled.print("WiFi: ESP32-DigiMeter");
//   oled.setCursor(5, 54);  oled.print("Pass: 12341234");
//   oled.display();
//   delay(3000);

//   // ── Mutex ──
//   dataMutex = xSemaphoreCreateMutex();

//   // ── RTOS Tasks ──
//   // Core 0: WiFi + Web server
//   xTaskCreatePinnedToCore(taskWiFi,    "WiFi",    8192, NULL, 1, NULL, 0);

//   // Core 1: Measurement + OLED + Button + Serial
//   xTaskCreatePinnedToCore(taskMeasure, "Measure", 4096, NULL, 3, NULL, 1);
//   xTaskCreatePinnedToCore(taskOLED,    "OLED",    4096, NULL, 2, NULL, 1);
//   xTaskCreatePinnedToCore(taskButton,  "Button",  2048, NULL, 2, NULL, 1);
//   xTaskCreatePinnedToCore(taskSerial,  "Serial",  2048, NULL, 1, NULL, 1);

//   Serial.println("[RTOS] All tasks started!");
//   Serial.println("[BTN]  1x press = Next Mode");
//   Serial.println("[BTN]  Hold 3s  = Reset Min/Max");
// }

// void loop() {
//   // Empty — everything runs in RTOS tasks
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
// }
