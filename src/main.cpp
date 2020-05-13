/* ----------------------------------------------------------------------------
 *   Minions-cam: program to run stereo pair on Minions floats
 *   May 12, 2020
 *   Authors: Junsu Jang, FOL/MIT
 *      Description: 
 *   
 *   Minions-cam is intended for Linux based embedded SBC to 
 *   control the stereo camera on Minions floats. 
 *   This firmware has three jobs: 
 *   
 *   1. Trigger images and strobe LEDs accordingly at specified 
 *      framerate
 * 
 *   2. Log relevant sensor data
 * 
 *   3. Save images accordingly once the camera has reached 
 *      below 20m 
 * 
 *   4. Synchronize time with the slave camera
 * 
 *   Jobs 1 and 2 are done by a timer interrupt, which toggles
 *   a flag. Inside the main loop, appropriate GPIO pins are
 *   toggled and sensor data is logged on a CSV file.
 * 
 *   Jobs 2 happens when the images arrive through the USB, images
 *   are saved along with the timestamp.
 * 
 *   Job 4 is processed in the main loop and requires another
 *   timer interrupt with 6 hour long interval. B connects
 *   to the WiFi hosted by A, and A runs a script that ssh
 *   into B and sets the clock on bash
 *   
*/

#include "KellerLD.h"


#define I2C_BUS 0


KellerLD *k_sensor = new KellerLD(I2C_BUS);

/* --
 * Mission details require following information
 *   - Deployment start depth (bar)
 *   - Framerate (fps)
 *   - Time synchronization interval (sec)
 *   - sensor measurement rate (regular) (sec (period))
 *   - Post deployment sensor measurement rate (sec (period))
 */


int main()
{
    // Find out if this is a master or a slave


    // Prior Deployment (on deck)
    //  - Regularly measure depth and temperature until below specified depth
    k_sensor->init();
    

    // During Deployment
    //  - Start taking images for programmed duration

    // Done Data Acquisition
    // Programmed data acquisition duration elapsed
    //  - Regularly measure depth and temperature until powered off.


    return 0;
}