#ifndef SLIP_H
#define SLIP_H
#define BYTE unsigned char
void empaquetaSlip(BYTE* dst, BYTE *src, int len);
int desempaquetaSlip(BYTE* dst, BYTE *src);
#endif
