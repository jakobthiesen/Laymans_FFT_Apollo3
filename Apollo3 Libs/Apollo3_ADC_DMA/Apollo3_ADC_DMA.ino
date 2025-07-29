//*****************************************************************************
//
//! @file adc_lpmode0_dma.c
//!
//! @brief This example takes samples with the ADC at high-speed using DMA.
//!
//! Purpose: This example shows the CTIMER-A3 triggering repeated samples of an external
//! input at 1.2Msps in LPMODE0.  The example uses the CTIMER-A3 to trigger
//! ADC sampling.  Each data point is 8 sample average and is transferred
//! from the ADC FIFO into an SRAM buffer using DMA.

#include "am_bsp.h"

// Define a circular buffer to hold the ADC samples
//*****************************************************************************
#define ADC_EXAMPLE_DEBUG   1
#define ResolutionBits 14

// ADC Sample buffer.
#define ADC_SAMPLE_BUF_SIZE 512
uint32_t g_ui32ADCSampleBuffer[ADC_SAMPLE_BUF_SIZE];

am_hal_adc_sample_t SampleBuffer[ADC_SAMPLE_BUF_SIZE];

unsigned long useconds_refresh;

// ADC Device Handle.
//static void *g_ADCHandle;


// ADC DMA complete flag.
volatile bool                   g_bADCDMAComplete;

// ADC DMA error flag.
volatile bool                   g_bADCDMAError;
static void *g_ADCHandle;
// Define the ADC SE0 pin to be used. (A4)
const am_hal_gpio_pincfg_t g_AM_PIN_11_ADCSE2 =
{
    .uFuncSel       = AM_HAL_PIN_11_ADCSE2,
};


// Interrupt handler for the ADC.
//*****************************************************************************
extern "C" void am_adc_isr()  //must use 'extern "C" ' for the Artemis
{
    uint32_t ui32IntMask;

    // Read the interrupt status.
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_interrupt_status(g_ADCHandle, &ui32IntMask, false))
    {
        //am_util_stdio_printf("Error reading ADC interrupt status\n");
        Serial.println("Error reading ADC interrupt status");
    }

    // Clear the ADC interrupt.
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_interrupt_clear(g_ADCHandle, ui32IntMask))
    {
        //am_util_stdio_printf("Error clearing ADC interrupt status\n");
        Serial.println("Error clearing ADC interrupt status");
    }

    // If we got a DMA complete, set the flag.
    if (ui32IntMask & AM_HAL_ADC_INT_DCMP)
    {
        g_bADCDMAComplete = true;
    }

    // If we got a DMA error, set the flag.
    if (ui32IntMask & AM_HAL_ADC_INT_DERR)
    {
        g_bADCDMAError = true;
    }
}

// Set up the core for sleeping, and then go to sleep.
//*****************************************************************************
void coreSleep()
{
    // Disable things that can't run in sleep mode.
//#if (0 == ADC_EXAMPLE_DEBUG)
//    am_bsp_debug_printf_disable();
//#endif

    // Go to Deep Sleep.
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    
    // Re-enable peripherals for run mode.
//#if (0 == ADC_EXAMPLE_DEBUG)
//    am_bsp_debug_printf_enable();
//#endif

}

// Configure the ADC.
//*****************************************************************************
void adc_config_dma()
{
    am_hal_adc_dma_config_t       ADCDMAConfig;

    // Configure the ADC to use DMA for the sample transfer.
    ADCDMAConfig.bDynamicPriority = true;
    ADCDMAConfig.ePriority = AM_HAL_ADC_PRIOR_SERVICE_IMMED;
    ADCDMAConfig.bDMAEnable = true;
    ADCDMAConfig.ui32SampleCount = ADC_SAMPLE_BUF_SIZE;
    ADCDMAConfig.ui32TargetAddress = (uint32_t)g_ui32ADCSampleBuffer;
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_configure_dma(g_ADCHandle, &ADCDMAConfig))
    {
        //am_util_stdio_printf("Error - configuring ADC DMA failed.\n");
        Serial.println("Error - configuring ADC DMA failed.");
    }
    // Reset the ADC DMA flags.
    g_bADCDMAComplete = false;
    g_bADCDMAError = false;
    
}


