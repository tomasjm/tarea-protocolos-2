#ifndef PROTOCOL_H
#define PROTOCOL_H

#define BYTE unsigned char
#define N 127
struct Frame {
  // 2 bytes
  int length; // 7 bits
  int cmd; // 4 bits
  int sa; // 5 bits
  // 127 bytes
  BYTE data[N]; // 127 bytes
  // total 129 bytes
  BYTE frame[N+2]; // 127 + 2 bytes
};

void generateFrameToSend(Frame &f);

void generateReceivedFrame(Frame &f);


#endif
