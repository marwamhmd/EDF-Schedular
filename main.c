/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*---------TASK Macros---------------------------------------*/
#define Btn1MonitorSTACK_SIZE      configMINIMAL_STACK_SIZE
#define Btn2MonitorSTACK_SIZE      configMINIMAL_STACK_SIZE
#define PeriodicTxSTACK_SIZE       configMINIMAL_STACK_SIZE
#define UARTReceiverSTACK_SIZE     configMINIMAL_STACK_SIZE
#define Load1SimSTACK_SIZE         configMINIMAL_STACK_SIZE
#define Load2SimSTACK_SIZE         configMINIMAL_STACK_SIZE

#define Btn1MonitorTaskPERIOD        ((TickType_t) 50)
#define Btn2MonitorTaskPERIOD        ((TickType_t) 50)
#define PeriodicTxTaskPERIOD         ((TickType_t) 100)
#define UARTReceiverTaskPERIOD       ((TickType_t) 20)
#define Load1SimTaskPERIOD           ((TickType_t) 10)
#define Load2SimTaskPERIOD           ((TickType_t) 100)

#define Btn1MonitorPRIORITY           1
#define Btn2MonitorPRIORITY           1
#define PeriodicTxPRIORITY            2
#define UARTReceiverPRIORITY          2
#define Load1SimPRIORITY              3
#define Load2SimPRIORITY              3


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
/**********************************************************************************************************************
* LOCAL FUNCTION PROTOTYPES
**********************************************************************************************************************/
void vButton1MonitorTask(void *pvParameters );
void vButton2MonitorTask(void *pvParameters);
void vPeriodicTransmitterTask(void *pvParameters);
void vUARTRecTask(void *pvParameters);
void vLoad1SimTask(void *pvParameters);
void vLoad2SimTask(void *pvParameters);
static void vCreateEdfTestTask(void);
void ToggleTest(void *pvParameters);
/**********************************************************************************************************************/
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
/****************************Queue Info********************************************************************************/
QueueHandle_t xQueue;
#define QUEUE_SIZE   10 
#define DATA_SIZE    10
typedef enum
{
	  csButton1StateCmd,
    csButton2StateCmd,	
	  csPeriodicStringCmd,
}enumMessageType;
typedef struct 
{
    enumMessageType objenumMessageType;
	  char            Data[DATA_SIZE];
}SystemMessage;
/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
  /* Create Tasks here */
  vCreateEdfTestTask();
  /* Create Tasks here */
	xQueue = xQueueCreate(QUEUE_SIZE, sizeof(SystemMessage));
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*------------------Task Functions-----------------------------*/
/* START FUNCTION DESCRIPTION ******************************************************************************************
vCreateEdfTestTask()                                           <main.C>

SYNTAX:         vCreateEdfTestTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
static void vCreateEdfTestTask(void)
{
	/*xTaskPeriodicCreate( ToggleTest, "Toggle", Btn1MonitorSTACK_SIZE, NULL, \
	                     Btn1MonitorPRIORITY,Btn1MonitorTaskPERIOD,( TaskHandle_t * ) NULL );
*/
	
  #if( ConfigUSE_EDF_SCHEDULAR == 1)
	xTaskPeriodicCreate(vButton1MonitorTask, "Button_1_Monitor", Btn1MonitorSTACK_SIZE, NULL, \
	                     Btn1MonitorPRIORITY,Btn1MonitorTaskPERIOD,( TaskHandle_t * ) NULL );
	#else
	xTaskCreate( vButton1MonitorTask, "Button_1_Monitor", Btn1MonitorSTACK_SIZE, NULL, \
	                     Btn1MonitorPRIORITY,( TaskHandle_t * ) NULL );
	#endif

	#if( ConfigUSE_EDF_SCHEDULAR == 1)
	xTaskPeriodicCreate(vButton2MonitorTask, "Button_2_Monitor", Btn2MonitorSTACK_SIZE, NULL, \
	                     Btn2MonitorPRIORITY,Btn2MonitorTaskPERIOD,( TaskHandle_t * ) NULL );
	#else
	xTaskCreate( vButton2MonitorTask, "Button_2_Monitor", Btn2MonitorSTACK_SIZE, NULL, \
	                     Btn2MonitorPRIORITY,( TaskHandle_t * ) NULL );
	#endif

	#if( ConfigUSE_EDF_SCHEDULAR == 1)
	xTaskPeriodicCreate(vPeriodicTransmitterTask, "Periodic Transmitter", PeriodicTxSTACK_SIZE, NULL, \
	                     PeriodicTxPRIORITY,PeriodicTxTaskPERIOD,( TaskHandle_t * ) NULL );
	#else
	xTaskCreate( vPeriodicTransmitterTask, "Periodic Transmitter", PeriodicTxSTACK_SIZE, NULL, \
	                     PeriodicTxPRIORITY,( TaskHandle_t * ) NULL );
	#endif

