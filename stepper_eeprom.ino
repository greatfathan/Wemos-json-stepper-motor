#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <Stepper.h>
#include <EEPROM.h>

// Define pin motor stepper
#define PIN_STEPPER_D1 D1
#define PIN_STEPPER_D2 D2
#define PIN_STEPPER_D3 D3
#define PIN_STEPPER_D4 D4

// Kalibrasi putaran sesuai rasio gigi pada motor
// 32 langkah untuk mencapai 1 putaran penuh
#define STEPSMOTOR 32 

// membuat langkah detail dengan tipe data float 
float STEPS_OUTPUT = STEPSMOTOR*64; // gear ratio dari motor stepper

// init stepper
//Stepper stepper(STEPSMOTOR,PIN_STEPPER_D1,PIN_STEPPER_D3,PIN_STEPPER_D2,PIN_STEPPER_D4); // pin pin yang terhubung
Stepper stepper(STEPSMOTOR, D1, D3, D2, D4);

// WIfi Multi
ESP8266WiFiMulti WiFiMulti;

// init internal LED
int ledState = LOW;

/**
 * STRUCT untuk penyimpanan data EEPROM
 */
typedef struct {
  String deviceId;
  String currentData;
} EEPROMData;

/**
 * Fungsi untuk membuat format json yang akan disimpan kedalam EEPROM
 */
String _GenerateJsonData(String deviceId, String currentData) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  root["DEVICE_ID"] = deviceId;
  root["CURR_DATA"] = currentData;
  
  String output;
  root.printTo(output);
  return output;
}

/**
 * Overload
 * Fungsi untuk generate string json ke json object
 */
EEPROMData _ConvertStrJsonToObject(String strJson) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(strJson);
  
  EEPROMData eedata;
  if(root.containsKey("DEVICE_ID")) { 
   const char* dvc_id = root["DEVICE_ID"];
   eedata.deviceId = dvc_id;
  }
  if(root.containsKey("CURR_DATA")) { 
    const char* cur_dt = root["CURR_DATA"];
    eedata.currentData = cur_dt;
  }
  return eedata;
}

/**
 * Fungsi untuk init panjang memory EEPROM
 */
void initEEPROMData() {
    EEPROM.begin(1024);
}

/**
 * Fungsi untuk menge-set EEPROM dengan inputan yang sudah ditentukan
 */
void setEEPROMData(String deviceID, String currentData){ 
  String str = _GenerateJsonData(deviceID,currentData);
  if(str.length() < EEPROM.length()) {
    for(int i = 0; i < str.length(); i++) {
      char item = str.charAt(i);
      EEPROM.write(i,item);
    }
  }
  EEPROM.commit();
}

/**
 * Fungsi untuk mengambil data dari EEPROM dengan output STRUCT
 */
EEPROMData getEEPROMData(){  
  String strJson = "";
  for(int i = 0; i < EEPROM.length(); i++) {
    uint8_t e = EEPROM.read(i);
    if(e < 1) break;
    strJson += (char)e;
  }

  EEPROMData result = _ConvertStrJsonToObject(strJson);
  return result;
}

/**
 * Fungsi untuk mereset EEPROM agar kembali 0
 */
void resetEEPROMData() {
  for(int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i,0);
  }
}


//// fungsi untuk merubah rotate
//void setRotate(int degree) {
//  float STEPS_START = (STEPS_OUTPUT/360)*degree;
//  stepper.setSpeed(700); // kecepatan putaran maksimal
//  stepper.step(STEPS_START); // memerintahkan untuk berputar
//}

// fungsi untuk merubah rotate
void setRotate(int degree) {
  float STEPS_START = (STEPS_OUTPUT/360)*degree;
  
  if(degree > 0) {
    for(int i = 0; i < STEPS_START; i++) {
      stepper.setSpeed(700); // kecepatan putaran maksimal
      stepper.step(1); // memerintahkan untuk berputar
      delay(1);
    }
  }

  if(degree < 0) {
    for(int i = 0; i > STEPS_START; i--) {
      stepper.setSpeed(700); // kecepatan putaran maksimal
      stepper.step(-1); // memerintahkan untuk berputar
      delay(1);
    }
  }
}

void setup() {
  // init eeprom
  initEEPROMData();
  // put your setup code here, to run once:
  Serial.begin(115200);
  // initialize onboard LED as output
  pinMode(BUILTIN_LED, OUTPUT);
  // set Wifi SSID dan passwordnya
  WiFiMulti.addAP("INFINET", "satuduatiga");
  WiFiMulti.addAP("LAB-F", "excellent123");
  WiFiMulti.addAP("B75", "ericsson1");
  WiFiMulti.addAP("Tisha", "tishanenen");

  // set pin mode
  pinMode(PIN_STEPPER_D1, OUTPUT);
  pinMode(PIN_STEPPER_D2, OUTPUT);
  pinMode(PIN_STEPPER_D3, OUTPUT);
  pinMode(PIN_STEPPER_D4, OUTPUT);
}

String getData(String url) {
    HTTPClient http;
    String json;
    
    // ganti dengan URL API Last Feed punyamu sendiri
    http.begin(url);

    // mulai koneksi dan ambil HTTP Header
    int httpCode = http.GET();
    
    // httpCode akan bernilai negatif bila error
    if(httpCode > 0)
    {
        // cetak httpCode ke Serial
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // bila nilai dari server diterima
        if(httpCode == HTTP_CODE_OK)
        {
            // cetak string json dari server
            json = http.getString();
            Serial.println(json);
        }

    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    // tutup koneksi HTTP
    http.end();

    return json;
}

String * parseJson (String json) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()){
      Serial.printf("parseObject() failed\n");
      // return NULL;
    }

    // semua value bertipe data char pointer
    const char* device_id = root["device_id"];
    const char* device_status = root["device_status"];
    const char* device_value = root["device_value"];
    const char* device_rand = root["device_rand"];

    String* results = new String[4];
    results[0] = device_id;
    results[1] = device_value;
    results[2] = device_status;
    results[3] = device_rand;
    return results;
} 

void loop() {
    // put your main code here, to run repeatedly:
    // tunggu koneksi Wifi
    if((WiFiMulti.run() == WL_CONNECTED))
    {            
      
        // toggle the LED
        ledState = !ledState;
        digitalWrite(BUILTIN_LED, ledState);
        
        // debug notification
        Serial.printf("Succesfully Connected Wifi\n");  
        Serial.print("Connecting to : \t");      
        Serial.println(WiFi.SSID());
        Serial.print("ip address : \t");      
        Serial.println(WiFi.localIP());        
        
        String * data = parseJson(getData("http://iot.clothingkita.com/rest.php?action=detail&id=IOT01"));
        String deviceId = data[0];
        int valueReading = data[1].toInt();
        String device_rand = data[3];
        device_rand.trim();
        
        if (device_rand != "") {
          EEPROMData o = getEEPROMData();
          Serial.print(device_rand);Serial.print("AND");Serial.println(o.currentData);
          if(!o.currentData.equals(device_rand)) {
            setRotate(valueReading);
            // save data to eeprom
            setEEPROMData(deviceId,String(device_rand));
          }   
        }
        
             
    } 
    else {
      Serial.printf("Failed Connected Wifi\n");
    }

    // Give delay 5 Second
    delay(5000);
}
