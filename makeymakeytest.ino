/**
*
* The MIT License (MIT)
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
* and associated documentation files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
* and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
* OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/
#include "Timer.h"
#include <math.h>
#include <Adafruit_NeoPixel.h>


#define MAKEYMAKEY_DIGITAL_PIN 5
#define LOUDNESS_THRESHOLD 18
#define SECONDS_BEFORE_START 3
#define ONE_SECOND 1000
#define MAXUSEDPIXELS 30
#define SAMPLE_WINDOW 50


// Our LEDS
Adafruit_NeoPixel strip = Adafruit_NeoPixel(30, 6, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightEye = Adafruit_NeoPixel(12, 5, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel leftEye = Adafruit_NeoPixel(12, 4, NEO_GRB + NEO_KHZ800);


unsigned int sample;	// loudness to volts
double maxSignalRecorded = 0.0; // loudness to volts
double volts; // loudness to volts
int score = 0; // game score
int currentLedIndex = 0; // led index
Timer t;
int eyesCounter = SECONDS_BEFORE_START;
boolean isStart = true;
int gameStartTimer;
int scoreCheckTimer;

// Colors
int purple[] = { 107,35,148 };
int blue[] = { 35,35,220 };
int green[] = { 87,157,28 };
int yellow[] = { 255,255,0 };
int orange[] = { 255,149,14 };
int red[] = { 255,0,0 };
int currentColor[3] = { 255,255,255 };
int eyeColor[] = { 87,157,28 };

// thresholds
#define PURPLE_LEVEL 55
#define BLUE_LEVEL 34
#define GREEN_LEVEL 21
#define YELLOW_LEVEL 13
#define ORANGE_LEVEL 8
#define RED_LEVEL 5


/*----------------------------------------- 
	ACTUAL PROGRAM
-----------------------------------------*/

void setup()
{
	leftEye.begin();
	rightEye.begin();
	strip.begin();
	clearStrip();
	Serial.begin(9600);
}

/**
* Our main loop which handles of the logic:
* 1 - we check that we get a contact from the makey makey (players holding hands + holding copper areas)
* 2 - if that's the case we get the loudness level and change color of the LED strip according to the score
*/
void loop() {

	// update our timer to check score
	t.update();


	// check makey makey
	if (!contactDone()) {
		// no signal incoming from makey makey = we shutdown the leds and reset the game
		resetScore();
		clearStrip();
		isStart = true;
		eyesCounter = SECONDS_BEFORE_START * 2;
		t.stop(scoreCheckTimer);
		closeEyes();
		return;
	}


	// Lets make our yes blink before starting
	if (isStart) {
		isStart = false;
		gameStartTimer = t.every(ONE_SECOND / 2, blinkEyes, SECONDS_BEFORE_START * 2);
		return;
	}


	// check our start game timer
	if (eyesCounter > 0) return;


	// collect data for 50 ms on our Loudness sensor
	// convert it to volts
	// determine LED index and illuminate LED strip
	unsigned long startMillis = millis();  // Start of sample window
	unsigned int peakToPeak = 0;   // peak-to-peak level
	unsigned int signalMax = 0;
	unsigned int signalMin = 1024;
	while (millis() - startMillis < SAMPLE_WINDOW) {
		sample = analogRead(0);
		if (sample < 1024) {
			if (sample > signalMax) {
				signalMax = sample;  // save just the max levels
			}
			else if (sample < signalMin) {
				signalMin = sample;  // save just the min levels
			}
		}
	}
	peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
	volts = (peakToPeak * 3.3) / 1024;  // convert to volts
	if (volts > maxSignalRecorded) maxSignalRecorded = volts;
	double max = 2.7;
	double oneLed = max / MAXUSEDPIXELS;
	currentLedIndex = ceil(volts / oneLed);
	clearStrip();
	if (currentLedIndex > 2) {
		colorWipe(strip.Color(currentColor[0], currentColor[1], currentColor[2]), currentLedIndex);
	}
	/*
	String output = "";
	output.concat(volts);
	output.concat(" / ");
	output.concat(currentLedIndex);
	Serial.println(output);
	*/

}

