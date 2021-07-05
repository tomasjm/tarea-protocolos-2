#ifndef SLIP_H
#define SLIP_H
#define BYTE unsigned char
void packSlip(BYTE* dst, BYTE *src, int len);
int unpackSlip(BYTE* dst, BYTE *src);
#endif
