#include "turboSPOT.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(1000);
  
  if (REQUEST_TURBO_SPOT()==1) {
    Serial.println("Now running at 96 MHz");
    delay(500);
  }
  if(STOP_TURBO_SPOT() == 0) {
    Serial.println("Now running at 48 MHz");
    delay(500);
  }


}

void loop() {
  // put your main code here, to run repeatedly:

}
