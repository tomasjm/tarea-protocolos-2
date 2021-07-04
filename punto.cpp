#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include "ethernet/ethernet.h"
#include "protocol/protocol.h"
#include "slip/slip.h"
#include "menu/menu.h"
#include "helpers/helpers.h"

#define MAX_TRANSFER_SIZE 149

#define CLOCK_PIN_SEND 0
#define TX_PIN_SEND 2
#define RX_PIN_SEND 3

#define CLOCK_PIN_RECEIVE 23
#define TX_PIN_RECEIVE 22
#define RX_PIN_RECEIVE 21

void processBit(bool level);
void cbSend(void);
void cbReceive(void);
void printByteArray(BYTE *arr, int len);

volatile int nbitsSend, nbitsReceive = 0;
volatile int nbytesSend, nbytesReceive = 0;

int endCount = 0;
BYTE bytesToSend[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

char MAC_ADDRESS[18] = "b8:27:eb:15:b1:ca";
char MAC_ADDRESS2[18] = "11:27:bb:44:b1:ca";

BYTE slipArrayToSend[MAX_TRANSFER_SIZE];
BYTE slipArrayReceived[MAX_TRANSFER_SIZE];

Ethernet ethernet;
//Frame frame, receivedFrame;

bool waitForFrame = true;
bool frameReceived = false;

bool transmissionStartedSend = false;

bool transmissionStartedReceive = false;
typedef enum
{
    SENDING,
    RECEIVING,
    IDLE
} TRANSMISION_TYPE;

TRANSMISION_TYPE transmissionType = IDLE;

void startTransmission();

int main(int argc, char *args[])
{
    if (wiringPiSetup() == -1)
    {
        printf("No fue posible iniciar wiring pi\n");
        exit(1);
    }
    piHiPri(99);
    //CONFIGURA PINES DE ENTRADA SALIDA

    //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
    if (wiringPiISR(CLOCK_PIN_RECEIVE, INT_EDGE_FALLING, &cbReceive) < 0)
    {
        printf("Unable to start interrupt function\n");
    }

    if (wiringPiISR(CLOCK_PIN_SEND, INT_EDGE_RISING, &cbSend) < 0)
    {
        printf("Unable to start interrupt function\n");
    }

    pinMode(RX_PIN_RECEIVE, INPUT);
    pinMode(TX_PIN_RECEIVE, OUTPUT);

    pinMode(RX_PIN_SEND, INPUT);
    pinMode(TX_PIN_SEND, OUTPUT);
    for (int i = 0; i<argc; i++) {
        printf("arg %s i=%d\n", args[i], i);
    }
    if (atoi(args[1]) == 1)
    {
        empaquetaSlip(slipArrayToSend, bytesToSend, 10);
        printf("Paquete slip: ");
        printByteArray(slipArrayToSend, 20);

        //TRANSMITE EL MENSAJE
        startTransmission();
        while (transmissionStartedSend)
            printf("Sending data... %d bytes\n", nbytesSend);
    }
    else
    {
        printf("Delay\n");
        while (!frameReceived)
            delay(300);

        printf("Frame slip recibido!\n");
        BYTE data[50];
        for (int i = 0; i < 50; i++)
        {
            printf("Byte %d: 0x%x\n", i, slipArrayReceived[i]);
        }
        int len = desempaquetaSlip(data, slipArrayReceived);

        printf("\nData:\n");
        for (int i = 0; i < len; i++)
        {
            printf("Byte %d: %d\n", i, data[i]);
        }
    }

    return 0;
}
void cbReceive(void)
{
    bool level = digitalRead(RX_PIN_RECEIVE);
    processBit(level);
}

void processBit(bool level)
{
    //Inserta nuevo bit en byte actual
    BYTE pos = nbitsReceive;
    if (nbitsReceive > 7)
    {
        pos = 7;
        slipArrayReceived[nbytesReceive] = slipArrayReceived[nbytesReceive] >> 1;
        slipArrayReceived[nbytesReceive] &= 0x7f;
    }
    slipArrayReceived[nbytesReceive] |= level << pos;

    //Verifica si comienza transmisión
    if (!transmissionStartedReceive && slipArrayReceived[nbytesReceive] == 0xC0)
    {
        transmissionStartedReceive = true;
        nbitsReceive = 0;
        nbytesReceive++;
        return;
    }

    //Actualiza contadores y banderas
    nbitsReceive++;
    if (transmissionStartedReceive)
    {
        if (nbitsReceive == 8)
        {
            nbitsReceive = 0;
            if (slipArrayReceived[nbytesReceive] == 0xC0 && nbytesReceive > 0)
            {
                transmissionStartedReceive = false;
                //memcpy((void *)slipFrame, (void *)bytes, nbytesReceive + 1);
                nbytesReceive = 0;
                frameReceived = true;
                return;
            }
            nbytesReceive++;
        }
    }
}

void cbSend(void)
{
    if (transmissionStartedSend)
    {
        if (endCount == 0 && slipArrayToSend[nbytesSend] != 0xC0)
        {
            nbytesSend++;
            return;
        }

        //Escribe en el pin TX
        digitalWrite(TX_PIN_SEND, (slipArrayToSend[nbytesSend] >> nbitsSend) & 0x01); //Bit de dato

        //Actualiza contador de bits
        nbitsSend++;

        //Actualiza contador de bytes
        if (nbitsSend == 8)
        {
            nbitsSend = 0;
            endCount += slipArrayToSend[nbytesSend] == 0xC0;
            //Finaliza la comunicación
            if (slipArrayToSend[nbytesSend] == 0xC0 && endCount > 1)
            {
                endCount = 0;
                nbytesSend = 0;
                transmissionStartedSend = false;
                return;
            }
            nbytesSend++;
        }
    }
    else
    {
        //Canal en reposo
        digitalWrite(TX_PIN_SEND, 1);
    }
}

void startTransmission()
{
    transmissionStartedSend = true;
}

void printByteArray(BYTE *arr, int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("0x%x ", arr[i]);
    }
    printf("\n");
}