//------------------------------------------------------------------------
//
// NAME
//  smash --a shell
//
// SYNOPSIS
//
//
// DESCRIPTION
//
//
// ERRORS
//  Prints usage message and exits abnormally for invalid commands.  Prints an
//  error message and exits abnormally for other issues.
//
// LIMITATIONS
//  Lines of text are limited to a maximum of 4096 chars.
//
// AUTHORS
//  AJ Trantham
//
//-----------------------------------------------------------------------------
#define _BSD_SOURCE
// required libraries
#include <stdio.h>
#include <stdlib.h>   //Standard library (includes exit function)
#include <string.h>  //String manipulation functions

#include "smash.h"
#include "commands.c"
//#include "history.c"
#define MAXLINE 4096

//---------------------------------------------------------------------------

int main(int argc, char **argv){

  init_history();
  char bfr[MAXLINE];

  fputs("$ ", stdout);
  while(fgets(bfr, MAXLINE, stdin) != NULL){


    //Check for input that exceeds the expected length
    if(strlen(bfr) == 4096 && bfr[4095] != '\n'){
      fputs("Input is too long", stderr);
      exit(-1);
    }

    bfr[strlen(bfr) - 1] = '\0'; //replace the last character with a NUL



    // Also check for no input, don't want to give it a new line unecessarily
    if(strlen(bfr) > 1){executeCommand(bfr);}
    fputs("$ ", stderr);
  }

  printf("\n");

  return 0;
}
