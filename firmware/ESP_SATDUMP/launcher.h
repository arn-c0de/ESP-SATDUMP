#pragma once

// Check activation condition and run launcher menu if triggered.
// Returns immediately if launcher is not activated.
void launcherCheckAndRun();

// Set NVS boot flag and reboot — launcher menu will appear on next boot.
void launcherRebootToLauncher();
