

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "esp_bt_main.h"
#include "esp_bt_device.h"


#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>


#include <Adafruit_NeoPixel.h>
#define LED_PIN    2 //PIN FOR NEOPIXEL LEDS
#define LED_COUNT 14 //LED COUNT
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ400);

#define VERSION "V7"
#define WIFI_PW "SPUTTISPUTTI"
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_ANIMATION "beb5483e-36e1-4688-b7f5-ea07361b26a8"

int animation = 1;
bool wifi_state = false;
bool break_loop = false;
bool animation_changed = false;



WiFiClient client;
long contentLength = 0;
bool isValidContentType = false;

const char* SSID = "SPUTTI";
const char* PSWD = "00000000";

// S3 Bucket Config
String host = "sputtifirmware.s3.eu-central-1.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
String bin = "/SPUTTI_FIRMWARE.bin"; // bin file name with a slash in front.

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

// OTA Logic 
void execOTA() {
  Serial.println("Connecting to: " + String(host));
  // Connect to S3
  if (client.connect(host.c_str(), port)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(bin));

    // Get the contents of the bin file
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        client.stop();
        return;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}

    */
    while (client.available()) {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
        if (contentType == "application/macbinary") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);

    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
}















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



void printDeviceAddress() {
  Serial.println("--- BLE ADDR ---");
  const uint8_t* point = esp_bt_dev_get_address();
  for (int i = 0; i < 6; i++) {
    char str[3];
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);
    if (i < 5){
      Serial.print(":");
    }
  }
}



 
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

  BLECharacteristic *pCharacteristic_animationstate = pService->createCharacteristic(CHARACTERISTIC_UUID_ANIMATION,BLECharacteristic::PROPERTY_READ |BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic_animationstate->setCallbacks(new MyCallbacks());

  pCharacteristic_animationstate->setValue(String(animation).c_str());
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();


  printDeviceAddress();

  Serial.printf("ESP32 Chip ID = %04X",(uint16_t)(chipid>>32));//print High 2 bytes
  Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.



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
  if(break_loop){return;}
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
        strip.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*red,
                   ((sin(i+Position) * 127 + 128)/255)*green,
                   ((sin(i+Position) * 127 + 128)/255)*blue);
      }
      
      strip.show();
      delay(WaveDelay);
      if(break_loop){return;}
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
  if(break_loop){return;}
}




void loop() {

  if(animation_changed){
    strip.setBrightness(10);
    strip.clear();
    strip.show();
    animation_changed = false;
    }


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
  
if(WiFi.status() == WL_CONNECTED && animation != 99){
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
      WiFi.begin(SSID, PSWD);

      while (WiFi.status() != WL_CONNECTED) {
        Serial.print("."); // Keep the serial monitor lit!
        strip.clear();
        strip.setPixelColor(0,strip.Color(0, 127, 0));
        strip.setPixelColor(1,strip.Color(127, 0, 0));
        strip.show();
        delay(500);
        strip.setPixelColor(0,strip.Color(127, 0, 0));
        strip.setPixelColor(1,strip.Color(0, 127, 0));
        strip.show();
      }

  
      IPAddress IP = WiFi.softAPIP();
      wifi_state = true;
      Serial.print("-- STARTING OTA --");
      strip.clear();
      strip.setPixelColor(0,strip.Color(0, 127, 0));
      strip.show();
    
      execOTA();
      }
   
    
   }
  break_loop = false;
  delay(100);
}
