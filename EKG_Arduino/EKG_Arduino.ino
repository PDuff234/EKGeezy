#include <ArduinoBLE.h>
#include <Wire.h>
//#include "MAX30105.h"
//#include "spo2_algorithm.h"

// *******************************************************************************************************************
#define MAX_BRIGHTNESS 255

// *******************************************************************************************************************
 // BLE Heart rate Service
BLEService heartRateService("180D"); // need to add this 
/*
// were going to first try making the string characteristic and add all 7 characteristics 
BLEStringCharacteristic firstCharacteristic("19b10010-e8f2-537e-4f6c-d104768a1214",  // standard 16-bit characteristic UUID
    BLERead | BLENotify, 100); // remote clients will be able to get notifications if this characteristic changes, change to heartrate, the 50 is how long the string is */

BLEFloatCharacteristic irLEDLevelChar("2B0A",  // standard 16-bit characteristic UUID, UUID for illuminating voltage (LED)
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEFloatCharacteristic redLEDLevelChar("2B0A",  // standard 16-bit characteristic UUID, UUID for illuminating voltage (LED)
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEFloatCharacteristic bufferLengthLevelChar("2AF5",  // standard 16-bit characteristic UUID, fixed string UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEFloatCharacteristic heartRateLevelChar("2a37",  // standard 16-bit characteristic UUID, heart rate uuid (obviously)
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEFloatCharacteristic heartRateValidLevelChar("2AE2",  // standard 16-bit characteristic UUID, used the boolean uuid, since values would be either 0 or 1 
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEFloatCharacteristic spo2LevelChar("2A5E",  // standard 16-bit characteristic UUID, spo2 uuid (obviously)(previous options: 2AF7, 2A5E
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEFloatCharacteristic spo2ValidLevelChar("2AE2",  // standard 16-bit characteristic UUID, used the boolean uuid
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

// *******************************************************************************************************************

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

long lastUpdate = 0; 

byte pulseLED = 11; //Must be on PWM pin
byte readLED = 13; //Blinks with each data read

// float characteristic values (essentially give a start value to each of the values (ir, red led, heartrate ect) 
// so that when its initially called, its not a null value, if that makes sense 
// Question, can we write just float (list all variables followed be a comma each?) like below? 
// float redBuffer, irBuffer, heartRate, validHeartRate, spo2, validSPO2; 
// im going to actually comment this part out here and move it into the void heart rate func since i dont want these 
// variables to be voided in the spo2 algorithm, were going back to the original float list func
/*
float redBuffer;
float irBuffer;
float heartRate;
float validHeartRate;
float spo2;
float validSPO2;
*/

float oldHeartRateReading = 0; // need to name this starting value as a string 
// *******************************************************************************************************************
void setup() {
  Serial.begin(9600);    // initialize serial communication
  //while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    
    while (1);
  }

  /* Set a local name for the BLE device
     This name will appear in advertising packets
     and can be used by remote devices to identify this BLE device
     The name can be changed but maybe be truncated based on space left in advertisement packet
  */
  BLE.setLocalName("NanoDevice");
  BLE.setAdvertisedService(heartRateService); // add the service UUID
  heartRateService.addCharacteristic(irLEDLevelChar); // add the characteristic
  heartRateService.addCharacteristic(redLEDLevelChar); // add the characteristic
  heartRateService.addCharacteristic(heartRateLevelChar); // add the characteristic
  heartRateService.addCharacteristic(heartRateValidLevelChar); // add the characteristic
  heartRateService.addCharacteristic(spo2LevelChar); // add the characteristic
  heartRateService.addCharacteristic(spo2ValidLevelChar); // add the characteristic

  BLE.addService(heartRateService); // Add the service
  irLEDLevelChar.writeValue(oldHeartRateReading); // set initial value for this characteristic
  redLEDLevelChar.writeValue(oldHeartRateReading); // set initial value for this characteristic
  heartRateLevelChar.writeValue(oldHeartRateReading); // set initial value for this characteristic
  heartRateValidLevelChar.writeValue(oldHeartRateReading); // set initial value for this characteristic
  spo2LevelChar.writeValue(oldHeartRateReading); // set initial value for this characteristic
  spo2ValidLevelChar.writeValue(oldHeartRateReading); // set initial value for this characteristic, 
  // was getting an error before, needed to redefine the oldHeartRateReading as a float value, not a char string value


  /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */

  // start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}  


void loop() {
  // wait for a BLE central
  BLEDevice central = BLE.central();

  // if a central is connected to the peripheral:
  if (central) {
    //Serial.print("Connected to central: ");
    // print the central's BT address:
    //Serial.println(central.address());
    // turn on the LED to indicate the connection:
    digitalWrite(LED_BUILTIN, HIGH);

    // while the central is connected:
    while (central.connected()) {
        update();
        delay(5);
    }
    // when the central disconnects, turn off the LED:
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void update() {
  int heartValue = analogRead(heartPin);
  Serial.println(heartValue);
  heartRateLevelChar.setValue(heartValue); 
}
