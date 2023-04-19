#include <Adafruit_SSD1306.h>

#include <DHT.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "./configuration.h"

#define cbi(by, bi) (by &= ~(1 << bi))
#define sbi(by, bi) (by |= (1 << bi))

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SLEEP_TIME 1000 * 15

/**
  TEMPERATURE AND HUMIDITY SENSOR
*/
#define DHTPIN 2
#define DHTTYPE DHT22

#define LIGHT_SENSOR A3

/**
 * SOIL SENSOR PINS
 */

#define SOIL_SENSOR_01 A0
#define SOIL_SENSOR_02 A1
#define SOIL_SENSOR_03 A2

/**
  BUTTONS PINS
*/
#define BTN_1  3 // PD3 PCINT19
#define BTN_2  4 // PD4 PCINT20
#define BTN_3  5 // PD5 PCINT21

/** PUMPS **/
#define PUMP_01 6
#define PUMP_02 7
#define PUMP_03 8

/**
  State of the app
*/
enum AppState {
  HOME, // Show the home screen
  SETTINGS // Show the Settings screen
};

/**
  State of the Settings screen
*/
enum SettingsState {
  FREQUENCY,
  SECONDS_PUMP,
  SOIL_SENSOR_SET,
  LIGHT_SENSOR_SET,
  CALIBRATE_SOIL_SENSOR,
  SAVE,
  CALIBRATE_LIGHT_SENSOR
};

// 'plant_dry', 13x13px
const unsigned char epd_bitmap_plant_dry [] PROGMEM = {
  0x18, 0x00, 0x24, 0xc0, 0x75, 0x20, 0x75, 0x70, 0x52, 0x70, 0x02, 0x50, 0x12, 0x00, 0x0a, 0x80, 
  0x0f, 0x00, 0x07, 0x00, 0x06, 0x00, 0x02, 0x00, 0x07, 0x00
};

// 'plant_good', 13x13px
const unsigned char epd_bitmap_plant_good [] PROGMEM = {
  0x0a, 0x80, 0x05, 0x00, 0x07, 0x00, 0x07, 0x00, 0x12, 0x40, 0x1a, 0xc0, 0x0f, 0x80, 0x07, 0x00, 
  0x02, 0x00, 0x02, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00
};

// 'moon', 8x8px
const unsigned char epd_bitmap_moon [] PROGMEM = {
  0x78, 0x3c, 0x1e, 0x1e, 0x1e, 0x1e, 0x3c, 0x78
};

// 'sun', 9x9px
const unsigned char epd_bitmap_sun [] PROGMEM = {
  0x08, 0x00, 0x49, 0x00, 0x22, 0x00, 0x1c, 0x00, 0xd5, 0x80, 0x1c, 0x00, 0x22, 0x00, 0x49, 0x00, 
  0x08, 0x00
};

// Start the app in HOME
AppState appState = HOME;
SettingsState settingsState = FREQUENCY;
// Which pump is being setup in the settings

uint8_t pumpIdxSettings = 0;
// List of available Pumps
const uint8_t pumpsPins[NUM_PUMPS] = {PUMP_01};
// List of Soil sensors
const uint8_t soilSensorsPins[NUM_PUMPS] = {SOIL_SENSOR_01};
// Pumps state
byte pumpsActive = 0b000;
// Current pump showing info - home screen
byte pumpInfoIdx = 0;
// Last time the pump started, in in milliseconds
uint32_t pumpStartedMs[NUM_PUMPS];

boolean sleeping = false;


/**
 * Avoid interference at the buttons when clicked - debounce them
 */
#define DEBOUNCE_DELAY_MS 200
/**
 * Debounce the buttons
 */
volatile uint32_t lastDebounceTimeMs = 0;

/**
   DATA INITIALIZATION
*/
// Initialize display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Initialize temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

char lineBuffer[22];
volatile uint32_t currentMillis;

