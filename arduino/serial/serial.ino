/* 
 *  Serial version of the mags arduino interface
 *  
 */

int dialValue = 0;   //starting value, change it up!

#include <ArduinoJson.h>
#include <SPI.h>

int ledPinG = 10;                  //encoder Green LED
int ledPinR = 13;                  //encoder Red LED

unsigned long lastSubscribe = 0;
unsigned long lastPublish = 0;
unsigned long DepreciationMarker = 0;
int DepreciationRate = 1000;
int subscribeRate = 1510;              //refresh rate for reading values
int publishRate = 1510;              //refresh rate for reading values

int subscribeValue;                       //variables holding the values coming from the server
int publishValue;                         //variable holding the vaues going to the server

int encoderFadeAmount = 10;    // how many points to fade the LED by
unsigned long encoderCurrentTime;
unsigned long encoderLoopTime;
const int encoderPin_A = 12;  // pin 12
const int encoderPin_B = 11;  // pin 11
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev = 0;

//vvvvvvvvNeoPixel//////

#include <Adafruit_NeoPixel.h>
int neoPin = 6;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel singlePixel = Adafruit_NeoPixel(1, neoPin, NEO_KHZ800);

void setup()  {
  Serial.begin(9600);

  pinMode(ledPinG, OUTPUT);
  pinMode(ledPinR, OUTPUT);
  
  neoPixelSetup();
  encoderSetup();
}

void neoPixelSetup() {
  singlePixel.begin();
  singlePixel.show(); // Initialize all pixels to 'off'
}

void encoderSetup()  {
  // declare pin 9 to be an output:
  //pinMode(9, OUTPUT);
  pinMode(encoderPin_A, INPUT_PULLUP);
  pinMode(encoderPin_B, INPUT_PULLUP);
  encoderCurrentTime = millis();
  encoderLoopTime = encoderCurrentTime;
}


void loop() {
  neoPixelLoop();
  encoderLoop();
  Depreciation();
}

void Depreciation() {
  if (millis() - DepreciationMarker >= DepreciationRate) //theTimer to trigger the reads
  {
    if (dialValue < 0) {
      ++dialValue;
    }

    if (dialValue > 0) {
      --dialValue;
    }

    //Serial.print("depreciate: "); 
    Serial.write((255+dialValue)/2);

    DepreciationMarker = millis();
  }
}

void neoPixelLoop() {
  //singlePixel.setPixelColor(n, red, green, blue); //RGB strips
  //singlePixel.setPixelColor(n, red, green, blue, white); // RGB + W strips
  if (subscribeValue >= 0) {
    singlePixel.setPixelColor(0, 0, 255, 0);
    singlePixel.setBrightness(subscribeValue);
  }
  else {
    singlePixel.setPixelColor(0, 255, 0, 0);
    singlePixel.setBrightness(map(subscribeValue, -1, -255, 0, 255));
  }


  //uint32_t magenta = .Color(255, 0, 255);
  //You can also convert separate red, green and blue values into a single 32-bit type for later use:
  singlePixel.show();//This updates the whole strip at once
  //You can query the color of a previously-set pixel using getPixelColor():
  //uint32_t color = singlePixel.getPixelColor(11);

  //The overall brightness of all the LEDs can be adjusted using setBrightness()
  singlePixel.setBrightness(subscribeValue);

}

void encoderLoop()  {
  // get the current elapsed time
  encoderCurrentTime = millis();
  if (encoderCurrentTime >= (encoderLoopTime + 5)) {
    // 5ms since last check of encoder = 200Hz
    encoder_A = digitalRead(encoderPin_A);    // Read encoder pins
    encoder_B = digitalRead(encoderPin_B);
    if ((!encoder_A) && (encoder_A_prev)) {
      // A has gone from high to low
      if (encoder_B) {
        // B is high so clockwise
        // increase the brightness, dont go over 255
        if (dialValue + encoderFadeAmount <= 255) dialValue += encoderFadeAmount;
      }
      else {
        // B is low so counter-clockwise
        // decrease the brightness, dont go below 0
        if (dialValue - encoderFadeAmount >= -255) dialValue -= encoderFadeAmount;
      }

    }
    encoder_A_prev = encoder_A;     // Store value of A for next time

      

    if (dialValue <= 0) {
      analogWrite(ledPinR, abs(dialValue));
      analogWrite(ledPinG, 0);
      
    }

    else {
      analogWrite(ledPinG, dialValue);
      analogWrite(ledPinR, 0);

    }
    encoderLoopTime = encoderCurrentTime;  // Updates encoderLoopTime
  }
  // Other processing can be done here


}

