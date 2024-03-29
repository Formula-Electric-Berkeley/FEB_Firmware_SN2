// ********************************** Includes & External **********************************

#include "FEB_CAN.h"

extern CAN_HandleTypeDef hcan1;

// ********************************** CAN Configuration **********************************

// static CAN_TxHeaderTypeDef FEB_CAN_TxHeader;
static CAN_RxHeaderTypeDef FEB_CAN_RxHeader;
static uint8_t FEB_CAN_FIFO_Assignment = CAN_RX_FIFO0;
static uint32_t FEB_CAN_FIFO_Interrupt = CAN_IT_RX_FIFO0_MSG_PENDING;

uint8_t FEB_CAN_TxData[8];
uint8_t FEB_CAN_RxData[8];
uint32_t FEB_CAN_TxMailbox;

// ******************** CAN ********************

void FEB_CAN_Init() {
	FEB_CAN_Filter_Config();
	if (HAL_CAN_Start(&hcan1) != HAL_OK) {
		FEB_BMS_Shutdown_Initiate();
	}
	HAL_CAN_ActivateNotification(&hcan1, FEB_CAN_FIFO_Interrupt);
}

void FEB_CAN_Filter_Config() {
	uint8_t bank = 0;
	bank = FEB_CAN_IVT_Filter_Config(&hcan1, FEB_CAN_FIFO_Assignment, bank);
	// bank = FEB_CAN_Charger_Filter_Config(&hcan1, FIFO_Assignment, bank);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (HAL_CAN_GetRxMessage(hcan, FEB_CAN_FIFO_Assignment, &FEB_CAN_RxHeader, FEB_CAN_RxData) == HAL_OK) {
		FEB_CAN_IVT_Store_Msg(&FEB_CAN_RxHeader, FEB_CAN_RxData);
	}
}
