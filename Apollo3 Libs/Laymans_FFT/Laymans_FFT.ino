#include "Laymans_FFT.h"
#include <arm_math.h>
#include <Apollo3_ADC_LIB.h>
#include "am_bsp.h"



#define FFT_SIZE 512
// #define LUT_SIZE 512


#define ADC_EXAMPLE_DEBUG 1

adc_handle_t* adc_handle = adc_get_handle();


// int16_t sin_LUT[LUT_SIZE];
// int32_t tw_LUT[FFT_SIZE];

int32_t w_ri = 0;
int32_t smpl_data[FFT_SIZE];
int32_t smpl_data_temp[FFT_SIZE];
float mag[FFT_SIZE];

unsigned long timer = 0;

const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data

boolean newData = false;

int16_t r_data = 0;
int16_t i_data = 0;
float r_data_f = 0;
float i_data_f = 0;

// fft_handle_t fft_handle;

fft_handle_t* fft_handle = fft_get_handle();



void setup() {
  Serial.begin(500000);
  Serial.setTimeout(1);

  init_fft(fft_handle);  
  fft_setup(fft_handle, FFT_SIZE, HAMMING);

  adc_config(adc_handle, 100000, FFT_SIZE, BURST_SCAN, ADC_A1, OSR_1, ADC_14BIT);
  adc_setup(adc_handle, smpl_data);
  adc_arm_burst_scan(adc_handle, smpl_data);
  delay(1000);

  adc_software_burst_trigger(adc_handle, smpl_data);
  while(1){
    if(adc_smpl_status(adc_handle) == 1) {
      break;
    }
    delay(10);
  }
  adc_transfer_data(adc_handle, smpl_data);
  adc_clear_status(adc_handle);
  adc_arm_burst_scan(adc_handle, smpl_data);
  
  adc_software_burst_trigger(adc_handle, smpl_data);
  while(1){
    if(adc_smpl_status(adc_handle) == 1) {
      break;
    }
    delay(10);
  }
  adc_transfer_data(adc_handle, smpl_data);
  adc_clear_status(adc_handle);
  adc_arm_burst_scan(adc_handle, smpl_data);




  run_fft_w_mag_db(fft_handle, smpl_data, mag); 

  // window(fft_handle, smpl_data, BLACKMAN_HARRIS);
  uint16_t k = (uint16_t)(FFT_SIZE/2);
  for(uint16_t i = 0; i < k; i++) {
    Serial.println(mag[i]);
    // Serial.println(smpl_data[i]);
    delay(5);
  }

}

void loop() {
  // put your main code here, to run repeatedly:

  // For FFT plotting with python tool
  // recvWithEndMarker();
  // if(CheckData('H')) {
  //   Serial.println(FFT_SIZE/2);
  //   smpl();
  //   run_fft_w_mag_db(fft_handle, smpl_data, mag);    
  //   // fft_handle.full_fft_w_mag(&fft_handle, smpl_data, sin_LUT, mag, HANN,0);


  //   TXSmpl(mag, FFT_SIZE, (400.0));

  // }

  //For simple TX Transfer
  // smpl();
  // for(int i = 0; i < FFT_SIZE; i++){
  //   smpl_data_temp[i] = smpl_data[i];
  // }
  // // run_fft_w_mag_db(fft_handle, smpl_data, mag);  
  // run_fft(fft_handle, smpl_data);
  // for(int i = 0; i < 50; i++){
  //   smpl_data[i] = 0;
  // }
  // run_ifft(fft_handle, smpl_data);
  // // get_mag_db(fft_handle, smpl_data, mag);
  
  // for(uint16_t i = 0; i < (uint16_t)(FFT_SIZE/2); i++){
  //   // float re = (float)((int16_t)(smpl_data[i]&0x0000FFFF));
  //   // float im = (float)((int16_t)(smpl_data[i]>>16));
  //   Serial.print("real:");
  //   Serial.print((int16_t)(smpl_data[i]&0x0000FFFF));
  //   Serial.print(",");
  //   Serial.print("Imag:");
  //   Serial.print((int16_t)(smpl_data[i]>>16));
  //   Serial.print(",");
  //   Serial.print("Original data:");
  //   Serial.println(smpl_data_temp[i]);

  //   // Serial.println(mag[i]);

  // }
  delay(2000);

}






void printSmpl(float *data, uint16_t smpl_size, float fs) {
    for(uint16_t i = 0; i < smpl_size/2; i++) {
    Serial.print((fs/((float)smpl_size))*i);
    Serial.print(" Hz: ");
    Serial.println(data[i]),3;

    // Serial.println((int16_t)(smpl_data[i]&0x0000FFFF));
  }
}

void TXSmpl(float *data, uint16_t smpl_size, float fs) {
  Serial.println("H");
  // Serial.println(smpl_size/2);
  delayMicroseconds(10);
    for(uint16_t i = 0; i < smpl_size/2; i++) {
    Serial.print((fs/((float)smpl_size))*i);
    Serial.print("Hz:");
    Serial.print(data[i]),3;
    Serial.println("dBV");
    // delayMicroseconds(1);
    // Serial.println((int16_t)(smpl_data[i]&0x0000FFFF));
  }
  Serial.println("!");
}



void smpl() {

  adc_software_burst_trigger(adc_handle, smpl_data);
  while(1) {
    if(adc_smpl_status(adc_handle) == 1) {
      break;
    }
  }
  delay(10);
  adc_transfer_data(adc_handle, smpl_data);
  adc_clear_status(adc_handle);
  for(int i = 0;i<FFT_SIZE;i++) {
    smpl_data[i] -= 8192;
    smpl_data[i]*=4;

  }

}



void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

bool CheckData(char checkVal) {
  bool r = 0;  
    if (newData == true) {
      // Serial.println(receivedChars);
        if(receivedChars[0] == checkVal) {
          r = 1;
        }
        newData = false;
    }
  return r;
}