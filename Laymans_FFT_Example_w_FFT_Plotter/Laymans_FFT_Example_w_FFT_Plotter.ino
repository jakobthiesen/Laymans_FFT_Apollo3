#include <Apollo3_ADC_LIB.h>
#include <Laymans_FFT.h>

#define FFT_SIZE 2048
#define TX_SIZE 2048
#define SMPL_RATE 1200000
#define OSR_RATIO 4

adc_handle_t* adc_handle = adc_get_handle();
fft_handle_t* fft_handle = fft_get_handle();

int32_t smpl_data[FFT_SIZE];
float mag[FFT_SIZE];

const byte num_chars = 32;
char received_chars[num_chars];

bool new_data = false;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  Serial.setTimeout(1);

  adc_config(adc_handle, SMPL_RATE, FFT_SIZE, ADC_A1, OSR_4, ADC_14BIT);
  adc_setup(adc_handle, smpl_data);

  delay(1000);

  init_fft(fft_handle);
  fft_setup(fft_handle, FFT_SIZE, HIGH_IMPERIAL);

  


}

void loop() {
  // put your main code here, to run repeatedly:
  rec_with_end_marker();
  if(check_data('H')) {
    Serial.println(TX_SIZE/2);
    smpl();
    run_fft_w_mag_db(fft_handle, smpl_data, mag);

    tx_smpl(mag, FFT_SIZE, TX_SIZE, SMPL_RATE, OSR_RATIO);
  }
}


void print_smpl(float *data, uint16_t smpl_size, float smpl_rate_kHz, float osr_ratio) {
  float eqv_smpl_rate = smpl_rate_kHz/(osr_ratio*1000.0);
  for(uint16_t i = 0; i< smpl_size/2; i++) {
    Serial.print((eqv_smpl_rate/((float)smpl_size))*i);
    Serial.print(" kHz:");
    Serial.println(data[i],3);
  }
}

void tx_smpl(float *data, uint16_t smpl_size, uint16_t tx_size, float smpl_rate_kHz, float osr_ratio){
  Serial.println("H");
  delayMicroseconds(10);
  float eqv_smpl_rate = smpl_rate_kHz/(osr_ratio*1000.0);
  for(uint16_t i = 0; i < tx_size/2; i++) {
    Serial.print((eqv_smpl_rate/((float)smpl_size))*i);
    Serial.print("kHz:");
    Serial.print(data[i]);
    Serial.println("dBV");
  }
  Serial.println("!");
}

void smpl(){
  adc_software_trigger(adc_handle, smpl_data);
  while(1){
    if(adc_smpl_status(adc_handle) == 1) {
      break;
    }
  }
  delay(10);
  adc_transfer_data(adc_handle, smpl_data);
  adc_clear_status(adc_handle);
  for(int i = 0; i<FFT_SIZE; i++) {
    // smpl_data[i] *= 16;
    smpl_data[i] -= 8192;
    smpl_data[i] *= 4;
    // smpl_data[i] = 32000*sinf(2*3.14*i/FFT_SIZE);
  }
}

void rec_with_end_marker(){
  static byte ndx = 0;
  char end_marker = '\n';
  char rc;

  while(Serial.available() > 0 && new_data == false) {
    rc = Serial.read();
    if(rc != end_marker) {
      received_chars[ndx] = rc;
      ndx++;
      if(ndx >= num_chars) {
        ndx = num_chars -1;
      }
    } else {
      received_chars[ndx] = '\0';
      ndx = 0;
      new_data = true;
    }
  }
}

bool check_data(char check_val) {
  bool r = 0;
  if(new_data == true) {
    if(received_chars[0] == check_val) {
      r = 1;
    }
    new_data = false;
  }
  return r;
}

