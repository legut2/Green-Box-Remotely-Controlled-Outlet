#include "ACS712.h"
#include <NotecardPseudoSensor.h>
#include <Notecard.h>

#define ADC_PIN_A4_FEATHER PC4
#define SAMPLE_SIZE 200
// ACS712 5A  uses 185 mV per A
// ACS712 20A uses 100 mV per A
// ACS712 30A uses  66 mV per A
// ESP 32 example (requires resistors to step down the logic voltage)
ACS712  ACS(ADC_PIN_A4_FEATHER, 3.3, 4095, 66);

int frequency = 0;

void setup_acs712() {
  // frequency = ACS.detectFrequency(40);
  frequency = 60;
  Serial.println("Frequency:");
  Serial.println(frequency);

  Serial.print("MidPoint: ");
  Serial.println(ACS.autoMidPoint(frequency,200));
  Serial.print("Noise mV: ");
  Serial.println(ACS.getNoisemV());
}
float read_simple_average_of_current_ma() {
  int m1 = ACS.mA_AC(frequency,SAMPLE_SIZE);
  Serial.print("mA:\t");
  Serial.println(m1);
  return m1;
}
void send_sensor_value_to_blues(float measured_current_ma, Notecard &notecard) {
  J *req = notecard.newRequest("note.add");
  if (req != NULL)
  {
    JAddStringToObject(req, "file", "sensors.qo");
    JAddBoolToObject(req, "sync", true);
    J *body = JAddObjectToObject(req, "body");
    if (body)
    {
      JAddNumberToObject(body, "mA", measured_current_ma);
    }
    notecard.sendRequest(req);
  }
}