// Include libraries
#include <M5Core2.h>
#include <stdio.h>
#include <stdlib.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Include fonts
#include "BSComp_Bold30pt7b.h"
#include "BSComp_Bold28pt7b.h"
#include "BSComp_Bold24pt7b.h"
#include "BSComp_Bold20pt7b.h"
#include "BSComp_Bold12pt7b.h"
#include "BSComp_Bold10pt7b.h"

#include "BSComp_Book24pt7b.h"
#include "BSComp_Book20pt7b.h"
#include "BSComp_Book12pt7b.h"
#include "BSComp_Book10pt7b.h"

#include "BS_Light10pt7b.h"
#include "BS_Light12pt7b.h"
#include "BS_Light20pt7b.h"
#include "BS_Light24pt7b.h"

// Include logo
#include "CyberAlfame.h"

// Task handles
TaskHandle_t Handle_touchScreenTask;
TaskHandle_t Handle_getTemperatureTask;
TaskHandle_t Handle_drawGraphTask;
TaskHandle_t Handle_powerConsumptionTask;
TaskHandle_t Handle_statisticsTask;

// Bluetooth scanning parameters
BLEScan *ruuviScan;
String MAC_add = "de:3a:50:d2:d7:a3";
int aliveCount = 0;
int batteryCount = 0;

// RuuviTag data structure
struct ruuvi
{
  ruuvi()
      : temperature(0.0), humidity(0.0), pressure(0.0), voltage(0.0) {}
  float temperature;
  float humidity;
  float pressure;
  float voltage;
} ruuviTag;

// Power consumption statistics data structure
struct stat
{
  stat()
      : powerConsumption(0.0), batteryCapacity(0.0), USBVoltage(0.0), charging(0.0) {}
  float powerConsumption;
  float batteryCapacity;
  float USBVoltage;
  bool charging;
} M5Stat;

// TFT display objects
TFT_eSprite temperature = TFT_eSprite(&M5.Lcd);
TFT_eSprite other = TFT_eSprite(&M5.Lcd);
TFT_eSprite graph = TFT_eSprite(&M5.Lcd);
TFT_eSprite cover = TFT_eSprite(&M5.Lcd);

// Array to hold temperature data
float temperatures[285];
int fillCounter = 0;

// Function to convert hexadecimal string to integer
int hexToInt(String h)
{
  return (int)strtol(h.c_str(), 0, 16);
}

/**
 * Decodes raw data received from the RuuviTag
 * and arranges it in an Ruuvit struct of temperature, humidity, pressure, and voltage.
 */
void decodeRuuvi(String hex_data, int rssi)
{
  Serial.println(hex_data);
  if (hex_data.substring(4, 6) == "05")
  {
    ruuviTag.temperature = hexToInt(hex_data.substring(6, 10)) * 0.005;
    ruuviTag.humidity = hexToInt(hex_data.substring(10, 14)) * 0.0025;
    ruuviTag.pressure = hexToInt(hex_data.substring(14, 18)) * 1 + 50000;

    int voltage_power = hexToInt(hex_data.substring(30, 34));
    ruuviTag.voltage = (uint32_t)((voltage_power & 0x0b1111111111100000) >> 5) + 1600;
  }
}

// A class for handling callbacks from BLE devices that have been discovered
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  // The function that is called when a BLE device is discovered
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    // Check if the discovered device's MAC address is in the list of allowed MAC addresses
    if (MAC_add.indexOf(advertisedDevice.getAddress().toString().c_str()) >= 0)
    {
      // Serial.print("Device found ");
      // Serial.println(advertisedDevice.getAddress().toString().c_str());
      // Serial.println(advertisedDevice.getManufacturerData().data());
      // Convert the manufacturer data from the discovered device to a hex string and decode it
      String raw_data = String(BLEUtils::buildHexData(nullptr, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length()));
      raw_data.toUpperCase();
      decodeRuuvi(raw_data, advertisedDevice.getRSSI());
      // Device found. Stop BLE scanning
      advertisedDevice.getScan()->stop();
    }
  }
};

