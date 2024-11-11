// Include Libraries
#include "Arduino.h"
#include "Encoder.h"
#include <EEPROM.h>
#include <NewPing.h>
#include <Wire.h>

//#define DEBUG

// Parmeters
#define MAX_DISTANCE             200 // Maximum distance we want to ping for (in centimeters)
#define MAX_PING_SENSORS         5
#define ENCODER_UPDATE_PERIOD_MS (1000.0 / 25.0) // 25 Hz
#define PING_UPDATE_PERIOD_MS    (1000.0 / 10.0) // 10 Hz
#define NUM_PING_SENSORS_ADDR    0

// Hardware
#define I2C_ADDRESS    0x08
#define AXLENC_PIN_CLK 2
#define AXLENC_PIN_D   3
#define AXLENC_PIN_S1  4

// message data types
#define CONFIGURE_MSG_TYPE    0
#define ENCODER_INIT_MSG_TYPE 1
#define ENCODER_MSG_TYPE      2
#define PING_INIT_MSG_TYPE    3
#define PING_MSG_TYPE         4
#define INVALID_MSG_TYPE      255

// general
byte msgRequest           = INVALID_MSG_TYPE;
unsigned int msgCounter   = 0;
unsigned int numBytesLeft = 0;

// wheel encoder
float tps; // encoder ticks per second
uint8_t tpsOutBuf[2];
long axleEncOldPosition;
unsigned long lastEncoderUpdate;
Encoder axleEnc(AXLENC_PIN_D, AXLENC_PIN_CLK);
long axleEncLastPosition;
bool encoderInitialized = false;

// ping sensor
const int ping0Pin               = 5; // left 90 (driver's perspective)
const int ping1Pin               = 6; // left 45
const int ping2Pin               = 7; // forward
const int ping3Pin               = 8; // right 45
const int ping4Pin               = 9; // right 90
NewPing sonars[MAX_PING_SENSORS] = {
  NewPing(ping0Pin, ping0Pin, MAX_DISTANCE), NewPing(ping1Pin, ping1Pin, MAX_DISTANCE),
  NewPing(ping2Pin, ping2Pin, MAX_DISTANCE), NewPing(ping3Pin, ping3Pin, MAX_DISTANCE),
  NewPing(ping4Pin, ping4Pin, MAX_DISTANCE)};
uint8_t pingData[MAX_PING_SENSORS] = {0, 0, 0, 0, 0};
uint8_t pingIter                   = 0;
uint8_t numPingSensors;
unsigned long lastPingUpdate;
bool pingInitialized = false;
bool pingReceived    = false;

// setup (interupts are enabled before setup is called)
void setup()
{
  // Setup encoder
  pinMode(AXLENC_PIN_S1, INPUT_PULLUP);
  lastEncoderUpdate   = millis();
  axleEncLastPosition = axleEnc.read();

  // Setup ping
  numPingSensors = EEPROM.read(NUM_PING_SENSORS_ADDR);
  if (numPingSensors == 255) // EEPROM values that have never been written start at 255
  {
    numPingSensors = 0;
  }
  numPingSensors = 1;
  lastPingUpdate = millis();

  // Setup I2C
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

#ifdef DEBUG
  // Setup Serial which is useful for debugging
  // Use the Serial Monitor to view printed messages
  Serial.begin(115200);
  while (!Serial)
  {
    // wait for serial port to connect. Needed for native USB
  }
  Serial.println("start");
#endif
}

