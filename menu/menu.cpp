#include <stdio.h>
#include <stdlib.h>
#include "menu.h"

void clearScreen() {
  system("clear");
}

void printMenu() {
  clearScreen();
  printf("Main menu\n");
  printf("Please select an option.\n");
  printf("1. Get a value of temperature with it timestamp and send it \n");
  printf("2. Send a string message of 30 characters\n");
  printf("3. Close program (Self)\n");
}

void getOptionAndValidate(int *optPtr) {
  scanf("%d", optPtr);
  while(*optPtr < 1 || *optPtr > 3) {
    printMenu();
    printf("Please select a valid option...\n");
    getOptionAndValidate(optPtr);
  }
}
