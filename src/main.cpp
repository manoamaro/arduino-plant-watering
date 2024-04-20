#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include <DHT.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "configuration.h"
#include "pump.h"
#include "sensor_data.h"
#include "images.h"

#define SLEEP
#define WD

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SLEEP_TIME 1000 * 15

#define NUM_PUMPS 3

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
#define BTN_1 3 // PD3 PCINT19
#define BTN_2 4 // PD4 PCINT20
#define BTN_3 5 // PD5 PCINT21

/** PUMPS **/
#define PUMP_01 6
#define PUMP_02 7
#define PUMP_03 8

/**
  State of the app
*/
enum AppState
{
  HOME,    // Show the home screen
  SETTINGS // Show the Settings screen
};

/**
  State of the Settings screen
*/
enum SettingsState
{
  FREQUENCY,
  SECONDS_PUMP,
  PUMP_POWER,
  SOIL_SENSOR_SET,
  LIGHT_SENSOR_SET,
  CALIBRATE_SOIL_SENSOR,
  SAVE,
  CALIBRATE_LIGHT_SENSOR
};

// Start the app in HOME
AppState appState = HOME;
SettingsState settingsState = FREQUENCY;
// Which pump is being setup in the settings
// Current pump in settings
uint8_t pumpIdxSettings = 0;
// Current pump showing info - home screen
byte pumpIdxHome = 0;

Pump pumps[NUM_PUMPS] = {
    Pump(PUMP_01),
    Pump(PUMP_02),
    Pump(PUMP_03),
};
// List of Soil sensors
const uint8_t soilSensorsPins[NUM_PUMPS] = {
  SOIL_SENSOR_01,
  SOIL_SENSOR_02,
  SOIL_SENSOR_03,
};

boolean sleeping = false;

/**
  Sensor information
*/
SensorData sensorData;
SensorConfig sensorConfig;

/**
 * Avoid interference at the buttons when clicked - debounce them
 */
#define DEBOUNCE_DELAY_MS 200
/**
 * Debounce the buttons
 */
uint32_t lastDebounceTimeMs = 0;

/**
   DATA INITIALIZATION
*/
// Initialize display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Initialize temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

char lineBuffer[22];

bool anyPumpIsRunning()
{
  for (uint8_t i = 0; i < NUM_PUMPS; i++)
  {
    if (pumps[i].isRunning())
    {
      return true;
    }
  }
  return false;
}

/**
  Set the state to PUMPING and start the water pump.
  */
void startPump(uint8_t pumpIdx)
{
  Pump *pump = &pumps[pumpIdx];
  pump->setStartedAtMs(millis());
  pump->setRunning(true);
}

/**
  Set the state to PUMPING and stop the water pump.
  */
void stopPump(uint8_t pumpIdx)
{
  Pump *pump = &pumps[pumpIdx];
  pump->setRunning(false);
}

void startAllPumps()
{
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++)
  {
    Pump *pump = &pumps[pumpIdx];
    pump->setLastRunMs(millis());
    startPump(pumpIdx);
  }
}

void stopAllPumps()
{
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++)
  {
    stopPump(pumpIdx);
  }
}

void readSensors()
{
  sensorData.temperature = int(dht.readTemperature());
  sensorData.humidity = int(dht.readHumidity());

  int sensorRead;

  sensorRead = analogRead(LIGHT_SENSOR);
  sensorRead = map(sensorRead, sensorConfig.lightSensorNightValue, sensorConfig.lightSensorDayValue, 0, 100);
  sensorRead = constrain(sensorRead, 0, 100);
  sensorData.light = filterNoise(sensorData.light, sensorRead);

  for (uint8_t i = 0; i < NUM_PUMPS; i++)
  {
    sensorRead = analogRead(soilSensorsPins[i]);
    sensorRead = map(sensorRead, sensorConfig.soilSensorDryValue[i], sensorConfig.soilSensorWetValue[i], 0, 100);
    sensorRead = constrain(sensorRead, 0, 100);
    sensorData.soilMoisture[i] = filterNoise(sensorData.soilMoisture[i], sensorRead);
  }
}

void printCenterH(const char *text, uint8_t size, int16_t x, int16_t y)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  display.setCursor(x1 + (((SCREEN_WIDTH - x1) / 2) - (w / 2)), y1);
  display.print(text);
}

void printCenterH(const __FlashStringHelper *text, uint8_t size, int16_t x, int16_t y)
{
  strcpy_P(lineBuffer, (const char *)text);
  printCenterH(lineBuffer, size, x, y);
}

