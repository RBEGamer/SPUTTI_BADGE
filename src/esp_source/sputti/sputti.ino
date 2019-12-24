
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>


#include <ArduinoOTA.h>


#include <Adafruit_NeoPixel.h>
#define LED_PIN    2 //PIN FOR NEOPIXEL LEDS
#define LED_COUNT 14 //LED COUNT
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ400);

#define VERSION "V4"
#define WIFI_PW "SPUTTISPUTTI"


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

int animation = 1;
bool wifi_state = false;
bool break_loop = false;
bool animation_changed = false;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
          if(value == "0"){
          animation = 0;
          }else if(value == "1"){
          animation = 1;
          }else if(value == "2"){
          animation = 2;
          }else if(value == "3"){
          animation = 3;
          }else if(value == "4"){
          animation = 4;
          }else if(value == "5"){
          animation = 5;
          }else if(value == "6"){
          animation = 6;
          }else if(value == "7"){
          animation = 7;

          }else if(value == "99"){
          animation = 99;
          }else if(value == "100"){
          animation = 100;
          }
        animation_changed = true;
      break_loop = true;
        
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

 String name_compl = "SPUTTI";

 
void setup() {
  Serial.begin(115200);

  uint64_t chipid=ESP.getEfuseMac();
  
    strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();            // Turn OFF all pixels ASAP
    strip.setBrightness(10);

  strip.setPixelColor(0,strip.Color(  0,   0,   10));strip.show();
  

  
  Serial.println("Starting BLE work!");
  
  name_compl = "SPUTTI_" + String((uint32_t)chipid) +"_"+ VERSION;
  BLEDevice::init(name_compl.c_str());
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pCharacteristic->setValue("0");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
      if(break_loop){return;}
    }
  }
}


void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
    if(break_loop){return;}
  }
}


void rainbow(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
    if(break_loop){return;}
  }
}


void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < LED_COUNT; j++) {
    strip.setPixelColor(j, heat[j] );
  }

  strip.show();
  delay(SpeedDelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    strip.setPixelColor(Pixel, strip.Color(255, 255, heatramp));
  } else if( t192 > 0x40 ) {             // middle
    strip.setPixelColor(Pixel, strip.Color(255, heatramp, 0));
  } else {                               // coolest
    strip.setPixelColor(Pixel,strip.Color( heatramp, 0, 0));
  }
}



void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position=0;
  
  for(int j=0; j<LED_COUNT*2; j++)
  {
      Position++; // = 0; //Position + Rate;
      for(int i=0; i<LED_COUNT; i++) {
        // sine wave, 3 offset waves make a rainbow!
        //float level = sin(i+Position) * 127 + 128;
        //setPixel(i,level,0,0);
        //float level = sin(i+Position) * 127 + 128;
        strip.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*red,
                   ((sin(i+Position) * 127 + 128)/255)*green,
                   ((sin(i+Position) * 127 + 128)/255)*blue);
      }
      
      strip.show();
      delay(WaveDelay);
  }
}

void Twinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
  strip.clear();

  
  for (int i=0; i<Count; i++) {
     strip.setPixelColor(random(LED_COUNT),random(5*65536));
     strip.show();
     delay(SpeedDelay);
     if(OnlyOne) { 
       strip.clear();
     }
   }
  
  delay(SpeedDelay);
}




void loop() {

  if(animation_changed){
    strip.setBrightness(10);
    strip.clear();
    strip.show();
    animation_changed = false;
    }
  // put your main code here, to run repeatedly:
  if(animation == 0){
    strip.setBrightness(10);
    strip.clear();
    strip.show();
    delay(300);
    return;
   }

 if(animation == 1){
  rainbow(100);
  }
  
  if(animation == 2){
    theaterChase(strip.Color(127, 127, 127), 200);
  }

  if(animation == 3){
  for(int i = 0;i< LED_COUNT;i++){
    strip.setPixelColor(i,strip.Color(50, 50, 50));
     strip.show();
     delay(100);
  }
  }

 if(animation == 4){
  strip.setBrightness(255);
  for(int i = 0;i< LED_COUNT;i++){
    strip.setPixelColor(i,strip.Color(255, 255, 255));
     strip.show();
     delay(100);
  }
  strip.setBrightness(10);
  }


   if(animation == 5){
    Fire(55,120,350);
  }

   if(animation == 6){
    RunningLights(0xff,0xff,0xff, 150);  // white
    }


if(animation == 7){
  Twinkle(0xff, 0, 0, 10, 100, false);
  }

if(animation == 100){
  btStop();
  animation = 1;
  
  WiFi.mode( WIFI_MODE_NULL );
  Serial.println("-- SHITDOWN WIFI --");
  wifi_state = false;
  }
  
if(wifi_state && animation != 99){
  WiFi.mode( WIFI_MODE_NULL );
  Serial.println("-- SHITDOWN WIFI --");
  wifi_state = false;
  strip.clear();
  strip.setPixelColor(0,strip.Color(127, 0, 0));
  strip.show();
  }

  if(animation == 99){
    //SETUP OTA
    if(!wifi_state){
      WiFi.softAP(name_compl.c_str(), WIFI_PW);
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      //SETUP OTA PARAMETERS
      ArduinoOTA.setHostname(name_compl.c_str());
      wifi_state = true;
      Serial.print("-- STARTING OTA --");
      ArduinoOTA.begin();
      strip.clear();
      strip.setPixelColor(0,strip.Color(0, 127, 0));
      strip.show();
      }
    //HANDLE OTA
    if(wifi_state){
      ArduinoOTA.handle();
      }
    
   }
  break_loop = false;
  delay(50);
}
