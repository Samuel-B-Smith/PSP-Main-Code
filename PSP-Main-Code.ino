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
#define EEPROM_SIZE 65533    // 64KB = 65536 bytes shortened to prevent overwrite of log
#define DATA_SIZE 2          // Number of analog inputs
#define READ_INTERVAL_MS 250 // The desired time between sensor readings in milliseconds

unsigned int currentAddress = 0; // Current address for both EEPROMs
unsigned int logAddress = 65534; //location where data will be logged
int counter = 0;

unsigned long timeOfLastSensorRead;

void setup() {
    Serial.begin(115200);
    Wire.begin(); // Start the I2C bus
    Wire.setClock(1000000); // Set I2C speed to 1 MHz
    currentAddress = failCheck(logAddress, EEPROM1_ADDRESS);

    if (currentAddress == 0) {
      for (int initialDelay = 0; initialDelay > 0; initialDelay--) {
        Serial.print("Waiting for ");
        Serial.print(initialDelay);
        Serial.println(" minutes");
        delay(60000);
      }
    }
    else {
      Serial.print("Code has already been started, resuming at ");
      Serial.println(currentAddress);
    }
}

void loop() {
    //Record millis right before the first sensor reading
    timeOfLastSensorRead = millis();

    // Read analog values from A0, A1, A2, A3
    unsigned int data1[DATA_SIZE];
    unsigned int data2[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; i++) {
        data1[i] = analogRead(A0 + i); //Log A0 and A1
        data2[i] = analogRead(A2 + i); //Log A2 and A3

      //Adding last 6 bits as a counter
      data1[i] = (data1[i] & 0x3FF) | ((counter & 0x3F) << 10);
      data2[i] = (data2[i] & 0x3FF) | ((counter & 0x3F) << 10);
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

    //iterate counter and reset at 63
    counter = (counter + 1) % 64;
    //Move to next address
    currentAddress += 4;

    //Wait for READ_INTERVAL_MS time to pass before redoing the loop
    while(millis() - timeOfLastSensorRead < READ_INTERVAL_MS)
    {
        //Do nothing...
    }
}

unsigned int failCheck(unsigned int logAddress, byte eepromAddress) {
    byte highLog  = readEEPROM(eepromAddress, logAddress);
    byte lowLog  = readEEPROM(eepromAddress, logAddress + 1);

  currentAddress = (highLog << 8) | lowLog;
  return currentAddress;
}

byte readEEPROM(int eepromAddress, int address) {
    Wire.beginTransmission(eepromAddress);
    Wire.write((int)(address >> 8));   // Send high byte
    Wire.write((int)(address & 0xFF)); // Send low byte
    Wire.endTransmission();

    Wire.requestFrom(eepromAddress, 1); // Request 1 byte
    if (Wire.available()) {
        return Wire.read(); // Read the value
    }
    Serial.println("Error");
    return 0;
}


void writeEEPROM(byte eepromAddress, unsigned int currentAddress, unsigned int* data) {
    Wire.beginTransmission(eepromAddress);
    Wire.write((int)(currentAddress >> 8));   // Send high byte
    Wire.write((int)(currentAddress & 0xFF)); // Send low byte

    for (int i = 0; i < DATA_SIZE; i++) {
      Wire.write((int)(data[i] >> 8));   // Send high byte of data
      Wire.write((int)(data[i] & 0xFF)); // Send low byte of data
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
