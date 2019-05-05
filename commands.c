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
//NEED TO INCLUDE WHATEVER IS NEEDED FOR OPEN API
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>





int sequenceNumber = 0;    //number of command recieved, used for history
char **toks;
char* strDup;

char **tokens; //similar to comment below
char *historyEntry; //used by smash to add the complete history cmd, needs to be freed here

pthread_t posixThreadId;
int result;
int threadCreated;
int threadExitStatus;
void *pThreadExitStatus = &threadExitStatus;
void *theThread(void *arg);

//--------------------------Thread Function--------------------------
/**
 ** This is where the child thread starts executing
 */
void *theThread(void *arg) {

  int (*values)[2]= (int (*)[2])arg;
  //Print the process pid and exit status
  fputs("PID ", stderr);
  fprintf(stderr, "%d", *values[0]);
  fputs(" exited, ",stderr);
  fputs("status = ", stderr);
  fprintf(stderr, "%d\n", *values[1]);
  return NULL;
}

/*Finds the arg in the arg array*/
int findIn(char ** argv, int argvLen){
  //int i = 0;
  int argIndex = -1;
  for(int i = 0; i < argvLen -1; i++){
    //printf(argv[i]);
    if(strstr(argv[i], "<") != NULL){
      argIndex = i;
    }
  }
  return argIndex;
}

/*Removes the I/O file name since it does not need to be execvped*/
void cleanCmd(char **argv, int startingIndex){
  int i = startingIndex;
  while(argv[i] != NULL){
    argv[i] = argv[i+1];
    i++;
  }
}

/*Finds the arg out the arg array*/
int findOut(char ** argv, int argvLen){
  int argIndex = -1;
  for(int i = 0; i < argvLen -1; i++){
    if(strstr(argv[i], ">") != NULL){
      argIndex = i;
    }
  }
  return argIndex;
}

/*checks if there should be a redirect */
int checkRedirect(char *cmd){
  int IOcase = -1;
  char * inFile;
  char * outFile;
  char *out = strstr(cmd, ">");
  char *in = strstr(cmd, "<");

  //printf("Through assignments \n");
  // redirect both stdin and stdout - NOT GETTING IN HERE INF LOOP
  if(in != NULL && out != NULL){

    //stdout
    outFile = out + 1;
    outFile = strtok(outFile, " ");
    int fd = open(outFile, O_RDWR | O_CREAT, S_IRWXU); // sets stdout to next available fd 1


    //stdin
    int fd2;
    inFile = in + 1;
    inFile = strtok(inFile, " ");
    if((fd2 = open(inFile, O_RDONLY, S_IRWXU)) < 0){
      perror("open error");
      return -1;
    }

    close(1);//close stdout
    dup2(fd, 1); //set stdout to fd
    close(fd); //close fd

    close(0);
    dup2(fd2, 0);
    close(fd2);

    IOcase = 2;
  }

  // redirect stdin case: 1
  else if(in != NULL){

    int fd;
    IOcase = 1;
    inFile = in + 1;
    inFile = strtok(inFile, " ");
    //printf(fileName);
    //int fd = open(fileName, O_RDWR | O_CREAT, S_IRWXU); //Do I want to open a file Here should be an existing file used for stdin
    if((fd = open(inFile, O_RDONLY, S_IRWXU)) < 0){
      perror("open error");
      return -1;
    }
    close(0);
    dup2(fd, 0);
    close(fd);
  }

  // redirect stdout case: 0
  else if(out != NULL){

    IOcase = 0;
    outFile = out + 1;
    outFile = strtok(outFile, " ");
    int fd = open(outFile, O_RDWR | O_CREAT, S_IRWXU); // sets stdout to next available fd 1
    close(1);//close stdout
    dup2(fd, 1); //set stdout to fd
    close(fd); //close fd
  }

  return IOcase;
}

