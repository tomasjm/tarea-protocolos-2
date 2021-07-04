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

volatile int nbits = 0;
volatile int nbytes = 0;
int endCount = 0;
BYTE bytes[10] = {1,2,3,4,5,6,7,8,9,10};

char MAC_ADDRESS[18] = "b8:27:eb:15:b1:ca";
char MAC_ADDRESS2[18] = "11:27:bb:44:b1:ca";

BYTE slipArrayToSend[MAX_TRANSFER_SIZE];
BYTE slipArrayReceived[MAX_TRANSFER_SIZE];

Ethernet ethernet;
Frame frame, receivedFrame;

bool waitForFrame = true;

bool transmissionStarted = false;
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
  //INICIA WIRINGPI
  if (wiringPiSetup() == -1)
    exit(1);

  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if (wiringPiISR(CLOCK_PIN_SEND, INT_EDGE_RISING, &cbSend) < 0)
  {
    printf("Unable to start interrupt function\n");
  }
  if (wiringPiISR(CLOCK_PIN_RECEIVE, INT_EDGE_RISING, &cbReceive) < 0)
  {
    printf("Unable to start interrupt function\n");
  }
  //CONFIGURA PINES DE ENTRADA SALIDA
  pinMode(RX_PIN_RECEIVE, INPUT);
  pinMode(TX_PIN_RECEIVE, OUTPUT);
  pinMode(RX_PIN_SEND, INPUT);
  pinMode(TX_PIN_SEND, OUTPUT);

  int option = 0;
  while (true) {
    if (transmissionType == IDLE) {
      printMenu();
      getOptionAndValidate(&option);
    if (option == 0)
      continue;
    if (option == 1) {
      empaquetaSlip(slipArrayToSend, bytes, 10);
      startTransmission();
    } else if (option == 3) {
      exit(1);
    }
    }
    if (transmissionType == SENDING) {
      printf("sending data...\n");
      delay(1000);
    }
    if (transmissionType == RECEIVING) {
      printf("receiving data...\n");
      while(waitForFrame)
        delay(1000);
      BYTE data[50];
      int len = desempaquetaSlip(data, slipArrayReceived);

      printf("\nData:\n");
      for(int i = 0; i<len; i++){
        printf("Byte %d: %d\n", i, data[i]);
      }
      delay(20000);
    }
  }
  return 0;
}
void startTransmission() {
  transmissionType = SENDING;
  transmissionStarted = true;
}

void cbReceive(void){
  bool level = digitalRead(RX_PIN_RECEIVE);
  processBit(level);
}

void processBit(bool level) {
  //Inserta nuevo bit en byte actual
  BYTE pos = nbits;
  if(nbits>7){
    pos = 7;
    slipArrayReceived[nbytes] = slipArrayReceived[nbytes] >> 1;
    slipArrayReceived[nbytes] &= 0x7f;
  }
  slipArrayReceived[nbytes] |= level << pos;

  //Verifica si comienza transmisión
  if(!transmissionStarted && slipArrayReceived[nbytes] == 0xC0){
    transmissionStarted = true;
    nbits = 0;
    nbytes++;
    // printf("Encuentra 0xc0\n");
    return;
  }

  //Actualiza contadores y banderas
  nbits++;
  if(transmissionStarted){
    if(nbits==8){
      nbits = 0;
      if(slipArrayReceived[nbytes] == 0xC0 && nbytes>0){
        transmissionStarted = false;
        //memcpy((void*)slipFrame, (void*)bytes, nbytes+1);
        nbytes = 0;
        waitForFrame = false;
        transmissionType = IDLE;
        return;
      }
      nbytes++;
    }
  }
}

void cbSend(void){
  if(transmissionStarted){
    if(endCount == 0 && slipArrayToSend[nbytes] != 0xC0){
      nbytes++;
      return;
    }

    //Escribe en el pin TX
    digitalWrite(TX_PIN_SEND, (slipArrayToSend[nbytes] >> nbits) & 0x01); //Bit de dato

    //Actualiza contador de bits
    nbits++;

    //Actualiza contador de bytes
    if(nbits == 8){
      nbits = 0;
      endCount += slipArrayToSend[nbytes] == 0xC0;
      //Finaliza la comunicación
      if(slipArrayToSend[nbytes] == 0xC0 && endCount>1){
        endCount = 0;
        nbytes = 0;
        transmissionStarted = false;
        transmissionType = IDLE;
        return;
      }
      nbytes++;      
    }
  }else{
    //Canal en reposo
    digitalWrite(TX_PIN_SEND, 1);
  }
}
