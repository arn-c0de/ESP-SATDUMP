#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// SD Card Pins (MISO=12, MOSI=14, SCK=22, CS=13)
#define SD_MISO_PIN  12
#define SD_MOSI_PIN  14
#define SD_SCK_PIN   22
#define SD_CS_PIN    13

SPIClass sdSPI(HSPI);

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- SD Card Test (MISO 12, MOSI 14) ---");
    Serial.printf("MOSI: %d, MISO: %d, SCK: %d, CS: %d\n", SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN, SD_CS_PIN);

    // Initialize SPI
    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, -1);

    Serial.println("Running power-on sequence...");
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(50);

    // SD power-on: ≥74 clocks with CS HIGH
    sdSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for (uint8_t i = 0; i < 20; i++) sdSPI.transfer(0xFF);
    sdSPI.endTransaction();
    delay(20);

    // Diagnostic: CMD0 Reset
    Serial.println("Sending CMD0 (Reset)...");
    digitalWrite(SD_CS_PIN, LOW);
    delay(1);
    uint8_t cmd0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    sdSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for(int i=0; i<6; i++) sdSPI.transfer(cmd0[i]);
    uint8_t response = 0xFF;
    for(int i=0; i<10 && response == 0xFF; i++) response = sdSPI.transfer(0xFF);
    sdSPI.endTransaction();
    digitalWrite(SD_CS_PIN, HIGH);
    Serial.printf("CMD0 Response: 0x%02X\n", response);

    Serial.println("Initializing SD.begin...");
    if (SD.begin(SD_CS_PIN, sdSPI, 400000)) {
        Serial.println("SUCCESS!");
        Serial.printf("Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    } else {
        Serial.println("FAILED!");
    }
}

void loop() { delay(1000); }
