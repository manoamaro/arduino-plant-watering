#include <EEPROM.h>
#include <Adafruit_PCD8544.h>
#include "DHT.h"
#include <TimeLib.h>

/**
  TEMPERATURE AND HUMIDITY SENSOR
*/
#define DHTPIN 4
#define DHTTYPE DHT22

#define PUMP 10

/**
  DISPLAY PINS
*/
#define DISPAY_SCLK 5
#define DISPAY_DIN  6
#define DISPAY_DC   7
#define DISPAY_CS   9
#define DISPAY_RST  8

/**
  BUTTONS PINS
*/
#define BTN_1  2
#define BTN_2  3

/**
   TYPES DEFINITION
*/

/**
  Configuration that will be saved into EEPROM
*/
struct Configuration {
  byte frequency; // byte representation: 0 Sun Mon Tue Wed Thr Fri Sat
  byte hourOfDay; // 0-24
  int secondsPump; // Time in sec to keep the pump running
};

/**
  State of the app
*/
enum AppState {
  HOME, // Show the home screen
  SETTINGS, // Show the Settings screen
  PUMPING // Pump activated
};

/**
  State of the Settings screen
*/
enum SettingsState {
  TIME_HOUR,
  TIME_MIN,
  WEEKDAY,
  FREQUENCY_SUN,
  FREQUENCY_MON,
  FREQUENCY_TUE,
  FREQUENCY_WED,
  FREQUENCY_THR,
  FREQUENCY_FRI,
  FREQUENCY_SAT,
  WHEN_HOUR_DAY,
  SECONDS_PUMP,
  SAVE
};


/**
   DATA INITIALIZATION
*/
// Initialize display
Adafruit_PCD8544 display = Adafruit_PCD8544(DISPAY_SCLK, DISPAY_DIN, DISPAY_DC, DISPAY_CS, DISPAY_RST);
// Initialize temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

// in memory Configuration
Configuration configuration = {
  B11111111, // Run everyday
  12, // Run at 12:00
  30, // 30 sec of pump running
};

// Start the app in HOME
AppState appState = HOME;
// First settings screen is HOUR
SettingsState settingsState = TIME_HOUR;
// Last time the pump started, in secs since Epoch
time_t pumpStarted = 0;


/**
  Render the Home screen.
  Show the current time, weekday, temperature, humidity and if should run.
*/
void renderHome() {
  float humTemp[2] = {0};
  char line[display.width()];

  dht.readTempAndHumidity(humTemp);

  // Time information
  display.setCursor(0, 0);
  sprintf(line, "%s      %02d:%02d\n", dayShortStr(weekday()), hour(), minute());
  display.write(line);

  // Humidity and Temperature
  String humidity = String(humTemp[0]);
  humidity.concat("%");
  String temperature = String(humTemp[1]);
  temperature.concat("C");
  display.setCursor(0,10);
  display.write(humidity.c_str());

  display.setCursor(0,20);
  display.write(temperature.c_str());

  renderWeekdays(2, 40, weekday() - 1);

  display.display();
}

/**
  Helper function to render the weekdays in blocks. Show if is active or not according to configuration
  */
void renderWeekdays(int x, int y, int today) {
  for (int i = 0; i < 7; i++) {
    bool isOn = (configuration.frequency & (1 << (6 - i))) > 0;
    display.fillRoundRect(x + i * 12, y, 7, 7, 1, BLACK);
    if (!isOn) {
      display.fillRoundRect(x + 1 + i * 12, y + 1, 5, 5, 1, WHITE);
    }
    if (today == i) {
      display.drawPixel(x + 3 + i * 12, y - 3, BLACK);
    }
  }
}

/**
  Render settings screen. For each SettingsState, render different information.
  */
void renderSettings() {
  char fmtTime[display.width()];
  char fmtWeekday[display.width()];
  char fmtHourOfDay[display.width()];
  char fmtSecondsPump[display.width()];

  sprintf(fmtTime, "    %02d:%02d    ", hour(), minute());
  sprintf(fmtWeekday, "  %s  ", dayStr(weekday()));
  sprintf(fmtHourOfDay, "    %02d:00    ", configuration.hourOfDay);
  sprintf(fmtSecondsPump, "%04d Seconds", configuration.secondsPump);

  display.println("---Settings---");

  switch (settingsState) {
    case TIME_HOUR:
      display.println("\n   Set hour   \n");
      display.println(fmtTime);
      break;
    case TIME_MIN:
      display.println("\n  Set minute  \n");
      display.println(fmtTime);
      break;
    case WEEKDAY:
      display.println("\n Set weekday  \n");
      display.println(fmtWeekday);
      break;
        case FREQUENCY_SUN:
        case FREQUENCY_MON:
        case FREQUENCY_TUE:
        case FREQUENCY_WED:
        case FREQUENCY_THR:
        case FREQUENCY_FRI:
        case FREQUENCY_SAT:
        display.println("Run on\n");
        display.println("Sun .    . Sat");
        renderWeekdays(2, 40, settingsState - 3);
      break;
    case WHEN_HOUR_DAY:
      display.println("Run at\n");
      display.println(fmtHourOfDay);
      break;
    case SECONDS_PUMP:
      display.println("Pump run time\n");
      display.println(fmtSecondsPump);
      break;
    case SAVE:
      display.println("\n\nSave Settings?");
      break;
    default:
      display.println(settingsState);
      break;
  }
  display.display();
}

