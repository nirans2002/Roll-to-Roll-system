/* Node 1
 *  laser distance sensor 
 *  sends measured distance via to the control Mqtt 
 */

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SparkFun_VL53L1X.h"

//Optional interrupt and shutdown pins.
#define SHUTDOWN_PIN 2
#define INTERRUPT_PIN 3

SFEVL53L1X distanceSensor;
//Uncomment the following line to use the optional shutdown and interrupt pins.
//SFEVL53L1X distanceSensor(Wire, SHUTDOWN_PIN, INTERRUPT_PIN);

// WiFi credentials
const char* ssid = "AM-153";
const char* password = "password";
// MQTT broker
const char* mqtt_server = "makri.local";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
void setup(void)
{
  Wire.begin();
  Serial.begin(115200);
  
  Serial.println("initialising laser distance sensor ....");

  if (distanceSensor.begin() != 0) //Begin returns 0 on a good init
  {
    Serial.println("Sensor failed to begin. Please check wiring. Freezing...");
    while (1)
      ;
  }
  Serial.println("distance sensor online!");

//wifi 
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  randomSeed(micros());

  distanceSensor.setDistanceModeLong();
distanceSensor.setTimingBudgetInMs(50);
distanceSensor.startRanging();
}

void setup_wifi() {

  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void loop(void)
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (distanceSensor.checkForDataReady()) {

    float distance = distanceSensor.getDistance();
    distanceSensor.clearInterrupt();

    Serial.println(distance);

    char buffer[16];
    dtostrf(distance, 1, 2, buffer);

    if (client.connected()) {
      client.publish("/r2r/sag_distance", buffer);
    }
  }
}

void reconnect() {

  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");

    if (client.connect("Node_1")) {

      Serial.println("connected");

//      client.subscribe("/r2r/sag_distance");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");

      delay(5000);
    }
  }
}
