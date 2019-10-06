

char* getConfig()
{
     if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("Reading config file config.json");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        return buf.get();
      }
     }
}

void LoadConfig()
{
  
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("Reading config file config.json");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nParsed json");

          strcpy(module_name, json["module_name"]);
          strcpy(mqtt_prefix, json["mqtt_prefix"]);
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          humidityThreshold = json["humidity_threshold"];
          mqtt_publish_interval = json["mqtt_publish_interval"];

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
}


void SaveConfig()
{
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["module_name"] = module_name;
    json["mqtt_prefix"] = mqtt_prefix;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_publish_interval"] = mqtt_publish_interval;
    json["humidity_threshold"] = humidityThreshold;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

  
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
}
