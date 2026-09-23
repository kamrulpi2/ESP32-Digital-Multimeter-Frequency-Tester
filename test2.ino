// #define FREQ_PIN 27      // Frequency measurement input
// #define TEST_OUT 18      // Frequency generator output


// volatile uint32_t pulseCount = 0;
// volatile uint32_t lastEdge = 0;

// hw_timer_t *timer = NULL;

// portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// volatile float frequency = 0;


// // Generate test frequency
// void setupPWM()
// {
//   ledcSetup(0, 1000, 8);     // Channel 0, 1kHz, 8-bit
//   ledcAttachPin(TEST_OUT,0);
//   ledcWrite(0,128);          // 50% duty
// }


// // Frequency input interrupt
// void IRAM_ATTR freqISR()
// {
//   uint32_t now = micros();

//   if(now - lastEdge > 20)
//   {
//     pulseCount++;
//     lastEdge = now;
//   }
// }


// // Timer every 500ms
// void IRAM_ATTR onTimer()
// {

//   portENTER_CRITICAL_ISR(&timerMux);

//   uint32_t count = pulseCount;
//   pulseCount = 0;

//   portEXIT_CRITICAL_ISR(&timerMux);


//   if(count < 2)
//   {
//     frequency = 0;
//   }
//   else
//   {
//     frequency = count * 2.0;
//   }

// }



// void setup()
// {

//   Serial.begin(115200);


//   pinMode(FREQ_PIN, INPUT_PULLUP);


//   // Start test signal
//   setupPWM();


//   attachInterrupt(
//     digitalPinToInterrupt(FREQ_PIN),
//     freqISR,
//     RISING
//   );


//   timer = timerBegin(0,80,true);


//   timerAttachInterrupt(
//     timer,
//     &onTimer,
//     true
//   );


//   timerAlarmWrite(
//     timer,
//     500000,
//     true
//   );


//   timerAlarmEnable(timer);


//   Serial.println("Frequency Tester Ready");
//   Serial.println("GPIO18 -> GPIO27 TEST");
// }



// void loop()
// {

//  static uint32_t last=0;


//  if(millis()-last > 500)
//  {

//   last=millis();


//   Serial.print("Measured: ");


//   if(frequency < 1)
//   {
//     Serial.println("No Signal");
//   }

//   else if(frequency < 1000)
//   {
//     Serial.print(frequency);
//     Serial.println(" Hz");
//   }

//   else
//   {
//     Serial.print(frequency/1000);
//     Serial.println(" kHz");
//   }


//  }

// }