void header(int8_t temp, uint8_t humid, uint8_t light)
{
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 3);
  sprintf_P(lineBuffer, PSTR("%+2dC"), temp);
  display.print(lineBuffer);

  display.setCursor(104, 3);
  sprintf_P(lineBuffer, PSTR("%3d%%"), humid);
  display.print(lineBuffer);

  display.setCursor(49, 3);
  sprintf_P(lineBuffer, PSTR("%c%3d%%"), 0x0f, light);
  display.print(lineBuffer);

  display.drawFastHLine(0, 15, SCREEN_WIDTH, WHITE);
}

void footer(const __FlashStringHelper *b1, const __FlashStringHelper *b2, const __FlashStringHelper *b3)
{
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.fillRect(0, 54, SCREEN_WIDTH, 10, WHITE);

  display.setCursor(2, 55);
  display.print(b1);

  printCenterH(b2, 1, 0, 55);

  display.setCursor(SCREEN_WIDTH - (strlen_P((const char *)b3) * 6) - 2, 55);
  display.print(b3);

  display.setTextColor(WHITE);
}

#define BAR_SIZE 32

void body(Pump pump, uint8_t pumpIdx, SensorData sensorData)
{
  auto pumpConfig = pump.getConfig();

  display.setCursor(0, 24);
  display.setTextSize(3);
  display.print(F("P"));
  display.print(pumpIdx + 1);

  if (pump.isRunning())
  {
    display.setCursor(40, 22);
    display.setTextSize(1);
    display.print(F("...RUNNING..."));
    uint32_t configPumpMs = ((uint32_t)pumpConfig.secondsPump) * 1000ul;
    uint32_t elapsedMs = millis() - pump.getStartedAtMs();
    int secsLeft = int((configPumpMs - elapsedMs) / 1000ul);
    sprintf_P(lineBuffer, PSTR("%03dsecs left"), secsLeft);
    display.setCursor(40, 38);
    display.setTextSize(1);
    display.print(lineBuffer);
    // display.drawBitmap(107, 24, epd_bitmap_fan, 20, 20, WHITE);
    footer(F(""), F("Cancel"), F("Next"));
  }
  else
  {
    // Light Sensor
    display.setTextSize(1);
    display.setCursor(40, 22);
    display.write(0x0f);
    display.drawRoundRect(48, 22, BAR_SIZE, 7, 4, WHITE);
    display.fillRoundRect(48, 22, (BAR_SIZE * sensorData.light) / 100, 7, 4, WHITE);
    display.drawFastVLine(48 + (BAR_SIZE * pumpConfig.lightSensor) / 100, 19, 13, WHITE);
    // Soil Sensor
    display.setTextSize(1);
    display.setCursor(88, 22);
    display.write(0xef);
    display.drawRoundRect(95, 22, BAR_SIZE, 7, 4, WHITE);
    display.fillRoundRect(95, 22, (BAR_SIZE * sensorData.soilMoisture[pumpIdx]) / 100, 7, 4, WHITE);
    display.drawFastVLine(95 + (BAR_SIZE * pumpConfig.soilSensor) / 100, 19, 13, WHITE);

    display.setTextSize(1);
    display.setCursor(40, 35);
    sprintf_P(lineBuffer, PSTR("%02dh%02dmin"), pump.secondsToNextRun(millis()) / 60 / 60, (pump.secondsToNextRun(millis()) / 60) % 60);
    display.print(lineBuffer);
    sprintf_P(lineBuffer, PSTR("%03dsecs"), pumpConfig.secondsPump);
    display.setCursor(40, 45);
    display.print(lineBuffer);
    if (sensorData.soilMoisture[pumpIdx] > pumpConfig.soilSensor)
    {
      display.drawBitmap(95, 37, epd_bitmap_plant_good, 13, 13, WHITE);
    }
    else
    {
      display.drawBitmap(95, 37, epd_bitmap_plant_dry, 13, 13, WHITE);
    }
    if (sensorData.light > pumpConfig.lightSensor)
    {
      display.drawBitmap(112, 40, epd_bitmap_sun, 9, 9, WHITE);
    }
    else
    {
      display.drawBitmap(112, 40, epd_bitmap_moon, 8, 8, WHITE);
    }
    footer(F("Set."), F("Run"), F("Next"));
  }
}

/**
  Render the Home screen.
  Show the current time, weekday, temperature, humidity and if should run.
*/
void renderHome()
{
  Pump pump = pumps[pumpIdxHome];
  header(sensorData.temperature, sensorData.humidity, sensorData.light);
  body(pump, pumpIdxHome, sensorData);
}

/**
  Render settings screen. For each SettingsState, render different information.
  */
