#include <stdio.h>
#define BYTE unsigned char

//prototipo fcs
int fcs(BYTE* arr, int len);

int main(){
  BYTE arr[10] = {255,0,7,1,0,0x0F,0,1,0,0};
  int fcs_ = fcs(arr, 10);

  printf("Sumatoria de bits: %d\n", fcs_);
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