#if( ConfigUSE_EDF_SCHEDULAR == 1)
	xTaskPeriodicCreate(vUARTRecTask, "UART Receiver", UARTReceiverSTACK_SIZE, NULL, \
	                     UARTReceiverPRIORITY,UARTReceiverTaskPERIOD,( TaskHandle_t * ) NULL );
	#else
	xTaskCreate( vUARTRecTask, "UART Receiver", UARTReceiverSTACK_SIZE, NULL, \
	                     UARTReceiverPRIORITY,( TaskHandle_t * ) NULL );
	#endif

  #if( ConfigUSE_EDF_SCHEDULAR == 1)
	xTaskPeriodicCreate(vLoad1SimTask, "Load1Simulator", Load1SimSTACK_SIZE, NULL, \
	                     Load1SimPRIORITY,Load1SimTaskPERIOD,( TaskHandle_t * ) NULL );
	#else
	xTaskCreate(vLoad1SimTask, "Load1Simulator", Load1SimSTACK_SIZE, NULL, \
	                     Load1SimPRIORITY,( TaskHandle_t * ) NULL );
	#endif
	#if( ConfigUSE_EDF_SCHEDULAR == 1)
	xTaskPeriodicCreate(vLoad2SimTask, "Load2Simulator", Load2SimSTACK_SIZE, NULL, \
	                     Load2SimPRIORITY,Load2SimTaskPERIOD,( TaskHandle_t * ) NULL );
	#else
	xTaskCreate(vLoad2SimTask, "Load2Simulator", Load2SimSTACK_SIZE, NULL, \
	                     Load2SimPRIORITY,( TaskHandle_t * ) NULL );
	#endif

	
}
/* START FUNCTION DESCRIPTION ******************************************************************************************
vButton1MonitorTask()                                           <main.C>

SYNTAX:         vButton1MonitorTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
/*void ToggleTest(void *pvParameters)
{
		pinState_t   objButton1State;
	  SystemMessage objSystemMessage;
	  for(;;)
	  {
			GPIO_write(PORT_0, PIN2, PIN_IS_HIGH);
			 vTaskDelay(1000);
			GPIO_write(PORT_0, PIN2, PIN_IS_LOW);
			vTaskDelay(1000);   
		}
}*/
/* START FUNCTION DESCRIPTION ******************************************************************************************
vButton1MonitorTask()                                           <main.C>

SYNTAX:         vButton1MonitorTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
void vButton1MonitorTask(void *pvParameters)
{
		pinState_t   objButton1State;
	  SystemMessage objSystemMessage;
	  for(;;)
	  {
			   objButton1State = GPIO_read(PORT_0,PIN0);
			   if(xQueue != NULL)
				 {
					    objSystemMessage.objenumMessageType = csButton1StateCmd;
					    objSystemMessage.Data[0]            = objButton1State;    
							if(xQueueSend(xQueue, &objSystemMessage,  (TickType_t) 10) !=pdPASS)
							{
								   /*Cant Send Msg*/
							}
				 }
			   vTaskDelay(50);
	  }

}
/* START FUNCTION DESCRIPTION ******************************************************************************************
vButton1MonitorTask()                                           <main.C>

SYNTAX:         vButton1MonitorTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
void vButton2MonitorTask(void *pvParameters)
{
    pinState_t   objButton2State;
	  SystemMessage objSystemMessage;
	  for(;;)
	  {
			   objButton2State = GPIO_read(PORT_0,PIN1);
			   if(xQueue != NULL)
				 {
					    objSystemMessage.objenumMessageType = csButton2StateCmd;
					    objSystemMessage.Data[0]            = objButton2State;    
							if(xQueueSend(xQueue, &objSystemMessage,  (TickType_t) 10) !=pdPASS)
							{
								   /*Cant Send Msg*/
							}
				 }
			   vTaskDelay(50);
	  }
}

