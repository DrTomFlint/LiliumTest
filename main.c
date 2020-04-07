/** \file main.c
 *  \brief Main function, Tick_10ms interrupt, 2 example functions, and a fault handler.
 *
 * For an actual application we would keep each of these functions in
 * a file of its own.  Since this is just a short exercise, all the
 * functions are lumped into the main.c file.  There should be header files as well.
 *
 * @todo Break up this overly large file into 3 parts.
 * @todo Move the typedefs into a header file.
 *
 * @author Dr Tom Flint
 * @date 7 April 2020
 */


#include <stdio.h>

/** AirData structure to hold data from the Fairchild type 54c airspeed
 *  sensor.  See http://fairchild.com/sensor/type54c.html for description
 * 
 * Error Code Summary:
 * - 0  no error
 * - 1  colder than min temp 
 * - 2  hotter than max temp
 * - 3  loss of power
 * - 4  calibration failed
 *
 */
 
#define AIR_DATA_SCALE 1.00325		//< temperature scaling for air data

/// @todo move the fault codes to a header file, convert to enum
#define FAULT_NONE 0
#define FAULT_OVERRUN 1
#define FAULT_AIRFILTER 2
#define FAULT_COMPENSATE_DIV 3
#define FAULT_COMPENSATE_RANGE 4

 
typedef struct{
	unsigned char errorWord;	///< error Word, 0=ok
	unsigned short tempC;			///< temperature in C
	unsigned long speed;			///< airspeed 
	} AirData_t;						  ///< Structure for airspeed sensor data


/** Global variables */	
AirData_t RawAirData = { 1, 1000, 100000 };		///< Raw data from sensor
int AirDataProcessed = 1;											///< over run check

//----------------------------------------------------------------------
/** \brief interrupt function supported by compiler
 *  
 * Compiler supports an interrupt using second bank of register.  Must
 * take void arguments and return void.  
 * This interrupt is triggered by the 10 mSec timer T1.
 * It can be masked using the macro DI_T1 and unmasked using macro EI_T1
 * If this interrupt is called before the prior set of AirData was processed
 * throw a fault with code FAULT_OVERRUN.
 */
interrupt void Tick_10ms(void){
	
	// Check that prior interrupt was processed, if not we have an
	// overrun condition and should throw a fault
	if(AirDataProcessed!=1){
		Fault(FAULT_OVERRUN);
	}else{
		AirDataProcessed=0;
	}
	
	// Read the new values
	RawAirData.errorWord = ReadPort8(PORT_A);
	RawAirData.tempC = ReadPort16(PORT_A);
	RawAirData.speed = ReadPort32(PORT_A);

}

//----------------------------------------------------------------------
/** \brief Do some math on the raw input data to get a filtered result
*
* This functions reads from the global RawAirData structure, so the
* Tick_10ms interrupt must be disabled during the processing.
*
* Further description of the particular filtering, for this example the
* math is pretty meaningless.
* 
*/
int filterAirData(AirData_t * in, AirData_t * out){

	// Disable the Tick_10ms interrupt
	DI_T1;

	// copy the error word
	out->errorWord = in->errorWord;
	
	// scale the temperature
	out->tempC = in->tempC * AIR_DATA_SCALE;
	
	// copy the speed
	out->speed = in->speed;
		
	// set the processed flag
	AirDataProcessed = 1;
	
	// Re-enable the Tick_10ms interrupt
	EI_T1;
	
	// return an error code or 0 if no errors
	if(out->errorWord!=0){
		return(FAULT_AIRFILTER);
	}else{
		return(0);
	}
	
	// we should never reach this point, but coding standards may require
	// that we add a return that is not conditional.
	return(0);
}

//----------------------------------------------------------------------
/** \brief Handle any faults asserted in the code
*
* This should do an orderly shutdown of the application, log the fault
* with a timestamp, reset interrupts, etc.  The actions are determined
* by the severity of the fault and the application.
*/
void Fault(int faultCode){

	// turn off critical equipment
	// get a timestamp
	// log the fault

}

//----------------------------------------------------------------------
/** \brief Compensate the filtered air speed based on the temperature
*
* This function addresses two reliability concerns that were present in 
* the example code.  A divide by zero condition, and a range problem.
* 
* We will divide the speed by the tempC, placing the result back into 
* tempC.  So a 32 bit unsigned long, divided by a 16 bit unsigned long
* with the result going into a 16 bit unsigned long.  Since the result
* may not fit, we use a temporary variable, and check the range.
*
* This is not a realistic computation, but just to illustrate the idea
*/
int compensateAirData(&in, &out){
	
	unsigned long temp = 0L;					/// temp variable to hold div result
	
	
	// check for divide by zero, 
	if(in->tempC == 0){
		return(FAULT_COMPENSATE_DIV);
	}
	
	// do the division, but place the result in a temp variable
	temp = in->speed / in->tempC;
	// since these are unsigned, we only need to check the upper bound
	if(temp>65535){
		return(FAULT_COMPENSATE_RANGE);
	}
	// If we reach this point, our computation worked, now fill out the
	// output fields.  If the computation failed the output would remain
	// unchanged.
  out->tempC = temp;
  
  // copy the speed and errorWord
  out->speed = in->speed;
	out->errWord = in->errWord;

	return(0);
}

//----------------------------------------------------------------------
/** \brief main function for controller
 *
 * Further description of what is done in main(), any special concerns
 * or unusual methods.  Ideally, main will define the data structures, 
 * initialize them, and call other functions that handle the details.
 *
 */
int main(int argc, char **argv){
		
	AirData_t FilteredAirData	= { 0, 0, 0 };			        /// AirData after filter
	AirData_t	CompensatedAirData = { 1, 1000, 1000000 };  /// Compensated for temperature
	int err=0;																						/// error returned by functions
	int FilterErrorCount = 0;															/// filtering error count
	
	// Continuous processing, loop 1
	while(1){
			
		// ... Imagine other processing here .....
	
		// Filter the raw air data
		err=filterAirData(&RawAirData, &FilteredAirData);
		
		// Only throw a fault if there have been 5 errors in a row
		if(err){
			FilterErrorCount++;
			if(FilterErrorCount>5) Fault(FAULT_AIRFILTER);
		}else{
			FilterErrorCount=0;
		}
			
		// .... more processing code here ........
		
		// Compute the Air Data compensated for temperature
		err=compensateAirData(&FilteredAirData, &CompensatedAirData);
			
		// Throw a fault whenever compensation fails
		if(err){		
			Fault(err);
		}
		
		// ..... communication, data logging, other processing .....
		
		
	} // end of loop 1
	return 0;
}
//----------------------------------------------------------------------
