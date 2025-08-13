
#include "PyPlot_Serial.h"
#include <Arduino.h>
#include <stdint.h>


#define START_BYTE 'S'
#define ACK_BYTE 'K'
#define DONE_BYTE 'D'
#define END_BYTE 0XFF

const byte num_chars = 32;
char received_chars[num_chars];
bool new_data = false;

void tx_setup(uint32_t rate, float timeout){
  Serial.begin(rate);
  Serial.setTimeout(timeout);
}


bool fft_request() {
  if(Serial.available() > 0) {
    char c = Serial.read();
    if(c == START_BYTE) {
      return true;
    }
  }
  return false;
}

void tx_ack() {
  Serial.write(ACK_BYTE);
}

void tx_fft(float *data, uint16_t smpl_size, float smpl_rate_kHz) {
  Serial.write(DONE_BYTE);
  uint16_t bins = smpl_size/2;
  uint16_t tx_data = 0;
  Serial.write((uint8_t*)&bins, sizeof(bins));
  delay(5);
  Serial.write((uint8_t*)&smpl_rate_kHz, sizeof(smpl_rate_kHz));
  for(uint16_t i = 0; i<bins; i++) {
    tx_data = (uint16_t)(abs(data[i])*819.1875);
    Serial.write((uint8_t*)&tx_data, sizeof(uint16_t));
  }
}

void tx_end() {
  Serial.write(END_BYTE);
}

void tx_smpl(float *data, uint16_t smpl_size, uint16_t tx_size, float smpl_rate_kHz, float osr_ratio){
  Serial.println("H");
  delayMicroseconds(10);
  float eqv_smpl_rate = smpl_rate_kHz/(osr_ratio*1000.0);
  float frq = 0;
  float mag = 0;
  for(uint16_t i = 0; i < tx_size/2; i++) {

    // Serial.print((eqv_smpl_rate/((float)smpl_size))*i);
    frq = (eqv_smpl_rate/((float)smpl_size))*i;
    mag = data[i];
    Serial.write((uint8_t*)&frq, sizeof(frq));
    Serial.write((uint8_t*)&mag, sizeof(mag));
    // Serial.print("F:");
    // Serial.print(data[i]);
    // Serial.println("V");

  }
  Serial.println("!");
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