void renderSettings()
{
  Pump *pump = &pumps[pumpIdxSettings];

  display.setTextColor(WHITE);
  printCenterH(F("SETUP"), 2, 0, 0);
  display.drawFastHLine(0, 15, SCREEN_WIDTH, WHITE);

  if (settingsState != CALIBRATE_LIGHT_SENSOR)
  {
    display.setCursor(0, 24);
    display.setTextSize(3);
    display.print(F("P"));
    display.print(pumpIdxSettings + 1);
  }

  switch (settingsState)
  {
  case FREQUENCY:
    printCenterH(F("Frequency"), 1, 24, 20);
    sprintf_P(lineBuffer, PSTR("%02dh%02dmin"), pump->getConfig().frequency / 60, pump->getConfig().frequency % 60);
    printCenterH(lineBuffer, 1, 24, 35);
    footer(F("next"), F("+"), F("-"));
    break;
  case SECONDS_PUMP:
    printCenterH(F("Seconds Pump"), 1, 24, 20);
    sprintf_P(lineBuffer, PSTR("%03dsecs"), pump->getConfig().secondsPump);
    printCenterH(lineBuffer, 1, 24, 35);
    footer(F("next"), F("+"), F("-"));
    break;
  case PUMP_POWER:
    printCenterH(F("Pump Power"), 1, 24, 20);
    sprintf_P(lineBuffer, PSTR("%03d%%"), pump->getConfig().power);
    printCenterH(lineBuffer, 1, 24, 35);
    footer(F("next"), F("+"), F("-"));
    break;
  case SOIL_SENSOR_SET:
    printCenterH(F("Water if soil"), 1, 28, 20);
    printCenterH(F("below"), 1, 28, 30);
    sprintf_P(lineBuffer, PSTR("%c%3d%%"), 0xef, pump->getConfig().soilSensor);
    printCenterH(lineBuffer, 1, 28, 40);
    footer(F("next"), F("+"), F("-"));
    break;
  case LIGHT_SENSOR_SET:
    printCenterH(F("Water if light"), 1, 28, 20);
    printCenterH(F("above"), 1, 28, 30);
    sprintf_P(lineBuffer, PSTR("%c%3d%%"), 0x0f, pump->getConfig().lightSensor);
    printCenterH(lineBuffer, 1, 28, 40);
    footer(F("next"), F("+"), F("-"));
    break;
  case CALIBRATE_SOIL_SENSOR:
    printCenterH(F("Soil Calib."), 1, 28, 20);
    sprintf_P(lineBuffer, PSTR("Dry: %4d"), sensorConfig.soilSensorDryValue[pumpIdxSettings]);
    printCenterH(lineBuffer, 1, 28, 30);
    sprintf_P(lineBuffer, PSTR("Wet: %4d"), sensorConfig.soilSensorWetValue[pumpIdxSettings]);
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
    sprintf_P(lineBuffer, PSTR("Day: %4d"), sensorConfig.lightSensorDayValue);
    printCenterH(lineBuffer, 1, 0, 30);
    sprintf_P(lineBuffer, PSTR("Night: %4d"), sensorConfig.lightSensorNightValue);
    printCenterH(lineBuffer, 1, 0, 40);
    footer(F("save"), F("day"), F("night"));
    break;
  }
}

/**
  Main method to render the screen. Render according to AppState.
  */
