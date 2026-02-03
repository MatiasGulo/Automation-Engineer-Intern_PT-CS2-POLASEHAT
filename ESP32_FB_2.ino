#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "Arduino.h"
#include "stdlib.h"
#include <SoftwareSerial.h>
#include <ModbusMaster.h>

// Insert your network credentials
#define WIFI_SSID "Matias"
#define WIFI_PASSWORD "######"

// Insert Firebase project API Key
#define API_KEY "######"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "######" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
String MNL;
float valueSpeed;
float valuePercentage;

SoftwareSerial portOne(16, 17);
ModbusMaster node;
String receivedCommand;

void preTransmission()
{
  digitalWrite(5, HIGH);
}

void postTransmission()
{
  digitalWrite(5, LOW);
}

void setup() {
  Serial.begin(38400);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);
  portOne.begin(9600);
  node.begin(8, portOne);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

void loop() {
  uint8_t result;
  result = node.readInputRegisters(0x064, 10);
  if(result == node.ku8MBSuccess) {
    
    //Persentase
    int a = node.getResponseBuffer(0x04);
    String b = String(a, HEX);
    char all[50];
   
    sprintf(all, "%s0000", b);

    uint32_t valueDataPercentage = strtoul(all, NULL, 16);
    memcpy(&valuePercentage, &valueDataPercentage, sizeof(valueDataPercentage));
    Serial.println(valuePercentage);

    //Kecepatan m3/h
    int x = node.getResponseBuffer(0x00);
    int GH = node.getResponseBuffer(0x01);
    String y = String(x, HEX);
    String KL = String(GH, HEX);
    char all_2[50];

    sprintf(all_2, "%s%s", y, KL);

    uint32_t valueDataSpeed = strtoul(all_2, NULL, 16);
    memcpy(&valueSpeed, &valueDataSpeed, sizeof(valueDataSpeed));
    Serial.println(valueSpeed);

    //Total Flow Rate
    int IOP, AAK;
        
    IOP = node.getResponseBuffer(0x09);
    AAK = ((node.getResponseBuffer(0x08) * 65536 + node.getResponseBuffer(0x07)));
        
    MNL = String(AAK) + "." + String(IOP);
    Serial.println(MNL);
 }
 if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "nilai/persentase", valuePercentage)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "nilai/flowspeed", valueSpeed)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    if (Firebase.RTDB.setString(&fbdo, "nilai/total_flow", MNL)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}
