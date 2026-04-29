# Power Optimization Suggestions

Current estimated power consumption: **~180–300 mA** (varies with LCD voltage and BLE activity)
Target: **~30–60 mA** (70–85% reduction)

## Primary Approach: LCD off on battery, on-demand via touch

The LCD backlight consumes ~120–200 mA — roughly 60–70% of total power.
Core idea: **on battery, LCD is always off. Touch wakes it for 10 seconds.**

When user touches the screen:
1. Wake LCD, show latest data
2. Trigger a fresh BLE scan immediately
3. After 10 seconds of inactivity, sleep LCD again

### Implementation sketch

**New state variables:**
```cpp
bool displayActive = false;
unsigned long displayOffTime = 0;
bool immediateScan = false;
```

**Task 1 (touch) — modified:**
```cpp
void task1(void *pvParameters) {
  while (1) {
    M5.update();
    if (M5.Touch.changed && M5.Touch.points > 0) {
      if (!displayActive) {
        M5.Lcd.wakeup();              // Wake LCD
        M5.Axp.SetLcdVoltage(3000);   // Full brightness
        displayActive = true;
        immediateScan = true;         // Trigger BLE scan
        // Notify task2 to scan immediately
        xTaskNotifyGive(Handle_getTemperatureTask);
      }
      displayOffTime = millis() + 10000;  // Reset 10 s timer
      batteryCount = 0;
    }
    // Auto turn off after 10 seconds
    if (displayActive && millis() > displayOffTime) {
      M5.Lcd.sleep();
      displayActive = false;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
```

**Task 2 (BLE scan) — add immediate scan support:**
```cpp
void task2(void *pvParameters) {
  while (1) {
    // Normal periodic scan every 180 s
    BLEScanResults foundDevices = ruuviScan->start(5, false);
    ruuviScan->clearResults();

    // Wait for delay OR notification (touch-triggered scan)
    ulTaskNotifyTake(pdTRUE, 180000 / portTICK_PERIOD_MS);

    // If woken by touch: scan immediately again for fresh data
    if (immediateScan) {
      immediateScan = false;
      ruuviScan->start(5, false);
      ruuviScan->clearResults();
    }
  }
}
```

**Task 3 (display) — skip LCD ops when display is off:**
```cpp
void task3(void *pvParameters) {
  while (1) {
    drawGraph();       // Still updates temperature array
    showTemperature(); // Only draws sprites if display is active
    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
}
```

Add a flag check inside `showTemperature()` (and optionally `drawGraph()`):
```cpp
if (!displayActive) {
  temperature.deleteSprite();
  other.deleteSprite();
  return;  // or skip sprite creation entirely
}
```

**Task 4 (battery) — respect LCD sleep:**
```cpp
// Only SetLcdVoltage if display is active
if (displayActive) { ... adjust voltage ... }
```

### Estimated power savings

| State | Current | Duration | Weighted avg |
|-------|--------:|---------:|------------:|
| LCD off, CPU 80 MHz, BLE idle | ~25 mA | ~55 s/min | ~23 mA |
| LCD on (data shown) | ~170 mA | ~5 s/min | ~14 mA |
| BLE scanning (5 s) | ~50 mA | ~5 s/3 min | ~1.5 mA |
| **Estimated average** | | | **~38 mA** |

This is ~80% reduction from the current ~200 mA average.

### Corner cases

- **On USB power:** LCD could stay on permanently (charging flag in M5Stat)
- **First boot (fillCounter == 0):** Show logo briefly, then sleep LCD
- **BLE scan on touch:** If no RuuviTag found within 5 s, show stale data
  — still better than a blank screen

## Additional Optimizations (stack on top)

### CPU frequency: 80 MHz
ESP32 defaults to 240 MHz. This app only polls BLE and draws text:
```cpp
setCpuFrequencyMhz(80);   // in setup()
```
Saves ~30 mA.

### Shorter BLE scan
RuuviTag is typically found within 1–3 seconds. 5 s scan is sufficient:
```cpp
BLEScanResults foundDevices = ruuviScan->start(5, false);  // was 15
```

### Disable Serial after debug
UART peripheral draws ~5–10 mA. After `setup()` debug info:
```cpp
Serial.end();   // or remove Serial calls entirely
```

### Combined impact

| Optimization | Savings | Running total |
|-------------|--------:|--------------:|
| LCD off on battery, 10 s on touch | ~160 mA | ~40 mA |
| CPU 80 MHz | ~30 mA | ~28 mA |
| BLE 5 s scan | ~8 mA | ~25 mA |
| Serial off | ~5 mA | ~20 mA |

**Estimated final average: ~20–30 mA** — battery would last 8–12 hours
(on M5Core2's 1200 mAh battery) vs. current ~2–4 hours.

## Beyond: Deep Sleep

For **weeks of battery life**, the next step is deep sleep:
- RTC timer wakes ESP32 every 5 minutes
- Quick BLE scan + update display for 3 seconds
- Back to deep sleep
- Touch wake via RTC GPIO interrupt
- Estimated average: ~2–5 mA

This is a larger refactor — task-based architecture changes to a
wake-do-sleep cycle — but offers the best possible battery life.