/*-----------------------------------------
	HELPERS
-----------------------------------------*/


/**
 * Clear our LED Strip
 */
void clearStrip() {
	colorWipe(strip.Color(0, 0, 0), strip.numPixels());
}


/**
* Change our LED Strip's colors
*/
void colorWipe(uint32_t c, int index) {
	for (uint16_t i = 0; i< MAXUSEDPIXELS; i++) {
		if (i <index)
			strip.setPixelColor(i, c);
		else
			strip.setPixelColor(i, strip.Color(0, 0, 0));
		strip.show();
	}
}


/**
 * Check for signal incoming from Makey Makey 
 */
boolean contactDone() {
	boolean makeyMakeySentSignal = analogRead(MAKEYMAKEY_DIGITAL_PIN) > 500;
	return makeyMakeySentSignal;
}


/**
* RIGHT EYE
*/
void rightEyeDisplay(int r, int g, int b ) {
	for (uint16_t i = 0; i<rightEye.numPixels() + 2; i++) {
		rightEye.setPixelColor(i, rightEye.Color(r, g, b)); // Draw new pixel
		rightEye.show();
	}
}


/**
 * LEFT EYE
 */
void leftEyeDisplay(int r, int g, int b) {
	for (uint16_t i = 0; i < leftEye.numPixels() + 2; i++) {
		leftEye.setPixelColor(i, leftEye.Color(r, g, b)); // Draw new pixel
		leftEye.show();
	}
}

void closeEyes() {
	rightEyeDisplay(0, 0, 0);
	leftEyeDisplay(0, 0, 0);
}

void startGame() {
	scoreCheckTimer = t.every(ONE_SECOND, checkLevel);
}

/**
 * let's open or close our eyes before actual game start
 */
void blinkEyes() {
	eyesCounter--;
	if (eyesCounter <= 0) {
		t.stop(gameStartTimer);
		closeEyes();
		startGame();
	}
	else {
		if (eyesCounter % 2 == 0) {
			closeEyes();
		}
		else {
			// show eyes
			rightEyeDisplay(eyeColor[0], eyeColor[1], eyeColor[2]);
			leftEyeDisplay(eyeColor[0], eyeColor[1], eyeColor[2]);
		}
	}
}



/**
* Every second, the loudness is checked.
* change colors accordinly
*/
void checkLevel() {
	Serial.println(currentLedIndex);
	if (currentLedIndex > LOUDNESS_THRESHOLD) {
		score++;
		if (score >= PURPLE_LEVEL) {
			Serial.println("PURPLE!");
			currentColor[0] = purple[0];
			currentColor[1] = purple[1];
			currentColor[2] = purple[2];
		}
		else if (score >= BLUE_LEVEL) {
			Serial.println("BLUE!");
			currentColor[0] = blue[0];
			currentColor[1] = blue[1];
			currentColor[2] = blue[2];
		}
		else if (score >= GREEN_LEVEL) {
			Serial.println("GREEN!");
			currentColor[0] = green[0];
			currentColor[1] = green[1];
			currentColor[2] = green[2];
		}
		else if (score >= YELLOW_LEVEL) {
			Serial.println("YELLOW!");
			currentColor[0] = yellow[0];
			currentColor[1] = yellow[1];
			currentColor[2] = yellow[2];
		}
		else if (score >= ORANGE_LEVEL) {
			Serial.println("ORANGE!");
			currentColor[0] = orange[0];
			currentColor[1] = orange[1];
			currentColor[2] = orange[2];
		}
		else if (score >= RED_LEVEL) {
			Serial.println("RED!");
			currentColor[0] = red[0];
			currentColor[1] = red[1];
			currentColor[2] = red[2];
		}
	}
	else {
		resetScore();
	}
}


void resetScore() {
	currentColor[0] = 255;
	currentColor[1] = 255;
	currentColor[2] = 255;
	score = 0;
}
