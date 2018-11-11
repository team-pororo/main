#include "main.h"
#include "math.h"

// Team Pororo Robot Operator Control Code
// Copyright (c) Wesley Chalmers, 2018


// arm constants
#define ARM_MIN 3200
#define ARM_FLIP 2700
#define ARM_UNDER 2110 // goes under an elevated cap, doesn't flip it
#define ARM_DESCORE 1570
#define ARM_MAX 1000
#define ARM_EPSILON 16 // resolution

// intake-forward button is toggle on/off so it doesn't have to be held down
bool toggleIntake = true;

// Drive mode
#define DEFAULT_DRIVE 0 // Wesley's DefaultDrive control set - tank steer
#define CHANDLER_DRIVE 1 // Chandler's steer/speed-drive control set
int driveMode = DEFAULT_DRIVE;


// flickturn constants:
#define LEFT_FLICK -64
#define RIGHT_FLICK 64

int armlevel = ARM_MIN;
bool armHolding = true; // fake-pid can be disabled for manual arm control
bool intakeRunning = 0;
bool intakeButtonPressed = 0;

void calibrateArm() {
	// Write arm values to serial to allow for manual calibration of presets
	int value = analogRead(1);
	printf("%u\n", value);
}

void controlDrive() {
	if (driveMode == DEFAULT_DRIVE) {
		int left = joystickGetAnalog(1, 3);
		int right = joystickGetAnalog(1, 2);
		motorSet(2, left);
		motorSet(3, -right);
	} else if (driveMode == CHANDLER_DRIVE) {
		int steer = joystickGetAnalog(1, 4);
		int speed = joystickGetAnalog(1, 2);
		// flick the right stick left or right for a quick 90deg turn
		int flickturn = joystickGetAnalog(1, 1);

		int left = 0;
		int right = 0;

		if (flickturn < LEFT_FLICK) {
			left = -128;
			right = 127;
		} else if (flickturn > RIGHT_FLICK) {
			left = 127;
			right = -128;
		} else {
			left = speed;
			right = speed;
			if (steer < 0) {
				steer = (128 - steer) / 128;
				left *= -steer;
			} else {
				steer = (128 - steer) / 128;
				right *= steer;
			}
		}
		motorSet(2, left);
		motorSet(3, right);
	}
}

void controlPunch() {
	bool forward = joystickGetDigital(1, 6, JOY_UP);
	bool reverse = joystickGetDigital(1, 6, JOY_DOWN);
	if (forward) {
		motorSet(1, 127);
	} else if (reverse) {
		motorSet(1, -128);
	} else {
		motorSet(1, 0);
	}
}

void controlArm() {
	bool up = joystickGetDigital(1, 7, JOY_RIGHT);
	bool down = joystickGetDigital(1, 7, JOY_DOWN);
	bool pos1 = joystickGetDigital(1, 8, JOY_DOWN);
	bool pos2 = joystickGetDigital(1, 8, JOY_LEFT);
	bool pos3 = joystickGetDigital(1, 8, JOY_RIGHT);
	bool pos4 = joystickGetDigital(1, 8, JOY_UP);
	//bool deploy = joystickGetDigital(1, 7 , JOY_LEFT);
	//bool retract = joystickGetDigital(1, 7, JOY_RIGHT);
	if (up) {
		armHolding = false;
		motorSet(10, -16);
	} else if (down) {
		armHolding = false;
		motorSet(10, 16);
	} else {
		if (!armHolding) {
			armHolding = true;
			motorSet(10, 0); // pause until maintainArm() can take over
			armlevel = analogRead(1);
		}
	}
	if (pos1) {
		armlevel = ARM_MIN;
	} else if (pos2) {
		armlevel = ARM_FLIP;
	} else if (pos3) {
		armlevel = ARM_UNDER;
	} else if (pos4) {
		armlevel = ARM_DESCORE;
	}
}

void controlIntake() {
	bool forward = joystickGetDigital(1, 5, JOY_UP);
	bool reverse = joystickGetDigital(1, 5, JOY_DOWN);
	if (toggleIntake) {
		// watch for button presses, update running state
		if (forward != intakeButtonPressed) {
			intakeButtonPressed = forward;
			if (forward) {
				intakeRunning = !intakeRunning;
			}
		}

		if (reverse) {
			motorSet(4, 127);
		} else {
			if (intakeRunning) {
				motorSet(4, -128);
			} else {
				motorSet(4, 0);
			}
		}
	} else {
	 // dumb stupid intake controls
		if (reverse) {
			motorSet(4, 127);
		} else if (forward) {
			motorSet(4, -128);
		} else {
			motorSet(4, 0);
		}
	}
}

void maintainArm() {
	if (!armHolding) {
		return;
	}
	int currentArmState = analogRead(1);
	int scalar;
	int delta = abs(currentArmState - armlevel);
	if (delta < 32) {
		scalar = 4;
	} else if (delta < 64) {
		scalar = 8;
	} else if (delta < 128) {
		scalar = 16;
	} else if (delta < 256) {
		scalar = 32;
	} else {
		scalar = 64;
	}
	if (currentArmState < armlevel - ARM_EPSILON) {
		motorSet(10, -scalar);
	} else if (currentArmState > armlevel + ARM_EPSILON) {
		motorSet(10, scalar);
	} else {
		motorSet(10, 0);
	}
}

// the operatorControl loop
void operatorControl() {
	while (1) {
		controlDrive();
		controlIntake();
		controlPunch();
    //calibrateArm();
		maintainArm();
		controlArm();
		delay(20);
	}
}
