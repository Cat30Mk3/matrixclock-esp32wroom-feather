#pragma once

// Template for public repo - copy to secrets.h and fill in real credentials
// secrets.h is git-ignored and never committed
// The configDb_t struct is defined in globals.h

// Dummy/template values - copy secrets_template.h to secrets.h and fill in real values
configDb_t configDb = {
  {"pub/Topic1", "sub/Topic1", "sub/Topic2"},  // pub / sub topics must match those sent by the MQTT devices
  "YOUR_SSID",  // ssid
  "YOUR_PASSWORD",  // password
  "mail.smtp2go.com",  // smtpServer
  "dummy@example.com",  // smtpUserId
  "DUMMY_PASSWORD",  // smtpPassword
  "sender@example.com",  // sendingAddress
  "receiver@example.com",  // receivingAddress
  "mqtt.example.com",  // mqttServer
  "DUMMY_MQTT_USER_ID",  // mqttUserId
  "dummyClientId",  // mqttClientId
  "DUMMY_MQTT_PASSWORD",  // mqttPassword
  "cmnd/Dummy_1",  // lampCmndTopic
  "stat/Dummy_1"  // lampStatTopic
};
