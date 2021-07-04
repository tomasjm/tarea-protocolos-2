#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include "ethernet/ethernet.h"
#include "protocol/protocol.h"
#include "slip/slip.h"
#include "menu/menu.h"
#include "helpers/helpers.h"
#include "serial/serial.h"

#define MAX_TRANSFER_SIZE 300
#define BYTE unsigned char

// GLOBAL VARS FOR SENDING PURPOSES
volatile int nbitsSend = 0;
BYTE bytesToSend[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
BYTE slipArrayToSend[MAX_TRANSFER_SIZE];
volatile int nbytesSend = 0;
BYTE len = 10;
int nones = 0;
bool transmissionStartedSend = false;
int endCount = 0;
Ethernet ethernet;
Frame frame;

char macDestiny[18];

// GLOBAL VARS FOR RECEIVING PURPOSES
volatile int nbitsReceived = 0;
volatile int nbytesReceived = 0;
bool transmissionStartedReceive = false;
bool boolReceivedFrame = false;
BYTE bytesReceived[MAX_TRANSFER_SIZE];
BYTE slipArrayReceived[MAX_TRANSFER_SIZE];
bool error = false;
Frame receivedFrame;
char macOrigin[18];

int clockPin;
int txPin;
int rxPin;

void startTransmission();
void cb(void);
void processBit(bool level);

int main(int argc, char *args[])
{
    if (argc >= 6)
    {
        memcpy(macOrigin, args[1], sizeof(macOrigin));
        memcpy(macDestiny, args[2], sizeof(macDestiny));
        clockPin = atoi(args[3]);
        txPin = atoi(args[4]);
        rxPin = atoi(args[5]);
    }
    if (wiringPiSetup() == -1)
    {
        printf("Error initializing wiring pi\n");
        exit(1);
    }
    //pin config
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    printf("Pines clock: %d | tx: %d | rx: %d\n", clockPin, txPin, rxPin);
    delay(5000);

    // CONFIGURE INTERRUPT FOR SENDING AND RECEIVING DATA
    if (wiringPiISR(clockPin, INT_EDGE_BOTH, &cb) < 0)
    {
        printf("Unable to start interrupt function\n");
    }

    int option = 0;
    while (true)
    {
        if (!(transmissionStartedSend || transmissionStartedReceive))
        {
            BYTE boption[2];
            printMenu();
            fflush(stdout);
            int in = fileno(stdin);
            int n = readPort(in, boption, 2, 5000);
            //option = atoi(boption);
            printf("BOPTION %s\n", boption);
            boption[n-1] = '\0';
            delay(5000);
            if (option == 1)
            {
                prepareTransmissionOfTemperature(slipArrayToSend, macOrigin, macDestiny, ethernet, frame);
                startTransmission();
            }
            if (option == 2)
            {
                prepareTransmissionOfTextMessage(slipArrayToSend, macOrigin, macDestiny, ethernet, frame);
                startTransmission();
            }
            if (option == 3)
            {
                exit(1);
            }
        }
        if (transmissionStartedSend)
        {
            while (transmissionStartedSend)
            {
                clearScreen();
                printf("Sending data... %d bytes\n", nbytesSend);
                delay(1000);
            }
            memset(slipArrayToSend, 0, sizeof(slipArrayToSend));
        }

        while (transmissionStartedReceive)
        {
            clearScreen();
            printf("Receiving data... % bytes\n", nbytesReceived);
            delay(1000);
        }
        if (boolReceivedFrame)
        {
            error = getFrameFromTransmission(slipArrayReceived, receivedFrame);
            if (error)
            {
                printf("----- AN ERROR WAS DETECTED WITH FCS ----- \n");
                printf("-----    IGNORING COMPLETE MESSAGE   ----- \n");
                delay(5000);
            }
            else
            {
                if (receivedFrame.cmd == 1)
                {
                    printf("RECIBIDA TELEMETRIA\n");
                    delay(5000);
                }
                else if (receivedFrame.cmd == 2)
                {
                    printf("RECIBIDO MENSAJE DE TEXTO\n");
                    delay(5000);
                }
                else
                {
                    printf("RECIBIDO CMD DESCONOCIDO\n");
                    delay(5000);
                }
            }
            boolReceivedFrame = false;
            memset(slipArrayReceived, 0, sizeof(slipArrayReceived));
            memset(&receivedFrame, 0, sizeof(receivedFrame));
        }
    }

    return 0;
}

void cb(void)
{
    bool level = digitalRead(rxPin);
    processBit(level);
    if (transmissionStartedSend)
    {
        if (endCount == 0 && slipArrayToSend[nbytesSend] != 0xC0)
        {
            nbytesSend++;
            return;
        }

        //Escribe en el pin TX
        digitalWrite(txPin, (slipArrayToSend[nbytesSend] >> nbitsSend) & 0x01); //Bit de dato

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
        digitalWrite(txPin, 1);
    }
}

void processBit(bool level)
{

    //Inserta nuevo bit en byte actual
    BYTE pos = nbitsReceived;
    if (nbitsReceived > 7)
    {
        pos = 7;
        bytesReceived[nbytesReceived] = bytesReceived[nbytesReceived] >> 1;
        bytesReceived[nbytesReceived] &= 0x7f;
    }
    bytesReceived[nbytesReceived] |= level << pos;

    //Verifica si comienza transmisión
    if (!transmissionStartedReceive && bytesReceived[nbytesReceived] == 0xC0)
    {
        transmissionStartedReceive = true;
        nbitsReceived = 0;
        nbytesReceived++;
        // printf("Encuentra 0xc0\n");
        return;
    }

    //Actualiza contadores y banderas
    nbitsReceived++;
    if (transmissionStartedReceive)
    {
        if (nbitsReceived == 8)
        {
            nbitsReceived = 0;
            // printf("0x%x\n", bytesReceived[nbytesReceived]);
            if (bytesReceived[nbytesReceived] == 0xC0 && nbytesReceived > 0)
            {
                transmissionStartedReceive = false;
                memcpy((void *)slipArrayReceived, (void *)bytesReceived, nbytesReceived + 1);
                nbytesReceived = 0;
                boolReceivedFrame = true;
                return;
            }
            nbytesReceived++;
        }
    }
}

void startTransmission()
{
    transmissionStartedSend = true;
}