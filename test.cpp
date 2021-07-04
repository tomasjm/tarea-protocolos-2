#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ethernet/ethernet.h"
#include "protocol/protocol.h"
#include "helpers/helpers.h"

#define MAXSIZE 300
#define BYTE unsigned char

char MAC_ADDRESS[18]="b8:27:eb:15:b1:ca";

BYTE slipArray[MAXSIZE];
BYTE sf[MAXSIZE];
Ethernet ethernet, ef;
Frame frame, ff;

int main(int argc, char* args[]) {
  prepareTransmissionOfTemperature(slipArray, MAC_ADDRESS, MAC_ADDRESS, ethernet, frame);
  memcpy(sf, slipArray, sizeof(sf));
  getFrameFromTransmission(sf, ff, ef);

  int temp = 0;
  int timestamp = 0;
  getValuesFromTemperatureFrame(ff, &temp, &timestamp);
  printf("mensaje %d %d \n", temp, timestamp);

  printByteArray()

  return 0;
}
