# Power Optimization Suggestions

Current estimated power consumption: **~180–300 mA** (varies with LCD voltage and BLE activity)
Target: **~50–90 mA** (at least 50% reduction)

## Quick Wins (minimal code changes)

### 1. LCD on-demand — turn off backlight between updates

The display only needs to show new data once per minute (task3). Turning off the LCD
backlight between updates saves **~120–200 mA** for ~55 seconds out of every 60.

**Implementation sketch:**
```cpp
// In task3, after drawing:
M5.Lcd.sleep();                    // Turn off LCD
// When touch detected (task1), or at next update:
M5.Lcd.wakeup();
M5.Axp.SetLcdVoltage(3000);
```

With LCD off 55/60 seconds: average savings ~110–180 mA.

### 2. Reduce CPU frequency to 80 MHz

ESP32 runs at 240 MHz by default. This application (BLE scan + text display) does not
need that speed. 80 MHz is sufficient and cuts CPU current from ~50 mA to ~20 mA.

**Implementation:** In `setup()`, before BLE init:
```cpp
setCpuFrequencyMhz(80);   // or 160 as a middle ground
```

### 3. Reduce BLE scan time

Current scan is 15 seconds every 3 minutes. The callback typically finds the RuuviTag
within 1–3 seconds. Reducing to 5 seconds saves scan power without missing packets.

**Implementation:** In task2:
```cpp
BLEScanResults foundDevices = ruuviScan->start(5, false);  // was 15
```

### 4. Disable Serial output after debug

`Serial.print()` keeps the UART peripheral active (~5–10 mA). After initial debug info
in `setup()`, the Serial console is only used by `taskMonitor()` every 62 seconds.

**Implementation:** Either remove Serial calls entirely, or:
```cpp
if (Serial) Serial.end();  // after setup debug output
```

## Medium Effort

### 5. CPU light sleep between task cycles

ESP32's `light_sleep` pauses the CPU between FreeRTOS ticks. Combined with idle tasks
yielding, this can reduce baseline current from ~50 mA to ~10–15 mA. Requires enabling
in menuconfig or calling `esp_light_sleep_start()` judiciously.

### 6. Combine task cycles into a single wake-schedule

Currently tasks 3, 4, and 5 all run on different timers. They could be merged so the
device wakes once per minute, does everything, then sleeps. This allows deeper idle
between cycles.

## High Impact — Deep Sleep (most complex)

### 7. Deep sleep between measurements

**Estimated average current: ~5–15 mA** (95%+ reduction from current)

Flow:
1. Enter deep sleep for 5 minutes
2. RTC timer wakes ESP32
3. Re-init BLE, scan for RuuviTag (~3 s)
4. Update display (~2 s)
5. Back to deep sleep

**Trade-offs:**
- Slower touch response (touch would need to wake from deep sleep via RTC GPIO)
- Slightly longer library re-init on each wake
- Best battery life (weeks on a single charge)

### 8. Use UltraLowPower (ULP) coprocessor for periodic BLE

ESP32 has a ULP coprocessor that can run during deep sleep. While it cannot run the
BLE controller directly, it can wake the main CPU only when a GPIO event occurs.

## Recommended First Steps

For a **reliable 50%+ reduction** with minimal code risk:

| Step | Savings | Complexity |
|------|---------|------------|
| 1. LCD off between updates | ~150 mA | Low |
| 2. CPU 80 MHz | ~30 mA | Low |
| 3. BLE scan 5 s instead of 15 | ~10 mA | Low |
| 4. Disable Serial | ~5 mA | Low |
| **Total estimated** | **~195 mA** | → ~50–70 mA |

If more savings are needed after these steps, consider deep sleep (option 7).