// main routine of the program
void loop()
{
  //*******ENCODER*********//
  if (millis() >= (lastEncoderUpdate + ENCODER_UPDATE_PERIOD_MS))
  {
    unsigned long curTime   = millis();
    long axleEncNewPosition = axleEnc.read();
    tps = ((axleEncNewPosition - axleEncLastPosition) * (1000.f / (curTime - lastEncoderUpdate)));
    lastEncoderUpdate   = curTime;
    axleEncLastPosition = axleEncNewPosition;
    encoderInitialized  = true; // set flag that we've received an encoder update
#ifdef DEBUG
    Serial.print("tps = ");
    Serial.println(tps);
#endif
  }

  //*******PING*********//
  if (numPingSensors > 0 && millis() >= (lastPingUpdate + PING_UPDATE_PERIOD_MS))
  {
    lastPingUpdate = millis();

    if (!pingReceived)
    {
      pingData[pingIter] = 0; // failed to get ping data (return likely too far)
                              // set value to 0 to indicate no return
#ifdef DEBUG
      Serial.print("Failed to get ping ");
      Serial.println(pingIter);
#endif
    }

    sonars[pingIter].timer_stop(); // just in case the ping timer didn't finish
    pingIter =
      (pingIter + 1) %
      numPingSensors; // since pingIter starts at 0, this will start at the second ping sensor
    sonars[pingIter].ping_timer(echoCheck); // Send out the ping, calls "echoCheck" function every
                                            // 24uS to poll the result (or timeout)
    pingReceived = false;

    if (pingIter == 0)
    {
      // all ping sensors have fired
      pingInitialized = true;
    }
  }
}

void receiveData(int bytecount)
{
  if (bytecount != 1)
  {
    // this should not happen but just in case
    while (Wire.available())
    {
      Wire.read(); // throw away the invalid bytes
    }
    return;
  }

  byte readbyte = Wire.read();
#ifdef DEBUG
  Serial.print("readbyte = ");
  Serial.println(readbyte);
#endif

  if (numBytesLeft > 0)
  {
    switch (msgRequest)
    {
      case CONFIGURE_MSG_TYPE:
      {
        byte numPings = readbyte; // data byte sets the desired number of ping sensors;
        if (numPingSensors != numPings && numPings <= MAX_PING_SENSORS)
        {
          // only write to EEPROM if value changes (~100,000 writes for EEPROM address)
          EEPROM.write(NUM_PING_SENSORS_ADDR, numPings);
          numPingSensors = numPings;
        }
      }
        numBytesLeft--;
        break;
      default:
        numBytesLeft--;
        break;
    }
  }
  else
  {
    msgRequest = readbyte;
    msgCounter = 0;

    switch (msgRequest)
    {
      case CONFIGURE_MSG_TYPE:
        numBytesLeft = 1; // waiting to read num ping sensors
        break;
      case ENCODER_MSG_TYPE:
        // calculate the tps here since updates might happen during transmission and cause data
        // corruption
        {
          int tps_i    = round(tps);
          tpsOutBuf[0] = tps_i >> 8; // shift right 8 bits, leaving only the 8 high bits.
          tpsOutBuf[1] = tps_i & 0xFF;
        }
        numBytesLeft = 0;
        break;
      default:
        numBytesLeft = 0;
        break;
    }
  }
}

void sendData()
{
  switch (msgRequest)
  {
    case CONFIGURE_MSG_TYPE:
      if (msgCounter < 1)
      {
        Wire.write(numPingSensors);
        msgCounter++;
      }
      break;
    case ENCODER_INIT_MSG_TYPE:
      if (msgCounter < 1)
      {
        Wire.write(encoderInitialized);
        msgCounter++;
      }
      break;
    case ENCODER_MSG_TYPE:
      if (msgCounter < sizeof(tpsOutBuf))
      {
        Wire.write(tpsOutBuf[msgCounter++]);
      }
      break;
    case PING_INIT_MSG_TYPE:
      if (msgCounter < 1)
      {
        Wire.write(pingInitialized);
        msgCounter++;
      }
      break;
    case PING_MSG_TYPE:
      if (msgCounter < numPingSensors * sizeof(pingData[0]))
      {
        Wire.write(pingData[msgCounter++]);
      }
    default:
      break;
  }
}

void echoCheck()
{ // Timer2 interrupt calls this function every 24uS where you can check the ping status.
  if (sonars[pingIter].check_timer())
  { // This is how you check to see if the ping was received.
    sonars[pingIter].timer_stop();
    pingData[pingIter] = (uint8_t)(sonars[pingIter].ping_result / US_ROUNDTRIP_CM);
    pingReceived       = true;
#ifdef DEBUG
    Serial.print("Ping: ");
    Serial.print(pingData[pingIter]); // Ping returned, uS result in ping_result, convert to cm with
                                      // US_ROUNDTRIP_CM.
    Serial.println("cm");
#endif
  }
}