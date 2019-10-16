

void handleRoot() {
  Serial.println("handleRoot");
  //char msg[1024];
  //snprintf(msg, 1024, "<html><body style=\"text-align:center\"><center><table cellspacing=10><tr><td>Humidity</td><td>Temperature</td></tr><tr><td><h1>%4.2f &percnt;</h1></td><td><h1>%+4.2f &#8451;</h1></td></tr><table></center> <form method=\"post\" action=\"/save\">MQTT Prefix:<br/><input name=\"mqtt_prefix\" value=\"%s\" /><br/>Module Name:<br/><input name=\"module_name\" value=\"%s\" /><br/>Humidity Threshold:<br/><input name=\"humidity_threshold\" value=\"%4.2f\" /><input type=submit name=Submit text=Submit/></form><hr/><a href=/data>Json Data</a>&nbsp;-&nbsp<a href=/reboot>Reboot</a></body></html>", humidity, temperature, mqtt_prefix, module_name, humidityThreshold);
  //server.send(200, "text/html", msg);
  if(SPIFFS.exists("/index.html")){
    File dataFile = SPIFFS.open("/index.html", "r");
    if (!dataFile) {
        server.send(200, "text/html","file index.html open failed");
    }
    else{
      server.streamFile(dataFile, "text/html");
    }
  }
  else{
      server.send(200, "text/html","file index.html not found");
  }
  
    
}

void handleConfig() {
    char jsonConfig[1024];
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["module_name"] = module_name;
    json["mqtt_prefix"] = mqtt_prefix;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_publish_interval"] = mqtt_publish_interval;
    json["humidity_threshold"] = humidityThreshold;
    json.printTo(jsonConfig);
    server.send(200, "application/json", jsonConfig);
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  pinMode(DHTPIN, OUTPUT);
  delay(3000);
  
  ESP.restart();
}


void handleData() {
  char jsonStatus[1024];
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["error"] = errorReadingMeasures;
  if(!errorReadingMeasures)
  {
    json["temperature"] = temperature;
    json["humidity"] = humidity;
    json["heatindex"] = heatIndex;
    json["relaystate"] = relayOn;
  }
  json["humidity_threshold"] = humidityThreshold;
  IPAddress ip = WiFi.localIP();
  String ipString = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  json["ip"] = ipString;
  json.printTo(jsonStatus);
  server.send(200, "application/json", jsonStatus);
}

void handleSave() {
  bool shouldSave = false;
  bool shouldChangeMqtt = false;
  bool shouldRestart = false;
  String message="";
  if(server.hasArg("humidity_threshold")) {
    humidityThreshold = atof(server.arg("humidity_threshold").c_str());
    shouldSave = true;
  }

  if(server.hasArg("mqtt_prefix")) {
    strcpy(mqtt_prefix, server.arg("mqtt_prefix").c_str());
    shouldSave = true;
  }

  if(server.hasArg("mqtt_server")) {
    strcpy(mqtt_server, server.arg("mqtt_server").c_str());
    shouldSave = true;
    shouldChangeMqtt = true;
    
  }

  if(server.hasArg("mqtt_port")) {
    strcpy(mqtt_port, server.arg("mqtt_port").c_str());
    shouldSave = true;
    shouldChangeMqtt = true;
  }

  if(server.hasArg("mqtt_publish_interval")) {
    int val = server.arg("mqtt_publish_interval").toInt();
    if(val!=0)
    {
      mqtt_publish_interval = val;
      shouldSave = true;
    }
  }

  if(server.hasArg("module_name")) {
    strcpy(module_name, server.arg("module_name").c_str());
    shouldSave = true;
  }

  if(shouldSave) {
    SaveConfig();
    message += "\nConfig saved\n";
  }
  if(shouldChangeMqtt)
  {
    mqtt.setHost(mqtt_server, atoi(mqtt_port));
  }
  if(shouldRestart) {
    ESP.reset();
    delay(5000);
  }
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(200, "text/plain", message);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}


void setup_webServer(){
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/reboot", handleReboot);

  server.onNotFound(handleNotFound);
  server.begin();
    
  Serial.println("HTTP server started");
}