// Function for drawing a graph
void drawGraph()
{
  const int fillCounterMax = 285;
  int min = 100;
  int max = -100;
  int scale = 1;
  const int xMax = 285, yMax = 109;
  int val = 0;
  const int xShift = 6;
  const int yShift = 5;

  if (fillCounter < fillCounterMax)
  {
    temperatures[fillCounter] = (float)ruuviTag.temperature;
    for (int i = 0; i < fillCounter; i++)
    {
      if (temperatures[i] > max)
        max = temperatures[i];
      if (temperatures[i] < min)
        min = temperatures[i];
    }
  }
  else
  {
    for (int i = 0; i < (fillCounterMax - 1); i++)
    {
      temperatures[i] = temperatures[i + 1];
    }
    temperatures[284] = (int)ruuviTag.temperature;
    for (int i = 0; i < fillCounterMax; i++)
    {
      if (temperatures[i] > max)
        max = temperatures[i];
      if (temperatures[i] < min)
        min = temperatures[i];
    }
    // Reset the fill counter to the fill counter maximum
    fillCounter = 285;
  }

  min = min - 3;
  max = max + 3;
  scale = max - min; // for screen min and max values

  // Create a sprite for the graph and draw the graph on it
  graph.createSprite(320, 140);
  graph.fillSprite(WHITE);
  graph.setTextColor(BLACK, WHITE);
  graph.drawLine(7, 115, 299, 115, DARKGREY);
  graph.drawLine(7, 116, 299, 116, LIGHTGREY);
  graph.drawLine(291, 114, 291, 5, DARKGREY);
  graph.drawLine(290, 114, 290, 5, DARKGREY);
  graph.drawLine(291, 5, 299, 5, DARKGREY);
  graph.drawLine(291, 6, 299, 6, LIGHTGREY);

  for (int i = 0; i < fillCounter; i++)
  {
    if (i == 0)
    {
      graph.drawLine((i + 5), map((int)(temperatures[i] * 100.00), min * 100, max * 100, 114, 5), (i + 5), map((int)(temperatures[i] * 100.00), min * 100, max * 100, 114, 5), DARKGREY);
    }
    else
    {
      graph.drawLine((i + 4), map((int)(temperatures[i - 1] * 100.00), min * 100, max * 100, 114, 5), (i + 5), map((int)(temperatures[i] * 100.00), min * 100, max * 100, 114, 5), DARKGREY);
      // Serial.println(String(i + 4) + "," + String(map((int)temperatures[i - 1] * 100.00, min * 100, max * 100, 114, 5)) + "," + String(i + 5) + "," + String(map((int)temperatures[i] * 100.00, min * 100, max * 100, 114, 5)));
    }
  }

  // Serial.println("min:max:scale:value " + String(min) + ":" + String(max) + ":" + String(scale) + ":" + String(val));
  // Serial.println(String("Lämpötila ") + String((int)(temperatures[fillCounter] * 100.00)));
  graph.drawString(String((int)min), 295, 95, 2);
  graph.drawString(String((int)max), 295, 6, 2);
  graph.pushSprite(0, 100, GREEN);
  graph.deleteSprite();
  fillCounter++;
}

