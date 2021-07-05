#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ethernet.h"
#define BYTE unsigned char


void packEthernet(Ethernet &eth) {
  for (int i = 0; i<6; i++) {
    eth.frame[i] = eth.source[i];
    eth.frame[i+6] = eth.destiny[i];
  }
  eth.frame[12] = eth.length & 0xFF;
  eth.frame[13] = (eth.length>>8) & 0xFF;
  for (int i = 0; i<eth.length; i++) {
    eth.frame[i+14] = eth.data[i];
  }
  eth.fcs = 0;
  eth.fcs = fcs(eth.frame, 14+eth.length);
  for(int i=0;i<4;i++){
		eth.frame[14+eth.length+i] = (eth.fcs>>(8*i))&0xFF;
	}
}

bool unpackEthernet(Ethernet &eth) {
  eth.length = ((eth.frame[13]&0xFF)<<8) | eth.frame[12] & 0xFF;
  eth.fcs = 0;
  int checkFcs = fcs(eth.frame, 14+eth.length);
  for (int i = 0; i<4; i++) {
    eth.fcs |= (eth.frame[14+eth.length+i] << (i*8));
  }
  if (eth.fcs != checkFcs) {
    return true;
  } 
  for (int i = 0; i<6; i++) {
    eth.source[i] = (eth.frame[i]);  
    eth.destiny[i] = (eth.frame[i+6]);  
  }
  
  for (int i = 0; i<eth.length; i++) {
    eth.data[i] = eth.frame[i+14];
  }
  return false;

}

void convertMacAddressToByteArray(char strMac[], BYTE mac[]) {
  sscanf(strMac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}
void printMacAddress(BYTE* mac) {
  printf("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool compareMacAddress(BYTE* mac1, BYTE* mac2){
	for (int i=0;i<6;i++){
		if(mac1[i]!=mac2[i]){
			return false;
		}
	}

  return true;
}

int fcs(BYTE* arr, int len){
  int sumBits = 0;
  for(int i = 0; i<len; i++){
    for(int nbit = 0; nbit<8; nbit++){
      sumBits += (arr[i] >> nbit) & 0x01;
    }
  }
  return sumBits;
}
