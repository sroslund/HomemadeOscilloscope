/* ========================================
 *
 * Tiny Scope for the PSoC 6 code
 * 
 * Author: Scott Oslund
 *
 * Program Synopsis:
 * This program contains the code to run the PSoC 6 Tinyscope.
 * It has the main functions, ISRs, and the tasks the main loop
 * must complete. This file is dependent on the files HelperFunctions.h
 * and HelperFunctions.c for functions, structures, and defines. When
 * run with these files on the PSoC 6 connected to the newHaven display
 * the tinyscope will run.
 *
 * ========================================
*/

/* All defines and structs are in HelperFunctions.h */

/* Included libraries */
#include "HelperFunctions.h"                                                  // this file also has additional included files within it

SCOPE_SETTINGS SCOPE = {DEFAULT,DEFAULT,TRUE,POSITIVE,DEFAULT, FALSE, TRUE};  // instatiating the scope structure with the default values

WAVEFORM_DATA WAVE = {{0},{0},{0},{0},FALSE,FALSE,{0},{0},{0},{0},0,0,0,0};   // intantiating the wave structure with the default values

/* Ping pong buffers for storing adc data - 2 per channel */
uint16_t CH1_Data1[SIZE];                                                     // channel 1 ping pong buffers
uint16_t CH1_Data2[SIZE];
uint16_t CH2_Data1[SIZE];                                                     // channel 2 ping pong buffers
uint16_t CH2_Data2[SIZE];

/* Flags */
uint8_t CH1_FLAG = FALSE;                                                     // channel 1 flag to indicate the DMA finished
int ReadyToDraw_ch1 = FALSE;                                                  // flag for drawing each channel


/*
CH1_ISR:
This simple ISR responds to a descriptor finishing filling an array of data read from channel 1. It clears the interrupt,
raises a flag, and updates the buffer that was most recently finised.
*/
void CH1_ISR()
{
    Cy_DMA_Channel_ClearInterrupt(DMA_1_HW,DMA_1_DW_CHANNEL);      // clearing the interrupt
    
    CH1_FLAG = TRUE;                                               // raising a flag indicating an event has occured for main to respond to
    
    if(WAVE.Wave1_Buffer1){                                        // If buffer 1 was last read from it is no longer not ready to be read from (false) otherwise we set it to true
        WAVE.Wave1_Buffer1 = FALSE;   
    }else{
        WAVE.Wave1_Buffer1 = TRUE;   
    }
}

/*
CH1_ISR:
This simple ISR responds to a descriptor finishing filling an array of data read from channel 2. It clears the interrupt,
raises a flag, and updates the buffer that was most recently finised.
*/
void CH2_ISR()
{
    Cy_DMA_Channel_ClearInterrupt(DMA_2_HW,DMA_2_DW_CHANNEL);      // clearing the interrupt
    
    if(WAVE.Wave2_Buffer1){                                        // If buffer 1 was last read from it is no longer not ready to be read from (false) otherwise we set it to true
        WAVE.Wave2_Buffer1 = FALSE;   
    }else{
        WAVE.Wave2_Buffer1 = TRUE;   
    }
}

