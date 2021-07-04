#ifndef ETHERNET_H
#define ETHERNET_H
#include <string.h>
#define BYTE unsigned char
#define N_DATA 129
struct Ethernet {
  BYTE source[6];
  BYTE destiny[6];
  int length;
  BYTE data[N_DATA];
  int fcs;
  BYTE frame[N_DATA+6+6+2+4];
  // size 6+6+2+129+4=147
};

void packEthernet(Ethernet &eth);
bool unpackEthernet(Ethernet &eth);
void convertMacAddressToByteArray(char strMac[18], BYTE mac[]);
void printMacAddress(BYTE* mac);
bool compareMacAddress(BYTE* mac1,BYTE* mac2);
int fcs(BYTE* arr, int len);
#endif