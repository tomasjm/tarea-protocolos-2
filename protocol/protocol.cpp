#include "protocol.h"
#include <stdio.h>
void generateFrameToSend(Frame &f) {
  f.frame[0] = (f.length & 0x7F) | (f.cmd & 0x1) << 7;
  f.frame[1] = (f.cmd & 0xE) >> 1 | (f.sa & 0x1F) << 3;
  for (int i = 0; i<f.length; i++) {
    f.frame[2+i] = f.data[i];
  }
}

void generateReceivedFrame(Frame &f) {
  f.length = (f.frame[0] & 0x7F);
  f.cmd = (f.frame[0]&0x80) >> 7 | (f.frame[1]&0x7)<<1;
  f.sa = (f.frame[1]&0xF8)>>3;
  for (int i = 0; i<f.length; i++) {
    f.data[i] = f.frame[2+i];
  }
}