// This function displays temperature, humidity, pressure and power consumption data on the screen
void showTemperature()
{
  temperature.createSprite(200, 100);
  temperature.fillSprite(WHITE);
  temperature.setTextColor(BLACK, WHITE);
  temperature.drawString(String(ruuviTag.temperature) + String("°C"), 15, 10, 1);
  temperature.pushSprite(0, 0, GREEN);

  other.createSprite(120, 100);
  other.fillSprite(WHITE);
  other.setTextColor(BLACK, WHITE);
  other.drawString(String(ruuviTag.humidity) + String(" %"), 0, 10, 1);
  other.drawString(String((float)ruuviTag.pressure / 100) + String(" hPa"), 0, 38, 1);
  if (!M5Stat.charging)
  {
    other.drawString(String(M5Stat.powerConsumption) + String(" mW"), 0, 66, 1);
  }
  else
  {
    other.drawString(String(M5Stat.USBVoltage) + String(" V"), 0, 66, 1);
  }
  other.pushSprite(200, 0, GREEN);

  // Delete the sprites to free up memory
  temperature.deleteSprite();
  other.deleteSprite();
}

// This function displays the stack size of various tasks and the fill counter value
void taskMonitor()
{
  Serial.print("touchScreenTask (Stack size):\t\t");
  Serial.println(uxTaskGetStackHighWaterMark(Handle_touchScreenTask));
  Serial.print("getTemperatureTask (Stack size):\t");
  Serial.println(uxTaskGetStackHighWaterMark(Handle_getTemperatureTask));
  Serial.print("drawGraphTask (Stack size):\t\t");
  Serial.println(uxTaskGetStackHighWaterMark(Handle_drawGraphTask));
  Serial.print("powerConsumptionTask (Stack size):\t");
  Serial.println(uxTaskGetStackHighWaterMark(Handle_powerConsumptionTask));
  Serial.print("statisticsTask (Stack size):\t\t");
  Serial.println(uxTaskGetStackHighWaterMark(Handle_statisticsTask));
  Serial.print("Temperature:\t\t");
  Serial.println(String(ruuviTag.temperature) + String("°C"));
  Serial.print("Fill Counter:\t\t");
  Serial.println(String(fillCounter));
  Serial.println("--------------------------------------------");
}

// Task 1: A continuous loop that checks for touch input and updates the display.
void task1(void *pvParameters)
{
  const TickType_t xDelay = 200 / portTICK_PERIOD_MS;
  while (1)
  {
    M5.update();
    if (M5.Touch.changed)
    {
      if (M5.Touch.points > 0)
      {
        M5.Lcd.wakeup();
        M5.Axp.SetLcdVoltage(3000);
        batteryCount = 0;
      }
    }
    // if(M5.Axp.isCharging())
    //   M5.Lcd.println("Charging");
    vTaskDelay(xDelay); // wait for 200 ms
  }
}

// Task 2: A continuous loop that scans for RuuviTag BLE devices every 3 minutes
void task2(void *pvParameters)
{
  const TickType_t xDelay = (1000 * 180) / portTICK_PERIOD_MS; // 60 second
  int monitor = 0;
  while (1)
  {
    Serial.print("Task 2: scanning: ");
    Serial.println(aliveCount++);
    BLEScanResults foundDevices = ruuviScan->start(15, false); // 5 seconds, no duplicates
    Serial.print("Found devices: ");
    Serial.println(foundDevices.getCount());
    ruuviScan->clearResults(); // clear results
    vTaskDelay(xDelay);        // wait 30 seconds
  }
}

// Task 3: A continuous loop that updates the display with temperature and humidity data and draws a graph.
void task3(void *pvParameters)
{
  const TickType_t xDelay = (1000 * 60) / portTICK_PERIOD_MS;
  while (1)
  {
    if (fillCounter == 0)
    {
      delay(15000);
      M5.Lcd.fillScreen(WHITE);
    }
    // Serial.println("drawGraph");
    drawGraph();
    showTemperature();
    vTaskDelay(xDelay);
  }
}

