/* ========================================
 *
 * Tiny Scope for the PSoC 6 helper function definitions
 * 
 * Author: Scott Oslund
 *
 * File Synopsis:
 * This file provides a variety of helper functions for
 * the tiny scope. These include functions to calculate
 * the frequency of a signal, draw a waveform on the display,
 * and more
 *
 * ========================================
*/

/* included file */
#include "HelperFunctions.h"


/*
DrawWaveForm:
This function draws the waveform onto the newHaven display It needs the x coordinates of the points in an array,
the y coordinates of points in an array, the size of each array, and the starting location of the waveform (on the y axis)
*/
void DrawWaveForm(int WaveX[X_PIXELS], int WaveY[X_PIXELS], int size, int Start_point)
{
    for(int i=0;i<size-1;i++){                                                           // we iterate through the data in the array
        GUI_DrawLine(WaveX[i],WaveY[i]+Start_point,WaveX[i+1],WaveY[i+1]+Start_point);   // we draw lines between each point in the arrays to form the waveform
    }
}


/*
Copy:
This is a simple function to copy over the data from one array to another. This function is used to store the data
of the wavefrom recently drawn so we can draw over it later. It intakes a source and destination array.
*/
void Copy(int source[],int destination[])
{
    for(int i=0; i<X_PIXELS;i++){
        destination[i] = source[i];             // iterates through all 320 elements and copies them from the source array to the destination
    }
}


/*
Middle:
Helper function for finding the middle point of an array to be used for calculating frequency. This function intakes an array of channel data,
seaches through it for the max and min value, and returns the middle point between the two.
*/
uint16_t Middle(uint16_t arr_1[])
{
    uint16_t max = 0;                          // initing max to the lowest possible value and min to the highest possible value - these will be updated
    uint16_t min = 0xFFFF;

    for(int i=0;i<SIZE;i+=CHECK_FREQ){         // we loop through the array and continually update the max and min if we find a smaller/larger data point
        if(arr_1[i] > max){
            max = arr_1[i];   
        }
        if(arr_1[i] < min){
            min = arr_1[i];
        }
    }
    
    if(max - min < NOISE_THRESHOLD){          // if the difference between data points is too small we assume it is just noise and we return 0 - indicating no significant difference 
        return 0;    
    }
    
    return (max-min)/2 + min;                 // we return the middle value in the array
}


/*
FindTrigger:
This helper function takes in a one of the ping-pong buffers with the data from the ADC and searches the data for the trigger condition
depending on the trigger level and the trigger slope. It returns the index of the array where it found the trigger scaled by 100, or 
if no trigger was found it returns 1. (It is not possible for 1 to be the index where the trigger was found since it starts the search at 3)
*/

uint64_t FindTrigger(uint16_t arr[], SCOPE_SETTINGS SCOPE)
{
    for(int i=NOISE_MARGIN;i<SIZE-NOISE_MARGIN;i++){                                                                // iterating through the channel data
        
        if(arr[i] & UNDERFLOW_CHECK || arr[i+1] & UNDERFLOW_CHECK 
        || arr[i-NOISE_MARGIN]& UNDERFLOW_CHECK || arr[i+NOISE_MARGIN]& UNDERFLOW_CHECK){                          // checking for underflow to prevent unexpected triggers
            i++;
            continue;
        }
        
        if(SCOPE.triggerDir == POSITIVE){                                                                           // our condition depends on the slope setting
            if(arr[i] < SCOPE.triggerLevel && arr[i+1] >= SCOPE.triggerLevel &&                                     // checking condition and filtering noise 
            arr[i-NOISE_MARGIN] < SCOPE.triggerLevel && arr[i+NOISE_MARGIN] >= SCOPE.triggerLevel){
                return i * INDEX_SCALE;                                                                             // returning the index the trigger was found scaled by 100
            }
        }else {                                                                                                     // repeat of the above code for negative slope
            if(arr[i] > SCOPE.triggerLevel && arr[i+1] <= SCOPE.triggerLevel &&                                     // checking condition and filtering noise 
            arr[i-NOISE_MARGIN] > SCOPE.triggerLevel && arr[i+NOISE_MARGIN] <= SCOPE.triggerLevel){
                return i * INDEX_SCALE;
            }  
        }
    }
    return ERROR;                                                                                                   // returns an error if no trigger is found
}




