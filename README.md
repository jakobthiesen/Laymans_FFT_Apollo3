# Laymans_FFT_Apollo3
A repository to share this "built in a shed DSP software" for the Apollo3 (Artemis Redboard). Currently, a library to use the ADC with DMA and an FFT library is included, with more to come.
(Bear with the errors in the grammar.)

At this time, four things can be found in this Laymans FFT folder: a library to use the ADC together with the DMA on the Apollo3 (Redboard Artemis as of right now), a library to perform a standard radix-2 FFT on a set of samples using the DSP instructions for the M4 core, a library to boost the core to 96 MHz and one to transmit the FFT data to a PC using an effecient custom serial protocol. The FFT performs quite well and is rather fast. To save memory and leverage flexibility, the FFT uses a 512-entry LUT for twiddle factors and a first-order Taylor approximation for values in between. The FFT uses Q15, i.e., 16-bit fixed-point math.

You can expect the following performance of you only sample and run FFT:
100 2048-point FFTs → 2.8087 s → 35.60 FFTs/s
100 512-point FFTs  → 0.6039 s → 165.60 FFTs/s
100 128-point FFTs  → 0.1363 s → 733.55 FFTs/s

To get up and running, make sure to use the Arduino IDE with the SparkFun drivers for the Apollo3.
To add the libraries, open %appdata%, navigate to Local -> Arduino15 -> packages -> SparkFun -> hardware -> apollo3 -> 2.1.1 (or whatever version you are using) -> libraries.

In this folder (libraries), add the Apoll3_ADC_LIB, Laymans_FFT, Apollo3_TurboSPOT and  PyPlot_Serial folders. This will allow you to simply import the libraries.

All libraries are built as close to a class as one can in pure C, using a handle as the UI.

For the ADC library, you must configure the following:
- An array to store the samples in; the DMA will unload the samples to this array.
- A sample frequency; this can be up to 1.2 MHz at 14 bits, 1.5 MHz at 12 bits, 2 MHz at 10 bits, and 2.4 MHz at 8 bits.
- A resolution; 8, 10, 12, and 14 bits are available.
- A pointer of the type adc_handle_t*; this will be used to interact with the library back-end.
- An oversampling ratio; the Apollo3 is capable of hardware-accelerated averaging, i.e., it can store x samples, find their mean, and store that mean at the targeted DMA address. Note that an oversampling ratio effectively reduces the sampling rate by that amount, but also improves SNR.
- An ADC channel to be used.

The following ENUMS are available for the user to set this up:
```c
typedef enum {
  ADC_A0 = AM_HAL_ADC_SLOT_CHSEL_SE1,
  ADC_A1 = AM_HAL_ADC_SLOT_CHSEL_SE2,
  ADC_A2 = AM_HAL_ADC_SLOT_CHSEL_SE6,
  ADC_A3 = AM_HAL_ADC_SLOT_CHSEL_SE5,
  ADC_A4 = AM_HAL_ADC_SLOT_CHSEL_SE0,
  ADC_A5 = AM_HAL_ADC_SLOT_CHSEL_SE3
} adc_pin_t;

typedef enum {
  OSR_1 = AM_HAL_ADC_SLOT_AVG_1,
  OSR_2 = AM_HAL_ADC_SLOT_AVG_2,
  OSR_4 = AM_HAL_ADC_SLOT_AVG_4,
  OSR_8 = AM_HAL_ADC_SLOT_AVG_8,
  OSR_16 = AM_HAL_ADC_SLOT_AVG_16,
  OSR_32 = AM_HAL_ADC_SLOT_AVG_32,
  OSR_64 = AM_HAL_ADC_SLOT_AVG_64,
  OSR_128 = AM_HAL_ADC_SLOT_AVG_128
} osr_t;

typedef enum {
  ADC_8BIT = AM_HAL_ADC_SLOT_8BIT,
  ADC_10BIT = AM_HAL_ADC_SLOT_10BIT,
  ADC_12BIT = AM_HAL_ADC_SLOT_12BIT,
  ADC_14BIT = AM_HAL_ADC_SLOT_14BIT
} adc_resolution_bits_t;
```

The following functions are available for the user for the ADC:

- `adc_handle_t* adc_get_handle(void);`
- `int8_t adc_setup(struct adc_handle_t *handle, int32_t *smpl_buffer);`
- `int8_t adc_software_trigger(struct adc_handle_t *handle, int32_t *smpl_buffer);`
- `int8_t adc_smpl_status(struct adc_handle_t *handle);`
- `int8_t adc_config(struct adc_handle_t *handle, uint32_t smpl_frq, uint32_t smpl_size, adc_pin_t pin, osr_t osr, adc_resolution_bits_t resolution);`
- `int8_t adc_clear_status(struct adc_handle_t *handle);`
- `int8_t adc_transfer_data(struct adc_handle_t *handle, int32_t *smpl_buffer);`
- `int8_t adc_get_true_smpl_frq(struct adc_handle_t *handle, float *true_smpl_frq);`

The Laymans_FFT library is built in much the same way. The LUT for twiddles, etc., is hidden from the user. All you as a user must do is set a sampling rate and an FFT size; this is typically the same size as the number of samples you have chosen for the DMA to dump.

**Note!** This is a radix-2 FFT; you can only perform this on powers of 2 sample sizes, i.e., 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, etc. Not much is gained after 8192 with the resolution and phase noise of the Apollo3.
You as the user must also decide on what window to use.

The following windows are available:
```c
typedef enum {
  RECTANGULAR,
  BARTLETT,
  HAMMING,
  HANN,
  IMPERIAL,
  HIGH_IMPERIAL,
  FLAT_TOP,
  BLACKMAN,
  BLACKMAN_HARRIS
} windows_t;
```

For most cases, I recommend using HAMMING, HANN, or IMPERIAL. For amplitude-specific tasks, use FLAT_TOP. For high-frequency separation, use BLACKMAN_HARRIS or HIGH_IMPERIAL.

IMPERIAL and HIGH_IMPERIAL are SINC windows of my own creation. They are simple SINC functions tailored to ensure 0 at the edges of the window and 1 at the center. They offer a balance between BLACKMAN_HARRIS and HANN windows.

The following functions are available in the FFT library:
- `fft_handle_t* fft_get_handle(void);`
- `int8_t init_fft(fft_handle_t *handle);`
- `int8_t fft_setup(fft_handle_t *handle, uint16_t smpl_size, windows_t window_type);`
- `int8_t run_fft(fft_handle_t *handle, int32_t *smpl_data);`
- `int8_t get_mag_db(fft_handle_t *handle, int32_t *fft_data, float *mag_data);`
- `int8_t run_fft_w_mag_db(fft_handle_t *handle, int32_t *smpl_data, float *mag_data);`

As for the ADC, one must interact with the back-end by using the `fft_handle_t*` pointer.

Finally, a simple Python script has been provided to plot the FFT data. Make sure you have installed `pyserial` and `matplotlib` to run this. I recommend using a virtual
