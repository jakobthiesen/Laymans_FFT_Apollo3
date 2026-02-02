#include "am_bsp.h"
#include "Apollo3_ADC_LIB.h"

#define ResolutionBits 14

#define ADC_SMPL_BUF_SIZE 1024

int32_t SMPL_BUF[ADC_SMPL_BUF_SIZE];
adc_handle_t* x = adc_get_handle();


void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  // adc_config(x, 100000, ADC_SMPL_BUF_SIZE, ADC_A1, OSR_1, ADC_14BIT);
  adc_config_dual_channel(x, 200000, ADC_SMPL_BUF_SIZE, ADC_A1, ADC_A2, OSR_1, ADC_14BIT);
  adc_setup(x, SMPL_BUF);
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
 if(adc_smpl_status(x) == 1) {
    adc_transfer_data(x, SMPL_BUF);
    adc_clear_status(x);
    for(uint16_t i = 0; i < (uint32_t)(ADC_SMPL_BUF_SIZE/2); i=i+2) {
      // int32_t result = SMPL_BUF[i];
      // result >>= 6; //bitshift due to the average in the register being of the format 14.6.
      // Serial.println(result*2.0/16383.0);
      Serial.print("Variable_1:");
      Serial.print(SMPL_BUF[i]);
      Serial.print(",");
      Serial.print("Variable_2:");
      Serial.println(SMPL_BUF[i+1]*100);
      
    }
    delay(1000);
    adc_software_trigger(x, SMPL_BUF);
 }
} 















