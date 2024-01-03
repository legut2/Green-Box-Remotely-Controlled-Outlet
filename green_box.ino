// Blues.io imports
#include <NotecardPseudoSensor.h>
#include <Notecard.h>
using namespace blues;
#define usbSerial Serial
#define productUID "com.devmandan.daniel:remote_outlet"
Notecard notecard;
NotecardPseudoSensor sensor(notecard);
// Blues.io imports

#include <retrieve_command_state_for_relay.h>
#include <send_current_reading.h>

#include <Arduino.h>

#define DIGITAL_RELAY_CONTROL D5 //pin definition for which pin the relay is soldered to

void blues_setup() {
  notecard.begin();
  notecard.setDebugOutputStream(usbSerial);
  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", productUID);
  JAddStringToObject(req, "mode", "continuous");
  JAddBoolToObject(req, "sync", true);
  J *rsp = notecard.requestAndResponse(req);
  
  notecard.deleteResponse(rsp);
}

void relay_setup() {
  //set pin to be an output
  pinMode(DIGITAL_RELAY_CONTROL, OUTPUT);

  //Sends voltage high signal to relay
  digitalWrite(DIGITAL_RELAY_CONTROL, LOW);// set relay pin to HIGH
}

void setup() {
  //tell blues we've booted up and send some data
  blues_setup();

  //setup relay and keep on initially (NC)
  relay_setup();
    
  
  // initialize digital pin LED_BUILTIN as an output, so we know when relay is closed or not
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while (!Serial);
  Serial.println(__FILE__);
  Serial.print("ACS712_LIB_VERSION: ");
  Serial.println(ACS712_LIB_VERSION);

  //setup the ammeter that measures current
  setup_acs712();
}

int mismatch_counter = 0;
bool local_relay_state = true;

std::optional<bool> update_local_relay_state() {
  std::optional<bool> desired_relay_state = check_notes_for_digital_relay_control_command(notecard);
  if (desired_relay_state.has_value()){ 
    bool desired_state = desired_relay_state.value();
    bool state_mismatch = (desired_state != local_relay_state);
    if (state_mismatch) {
      mismatch_counter++;
    }
    if (state_mismatch && mismatch_counter > 2) { //this logic makes relay more reliable in terms of state (on or off) but less responsive overall
      local_relay_state = desired_state;
      mismatch_counter = 0;
    }
  }
  if (local_relay_state) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED off by making the voltage LOW
    digitalWrite(DIGITAL_RELAY_CONTROL, LOW);// //Sends voltage low signal to relay
  } else {
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    digitalWrite(DIGITAL_RELAY_CONTROL, HIGH);// //Sends voltage low signal to relay
  }
  return desired_relay_state;
}

// simple logic of greenbox
void loop() {
  //definition and implementation of read_simple_average_of_current_ma and send_sensor_value_to_blues can be found within send_current_reading.h file
  send_sensor_value_to_blues(read_simple_average_of_current_ma(), notecard);
  //gets desired relay state from blues and checks to see if there is a mismatch to local relay state, if there is and it isn't a fluke - updates relay state
  std::optional<bool> desired_relay_state = update_local_relay_state();
  Serial.print("desired relay state: ");
  if (desired_relay_state.has_value()) {
    Serial.println(desired_relay_state.value() ? "true" : "false");
  } else {
    Serial.println("");
  }
  Serial.print("local relay state: ");
  Serial.println(local_relay_state==1? "true" : "false");
  Serial.print("mismatch counter: ");
  Serial.println(mismatch_counter); # greater than two causes local_relay_state to be changed to desired_relay_state within update_local_relay_state
  delay(5000);
}