/*
FindFrequency:
This function iterates through a buffer to find the frequency of the signal it contains. It intakes the buffer and the previously calculated
middle value of it. It then searches for two either positive or negative edge crossings and uses the spacing in the buffer to calculate the
frequency of the signal
*/
int FindFrequency(uint16_t arr[], uint16_t middleVal)
{
    
    /* variables */
    volatile uint16_t FirstFind = 0;                                             // holds the index of the first crossing we find
    int Dir = DIRECTION_NOT_SET;                                                 // for storing if we are looking for a positive or negative slope
    
    for(int i=NOISE_MARGIN;i<SIZE-NOISE_MARGIN;i++){                             // iterating through data array
        
        if(arr[i] & UNDERFLOW_CHECK || arr[i+1] & UNDERFLOW_CHECK                // checking for underflow to prevent unexpected results
        || arr[i-NOISE_MARGIN]& UNDERFLOW_CHECK || arr[i+NOISE_MARGIN]& UNDERFLOW_CHECK){                         
            i++;
            continue;
        }
        
        if(arr[i] < middleVal && arr[i+1] >= middleVal &&                        // checking for positive crossing
        arr[i-NOISE_MARGIN] < middleVal && arr[i+NOISE_MARGIN] >= middleVal && Dir != NEGATIVE){      
            if(FirstFind == 0){
                FirstFind = i;                                                   // updating the first index we found and the edge slope we are looking for next time
                Dir = POSITIVE;
            } else {
                uint16_t freq = SAMPLING_RATE/(i-FirstFind);                     // calculating frequency
                if(freq > MAX_FREQ){                                             // if the frequency is within our range (1kHZ max) we return it - otherwise we return 1 for an error
                    return ERROR;
                } else {
                    return freq;                                                
                }
            }
        } else if(arr[i] > middleVal && arr[i+1] <= middleVal &&                 // checking for negative crossing
        arr[i-NOISE_MARGIN] > middleVal && arr[i+NOISE_MARGIN] <= middleVal && Dir != POSITIVE){   
            if(FirstFind == 0){
                FirstFind = i;                                                   // updating the first index we found and the edge slope we are looking for next time
                Dir = NEGATIVE;
            } else {
                uint16_t freq = SAMPLING_RATE/(i-FirstFind);                     // calculating frequency
                if(freq > MAX_FREQ){                                             // if the frequency is within our range (1kHZ max) we return it - otherwise we return 1 for an error
                    return ERROR;
                } else {
                    return freq;    
                }
            }
        }
    }

    return ERROR;                                                                // if we don't find two crossing we return 1 to indicate an error
} 



/*
SetBackground:
This function sets up the background of the NewHaven display by drawing the grid, the frequencies, and 
the x and y scales.
*/
void SetBackground(SCOPE_SETTINGS SCOPE, WAVEFORM_DATA WAVE)
{
    GUI_SetColor(GUI_LIGHTGRAY);                                   // the grid lines are grey and dashed
    GUI_SetLineStyle(GUI_LS_DASH);
    
    for(int i=(PIXELS_PER_X-1); i<X_PIXELS; i+= PIXELS_PER_X){
        GUI_DrawLine(i,0,i,Y_PIXELS);                              // we draw 9 vertical lines evenly spaced stretching accross the screen at intervals 32 pixels
    }

    for(int i=(PIXELS_PER_Y-1);i<Y_PIXELS;i+=PIXELS_PER_Y){
        GUI_DrawLine(0,i,X_PIXELS,i);                              // we draw 9 horizontal lines evenly spaced stretching accross the screen at intervals 30 pixels
    }
    
    GUI_SetColor(GUI_WHITE);                                       // using black for displaying the frequency and scales
    
    char str[STRLEN];
    sprintf(str,"Ch1 Freq: %d HZ    ",WAVE.Freq1);                 // formatting and printing channel 1 frequency
    GUI_DispStringAt(str,MARGIN,MARGIN);
    sprintf(str,"Ch2 Freq: %d HZ    ",WAVE.Freq2);                 // formatting and printing channel 2 frequency
    GUI_DispStringAt(str,MARGIN,LOWER_MARGIN);
    sprintf(str,"Xscale: %d us    ",SCOPE.xScale);                 // printing the xscale
    GUI_DispStringAt(str,RIGHT_MARGIN,MARGIN);
    sprintf(str,"Yscale: %d mV    ",INVERT_YSCALE/SCOPE.yScale);   // printing yscale
    if(INVERT_YSCALE/SCOPE.yScale == YSCALE_1500){
        sprintf(str,"Yscale: 1500 mV    ");                        // we need a special case for when the yscale was set to 1500 mv
    }
    GUI_DispStringAt(str,RIGHT_MARGIN,LOWER_MARGIN);
}