// Task 4: A continuous loop that adjusts the LCD voltage based on the battery level and enters sleep mode when the battery is low.
void task4(void *pvParameters)
{
  const TickType_t xDelay = 333 / portTICK_PERIOD_MS;
  while (1)
  {
    if (M5Stat.charging)
    {
      M5.Axp.SetLcdVoltage(3300);
      batteryCount = 0;
    }
    else
    {
      // Smooth LCD voltage and sleep-mode handling
      if (batteryCount < 50)
      {
        M5.Axp.SetLcdVoltage(3000);
      }
      else if (batteryCount == 50)
      {
        M5.Axp.SetLcdVoltage(2800);
        // Serial.println(batteryCount);
      }
      else if (batteryCount == 200)
      {
        M5.Axp.SetLcdVoltage(2600);
      }
      else if (batteryCount > 500)
      {
        M5.Lcd.sleep();
        batteryCount = 500;
      }
    }
    batteryCount++;
    vTaskDelay(xDelay);
  }
}

// Task 5: A continuous loop that updates the power consumption and battery status and prints the statistics to Serial monitor
void task5(void *pvParameters)
{
  const TickType_t xDelay = (1000 * 62) / portTICK_PERIOD_MS;
  while (1)
  {
    M5Stat.powerConsumption = M5.Axp.GetBatPower();
    M5Stat.batteryCapacity = (M5.Axp.GetBatVoltage() < 3.2) ? 0 : (M5.Axp.GetBatVoltage() - 3.2) * 100;
    M5Stat.USBVoltage = M5.Axp.GetVBusVoltage();
    M5Stat.charging = M5.Axp.isCharging();
    taskMonitor();
    vTaskDelay(xDelay);
  }
}

// Initial setup method
void setup()
{
  M5.begin();                        // Init M5Core2
  M5.Axp.SetLcdVoltage(3000);        // Set LCD voltage
  M5.Lcd.fillScreen(WHITE);          // White background
  M5.Lcd.setTextColor(BLACK, WHITE); // Black font color

  // Set fonts
  temperature.setFreeFont(&BSComp_Bold30pt7b);
  other.setFreeFont(&BSComp_Book12pt7b);

  // Initialize temperature-array to -99
  for (int i = 0; i < 120; i++)
  {
    temperatures[i] = -99;
  }

  // Print sizes of variables to the serial monitor
  Serial.println(String("Size of ") + String(sizeof(CyberAlfame)));
  Serial.println(String("Size of ") + String(sizeof(*CyberAlfame)));
  Serial.println(configMINIMAL_STACK_SIZE);

  // Draw logo on the screen
  M5.Lcd.drawXBitmap(0, 0, CyberAlfame, 320, 240, BLACK, WHITE);
  delay(1000); // Wait for 1 second

  // Create a sprite for displaying text
  other.createSprite(120, 50);
  other.fillSprite(WHITE);
  other.setTextColor(BLACK, WHITE);
  other.drawString(String("IoT"), 0, 0, 1);
  other.pushSprite(140, 120, GREEN);

  // Set battery count to a negative value
  batteryCount = -200;

  // Initialize BLE device
  BLEDevice::init("");
  // Get BLE scan object
  ruuviScan = BLEDevice::getScan();
  // Set advertised device callbacks
  ruuviScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  // Set scan mode to active
  ruuviScan->setActiveScan(true);
  // Set scan interval
  ruuviScan->setInterval(100);
  // Set scan window
  ruuviScan->setWindow(99); // less or equal setInterval value

  // Creates tasks
  xTaskCreatePinnedToCore(task1, "touchScreenTask", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY, &Handle_touchScreenTask, 0);
  xTaskCreatePinnedToCore(task2, "getTemperatureTask", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY + 1, &Handle_getTemperatureTask, 0);
  xTaskCreatePinnedToCore(task3, "drawGraphTask", configMINIMAL_STACK_SIZE + 2048, NULL, tskIDLE_PRIORITY, &Handle_drawGraphTask, 0);
  xTaskCreatePinnedToCore(task4, "powerConsumptionTask", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY, &Handle_powerConsumptionTask, 0);
  xTaskCreatePinnedToCore(task5, "statisticsTask", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY, &Handle_statisticsTask, 0);
}

void loop()
{
}