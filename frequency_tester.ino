// /*
//  * =============================================
//  *  ESP32 Frequency Tester
//  *  OLED 0.96" (SSD1306) + Serial Monitor
//  * =============================================
//  *  Wiring:
//  *  OLED SDA  --> GPIO 21
//  *  OLED SCL  --> GPIO 22
//  *  OLED VCC  --> 3.3V
//  *  OLED GND  --> GND
//  *
//  *  Signal IN --> GPIO 34 (input only pin)
//  *  Signal GND --> GND
//  * =============================================
//  *  Required Libraries:
//  *  - Adafruit SSD1306
//  *  - Adafruit GFX
//  * =============================================
// */

// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// // ---- OLED Config ----
// #define SCREEN_WIDTH  128
// #define SCREEN_HEIGHT  64
// #define OLED_RESET     -1
// #define OLED_ADDRESS  0x3C

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// // ---- Frequency Input Pin ----
// #define SIGNAL_PIN  34   // GPIO34 - input only (no pullup needed for most signals)

// // ---- Variables ----
// volatile unsigned long pulseCount = 0;
// unsigned long lastTime            = 0;
// float frequency                   = 0.0;
// float minFreq                     = 999999.0;
// float maxFreq                     = 0.0;
// unsigned long measureInterval     = 1000; // 1 second measurement window
// unsigned long displayUpdateMs     = 200;  // OLED refresh rate
// unsigned long lastDisplayUpdate   = 0;
// unsigned long readingCount        = 0;

// // ---- ISR: Count pulses ----
// void IRAM_ATTR onPulse() {
//   pulseCount++;
// }

// // ---- Setup ----
// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   Serial.println("==============================");
//   Serial.println("   ESP32 Frequency Tester");
//   Serial.println("   Signal Pin: GPIO34");
//   Serial.println("   OLED: SDA=21, SCL=22");
//   Serial.println("==============================");

//   // OLED Init
//   if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
//     Serial.println("[ERROR] OLED not found! Check wiring.");
//     while (true); // halt
//   }

//   display.clearDisplay();
//   display.setTextColor(SSD1306_WHITE);

//   // Splash screen
//   display.setTextSize(1);
//   display.setCursor(10, 10);
//   display.println("  Frequency Tester");
//   display.setCursor(20, 26);
//   display.println("  ESP32 + OLED");
//   display.setCursor(15, 42);
//   display.println("Signal -> GPIO 34");
//   display.setCursor(28, 54);
//   display.println("GND -> GND");
//   display.display();
//   delay(2500);

//   // Attach interrupt on rising edge
//   pinMode(SIGNAL_PIN, INPUT);
//   attachInterrupt(digitalPinToInterrupt(SIGNAL_PIN), onPulse, RISING);

//   lastTime = millis();
//   Serial.println();
//   Serial.println("Reading#   Frequency(Hz)   Period(us)   Min(Hz)   Max(Hz)");
//   Serial.println("----------------------------------------------------------");
// }

// // ---- Format frequency nicely ----
// String formatFreq(float f) {
//   if (f >= 1000000.0) {
//     return String(f / 1000000.0, 3) + " MHz";
//   } else if (f >= 1000.0) {
//     return String(f / 1000.0, 3) + " kHz";
//   } else {
//     return String(f, 2) + " Hz";
//   }
// }

// // ---- Draw OLED ----
// void updateOLED() {
//   display.clearDisplay();

//   // --- Title bar ---
//   display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
//   display.setTextColor(SSD1306_BLACK);
//   display.setTextSize(1);
//   display.setCursor(18, 2);
//   display.print("FREQUENCY TESTER");
//   display.setTextColor(SSD1306_WHITE);

//   // --- Main frequency ---
//   String freqStr = formatFreq(frequency);
//   display.setTextSize(2);
//   // Center the text
//   int16_t x1, y1;
//   uint16_t w, h;
//   display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
//   int cx = (SCREEN_WIDTH - w) / 2;
//   display.setCursor(cx, 16);
//   display.print(freqStr);

//   // --- Period ---
//   display.setTextSize(1);
//   if (frequency > 0) {
//     float period_us = 1000000.0 / frequency;
//     String pStr;
//     if (period_us >= 1000.0) {
//       pStr = "T: " + String(period_us / 1000.0, 3) + " ms";
//     } else {
//       pStr = "T: " + String(period_us, 2) + " us";
//     }
//     display.setCursor(4, 38);
//     display.print(pStr);
//   } else {
//     display.setCursor(4, 38);
//     display.print("T: No Signal");
//   }

//   // --- Min / Max ---
//   display.setCursor(4, 50);
//   display.print("Min:");
//   display.print(minFreq >= 999999.0 ? "---" : formatFreq(minFreq));

//   display.setCursor(4, 58);
//   display.print("Max:");
//   display.print(maxFreq == 0 ? "---" : formatFreq(maxFreq));

//   // --- Signal indicator dot ---
//   if (frequency > 0) {
//     display.fillCircle(122, 6, 3, SSD1306_BLACK); // inside white bar
//     display.fillCircle(122, 6, 2, SSD1306_WHITE);
//   }

//   display.display();
// }

// // ---- Loop ----
// void loop() {
//   unsigned long now = millis();

//   // Measure every `measureInterval` ms
//   if (now - lastTime >= measureInterval) {
//     // Safely read pulse count
//     noInterrupts();
//     unsigned long count = pulseCount;
//     pulseCount = 0;
//     interrupts();

//     unsigned long elapsed = now - lastTime;
//     lastTime = now;

//     // Calculate frequency
//     frequency = (float)count / (elapsed / 1000.0);

//     if (frequency > 0) {
//       readingCount++;
//       if (frequency < minFreq) minFreq = frequency;
//       if (frequency > maxFreq) maxFreq = frequency;

//       float period_us = 1000000.0 / frequency;
//       String pStr = (period_us >= 1000.0)
//                     ? String(period_us / 1000.0, 3) + " ms"
//                     : String(period_us, 2) + " us";

//       // Serial output (tabular)
//       Serial.printf("%-10lu %-16s %-12s %-10s %-10s\n",
//         readingCount,
//         formatFreq(frequency).c_str(),
//         pStr.c_str(),
//         (minFreq >= 999999.0 ? "---" : formatFreq(minFreq)).c_str(),
//         formatFreq(maxFreq).c_str()
//       );
//     } else {
//       Serial.println("--         No Signal        ---          ---        ---");
//     }
//   }

//   // Update OLED at refresh rate
//   if (now - lastDisplayUpdate >= displayUpdateMs) {
//     updateOLED();
//     lastDisplayUpdate = now;
//   }
// }