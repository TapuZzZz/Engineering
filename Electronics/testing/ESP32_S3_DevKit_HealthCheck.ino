// =================================================================
//  ESP32-S3 DEVKIT  -  GENERAL BOARD HEALTH CHECK
//  Checks the board/chip itself - NOT the wired components (those
//  already have their own TEST_*.ino files). This answers: "is the
//  ESP32-S3 chip, its flash, its PSRAM, and its WiFi radio actually
//  healthy?" - useful given the earlier boot-loop issue.
//
//  WHAT IT CHECKS:
//    - Chip model, revision, core count
//    - Flash size and speed (as the chip actually detects it -
//      compare against what Tools > Flash Size says you selected)
//    - PSRAM presence and size (compare against Tools > PSRAM)
//    - Free heap memory
//    - MAC address
//    - Reset reason (WHY did it last reboot - useful for catching
//      brownouts/watchdog resets/crashes after the fact)
//    - A WiFi network scan, to confirm the radio itself works,
//      independent of any specific network credentials
// =================================================================

#include <WiFi.h>
#include "esp_system.h"

void printResetReason() {
  esp_reset_reason_t reason = esp_reset_reason();

  Serial.print("Last reset reason: ");
  switch (reason) {
    case ESP_RST_POWERON:   Serial.println("POWERON (normal power-up)"); break;
    case ESP_RST_EXT:       Serial.println("EXT (external reset pin)"); break;
    case ESP_RST_SW:        Serial.println("SW (software reset, e.g. ESP.restart())"); break;
    case ESP_RST_PANIC:     Serial.println("PANIC (crash/exception!)"); break;
    case ESP_RST_INT_WDT:   Serial.println("INT_WDT (interrupt watchdog timeout!)"); break;
    case ESP_RST_TASK_WDT:  Serial.println("TASK_WDT (task watchdog timeout!)"); break;
    case ESP_RST_WDT:       Serial.println("WDT (other watchdog timeout!)"); break;
    case ESP_RST_BROWNOUT:  Serial.println("BROWNOUT (power dipped below threshold!)"); break;
    case ESP_RST_SDIO:      Serial.println("SDIO"); break;
    default:                Serial.println("Unknown/other"); break;
  }

  if (reason == ESP_RST_BROWNOUT) {
    Serial.println("-> This confirms a power supply issue caused the last reset.");
  } else if (reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT) {
    Serial.println("-> This confirms a software crash/hang caused the last reset, not power.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("");
  Serial.println("=== ESP32-S3 DEVKIT - GENERAL BOARD HEALTH CHECK ===");
  Serial.println("");

  // --- Reset reason (check this FIRST - tells you why it's rebooting) ---
  printResetReason();
  Serial.println("");

  // --- Chip info ---
  Serial.print("Chip model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Chip revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("CPU cores: ");
  Serial.println(ESP.getChipCores());
  Serial.print("CPU frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.println("");

  // --- Flash ---
  Serial.print("Flash size (detected): ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
  Serial.println(" MB  <- compare against Tools > Flash Size selection");
  Serial.print("Flash speed: ");
  Serial.print(ESP.getFlashChipSpeed() / 1000000);
  Serial.println(" MHz");
  Serial.println("");

  // --- PSRAM ---
  if (psramFound()) {
    Serial.print("PSRAM: FOUND, size = ");
    Serial.print(ESP.getPsramSize() / (1024 * 1024));
    Serial.println(" MB  <- compare against Tools > PSRAM selection");
  } else {
    Serial.println("PSRAM: NOT FOUND");
    Serial.println("-> If you expect PSRAM on this board, check Tools > PSRAM > OPI PSRAM is enabled.");
  }
  Serial.println("");

  // --- Memory ---
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("Free heap (minimum ever seen since boot): ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.println(" bytes");
  Serial.println("");

  // --- MAC address ---
  Serial.print("WiFi MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("");

  // --- WiFi scan (confirms the radio itself works) ---
  Serial.println("Scanning for WiFi networks (radio health check)...");
  WiFi.mode(WIFI_STA);
  int networksFound = WiFi.scanNetworks();

  if (networksFound == 0) {
    Serial.println("RESULT: No networks found. If you're sure networks exist nearby,");
    Serial.println("this could indicate a WiFi radio/antenna problem.");
  } else {
    Serial.print("RESULT: WiFi radio OK - found ");
    Serial.print(networksFound);
    Serial.println(" network(s):");
    for (int i = 0; i < networksFound; i++) {
      Serial.print("  ");
      Serial.print(i + 1);
      Serial.print(". ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");
    }
  }

  Serial.println("");
  Serial.println("=== HEALTH CHECK COMPLETE ===");
}

void loop() {
  // Nothing to do - all checks run once in setup().
}
