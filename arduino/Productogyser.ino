//Rotary Encoder code source: http://www.hobbytronics.co.uk/arduino-tutorial6-rotary-encoder
//PubNub code source: https://github.com/DigitalFuturesOCADU/creationANDcomputation/blob/master/Arduino%20Examples/FeatherNetworking/PubNub/_06PubNub_readOnTimer_sendOnPress/_06PubNub_readOnTimer_sendOnPress.ino
//Neopixel code source: https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

const static char pubChannel[] = "Channel1"; //choose a name for the channel to publish messages to
int dialValue = 0;   //starting value.

#include <ArduinoJson.h>
#include <SPI.h>

#include <WiFi101.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>

static char ssid[] = "Lanalgo 2.4";      //SSID of the wireless network
static char pass[] = "zapzapzap";    //password of that network

int status = WL_IDLE_STATUS;                // the Wifi radio's status

const static char pubkey[] = "pub-c-6ccec77b-ca42-473d-a464-74d2db31f511";  //get this from your PUbNub account
const static char subkey[] = "sub-c-d85c438c-d64a-11e7-bcb2-02515ebb3dc0";  //get this from your PubNub account

const static char subChannel[] = "Channel0"; //choose a name for the channel to subscribe to.

int ledPinG = 10;                  //encoder Green LED
int ledPinR = 13;                  //encoder Red LED

unsigned long lastSubscribe = 0;
unsigned long lastPublish = 0;
unsigned long DepreciationMarker = 0;
int DepreciationRate = 1510;
int subscribeRate = 1510;              //refresh rate for PubNub subscribe
int publishRate = 1510;                //refresh rate for PubNub publish

int subscribeValue;                       //variables holding the values coming from the server
int publishValue;                         //variable holding the vaues going to the server

int encoderFadeAmount = 5;    // how many points to fade the LED by
unsigned long encoderCurrentTime;
unsigned long encoderLoopTime;
const int encoderPin_A = 12;  // pin 12
const int encoderPin_B = 11;  // pin 11
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev = 0;

//vvvvvvvvNeoPixel//////

#include <Adafruit_NeoPixel.h>
#define PIN            6

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel singlePixel = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

void setup()  {
  neoPixelSetup();
  encoderSetup();
  PubNubSetup();
}

void neoPixelSetup() {
  singlePixel.begin();
  singlePixel.show(); // Initialize all pixels to 'off'
}

void encoderSetup()  {
  pinMode(ledPinG, OUTPUT);
  pinMode(ledPinR, OUTPUT);
  pinMode(encoderPin_A, INPUT_PULLUP);
  pinMode(encoderPin_B, INPUT_PULLUP);
  encoderCurrentTime = millis();
  encoderLoopTime = encoderCurrentTime;
}

void PubNubSetup()
{
  Serial.begin(9600);
  connectToServer();
}

void loop() {
  neoPixelLoop();
  encoderLoop();
  PubNubloop();
  Depreciation();
}

void Depreciation() { //depreciates user focusDesire over time, creating a 'dead man's switch'. Any value reverts to 0 over time.
  if (millis() - DepreciationMarker >= DepreciationRate) //theTimer to trigger the reads
  {
    if (dialValue < 0) {
      ++dialValue;
    }

    if (dialValue > 0) {
      --dialValue;
    }

    Serial.print("outgoingState: "); Serial.println(dialValue);

    DepreciationMarker = millis();
  }
}

void neoPixelLoop() { //sets neopixel to display global office state.
  if (subscribeValue < 0) {
    singlePixel.setPixelColor(0, singlePixel.Color(255, 0, 0)); // red
    singlePixel.setBrightness(abs(subscribeValue));

  }
  if (subscribeValue > 0) {
    singlePixel.setPixelColor(0, singlePixel.Color(0, 255, 0)); // green
    singlePixel.setBrightness(subscribeValue);

  }

  singlePixel.show(); // This sends the updated pixel color to the hardware.
  if (subscribeValue == 0) {
    singlePixel.setBrightness(0);
  }

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

    // set the brightness of pin 9:
    //analogWrite(9, dialValue);
    //Serial.println(dialValue);
    //Serial.println(dialValue);

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


}

