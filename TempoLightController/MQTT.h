
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
