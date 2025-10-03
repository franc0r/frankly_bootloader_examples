# LED Status Indication

The RP2040 Pico bootloader uses the onboard LED (GPIO 25) to provide visual feedback about the current state.

## LED Patterns

### 1. Fast Blink (Auto-boot Wait)
**Pattern**: 250ms period (125ms on, 125ms off)  
**When**: First 2 seconds after boot  
**Meaning**: Bootloader is checking for a valid application and will auto-start if found

### 2. Fast Flash (Active Communication)
**Pattern**: 100ms period (50ms on, 50ms off)  
**When**: Data is being received or transmitted via USB CDC  
**Meaning**: Bootloader is actively communicating with the host

**Detection Logic**: 
- Communication timestamp updated when bytes received from USB CDC
- Considered "communicating" if data received within last 500ms
- Automatically switches to idle pattern after 500ms of no activity

### 3. Slow Blink (Idle)
**Pattern**: 2 second period (1s on, 1s off)  
**When**: No communication activity for 500ms  
**Meaning**: Bootloader is ready and waiting for commands

### 4. Off
**Pattern**: LED completely off  
**When**: Application firmware is running  
**Meaning**: Control has been transferred to the application

## Implementation Details

### Timer-Based Control
- Main timer runs at 100ms interval
- LED state managed by `FRANKLYBOOT_updateLED()` function
- Called from timer callback after auto-boot period expires

### Communication Tracking
```c
// Communication timestamp updated when data received
last_comm_time_ms = to_ms_since_boot(get_absolute_time());

// Check if communicating (within 500ms window)
bool is_communicating = (current_time - last_comm_time_ms) < 500;
```

### State Machine

```
[Boot] 
  ↓
[Fast Blink - 2s] → (Valid app?) → [Jump to App] → [LED Off]
  ↓                    
  (Timeout)
  ↓
[Check Communication]
  ├─ (Active) → [Fast Flash]
  └─ (Idle)   → [Slow Blink]
```

## User Experience

1. **Power On**: LED starts fast blinking
2. **No App Found**: After 2s, switches to slow blink (ready)
3. **Valid App Found**: After 2s, LED turns off (app running)
4. **USB Connected**: LED fast flashes during firmware update
5. **Update Complete**: Returns to slow blink after 500ms idle

## Code References

- `main.c:79` - Timer callback with LED logic
- `bootloader_api.cpp:262` - `FRANKLYBOOT_isCommunicating()`
- `bootloader_api.cpp:267` - `FRANKLYBOOT_updateLED()`
- `bootloader_api.cpp:167` - Communication timestamp update

## Benefits

1. **Clear Status**: Users can immediately see bootloader state
2. **Activity Indication**: Fast flash confirms data transfer
3. **Idle Detection**: Slow blink shows bootloader is ready
4. **Low Power**: LED off when application runs
5. **Diagnostic Aid**: Pattern helps debug connection issues

## Troubleshooting

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| LED stays in fast blink | No valid app | Flash application firmware |
| LED never fast flashes | USB not connected | Check USB cable and enumeration |
| LED stuck in slow blink | No commands sent | Start firmware update tool |
| LED flashes erratically | Communication errors | Check USB connection quality |
