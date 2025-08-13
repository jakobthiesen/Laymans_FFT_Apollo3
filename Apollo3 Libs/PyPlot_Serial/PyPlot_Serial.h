#ifndef PYPLOT_SERIAL_H
#define PYPLOT_SERIAL_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

void TX_setup(uint32_t rate, float timeout);
void tx_smpl(float *data, uint16_t smpl_size, uint16_t tx_size, float smpl_rate_kHz, float osr_ratio);
void rec_with_end_marker();
bool check_data(char check_val);


void tx_end();
void tx_fft(float *data, uint16_t smpl_size, float smpl_rate_kHz);
void tx_ack();
bool fft_request();


#ifdef __cplusplus
}
#endif
#endif