/* PSP Payload Prometheus Relaunch Mission Code
 * Verifies that chip is in write mode. If switch is not set to write, code does not continue
 * Gathers data from 4 Wheatstone Bridge chips on pins A0-A3 at a rate of 4 samples per second
 * Writes Data from Wheatstone bridges on to 2 seperate EEPROM Chips using A4 and A5 pins
 * Writes to chip 1's address 50 until filled before moving to chip 2 at address of 51
 * Stops writing data when both chips are full
 * Capable of losing power and continuing from where code was left at
 * 
 * Author(s):
 * Samuel Brice Smith
 * smit4344@purdue.ecu
 * 
 * Electronics and Code assistance provided by:
 * Andrew Smith
 * andrewsmithcu@gmail.com
 * 
 */

#include <arduino.h>
#include <Wire.h>

#define EEPROM1_ADDRESS 0x50 // First EEPROM
#define EEPROM2_ADDRESS 0x51 // Second EEPROM
#define EEPROM_SIZE 65536    // 64KB = 65536 bytes
#define DATA_SIZE 2          // Number of analog inputs
#define READ_INTERVAL 240    // 0.24 seconds (reduced to prevent drift from write delay)

int currentAddress = 0; // Current address for both EEPROMs
int logAddress = 65535;

void setup() {
    Serial.begin(115200);
    Wire.begin(); // Start the I2C bus
    Wire.setClock(1000000); // Set I2C speed to 1 MHz
}

void loop() {
    // Read analog values from A0, A1, A2, A3
    byte data1[DATA_SIZE];
    byte data2[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; i++) {
        data1[i] = analogRead(A0 + i); //Log A0 and A1
        data2[i] = analogRead(A2 + i); //Log A2 and A3
    }

    // Write data to EEPROM
    if (currentAddress < EEPROM_SIZE) {
        writeEEPROM(EEPROM1_ADDRESS, currentAddress, data1); //write A0 and A1 to EEPROM 1
        writeEEPROM(EEPROM2_ADDRESS, currentAddress, data2); //write A2 and A3 to EEPROM 2

        writeBackup(EEPROM1_ADDRESS, currentAddress, logAddress);
        currentAddress++;
      } else {
        // Both EEPROMs are full, stop writing
        Serial.println("Both EEPROMs are full. Stopping data collection.");
        while (true); // Stop further execution
    }

    // Delay for 0.24 seconds
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

void writeBackup(int eepromAddress, int address, int logAddress) {
    Wire.beginTransmission(eepromAddress);
    Wire.write((int)(logAddress >> 8));   // Send high byte
    Wire.write((int)(logAddress & 0xFF)); // Send low byte
    
    //Sending Last used address
    Wire.write((int)(address >> 8));   // Send high byte
    Wire.write((int)(address & 0xFF)); // Send low byte
    Wire.endTransmission();
    delay(5); // Wait for EEPROM to write

}
