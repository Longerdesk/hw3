// A header file for helpers.c
// Declare any additional functions in this file
#include <signal.h>
#include "icssh.h"



extern volatile sig_atomic_t condflag;
extern volatile sig_atomic_t usr2flag;
int compareBG(const void* data1, const void* data2);
void printBG(void* data1, void* data2);
void deleteBG(void* data);

void sigchld_handler();

void sigusr2_handler();
