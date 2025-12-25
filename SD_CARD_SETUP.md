# ESP32-CAM microSD Card Setup Guide

## 📋 Overview

Your ESP32-CAM now supports **microSD card storage** with a **4GB usage limit** (even with 8GB physical card).

---

## 🔌 microSD Card Module Wiring

### Physical Module

```
┌─────────────────────────────────┐
│  microSD Card Adapter Module    │
│  (Commonly available on Amazon) │
│                                  │
│  ┌────────────────────────────┐ │
│  │  microSD Slot  │ DIP Switch│ │
│  │  (Insert card) │ (Leave ON)│ │
│  └────────────────────────────┘ │
│                                  │
│  Pins (looking at module):       │
│  CS   → GPIO 13 (Chip Select)   │
│  MOSI → GPIO 2  (Master Out)    │
│  MISO → GPIO 16 (Master In)     │
│  CLK  → GPIO 17 (Clock)         │
│  GND  → GND (Ground)            │
│  3V3/5V → 5V (Power)           │
└─────────────────────────────────┘
```

### ESP32-CAM Pin Connections

```
ESP32-CAM (AI-Thinker) → microSD Module
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

From pins.h:

Pin 13  → SD_CS   (Chip Select)
Pin 2   → SD_MOSI (SPI Master Out, Slave In)
Pin 16  → SD_MISO (SPI Master In, Slave Out)
Pin 17  → SD_CLK  (SPI Clock)
GND     → GND     (Ground - common)
5V      → 5V      (Power)
```

---

## 📊 Configuration Settings

From `config.h`:

```cpp
// microSD Card Settings
#define SD_CARD_ENABLED 1                    // Enable SD card support
#define SD_MAX_SIZE_GB 4                     // Max usable storage: 4GB
#define SD_MAX_SIZE_BYTES (4UL * 1024 * 1024 * 1024)  // 4GB in bytes
#define SD_MOUNT_POINT "/sdcard"            // Mount point for SD card
#define SD_SPI_FREQUENCY 10000000           // 10 MHz SPI clock
```

---

## 🔧 Pin Definitions

From `pins.h`:

```cpp
#ifdef CAMERA_MODULE

// microSD Card (SPI Mode)
#define SD_CS 13    // Chip Select (GPIO 13)
#define SD_MOSI 2   // SPI MOSI (GPIO 2)
#define SD_MISO 16  // SPI MISO (GPIO 16)
#define SD_CLK 17   // SPI CLK  (GPIO 17)

#endif
```

---

## 📁 File System Layout

After initialization, SD card will have:

```
/sdcard/
├── storage/              (Created automatically)
│   ├── videos/          (Video storage)
│   ├── images/          (Image storage)
│   └── logs/            (System logs)
└── README.txt
```

---

## ⚠️ Storage Limit: 4GB

### How the 4GB Limit Works

1. **Physical Card**: 8GB microSD
2. **Usable Limit**: 4GB (enforced by software)
3. **Monitoring**: System checks every 5 seconds
4. **Warnings**:
   - ⚠️ At 80% (3.2GB) - Warning message sent
   - 🛑 At 100% (4GB) - Write operations blocked

### Status Messages

```
[SD] ⚠️ Storage warning: 85% full
[SD] Usable limit set to: 4 GB
⚠️⚠️⚠️ SD CARD: 4GB LIMIT REACHED ⚠️⚠️⚠️
Storage full - cannot write more data
```

---

## 💾 Code Integration

### Initialization Sequence

```cpp
1. SPI bus setup (GPIO 17, 2, 16, 13)
2. SD.begin() - Mount card
3. Create /storage directory
4. Initialize status tracking
5. Start 5-second monitoring interval
```

### Monitoring Loop

```cpp
Every 5 seconds:
  ├─ Check used bytes: SD.usedBytes()
  ├─ Verify against limit: 4GB
  ├─ Send warnings if 80%+ full
  └─ Block writes if 100% full
```

### Status Updates to Front ESP

```json
{
  "type": "alert",
  "source": "camera",
  "data": {
    "alert_type": "storage_full",
    "used_bytes": 4294967296,
    "max_bytes": 4294967296
  }
}
```

---

## 🛠️ Troubleshooting

### SD Card Not Detected

```
❌ SD card initialization failed!
```

**Check:**

- [ ] microSD card inserted correctly
- [ ] Wiring correct (GPIO 2, 13, 16, 17)
- [ ] Card formatted as FAT32
- [ ] 5V power supply stable
- [ ] SPI clock speed (10 MHz)

### Write Failures After 4GB

```
Storage full - cannot write more data
```

**Solution:**

1. Connect card to computer via USB reader
2. Backup important files
3. Format card or clear old files
4. Reset limit by changing `SD_MAX_SIZE_GB` in `config.h`

### Slow Write Speed

**Optimize:**

- Use Class 10 microSD card (minimum)
- Reduce SPI clock if errors occur
- Keep file writes sequential

---

## 📝 Serial Output Example

```
>>> Initializing microSD Card...
  [SD] Initializing SPI bus for SD card...
  [SD] Mounting SD card...
  [SD] ✓ SD card mounted successfully
  [SD] Physical card size: 8 GB
  [SD] Total space: 8 GB
  [SD] Free space: 7.8 GB
  [SD] Usable limit set to: 4 GB
  [SD] Creating /storage directory...
  [SD] SD card ready for use (4GB limit enforced)
```

---

## 🎯 Summary

| Feature            | Value       |
| ------------------ | ----------- |
| **Physical Card**  | 8GB microSD |
| **Usable Limit**   | 4GB         |
| **SPI Speed**      | 10 MHz      |
| **Mount Point**    | /sdcard     |
| **Check Interval** | 5 seconds   |
| **Warning Level**  | 80% (3.2GB) |
| **Full Level**     | 100% (4GB)  |

---

## Next Steps

1. ✅ Assemble microSD module to breadboard
2. ✅ Upload firmware with SD card support
3. ✅ Monitor serial output for initialization
4. ✅ Insert 8GB microSD card
5. ✅ Verify 4GB limit is enforced
6. ✅ Test file writing capability

Your ESP32-CAM is now ready with 4GB of storage! 🎉