void setup() {

  wdt_enable(WDTO_1S);
  Serial.begin(9600);

  // Init the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.begin(9600);
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setTextColor(WHITE);
  display.clearDisplay();

  // Init temperature and humidity sensor
  dht.begin();

  // Init PINs
  pinMode(LIGHT_SENSOR, INPUT);

  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  
  for (uint8_t i = 0; i < NUM_PUMPS; i++) {
    pinMode(pumpsPins[i], OUTPUT);
    digitalWrite(pumpsPins[i], HIGH);
    delay(200);
    digitalWrite(pumpsPins[i], LOW);
  }

  for (uint8_t i = 0; i < NUM_PUMPS; i++) {
      pinMode(soilSensorsPins[i], INPUT);
  }

  cli();
  // Set PIN On Change Interrupts
  PCICR  = 0b00000100;
  PCMSK2 = 0b00111000;

  // Init TIMER2: Overflow each 1sec, async
  sbi(ASSR, AS2);
  TCCR2A = 0x00;
  // Prescaler 128
  sbi(TCCR2B, CS22);
  cbi(TCCR2B, CS21);
  sbi(TCCR2B, CS20);
  while(!TCN2UB && !OCR2AUB && !OCR2BUB && !TCR2AUB && !TCR2BUB);
  TIMSK2 = 0x01;

  set_sleep_mode(SLEEP_MODE_PWR_SAVE);

  sei();
  
  loadEEPROM();
}


ISR(TIMER2_OVF_vect) {
  uint32_t m = currentMillis;
  m += 1000;
  currentMillis = m;
}

ISR(PCINT2_vect) {
  lastDebounceTimeMs = currentMillis;
}


void loop() {
  static volatile uint8_t isSleeping = 0;

  readSensors();
  checkSchedule();
  runPumps();

  wdt_reset();

  // Enter sleep mode after SLEEP_TIME and if no pump is active
  isSleeping = (lastDebounceTimeMs + SLEEP_TIME < currentMillis) && (pumpsActive == 0);

  if (!isSleeping) {
    int btn1State = digitalRead(BTN_1);
    int btn2State = digitalRead(BTN_2);
    int btn3State = digitalRead(BTN_3);
  
    if (uint32_t(currentMillis - lastDebounceTimeMs) >= DEBOUNCE_DELAY_MS) {
      if (!isSleeping) {
        if (btn1State == LOW) {
          btn1Press();
        } else if (btn2State == LOW) {
          btn2Press();
        } else if (btn3State == LOW) {
          btn3Press();
        }
      }
    }
    render();
  } else {
    display.clearDisplay();
    display.display();
    wdt_disable();
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    wdt_enable(WDTO_1S);
  }
}

void readSensors() {
  sensorData.temperature = int(dht.readTemperature());
  sensorData.humidity = int(dht.readHumidity());

  int sensorRead;

  sensorRead = analogRead(LIGHT_SENSOR);
  sensorRead = map(sensorRead, sensorData.lightNightValue, sensorData.lightDayValue, 0, 100);
  sensorRead = constrain(sensorRead, 0, 100);
  sensorData.light = filterNoise(sensorData.light, sensorRead);

  for (uint8_t i = 0; i < NUM_PUMPS; i++) {
    Pump* pump = &pumps[i];
    sensorRead = analogRead(soilSensorsPins[i]);
    sensorRead = map(sensorRead, pump->config.soilSensorAirValue, pump->config.soilSensorWaterValue, 0, 100);
    sensorRead = constrain(sensorRead, 0, 100);
    pump->soilMoisture = filterNoise(pump->soilMoisture, sensorRead);
  }
}

/**
  Main method to render the screen. Render according to AppState.
  */
void render() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();

  switch (appState) {
    case HOME:
      renderHome();
      break;
    case SETTINGS:
      renderSettings();
      break;
    default:
      break;
  }

  display.display();
}


void header(int8_t temp, uint8_t humid, uint8_t light) {
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0,3);
  sprintf_P(lineBuffer, PSTR("%+2dC"), temp);
  display.print(lineBuffer);

  display.setCursor(104,3);
  sprintf_P(lineBuffer, PSTR("%3d%%"), humid);
  display.print(lineBuffer);

  display.setCursor(49,3);
  sprintf_P(lineBuffer, PSTR("%c%3d%%"), 0x0f, light);
  display.print(lineBuffer);

  display.drawFastHLine(0, 15, SCREEN_WIDTH, WHITE);
}

#define BAR_SIZE 32

