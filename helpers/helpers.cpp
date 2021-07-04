#include <time.h>
#include <stdio.h>
#include "helpers.h"
#include "../ethernet/ethernet.h"
#include "../protocol/protocol.h"
#include "../slip/slip.h"

#define BYTE unsigned char

void cDelay(int milliSeconds) {
  clock_t startTime = clock()/CLOCKS_PER_SEC;
  while (clock()/CLOCKS_PER_SEC < startTime + milliSeconds/1000);
}

void readSensorData(int q, int valuesArr[], int timesArr[]) {
  // FILE *fp;
  // int rawTemp;
  // for (int i = 0; i<q; i++) {
  // fp = popen("cat /sys/bus/w1/devices/28-0113126a6baa/w1_slave | grep -i -o \"t=[0-9]*\" | grep -o \"[0-9]*\"", "r");
  // fscanf(fp, "%d", &rawTemp);
  // valuesArr[i] = rawTemp+10000;
  // timesArr[i] = time(NULL);
  // printf("Got %d temperature values of %d\n", i+1, q);
  // printf("Raw temperature value %d\n", rawTemp);
  // pclose(fp);
  // }
  valuesArr[0] = 999;
  timesArr[0] = time(NULL);
  printf("Raw temperature value %d\n", valuesArr[0]);
  printf("Raw timestamp value %d\n", timesArr[0]);
}

void getByteArrayOfInteger(int v, BYTE arr[]) {
  int l = sizeof(v);
  for (int i = 0; i<l; i++) {
    arr[i] = (v >> (8*i)) & 0xFF;
  }
}

void getIntegerOfByteArray(BYTE arr[], int *vPtr) {
  for (int i = 0; i< sizeof(int); i++) {
    *vPtr |= (arr[i] << (i*8));
  }
}

void prepareTransmissionOfTemperature(BYTE slipArray[], char strMacSource[18], char strMacDestiny[18], Ethernet &ethf, Frame &f) {
  int tempValue[1];
  int timeValue[1];
  readSensorData(1, tempValue, timeValue);
  BYTE byteTempValue[4];
  BYTE byteTimeValue[4];
  BYTE dataFrameToSend[8];
  getByteArrayOfInteger(tempValue[0], byteTempValue);
  getByteArrayOfInteger(timeValue[0], byteTimeValue);
  for (int i = 0; i<4; i++) {
    dataFrameToSend[i] = byteTempValue[i];
    dataFrameToSend[i+4] = byteTimeValue[i];
  }
  generateRawFrame(f, 1, 0, 8, dataFrameToSend);
  generateRawEthernet(ethf, f, strMacSource, strMacDestiny);
  empaquetaSlip(slipArray, ethf.frame, 28);
}

void prepareTransmissionOfTextMessage(BYTE slipArray[], char strMacSource[18], char strMacDestiny[18], Ethernet &ethf, Frame &f) {
  BYTE msg[30];
  printf("Enter message of 30 characters to send: \n");
  getTextMessage((char*)msg, 30);
  generateRawFrame(f, 2, 0, 30, msg);
  generateRawEthernet(ethf, f, strMacSource, strMacDestiny);
  empaquetaSlip(slipArray, ethf.frame, 50);
}

void getTextMessage(char msg[], int length) {
  char c;
  while ((c = getchar()) != '\n' && c != EOF); 
  fgets(msg, length, stdin);
}

void getValuesFromTemperatureFrame(Frame &f, int *vTemp, int *vTimestamp) {
  BYTE temp[4];
  BYTE timestamp[4];
  for (int i = 0; i<4; i++) {
    temp[i] = f.data[i];
    timestamp[i] = f.data[i+4];
  }
  getIntegerOfByteArray(temp, vTemp);
  getIntegerOfByteArray(timestamp, vTimestamp);
}

void getMessageFromTextMessageFrame(Frame &f, char msg[]) {
  for (int i = 0; i<30; i++) {
    msg[i] = f.data[i];
  }
}

void generateRawEthernet(Ethernet &ethf, Frame &f, char strMacSource[18], char strMacDestiny[18]) {
  BYTE macSource[6], macDestiny[6];
  convertMacAddressToByteArray(strMacSource, macSource);
  convertMacAddressToByteArray(strMacDestiny, macDestiny);
  memcpy(ethf.source, macSource, sizeof(ethf.source));
  memcpy(ethf.destiny, macDestiny, sizeof(ethf.destiny));
  ethf.length = 2+f.length;
  memcpy(ethf.data, f.frame, sizeof(ethf.data));
  packEthernet(ethf);
}

void generateRawFrame(Frame &f, int cmd, int sa, int length, BYTE data[]) {
  f.cmd = cmd;
  f.sa = sa;
  f.length = length;
  memcpy(f.data, data, length);
  generateFrameToSend(f);
}

bool getFrameFromTransmission(BYTE slipArray[], Frame &f) {
  Ethernet ef;
  int n = desempaquetaSlip(ef.frame, slipArray);
  bool error = unpackEthernet(ef);
  if (error) {
    printf("error\n");
    return true;
  }
  printByteArray(ef.data, 34);
  memcpy(f.frame, ef.data, sizeof(f.frame));
  generateReceivedFrame(f);
  return false;
}

void printByteArray(BYTE* arr, int len){
  for(int i = 0; i<len; i++){
    printf("0x%x ", arr[i]);
  }
  printf("\n");
}