/**
*  \file client.c
*    
*  \brief This is a client program which connects to echoserver to
*        start communication with echoserver.
*/
/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PKT_SIZE 1024  //buffer
//#define MAX_NUM_PKTS 50

#define	MY_MESSAGE	"Hello World\n"
int main(int argc, char*argv[])
{
    int sock;
    unsigned int i;  
    int snd_err = 0;
    int rcv_err = 0;
    struct sockaddr_in server_addr;
    unsigned char send_data[1024], recv_data[1024];

    if(argc!=2)
    {
         printf("server ip needs to be passed as command line argument\n");
         return -1;
    }

    printf("\nNumber of bytes  to be transferred %d\n" , MAX_PKT_SIZE);

    /* Allocate send and receive buffers */
   // send_data = calloc(MAX_PKT_SIZE, sizeof(char));
    //recv_data = calloc(MAX_PKT_SIZE, sizeof(char));

    if (!send_data || !recv_data)
    {
        printf("Error ! No mem for buffers \n");
    }
    strcpy(send_data,MY_MESSAGE);
    //for (i = 0; i < MAX_PKT_SIZE; i++)
    //{
	//send_data[i] = ;	
    //}

    memset(recv_data, 0, MAX_PKT_SIZE);
  
    /* create a socket */ 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
            perror("Socket");
            exit(1);
    }

    server_addr.sin_family = AF_INET;     
    server_addr.sin_port = htons(2000);   
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    bzero(&(server_addr.sin_zero),8); 

    printf("connecting .......\n");
    /* connect to the server */
    if (connect(sock, (struct sockaddr *)&server_addr,
                       sizeof(struct sockaddr)) == -1) 
    {
            perror("Connect");
            exit(1);
    }
   
    //for(i = 0; i < MAX_NUM_PKTS ; i++)
    //{
       unsigned int j;

        /* send data to server */    
        printf("\nsending data..........");

        snd_err = send(sock, send_data, strlen(send_data +1), 0);

        printf("tranferred data : %s",send_data);

	if (snd_err < 0)
	    printf("\n send error : %s\n", strerror(snd_err));

        printf("\nreceiving data..........\n");
    
        /* receive the data from server */
        rcv_err = recv(sock, recv_data, MAX_PKT_SIZE, MSG_WAITALL);

	if (rcv_err < 0)
	    printf("\n recv error %s\n", strerror(rcv_err));
 
 	printf("number of bytes received = %d\n" , rcv_err);

	/*for (j = 0; (j < MAX_PKT_SIZE) && (snd_err >= 0 && rcv_err >= 0); j++)
	{
	    if ( recv_data[j] != send_data[j])
	    {
		printf("Data error at %d for %d transfer \n", j, i);
		printf("send %d ; recv : %d \n", send_data[j], recv_data[j]);
		break;
	    }
        }
	
	if (j == MAX_PKT_SIZE)
        {
	    printf("Data passed for %d  transfer \n", i);
        }*/
printf("received data : %s ",recv_data);

        memset(recv_data, 0, MAX_PKT_SIZE);
    
    return 0;

}








#include "uart_irda_cir.h"
#include "soc_AM335x.h"
#include "interrupt.h"
#include "beaglebone.h"
#include "consoleUtils.h"
#include "hw_types.h"
#include "watchdog.h"

/******************************************************************************
**              INTERNAL MACRO DEFINITIONS
******************************************************************************/
#define BAUD_RATE_115200          (115200)
#define UART_MODULE_INPUT_CLK     (48000000)

/*
** The number of data bytes to be transmitted to Transmit FIFO of UART
** per generation of the Transmit Empty interrupt. This can take a maximum
** value of TX Trigger Space which is 'TX FIFO size - TX Threshold Level'.
*/
#define NUM_TX_BYTES_PER_TRANS    (56)

/******************************************************************************
**              INTERNAL FUNCTION PROTOTYPES
******************************************************************************/
static void UartInterruptEnable(void);
static void UART0AINTCConfigure(void);
static void UartFIFOConfigure(void);
static void UartBaudRateSet(void);
static void UARTIsr(void);

/******************************************************************************
**              GLOBAL VARIABLE DEFINITIONS
******************************************************************************/
unsigned char txArray[] = "StarterWare AM335X UART Interrupt application\r\n";

/* A flag used to signify the application to transmit data to UART TX FIFO. */
unsigned int txEmptyFlag = FALSE;

/*
** A variable which holds the number of bytes of the data block transmitted to
** UART TX FIFO until the current instant.
*/
unsigned int currNumTxBytes = 0;

/******************************************************************************
**              FUNCTION DEFINITIONS
******************************************************************************/