void body(uint8_t pumpIdx, uint16_t secondsNextPump, uint16_t secondsPump, uint8_t soil, uint8_t minSoil, uint8_t light, uint8_t minLight) {
  display.setCursor(0,24);
  display.setTextSize(3);
  display.print(F("P"));
  display.print(pumpIdx + 1);

  if (isPumping(pumpIdx)) {
    display.setCursor(40, 22);
    display.setTextSize(1);
    display.print(F("...RUNNING..."));
    uint32_t configPumpMs = ((uint32_t) secondsPump) * 1000ul;
    uint32_t elapsedMs = currentMillis - pumpStartedMs[pumpIdx];
    int secsLeft = int((configPumpMs - elapsedMs) / 1000ul);
    sprintf_P(lineBuffer, PSTR("%03dsecs left"), secsLeft);    
    display.setCursor(40, 38);
    display.setTextSize(1);
    display.print(lineBuffer);
    //display.drawBitmap(107, 24, epd_bitmap_fan, 20, 20, WHITE);
    footer(F(""), F("Cancel"), F("Next"));
  } else {
    // Light Sensor
    display.setTextSize(1);
    display.setCursor(40, 22);
    display.write(0x0f);
    display.drawRoundRect(48, 22, BAR_SIZE, 7, 4, WHITE);
    display.fillRoundRect(48, 22, (BAR_SIZE * light) / 100, 7, 4, WHITE);
    display.drawFastVLine(48 + (BAR_SIZE * minLight) / 100, 19, 13, WHITE);
    // Soil Sensor
    display.setTextSize(1);
    display.setCursor(88, 22);
    display.write(0xef);
    display.drawRoundRect(95, 22, BAR_SIZE, 7, 4, WHITE);
    display.fillRoundRect(95, 22, (BAR_SIZE * soil) / 100, 7, 4, WHITE);
    display.drawFastVLine(95 + (BAR_SIZE * minSoil) / 100, 19, 13, WHITE);

    display.setTextSize(1);
    display.setCursor(40, 35);
    sprintf_P(lineBuffer, PSTR("%02dh%02dmin"), secondsNextPump / 60 / 60, (secondsNextPump / 60) % 60);
    display.print(lineBuffer);
    sprintf_P(lineBuffer, PSTR("%03dsecs"), secondsPump);    
    display.setCursor(40, 45);
    display.print(lineBuffer);
    if (soil > minSoil) {
      display.drawBitmap(95, 37, epd_bitmap_plant_good, 13, 13, WHITE);
    } else {
      display.drawBitmap(95, 37, epd_bitmap_plant_dry, 13, 13, WHITE);
    }  
    if (light > minLight) {
      display.drawBitmap(112, 40, epd_bitmap_sun, 9, 9, WHITE);
    } else {
      display.drawBitmap(112, 40, epd_bitmap_moon, 8, 8, WHITE);
    }
    footer(F("Set."), F("Run"), F("Next"));
  }
}

/**
  Render the Home screen.
  Show the current time, weekday, temperature, humidity and if should run.
*/
void renderHome() {
  Pump* pump = &pumps[pumpInfoIdx];
  header(sensorData.temperature, sensorData.humidity, sensorData.light);
  body(pumpInfoIdx, secondsToNextRun(pump, currentMillis), pump->config.secondsPump, pump->soilMoisture, pump->config.soilSensor, sensorData.light, pump->config.lightSensor);
}

/**
  Render settings screen. For each SettingsState, render different information.
  */
