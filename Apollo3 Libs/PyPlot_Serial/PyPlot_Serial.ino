#include <Apollo3_ADC_LIB.h>
#include <Laymans_FFT.h>
#include "PyPlot_Serial.h"
#include <turboSPOT.h>

#define FFT_SIZE 1024
#define TX_SIZE FFT_SIZE
#define SMPL_RATE 1200000
#define OSR_RATIO 1

adc_handle_t* adc_handle = adc_get_handle();
fft_handle_t* fft_handle = fft_get_handle();

int32_t smpl_data[FFT_SIZE];
float mag[FFT_SIZE];



void setup() {
  // put your setup code here, to run once:
  Serial.begin(1500000);
  Serial.setTimeout(0.5);

  adc_config(adc_handle, SMPL_RATE, FFT_SIZE, ADC_A1, OSR_1, ADC_14BIT);
  adc_setup(adc_handle, smpl_data);

  delay(1000);
  init_fft(fft_handle);
  fft_setup(fft_handle, FFT_SIZE, IMPERIAL);
  pinMode(13, OUTPUT);
}


void loop() {
  // put your main code here, to run repeatedly:
  // rec_with_end_marker();
  // if(check_data('S')) {
  //   // delayMicroseconds(10);
  //   Serial.println('K');
    
  //   smpl();
  //   run_fft_w_mag_db(fft_handle, smpl_data, mag);
  //   Serial.println('D');
  //   delay(1);

  //   Serial.println(TX_SIZE/2);
  //   tx_smpl(mag, FFT_SIZE, TX_SIZE, SMPL_RATE, OSR_RATIO);
  // }

  if(fft_request() == true) {
    tx_ack();
  
    bool fft_status = 1;
    digitalWrite(13, fft_status);
    smpl();
    tx_ack();
    REQUEST_TURBO_SPOT();
    run_fft_w_mag_db(fft_handle, smpl_data, mag);
    STOP_TURBO_SPOT();
    fft_status = 0;
    digitalWrite(13, fft_status);
    tx_fft(mag, FFT_SIZE, SMPL_RATE*0.9959072);
  }
}



void smpl(){
  adc_software_trigger(adc_handle, smpl_data);
  while(1){
    if(adc_smpl_status(adc_handle) == 1) {
      break;
    }
  }
  // delay(10);
  adc_transfer_data(adc_handle, smpl_data);
  adc_clear_status(adc_handle);
  for(int i = 0; i<FFT_SIZE; i++) {
    // smpl_data[i] *= 16;
    smpl_data[i] -= 8192;
    smpl_data[i] *=4;
    // smpl_data[i] -= 512;
    // smpl_data[i] *= 64;
    // smpl_data[i] = 32000*sinf(2*3.14*i/FFT_SIZE);
  }
}









