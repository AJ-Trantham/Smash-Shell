//-----------------------------------------------------------------------------
//
// NAME
//  commands.c -- Implements the excecuteCommand function
//             -- Exceutes internal and external commands
//
// Author
// AJ Trantham
//-----------------------------------------------------------------------------
#define _BSD_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "history.c"



static int sequenceNumber = 0;    //number of command recieved, used for history
char **toks;
char* strDup;


/*Function to exectute external commands*/
int excecuteExternalCommand(char **argv){
  int adjustedExitStatus = 0;
  fflush(stdout);

  // fork a new process
  int pid = fork();

  //Child Process
  if(pid == 0) {
    execvp(argv[0],argv); //Execute the external command
    //execvp exits if sucessful. This will only execute if execvp returns which only happens when ther is an error
    perror(argv[0]);      //Execvp failed -- print the error message
    free(toks);
    free(strDup);
    clear_history();
    fclose(stdout);
    exit(127); //Report failure to parent process
  }

  //Error Forking
  else if(pid < 0) {
    perror("fork failed\n");
  }

  // Parent Process
  else {
    int exitStatus;
    int pid = wait(&exitStatus);  //Wait for child to exit and retrieve its status
    if(pid == 0){}
    adjustedExitStatus= WEXITSTATUS(exitStatus);  //retrieves child's exit status
  }
  return adjustedExitStatus;
}

void executeCommand(char *str){
     strDup = strdup(str); //change
    char* token = strtok(str, " ");
    char* cmd;
    int exitStatus = 0;

    // allocate space for the toks array of pointers
    toks = malloc(4096);

    // build the array of tokens, toks
    int i = 0;
    while(token != NULL){
      toks[i++] = token;
      token = strtok(NULL, " ");
    }

    cmd = toks[0];
    sequenceNumber++;

    // the exit command: exit the utility
    if(strcmp(cmd,"exit") == 0 && i < 2) {
      free(toks);
      free(strDup);//change
      clear_history();
      exit(0);
    }

    else if(strcmp(cmd,"cd") == 0 && i == 2 ){
      //printf("%s\n", getcwd(cwd, 100));

      // dir does not exist
      if(chdir(toks[1]) != 0){
        perror(toks[1]);
        // reset the value of cmd here
        cmd = "no-such-diectory";
        exitStatus = 1;
      }

    }

    // history cmd
    else if(strcmp(cmd, "history") == 0 && i < 2) {
      print_history();
    }

    //externalCommand
    else {
      //fputs("ExternalCommand\n",stderr);
      char* cmdPointers[4096];
      for(int j = 0; j < i; j++){
        cmdPointers[j] = toks[j];
      }
      // set the last value to null, required for execvp
      cmdPointers[i] = NULL;

      // call external command and return exit status
      int externalExitStatus = excecuteExternalCommand(cmdPointers);
      exitStatus = externalExitStatus;

      if(exitStatus == 127 || exitStatus < 0){
        cmd = "no-such-command";
      }
	
	//for(int j = 0; j < i; j++){
       // free(cmdPointers[j]);
    	  //}
    
  }
    free(toks);
    add_history(strDup, exitStatus, sequenceNumber);
    free(strDup); //free strdup in mem change
}
