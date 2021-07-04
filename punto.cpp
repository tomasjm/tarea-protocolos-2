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

#define CLOCK_PIN       23
#define TX_PIN          22
#define RX_PIN          21

void processBit(bool level);
void cb(void);

volatile int nbytes = 0;
int endCount = 0;

char MAC_ADDRESS[18]="b8:27:eb:15:b1:ca";
char MAC_ADDRESS2[18]="11:27:bb:44:b1:ca";

BYTE slipArrayToSend[MAX_TRANSFER_SIZE];
BYTE slipArrayReceived[MAX_TRANSFER_SIZE];

Ethernet ethernet;
Frame frame, receivedFrame;

bool transmissionStarted = false;
void startTransmission();



int main(int argc, char* args[]) {
  //INICIA WIRINGPI
  if(wiringPiSetup() == -1)
    exit(1);

  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(CLOCK_PIN, INT_EDGE_RISING, &cb) < 0){
    printf("Unable to start interrupt function\n");
  }

  //CONFIGURA PINES DE ENTRADA SALIDA
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  return 0;
}

void cb(void){
  if(transmissionStarted){
    if(endCount == 0 && slipArrayToSend[nbytes] != 0xC0){
      nbytes++;
      return;
    }

    //Escribe en el pin TX
    digitalWrite(TX_PIN, (slipArrayToSend[nbytes] >> nbits) & 0x01); //Bit de dato

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
        return;
      }
      nbytes++;      
    }
  }else{
    //Canal en reposo
    digitalWrite(TX_PIN, 1);
  }
}

void startTransmission() {
  transmissionStarted = true;
}

