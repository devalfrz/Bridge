/*
  Arduino Yún Bridge example

  This example for the Arduino Yún shows how to use the
  Bridge library to access the digital and analog pins
  on the board through REST calls. It demonstrates how
  you can create your own API when using REST style
  calls through the browser.

  Possible commands created in this shetch:

  "/arduino/digital/13"     -> digitalRead(13)
  "/arduino/digital/13/1"   -> pinMode(13, OUTPUT), digitalWrite(13, HIGH)
  "/arduino/toggle/13"      -> pinMode(13, OUTPUT), digitalWrite(13, !last_state)
  "/arduino/analog/2/123"   -> pinMode(2, OUTPUT), analogWrite(2, 123)
  "/arduino/analog/2"       -> analogRead(2)
  "/arduino/mode/13/input"  -> pinMode(13, INPUT)
  "/arduino/mode/13/output" -> pinMode(13, OUTPUT)

  This example code is part of the public domain

  http://www.arduino.cc/en/Tutorial/Bridge

*/

#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
#include <EEPROM.h>

// Define constants EEPROM
#define PIN_MODE 0
#define PIN_VALUE 1
#define MEMORY_SLOT_SIZE 2
#define TOTAL_PINS 20

// Listen to the default port 5555, the Yún webserver
// will forward there all the HTTP requests you send
BridgeServer server;
String response;

void setup() {
  // Recover pin states from memory
  recoverPinStates();
  
  // Bridge startup
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);


  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
}

void loop() {
  // Get clients coming from server
  BridgeClient client = server.accept();

  // There is a new client?
  if (client) {
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  delay(50); // Poll every 50ms
}

void process(BridgeClient client) {
  // read the command
  String command = client.readStringUntil('/');

  // is "digital" command?
  if (command == "digital") {
    digitalCommand(client);
  }
  
  // is "toggle" command?
  if (command == "toggle") {
    toggleCommand(client);
  }

  // is "analog" command?
  if (command == "analog") {
    analogCommand(client);
  }

  // is "mode" command?
  if (command == "mode") {
    modeCommand(client);
  }
}

void digitalCommand(BridgeClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/') {
    value = client.parseInt();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, value);
  } else {
    value = digitalRead(pin);
  }

  // Send feedback to client
  response = String(digitalRead(pin));
  client.print(String("{\"status\":"+response+"}"));

  // Save Pin State
  savePinState(pin,false,value);
  
  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void toggleCommand(BridgeClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  value = !digitalRead(pin);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, value);

  // Send feedback to client
  response = String(digitalRead(pin));
  client.print(String("{\"status\":"+response+"}"));

  // Save Pin State
  savePinState(pin,false,value);
  
  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void analogCommand(BridgeClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/') {
    // Read value and execute command
    pinMode(pin, OUTPUT);
    value = client.parseInt();
    analogWrite(pin, value);

    // Send feedback to client
    response = String(value);
    client.print(String("{\"analog\":"+response+"}"));

    // Save Pin State
    savePinState(pin,true,value);

    // Update datastore key with the current pin value
    String key = "D";
    key += pin;
    Bridge.put(key, String(value));
  } else {
    // Read analog pin
    value = analogRead(pin);

    // Send feedback to client
    response = String(value);
    client.print(String("{\"analog\":"+response+"}"));

    // Update datastore key with the current pin value
    String key = "A";
    key += pin;
    Bridge.put(key, String(value));
  }
}

void modeCommand(BridgeClient client) {
  int pin;

  // Read pin number
  pin = client.parseInt();

  // If the next character is not a '/' we have a malformed URL
  if (client.read() != '/') {
    client.print(String("{\"error\":\"Malformed URL\"}"));
    return;
  }

  String mode = client.readStringUntil('\r');

  if (mode == "input") {
    pinMode(pin, INPUT);
    // Send feedback to client
    response = String(INPUT);
    client.print(String("{\"mode\":"+response+"}"));

    // Clear pin state
    clearPinState(pin);

    return;
  }

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    // Send feedback to client
    response = String(OUTPUT);
    client.print(String("{\"mode\":"+response+"}"));
    
    return;
  }

  client.print(String("{\"error\":\"Invalid mode "+mode+"\"}"));
}

void recoverPinStates(){
  int i,offset;
  for(i=0;i<TOTAL_PINS;i++){
    offset = i*MEMORY_SLOT_SIZE;
    if(EEPROM.read(offset)==1){//Digital Output
      pinMode(i,OUTPUT);
      digitalWrite(i,EEPROM.read(offset+PIN_VALUE));
    }else if(EEPROM.read(offset)==2){//Digital Output
      pinMode(i,OUTPUT);
      analogWrite(i,EEPROM.read(offset+PIN_VALUE));
    }
  }
}

void savePinState(int pin, uint8_t analog, uint8_t value){
  int offset;
  offset = pin*MEMORY_SLOT_SIZE;
  EEPROM.write(offset+PIN_MODE,analog+1); // 1 if digital, 2 if analog
  EEPROM.write(offset+PIN_VALUE,value);
}

void clearPinState(int pin){
  int offset;
  offset = pin*MEMORY_SLOT_SIZE;
  EEPROM.write(offset+PIN_MODE,0);
}