// Configure the ADC.
//*****************************************************************************
void adc_config()
{
    am_hal_adc_config_t           ADCConfig;
    am_hal_adc_slot_config_t      ADCSlotConfig;

    // Initialize the ADC and get the handle.
    if ( AM_HAL_STATUS_SUCCESS != am_hal_adc_initialize(0, &g_ADCHandle) )
    {
        //am_util_stdio_printf("Error - reservation of the ADC instance failed.\n");
        Serial.println("Error - reservation of the ADC instance failed.");
    }

    // Power on the ADC.
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_power_control(g_ADCHandle,
                                                          AM_HAL_SYSCTRL_WAKE,
                                                          false) )
    {
        //am_util_stdio_printf("Error - ADC power on failed.\n");
        Serial.println("Error - ADC power on failed.");
    }

    // Set up the ADC configuration parameters. These settings are reasonable
    // for accurate measurements at a low sample rate.
    ADCConfig.eClock             = AM_HAL_ADC_CLKSEL_HFRC;
    ADCConfig.ePolarity          = AM_HAL_ADC_TRIGPOL_RISING;
    ADCConfig.eTrigger           = AM_HAL_ADC_TRIGSEL_SOFTWARE;
    ADCConfig.eReference         = AM_HAL_ADC_REFSEL_INT_2P0;
    /*
    AM_HAL_ADC_REFSEL_INT_2P0,
    AM_HAL_ADC_REFSEL_INT_1P5,
    AM_HAL_ADC_REFSEL_EXT_2P0,
    AM_HAL_ADC_REFSEL_EXT_1P5
    */
    ADCConfig.eClockMode         = AM_HAL_ADC_CLKMODE_LOW_LATENCY;
    ADCConfig.ePowerMode         = AM_HAL_ADC_LPMODE0; 
    ADCConfig.eRepeat            = AM_HAL_ADC_REPEATING_SCAN; //AM_HAL_ADC_SINGLE_SCAN
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_configure(g_ADCHandle, &ADCConfig))
    {
        //am_util_stdio_printf("Error - configuring ADC failed.\n");
        Serial.println("Error - configuring ADC failed.");
    }

    // Set up an ADC slot
    ADCSlotConfig.eMeasToAvg      = AM_HAL_ADC_SLOT_AVG_1;
    ADCSlotConfig.ePrecisionMode  = AM_HAL_ADC_SLOT_14BIT;
    ADCSlotConfig.eChannel        = AM_HAL_ADC_SLOT_CHSEL_SE2; 
    ADCSlotConfig.bWindowCompare  = false;
    ADCSlotConfig.bEnabled        = true;
    /*
    AM_HAL_ADC_SLOT_AVG_1,
    AM_HAL_ADC_SLOT_AVG_2,
    AM_HAL_ADC_SLOT_AVG_4,
    AM_HAL_ADC_SLOT_AVG_8,
    AM_HAL_ADC_SLOT_AVG_16,
    AM_HAL_ADC_SLOT_AVG_32,
    AM_HAL_ADC_SLOT_AVG_64,
    AM_HAL_ADC_SLOT_AVG_128
    
    AM_HAL_ADC_SLOT_14BIT,
    AM_HAL_ADC_SLOT_12BIT,
    AM_HAL_ADC_SLOT_10BIT,
    AM_HAL_ADC_SLOT_8BIT

        // Single-ended channels
    AM_HAL_ADC_SLOT_CHSEL_SE0, (pad 16, ArtemisRB A4)
    AM_HAL_ADC_SLOT_CHSEL_SE1, (pad 29, ArtemisRB A0)
    AM_HAL_ADC_SLOT_CHSEL_SE2, (pad 11, ArtemisRB A1)
    AM_HAL_ADC_SLOT_CHSEL_SE3, (pad 31, ArtemisRB A5)
    AM_HAL_ADC_SLOT_CHSEL_SE4, (pad 32, ArtemisRB A8)
    AM_HAL_ADC_SLOT_CHSEL_SE5, (pad 33, ArtemisRB A3)
    AM_HAL_ADC_SLOT_CHSEL_SE6, (pad 34, ArtemisRB A2)
    AM_HAL_ADC_SLOT_CHSEL_SE7, (pad 35, ArtemisRB n/a)
    AM_HAL_ADC_SLOT_CHSEL_SE8, (pad 13, ArtemisRB A10)
    AM_HAL_ADC_SLOT_CHSEL_SE9, (pad 12, ArtemisRB A9)
    // Differential channels. 
    AM_HAL_ADC_SLOT_CHSEL_DF0, pads 12 (-), pad 13(+)
    AM_HAL_ADC_SLOT_CHSEL_DF1, pads 15 (-), pad 14(+) (ArtemisRB n/a)
    // Miscellaneous other signals.
    AM_HAL_ADC_SLOT_CHSEL_TEMP,
    AM_HAL_ADC_SLOT_CHSEL_BATT,
    AM_HAL_ADC_SLOT_CHSEL_VSS


     */
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_configure_slot(g_ADCHandle, 0, &ADCSlotConfig))
    {
        //am_util_stdio_printf("Error - configuring ADC Slot 0 failed.\n");
        Serial.println("Error - configuring ADC Slot 0 failed.");
    }

    // Configure the ADC to use DMA for the sample transfer.
    adc_config_dma();
    // For this example, the samples will be coming in slowly. This means we
    // can afford to wake up for every conversion.
    //
    am_hal_adc_interrupt_enable(g_ADCHandle, AM_HAL_ADC_INT_DERR | AM_HAL_ADC_INT_DCMP );

    // Enable the ADC.
    //
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_enable(g_ADCHandle))
    {
        //am_util_stdio_printf("Error - enabling ADC failed.\n");
        Serial.println("Error - enabling ADC failed.");
    }
}


