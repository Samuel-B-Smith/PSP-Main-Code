/* PSP Payload Prometheus Relaunch Mission Code
 * Verifies that chip is in write mode. If switch is not set to write, code does not continue
 * Gathers data from 4 Wheatstone Bridge chips on pins A0-A3 at a rate of 4 samples per second
 * Writes Data from Wheatstone bridges on to 2 seperate EEPROM Chips using A4 and A5 pins
 * Writes to chip 1's address 50 until filled before moving to chip 2 at address of 51
 * Stops writing data when both chips are full
 * 
 * Author(s):
 * Samuel Brice Smith
 * smit4344@purdue.ecu
 * 
 * Electronics assistance provided by:
 * Andrew Smith
 * andrewsmithcu@gmail.com
 * 
 * 
 * 
 */

#include <arduino.h>
#include <Wire.h>

#define EEPROM1_ADDRESS 0x50 // First EEPROM
#define EEPROM2_ADDRESS 0x51 // Second EEPROM
#define EEPROM_SIZE 64000    // 64KB = 65536 bytes
#define DATA_SIZE 4          // Number of analog inputs
#define READ_INTERVAL 250    // 0.25 seconds

int currentAddress1 = 0; // Current address for EEPROM 1
int currentAddress2 = 0; // Current address for EEPROM 2
int writeSwitch = 8; // Write protection switch

void setup() {
    Wire.begin();
    Serial.begin(9600);
}

void loop() {
    // Read analog values from A0, A1, A2, A3
    byte data[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = analogRead(A0 + i) / 4; // Scale to 0-255 for EEPROM
    }

    // Write data to EEPROM
    if (currentAddress1 < EEPROM_SIZE) {
        writeEEPROM(EEPROM1_ADDRESS, currentAddress1, data);
        currentAddress1++;
    } else if (currentAddress2 < EEPROM_SIZE) {
        writeEEPROM(EEPROM2_ADDRESS, currentAddress2, data);
        currentAddress2++;
    } else {
        // Both EEPROMs are full, stop writing
        Serial.println("Both EEPROMs are full. Stopping data collection.");
        while (true); // Stop further execution
    }

    // Delay for 0.25 seconds
    delay(READ_INTERVAL);
}

void writeEEPROM(int eepromAddress, int address, byte* data) {
    Wire.beginTransmission(eepromAddress);
    Wire.write((int)(address >> 8));   // Send high byte
    Wire.write((int)(address & 0xFF)); // Send low byte
    for (int i = 0; i < DATA_SIZE; i++) {
        Wire.write(data[i]);            // Send each byte of data
    }
    Wire.endTransmission();
    delay(5); // Wait for EEPROM to write
}