/*Function to exectute external commands*/
int excecuteExternalCommand(char **argv, char *str, int argvLen){
  int adjustedExitStatus = 0;
  int proID = 0; //for threading

    // fork a new process
    int pid = fork();

    //Child Process
    if(pid == 0) {
      fflush(stdout);

      //printf("Starting\n");
      int red = checkRedirect(str);
      //printf("Finished Check Red\n");
      int indexIn = findIn(argv, argvLen); // finds arg to remove before execvp
      if(indexIn != -1){
        //cleanCmd(argv, indexIn);
      }
      int indexOut = findOut(argv, argvLen); // finds arg to remove before execvp
      if(indexOut != -1){
        //cleanCmd(argv, indexOut);
      }

      //TODO: move code to methods or simplify
      //TODO: Will also need to check if indexMethods =! -1 to remove the argv values
      //TODO: Re-organize indexIn remove, indexOur remove as indexes will change when doin both

      //TODO: double redirection is not working


      // double redirection - remove both files from the cmd
      if(red == 2){
        int i = indexOut;
        while(argv[i] != NULL){
          argv[i] = argv[i+1];
          i++;
        }

        int j = indexIn;
        while(argv[j] != NULL){
          argv[j] = argv[j+1];
          i++;
        }
      }

      // out
      else if(red == 0){
        //argv[argvLen - 2] = NULL; // may want to make sure this removes the right arg id redirect is first
        int i = indexOut;
        while(argv[i] != NULL){
          argv[i] = argv[i+1];
          i++;
        }
      }

      //in
      if(red == 1){
        int i = indexIn;
        while(argv[i] != NULL){
          argv[i] = argv[i+1];
          i++;
        }
      }


      //Do the history cmd, and check for I/O
      if(strcmp(argv[0], "history") == 0) {
        print_history();
        exit(0);
      }

      else{
        execvp(argv[0],argv); //Execute the external command
        //execvp exits if sucessful. This will only execute if execvp returns which only happens when ther is an error
        perror(argv[0]);      //Execvp failed -- print the error message
        free(toks);
        free(strDup);
        free(tokens);
        free(historyEntry);
        clear_history();
        fclose(stdout);
        exit(127); //Report failure to parent process
      }
    }

    //Error Forking
    else if(pid < 0) {
      perror("fork failed\n");
    }

    // Parent Process
    else {
      int exitStatus;
      proID = wait(&exitStatus);  //Wait for child to exit and retrieve its status

      adjustedExitStatus= WEXITSTATUS(exitStatus); //retrieves child's exit status
    }

    int values[2];
    values[0] = proID;
    values[1] = adjustedExitStatus;

    threadCreated = 1;
    //Create and start the new thread with default (NULL) attributes
    result = pthread_create(&posixThreadId, NULL, theThread, &values);
    if (result!=0) printf("pthread_create failed, error=%d\n",result);

    //Wait for the child thread to exit
    if(threadCreated == 1){
      result = pthread_join(posixThreadId, NULL);
      free(toks);
      free(historyEntry);
      free(tokens);
      free(strDup);
      clear_history();
      if (result!=0) printf("pthread_join failed, error=%d\n",result);
      threadCreated = 0;
    }
    return adjustedExitStatus;
}

/*Excecutes a command*/
int executeCommand(char *str){
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

    //  checkRedirect(strDup);

    // the exit command: exit the utility
    if(strcmp(cmd,"exit") == 0 && i < 2) {
      //free all memory before exiting
      free(toks);
      free(historyEntry);
      free(tokens);
      free(strDup);
      clear_history();
      exit(0);
    }

    //Should I take in whole cmd, would need to for history?

    // If pipes are present maybe call a whole new function, that function could then call excecuteComds
    // to do the individual commands where output is just changed with the file descriptors.
    //
    // Need to use str... cmd to check if a pipe or redirect exists and then call an additional command to handle it

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
      int externalExitStatus = excecuteExternalCommand(cmdPointers, strDup, i+1);
      exitStatus = externalExitStatus;

      // I don't think this does anything...
      if(exitStatus == 127 || exitStatus < 0){
        cmd = "no-such-command";
      }

  }
    free(toks);
    add_history(strDup, exitStatus, sequenceNumber);
    free(strDup); //free strdup in mem change
    return exitStatus;
}
