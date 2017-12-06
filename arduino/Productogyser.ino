const static char pubChannel[] = "Channel1"; //choose a name for the channel to publish messages to
int dialValue = 0;   //starting value, change it up!

#include <ArduinoJson.h>
#include <SPI.h>

#include <WiFi101.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
//
static char ssid[] = "Century House";      //SSID of the wireless network
static char pass[] = "5875219970";    //password of that network

//static char ssid[] = "ocadu-embedded";      //SSID of the wireless network
//static char pass[] = "internetofthings";    //password of that network

int status = WL_IDLE_STATUS;                // the Wifi radio's status

const static char pubkey[] = "pub-c-6ccec77b-ca42-473d-a464-74d2db31f511";  //get this from your PUbNub account
const static char subkey[] = "sub-c-d85c438c-d64a-11e7-bcb2-02515ebb3dc0";  //get this from your PubNub account

const static char subChannel[] = "Channel0"; //choose a name for the channel to publish messages to

int ledPinG = 10;                  //encoder Green LED
int ledPinR = 9;                  //encoder Red LED

unsigned long lastSubscribe = 0;
unsigned long lastPublish = 0;
unsigned long DepreciationMarker = 0;
int DepreciationRate = 1510;
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
  neoPixelSetup();
  encoderSetup();
  PubNubSetup();
}

void neoPixelSetup() {
  singlePixel.begin();
  singlePixel.show(); // Initialize all pixels to 'off'
}

void encoderSetup()  {
  // declare pin 9 to be an output:
  pinMode(9, OUTPUT);
  pinMode(encoderPin_A, INPUT_PULLUP);
  pinMode(encoderPin_B, INPUT_PULLUP);
  encoderCurrentTime = millis();
  encoderLoopTime = encoderCurrentTime;
}


void PubNubSetup()
{
  pinMode(ledPinG, OUTPUT);
  pinMode(ledPinR, OUTPUT);
  Serial.begin(9600);
  connectToServer();
}

void loop() {
  neoPixelLoop();
  encoderLoop();
  PubNubloop();
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

    Serial.print("outgoingState: "); Serial.println(dialValue);

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

    // set the brightness of pin 9:
    //analogWrite(9, dialValue);
    //Serial.println(dialValue);
    if (dialValue <= 0) {
      analogWrite(ledPinR, map(dialValue, 0, -255, 0, 255));
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

void PubNubloop()
{


  //analogWrite(ledPin, yourVal2);                  //adjust the value of the led using the value from the server

  //buttonVal = digitalRead(buttonPin);             //read the button


  publishValue = dialValue;                //read the potentiometer
  //myVal2 = random(100,200);                       //store a random value



  //
  // if((buttonVal==1)&&(buttonPrev==0))  //trigger the feed update with a button, uses both current and prev value to only change on the switch
  // {
  // publishToPubNub();                    //send your values to PubNub
  // }



  if (millis() - lastSubscribe >= subscribeRate) //theTimer to trigger the reads
  {
    readFromPubNub();                   //read values from PubNub
    //publishToPubNub();

    lastSubscribe = millis();
  }
  if (millis() - lastPublish >= publishRate) //theTimer to trigger the publish
  {
    //readFromPubNub();                   //read values from PubNub
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
