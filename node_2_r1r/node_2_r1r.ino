#define EncoderPinA 33

#define motor1Pin1 26 
#define motor1Pin2 27 
#define enable1Pin 14 

#define CURRENT_SENSOR_PIN 15

//#define LED 2

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int oneWireBus = 18;

// WiFi credentials
const char* ssid = "AM-153";
const char* password = "password";

// MQTT broker 
const char* mqtt_server = "makri.local";

OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);


WiFiClient espClient;
PubSubClient client(espClient);

const float Vref = 3.3;
const int ADC_RES = 4095;

// 5A version sensitivity
const float sensitivity = 0.185;
float offsetVoltage = 1.65;


// Topics
const char* sub_topic = "/r2r/sag_distance";

const char* pub_speed = "/r2r/motor_speed";

const char* pub_current = "/r2r/motor_current";

const char* pub_vib_x = "/r2r/motor_vib_x";
const char* pub_vib_y = "/r2r/motor_vib_y";
const char* pub_vib_z = "/r2r/motor_vib_z";

const char* pub_temp = "/r2r/motor_temp";


// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 80;


volatile long encoderCount = 0;

unsigned long prevTime = 0;
float rpm = 0;

// Set this correctly based on your encoder
const int PPR = 360;  

// Variables
float curr_motor_speed= 0;
float motor_current = 0;
float motorSpeed = 0;
float motor_temp = 0;
float vib_x =0;
float vib_y =0;
float vib_z =0;

// -------- CONTROL --------
float sag_measured = 0.0;
float SAG_SET = 100.0;   // desired sag
float Kp = 10.0;        // tune this


Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);


void IRAM_ATTR updateEncoder() {
  encoderCount++;   // only counting pulses
}


void calc_speed() {
  unsigned long currentTime = millis();

  if (currentTime - prevTime >= 1000) {  // 1 second window
    noInterrupts();
    long count = encoderCount;
    encoderCount = 0;
    interrupts();

    rpm = (count * 60.0) / PPR;

//    Serial.print("RPM: ");
//    Serial.println(rpm);

    prevTime = currentTime;
  }
}
// ----------- WiFi Setup -----------
void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
  }
//digitalWrite(LED,HIGH);
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
}

// ----------- MQTT Callback (Subscriber) -----------
void callback(char* topic, byte* payload, unsigned int length) {

//  Serial.print("Message received on topic: ");
//  Serial.println(topic);

  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

//  Serial.print("Message: ");
//  Serial.println(msg);

  sag_measured = msg.toFloat();
  // Example: control motor speed from incoming message
//  if (String(topic) == sub_topic) {
//    curr_motor_speed= msg.toFloat();   // simple control
//    Serial.print("Updated motor speed command: ");
//    Serial.println(motor_speed);
//  }
}

// ----------- MQTT Reconnect -----------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP32_Node")) {
      Serial.println("connected");

      // Subscribe
      client.subscribe(sub_topic);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 3 seconds");
      delay(3000);
    }
  }
}


// Simple averaging filter (reduces noise)
float getFilteredVoltage() {
  float sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(CURRENT_SENSOR_PIN);
  }
  float avg = sum / 20.0;
  return (avg * Vref) / ADC_RES;
}



// -------- CONTROLLER --------
int computeDutyCycle() {
  float sag_error = SAG_SET - sag_measured;
  Serial.println(sag_measured);
  float control = 120 + Kp * sag_error;

  int duty = (int)control;

  // Allow bidirectional control
  if (duty > 255) duty = 255;
  if (duty < 120) duty = 120;

  return duty;
}

// -------- MOTOR CONTROL (MX1508) --------
void setMotor(int duty) {

  if (duty > 0) {
    // Forward
    ledcWrite(motor1Pin1, duty);
    ledcWrite(motor1Pin2, 0);
  } 
  else if (duty < 0) {
    // Reverse
    ledcWrite(motor1Pin1, 0);
    ledcWrite(motor1Pin2, -duty);
  } 
  else {
    // Stop
    ledcWrite(motor1Pin1, 0);
    ledcWrite(motor1Pin2, 0);
  }
}

// ----------- Setup -----------
void setup() {
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
//  pinMode(LED,OUTPUT);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
   // Attach PWM to BOTH pins (important for MX1508)
  ledcAttach(motor1Pin1, freq, resolution);
  ledcAttach(motor1Pin2, freq, resolution);
  pinMode(CURRENT_SENSOR_PIN, INPUT);
 
  // configure LEDC PWM
  ledcAttachChannel(enable1Pin, freq, resolution, pwmChannel);
   pinMode(EncoderPinA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(EncoderPinA), updateEncoder, RISING);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
    /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  accel.setRange(ADXL345_RANGE_16_G);
  sensors.begin();

  // Calibrate offset (no current condition)
  float sum = 0;
  for (int i = 0; i < 500; i++) {
    int adc = analogRead(CURRENT_SENSOR_PIN);
    float voltage = (adc * Vref) / ADC_RES;
    sum += voltage;
    delay(2);
  }
  offsetVoltage = sum / 500.0;
}


// ----------- Loop -----------
void loop() {
//  set motor high
digitalWrite(motor1Pin1,HIGH);
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // -------- Current sensing --------
  float voltage = getFilteredVoltage();
  motor_current = -(voltage - offsetVoltage) / sensitivity;

  // -------- Speed calculation --------
  calc_speed();
  curr_motor_speed= rpm;

  // ---------vib sensing-----------
   /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);

  vib_x =event.acceleration.x;
  vib_y =event.acceleration.y;
  vib_z =event.acceleration.z;

  // -------- temp sensing--------
   sensors.requestTemperatures(); 
  float temp = sensors.getTempCByIndex(0);
  // -------- sag compensation calculations--------
   // -------- sag compensation calculations --------

 // Compute control
  int duty = computeDutyCycle();

  // Apply motor control
  setMotor(duty);
  Serial.print("Duty: ");
  Serial.println(duty);
  // -------- Publishing --------
  static unsigned long lastPub = 0;
  unsigned long now = millis();

  if (now - lastPub > 50) {  // 20 Hz
    lastPub = now;

    char speed_buf[16];
    char current_buf[16];
    char vib_x_buf[16];
    char vib_y_buf[16];
    char vib_z_buf[16];
    char temp_buf[16];
    
    dtostrf(curr_motor_speed, 1, 2, speed_buf);
    dtostrf(motor_current, 1, 2, current_buf);
    dtostrf(vib_x, 1, 2, vib_x_buf);
    dtostrf(vib_y, 1, 2, vib_y_buf);
    dtostrf(vib_z, 1, 2, vib_z_buf);
    dtostrf(temp, 1, 2, temp_buf);
    
    client.publish(pub_speed, speed_buf);
    client.publish(pub_current, current_buf);
    client.publish(pub_vib_x, vib_x_buf);
    client.publish(pub_vib_y, vib_y_buf);
    client.publish(pub_vib_z, vib_z_buf);
    client.publish(pub_temp, temp_buf);
  }
}
