#include <Servo.h>

/***************************************************
/ Example provided by MINDS-i
/ Try checking out our arduino resource guide at
/ http://mindsieducation.com/programming-resources
/ Questions? Concerns? Bugs? email code@mymindsi.com
/
/ This example is to be used to set the open position
/ of the MINDS-i Sprayer Trailer servos attached to
/ pin 2 and pin 3
/***************************************************/

Servo leftBase, rightBase;

void setup() {
    // Defines the pins the servos and ESC are plugged into
    leftBase.attach(2);
    rightBase.attach(3);

    // Sets the starting position for the servos and ESC
    leftBase.write(135); // 135 closed -  45 open
    rightBase.write(45); // 45 closed - 135 open
}

void loop() {
    // put your main code here, to run repeatedly:
}
