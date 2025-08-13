#include "am_bsp.h"
#include "Apollo3_ADC_LIB.h"


#define ResolutionBits 14

#define ADC_SMPL_BUF_SIZE 256

int32_t SMPL_BUF[ADC_SMPL_BUF_SIZE];
adc_handle_t* x = adc_get_handle();


void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  adc_config(x, 2660000, 256, ADC_A1, OSR_1, ADC_14BIT);
  adc_setup(x, SMPL_BUF);
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
 if(adc_smpl_status(x) == 1) {
    adc_transfer_data(x, SMPL_BUF);
    adc_clear_status(x);
    for(uint16_t i = 0; i < 256; i++) {
      // int32_t result = SMPL_BUF[i];
      // result >>= 6; //bitshift due to the average in the register being of the format 14.6.
      // Serial.println(result*2.0/16383.0);
      Serial.println(SMPL_BUF[i]);
    }
    delay(1000);
    adc_software_trigger(x, SMPL_BUF);
 }
} 
















