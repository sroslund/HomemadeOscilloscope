/* ========================================
 *
 * Tiny Scope for the PSoC 6 header file
 * 
 * Author: Scott Oslund
 *
 * File Synopsis:
 * This file provides the structure definitions,
 * defines, and function prototypes used by the 
 * tiny scope.
 *
 * ========================================
*/

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "project.h"
#include "GUI.h"

/* Defines */
#define NEGATIVE 1                // for keeping track of trigger slope
#define POSITIVE 0                // for trigger slope
#define TRUE 1                    // generally useful define
#define FALSE 0                   // generally useful define
#define SIZE 3200                 // size of the ping pong buffers
#define X_PIXELS 320              // number of x pixels on the display
#define Y_PIXELS 240              // number of y pixels on dislpay
#define INDEX_SCALE 100           // for scaling the index down and up - to prevent floating point operations
#define SAMPLING_RATE 231481      // sampling rate of the ADC
#define MAX_FREQ 1100             // maximum frequency allowed by scope specifications
#define ERROR 1                   // error when the frequency could not be found
#define MAX_ADC_OUTPUT 0x7FF      // this is the max value the adc can return
#define DIRECTION_NOT_SET -1      // used when we have an unknown slope
#define UNDERFLOW_CHECK 0x800     // this macro is used for checking for underflow the 11th bit should not be a 1 or else there was overflow
#define PIXELS_PER_X 32           // macro defining the number of pixels we have in each x-div
#define PIXELS_PER_Y 30           // defines the number of pixels in each y-div
#define MAX_INDEX 320000          // the max index possible after being scaled up by the index scale - used in Process Data functions
#define ADC_SCALE_DOWN 5          // for scaling down the value read from the potentiometer used for moving waveforms up and down
#define STRLEN 50                 // the length of strings 
#define MIN_YSCALE 500            // the minimum value of the yscale that is allowed
#define MAX_YSCALE 2000           // the maximum value of the yscale that is allowed
#define MIN_XSCALE 100            // the minimum value of the xscale that is allowed
#define MAX_XSCALE 10000          // the maximum value of the xscale that is allowed
#define MIN_TRIGGER_LEVEL 100     // the minimum value of the trigger level that is allowed
#define MAX_TRIGGER_LEVEL 3200    // the maximum value of the trigger level that is allowed
#define MAX_VOLTAGE 3300          // the max possible input voltage in millivolts
#define INVERT_YSCALE 1000000     // macro used to invert the Yscale to get the scaling amount 
#define NOISE_THRESHOLD 100       // if the differences between ADC reading is less than this amount we say this is just noise
#define VOLTAGE_INT 330           // a scaled up maximum voltage to prevent the need for floating point operations
#define VOLTAGE_SCALE_DOWN 3200   // macro for scaling down the reading from the ADC to get the voltage
#define INDEX_DIVISOR 128         // macro for scaling down the index when reseting it
#define DEFAULT 1000              // the default value the trigger, xscale, and yscale are set to
#define CHANNEL_1 1               // define for indicating the trigger is set to channel 1
#define CHANNEL_2 2               // define for indicating the trigger is set to channel 2
#define YSCALE_1500 1501          // macro used for checking the yscale value
#define CHECK_FREQ 100            // defines the frequency with which we check for a min/max value
#define NOISE_MARGIN 12           // used for spacing between samples to prevent noise causing false triggers / frequency
#define MARGIN 3                  // margin of spacing between text and edge of the screen
#define RIGHT_MARGIN 200          // margin of spacing between text and right edge of the screen 
#define LOWER_MARGIN 25           // margin of spacing from top to second text (below top text)

/* Defines for timing */
#define READY_TO_START 35
#define FIND_MIDDLE 37
#define FIND_FREQ 38
#define FORMAT_DATA 41

/* Structures for holding data */

typedef struct SCOPE_SETTINGS{    // structure for holding the settings of the tinyscope
    int xScale;                   // microseconds per division (set to 1000 by default)
    int yScale;                   // millivolts per division (set to 1000 by default)
    int freeRun;                  // for keeping track of if we are in freeRun mode(set to TRUE by default)
    int triggerDir;               // for keeping track of the trigger slope / direction (set to positive slope by default)
    int triggerLevel;             // millivolts for trigger to activate (set to 1500 millivolts by default)
    int Running;                  // for keeping track if the scope is running / has been started (set to false by default)
    int triggerChannel;           // for keeping track of which channel the trigger is set to  (set to channel 1 be default)
}SCOPE_SETTINGS;

typedef struct WAVEFORM_DATA{
    int Wave1X[X_PIXELS];         // array for holding channel 1 pixel x coordinates
    int Wave1Y[X_PIXELS];         // array for holding channel 1 pixel y coordinates
    int Prev_Wave1X[X_PIXELS];    // array for holding the x coordinates of the last wave we drew of channel 1 (used for erasing)
    int Prev_Wave1Y[X_PIXELS];    // array for holding the y coordinates of the last wave we drew of channel 1 (used for erasing)
    int Wave1_Buffer1;            // TRUE or FALSE indicating if we are using ping pong buffer one for channel 1
    int Wave2_Buffer1;            // TRUE or FALSE indicating if we are using ping pong buffer one for channel 2
    int Wave2X[X_PIXELS];         // array for holding channel 2 pixel x coordinates
    int Wave2Y[X_PIXELS];         // array for holding channel 2 pixel y coordinates
    int Prev_Wave2X[X_PIXELS];    // array for holding the x coordinates of the last wave we drew of channel 2 (used for erasing)
    int Prev_Wave2Y[X_PIXELS];    // array for holding the y coordinates of the last wave we drew of channel 2 (used for erasing)
    int Freq1;                    // integer for holding the frequency of the channel 1 waveform
    int Freq2;                    // integer for holding the frequency of the channel 2 waveform
    uint16_t Wave1Offset;         // the offset of the wave 1 determined by reading from the poteniometer
    uint16_t Wave2Offset;         // the offset of the wave 2 determined by reading from the poteniometer
}WAVEFORM_DATA;

/* Function prototypes */
void DrawWaveForm(int WaveX[X_PIXELS], int WaveY[X_PIXELS], int size, int Start_point);

void Copy(int source[],int destination[]);

uint16_t Middle(uint16_t arr_1[]);

uint64_t FindTrigger(uint16_t arr[], SCOPE_SETTINGS SCOPE);

void GetInput(SCOPE_SETTINGS *SCOPE);

void SetBackground(SCOPE_SETTINGS SCOPE, WAVEFORM_DATA WAVE);

int FindFrequency(uint16_t arr[], uint16_t middleVal);