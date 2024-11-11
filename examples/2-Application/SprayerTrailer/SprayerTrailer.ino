#include <MINDS-i-Common.h>

/***************************************************
/ Example provided by MINDS-i
/ Try checking out our arduino resource guide at
/ http://mindsieducation.com/programming-resources
/ Questions? Concerns? Bugs? email code@mymindsi.com
/
/ This example template will change the angle of
/ 2 servos based on the input from the tractor or a
/ RC receiver plugged in to pin 7.
/
/ The tractor will signal the trailer to run the code
/ in "trailerOn" when it reaches a waypoint tagged
/ trailer on. It will in turn run the code in
/ "trailerOff" whenever it reaches a non tagged waypont.
/***************************************************/

#define SERVO1_PIN 2
#define SERVO1_OFF 135
#define SERVO1_ON  45

#define SERVO2_PIN 3
#define SERVO2_OFF 45
#define SERVO2_ON  135

#define MAX_SERVO_DEGREES_PER_SECOND 500

namespace minds_i_control = minds_i_common::control;
namespace minds_i_comms = minds_i_common::comms;

minds_i_control::RateControlledServo servo1, servo2;

unsigned long ticks;

void setup() {
  ticks = 0;
  servo1.attachInitAngle(SERVO1_PIN, SERVO1_OFF);
  servo2.attachInitAngle(SERVO2_PIN, SERVO2_OFF);
}

void loop() {
  if (minds_i_comms::getRadio(7) > 90)
  {
    trailerOn();
  } 
  else
  {
    trailerOff();
  }
}

void trailerOff() {
  servo1.writeRateControlled(SERVO1_OFF, MAX_SERVO_DEGREES_PER_SECOND);
  servo2.writeRateControlled(SERVO2_OFF, MAX_SERVO_DEGREES_PER_SECOND);
}

void trailerOn() {
  servo1.writeRateControlled(SERVO1_ON, MAX_SERVO_DEGREES_PER_SECOND);
  servo2.writeRateControlled(SERVO2_ON, MAX_SERVO_DEGREES_PER_SECOND);
}