/*
Process_Channel:
This function is responsible for extracting data from the channels of the ADC whenever a ping
pong buffer is filled. It will calculate the frequency and create the coordinates for drawing
the pixels all while respecting the scope settings.
*/
void Proccess_Channel()
{
    
    /* variables - all static so the function begins where it left off time it is called*/
    static int i=0;                                               // this variable is used in for loops indexing the coordinate array
    static uint16_t iterations1 = 0;                              // keeps track of number of times the function was called
    static uint16_t middleVal = 0;                                // keeps track of middle data point of the current buffer for channel 1
    static uint16_t middleVal2 = 0;                               // keeps track of middle data point of the current buffer for channel 2
    static uint64_t index=0;                                      // for indexing the ping-pong buffer
    
    if(iterations1 == READY_TO_START){
        ReadyToDraw_ch1 = FALSE;                                  // when enough iterations pass that we are ready to update the data we are no longer ready to draw    
    }
    
    if(iterations1 == FIND_MIDDLE){                               // at this iteration stage we calculate the middle of both channels
        if(WAVE.Wave1_Buffer1){
            middleVal = Middle(CH1_Data1);                        // calculating the middle of ping-pong buffer 1 or 2 depending on which last finished updating   
        } else {
            middleVal = Middle(CH1_Data2);   
        }
        if(WAVE.Wave2_Buffer1){                                   // repeating process for second channel
            middleVal2 = Middle(CH2_Data1);    
        } else {
            middleVal2 = Middle(CH2_Data2);   
        }
    }
    
    if(iterations1 == FIND_FREQ){                                 // at this iteration stage we calculate the signal's frequency using the middle value
        uint16_t freq = 0;
        if(middleVal == 0){
            freq = 0;                                             // if there was no middle value (a flat signal) there is no frequency - we set it to 0
        } else if(WAVE.Wave1_Buffer1){
            freq = FindFrequency(CH1_Data1,middleVal);            // otherwise we get the frequency from one the channels depending on which ping-pong buffer was last used; 
        } else {
            freq = FindFrequency(CH1_Data2,middleVal); 
        }
        if(freq != ERROR){
            WAVE.Freq1 = freq;   
        }
        if(middleVal2 == 0){                                       // we repeat for channel 2
            freq = 0;                                              // if there was no middle value (a flat signal) there is no frequency - we set it to 0
        }else if(WAVE.Wave2_Buffer1){
            freq = FindFrequency(CH2_Data1,middleVal2);            // otherwise we get the frequency from one the channels depending on which ping-pong buffer was last used 
        } else {
            freq = FindFrequency(CH2_Data2,middleVal2); 
        }
        if(freq != ERROR){
            WAVE.Freq2 = freq;   
        }
    }
    
    if(iterations1 == FORMAT_DATA &&                               // at this iteration stage we look for a trigger if the scope is in trigger mode
        !(SCOPE.freeRun || SCOPE.triggerChannel != CHANNEL_1)){
        if(WAVE.Wave1_Buffer1){
            index = FindTrigger(CH1_Data1,SCOPE);                  // looking for a trigger in channel 1 buffer 1
            if(index == ERROR){                                    // if we get an error we restart and look for a new trigger
                iterations1--;
                index = 0;
                goto reset;
            }
        } else {
            index = FindTrigger(CH1_Data2, SCOPE);                  // looking for a trigger in channel 1 buffer 2
            if(index == ERROR){                                     // if we get an error we restart and look for a new trigger
                iterations1--;
                index = 0;
                goto reset;  
            }
        }
    } else if(iterations1 == FORMAT_DATA &&                         // we repeat if the trigger channel is channel 2
        !(SCOPE.freeRun || SCOPE.triggerChannel != CHANNEL_2)){
        if(WAVE.Wave2_Buffer1){
            index = FindTrigger(CH2_Data2, SCOPE);                  // looking for a trigger in channel 2 buffer 1  
            if(index == ERROR){                                     // if we get an error we restart and look for a new trigger
                iterations1--;
                index = 0;
                goto reset;
            }
        } else {
            index = FindTrigger(CH2_Data1, SCOPE);                  // looking for a trigger in channel 2 buffer 2
            if(index == ERROR){                                     // if we get an error we restart and look for a new trigger
                iterations1--;
                index = 0;
                goto reset;  
            }
        }
    }else {
        index = 0;                                                  // if we are in free run mode we start at index 0 of the data
    }
    
    if(iterations1 >= FORMAT_DATA){                                 // when we have passed the trigger check above, we start printing the data
        if(WAVE.Wave1_Buffer1){                                     // we check which buffer we should read from
            for(;i<X_PIXELS;i++){                                   // iterating through all pixels to set to create a waveform
                WAVE.Wave1X[i] = i;
                if(CH1_Data1[index/INDEX_SCALE] & UNDERFLOW_CHECK){
                    WAVE.Wave1Y[i] = 0;                             // if there is overflow we set y coordinate to zero
                } else {                                            // otherwise we set the y-coordinate to the scaled ADC value
                    WAVE.Wave1Y[i] = (-CH1_Data1[index/INDEX_SCALE]*VOLTAGE_INT*SCOPE.yScale/(MAX_ADC_OUTPUT*VOLTAGE_SCALE_DOWN));
                }
                if(WAVE.Wave2_Buffer1){                             // we repeat the process for channel 2
                        WAVE.Wave2X[i] = i;                               
                        if(CH2_Data1[index/INDEX_SCALE] & UNDERFLOW_CHECK){
                            WAVE.Wave2Y[i] = 0;                     // adding channel 2 data to the channel 2 coordinates        
                        } else {                                          
                            WAVE.Wave2Y[i] = -CH2_Data1[index/INDEX_SCALE]*SCOPE.yScale*VOLTAGE_INT/(MAX_ADC_OUTPUT*VOLTAGE_SCALE_DOWN);
                        }
                    } else {
                        WAVE.Wave2X[i] = i;                         // adding channel 2 data to the channel 2 coordinates                 
                        if(CH2_Data2[index/INDEX_SCALE] & UNDERFLOW_CHECK){
                            WAVE.Wave2Y[i] = 0;                          
                        } else {                                          
                            WAVE.Wave2Y[i] = -CH2_Data1[index/INDEX_SCALE]*SCOPE.yScale*VOLTAGE_INT/(MAX_ADC_OUTPUT*VOLTAGE_SCALE_DOWN);
                        }
                    }
                index += (SCOPE.xScale*INDEX_SCALE)/INDEX_DIVISOR;  // updating the index (it is scaled to prevent floating point math)
                if(index > MAX_INDEX){
                    index -= MAX_INDEX;
                    goto reset;                                     // we reset the index and function if the index is out of bounds from the array
                }
            }
            i=0;
            iterations1 = 0;
            ReadyToDraw_ch1 = TRUE;                                 // if we get to this point we are done - we are ready to draw
        } else {                                                    // this is a repeat of the code above but using channel 1's buffer 2
            for(;i<X_PIXELS;i++){
                WAVE.Wave1X[i] = i;
                if(CH1_Data2[index/INDEX_SCALE] & UNDERFLOW_CHECK){
                    WAVE.Wave1Y[i] = 0;                             // if there is overflow we set y coordinate to zero 
                } else {                                            // otherwise we set the y-coordinate to the scaled ADC value 
                    WAVE.Wave1Y[i] = (-CH1_Data2[index/INDEX_SCALE]*VOLTAGE_INT*SCOPE.yScale/(MAX_ADC_OUTPUT*VOLTAGE_SCALE_DOWN));
                }
                if(WAVE.Wave2_Buffer1){
                        WAVE.Wave2X[i] = i;                         // adding channel 2 data to the channel 2 coordinates                          
                        if(CH2_Data1[index/INDEX_SCALE] & UNDERFLOW_CHECK){
                            WAVE.Wave2Y[i] = 0;                     // if there is overflow we set y coordinate to zero                          
                        } else {                                    // otherwise we set the y-coordinate to the scaled ADC value                                         
                            WAVE.Wave2Y[i] = -CH2_Data2[index/INDEX_SCALE]*SCOPE.yScale*VOLTAGE_INT/(MAX_ADC_OUTPUT*VOLTAGE_SCALE_DOWN);
                        }
                    } else {
                        WAVE.Wave2X[i] = i;                         // adding channel 2 data to the channel 2 coordinates                                
                        if(CH2_Data2[index/INDEX_SCALE] & UNDERFLOW_CHECK){
                            WAVE.Wave2Y[i] = 0;                     // if there is overflow we set y coordinate to zero                         
                        } else {                                          
                            WAVE.Wave2Y[i] = -CH2_Data2[index/INDEX_SCALE]*SCOPE.yScale*VOLTAGE_INT/(MAX_ADC_OUTPUT*VOLTAGE_SCALE_DOWN);
                        }
                    }
                index += (SCOPE.xScale*INDEX_SCALE)/INDEX_DIVISOR;  // updating the index (it is scaled to prevent floating point math)
                if(index >= MAX_INDEX){
                    index -= MAX_INDEX;
                    goto reset;                                     // we reset the index and function if the index is out of bounds from the array
                }
            }
            i=0;
            iterations1 = 0;
            ReadyToDraw_ch1 = TRUE;                                 // if we get to this point we are done - we are ready to draw
        }
    }
    
    reset:
    iterations1++;                                                  // incrementing the number of times we passed through this function
}


