#include "main.h"
#include "math.h"/*
#include "drivetrain.h"
#include "balllift.h"
#include "claw.h"
#include "clawlift.h"
#include "launcher.h"
#include "lifter.h"*/

#define ARM_MIN 220
#define ARM_FLIP 680
#define ARM_UNDER 1450 // goes under an elevated cap, doesn't flip it
#define ARM_DESCORE 1600
#define ARM_MAX 2200
#define ARM_EPSILON 16 // resolution

int armlevel = ARM_MIN;

void calibrateArm() {
	int value = analogRead(1);
	printf("%u\n", value);
}

void controlDrive() {
	int left = joystickGetAnalog(1, 3);
	int right = -joystickGetAnalog(1, 2) * 0.8;
	motorSet(2, left);
	motorSet(3, right);
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
	bool up = joystickGetDigital(1, 6, JOY_UP);
	bool down = joystickGetDigital(1, 6, JOY_DOWN);
	if (up) {
		armlevel += 50;
	} else if (down) {
		armlevel -= 50;
	}
}

void controlIntake() {
	bool forward = joystickGetDigital(1, 5, JOY_UP);
	bool reverse = joystickGetDigital(1, 5, JOY_DOWN);
	if (forward) {
		motorSet(4, 127);
	} else if (reverse) {
		motorSet(4, -128);
	} else {
		motorSet(4, 0);
	}
}

void maintainArm() {
	int currentArmState = analogRead(1);
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
		scalar = 127;
	}
	if (currentArmState < armlevel - ARM_EPSILON) {
		motorSet(1, scalar);
	} else if (currentArmState > armlevel + ARM_EPSILON) {
		motorSet(1, -scalar);
	} else {
		motorSet(1, 0);
	}
}

// the operatorControl loop
void operatorControl() {
	while (1) {
		//controlDrive();
		//controlIntake();
		//controlPunch();
    calibrateArm();
		maintainArm();
		controlArm();
		delay(20);
	}
}
