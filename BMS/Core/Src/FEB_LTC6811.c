// ********************************** Includes & External **********************************

#include "FEB_LTC6811.h"

extern UART_HandleTypeDef huart2;

// ********************************** LTC6811 Configuration **********************************

// Set configuration bits
static bool REFON = 1; 												//!< Reference Powered Up Bit
static bool ADCOPT = 0; 											//!< ADC Mode option bit
static bool GPIOBITS_A[5] = {1, 1, 0, 0, 0}; 						//!< GPIO Pin Control - (1, 2) ADC - (3, 4, 5) MUX Select
static uint16_t UV = UV_THRESHOLD; 									//!< Under-voltage Comparison Voltage
static uint16_t OV = OV_THRESHOLD; 									//!< Over-voltage Comparison Voltage
static bool DCCBITS_A[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 	//!< Discharge cell switch 	//Dcc 1,2,3,4,5,6,7,8,9,10,11,12
static bool DCTOBITS[4] = {1, 0, 1, 0}; 							//!< Discharge time value 	// Dcto 0,1,2,3 // Programmed for 4 min

// Set accumulator
static Accumulator accumulator;

// ********************************** Functions **********************************

void FEB_LTC6811_Setup(void) {
	for (int i = 0; i < CELLS_PER_DAUGHTER_BOARD; i++) {
		DCCBITS_A[i] = 0;
	}

	// Set cell balancing
	if (FEB_CAN_CHARGER_CHARGE_BOOL == 1) {
		FEB_LTC6811_Balance_Cells();
	}

	// Setup Board
	LTC6811_init_cfg(NUM_IC, accumulator.IC_config);

	for (uint8_t current_ic = 0; current_ic < NUM_IC; current_ic++) {
		LTC6811_set_cfgr(current_ic, accumulator.IC_config, REFON, ADCOPT, GPIOBITS_A, DCCBITS_A, DCTOBITS, UV, OV);
	}

	LTC6811_reset_crc_count(NUM_IC, accumulator.IC_config);
	LTC6811_init_reg_limits(NUM_IC, accumulator.IC_config);
}

// ******************** Read Voltage ********************

void FEB_LTC6811_Poll_Voltage(void) {
	FEB_LTC6811_Start_Cell_ADC_Measurement();
	FEB_LTC6811_Read_Cell_Voltage_Registers();
	FEB_LTC6811_Store_Voltage();
}

void FEB_LTC6811_Start_Cell_ADC_Measurement(void) {
	wakeup_sleep(NUM_IC);
	LTC6811_adcv(ADC_CONVERSION_MODE, ADC_DCP, CELL_CH_TO_CONVERT);
	LTC6811_pollAdc();
}

void FEB_LTC6811_Read_Cell_Voltage_Registers(void) {
	wakeup_sleep(NUM_IC);
	LTC6811_rdcv(SEL_ALL_REG, NUM_IC, accumulator.IC_config);
}

void FEB_LTC6811_Store_Voltage(void) {
    for (uint16_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
    	accumulator.banks[bank_idx].cells[16].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[0]);
    	accumulator.banks[bank_idx].cells[15].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[1]);
    	accumulator.banks[bank_idx].cells[14].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[2]);
    	accumulator.banks[bank_idx].cells[13].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[3]);
    	accumulator.banks[bank_idx].cells[12].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[6]);
    	accumulator.banks[bank_idx].cells[11].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[7]);
    	accumulator.banks[bank_idx].cells[10].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[8]);
    	accumulator.banks[bank_idx].cells[9].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2].cells.c_codes[9]);

    	accumulator.banks[bank_idx].cells[8].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[0]);
    	accumulator.banks[bank_idx].cells[7].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[1]);
    	accumulator.banks[bank_idx].cells[6].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[2]);
    	accumulator.banks[bank_idx].cells[5].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[3]);
    	accumulator.banks[bank_idx].cells[4].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[4]);
      	accumulator.banks[bank_idx].cells[3].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[6]);
      	accumulator.banks[bank_idx].cells[2].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[7]);
      	accumulator.banks[bank_idx].cells[1].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[8]);
      	accumulator.banks[bank_idx].cells[0].voltage = FEB_LTC6811_Convert_Voltage(accumulator.IC_config[bank_idx * 2 + 1].cells.c_codes[9]);
    }
}