void render()
{
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();

  switch (appState)
  {
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

void rotateSettings()
{
  // Still setting pump values - advance
  if (settingsState < SAVE)
  {
    settingsState = (SettingsState)(((uint8_t)settingsState) + 1);
  }
  else if (settingsState == SAVE && pumpIdxSettings < NUM_PUMPS - 1)
  {
    // Still some pumps left to setup, advance the pumpIdx and start again
    settingsState = FREQUENCY;
    pumpIdxSettings++;
  }
  else if (settingsState != CALIBRATE_LIGHT_SENSOR)
  {
    // If there is no more pump to set, move to the calibrate light sensor
    settingsState = CALIBRATE_LIGHT_SENSOR;
  }
  else
  {
    // after all move to home
    appState = HOME;
  }
}

/**
  Main button press action. Should switch AppState and SettingsState
  Debounced to avoid any noise.
  */
void btn1Press()
{
  if (appState == SETTINGS)
  {
    if (settingsState == SAVE)
    {
      // User didn't save. Read again the configuration from EEPROM
      loadEEPROM(pumps, NUM_PUMPS, &sensorConfig);
    }
    else if (settingsState == CALIBRATE_LIGHT_SENSOR)
    {
      saveEEPROM(pumps, NUM_PUMPS, sensorConfig);
    }
    rotateSettings();
  }
  else
  {
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
void btn2Press()
{
  if (appState == SETTINGS)
  {
    Pump *pump = &pumps[pumpIdxSettings];
    switch (settingsState)
    {
    case FREQUENCY:
      pump->incFrequency(true);
      break;
    case SECONDS_PUMP:
      pump->incSecondsPump(true);
      break;
    case PUMP_POWER:
      pump->incPumpPower(true);
      break;
    case SOIL_SENSOR_SET:
      pump->incSoilSensor(true);
      break;
    case LIGHT_SENSOR_SET:
      pump->incLightSensor(true);
      break;
    case CALIBRATE_SOIL_SENSOR:
      sensorConfig.soilSensorDryValue[pumpIdxSettings] = analogRead(soilSensorsPins[pumpIdxSettings]);
      break;
    case CALIBRATE_LIGHT_SENSOR:
      sensorConfig.lightSensorDayValue = analogRead(LIGHT_SENSOR);
      break;
    case SAVE:
      // User saved the config
      saveEEPROM(pumps, NUM_PUMPS, sensorConfig);
      rotateSettings();
      break;
    }
  }
  else if (appState == HOME)
  {
    if (pumps[pumpIdxHome].isRunning())
    {
      stopPump(pumpIdxHome);
    }
    else
    {
      startPump(pumpIdxHome);
    }
  }
  else
  {
    appState = HOME;
  }
}

void btn3Press()
{
  if (appState == SETTINGS)
  {
    Pump *pump = &pumps[pumpIdxSettings];
    switch (settingsState)
    {
    case FREQUENCY:
      pump->incFrequency(false);
      break;
    case SECONDS_PUMP:
      pump->incSecondsPump(false);
      break;
    case PUMP_POWER:
      pump->incPumpPower(false);
      break;
    case SOIL_SENSOR_SET:
      pump->incSoilSensor(false);
      break;
    case LIGHT_SENSOR_SET:
      pump->incLightSensor(false);
      break;
    case CALIBRATE_SOIL_SENSOR:
      sensorConfig.soilSensorWetValue[pumpIdxSettings] = analogRead(soilSensorsPins[pumpIdxSettings]);
      break;
    case CALIBRATE_LIGHT_SENSOR:
      sensorConfig.lightSensorNightValue = analogRead(LIGHT_SENSOR);
      break;
    case SAVE:
      appState = HOME;
      break;
    }
  }
  else if (appState == HOME)
  {
    pumpIdxHome = (pumpIdxHome + 1) % NUM_PUMPS;
  }
}

/**
  Check if, according to the configuration and current datetime, should start the water pump.
*/
void checkSchedule()
{
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++)
  {
    Pump *pump = &pumps[pumpIdx];
    if (!pump->isRunning() && pump->isTimeToRun(millis(), sensorData, pumpIdx))
    {
      startPump(pumpIdx);
    }
    else if (pump->isRunning())
    {
      uint32_t pumpMs = (uint32_t)pump->getConfig().secondsPump * 1000ul;
      if ((uint32_t)(millis() - pump->getStartedAtMs()) >= pumpMs)
      {
        stopPump(pumpIdx);
      }
    }
  }
}

/**
  Run the Pumps in case they are activated
  */
void runPumps()
{
  for (uint8_t pumpIdx = 0; pumpIdx < NUM_PUMPS; pumpIdx++)
  {
    Pump *pump = &pumps[pumpIdx];
    if (pump->isRunning())
    {
      analogWrite(pump->getPin(), (255 * pump->getConfig().power) / 100);
    }
    else
    {
      analogWrite(pump->getPin(), 0);
    }
  }
}

void setup()
{

  // Init the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    for (;;)
      ;
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

  for (uint8_t idx = 0; idx < NUM_PUMPS; idx++)
  {
    pinMode(pumps[idx].getPin(), OUTPUT);
    pinMode(soilSensorsPins[idx], INPUT);
  }

  cli();
  // Set PIN On Change Interrupts
  PCICR = 0b00000100;
  PCMSK2 = 0b00111000;

  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  sei();

  // saveEEPROM();
  loadEEPROM(pumps, NUM_PUMPS, &sensorConfig);
}

ISR(PCINT2_vect)
{
}

void loop()
{
  static volatile uint8_t isSleeping = 0;
  uint32_t currentMillis = millis();

  readSensors();
  checkSchedule();
  runPumps();

  wdt_reset();

  // Enter sleep mode after SLEEP_TIME and if no pump is active
  isSleeping = (lastDebounceTimeMs + SLEEP_TIME < currentMillis) && (!anyPumpIsRunning());

  if (!isSleeping)
  {
    int btn1State = digitalRead(BTN_1);
    int btn2State = digitalRead(BTN_2);
    int btn3State = digitalRead(BTN_3);

    if (uint32_t(currentMillis - lastDebounceTimeMs) >= DEBOUNCE_DELAY_MS)
    {
      lastDebounceTimeMs = millis();
      if (!isSleeping)
      {
        if (btn1State == LOW)
        {
          btn1Press();
        }
        else if (btn2State == LOW)
        {
          btn2Press();
        }
        else if (btn3State == LOW)
        {
          btn3Press();
        }
      }
    }
    render();
  }
  else
  {
    display.clearDisplay();
    display.display();
    wdt_disable();
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    wdt_enable(WDTO_1S);
  }
}