/*
UpdateDisplay:
This function updates the dispaly by drawing over the previous waveforms, reseting the background,
and drawing the new waveform
*/
void UpdateDisplay()
{
    GUI_SetPenSize(2);
    GUI_SetColor(GUI_BLACK);
    DrawWaveForm(WAVE.Prev_Wave2X,WAVE.Prev_Wave2Y,X_PIXELS,Y_PIXELS-WAVE.Wave2Offset);   // drawing over previous waveforms with the background color
    DrawWaveForm(WAVE.Prev_Wave1X,WAVE.Prev_Wave1Y,X_PIXELS,Y_PIXELS-WAVE.Wave1Offset);
    SetBackground(SCOPE, WAVE);                                                           // reseting the background
            
    WAVE.Wave1Offset = ADC_GetResult16(1) / ADC_SCALE_DOWN;                               // reading from potentiometers to allow for scrolling
    WAVE.Wave2Offset = ADC_GetResult16(3) / ADC_SCALE_DOWN;
    
    GUI_SetColor(GUI_YELLOW);
    DrawWaveForm(WAVE.Wave2X,WAVE.Wave2Y,X_PIXELS,Y_PIXELS-WAVE.Wave2Offset);             // drawing the waveforms
    GUI_SetColor(GUI_RED);
    DrawWaveForm(WAVE.Wave1X,WAVE.Wave1Y,X_PIXELS,Y_PIXELS-WAVE.Wave1Offset);
    Copy(WAVE.Wave1X,WAVE.Prev_Wave1X);                                                   // copying the data so we know what to erase next time
    Copy(WAVE.Wave2X,WAVE.Prev_Wave2X);
    Copy(WAVE.Wave2Y,WAVE.Prev_Wave2Y);
    Copy(WAVE.Wave1Y,WAVE.Prev_Wave1Y);
}