/*
 * @param value := Voltage (100uV) from LTC6811
 */
float FEB_LTC6811_Convert_Voltage(uint16_t value) {
	// Check for error: value = 2^16 - 1
	if (value == 65535) {
		return -42;
	}
	return value * 0.0001;
}

// ******************** Voltage Cell Balance ********************
void FEB_LTC6811_Balance_Cells(void) {
    for (uint8_t s_pin_read; s_pin_read < CELLS_PER_DAUGHTER_BOARD; s_pin_read++) {
    	wakeup_sleep(NUM_IC);
    	LTC6811_set_discharge(s_pin_read, NUM_IC, accumulator.IC_config);
    	LTC6811_wrcfg(NUM_IC, accumulator.IC_config);
    	wakeup_idle(NUM_IC);
    	LTC6811_rdcfg(NUM_IC, accumulator.IC_config);
    }
}

void FEB_LTC6811_Clear_Balance_Cells(void) {
    wakeup_sleep(NUM_IC);
    LTC6811_clear_discharge(NUM_IC, accumulator.IC_config);
    LTC6811_wrcfg(NUM_IC, accumulator.IC_config);
    wakeup_idle(NUM_IC);
    LTC6811_rdcfg(NUM_IC, accumulator.IC_config);
}

// ******************** Voltage Interface ********************
void FEB_LTC6811_Validate_Voltage(void) {
	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		for (uint8_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			float voltage = accumulator.banks[bank_idx].cells[cell_idx].voltage;
			if (voltage < MIN_VOLTAGE || voltage > MAX_VOLTAGE) {
				FEB_BMS_Shutdown_Initiate();
			}
		}
	}
}

void FEB_LTC6811_UART_Transmit_Voltage() {
	char UART_str[1024];
	char temp_str[256];

	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		// Add bank_idx, cell_idx to {@code UART_Str}
		sprintf(UART_str, "%d", (bank_idx << UART_BITS_PER_MESSAGE) + UART_VOLTAGE_ID);


		// Add values to {@code UART_Str}
		for (uint16_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			float voltage = accumulator.banks[bank_idx].cells[cell_idx].voltage;

			sprintf(temp_str, " %f", voltage);
			strncat(UART_str, temp_str, strlen(temp_str));
		}

		// Add '\n' to {@code UART_Str}
		sprintf(temp_str, "\n");
		strncat(UART_str, temp_str, strlen(temp_str));

		HAL_UART_Transmit(&huart2, (uint8_t*) UART_str, strlen(UART_str), 100);
	}
}

float FEB_LTC6811_Total_Bank_Voltage(void) {
	float total_voltage = 0;

	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		for (uint8_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			total_voltage += accumulator.banks[bank_idx].cells[cell_idx].voltage;
		}
	}

	return total_voltage;
}

uint8_t FEB_LTC6811_Cells_Charged(void) {
	const float cell_voltage_threshold = MAX_VOLTAGE * CHARGED_PERCENTAGE;

	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		for (uint8_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			float cell_voltage = accumulator.banks[bank_idx].cells[cell_idx].voltage;
			if (cell_voltage < cell_voltage_threshold) {
				return 0;
			}
		}
	}
	return 1;
}

void FEB_LTC6811_Clear_Voltage(void) {
	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		for (uint8_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			accumulator.banks[bank_idx].cells[cell_idx].voltage = 0;
		}
	}
}

// ******************** Read Temperature ********************

void FEB_LTC6811_Poll_Temperature(void) {
	for (uint8_t channel = 0; channel <= 4; channel++) {
		FEB_LTC6811_Update_GPIO(channel);
		FEB_LTC6811_Start_GPIO_ADC_Measurement();
		FEB_LTC6811_Read_Aux_Voltages();
		FEB_LTC6811_Store_Temperature(channel);
	}
}

