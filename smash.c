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

//Threads
#include <pthread.h>
#include <sys/time.h>

#include "smash.h"
#include "commands.c"
//#include "history.c"
#define MAXLINE 4096

//-----------------------Global Variables -------------------------------------

int sigDetected = 0;
//start thread that checks for child exit exitStatus
int stop = 0;



//------------------------Structs---------------------------------------------
struct pipe{
  int inlet;
  int outlet;
};

// --------------------Signal Handler--------------------------------
void myHandler(int sigNumber) {
  //write(STDERR_FILENO, "", 0);
  sigDetected = 1;
  fputs("\n$ ", stderr);
}



//------------------------------MAIN-----------------------------------
int main(int argc, char **argv){


  signal(SIGINT, myHandler);
  init_history();
  char bfr[MAXLINE];

  fputs("$ ", stdout);
  //outer loop that processess a line of user input
  while(fgets(bfr, MAXLINE, stdin) != NULL){

    threadCreated = 0;
    //Check for input that exceeds the expected length
    if(strlen(bfr) == 4096 && bfr[4095] != '\n'){
      fputs("Input is too long", stderr);
      exit(-1);
    }

    bfr[strlen(bfr) - 1] = '\0'; //replace the last character with a NUL





    // Also check for no input, don't want to give it a new line unecessarily
    if(strlen(bfr) >= 1){


      //Check for pipes
      int numPipes = 0;
      int numCmds = 0;
      historyEntry = strdup(bfr);

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

        sequenceNumber ++; //"increment" sequence for history, why not on its own?? This fixed
        int exitStatus;
        int adjustedExitStatus = 0;

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
        while(cmdsExcecuted < numCmds){

          setbuf(stdout,NULL);
          int i = cmdsExcecuted;

          //Fork a child process
          int child = fork();
          if (child<0) {
            perror("Fork child1 failed\n");
            exit(-1);
          }

          //if child and Producer
          if(child == 0 && cmdsExcecuted == 0){

            dup2(pipeArr[i].inlet,1);  //Child1's stdout must write to pipe

            //Close extraneous file descriptors in child1
            close(pipeArr[i].outlet);
            close(pipeArr[i].inlet);   //Note... stdout remains open to pipe

            int res = executeCommand(tokens[i]);
	    free(tokens);
            free(historyEntry);
	    clear_history();
            exit(res);
          }

          // if child and Producer/Consumer Pipe
          else if(child == 0 && cmdsExcecuted != (numCmds - 1)){
              dup2(pipeArr[i].inlet,1);  // stdout writes to next pipe
              dup2(pipeArr[i-1].outlet,0);     //stdin reads from previous pipe

              //Close extraneous file descriptors in child1
              close(pipeArr[i].outlet);
              close(pipeArr[i].inlet);
              close(pipeArr[i-1].outlet);
              close(pipeArr[i-1].inlet);

              //ececute the cmd for this child
              int res = executeCommand(tokens[i]);
	      free(tokens);
	      free(historyEntry);
	      clear_history();
              exit(res);
          }

          //if child and Consumer
          else if (child == 0 && cmdsExcecuted == (numCmds - 1)){

            dup2(pipeArr[i-1].outlet,0);     //Child3's stdin must read from pipe

            //Close extraneous file descriptors in child2
            close(pipeArr[i-1].outlet);      //Note... stdin remains open to pipe
            close(pipeArr[i-1].inlet);      //

            //ececute the cmd for this child
            int res = executeCommand(tokens[i]);
	    free(tokens);
	    //free(strDup);
	    clear_history();
	    free(historyEntry);
            
	    exit(res);
          }

          //when producer is finished,
          //Parent no longer needs the producer side of the pipe and must close
          //it or the consumer process (child3) will never reach EOF when reading.
          if(cmdsExcecuted == 0){
            close(pipeArr[i].inlet);
          }

          //Parent no longer needs the consumer or producer side of the pipe
          if(cmdsExcecuted == numCmds - 1){
            close(pipeArr[i-1].outlet);
            close(pipeArr[i - 1].inlet);
          }

          // Parent wait for children
           //,pid; pid = wait...
          int pid = wait(&exitStatus);
          adjustedExitStatus = WEXITSTATUS(exitStatus);
          //printf("PID %5d exited with %d\n",pid,exitStatus);
          cmdsExcecuted++;

          //int values[2];
          values[0] = pid;
          values[1] = adjustedExitStatus;

          threadCreated = 1;
          //Create and start the new thread with default (NULL) attributes
          result = pthread_create(&posixThreadId, NULL, theThread, &values);
          if (result!=0) printf("pthread_create failed, error=%d\n",result);
        }

        //add entire cmd to add_history
        add_history(historyEntry, adjustedExitStatus, sequenceNumber);
        //free(tokens);
        //free(historyEntry);
        //free(strDup);
        //free(toks);
      }

      //no pipes, proceed with single cmd
      else{
	// if(strcmp(bfr,"exit") == 0){
		//pthread_detach(posixThreadId);
	//	result = pthread_create(&posixThreadId, NULL, theThread, &values);
          //if (result!=0) printf("pthread_create failed, error=%d\n",result);
	//}
         executeCommand(bfr);
      }

      //free(tokens);
      //free(historyEntry);

      //Wait for the child thread to exit
      if(threadCreated == 1){
        result = pthread_join(posixThreadId, NULL);
        if (result!=0) printf("pthread_join failed, error=%d\n",result);
        threadCreated = 0;
      }
      free(tokens);
      free(historyEntry);
    }
    //check if $ needs to be printed
    if(sigDetected == 0){
      fputs("$ ", stderr);
    }
    sigDetected = 0;
    //free(tokens);
    //free(historyEntry);
  }

  printf("\n");
  //This code will never get excecuted
  //free(tokens);// don't free until very end here, let array override each time throught the loop
  return 0;
}