/*
Main:
This function first waits for the user to enter in start, then it inits all of the hardware. It starts the DMAs and ADC and responds to 
interrupt events by calling functions to process the data and by printing the waveforms
*/
int main(void)
{
    
    /* Initialization code */
    __enable_irq();                                                                // enabling interrupts 
    
    Cy_SCB_UART_Init(UART_HW, &UART_config,&UART_context);                         // initializing the UART
    Cy_SCB_UART_Enable(UART_HW);                                                   // enabling the UART
    
    UART_PutString("Welcome to Scott Oslund's oscilloscope!\n");                   // printing welcome message
    
    while(SCOPE.Running != TRUE){                                                  // waiting in infinite loop for user to enter start to begin the scope
        GetInput(&SCOPE);
    }
    
    /* Initializing the interrupts of the DMAs for both channel 2 and channel 1 */
    Cy_SysInt_Init(&CH1_INT_cfg, CH1_ISR);
    NVIC_EnableIRQ(CH1_INT_cfg.intrSrc);
    Cy_DMA_Channel_SetInterruptMask(DMA_1_HW,DMA_1_DW_CHANNEL,CY_DMA_INTR_MASK); 
    Cy_SysInt_Init(&CH2_INT_cfg, CH2_ISR);
    NVIC_EnableIRQ(CH2_INT_cfg.intrSrc);
    Cy_DMA_Channel_SetInterruptMask(DMA_2_HW,DMA_2_DW_CHANNEL,CY_DMA_INTR_MASK);
    
    /* Initing the NewHaven display and setting the background */
    GUI_Init();
    GUI_SetFont(GUI_FONT_16B_1);
    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();
    SetBackground(SCOPE, WAVE);
    
    /* Starting the ADC */
    ADC_Start();
    ADC_StartConvert();
    
    /* Starting the first DMA to transfer data from channel 1 to the channel 1 arrays */
    DMA_1_Start((uint32_t *)&(SAR->CHAN_RESULT[0]),CH1_Data1);
    Cy_DMA_Descriptor_SetSrcAddress(&DMA_1_Descriptor_2, (uint32_t *)&(SAR->CHAN_RESULT[0]));
    Cy_DMA_Descriptor_SetDstAddress(&DMA_1_Descriptor_2, CH1_Data2);
    
    /* Starting the second DMA to tranfer data from channel 2 to the channel 2 arrays */
    DMA_2_Start((uint32_t *)&(SAR->CHAN_RESULT[2]),CH2_Data1);
    Cy_DMA_Descriptor_SetSrcAddress(&DMA_2_Descriptor_2, (uint32_t *)&(SAR->CHAN_RESULT[2]));
    Cy_DMA_Descriptor_SetDstAddress(&DMA_2_Descriptor_2, CH2_Data2);
    
    uint16_t mainIterations = 0;                                                   // variable for keeping tack of passes through the main loop                                    
    
    /* infinite loop with each tasks */
    for(;;){
        
        GetInput(&SCOPE);                                                          // checking for new user input
        
        if(CH1_FLAG && SCOPE.Running){                                             // when channel 1 finishes transfering data process the data
            CH1_FLAG = FALSE;                                                      // lowering the flag
            Proccess_Channel();
        }
       

        if((ReadyToDraw_ch1 && SCOPE.Running)                                      // checking if we can update the display (ready to draw)
        || (!SCOPE.Running && mainIterations==0x2000)){
            ReadyToDraw_ch1 = FALSE;
            UpdateDisplay();                                                       // updating display
        }

        mainIterations++;                                                          // incrementing the number loops we finished
    }
}
