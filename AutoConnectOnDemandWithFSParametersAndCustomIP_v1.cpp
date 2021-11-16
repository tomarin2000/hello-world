#include <LittleFS.h>             //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

char mqtt_server[40];
char mqtt_port[6] = "8080";
char api_token[34] = "YOUR_APITOKEN";

char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";

#define TRIGGER_PIN 0

unsigned int  timeout   = 30; // seconds to run for

WiFiManager wifiManager;


WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 34);

bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void saveParamConfigCallback () {
  Serial.println("Should save params config");
  shouldSaveConfig = true;
  wifiManager.stopConfigPortal();
}

void setup() {
  Serial.begin(115200);
  
  wifiManager.setParamsPage(true);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setSaveParamsCallback(saveParamConfigCallback);
 
  if (LittleFS.begin()) {
    if (LittleFS.exists("/config.json")) {
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
#if ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(api_token, json["api_token"]);
          if (json["ip"]) {
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
          } 
        } 
      }
    }
  } 

  Serial.println(static_ip);
  Serial.println(api_token);
  Serial.println(mqtt_server);

  IPAddress _ip, _gw, _sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_api_token);

  custom_mqtt_server.setValue(mqtt_server, 40);
  custom_mqtt_port.setValue(mqtt_port, 6);
  custom_api_token.setValue(api_token, 34);

  wifiManager.setMinimumSignalQuality();

  if (wifiManager.autoConnect()){
    Serial.println("connected...yeey :)");
  }
}

void loop() {

  if (digitalRead(TRIGGER_PIN) == LOW) {
  
    wifiManager.setConfigPortalTimeout(120);
  
    if (!wifiManager.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
    }else{
      Serial.println("connected...yeey :)");
    }
  
    
    if (shouldSaveConfig) {
      Serial.println("saving config");
  #if ARDUINOJSON_VERSION_MAJOR >= 6
      DynamicJsonDocument json(1024);
  #else
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
  #endif
  
      json["mqtt_server"] = custom_mqtt_server.getValue();
      json["mqtt_port"] = custom_mqtt_port.getValue();
      json["api_token"] = custom_api_token.getValue();
  
      json["ip"] = WiFi.localIP().toString();
      json["gateway"] = WiFi.gatewayIP().toString();
      json["subnet"] = WiFi.subnetMask().toString();
  
      File configFile = LittleFS.open("/config.json", "w");
  
  #if ARDUINOJSON_VERSION_MAJOR >= 6
      serializeJson(json, Serial);
      serializeJson(json, configFile);
  #else
      json.printTo(Serial);
      json.printTo(configFile);
  #endif
      configFile.close();
    }
    shouldSaveConfig = false; 
  }
}
