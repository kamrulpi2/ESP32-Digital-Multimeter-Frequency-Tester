// /*
//  * =============================================
//  *  ESP32 Self-Test Frequency Tester
//  *  GPIO 18 = Test Signal Output
//  *  GPIO 34 = Frequency Input
//  *  OLED 0.96" SSD1306
//  * =============================================
//  *  Wiring:
//  *  GPIO 18 ──[1kΩ]──► GPIO 34
//  *  OLED SDA --> GPIO 21
//  *  OLED SCL --> GPIO 22
//  * =============================================
// */

// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// // ---- OLED ----
// #define SCREEN_WIDTH  128
// #define SCREEN_HEIGHT  64
// #define OLED_RESET     -1
// #define OLED_ADDRESS  0x3C
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// // ---- Pins ----
// #define SIGNAL_OUT  18   // Test signal output
// #define SIGNAL_IN   34   // Frequency measurement input

// // ---- Test Frequencies ----
// // যেকোনো frequency এখানে add করুন (Hz)
// const long testFrequencies[] = {
//   100, 500, 1000, 5000, 10000, 50000, 100000
// };
// const int totalTests = sizeof(testFrequencies) / sizeof(testFrequencies[0]);
// int currentTest = 0;
// unsigned long lastFreqChange = 0;
// unsigned long freqChangInterval = 5000; // 5 সেকেন্ড পর পর বদলাবে

// // ---- Frequency Measurement ----
// volatile unsigned long pulseCount = 0;
// unsigned long lastMeasureTime    = 0;
// float measuredFreq               = 0.0;
// float targetFreq                 = 0.0;
// float minFreq                    = 999999.0;
// float maxFreq                    = 0.0;
// unsigned long readingCount       = 0;
// float errorPercent               = 0.0;

// // ---- PWM Config ----
// #define PWM_CHANNEL   0
// #define PWM_RESOLUTION 8  // 8-bit

// // ---- ISR ----
// void IRAM_ATTR onPulse() {
//   pulseCount++;
// }

// // ---- Format Frequency ----
// String formatFreq(float f) {
//   if (f >= 1000000.0)     return String(f / 1000000.0, 3) + " MHz";
//   else if (f >= 1000.0)   return String(f / 1000.0, 3) + " kHz";
//   else                    return String(f, 1) + " Hz";
// }

// // ---- Set Output Frequency ----
// void setOutputFreq(long freqHz) {
//   ledcSetup(PWM_CHANNEL, freqHz, PWM_RESOLUTION);
//   ledcAttachPin(SIGNAL_OUT, PWM_CHANNEL);
//   ledcWrite(PWM_CHANNEL, 128); // 50% duty cycle
//   targetFreq = freqHz;

//   Serial.println();
//   Serial.println("================================================");
//   Serial.print  ("  TARGET: ");
//   Serial.print  (formatFreq(freqHz));
//   Serial.println("  (5 sec test)");
//   Serial.println("================================================");
//   Serial.println("Reading#   Measured       Period         Error%");
//   Serial.println("------------------------------------------------");
// }

// // ---- Update OLED ----
// void updateOLED() {
//   display.clearDisplay();

//   // Title bar
//   display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
//   display.setTextColor(SSD1306_BLACK);
//   display.setTextSize(1);
//   display.setCursor(12, 2);
//   display.print("SELF TEST MODE");
//   display.setTextColor(SSD1306_WHITE);

//   // Target frequency
//   display.setTextSize(1);
//   display.setCursor(0, 14);
//   display.print("TARGET: ");
//   display.print(formatFreq(targetFreq));

//   // Measured frequency (big)
//   String freqStr = (measuredFreq < 10.0) ? "No Signal" : formatFreq(measuredFreq);
//   display.setTextSize(2);
//   int16_t x1, y1; uint16_t w, h;
//   display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
//   display.setCursor((128 - w) / 2, 24);
//   display.print(freqStr);

