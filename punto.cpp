
//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip/slip.h"
#include <string.h>

//MACROS
#define CLOCK_PIN_RECEIVE       23
#define TX_PIN_RECEIVE          22
#define RX_PIN_RECEIVE          21

#define CLOCK_PIN_SEND 0
#define TX_PIN_SEND 2
#define RX_PIN_SEND 3

#define BYTE unsigned char

//PROTOTIPOS
void processBit(bool level);
void cbReceive(void);
void cbSend(void);
void startTransmission();
void printByteArray(BYTE* arr, int len);

//VARIABLES GLOBALES
volatile int nbits = 0;
volatile int nbytes = 0;
BYTE bytesToSend[10] = {1,2,3,4,5,6,7,8,9,10};
bool transmissionStarted = false;
bool frameReceived = false;
BYTE bytes[50];
BYTE slipFrame[50];

int main(){
  if(wiringPiSetup() == -1){
    printf("No fue posible iniciar wiring pi\n");
    exit(1);
  }
  piHiPri(99);
  //CONFIGURA PINES DE ENTRADA SALIDA


  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(CLOCK_PIN_RECEIVE, INT_EDGE_FALLING, &cb) < 0){
    printf("Unable to start interrupt function\n");
  }

  if(wiringPiISR(CLOCK_PIN_SEND, INT_EDGE_RISING, &cb) < 0){
    printf("Unable to start interrupt function\n");
  }

    pinMode(RX_PIN_RECEIVE, INPUT);
  pinMode(TX_PIN_RECEIVE, OUTPUT);

    pinMode(RX_PIN_SEND, INPUT);
  pinMode(TX_PIN_SEND, OUTPUT);

  for(int i = 0; i<50; i++){
    bytes[i] = 0;
  }

  printf("Delay\n");
  while(!frameReceived)
    delay(300);
  
  printf("Frame slip recibido!\n");
  BYTE data[50];
  for(int i = 0; i<50; i++){
    printf("Byte %d: 0x%x\n", i, slipFrame[i]);
  }
  int len = desempaquetaSlip(data, slipFrame);
  
  printf("\nData:\n");
  for(int i = 0; i<len; i++){
    printf("Byte %d: %d\n", i, data[i]);
  }
  
  return 0;
}

void cbReceive(void){
    printf("CB RECEIVE...\n");
  bool level = digitalRead(RX_PIN_RECEIVE);
  processBit(level);
}

void processBit(bool level){

  //Inserta nuevo bit en byte actual
  BYTE pos = nbits;
  if(nbits>7){
    pos = 7;
    bytes[nbytes] = bytes[nbytes] >> 1;
    bytes[nbytes] &= 0x7f;
  }
  bytes[nbytes] |= level << pos;

  //Verifica si comienza transmisión
  if(!transmissionStarted && bytes[nbytes] == 0xC0){
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
      // printf("0x%x\n", bytes[nbytes]);
      if(bytes[nbytes] == 0xC0 && nbytes>0){
        transmissionStarted = false;
        memcpy((void*)slipFrame, (void*)bytes, nbytes+1);
        nbytes = 0;
        frameReceived = true;
        return;
      }
      nbytes++;
    }
  }
}

void cbSend(void){
    printf("CB SEND...\n");
  if(transmissionStarted){
    if(endCount == 0 && slipFrame[nbytes] != 0xC0){
      nbytes++;
      return;
    }

    //Escribe en el pin TX
    digitalWrite(TX_PIN_SEND, (slipFrame[nbytes] >> nbits) & 0x01); //Bit de dato

    //Actualiza contador de bits
    nbits++;

    //Actualiza contador de bytes
    if(nbits == 8){
      nbits = 0;
      endCount += slipFrame[nbytes] == 0xC0;
      //Finaliza la comunicación
      if(slipFrame[nbytes] == 0xC0 && endCount>1){
        endCount = 0;
        nbytes = 0;
        transmissionStarted = false;
        return;
      }
      nbytes++;      
    }
  }else{
    //Canal en reposo
    digitalWrite(TX_PIN_SEND, 1);
  }
}

void startTransmission(){
  transmissionStarted = true;
}

void printByteArray(BYTE* arr, int len){
  for(int i = 0; i<len; i++){
    printf("0x%x ", arr[i]);
  }
  printf("\n");
}