#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ethernet/ethernet.h"
#include "protocol/protocol.h"
#include "slip/slip.h"

char MAC_ADDRESS[18]="b8:27:eb:15:b1:ca";

void generateRawFrame(Frame &f, int cmd, int sa, int length, BYTE data[]) {
  f.cmd = cmd;
  f.sa = sa;
  f.length = length;
  memcpy(f.data, data, length);
  generateFrameToSend(f);
}

void generateRawEthernet(Ethernet &ethf, Frame &f, char strMacOrigen[18], char strMacDestino[18]) {
  BYTE macOrigen[6], macDestino[6];
  convertMacAddressToByteArray(strMacOrigen, macOrigen);
  convertMacAddressToByteArray(strMacDestino, macDestino);
  memcpy(ethf.origen, macOrigen, sizeof(ethf.origen));
  memcpy(ethf.destino, macDestino, sizeof(ethf.destino));
  ethf.largo = sizeof(f.frame);
  memcpy(ethf.data, f.frame, sizeof(ethf.data));
  ethf.fcs = 37;
  packEthernet(ethf);
}


int main(int argc, char* args[]) {
  Ethernet eth, ethAux;
  Frame f, auxF;

  // empaquetar
  // frame
  int dato = 77;
  BYTE dataArr[sizeof(dato)];
  getByteArrayOfInteger(dato, dataArr);

  generateRawFrame(f, 4, 7, sizeof(dataArr), dataArr);
  printf("Raw frame info: %d %d %d %d\n", f.cmd, f.sa, f.length, sizeof(f.frame));

  // ethernet
  generateRawEthernet(eth, f, MAC_ADDRESS, MAC_ADDRESS);
  printf("Raw ethernet info: %d %d %d\n", eth.largo, eth.fcs, sizeof(eth.frame));
  // slip
  BYTE slipArr[2+sizeof(eth.frame)];
  empaquetaSlip(slipArr, eth.frame, sizeof(eth.frame));

  // se genera el envio
  BYTE rSlipArr[2+sizeof(eth.frame)];
   
  // desempaquetar

  //slip
  desempaquetaSlip(ethAux.frame, rSlipArr);

  // ethernet
  unpackEthernet(ethAux);
  printf("Raw ethernetAux info: %d %d %d\n", ethAux.largo, ethAux.fcs, sizeof(ethAux.frame));
  memcpy(auxF.frame, ethAux.data, sizeof(auxF.frame));

  // frame
  generateReceivedFrame(auxF);
  printf("Raw frameAux info: %d %d %d %d\n", auxF.cmd, auxF.sa, auxF.length, sizeof(auxF.frame));
  int auxDato = 0;
  getIntegerOfByteArray(auxF.data, &auxDato);
  printf("DATO %d\n", auxDato);
  printMacAddress(ethAux.destino);
  printMacAddress(ethAux.origen);
  return 0;
}