//   // Error %
//   display.setTextSize(1);
//   display.setCursor(0, 44);
//   if (measuredFreq > 10.0) {
//     display.print("Error: ");
//     display.print(errorPercent, 2);
//     display.print("%");
//   } else {
//     display.print("Waiting...");
//   }

//   // Progress bar (time left in current test)
//   unsigned long elapsed = millis() - lastFreqChange;
//   int barWidth = map(elapsed, 0, freqChangInterval, 0, 118);
//   display.drawRect(5, 55, 118, 7, SSD1306_WHITE);
//   display.fillRect(5, 55, barWidth, 7, SSD1306_WHITE);

//   // Test number
//   display.setCursor(0, 56);
//   display.setTextColor(SSD1306_BLACK);
//   display.print(currentTest + 1);
//   display.print("/");
//   display.print(totalTests);
//   display.setTextColor(SSD1306_WHITE);

//   display.display();
// }

// // ---- Setup ----
// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   // OLED
//   if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
//     Serial.println("[ERROR] OLED not found!");
//     while (true);
//   }

//   // Splash
//   display.clearDisplay();
//   display.setTextColor(SSD1306_WHITE);
//   display.setTextSize(1);
//   display.setCursor(10, 8);  display.println("ESP32 Self-Test");
//   display.setCursor(10, 22); display.println("GPIO18 -> 1k -> GPIO34");
//   display.setCursor(10, 36); display.println("Testing frequencies:");
//   display.setCursor(10, 48); display.println("100Hz to 100kHz");
//   display.display();
//   delay(3000);

//   // Input pin
//   pinMode(SIGNAL_IN, INPUT);
//   attachInterrupt(digitalPinToInterrupt(SIGNAL_IN), onPulse, RISING);

//   // Start first test
//   setOutputFreq(testFrequencies[currentTest]);
//   lastFreqChange = millis();
//   lastMeasureTime = millis();

//   Serial.println();
//   Serial.println("  Wiring: GPIO18 --[1kOhm]--> GPIO34");
//   Serial.println("  Each frequency runs for 5 seconds");
// }

// // ---- Loop ----
// void loop() {
//   unsigned long now = millis();

//   // ---- Measure every 1 second ----
//   if (now - lastMeasureTime >= 1000) {
//     noInterrupts();
//     unsigned long count = pulseCount;
//     pulseCount = 0;
//     interrupts();

//     unsigned long elapsed = now - lastMeasureTime;
//     lastMeasureTime = now;

//     measuredFreq = (float)count / (elapsed / 1000.0);

//     if (measuredFreq < 10.0) measuredFreq = 0.0;

//     if (measuredFreq > 0) {
//       readingCount++;
//       if (measuredFreq < minFreq) minFreq = measuredFreq;
//       if (measuredFreq > maxFreq) maxFreq = measuredFreq;

//       // Error calculation
//       errorPercent = abs(measuredFreq - targetFreq) / targetFreq * 100.0;

//       float period_us = 1000000.0 / measuredFreq;
//       String pStr = (period_us >= 1000.0)
//                     ? String(period_us / 1000.0, 3) + " ms "
//                     : String(period_us, 2) + " us ";

//       Serial.printf("%-10lu %-15s %-15s %.2f%%\n",
//         readingCount,
//         formatFreq(measuredFreq).c_str(),
//         pStr.c_str(),
//         errorPercent
//       );
//     }
//   }

//   // ---- Change frequency every 5 seconds ----
//   if (now - lastFreqChange >= freqChangInterval) {
//     currentTest = (currentTest + 1) % totalTests;
//     minFreq = 999999.0;
//     maxFreq = 0.0;
//     readingCount = 0;
//     setOutputFreq(testFrequencies[currentTest]);
//     lastFreqChange = now;
//   }

//   // ---- OLED update ----
//   updateOLED();
//   delay(100);
// }