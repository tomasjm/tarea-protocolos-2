#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

BYTE slipArrayToSend[MAX_TRANSFER_SIZE];
BYTE slipArrayReceived[MAX_TRANSFER_SIZE];

Ethernet ethernet;
Frame frame, receivedFrame;

bool transmissionStarted = false;
void startTransmission();



int main(int argc, char* args[]) {
  // if(wiringPiSetup() == -1){
  //   printf("No fue posible iniciar wiring pi\n");
  //   exit(1);
  // }
  // piHiPri(99);
  // //CONFIGURA PINES DE ENTRADA SALIDA
  // pinMode(RX_PIN, INPUT);
  // pinMode(TX_PIN, OUTPUT);

  // //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  // if(wiringPiISR(CLOCK_PIN, INT_EDGE_FALLING, &cb) < 0){
  //   printf("Unable to start interrupt function\n");
  //   return 1;
  // }

  int option = 0;
  while (true) {
    printMenu();
    getOptionAndValidate(&option);
    if (option == 0)
      continue;
    if (option == 1) {
      prepareTransmissionOfTemperature(slipArrayToSend, MAC_ADDRESS, MAC_ADDRESS, ethernet, frame);
      startTransmission();
    } else if (option == 2) {
      prepareTransmissionOfTextMessage(slipArrayToSend, MAC_ADDRESS, MAC_ADDRESS, ethernet, frame);
      startTransmission();
    } else if (option == 3) {
      exit(1);
    }
    while(transmissionStarted){
      transmissionStarted = false;
      memcpy(slipArrayReceived, slipArrayToSend, sizeof(slipArrayReceived));
      cDelay(2000);
    }
    getFrameFromTransmission(slipArrayReceived, receivedFrame);
    if (receivedFrame.cmd == 1) {
      int temp = 0;
      int timestamp = 0;
      getValuesFromTemperatureFrame(receivedFrame, &temp, &timestamp);
      printf("%d %d\n", temp, timestamp);

    } else if(receivedFrame.cmd == 2) {
      char msg[30];
      getMessageFromTextMessageFrame(receivedFrame, msg);
      printf("mensaje %s\n", msg);
    }
    cDelay(5000);
    memset(slipArrayToSend, 0, sizeof(slipArrayToSend));
    memset(&frame, 0, sizeof(frame));
    memset(&ethernet, 0, sizeof(ethernet));
  }
  
  return 0;
}

// void cb(void){
//   if(transmissionStarted){
//     if(endCount == 0 && slipArrayToSend[nbytes] != 0xC0){
//       nbytes++;
//       return;
//     }

//     //Escribe en el pin TX
//     digitalWrite(TX_PIN, (slipArrayToSend[nbytes] >> nbits) & 0x01); //Bit de dato

//     //Actualiza contador de bits
//     nbits++;

//     //Actualiza contador de bytes
//     if(nbits == 8){
//       nbits = 0;
//       endCount += slipArrayToSend[nbytes] == 0xC0;
//       //Finaliza la comunicación
//       if(slipArrayToSend[nbytes] == 0xC0 && endCount>1){
//         endCount = 0;
//         nbytes = 0;
//         transmissionStarted = false;
//         return;
//       }
//       nbytes++;      
//     }
//   }else{
//     //Canal en reposo
//     digitalWrite(TX_PIN, 1);
//   }
// }

void startTransmission() {
  transmissionStarted = true;
}

