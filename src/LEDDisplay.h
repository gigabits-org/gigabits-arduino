// This file contains utility routines that
// used to handle an actuator composed of a single LED.
typedef struct {
	boolean LEDIsLit;

	void turnOnDisplay() {
		LEDIsLit = true;
		digitalWrite(LEDPin, LOW);
	}

	void turnOffDisplay() {
		LEDIsLit = false;
		digitalWrite(LEDPin, HIGH);
	}

	void toggleDisplay() {
		if (LEDIsLit) {
			LEDIsLit = false;
			digitalWrite(LEDPin, HIGH);
		} else {
			LEDIsLit = true;
			digitalWrite(LEDPin, LOW);
		}
	}

	boolean getDisplay() {
		return LEDIsLit;
	}
} LEDDisplay;