/**
  Render Pumping screen, when the water pump is running.
  */
void renderPumping() {
  display.println("....PUMPING...");
  display.println("..............\n");
  char timeLeft[display.width()];
  sprintf(timeLeft, " %03dsecs left", configuration.secondsPump - (now() - pumpStarted));
  display.println(timeLeft);
  display.display();
}

/**
  Main method to render the screen. Render according to AppState.
  */
void render() {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.clearDisplay();

  switch (appState) {
    case HOME:
      renderHome();
      break;
    case SETTINGS:
      renderSettings();
      break;
    case PUMPING:
      renderPumping();
      break;
    default:
      break;
  }
}

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;
/**
  Main button press action. Should switch AppState and SettingsState
  Debounced to avoid any noise.
  */
void mainButtonPress() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (appState == SETTINGS) {
      if (settingsState >= SAVE) {
        appState = HOME;
        // Read again the configuration from EEPROM
        EEPROM.get(0, configuration);
      } else {
        // Rotate in settingsState
        settingsState = (SettingsState) (((int) settingsState) + 1);
      }
    } else {
      // Start SETTINGS screen
      appState = SETTINGS;
      settingsState = TIME_HOUR;
    }
    lastDebounceTime = millis();
  }
}

/**
  "Set" button press action, to confirm/change the settings.
  If in Home screen, activate the water pump for the defined amount of time.
  Debounced to avoid any noise.
  */
void setButtonPress() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    unsigned long oneDay = 24L * 60L * 60L;
    if (appState == SETTINGS) {
      switch(settingsState) {
        case TIME_HOUR:
          setTime((hour() + 1) % 24, minute(), second(), day(), month(), year());
          break;
        case TIME_MIN:
          setTime(hour(), (minute() + 1) % 60, second(), day(), month(), year());
          break;
        case WEEKDAY:
          adjustTime(oneDay);
          break;
        case FREQUENCY_SUN:
          configuration.frequency ^= 1 << 6;
          break;
        case FREQUENCY_MON:
          configuration.frequency ^= 1 << 5;
          break;
        case FREQUENCY_TUE:
          configuration.frequency ^= 1 << 4;
          break;
        case FREQUENCY_WED:
          configuration.frequency ^= 1 << 3;
          break;
        case FREQUENCY_THR:
          configuration.frequency ^= 1 << 2;
          break;
        case FREQUENCY_FRI:
          configuration.frequency ^= 1 << 1;
          break;
        case FREQUENCY_SAT:
          configuration.frequency ^= 1 << 0;
          break;
        case WHEN_HOUR_DAY:
          configuration.hourOfDay = (configuration.hourOfDay + 1) % 24;
          break;
        case SECONDS_PUMP:
          configuration.secondsPump = (configuration.secondsPump + 1) % 100;
          break;
        case SAVE:
          // save configuration into EEPROM
          EEPROM.put(0, configuration);
          appState = HOME;
          break;
      }
    } else if (appState == HOME) {
      startPump();
    } else if (appState == PUMPING) {
      appState = HOME;
    }
    lastDebounceTime = millis();
  }
}

/**
  Check if, according to the configuration and current datetime, should start the water pump.
*/
void checkSchedule() {
  int wday = weekday() - 1;

  bool isTimeOfDay = hour() == configuration.hourOfDay && minute() == 0 && second() < 5;
  bool isOn = (configuration.frequency & (1 << (6 - wday))) > 0;
  
  if (isOn && isTimeOfDay && appState != PUMPING) {
    startPump();
  }
}

/**
  Set the state to PUMPING and start the water pump.
  */
void startPump() {
    appState = PUMPING;
    pumpStarted = now();    
}

/** 
  Check if the current AppState is PUMPING, so it output HIGH to the water pump Pin.
  If not PUMPING, set LOW.
  */
void checkPump() {
  if (appState == PUMPING) {
    if ((now() - pumpStarted) < configuration.secondsPump) {
      analogWrite(PUMP, 255);
    } else {
      digitalWrite(PUMP, LOW);
      appState = HOME;
    }
  } else {
    digitalWrite(PUMP, LOW);
  }
}

void setup() {
  // Init temperature and humidity sensor
  dht.begin();
  // Init the display
  display.begin();
  display.setContrast(50);
  // Display logo to check if everyting ok
  display.display();
  delay(500);
  display.clearDisplay();

  // Setup pin modes
  pinMode(PUMP, OUTPUT);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);

  // Quick check if PUMP is running
  digitalWrite(PUMP, HIGH);
  delay(100);
  digitalWrite(PUMP, LOW);    
  
  // Attach interrupt to buttons
  attachInterrupt(digitalPinToInterrupt(BTN_1), mainButtonPress, LOW);
  attachInterrupt(digitalPinToInterrupt(BTN_2), setButtonPress, LOW);

  // Read configuration from EEPROM.
  EEPROM.get(0, configuration);
}

void loop() {
  render();
  checkSchedule();
  checkPump();
  delay(100);
}