// Initialize the ADC repetitive sample timer A3.
//*****************************************************************************
void init_timerA3_for_ADC()
{    
    // Start a timer to trigger the ADC periodically (1 second).?? Must be a hold-over from another version
    am_hal_ctimer_config_single(3, AM_HAL_CTIMER_TIMERA,
                                AM_HAL_CTIMER_HFRC_12MHZ    |
                                AM_HAL_CTIMER_FN_REPEAT     |
                                AM_HAL_CTIMER_INT_ENABLE);

    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA3);

    am_hal_ctimer_period_set(3, AM_HAL_CTIMER_TIMERA, 10, 5); //10 tick period, 5 ticks wide

    // Enable the timer A3 to trigger the ADC directly
    am_hal_ctimer_adc_trigger_enable();

    // Start the timer.
    am_hal_ctimer_start(3, AM_HAL_CTIMER_TIMERA);
}


void setup() {
     Serial.begin(500000);
     while (!Serial) {}
     Serial.println("Hi-speed ADC test");
    // Set the clock frequency.
     if (AM_HAL_STATUS_SUCCESS != am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0))
    {
        //am_util_stdio_printf("Error - configuring the system clock failed.\n");
        Serial.println("Error - configuring the system clock failed.");
    }

    // Set the default cache configuration and enable it.
    if (AM_HAL_STATUS_SUCCESS != am_hal_cachectrl_config(&am_hal_cachectrl_defaults))
    {
        //am_util_stdio_printf("Error - configuring the system cache failed.\n");
        Serial.println("Error - configuring the system cache failed.");
    }
    if (AM_HAL_STATUS_SUCCESS != am_hal_cachectrl_enable())
    {
        //am_util_stdio_printf("Error - enabling the system cache failed.\n");
        Serial.println("Error - enabling the system cache failed.");
    }

    // Configure the board for low power operation.
    // am_bsp_low_power_init();

    // Enable only the first 512KB bank of Flash (0).  Disable Flash(1)
    if (AM_HAL_STATUS_SUCCESS != am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_FLASH_512K))
    {
        //am_util_stdio_printf("Error - configuring the flash memory failed.\n");
        Serial.println("Error - configuring the flash memory failed.");
    }

    // Enable the first 32K of TCM SRAM.
    if (AM_HAL_STATUS_SUCCESS != am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_SRAM_MAX))
    {
        //am_util_stdio_printf("Error - configuring the SRAM failed.\n");
        Serial.println("Error - configuring the SRAM failed.");
    }

    // Start the ITM interface.
    am_bsp_itm_printf_enable();

    // Start the CTIMER A3 for timer-based ADC measurements.
    init_timerA3_for_ADC();

    // Enable interrupts.
    NVIC_EnableIRQ(ADC_IRQn);
    am_hal_interrupt_master_enable();

    // Set a pin to act as our ADC input
    am_hal_gpio_pinconfig(11, g_AM_PIN_11_ADCSE2); //artemis pad 16 is A4

    // Configure the ADC
    adc_config();

    // Trigger the ADC sampling for the first time manually.
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_sw_trigger(g_ADCHandle))
    {
        //am_util_stdio_printf("Error - triggering the ADC failed.\n");
        Serial.println("Error - triggering the ADC failed.");
    }

    // Print the banner.
    am_util_stdio_terminal_clear();
    //am_util_stdio_printf("ADC Example with 1.2Msps and LPMODE=0\n");
    Serial.println("ADC Example with 1.2Msps and LPMODE=0");

    // Allow time for all printing to finish.
    am_util_delay_ms(10);

    // We are done printing. Disable debug printf messages on ITM.
