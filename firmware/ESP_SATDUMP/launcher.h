#pragma once

/**
 * Multi-OS Launcher
 *
 * Activation:
 *   - Hold BTN_ROT_PIN (GPIO26) low during boot, OR
 *   - NVS boot flag set by launcherRebootToLauncher()
 *
 * SD card wiring (VSPI — separate from TFT HSPI):
 *   SD_MOSI → GPIO13   SD_MISO → GPIO21
 *   SD_SCK  → GPIO22   SD_CS   → GPIO27
 *   Firmware .bin files go in /firmware/ on a FAT32 SD card.
 *
 * Call launcherCheckAndRun() in setup() after displayInit() and encoderInit().
 * Call launcherRebootToLauncher() from anywhere to reboot into the OS menu.
 */

// Check activation condition and run launcher menu if triggered.
// Returns immediately if launcher is not activated.
void launcherCheckAndRun();

// Set NVS boot flag and reboot — launcher menu will appear on next boot.
void launcherRebootToLauncher();
