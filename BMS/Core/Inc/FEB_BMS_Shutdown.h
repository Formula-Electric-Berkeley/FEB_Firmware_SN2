#ifndef INC_FEB_BMS_SHUTDOWN_H_
#define INC_FEB_BMS_SHUTDOWN_H_

// ********************************** Includes **********************************

#include "stm32f4xx_hal.h"
#include <string.h>

#include "FEB_CAN_Charger.h"
#include "FEB_BMS_Precharge.h"

// ********************************** Functions **********************************

void FEB_BMS_Shutdown_Startup(void);	// Close shutdown switch to start car
void FEB_BMS_Shutdown_Initiate(void);	// Initiate shutdown circuit + open AIRs

#endif /* INC_FEB_BMS_SHUTDOWN_H_ */
