
#include <SPI.h>
#include <WiFi101.h>
#include <PubSubClient.h>
#include <OneWire.h> 
#include <Timer.h>
#include <DallasTemperature.h>

int DS18S20_Pin = 12; //DS18S20 Signal pin on digital 12

//Temperature chip i/o
OneWire oneWire(DS18S20_Pin);  // on digital pin 12

DallasTemperature sensors(&oneWire);

Timer t;

char ssid[] = "PACE-GUEST"; //  your network SSID (name)
//char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
//int keyIndex = 0;            // your network key Index number (needed only for WEP)
//const char* mqtt_server = "realtime.ngi.ibm.com";
const char* mqtt_server = "129.33.26.221";


const char* device = "3423525";
//#define TOPIC "Pace-Pond"
//#define TOPICin  "Pace-Pond-Debug"

#define LOCATION "Pace"
#define BODY_OF_WATER "Choate-Pond"
#define DEVICE_NAME "Pace-feather3"

String mqttinput = "";
char msg[50];
int value = 0;

int interval = 6;
int status = WL_IDLE_STATUS;

WiFiClient client;
PubSubClient client2(client);
int tickEvent;

void setup() {
  //Initialize serial and wait for port to open:
  WiFi.setPins(8,7,4,2);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  //printWiFiStatus();
  delay(250);
  client2.connect(mqtt_server);
  client2.setServer(mqtt_server, 1883);
  client2.setCallback(callback);
  
  t.every(interval*1000, doThingy);
  Serial.println(interval);

}


void callback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  String parameter = "";
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    mqttinput.concat((char)payload[i]);
  }
  Serial.println();


command = mqttinput.substring(0,1);

if(command == "i"){
  parameter = mqttinput.substring(2, mqttinput.length());
  interval = parameter.toInt();
  //interval = 60;
  Serial.print("Interval set to: ");
  Serial.println(interval);
  t.update();
  //tickEvent = t.every(interval*1000, doThingy);

}
}


void reconnect() {
  // Loop until we're reconnected
  while (!client2.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client2.connect(device)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client2.publish(TOPIC, );
      // ... and resubscribe
      client2.subscribe(LOCATION "/" BODY_OF_WATER "/" DEVICE_NAME);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client2.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


float temp;
void doThingy(){
    
    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);
    snprintf (msg, 75, "%f", temp);
    Serial.print("Publish message to "  LOCATION "/" BODY_OF_WATER ": ");
    Serial.println(temp);
    client2.publish((LOCATION "/" BODY_OF_WATER), msg);
  
}


void loop() {
  
  if (!client2.connected()) {
    reconnect();
  }
  client2.loop();
  
     t.update();
     //Serial.println(interval);
  
}


void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

   byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);


  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

