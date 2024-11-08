/* PSP Payload Prometheus Relaunch Mission Code
 * Verifies that chip is in write mode. If switch is not set to write, code does not continue
 * Gathers data from 4 Wheatstone Bridge chips on pins A0-A3 at a rate of 4 samples per second
 * Writes Data from Wheatstone bridges on to 2 seperate EEPROM Chips using A4 and A5 pins
 * Writes to chip 1's address 50 until filled before moving to chip 2 at address of 51
 * Stops writing data when both chips are full
 * Capable of losing power and continuing from where code was left at
 *
 * Target chip is ATMEGA328P (arduino pro mini)
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
#define EEPROM_SIZE 65535    // 64KB = 65536 bytes
#define DATA_SIZE 4          // Number of analog inputs
#define READ_INTERVAL_MS 250 // The desired time between sensor readings in milliseconds

uint32_t currentAddress = 0; // Current address for both EEPROMs
int counter = 0;
unsigned int data[DATA_SIZE]; // Data array 

unsigned long timeOfLastSensorRead;

void setup() {
    Serial.begin(115200);
    Wire.begin(); // Start the I2C bus
    Wire.setClock(1000000); // Set I2C speed to 1 MHz

    currentAddress = getFirstCleanAddress();

    if(currentAddress > 0xFFFF) {
        Serial.println("Stopping execution because there is nothing to do (out of EEPROM).");
        while(true);
    }

    //Clean EEPROM situation
    if (currentAddress == 0x0000) {
      for (int initialDelay = 0; initialDelay > 0; initialDelay--) {
        Serial.print("Waiting for ");
        Serial.print(initialDelay);
        Serial.println(" minutes");
        delay(60000);
      }
    }
    else {
      Serial.print("Code has already been started, resuming at ");
      Serial.println(currentAddress, HEX);
    }
}

void loop() {
    //Record millis right before the first sensor reading
    timeOfLastSensorRead = millis();

    // Read analog values from A0, A1, A2, A3


    data[0] = analogRead(A0); //Log A0
    data[1] = analogRead(A1); //Log A1
    data[2] = analogRead(A2); //Log A2
    data[3] = analogRead(A3); //Log A3

    //Adding last 6 bits as a counter
    data[0] = (data[0] & 0x3FF) | ((counter & 0x3F) << 10);
    data[1] = (data[1] & 0x3FF) | ((counter & 0x3F) << 10);
    data[2] = (data[2] & 0x3FF) | ((counter & 0x3F) << 10);
    data[3] = (data[3] & 0x3FF) | ((counter & 0x3F) << 10);

    // Write data to EEPROM
    if (currentAddress < (EEPROM_SIZE - sizeof(data))) {
        writeEEPROM(EEPROM1_ADDRESS,EEPROM2_ADDRESS, currentAddress, data); //write A0 and A1 to EEPROM 1

        currentAddress = currentAddress + 4;
      } 
      else {
        // Both EEPROMs are full, stop writing
        Serial.println("Both EEPROMs are full. Stopping data collection.");
        while (true); // Stop further execution
    }

    //iterate counter and reset at 63
    counter = (counter + 1) % 64;

    //Wait for READ_INTERVAL_MS time to pass before redoing the loop
    while(millis() - timeOfLastSensorRead < READ_INTERVAL_MS) {
        //Do nothing...
    }
}


uint32_t getFirstCleanAddress() {
    //This function only checks the first byte in a grouping of 4 (ie addresses that are perfectly divisible by 4)
    //because of the data encoding being used. This is why max is 16,383. The value is multiplied by 4 to get the address
    //(meaning the last searched address is 16,383 * 4 = 65,532, the final address with 4 contiguous bytes available)
    unsigned int low = 0;
    unsigned mid;
    unsigned int high = 16383;

    bool dataIsPresent;

    while(low <= high) {
        //Calculate the new midpoint
        mid = (low + high) / 2;

        dataIsPresent = readEEPROM(EEPROM1_ADDRESS, (mid*4)) != 0x00;

        //If the value at the midpoint is not 0
        if(dataIsPresent == true) {
            low = mid + 1;
        }
        else if(low == mid) {
            //Return the first blanked address (multiplied by 4 to give the true memory address)
            return mid*4;
        }
        else {
            high = mid;
        }
    }

    Serial.println("There were no clean addresses found");
    return 65536;
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
    Serial.println("Error reading EEPROM!");
    return 0xFF;
}


void writeEEPROM(byte eepromAddress1, byte eepromAddress2, unsigned int currentAddress, unsigned int* data) {
    Wire.beginTransmission(eepromAddress1);
    Wire.write((int)(currentAddress >> 8));   // Send high byte
    Wire.write((int)(currentAddress & 0xFF)); // Send low byte

    Wire.write((int)(data[1] >> 8));   // Send high byte of data 1
    Wire.write((int)(data[1] & 0xFF)); // Send low byte of data 1
    Wire.write((int)(data[2] >> 8));   // Send high byte of data 2
    Wire.write((int)(data[2] & 0xFF)); // Send low byte of data 2

    Wire.endTransmission();
    delay(5); // Wait for EEPROM to write

    //Write to EEPROM 2
    Wire.beginTransmission(eepromAddress2);
    Wire.write((int)(currentAddress >> 8));   // Send high byte
    Wire.write((int)(currentAddress & 0xFF)); // Send low byte

    Wire.write((int)(data[3] >> 8));   // Send high byte of data 3
    Wire.write((int)(data[3] & 0xFF)); // Send low byte of data 3 
    Wire.write((int)(data[4] >> 8));   // Send high byte of data 4
    Wire.write((int)(data[4] & 0xFF)); // Send low byte of data 4

    Wire.endTransmission();
    delay(5); // Wait for EEPROM to write
}