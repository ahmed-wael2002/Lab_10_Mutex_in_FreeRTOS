// FreeRTOS Libraries
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
// External Modules 
#include "led.h"
#include "uart.h"
#include "int.h"
#include "DIO.h"

/***************************************************************************/
/*								MACROS			 						   */
/***************************************************************************/
#define SWITCH_PORT 	    PORTF_ID
#define SWITCH_PIN		    PIN0_ID
#define KEY_PRESSED		    LOGIC_LOW
#define IS_ON(port,pin)	    (DIO_readPin(port,pin) == KEY_PRESSED)
#define UART_MODULE 	    UART_0

/***************************************************************************/
/*								TASK PROTOTYPES 						   */
/***************************************************************************/
void vInitTask( void *pvParameters);
void vLedTogglerTask(void *pvParameters);
void vCounterTask (void *pvParameters);
void vPrintString(char * buffer);


/***************************************************************************/
/*								GLOBAL VARIABLES 						   */
/***************************************************************************/
xSemaphoreHandle xBinarySemaphore;
xSemaphoreHandle xMutex;
uint8 counter = 0;


/***************************************************************************/
/*								MAIN FUNCTION	 						   */
/***************************************************************************/
int main( void )
{
	// Semaphore Initialization
	vSemaphoreCreateBinary(xBinarySemaphore);
	// Mutex Initialization
	xMutex = xSemaphoreCreateMutex();


	if( xBinarySemaphore != NULL )
	{	
		if ( xMutex != NULL) 
		{
			xTaskCreate( vInitTask, "Init Task", 240, NULL, 3, NULL );
			xTaskCreate( vLedTogglerTask, "Led Toggler Task", 240, NULL, 2, NULL );
			xTaskCreate( vCounterTask, "Counter Task", 240, NULL, 1, NULL );
			vTaskStartScheduler();
		}
	}
	/* If all is well we will never reach here as the scheduler will now be
	running the tasks. If we do reach here then it is likely that there was
	insufficient heap memory available for a resource to be created. */
	for( ;; );
}

/***************************************************************************/
/*								TASK DEFINITIONS 						   */
/***************************************************************************/

// Semaphore Task
void vLedTogglerTask(void *pvParameters){
	xSemaphoreTake(xBinarySemaphore,0);
	for(;;)
	{
		xSemaphoreTake(xBinarySemaphore,portMAX_DELAY);
		//use xSemaphoreTake function with max delay value, i.e. this task will be blocked until
		// it takes the semaphore from ISR
		//Toggle the LED
		xSemaphoreTake(xMutex,portMAX_DELAY);
		{
			UART_sendString(UART_MODULE, "This is LedToggler Task\r\n");
			LED_toggle(BLUE);
			vTaskDelay(100/portTICK_RATE_MS);
		}
		xSemaphoreGive(xMutex);
	}
}

void vCounterTask (void *pvParameters){
	for(;;)
	{
        //vPrintString("This is the Counter Task\n");
		xSemaphoreTake(xMutex,portMAX_DELAY);
		{
            UART_sendString(UART_MODULE, "This is the Counter Task\r\n" );
			for (counter=0; counter<=10; counter++){
				UART_sendInteger(UART_MODULE, counter);
                UART_sendString(UART_MODULE, "\r\n");
				// Simple software delay loop
				for (int i=0; i<1000000; i++);
			}
		}
		xSemaphoreGive(xMutex);
	}
}


void Button_Interrupt_Handler(void){
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( xBinarySemaphore, &xHigherPriorityTaskWoken );
	INT_clearInterrupt(SWITCH_PORT, SWITCH_PIN);
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}


void vInitTask( void *pvParameters){
	// Initializing UART0 using UART module 
	UART_ConfigType uart_configs = {UART_0,9600,DISABLED,ONE_BIT,BIT_8};
	UART_init(&uart_configs);
	// Initializing Push Button 1 in board - PF0
	DIO_Init(SWITCH_PORT, SWITCH_PIN, PIN_INPUT);
	// Initializing Interrupt for Push Button 1 & Function
	INT_init(SWITCH_PORT, SWITCH_PIN);
	INT_setCallBack(SWITCH_PORT, Button_Interrupt_Handler);
	// Initializing LED 
	LED_init();
    // Task Ended
    vTaskDelete(NULL);
}

