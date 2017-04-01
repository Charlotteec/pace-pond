
#include <SPI.h>
#include <WiFi101.h>
#include <PubSubClient.h>
#include <OneWire.h> 
#include <Timer.h>
#include <DallasTemperature.h>

#define DEBUG_ON 1

#define AIRTEMP 12 //DS18S20 Signal for Air Temp on digital pin  12
#define SENSOR1 11 // We can support 3 additional temperature sensors for the water on 11, 10 and 9
#define SENSOR2 10
#define SENSOR3 9

int total_sensors=2;  // Number of sensors in use (including the air sensor)

//Temperature chip i/o

OneWire oneWire_air(AIRTEMP);  // on digital pin 12
DallasTemperature air_sensor(&oneWire_air);

OneWire oneWire_sensor1(SENSOR1);  // on digital pin 11
DallasTemperature sensor1(&oneWire_sensor1);

OneWire oneWire_sensor2(SENSOR2);  // on digital pin 10
DallasTemperature sensor2(&oneWire_sensor2);

OneWire oneWire_sensor3(SENSOR3);  // on digital pin 9
DallasTemperature sensor3(&oneWire_sensor3);

                                    // variables for all of the temp's we read
float temps[3];                     // index=0 is air.  1,2,3 are three separate readings

Timer t;
int   PublishEvent;

Timer t2;
int   HeartBeatEvent;


char ssid[] = "PACE-GUEST"; //  your network SSID (name)
//char ssid[] = "ws2"; //  your network SSID (name)
//char pass[] = "";    // your network password (use for WPA, or use as key for WEP)

const char* mqtt_server = "realtime.ngi.ibm.com";

char device[30] = "feather4";

//
// the mqtt channel for publishing data will be LOCATION/BODY_OF_WATER/
// the mqtt channel for communiticating with the device will be:  LOCATION/BODY_OF_WATER/device 
//             (where device is formed from DEVICE_NAME-(device's MAC address)
// the mqtt channel for output from the device (as a result of commands) will be: LOCATION/BODY_OF_WATER/device/debug
//

#define LOCATION "DEV"
#define BODY_OF_WATER "Choate-Pond"
#define DEVICE_NAME "feather"          // first part of the mqtt device name 

char datachannel[50],commandchannel[50],debugchannel[50];
char devicestring[100];

char deviceMAC[8];            // Global variable to hold device's MAC address (from printwifistatus())

String mqttinput = "";
char msg[50];
int value = 0;

// system wide variables

int interval = 6; // How often take readings - Represented in number of seconds
int scale = 1;        // Temperature Scale : 1 = Farenheit 0 = Centigrade


int status = WL_IDLE_STATUS;

WiFiClient client;
PubSubClient client2(client);


void setup() {
  //Initialize serial and wait for port to open:
  WiFi.setPins(8,7,4,2);
  
#ifdef DEBUG_ON
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif  

  PublishEvent = t.every(interval*1000, doThingy);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
#ifdef DEBUG_ON    
    Serial.println("WiFi shield not present");
#endif
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    
#ifdef DEBUG_ON    
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
#endif

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid);


    // wait 10 seconds for connection:
    delay(10000);
  }

printWiFiStatus();

sprintf(device,"%s-%s",DEVICE_NAME,deviceMAC);
sprintf(datachannel,"%s/%s",LOCATION,BODY_OF_WATER);
sprintf(commandchannel,"%s/%s/%s",LOCATION,BODY_OF_WATER,device);
sprintf(debugchannel,"%s/%s/%s/debug",LOCATION,BODY_OF_WATER,device);

#ifdef DEBUG_ON  
  Serial.println("Connected to wifi");
  Serial.print("==>");
  Serial.println(device);
  Serial.print("Data Channel: ");
  Serial.println(datachannel);
  Serial.print("device Channel: ");
  Serial.println(commandchannel);
  Serial.print("Debug Channel: ");
  Serial.println(debugchannel);
  
#endif

  delay(250);
  
  client2.connect(mqtt_server);
  client2.setServer(mqtt_server, 1883);
  client2.setCallback(callback);
}


void callback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  String parameter = "";

