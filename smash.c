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

#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>



#include "smash.h"
#include "commands.c"
//#include "history.c"
#define MAXLINE 4096

//-----------------------Global Variables -------------------------------------
char **tokens;
//char* strDuplicate;

//------------------------Structs---------------------------------------------
struct pipe{
  int inlet;
  int outlet;
};




//------------------------------MAIN-----------------------------------
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
    if(strlen(bfr) >= 1){

      int numPipes = 0;
      int numCmds = 0;

      char* token = strtok(bfr, "|");

      tokens = malloc(4096); //allocate space for the array of tokens

      // build the array of tokens
      int i = 0;
      while(token != NULL){
        tokens[i++] = token;
        token = strtok(NULL, "|");
      }

      numCmds = i;
      numPipes = i -1;

      //pipes are present
      if(numPipes > 0){

        // array to hold all struct of pipes
        struct pipe pipeArr[numPipes];

        //create all the necessary numPipes
        for(int j = 0; j < numPipes; j++){
          int pipeFileDescriptors[2];

          //Create the pipe
          if (pipe(pipeFileDescriptors) < 0) {
            perror("Error creating pipe");
            exit(-1);
          }

          pipeArr[j].inlet = pipeFileDescriptors[1]; //the inlet
          pipeArr[j].outlet = pipeFileDescriptors[0]; // theoutlet
        }

        // for each cmd
        int cmdsExcecuted = 0;
        int producerDone = 0;
        int conDone = 0;
        while(cmdsExcecuted < numCmds){
          setbuf(stdout,NULL);
          int i = cmdsExcecuted;
          //printf("%d\n",i);

          //Fork a child process
          int child = fork();
          if (child<0) {
            perror("Fork child1 failed\n");
            exit(-1);
          }

          //if child and Producer
          if(child == 0 && cmdsExcecuted == 0){
            printf("I am producer child\n");

            //close(1);                        //Close stdout inherited from parent
            dup2(pipeArr[i].inlet,1);  //Child1's stdout must write to pipe

            //Close extraneous file descriptors in child1
            close(pipeArr[i].outlet);
            close(pipeArr[i].inlet);   //Note... stdout remains open to pipe

            executeCommand(tokens[i]);
            producerDone = 1;

            exit(0);
          }



          // if child and P/case
          else if(child == 0 && cmdsExcecuted != (numCmds - 1)){
              printf("I am P/C child\n");
              dup2(pipeArr[i].inlet,1);  // stdout writes to next pipe
              dup2(pipeArr[i-1].outlet,0);     //stdin reads from previous pipe

              //Close extraneous file descriptors in child1
              close(pipeArr[i].outlet);
              close(pipeArr[i].inlet);
              close(pipeArr[i-1].outlet);      //Note... stdin remains open to pipe
              close(pipeArr[i-1].inlet);      //

              executeCommand(tokens[i]);

              exit(0);
          }

          //if child and Consumer
          else if (child == 0 && cmdsExcecuted == (numCmds - 1)){
            printf("I am Consumer child\n");
            //close(0);                           //Close stdin inherited from bash
            dup2(pipeArr[i-1].outlet,0);     //Child3's stdin must read from pipe

            //Close extraneous file descriptors in child2
            close(pipeArr[i-1].outlet);      //Note... stdin remains open to pipe
            close(pipeArr[i-1].inlet);      //

            executeCommand(tokens[i]);
            conDone = 1;
            exit(0);
          }

          // needed
          if(cmdsExcecuted == 0){
            printf("producer done");
            close(pipeArr[i].inlet);
          }

          if(cmdsExcecuted == numCmds - 1){
            printf("Consumer Done");
            close(pipeArr[i-1].outlet);
            close(pipeArr[i - 1].inlet);
          }

          // Parentwait for children
          int exitStatus, pid;
          pid=wait(&exitStatus);
          printf("PID %5d exited with %d\n",pid,exitStatus);

          cmdsExcecuted++;
        }
      }

      //no pipes, proceed with single cmd
      else{
        executeCommand(bfr);
      }
    }
    fputs("$ ", stderr);
  }

  printf("\n");
  free(tokens); // don't free until very end here, let array override each time throught the loop
  return 0;
}
