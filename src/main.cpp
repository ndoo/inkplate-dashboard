#include "Inkplate.h"
#include "driver/rtc_io.h"
#include <ArduinoJson.h>
#include "Dashboard.h"
#include "ImageScreen.h"
#include "WiFi.h"
#include "config.h"
#include "SdFat.h"

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 30 * 60 // time to sleep in seconds

Inkplate display(INKPLATE_3BIT);
RTC_DATA_ATTR int selectedScreen = 0;

byte touchPadPin = 10;

// screens
Dashboard dashboard = Dashboard(display);
ImageScreen imageScreen = ImageScreen(display);

void connectWifi()
{
  display.print("Connecting to WiFi...");
  display.partialUpdate();

  //Connect to the WiFi network.
  int attempts = 0;
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED && attempts < 5)
  {
    attempts++;
    delay(500);
    display.print(".");
    display.partialUpdate();
  }

  // still not connected: reset wifi see issue: https://github.com/espressif/arduino-esp32/issues/2501
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi Connection could not be established. Will reset Wifi and try again.");
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(3000);

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Could still not connect to wifi.");
    }
    else
    {
      Serial.println("Connected to Wifi successfully.");
    }
  }
}

void displayScreen()
{
  connectWifi();
  display.clearDisplay();

  switch (selectedScreen)
  {
  case 0:
    dashboard.draw();
    break;
  case 1:
    imageScreen.draw();
    break;
  default:
    break;
  }
}

void nextScreen()
{
  display.clearDisplay();
  display.setTextColor(BLACK);
  display.setFont(&Roboto_Bold_36);
  DisplayHelpers::printCenteredText(display, "Loading Screen...", 400, 300);
  display.display();

  selectedScreen++;
  selectedScreen = selectedScreen % 2;
  displayScreen();
}

void setup()
{
  Serial.begin(9600);
  display.begin();

  if (!display.sdCardInit())
  {
    Serial.println("Could not initialize SD Card");
    return;
  }

  // Setup mcp interrupts
  // display.pinModeMCP(touchPadPin, INPUT);
  // display.setIntOutput(1, true, true, HIGH);
  // display.setIntPin(touchPadPin, RISING);

  // handle behaviour using wake up reason
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    nextScreen();
    break;
  default:
    displayScreen();
    break;
  }

  // send to deep sleep
  rtc_gpio_isolate(GPIO_NUM_12); //Isolate/disable GPIO12 on ESP32 (only to reduce power consumption in sleep)
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 1); // touchpad wake up
  esp_deep_sleep_start();
}

void loop()
{
  // nothing
}