void FEB_LTC6811_Update_GPIO(uint8_t channel) {
	GPIOBITS_A[0] = 0b1;					// ADC
	GPIOBITS_A[1] = 0b1;					// ADC
	GPIOBITS_A[2] = (channel >> 0) & 0b1;	// MUX Select
	GPIOBITS_A[3] = (channel >> 1) & 0b1;	// MUX Select
	GPIOBITS_A[4] = (channel >> 2) & 0b1;	// MUX Select

    wakeup_sleep(NUM_IC);
    for (uint8_t current_ic = 0; current_ic < NUM_IC;current_ic++) {
      LTC6811_set_cfgr(current_ic, accumulator.IC_config, REFON, ADCOPT, GPIOBITS_A, DCCBITS_A, DCTOBITS, UV, OV);
    }
    wakeup_idle(NUM_IC);
    LTC6811_wrcfg(NUM_IC, accumulator.IC_config);
}

void FEB_LTC6811_Start_GPIO_ADC_Measurement(void) {
	wakeup_sleep(NUM_IC);
	LTC6811_adax(ADC_CONVERSION_MODE, AUX_CH_TO_CONVERT);
	LTC6811_pollAdc();
}

void FEB_LTC6811_Read_Aux_Voltages(void) {
	wakeup_sleep(NUM_IC);
	LTC6811_rdaux(SEL_ALL_REG, NUM_IC, accumulator.IC_config);
}

void FEB_LTC6811_Store_Temperature(uint8_t channel) {
    for (uint16_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
    	switch (channel) {
    		case 0:
    			accumulator.banks[bank_idx].cells[12].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[16].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[1]);
    			accumulator.banks[bank_idx].cells[4].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[8].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[1]);
    			break;
    		case 1:
    			accumulator.banks[bank_idx].cells[11].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[15].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[1]);
    			accumulator.banks[bank_idx].cells[3].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[7].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[1]);
    			break;
    		case 2:
    			accumulator.banks[bank_idx].cells[10].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[14].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[1]);
    			accumulator.banks[bank_idx].cells[2].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[6].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[1]);
    			break;
    		case 3:
    			accumulator.banks[bank_idx].cells[9].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[13].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2].aux.a_codes[1]);
    			accumulator.banks[bank_idx].cells[1].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[0]);
    			accumulator.banks[bank_idx].cells[5].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[1]);
    			break;
    		case 4:
    			accumulator.banks[bank_idx].cells[0].temperature = FEB_LTC6811_Convert_Temperature(accumulator.IC_config[bank_idx * 2 + 1].aux.a_codes[0]);
    			break;
    	}
    }
}

/*
 * @param value := Voltage (100uV) from LTC6811
 */
float FEB_LTC6811_Convert_Temperature(uint16_t value) {
	// Check for error: value = 2^16 - 1
	if (value == 65535) {
		return -42;
	}
	//return value * 0.0001;
	return FEB_LTC6811_Temp_LUT_Get_Temperature(value * 0.0001);
}

// ******************** Temperature Interface ********************

void FEB_LTC6811_Validate_Temperature(void) {
	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		for (uint8_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			float temperature = accumulator.banks[bank_idx].cells[cell_idx].temperature;
			if (temperature < MIN_OPERATION_TEMPERATURE || temperature > MAX_OPERATION_TEMPERATURE) {
				FEB_BMS_Shutdown_Initiate();
			}
		}
	}
}

void FEB_LTC6811_UART_Transmit_Temperature() {
	char UART_Str[1024];
	char temp_str[256];

	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		// Add bank_idx, cell_idx to {@code UART_Str}
		sprintf(UART_Str, "%d", (bank_idx << UART_BITS_PER_MESSAGE) + UART_TEMPERATURE_ID);


		// Add values to {@code UART_Str}
		for (uint16_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			float temperature = accumulator.banks[bank_idx].cells[cell_idx].temperature;

			sprintf(temp_str, " %f", temperature);
			strncat(UART_Str, temp_str, strlen(temp_str));
		}

		// Add '\n' to {@code UART_Str}
		sprintf(temp_str, "\n");
		strncat(UART_Str, temp_str, strlen(temp_str));
		HAL_UART_Transmit(&huart2, (uint8_t*) UART_Str, strlen(UART_Str), 100);
	}
}

void FEB_LTC6811_Clear_Temperature(void) {
	for (uint8_t bank_idx = 0; bank_idx < NUM_BANKS; bank_idx++) {
		for (uint8_t cell_idx = 0; cell_idx < CELLS_PER_BANK; cell_idx++) {
			accumulator.banks[bank_idx].cells[cell_idx].temperature = 0;
		}
	}
}
