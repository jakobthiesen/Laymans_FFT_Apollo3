#include <Apollo3_ADC_LIB.h>
#include <Laymans_FFT.h>
#include <PyPlot_Serial.h>
#include <turboSPOT.h>

/*This is an example on how to use the Laymans_FFT library together with the Apollo3_ADC_LIB and PyPlot_Serial library for live-streaming fft
data. You should ensure that the input voltage of the selected analog channel is in the range of 0-2V, it is best to have any ac at a 
1V offset so you can use the full Dynamic range.

The functions available for FFT for the user are:
int8_t init_fft(fft_handle_t *handle);      -This initializes the library
int8_t fft_setup(fft_handle_t *handle, uint16_t smpl_size, windows_t window_type)     -This is used to configure the FFT for your sample size and window type.
int8_t run_fft(fft_handle_t *handle, uint32_t *smpl_data)     -This will run a full fft on the smpl_data array of the size in the fft_setup. 
                                                                FFT data overwrites the input data.
    -Note, data is returned as 2 16 bit values in a single 32 bit integer, 0-15 is real, 16-31 is imag, this is due to the use of CMSIS DSP instructions.
int8_t get_mag_db(fft_handle_t *handle, int32_t *fft_data, float *mag_data);      -This function will return the magnitude of the fft. data is stored in a float.
int8_t run_fft_w_mag_db(fft_handle_t *handle, int32_t *smpl_data, float *mag_data)      -This will run an fft and return the magnitude in the mag_data array.
All functions return with a 0 if there is no problem and a -1 or -2 if something fails. (-1 indicates NULL pointer)


You can expect the following performance when running the fft with magnitude and boosted core to 96 MHz:
// 100 2048 point FFTs performed in 2.8087 s, Resulting in 35.60 ffts/s
// 100 512 point FFTs performed in 0.6039 s, Resulting in 165.60 ffts/s
// 100 128 point FFTs performed in 0.1363 s, Resulting in 733.55 ffts/s
You will notice that there is a big performance decrease for the 2048 point fft.
This is due to the use of an internal LUT of size 512. When the FFT is this size or smaller, it can use direct look-up, 
when the fft is larger, it uses linea interpolation for accurate values, this more or less double the computation time,
it does however save a lot of RAM.
*/


//Different FFT and sampling parameters are defined here.
//Should you wish for a larger FFT such as 8192, simply define FFT size to that.
//Note that this is RADIX-2 FFT, it must be a power of 2.
#define FFT_SIZE 512
#define SMPL_RATE 1200000 //The desired sampling frequency. for 14 bits this caps out at 1.2 MSPS.
#define OSR_RATIO 1       //The over-sampling ratio, note that the sampling frequency is effectively reduced by this amount.
// Note: Oversampling effectively applies a low-pass filter (via averaging/decimation). 
// This means the frequency response is slightly attenuated near the high end of the visible spectrum.
adc_handle_t* adc_handle = adc_get_handle();    //The handle to configure and use the ADC is initialized here.
fft_handle_t* fft_handle = fft_get_handle();    //The handle to configure and use the FFT library is initialized here.

int32_t smpl_data[FFT_SIZE]; //This is where the sample data is stored, this will be where the DMA dumps the ADC samples.
float mag[FFT_SIZE]; //This is where the calculated magnitudes will be stored.
  

uint32_t timer = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  Serial.setTimeout(0.5);

  adc_config(adc_handle, SMPL_RATE, FFT_SIZE, ADC_A1, OSR_8, ADC_14BIT);    //Configure the ADC, if you wish for over-sampling use can use the following:
  //OSR_1 OSR_2 OSR_4 OSR_8 OSR_16  OSR_32  OSR_64  OSR_128

  //you can setup for the following analog channels:
  //ADC_A0  ADC_A1  ADC_A2  ADC_A3  ADC_A4  ADC_A5

  //You can set up the resolution to the following, including the max sample rates:
  //ADC_8BIT, allows for 2.4 MSPS
  //ADC_10BIT, allows for 2 MSPS
  //ADC_12BIT, allows for 1.5 MSPS (1.6MSPS in theory but integer limitations sets this to 1.5 MSPS)
  //ADC_14BIT, allows for 1.2 MSPS
  //Note that the ADC library will limit the sampling rate to the maximum allowable for the different resolutions.
  //This means, if you set the resolution to 14 bits and sampling rate to 2MSPS, you will only get 1.2 MSPS.

  adc_setup(adc_handle, smpl_data);   //Setup the ADC and DMA.

  delay(1000);//Allow the ADC core to start etc.
  init_fft(fft_handle); //Initialize the FFT library, setting up internal call-back functions etc.
  fft_setup(fft_handle, FFT_SIZE, HIGH_IMPERIAL);  //Setup the FFT for the desired sample-size and window function
  //The following windows are available:
  //  RECTANGULAR,      - Simple, offers very bad side lobes
  //  BARTLETT,         - A bit better than Rectangular, still not very good
  //  HAMMING,          - Offers narrow main lobe, large side lobes
  //  HANN,             - a good all-round window
  //  IMPERIAL,         - offers balance between HANN and Blackman
  //  HIGH_IMPERIAL,    - Offers smaller side-lobes than Imperial
  //  FLAT_TOP,         - Exceptionally good at amplitude measurements, not great frequency resolution
  //  BLACKMAN,         - Offers good side-lobe suppression and frequency resolution 
  //  BLACKMAN_HARRIS   - An improved version of Blackman

  pinMode(13, OUTPUT);

  // timer = micros();
  
  // //Run 100 FFTs and time them, print the resulting ffts/s
  // for(int i = 0; i < 100; i ++) {
  //   smpl();
  //   REQUEST_TURBO_SPOT(); // Boost the core clock to 96 MHz for the FFT calculations.
  //   run_fft_w_mag_db(fft_handle, smpl_data, mag);
  //   // run_fft(fft_handle, smpl_data);
  //   STOP_TURBO_SPOT();
  // }
  // timer = micros()-timer;
  // float fft_s = 100/((float)timer/1000000);
  // Serial.print("100 ");
  // Serial.print(FFT_SIZE);
  // Serial.print(" point FFTs performed in ");
  // Serial.print((float)timer/1000000.0,4);
  // Serial.print(" s, Resulting in ");
  // Serial.print(fft_s);
  // Serial.println(" ffts/s");

}


void loop() {

  //Await FFT request from PC
  if(fft_request() == true) {
    tx_ack();
  
    bool fft_status = 1;
    digitalWrite(13, fft_status);
    smpl();
    tx_ack();
    REQUEST_TURBO_SPOT(); //Boost core to 96 MHz
    run_fft_w_mag_db(fft_handle, smpl_data, mag);
    STOP_TURBO_SPOT();  //Reduce clock to 48 MHz
    fft_status = 0;
    digitalWrite(13, fft_status);
    tx_fft(mag, FFT_SIZE, (SMPL_RATE/OSR_RATIO)*0.9959072); //Transmit FFT data to PC, the sample rate is multiplied by a correction, as the sampling rate is not precisely 1.2MSPS.
  }
}


//Function used for sampling.
void smpl(){
  adc_software_trigger(adc_handle, smpl_data);
  while(1){
    if(adc_smpl_status(adc_handle) == 1) {
      break;
    }
  }
  adc_transfer_data(adc_handle, smpl_data);
  adc_clear_status(adc_handle);
  for(int i = 0; i<FFT_SIZE; i++) {
    smpl_data[i] -= 8192;   //Remove 1 V DC offset
    smpl_data[i] *=4;       //Scale to 16 bit value to fit the 16 bit fixed-point math of the FFT.

  }
}









