#include <MINDS-i-Common.h>
#include <Servo.h>

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

namespace minds_i_comms = minds_i_common::comms;

Servo servo1, servo2;

void setup() {
  servo1.attach(2);
  servo2.attach(3);

  servo1.write(90);
  servo2.write(90);
}

void loop() {
  if (minds_i_comms::getRadio(7) > 90) {
    trailerOn();
  } else {
    trailerOff();
  }
}

void trailerOff() {
  servo1.write(45);
  servo2.write(135);
}

void trailerOn() {
  servo1.write(135);
  servo2.write(45);
}