int main()
{
    unsigned int numByteChunks = 0;
    unsigned int remainBytes = 0;
    unsigned int bIndex = 0;

    WatchdogTimerDisable(SOC_WDT_1_REGS);
 ConsoleUtilsPrintf(" Data check\n ");

    /* Configuring the system clocks for UART0 instance. */
    UART0ModuleClkConfig();


    /* Performing the Pin Multiplexing for UART0 instance. */
    UARTPinMuxSetup(0);

    /* Performing a module reset. */
    UARTModuleReset(SOC_UART_0_REGS);

    /* Performing FIFO configurations. */
    UartFIFOConfigure();

    /* Performing Baud Rate settings. */
    UartBaudRateSet();

    /* Switching to Configuration Mode B. */
    UARTRegConfigModeEnable(SOC_UART_0_REGS, UART_REG_CONFIG_MODE_B);

    /* Programming the Line Characteristics. */
    UARTLineCharacConfig(SOC_UART_0_REGS, 
                         (UART_FRAME_WORD_LENGTH_8 | UART_FRAME_NUM_STB_1), 
                         UART_PARITY_NONE);

    /* Disabling write access to Divisor Latches. */
    UARTDivisorLatchDisable(SOC_UART_0_REGS);

    /* Disabling Break Control. */
    UARTBreakCtl(SOC_UART_0_REGS, UART_BREAK_COND_DISABLE);

    /* Switching to UART16x operating mode. */
    UARTOperatingModeSelect(SOC_UART_0_REGS, UART16x_OPER_MODE);

    /* Select the console type based on compile time check */
    ConsoleUtilsSetType(CONSOLE_UART);

    /* Performing Interrupt configurations. */
    UartInterruptEnable();

    numByteChunks = (sizeof(txArray) - 1) / NUM_TX_BYTES_PER_TRANS;
    remainBytes = (sizeof(txArray) - 1) % NUM_TX_BYTES_PER_TRANS;

    while(1)
    {
        /* This branch is entered if the transmission is not yet complete. */
        if(TRUE == txEmptyFlag)
        {
            if(bIndex < numByteChunks)
            {
                /* Transmitting bytes in chunks of NUM_TX_BYTES_PER_TRANS. */
                currNumTxBytes += UARTFIFOWrite(SOC_UART_0_REGS,
                                                &txArray[currNumTxBytes],
                                                NUM_TX_BYTES_PER_TRANS);

                bIndex++;
            }

            else
            {
                /* Transmitting remaining data from the data block. */
                currNumTxBytes += UARTFIFOWrite(SOC_UART_0_REGS,
                                                &txArray[currNumTxBytes],
                                                remainBytes);
            }

            txEmptyFlag = FALSE;

            /*
            ** Re-enables the Transmit Interrupt. This interrupt
            ** was disabled in the Transmit section of the UART ISR.
            */
            UARTIntEnable(SOC_UART_0_REGS, UART_INT_THR);
        }
    }
}

/*
** A wrapper function performing FIFO configurations.
*/

static void UartFIFOConfigure(void)
{
    unsigned int fifoConfig = 0;

    /*
    ** - Transmit Trigger Level Granularity is 4
    ** - Receiver Trigger Level Granularity is 1
    ** - Transmit FIFO Space Setting is 56. Hence TX Trigger level
    **   is 8 (64 - 56). The TX FIFO size is 64 bytes.
    ** - The Receiver Trigger Level is 1.
    ** - Clear the Transmit FIFO.
    ** - Clear the Receiver FIFO.
    ** - DMA Mode enabling shall happen through SCR register.
    ** - DMA Mode 0 is enabled. DMA Mode 0 corresponds to No
    **   DMA Mode. Effectively DMA Mode is disabled.
    */
    fifoConfig = UART_FIFO_CONFIG(UART_TRIG_LVL_GRANULARITY_4,
                                  UART_TRIG_LVL_GRANULARITY_1,
                                  UART_FCR_TX_TRIG_LVL_56,
                                  1,
                                  1,
                                  1,
                                  UART_DMA_EN_PATH_SCR,
                                  UART_DMA_MODE_0_ENABLE);

    /* Configuring the FIFO settings. */
    UARTFIFOConfig(SOC_UART_0_REGS, fifoConfig);
}

/*
** A wrapper function performing Baud Rate settings.
*/

static void UartBaudRateSet(void)
{
    unsigned int divisorValue = 0;

    /* Computing the Divisor Value. */
    divisorValue = UARTDivisorValCompute(UART_MODULE_INPUT_CLK,
                                         BAUD_RATE_115200,
                                         UART16x_OPER_MODE,
                                         UART_MIR_OVERSAMPLING_RATE_42);

    /* Programming the Divisor Latches. */
    UARTDivisorLatchWrite(SOC_UART_0_REGS, divisorValue);
}