void renderSettings() {
  Pump* pump = &pumps[pumpIdxSettings];

  display.setTextColor(WHITE);
  printCenterH(F("SETUP"), 2, 0, 0);
  display.drawFastHLine(0, 15, SCREEN_WIDTH, WHITE);

  if (settingsState != CALIBRATE_LIGHT_SENSOR) {
    display.setCursor(0,24);
    display.setTextSize(3);
    display.print(F("P"));
    display.print(pumpIdxSettings + 1);
  }

  switch (settingsState) {
    case FREQUENCY:
      printCenterH(F("Frequency"), 1, 24, 20);
      sprintf_P(lineBuffer, PSTR("%02dh%02dmin"), pump->config.frequency / 60, pump->config.frequency % 60);
      printCenterH(lineBuffer, 1, 24, 35);
      footer(F("next"), F("+"), F("-"));
      break;
    case SECONDS_PUMP:
      printCenterH(F("Seconds Pump"), 1, 24, 20);
      sprintf_P(lineBuffer, PSTR("%03dsecs") ,pump->config.secondsPump);
      printCenterH(lineBuffer, 1, 24, 35);
      footer(F("next"), F("+"), F("-"));
      break;
    case SOIL_SENSOR_SET:
      printCenterH(F("Water if soil"), 1, 28, 20);
      printCenterH(F("below"), 1, 28, 30);
      sprintf_P(lineBuffer, PSTR("%c%3d%%"), 0xef, pump->config.soilSensor);
      printCenterH(lineBuffer, 1, 28, 40);
      footer(F("next"), F("+"), F("-"));
      break;
    case LIGHT_SENSOR_SET:
      printCenterH(F("Water if light"), 1, 28, 20);
      printCenterH(F("above"), 1, 28, 30);
      sprintf_P(lineBuffer, PSTR("%c%3d%%"), 0x0f, pump->config.lightSensor);
      printCenterH(lineBuffer, 1, 28, 40);
      footer(F("next"), F("+"), F("-"));
      break;
    case CALIBRATE_SOIL_SENSOR:
      printCenterH(F("Soil Calib."), 1, 28, 20);
      sprintf_P(lineBuffer, PSTR("Dry: %4d"), pump->config.soilSensorAirValue);
      printCenterH(lineBuffer, 1, 28, 30);
      sprintf_P(lineBuffer, PSTR("Wet: %4d"), pump->config.soilSensorWaterValue);
      printCenterH(lineBuffer, 1, 28, 40);
      footer(F("next"), F("dry"), F("wet"));
      break;
    case SAVE:
      printCenterH(F("Save"), 1, 28, 26);
      printCenterH(F("Settings?"), 1, 28, 36);
      footer(F("no"), F("yes"), F("exit"));
      break;
    case CALIBRATE_LIGHT_SENSOR:
      printCenterH(F("Light Sensor Calib."), 1, 0, 20);
      sprintf_P(lineBuffer, PSTR("Day: %4d"), sensorData.lightDayValue);
      printCenterH(lineBuffer, 1, 0, 30);
      sprintf_P(lineBuffer, PSTR("Night: %4d"), sensorData.lightNightValue);
      printCenterH(lineBuffer, 1, 0, 40);
      footer(F("save"), F("day"), F("night"));
      break;
  }

}

void footer(const __FlashStringHelper* b1, const __FlashStringHelper* b2, const __FlashStringHelper* b3) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.fillRect(0, 54, SCREEN_WIDTH, 10, WHITE);

  display.setCursor(2, 55);
  display.print(b1);

  printCenterH(b2, 1, 0, 55);

  display.setCursor(SCREEN_WIDTH - (strlen_P((const char *) b3) * 6) - 2, 55);
  display.print(b3);

  display.setTextColor(WHITE);
}


void printCenterH(const __FlashStringHelper* text, uint8_t size, int16_t x, int16_t y) {
  strcpy_P(lineBuffer, (const char*) text);
  printCenterH(lineBuffer, size, x, y);
}

void printCenterH(const char* text, uint8_t size, int16_t x, int16_t y) {
  int16_t  x1, y1;
  uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  display.setCursor(x1 + (((SCREEN_WIDTH - x1) / 2) - (w / 2)), y1);
  display.print(text);
}

void rotateSettings() {
  // Still setting pump values - advance
  if (settingsState < SAVE) {
    settingsState = (SettingsState) (((uint8_t) settingsState) + 1);
  } else if (settingsState == SAVE && pumpIdxSettings < NUM_PUMPS - 1) {
  // Still some pumps left to setup, advance the pumpIdx and start again
    settingsState = FREQUENCY;
    pumpIdxSettings++;
  } else if (settingsState != CALIBRATE_LIGHT_SENSOR) {
    // If there is no more pump to set, move to the calibrate light sensor
    settingsState = CALIBRATE_LIGHT_SENSOR;
  } else {
    // after all move to home
    appState = HOME;
  }
}

/**
  Main button press action. Should switch AppState and SettingsState
  Debounced to avoid any noise.
  */
void btn1Press() {
  if (appState == SETTINGS) {
    if (settingsState == SAVE) {
      // User didn't save. Read again the configuration from EEPROM
      loadEEPROM();
    } else if (settingsState == CALIBRATE_LIGHT_SENSOR) {
      saveEEPROM();
    }
    rotateSettings();
  } else {
    // Start SETTINGS screen
    appState = SETTINGS;
    settingsState = FREQUENCY;
    pumpIdxSettings = 0;
  }
}

