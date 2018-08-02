/*
 * platform.c
 *
 *  Created on: Jul 30, 2018
 *      Author: simon
 */

// =============== Includes ============================================================================================================
#include "platformAdapter.h"
#include "platformAPI.h"
#include "platformModules.h"
#include "platformAPIConfig.h"

// =============== Defines =============================================================================================================
#define MAX_PWM_DUTYCYCLE 1800
#define UART_NR_BYTES 10

// =============== Typdefs =============================================================================================================

// =============== Variables ===========================================================================================================
volatile uint8_t flag_hallA_ADC_isRunning = 0;
volatile uint8_t flag_hallB_ADC_isRunning = 0;

static uint8_t uartBufferA[UART_NR_BYTES];
static uint8_t uartBufferB[UART_NR_BYTES];
static uint8_t *pActiveUartBuffer;
static uint8_t *pFirstFreeBufferSpace;

// =============== Function pointers ===================================================================================================

// =============== Function declarations ===============================================================================================

// =============== Functions ===========================================================================================================
// --------------- platformAPI.h -------------------------------------------------------------------------------------------------------
void entryNonInterruptableSection() {

}
void leaveNonInterruptableSection() {

}

// pwm
#ifdef PWM
void setPWMDutyCycle(uint8_t dutyCycle) {
	uint32_t tempDutyCycle = (dutyCycle*MAX_PWM_DUTYCYCLE); // 255 * 1800 = 0b111 00000000 11111000 --> 19bit, no overflow
	tempDutyCycle = tempDutyCycle / 255;

	__HAL_TIM_SET_COMPARE(pPWM_handle_A, PWM_A_HS_channel, tempDutyCycle);
	__HAL_TIM_SET_COMPARE(pPWM_handle_A, PWM_A_LS_channel, tempDutyCycle);
	__HAL_TIM_SET_COMPARE(pPWM_handle_B_and_C, PWM_B_HS_channel, tempDutyCycle);
	__HAL_TIM_SET_COMPARE(pPWM_handle_B_and_C, PWM_B_LS_channel, tempDutyCycle);
	__HAL_TIM_SET_COMPARE(pPWM_handle_B_and_C, PWM_C_HS_channel, tempDutyCycle);
	__HAL_TIM_SET_COMPARE(pPWM_handle_B_and_C, PWM_C_LS_channel, tempDutyCycle);
}

void enablePWM(Phase phase, Bridgeside side) {
	switch (phase) {
	case phase_A:
		if (side == bridgeside_highside) {
			HAL_TIM_PWM_Start(pPWM_handle_A, PWM_A_HS_channel);
		} else {
			HAL_TIM_PWM_Start(pPWM_handle_A, PWM_A_LS_channel);
		}
		break;

	case phase_B:
		if (side == bridgeside_highside) {
			HAL_TIM_PWM_Start(pPWM_handle_B_and_C, PWM_B_HS_channel);
		} else {
			HAL_TIM_PWM_Start(pPWM_handle_B_and_C, PWM_B_LS_channel);
		}
		break;

	case phase_C:
		if (side == bridgeside_highside) {
			HAL_TIM_PWM_Start(pPWM_handle_B_and_C, PWM_C_HS_channel);
		} else {
			HAL_TIM_PWM_Start(pPWM_handle_B_and_C, PWM_C_LS_channel);
		}
		break;
	}
}
void disablePWM(Phase phase, Bridgeside side) {
	switch (phase) {
		case phase_A:
			if (side == bridgeside_highside) {
				HAL_TIM_PWM_Stop(pPWM_handle_A, PWM_A_HS_channel);
			} else {
				HAL_TIM_PWM_Stop(pPWM_handle_A, PWM_A_LS_channel);
			}
			break;

		case phase_B:
			if (side == bridgeside_highside) {
				HAL_TIM_PWM_Stop(pPWM_handle_B_and_C, PWM_B_HS_channel);
			} else {
				HAL_TIM_PWM_Stop(pPWM_handle_B_and_C, PWM_B_LS_channel);
			}
			break;

		case phase_C:
			if (side == bridgeside_highside) {
				HAL_TIM_PWM_Stop(pPWM_handle_B_and_C, PWM_C_HS_channel);
			} else {
				HAL_TIM_PWM_Stop(pPWM_handle_B_and_C, PWM_C_LS_channel);
			}
			break;
		}
}
#endif /* PWM */

// comperators
#ifdef COMPERATORS
void enableComperator(Phase phase, Edge edge, Boolean enable) {
	/*ToDo: edge berücksichtigen (umkonfigurieren)*/
	if (enable == boolean_true) {
		switch (phase) {
		case phase_A:
			HAL_NVIC_EnableIRQ(IR_COMP_A_EXTI_IRQn);
			break;
		case phase_B:
			HAL_NVIC_EnableIRQ(IR_COMP_B_EXTI_IRQn);
			break;
		case phase_C:
			HAL_NVIC_EnableIRQ(IR_COMP_C_EXTI_IRQn);
			break;
		}

	} else {
		switch (phase) {
		case phase_A:
			HAL_NVIC_DisableIRQ(IR_COMP_A_EXTI_IRQn);
			break;
		case phase_B:
			HAL_NVIC_DisableIRQ(IR_COMP_A_EXTI_IRQn);
			break;
		case phase_C:
			HAL_NVIC_DisableIRQ(IR_COMP_A_EXTI_IRQn);
			break;
		}
	}
}
Boolean isComperatorSignalHigh(Phase phase) {
	switch (phase) {
	case phase_A:
		return HAL_GPIO_ReadPin(IR_COMP_A_GPIO_Port, IR_COMP_A_Pin);
	case phase_B:
		return HAL_GPIO_ReadPin(IR_COMP_B_GPIO_Port, IR_COMP_B_Pin);
	case phase_C:
		return HAL_GPIO_ReadPin(IR_COMP_C_GPIO_Port, IR_COMP_C_Pin);
	default:
		platformError("Invalid phase.", __FILE__, __LINE__);
		return 0;
	}
}
#endif /* COMPERATORS */

