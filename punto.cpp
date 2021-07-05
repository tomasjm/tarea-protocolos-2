#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include "ethernet/ethernet.h"
#include "protocol/protocol.h"
#include "slip/slip.h"
#include "menu/menu.h"
#include "helpers/helpers.h"
#include <unistd.h>
#include <poll.h>

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
Ethernet receivedEthernet;
char macOrigin[18];

int clockPin;
int txPin;
int rxPin;

void startTransmission();
void cb(void);
void processBit(bool level);

int main(int argc, char *args[])
{
    // Check for ARGS: macOrigin macDestiny clockPin txPin rxPin
    if (argc >= 6)
    {
        memcpy(macOrigin, args[1], sizeof(macOrigin));
        memcpy(macDestiny, args[2], sizeof(macDestiny));
        clockPin = atoi(args[3]);
        txPin = atoi(args[4]);
        rxPin = atoi(args[5]);
    }
    // Setup wiring pi
    if (wiringPiSetup() == -1)
    {
        printf("Error initializing wiring pi\n");
        exit(1);
    }
    //pin config
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    printf("Pins: clock: %d | tx: %d | rx: %d\n", clockPin, txPin, rxPin);
    delay(5000);

    // CONFIGURE INTERRUPT FOR SENDING AND RECEIVING DATA
    if (wiringPiISR(clockPin, INT_EDGE_BOTH, &cb) < 0)
    {
        printf("Unable to start interrupt function\n");
    }

    int option = 0;
    // Main LOOP
    while (true)
    {
        // if there is any transmission it wont print menu
        if (!(transmissionStartedSend || transmissionStartedReceive))
        {
            // prints menu and waits for a response with a timeout of 3s
            // timeout is in a loop, it purporse is to check if there is a transmission active ignoring pause of scanf
            printMenu();
            struct pollfd mypoll = {STDIN_FILENO, POLLIN | POLLPRI};
            if (poll(&mypoll, 1, 3000))
            {
                scanf("%d", &option);
            }
            else
            {
                option = 0;
            }
            if (option == 1)
            {
                // Prepares an SlipArray with Telemetry data and starts the transmission
                prepareTransmissionOfTemperature(slipArrayToSend, macOrigin, macDestiny, ethernet, frame);
                startTransmission();
            }
            if (option == 2)
            {
                // Prepares an SlipArray with a TextMessage data and starts the transmission
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
            // RESET ARRAY TO SEND
            memset(slipArrayToSend, 0, sizeof(slipArrayToSend));
        }

        while (transmissionStartedReceive)
        {
            clearScreen();
            printf("Receiving data... %d bytes\n", nbytesReceived);
            delay(1000);
        }
        // if we have received a transmission frame
        if (boolReceivedFrame)
        {
            //printByteArray(slipArrayReceived, sizeof(slipArrayReceived));
            delay(1000);
            // function returns an FCS error if it exists, also get the communication frame from a slip array
            error = getFrameFromTransmission(slipArrayReceived, receivedFrame, receivedEthernet);
            BYTE byteMacOrigin[6];
            convertMacAddressToByteArray(macOrigin, byteMacOrigin);
            bool isForMe = compareMacAddress(receivedEthernet.destiny, byteMacOrigin);
            if (error)
            {
                printf("----- AN ERROR WAS DETECTED WITH FCS ----- \n");
                printf("-----    IGNORING COMPLETE MESSAGE   ----- \n");
                delay(5000);
            }
            // check for if destiny mac address equals to local mac address
            else if (!isForMe) {
                printf("----- MESSAGE WASN'T FOR THIS MAC ADDRESS ----- \n");
                printf("-----    IGNORING COMPLETE MESSAGE   ----- \n");
                delay(5000);
            } else
            {
                
                // if cmd = 1, is a telemetry data
                if (receivedFrame.cmd == 1)
                {
                    int temp = 0;
                    int timestamp = 0;
                    // Gets telemetry data from a frame with cmd = 1
                    getValuesFromTemperatureFrame(receivedFrame, &temp, &timestamp);
                    printf("----- RECEIVED TELEMETRY ----- \n");
                    printf("Temperature: %.2f \n", ((float)temp-10000)/1000);
                    printf("Timestamp: %d \n", timestamp);
                    delay(5000);
                }
                // if cmd = 2, is a text message
                else if (receivedFrame.cmd == 2)
                {
                    char msg[30];
                    // Gets a text message data from a frame with cmd = 2
                    getMessageFromTextMessageFrame(receivedFrame, msg);
                    printf("----- RECEIVED TEXT MESSAGE ----- \n");
                    printf("Text message: %s \n", msg);
                    delay(5000);
                }
                else
                {
                    // if cmd has another value, probably is an undetected error
                    printf("Received an unknown cmd, probably an error...\n");
                    delay(5000);
                }
            }
            // resets received variables for a new transmission
            boolReceivedFrame = false;
            memset(&receivedFrame, 0, sizeof(receivedFrame));
            memset(&receivedEthernet, 0, sizeof(receivedEthernet));
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

        // Writes on TX Pin
        digitalWrite(txPin, (slipArrayToSend[nbytesSend] >> nbitsSend) & 0x01);

        // Update bit counter
        nbitsSend++;

        // Update byte counter
        if (nbitsSend == 8)
        {
            nbitsSend = 0;
            endCount += slipArrayToSend[nbytesSend] == 0xC0;
            // Finish transmission
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
        // Channel idle
        digitalWrite(txPin, 1);
    }
}

void processBit(bool level)
{

    //Insert a bit in actual byte
    BYTE pos = nbitsReceived;
    if (nbitsReceived > 7)
    {
        pos = 7;
        bytesReceived[nbytesReceived] = bytesReceived[nbytesReceived] >> 1;
        bytesReceived[nbytesReceived] &= 0x7f;
    }
    bytesReceived[nbytesReceived] |= level << pos;

    //Verify if transmission has started
    if (!transmissionStartedReceive && bytesReceived[nbytesReceived] == 0xC0)
    {
        transmissionStartedReceive = true;
        nbitsReceived = 0;
        nbytesReceived++;
        return;
    }

    // update counters and flags
    nbitsReceived++;
    if (transmissionStartedReceive)
    {
        if (nbitsReceived == 8)
        {
            nbitsReceived = 0;
            if (bytesReceived[nbytesReceived] == 0xC0 && nbytesReceived > 0)
            {
                transmissionStartedReceive = false;
                memcpy((void *)slipArrayReceived, (void *)bytesReceived, nbytesReceived + 1);
                memset(bytesReceived, 0, sizeof(bytesReceived));
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