
void mqtt_connect(const char* server, const char* port, const char* name) {

  Serial.println();
  Serial.print("Connecting to MQTT server : "); Serial.print(server); Serial.print(":"); Serial.println(port);

  mqtt.begin(server,atoi(port),wifi);

  while (!mqtt.connect(name)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("MQTT client connected.");

}


void publishHumidity(float humidity) {
    if (mqtt.connected()) {
      char topic[50],payload[15];
      sprintf(topic,"%s/%s/%s",mqtt_prefix,module_name,"humidity");
      sprintf(payload,"%s",f2s(humidity,2));
      mqtt.publish(topic,payload, false, 2);
    }
}

void publishTemperature(float temperature) {
    if (mqtt.connected()) {
      char topic[50],payload[15];
      sprintf(topic,"%s/%s/%s",mqtt_prefix,module_name,"temperature");
      sprintf(payload,"%s",f2s(temperature,2));
      mqtt.publish(topic,payload, false, 2);
    }
}

void publishHeatIndex(float heatIndex) {
    if (mqtt.connected()) {
      char topic[50],payload[15];
      sprintf(topic,"%s/%s/%s",mqtt_prefix,module_name,"heatindex");
      sprintf(payload,"%s",f2s(heatIndex,2));
      mqtt.publish(topic,payload, false, 2);
    }
}

void publishMeasure(){
    if (mqtt.connected()) {
      char topic[50],payload[15];  
      sprintf(topic,"%s/%s/%s",mqtt_prefix,module_name,"measure");
      char msg[256];
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
  
      json["temperature"] = temperature;
      json["humidity"] = humidity;
      json["heatindex"] = heatIndex;
      json.printTo(msg);
      json.printTo(Serial);
      mqtt.publish(topic,msg, false, 2);
    }
}

void publishRelayState(bool state) {
    if (mqtt.connected()) {
      char topic[50],payload[15];
      sprintf(topic,"%s/%s/%s",mqtt_prefix,module_name,"vmc");
      if(state) {
        sprintf(payload,"%s","on");
      }else {
        sprintf(payload,"%s","off");
      }
      mqtt.publish((const char*)topic, (const char*)payload, true, 2);
    }
}

void publishIPAddress() {
  char topic[50],payload[15];
  sprintf(topic,"%s/%s/%s",mqtt_prefix,module_name,"ip");
  sprintf(payload,"%s", WiFi.localIP().toString().c_str());
  mqtt.publish((const char*)topic, (const char*)payload, true, 2);
  
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {

  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();

}