//#if (0 == ADC_EXAMPLE_DEBUG)
//    am_bsp_debug_printf_disable();
//#endif

  
}

void loop() {
        // Go to Deep Sleep.
        useconds_refresh = micros();
        if (!g_bADCDMAComplete)
        {
            coreSleep();
        }

        // Check for DMA errors.
        if (g_bADCDMAError)
        {
            //am_util_stdio_printf("DMA Error occured\n");
            Serial.println("DMA Error occured");
            while(1);
        }

        // Check if the ADC DMA completion interrupt occurred.
        if (g_bADCDMAComplete)
        {  
            for (int i=0; i<ADC_SAMPLE_BUF_SIZE; i++){
                  //14 bit is 0-16383 on a range of 0-2V  
                uint32_t result = g_ui32ADCSampleBuffer[i];//*(2.0 /16383.0);
                //Shift result depending on resolution, averaging
                result = result >> (6);
               // Serial.println(result);
                Serial.println(result*(2.0 /16383.0),3);
              }
             while(1){}   //pause execution


            // Reset the DMA completion and error flags.
            g_bADCDMAComplete = false;

            // Re-configure the ADC DMA.
            adc_config_dma();

            // Clear the ADC interrupts.
            if (AM_HAL_STATUS_SUCCESS != am_hal_adc_interrupt_clear(g_ADCHandle, 0xFFFFFFFF))
            {
                //am_util_stdio_printf("Error - clearing the ADC interrupts failed.\n");
                Serial.println("Error - clearing the ADC interrupts failed.");
            }

            // Trigger the ADC sampling for the first time manually.
            if (AM_HAL_STATUS_SUCCESS != am_hal_adc_sw_trigger(g_ADCHandle))
            {
                //am_util_stdio_printf("Error - triggering the ADC failed.\n");
                Serial.println("Error - triggering the ADC failed.");
            }
        } // if ()

}