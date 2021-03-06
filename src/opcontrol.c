#include "main.h"
#include "math.h"

// Team Pororo Robot Operator Control Code
// Copyright (c) Wesley Chalmers, 2018

// motor ports
#define L_DRIVE 2
#define R_DRIVE 3
#define INTAKE 4
#define PUNCHER 5
#define ARM 1

// sensor ports
#define ARMPOT 1

// arm constants
#define ARM_MIN 700 // minimum safe position before hitting ground
#define ARM_FLIP 1220 // flip a cap on the ground
#define ARM_UNDER 1800 // goes under an elevated cap, doesn't flip it
#define ARM_DESCORE 2250 // max vertical position, descore a cap on post
#define ARM_MAX 3200 // maximum safe position vertically before hitting intake
#define ARM_EPSILON 16 // resolution

// intake-forward button is toggle on/off so it doesn't have to be held down
bool toggleIntake = true;

// Drive mode
#define DEFAULT_DRIVE 0 // Wesley's DefaultDrive control set - tank steer
#define CHANDLER_DRIVE 1 // Chandler's steer/speed-drive control set
												 // Like Team 254 "cheesy drive" or two-stick arcade drive

bool inverseDriveButtonState = false;
bool inverseDriving = false;
bool driveModeButtonState = false;
bool driveMode = DEFAULT_DRIVE;

void controlDrive() {
	bool driveModeButton = joystickGetDigital(1, 7, JOY_UP);
	if (driveModeButton != driveModeButtonState) {
		driveModeButtonState = driveModeButton;
		if (driveModeButton) {
			printf("Toggling drive mode");
			driveMode = !driveMode;
		}
	}
	bool inverseDrive = joystickGetDigital(1, 7, JOY_RIGHT);
	if (inverseDrive != inverseDriveButtonState) {
		inverseDriveButtonState = inverseDrive;
		if (inverseDrive) {
			inverseDriving = !inverseDriving;
		}
	}
	if (driveMode == DEFAULT_DRIVE) {
		int left = joystickGetAnalog(1, 3);
		int right = joystickGetAnalog(1, 2);
		if (inverseDriving) {
			motorSet(L_DRIVE, -right);
			motorSet(R_DRIVE, left * 0.9);
		} else {
			motorSet(L_DRIVE, left);
			motorSet(R_DRIVE, -right * 0.9);
		}
	} else if (driveMode == CHANDLER_DRIVE) {
		int x = -joystickGetAnalog(1, 4);
		int y = joystickGetAnalog(1, 2);
		int l = 0;
		int r = 0;

		int v = (128-abs(x))*(y/128)+y;
		int w = (128-abs(y))*(x/128)+x;

		r = (v+w)/2;
		l = (v-w)/2;

		if (inverseDriving) {
			motorSet(L_DRIVE, -r);
			motorSet(R_DRIVE, l);
		} else {
			motorSet(L_DRIVE, l);
			motorSet(R_DRIVE, -r);
		}
	}
}

int armlevel = ARM_MIN;
bool armHolding = true; // fake-pid can be disabled for manual arm control
bool intakeRunning = 0;
bool intakeButtonPressed = 0;

void calibrateArm() {
	// Write arm values to serial to allow for manual calibration of presets
	int value = analogRead(ARMPOT);
	printf("%u\t%u\n", value, armlevel);
	delay(200); // slow down the control loop to prevent serial link saturation
}

void controlPunch() {
	bool forward = joystickGetDigital(1, 6, JOY_UP);
	bool reverse = joystickGetDigital(1, 6, JOY_DOWN);
	if (forward) {
		motorSet(PUNCHER, 127);
	} else if (reverse) {
		motorSet(PUNCHER, -128);
	} else {
		motorSet(PUNCHER, 0);
	}
}

void controlArm() {
	bool up = joystickGetDigital(1, 7, JOY_LEFT);
	bool down = joystickGetDigital(1, 7, JOY_DOWN);
	bool pos1 = joystickGetDigital(1, 8, JOY_DOWN);
	bool pos2 = joystickGetDigital(1, 8, JOY_LEFT);
	bool pos3 = joystickGetDigital(1, 8, JOY_RIGHT);
	bool pos4 = joystickGetDigital(1, 8, JOY_UP);
	//bool deploy = joystickGetDigital(1, 7 , JOY_LEFT);
	//bool retract = joystickGetDigital(1, 7, JOY_RIGHT);
	int currentArmState = analogRead(1);
	//printf("UP/DN:\t%u\t%u\t%u\n", up, down, currentArmState);
	if (down && currentArmState > ARM_MIN) {
		armHolding = false;
		//printf("Setting armHolding to false");
		motorSet(ARM, -16);
	} else if (up && currentArmState < ARM_MAX) {
		armHolding = false;
		//printf("Setting armHolding to false");
		motorSet(ARM, 64);
	} else {
		if (!armHolding) {
			armHolding = true;
			motorSet(ARM, 0); // pause until maintainArm() can take over
			armlevel = analogRead(ARMPOT);
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
			motorSet(INTAKE, 127);
		} else {
			if (intakeRunning) {
				motorSet(INTAKE, -128);
			} else {
				motorSet(INTAKE, 0);
			}
		}
	} else {
	 // dumb stupid intake controls
		if (reverse) {
			motorSet(INTAKE, 127);
		} else if (forward) {
			motorSet(INTAKE, -128);
		} else {
			motorSet(INTAKE, 0);
		}
	}
}

void maintainArm() {

	// WHO'S CALLING MY CODE SHADY! I PITY THE FOOL WHO'S CALLIN WESLEY'S
	// CODE SHADY! PITY I SAY!

	int currentArmState = analogRead(ARMPOT);
	if (armHolding) {
		int scalar;
		int delta = abs(currentArmState - armlevel);
		if (delta < 32) {
			scalar = 8;
		} else if (delta < 64) {
			scalar = 16;
		} else if (delta < 128) {
			scalar = 32;
		} else if (delta < 256) {
			scalar = 64;
		} else {
			scalar = 128;
		}
		if (currentArmState < armlevel - ARM_EPSILON) {
			motorSet(ARM, scalar);
		} else if (currentArmState > armlevel + ARM_EPSILON) {
			motorSet(ARM, -scalar);
		} else {
			motorSet(ARM, 0);
		}
	} else {
		if (currentArmState > ARM_MAX || currentArmState < ARM_MIN) {
			motorSet(ARM, 0);
		}
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