/*
** A wrapper function performing Interrupt configurations.
*/

static void UartInterruptEnable(void)
{
    /* Enabling IRQ in CPSR of ARM processor. */
    IntMasterIRQEnable();

    /* Configuring AINTC to receive UART0 interrupts. */
    UART0AINTCConfigure();

    /* Enabling the specified UART interrupts. */
    UARTIntEnable(SOC_UART_0_REGS, (UART_INT_LINE_STAT | UART_INT_THR |
                                    UART_INT_RHR_CTI));
}

/*
** Interrupt Service Routine for UART.
*/

static void UARTIsr(void)
{
    unsigned int rxErrorType = 0;
    unsigned char rxByte = 0;
    unsigned int intId = 0;
    unsigned int idx = 0;

    /* Checking ths source of UART interrupt. */
    intId = UARTIntIdentityGet(SOC_UART_0_REGS);

    switch(intId)
    {
        case UART_INTID_TX_THRES_REACH:

            /*
            ** Checking if the entire transmisssion is complete. If this
            ** condition fails, then the entire transmission has been completed.
            */
            if(currNumTxBytes < (sizeof(txArray) - 1))
            {
                txEmptyFlag = TRUE;
            }

            /*
            ** Disable the THR interrupt. This has to be done even if the
            ** transmission is not complete so as to prevent the Transmit
            ** empty interrupt to be continuously generated.
            */
            UARTIntDisable(SOC_UART_0_REGS, UART_INT_THR);

        break;

        case UART_INTID_RX_THRES_REACH:
            rxByte = UARTCharGetNonBlocking(SOC_UART_0_REGS);
            UARTCharPutNonBlocking(SOC_UART_0_REGS, rxByte);
        break;

        case UART_INTID_RX_LINE_STAT_ERROR:

            rxErrorType = UARTRxErrorGet(SOC_UART_0_REGS);

            /* Check if Overrun Error has occured. */
            if(rxErrorType & UART_LSR_RX_OE)
            {
                ConsoleUtilsPrintf("\r\nUART Overrun Error occured."
                              " Reading and Echoing all data in RX FIFO.\r\n");

                /* Read the entire RX FIFO and the data in RX Shift register. */
                for(idx = 0; idx < (RX_FIFO_SIZE + 1); idx++)
                {
                    rxByte = UARTFIFOCharGet(SOC_UART_0_REGS);
                    UARTFIFOCharPut(SOC_UART_0_REGS, rxByte);
                }

                break;
            }

            /* Check if Break Condition has occured. */
            else if(rxErrorType & UART_LSR_RX_BI)
            {
                ConsoleUtilsPrintf("\r\nUART Break Condition occured.");
            }

            /* Check if Framing Error has occured. */
            else if(rxErrorType & UART_LSR_RX_FE)
            {
                ConsoleUtilsPrintf("\r\nUART Framing Error occured.");
            }

            /* Check if Parity Error has occured. */
            else if(rxErrorType & UART_LSR_RX_PE)
            {
                ConsoleUtilsPrintf("\r\nUART Parity Error occured.");
            }

            ConsoleUtilsPrintf(" Data at the top of RX FIFO is: ");
            rxByte = UARTFIFOCharGet(SOC_UART_0_REGS);
            UARTFIFOCharPut(SOC_UART_0_REGS, rxByte);

        break;

        case UART_INTID_CHAR_TIMEOUT:

            ConsoleUtilsPrintf("\r\nUART Character Timeout Interrupt occured."
                              " Reading and Echoing all data in RX FIFO.\r\n");

            /* Read all the data in RX FIFO. */
            while(TRUE == UARTCharsAvail(SOC_UART_0_REGS))
            {
                rxByte = UARTFIFOCharGet(SOC_UART_0_REGS);
                UARTFIFOCharPut(SOC_UART_0_REGS, rxByte);
            }

        break;
    
        default:
        break;    
    }

}

/*
** This function configures the AINTC to receive UART interrupts.
*/

static void UART0AINTCConfigure(void)
{
    /* Initializing the ARM Interrupt Controller. */
    IntAINTCInit();

    /* Registering the Interrupt Service Routine(ISR). */
    IntRegister(SYS_INT_UART0INT, UARTIsr);

    /* Setting the priority for the system interrupt in AINTC. */
    IntPrioritySet(SYS_INT_UART0INT, 0, AINTC_HOSTINT_ROUTE_IRQ);

    /* Enabling the system interrupt in AINTC. */
    IntSystemEnable(SYS_INT_UART0INT);    
}

/******************************* End of file *********************************/