#ifdef DEBUG_ON
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
#endif

  for (int i = 0; i < length; i++) {
    mqttinput.concat((char)payload[i]);
  }
  
#ifdef DEBUG_ON  
  Serial.println();
#endif

command = mqttinput.substring(0,1);

if(command == "i"){
  parameter = mqttinput.substring(2,mqttinput.length());
  interval = parameter.toInt();

  t.stop(PublishEvent);
  PublishEvent = t.every(1000*interval,doThingy);
  sprintf(devicestring,"Interval changed to: %d",interval);
  client2.publish(debugchannel,devicestring);
  
#ifdef DEBUG_ON  
  Serial.print("Interval set to: ");
  Serial.println(interval*1);
#endif

}
command = mqttinput.substring(0,1);

if(command == "s"){
  parameter = mqttinput.substring(2,mqttinput.length());
  if(parameter == "c"){
    scale = 0;
  }else if(parameter = "f"){
    scale = 1;
  }
  #ifdef DEBUG_ON  
  Serial.print("Scale set to: ");
  Serial.println(parameter);
#endif
}

/*command = mqttinput.substring(0,1);

if(command == "s"){
  parameter = mqttinput.substring(2,mqttinput.length());
  if(parameter == "c"){
    scale = 0;
  }else if(parameter = "f"){
    scale = 1;
  }
  #ifdef DEBUG_ON  
  Serial.print("Scale set to: ");
  Serial.println(scale);
#endif
}*/
mqttinput="";


}


void reconnect() {
  // Loop until we're reconnected
  while (!client2.connected()) {
    
#ifdef DEBUG_ON    
    Serial.print("Attempting MQTT connection...");
#endif

    // Attempt to connect
    if (client2.connect(device)) {

      client2.publish(debugchannel,device);
      
#ifdef DEBUG_ON      
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client2.publish(TOPIC, );
      // ... and resubscribe
#endif

      client2.subscribe(commandchannel);
    } else {

#ifdef DEBUG_ON      
      Serial.print("failed, rc=");
      Serial.print(client2.state());
      Serial.println(" try again in 5 seconds");
#endif
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void doThingy(){

    get_temperatures();
    snprintf (msg, 75, "%f", temps[1]);
    
#ifdef DEBUG_ON    
    Serial.print("Publish message to "  LOCATION "/" BODY_OF_WATER ": ");
    Serial.print(temps[0]);
    Serial.print(" - ");
    Serial.print(temps[1]);
    Serial.print(" - ");
    Serial.print(temps[2]);
    Serial.print(" - ");
    Serial.println(temps[3]);
#endif
    
    client2.publish((LOCATION "/" BODY_OF_WATER), msg);
  
}

void get_temperatures()
{
  air_sensor.requestTemperatures();
    if (scale == 0) {
      temps[0] = air_sensor.getTempCByIndex(0);
    }
    else {
      temps[0] = air_sensor.getTempFByIndex(0);
    }

    delay(1000);
    
    sensor1.requestTemperatures();
    if (scale == 0) {
      temps[1] = sensor1.getTempCByIndex(0);
    }
    else {
      temps[1] = sensor1.getTempFByIndex(0);
    }

    delay(1000);
    
    sensor2.requestTemperatures();
    if (scale == 0) {
      temps[2] = sensor2.getTempCByIndex(0);
    }
    else {
      temps[2] = sensor2.getTempFByIndex(0);
    }

    delay(1000);
    
    sensor3.requestTemperatures();
    if (scale == 0) {
      temps[3] = sensor3.getTempCByIndex(0);
    }
    else {
      temps[3] = sensor3.getTempFByIndex(0);
    }
}

void loop() {
  
  if (!client2.connected()) {
    reconnect();
  }
  client2.loop();
  
     t.update();

  
}


void printWiFiStatus() {
  // determine devices MAC address and optionally print out network health information

  byte mac[6];

  WiFi.macAddress(mac);
  sprintf(deviceMAC,"%x%x%x%x%x%x",mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]);
  
#ifdef DEBUG_ON  
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip); 
  
  Serial.print("MAC address: ");
  Serial.println(deviceMAC);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
#endif

}