// gpio's
Boolean isBoardEnabled() {
	return HAL_GPIO_ReadPin(DI_ENABLE_PRINT_GPIO_Port, DI_ENABLE_PRINT_Pin);
}
Boolean isNFaultFromBridgeDriver() {
	return HAL_GPIO_ReadPin(DI_DRIVER_NFAULT_GPIO_Port, DI_DRIVER_NFAULT_Pin);
}
Boolean isNOCTWFromBridgeDriver() {
	return HAL_GPIO_ReadPin(DI_DRIVER_NOCTW_GPIO_Port, DI_DRIVER_NOCTW_Pin);
}
Boolean isPWRGDFromBridgeDriver() {
	return HAL_GPIO_ReadPin(DI_DRIVER_PWRGD_GPIO_Port, DI_DRIVER_PWRGD_Pin);
}
Boolean isEncoderEnabled() {
	return HAL_GPIO_ReadPin(DI_INC_ENABLE_GPIO_Port, DI_INC_ENABLE_Pin);
}
Boolean isCalibrateEncoder() {
	return HAL_GPIO_ReadPin(DI_INC_CALIBRATE_GPIO_Port, DI_INC_CALIBRATE_Pin);
}

ControlSignalType getControlSignalType() {
	uint8_t temp = HAL_GPIO_ReadPin(DI_USER_IN_GPIO_Port, DI_USER_IN_Pin);

	if(temp){
		return controlSignalType_positive_torque;
	}
	return controlSignalType_negative_torque;
}

void setLED(Boolean ledON) {
	HAL_GPIO_WritePin(DO_LED_1_GPIO_Port, DO_LED_1_Pin, ledON);
}
void setEnableBridgeDriver(Boolean enable) {
	HAL_GPIO_WritePin(DO_DRIVER_EN_GPIO_Port, DO_DRIVER_EN_Pin, enable);
}
void setDCCalBridgeDriver(Boolean dcCal) {
	HAL_GPIO_WritePin(DO_DRIVER_DC_CAL_GPIO_Port, DO_DRIVER_DC_CAL_Pin, dcCal);
}

// uart
void sendUartData(uint8_t *pData, uint8_t size) {
	HAL_UART_Transmit_IT(pUART_handle, pData, size);
	//HAL_UART_Transmit(pUART_handle, &data, 1, 1);
}

// encoder
#ifdef ENCODER
void enableEncoder(Boolean enable) {

}
uint32_t getRotadedDegreesEncoder() {

}
void setRotadedDegreesEncoder(uint32_t rotadedDeg) {

}
void resetRotadedDegreesEncoder() {

}

Boolean isEncoderSignalA() {

}
Boolean isEncoderSignalB() {

}

void setEncoderCalibrationReferencePosition(Boolean state) {

}
#endif /* ENCODER */
// --------------- platformAdapter.h-------- -------------------------------------------------------------------------------------------
void startupPlatform() {
#ifdef SYSTIME
	initSystime();
#endif /* SYSTIME */
	// start PWM timers
	HAL_TIM_Base_Start(pPWM_handle_A);
	HAL_TIM_Base_Start(pPWM_handle_B_and_C);

	// start receive over UART
	pActiveUartBuffer = uartBufferA;
	HAL_UART_Receive_IT(pUART_handle, pActiveUartBuffer, UART_NR_BYTES);

	startup();
}
void proceedPlatform() {
	// handle UART
	/*for(uint8_t cnt = 0; cnt < 100; cnt++){
		uint8_t buffer = 0;
		HAL_UART_Receive(pUART_handle, &buffer, 1, 1);

		if(buffer != 0){
			event_uartDataReceived(buffer);
		}
	}*/
	proceed();
}

#ifdef COMPERATORS
void extIRQ_compPhaseA() {
	Edge tempEdge = edge_falling;
	if (isComperatorSignalHigh(phase_A) == boolean_true) {
		tempEdge = edge_rising;
	}

	event_comperatorSignalChanged(phase_A, tempEdge);
}
void extIRQ_compPhaseB() {
	Edge tempEdge = edge_falling;
	if (isComperatorSignalHigh(phase_B) == boolean_true) {
		tempEdge = edge_rising;
	}

	event_comperatorSignalChanged(phase_B, tempEdge);
}
void extIRQ_compPhaseC() {
	Edge tempEdge = edge_falling;
	if (isComperatorSignalHigh(phase_C) == boolean_true) {
		tempEdge = edge_rising;
	}

	event_comperatorSignalChanged(phase_C, tempEdge);
}
#endif /* COMPERATORS */

#ifdef ENCODER
void extIRQ_encoderInReferencePos() {

}

void timerIRQ_encoderPulses() {

}
#endif /* ENCODER */

void uartIRQ_dataSendet(){

}
void uartIRQ_dataReceived(){

	event_uartDataReceived(uartBufferA[0]);
}