/* START FUNCTION DESCRIPTION ******************************************************************************************
vPeriodicTransmitterTask()                                           <main.C>

SYNTAX:         vPeriodicTransmitterTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
void vPeriodicTransmitterTask(void *pvParameters)
{
    /* The parameters are not used. */
		SystemMessage objSystemMessage;
    for(;;)
	  {
			  objSystemMessage.objenumMessageType = csPeriodicStringCmd;
				strcpy(objSystemMessage.Data, "Test EDF"); 
				if(xQueueSend(xQueue, &objSystemMessage,  (TickType_t) 10) !=pdPASS)
				{
						 /*Cant Send Msg*/
				}
			  vTaskDelay(100);
	  } 	
	
}
/* START FUNCTION DESCRIPTION ******************************************************************************************
vUARTRecTask()                                           <main.C>

SYNTAX:         vUARTRecTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
void vUARTRecTask(void *pvParameters)
{
    SystemMessage objSystemMessage;
	  for(;;)
	  {
			   if(xQueueReceive(xQueue , &(objSystemMessage), (TickType_t)10) == pdPASS)
				 {
					   switch(objSystemMessage.objenumMessageType)
						 {
								 case csButton1StateCmd:
									    if(objSystemMessage.Data[0] == PIN_IS_HIGH)
											{	
												  vSerialPutString("Button 1 is HIGH", strlen("Button 1 is HIGH"));
											}
										 	else
										  {
													vSerialPutString("Button 1 is low", strlen("Button 1 is Low"));
											}
											break;
								 case csButton2StateCmd:
									    if(objSystemMessage.Data[0] == PIN_IS_HIGH)
											{	
												  vSerialPutString("Button 2 is HIGH", strlen("Button 2 is HIGH"));
											}
										 	else
										  {
													vSerialPutString("Button 2 is low", strlen("Button 2 is low"));
											}
											break;
								 case csPeriodicStringCmd:
									    vSerialPutString((const signed char * const )objSystemMessage.Data, 8);
											break;
								 default:
										 break;
						 }							 
			   }
			   vTaskDelay(50);
	  }
}
/* START FUNCTION DESCRIPTION ******************************************************************************************
vLoad1SimTask()                                           <main.C>

SYNTAX:         vLoad1SimTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
void vLoad1SimTask(void *pvParameters)
{
	  int XCounter ;
	  for(;;)
		{
			  GPIO_write(PORT_0, PIN2, PIN_IS_HIGH);
				for(XCounter =0; XCounter < 36791; XCounter++ )
				{
					
				}
				GPIO_write(PORT_0, PIN2, PIN_IS_LOW);
				vTaskDelay(10);
		}
		
}
/* START FUNCTION DESCRIPTION ******************************************************************************************
vLoad2SimTask()                                           <main.C>

SYNTAX:         vLoad2SimTask()

DESCRIPTION :  

PARAMETER1  :   NA

RETURN VALUE:  NA

Note        :   NA

END DESCRIPTION *******************************************************************************************************/
void vLoad2SimTask(void *pvParameters)
{
    int XCounter ;
	  for(;;)
		{
			  GPIO_write(PORT_1, PIN0, PIN_IS_HIGH);
				for(XCounter =0; XCounter < 88298; XCounter++ )
				{
					
				}
				GPIO_write(PORT_1, PIN0, PIN_IS_LOW);
				vTaskDelay(100);
		}	
}
/*-----------------------------------------------------------*/