void PubNubloop()
{


  publishValue = dialValue;                //read the potentiometer

  if (millis() - lastSubscribe >= subscribeRate) //the timer to trigger the reads
  {
    readFromPubNub();                   //read values from PubNub
    lastSubscribe = millis();
  }
  if (millis() - lastPublish >= publishRate) //the timer to trigger the publish
  {
    publishToPubNub();
    lastPublish = millis();
  }

}


void connectToServer()
{
  WiFi.setPins(8, 7, 4, 2); //This is specific to the feather M0

  status = WiFi.begin(ssid, pass);                    //attempt to connect to the network
  Serial.println("***Connecting to WiFi Network***");


  for (int trys = 1; trys <= 10; trys++)                 //use a loop to attempt the connection more than once
  {
    if ( status == WL_CONNECTED)                        //check to see if the connection was successful
    {
      Serial.print("Connected to ");
      Serial.println(ssid);

      PubNub.begin(pubkey, subkey);                      //connect to the PubNub Servers
      Serial.println("PubNub Connected");
      break;                                             //exit the connection loop
    }
    else
    {
      Serial.print("Could Not Connect - Attempt:");
      Serial.println(trys);

    }

    if (trys == 10)
    {
      Serial.println("I don't this this is going to work");
    }
    delay(1000);
  }


}


void publishToPubNub()
{
  WiFiClient *client;
  StaticJsonBuffer<800> messageBuffer;                    //create a memory buffer to hold a JSON Object
  JsonObject& pMessage = messageBuffer.createObject();    //create a new JSON object in that buffer

  ///the imporant bit where you feed in values
  pMessage["focusDesire"] = publishValue;                      //add a new property and give it a value
  //pMessage["randoVal2"] = myVal2;                     //add a new property and give it a value


  ///                                                       //you can add/remove parameter as you like

  //pMessage.prettyPrintTo(Serial);   //uncomment this to see the messages in the serial monitor


  int mSize = pMessage.measureLength() + 1;                   //determine the size of the JSON Message
  char msg[mSize];                                            //create a char array to hold the message
  pMessage.printTo(msg, mSize);                              //convert the JSON object into simple text (needed for the PN Arduino client)

  client = PubNub.publish(pubChannel, msg);                      //publish the message to PubNub

  if (!client)                                                //error check the connection
  {
    Serial.println("client error");
    delay(1000);
    return;
  }

  if (PubNub.get_last_http_status_code_class() != PubNub::http_scc_success)  //check that it worked
  {
    Serial.print("Got HTTP status code error from PubNub, class: ");
    Serial.print(PubNub.get_last_http_status_code_class(), DEC);
  }

  while (client->available())
  {
    Serial.write(client->read());
  }
  client->stop();
  Serial.println("Successful Publish");



}


void readFromPubNub()
{
  StaticJsonBuffer<1200> inBuffer;                    //create a memory buffer to hold a JSON Object
  PubSubClient *sClient = PubNub.subscribe(subChannel);

  if (!sClient) {
    Serial.println("message read error");
    delay(1000);
    return;
  }

  while (sClient->connected())
  {
    while (sClient->connected() && !sClient->available()) ; // wait
    char c = sClient->read();
    JsonObject& sMessage = inBuffer.parse(*sClient);

    if (sMessage.success())
    {
      //sMessage.prettyPrintTo(Serial); //uncomment to see the JSON message in the serial monitor
      subscribeValue = sMessage["state"];  //store the value from the JSON into the variable
      Serial.print("incomingState: ");        //you need to know the name of the parameter in the message for this to work
      Serial.println(subscribeValue);
      // yourVal2 = sMessage["randoVal2"];  //this is the one that connects to the LED
      // Serial.print("randoVal2 ");
      // Serial.println(yourVal2);
      break;

    }


  }

  sClient->stop();

}