/*
GetInput:
This function constantly checks the UART for incoming commands from the user. It constructs a
string from the data (with whitespace left out) then executes a command depending on the command
sent.
*/
void GetInput(SCOPE_SETTINGS *SCOPE)
{
    
    /* Variables */
    static char str[STRLEN] = "";                                             // string for writing user input to
    char toPrint[STRLEN];                                                     // string to send responses to the user through the UART
    static int index = 0;                                                     // index for acessing part of the string
    
    
    if(UART_GetArray(str+index, sizeof(char))){                               // add input to the string at location specified by the index
        if(*(str+index) != ' ' && *(str+index) != '\t'){
            index++;                                                          // if not whitespace we update the index to the next open slot in the string
        }
     
        if(str[index-1] == '\n' || index >= STRLEN-1){                        // this indicates the end of the command from the user
            str[index] = 0;                                                   // adds NULL terminator to string
            index = 0;                                                        // reseting index to prepare for new user input
            
            /* Long list of checks for each possible command using case-insensitive string comparison 
            In each we compare the contents of the array and the string. Note: the numbers are the 
            lengths of the strings to be compared - these are not hardware dependent so they are not
            magic number */
            if(!strncasecmp(str,"setmodefree",11)){                          
                UART_PutString("Mode set to free-running\n");
                SCOPE->freeRun = TRUE;                                                  // updating mode to free-running
            } else if(!strncasecmp(str,"setmodetrigger",14) && !SCOPE->Running){
                UART_PutString("Mode set to trigger\n");
                SCOPE->freeRun = FALSE;                                                  // updating mode to trigger
            } else if(!strncasecmp(str,"settrigger_slopenegative",24) && !SCOPE->Running){
                UART_PutString("Trigger slope set to negative\n");
                SCOPE->triggerDir = NEGATIVE;                                            // updating slope to negative
            } else if(!strncasecmp(str,"settrigger_slopepositive",24) && !SCOPE->Running){
                UART_PutString("Trigger slope set to positive\n");
                SCOPE->triggerDir = POSITIVE;                                            // updating slope to positive
            } else if(!strncasecmp(str,"settrigger_channel1",19)){
                SCOPE->triggerChannel = CHANNEL_1;                                       // updating trigger channel to channel 1
                UART_PutString("Trigger source set to channel 1\n");
            } else if(!strncasecmp(str,"settrigger_channel2",19)){
                SCOPE->triggerChannel = CHANNEL_2;                                       // updating trigger channel to channel 2
                UART_PutString("Trigger source set to channel 2\n");
            } else if(!strncasecmp(str,"setxscale",9)){
                int xScale = atoi(&str[9]);                                             // set xscale has a number argument - we convert it to an integer
                if(xScale >= MIN_XSCALE && xScale <= MAX_XSCALE){                       // check if the argument is in the range (if invalid argument atoi returns 0)
                    SCOPE->xScale = xScale;                                             // updating the scale and we print the result
                    sprintf(toPrint,"set xscale to %d us\n",xScale);
                    UART_PutString(toPrint);
                } else {
                    UART_PutString("Invalid number to set xScale to\n");
                }
            } else if(!strncasecmp(str,"setyscale",9)){
                int yScale = atoi(&str[9]);                                             // set yscale has a number argument - we convert it to an integer
                if(yScale >= MIN_YSCALE && yScale <=MAX_YSCALE){                        // check if the argument is in the range (if invalid argument atoi returns 0)
                    SCOPE->yScale = INVERT_YSCALE/yScale;                               // updating the scale and we print the result
                    sprintf(toPrint,"set yscale to %d mV\n",yScale);
                    UART_PutString(toPrint);
                } else {
                    UART_PutString("Invalid number to set yScale to\n");
                }
            } else if(!strncasecmp(str,"settrigger_level",16) && !SCOPE->Running){
                int tLevel = atoi(&str[16]);                                            // set trigger level has a number argument - we convert it to an integer
                if(tLevel >= MIN_TRIGGER_LEVEL && tLevel <=MAX_TRIGGER_LEVEL){          // checking if within range
                    SCOPE->triggerLevel = (tLevel * MAX_ADC_OUTPUT) / MAX_VOLTAGE;      // setting trigger level after being scaled to fit scope voltage range
                    sprintf(toPrint,"set trigger level to %d mV\n",tLevel);
                    UART_PutString(toPrint);
                } else {
                    UART_PutString("Invalid number to set trigger level to\n");
                }
            } else if(!strncasecmp(str,"start",5)){
                SCOPE->Running = TRUE;                                                 // starting the scope
                UART_PutString("Started the scope\n");
            } else if(!strncasecmp(str,"stop",4)){
                UART_PutString("Stopped the scope\n");
                SCOPE->Running = FALSE;                                                // stopping the scope
            } else {
                UART_PutString("Error - Invalid input\n");                             // if the string does not match any command we send an error message
            }
        }
    }
}