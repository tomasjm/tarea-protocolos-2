#include "slip.h"
#include <stdio.h>

void empaquetaSlip(BYTE *dst, BYTE *src, int len){
  int j = 0;
  dst[j] = 0xC0;
  j++;
  for(int i = 0; i<len; i++){
    if(src[i] == 0xC0){
      dst[j] = 0xDB;
      j++;
      dst[j] = 0xDC;
      j++;
    }else if(src[i] == 0xDB){
      dst[j] = 0xDB;
      j++;
      dst[j] = 0xDD;
      j++;
    }else{
      dst[j] = src[i]; j++;
    }
  }
  dst[j] = 0xC0;
}

int desempaquetaSlip(BYTE *dst, BYTE * src){
  int i = 0, j=0;
  while(src[i] != 0xC0)
    i++;
  i++;
  while(src[i] != 0xC0){
    if(src[i] == 0xDB){
      i++;
      if(src[i] == 0xDC){
        dst[j] = 0xC0;
        j++;
      }else if(src[i] == 0xDD){
        dst[j] = 0xDB;
        j++;
      }
    }else{

       dst[j] = src[i];j++;
    }
    i++;
  }
  return j;
}