/**
  "Set" button press action, to confirm/change the settings.
  If in Home screen, activate the water pump for the defined amount of time.
  Debounced to avoid any noise.
  */
void btn2Press() {
  if (appState == SETTINGS) {
    Pump* pump = &pumps[pumpIdxSettings];
    switch(settingsState) {
      case FREQUENCY:
        incFrequency(pump, true);
        break;
      case SECONDS_PUMP:
        incSecondsPump(pump, true);
        break;
      case SOIL_SENSOR_SET:
        incSoilSensor(pump, true);
        break;
      case LIGHT_SENSOR_SET:
        incLightSensor(pump, true);
        break;
      case CALIBRATE_SOIL_SENSOR:
        pump->config.soilSensorAirValue = analogRead(soilSensorsPins[pumpIdxSettings]);
        break;
      case CALIBRATE_LIGHT_SENSOR:
        sensorData.lightDayValue = analogRead(LIGHT_SENSOR);
        break;
      case SAVE:
        // User saved the config
        saveEEPROM();
        rotateSettings();
        break;
    }
  } else if (appState == HOME) {
    if (pumpsActive > 0) {
      stopAllPumps();
    } else {
      startAllPumps();
    }
  } else {
    appState = HOME;
  }
}

void btn3Press() {
 if (appState == SETTINGS) {
   Pump* pump = &pumps[pumpIdxSettings];
   switch(settingsState) {
     case FREQUENCY:
       incFrequency(pump, false);
       break;
     case SECONDS_PUMP:
       incSecondsPump(pump, false);
       break;
     case SOIL_SENSOR_SET:
       incSoilSensor(pump, false);
       break;
     case LIGHT_SENSOR_SET:
       incLightSensor(pump, false);
       break;
     case CALIBRATE_SOIL_SENSOR:
       pump->config.soilSensorWaterValue = analogRead(soilSensorsPins[pumpIdxSettings]);
       break;
     case CALIBRATE_LIGHT_SENSOR:
        sensorData.lightNightValue = analogRead(LIGHT_SENSOR);
       break;
     case SAVE:
       appState = HOME;
       break;
   }
 } else if (appState == HOME) {
   pumpInfoIdx = (pumpInfoIdx + 1) % NUM_PUMPS;        
 }
}

void startAllPumps() {
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++) {
    Pump* pump = &pumps[pumpIdx];
    pump->lastRunMs = currentMillis;
    startPump(pumpIdx);
  }
}

void stopAllPumps() {
  pumpsActive = 0b000;
}


/**
  Check if, according to the configuration and current datetime, should start the water pump.
*/
void checkSchedule() {
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++) {
    Pump* pump = &pumps[pumpIdx];
    if(!isPumping(pumpIdx) && isTimeToRun(pump, currentMillis)) {
      startPump(pumpIdx);
    } else if (isPumping(pumpIdx)) {
      uint32_t pumpMs = (uint32_t)pump->config.secondsPump * 1000ul;
      if((uint32_t)(currentMillis - pumpStartedMs[pumpIdx]) >= pumpMs) {
        stopPump(pumpIdx);
      }
    }
  }
}

/**
  Set the state to PUMPING and start the water pump.
  */
void startPump(uint8_t pump) {
  pumpStartedMs[pump] = currentMillis;
  setIsPumping(pump, true);
}

/**
  Set the state to PUMPING and stop the water pump.
  */
void stopPump(uint8_t pump) {
  setIsPumping(pump, false);
}

bool isPumping(uint8_t pumpIdx) {
  return (pumpsActive & _BV(pumpIdx)) > 0;
}

bool setIsPumping(int pumpIdx, bool value) {
  if (value) {
    pumpsActive |= _BV(pumpIdx);
  } else {
    pumpsActive &= ~_BV(pumpIdx);
  }
}

/** 
  Run the Pumps in case they are activated
  */
void runPumps() {
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++) {
    uint8_t pumpPin = pumpsPins[pumpIdx];
    if (isPumping(pumpIdx)) {
      digitalWrite(pumpPin, HIGH);
    } else {
      digitalWrite(pumpPin, LOW);
    }
  }
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
