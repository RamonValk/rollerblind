#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>

// Network setup
const char* client_name = "rollerblind";
const char* ssid = "SSID";
const char* password = "Password";
const char* mqtt_server = "Server";
const int mqtt_port = 18373;
const char* mqtt_username = "user";
const char* mqtt_password = "password";
const char* mqtt_topic = "Rollerblind1";
const int revolutions_rollerblind = 10;

// Stepper setup
const int steps_per_revolution = 200;


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

Stepper stepper(steps_per_revolution, 14, 12, 13, 15);

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  stepper.setSpeed(60);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //change watchdog
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  delay(10);
  // We start by connecting to a WiFi network
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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  int rotation = 0;
  rotation = (char)payload[1] - '0';
  rotation = rotation * 10;
  rotation = rotation + (char)payload[2] - '0';
  
  if ((char)payload[0] == '+') {
      //rotation = (char)payload[1] + (char)payload[2];
      //rotation = rotation - '0';
      Serial.print("Moving down by ");
      Serial.print(rotation);
      Serial.println(" %;");
      setRollerblind(false, rotation);
  } else if ((char)payload[0] == '-'){
      //rotation = (char)payload[1] + (char)payload[2];
      //rotation = rotation - '0';
      Serial.print("Moving up by ");
      Serial.print(rotation);
      Serial.println(" %;");
      setRollerblind(true, rotation);
  } else {
      Serial.println("Something went wrong;");
    }


  // Switch on the LED if an 1 was received as first character OLD
  /*if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    setRollerblind();
  }*/

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_name, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void disableStepper() {
  digitalWrite(12,LOW);
  digitalWrite(13,LOW);
  digitalWrite(14,LOW);
  digitalWrite(15,LOW);
  Serial.println("Powering down stepper...");
}

//Set rollerblind based on incoming percentage
void setRollerblind(boolean move_up, int revolutions_percentage) {
  int total_steps = steps_per_revolution * revolutions_rollerblind;
  int steps_to_take = total_steps * revolutions_percentage / 100;
  int revolutions_to_make = steps_to_take / steps_per_revolution;
  int steps_left = steps_to_take % steps_per_revolution;
  Serial.print("Percentage coming in: ");
  Serial.println(revolutions_percentage);
  Serial.print("Total amount of steps: ");
  Serial.print(total_steps);
  Serial.print("; Amount of steps to take: ");
  Serial.print(steps_to_take);
  Serial.print("; Revolutions: ");
  Serial.print(revolutions_to_make);
  Serial.print("; Steps left over after full revolutions: ");
  Serial.print(steps_left);
    Serial.print("; \n");
    //Divide steps in revolutions and remaining steps to stay connected while executing
    if (move_up == false) {
        for (int i = 0; i <= revolutions_to_make; i++) {
          stepper.step(steps_per_revolution);
          yield();
          Serial.print("Stepped: ");
          Serial.println(steps_per_revolution);
         }
       stepper.step(steps_left);
       Serial.print("Stepped: ");
       Serial.println(steps_left);
    } else {
        for (int i = 0; i <= revolutions_to_make; i++) {
          stepper.step(-steps_per_revolution);
          yield();
          Serial.print("Stepped: ");
          Serial.println(-steps_per_revolution);
         }
      stepper.step(-steps_left);  
      Serial.print("Stepped: ");
      Serial.println(-steps_left);
    }
    ESP.wdtFeed();
    yield();
    Serial.println("Stepper done!");
  //}
  //disableStepper();
 
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
