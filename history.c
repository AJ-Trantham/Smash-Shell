//------------------------------------------------------------------------
//
// NAME
// history.c - history functionality for smash shell
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

// required libraries
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>   //Standard library (includes exit function)
#include <string.h>  //String manipulation functions
#include "history.h"

//---------------------------------------------------------------------------

int size;

//Define external variables
static struct Cmd* headNode = NULL;
static struct Cmd* tailNode = NULL;

//[Re-]Init the history module
void init_history(void) {
   headNode = NULL;
   tailNode = NULL;
   size = 0;
}

// adds an entry to the linked list that recors the history
void add_history(char *cmd, int exitStatus, int sequenceNumber){
  //configue the next node holding the given cmd and exit status
  struct Cmd *newCmd = malloc(sizeof(struct Cmd));

  char* cmdDup = strdup(cmd);
  newCmd->cmd = cmdDup;
  newCmd->exitStatus = exitStatus;
  newCmd->next = NULL;
  newCmd->sequenceNumber = sequenceNumber;

  // 1st history record
  if(size == 0){
    headNode = newCmd;
    tailNode = newCmd;
    //printf("size 0");
  }

  //tail will always point to the new node we are making, then update tail
  else{
    tailNode->next = newCmd;
    tailNode = newCmd;
  }

  size++;

  // limit size to 10, perform remove from front opperation
  if(size > 10){
    struct Cmd *temp = headNode;
    headNode = headNode->next;
    free(temp->cmd);
    free(temp); 
    size--;
  }
  newCmd = NULL;
}

// frees up all the memory
void clear_history(void){
  struct Cmd *current = headNode;
  struct Cmd* temp;
  while(current != NULL){
    temp = current;
    current = current->next;
    free(temp->cmd);
    free(temp);
  }
}

// Prints out the history list
void print_history(void){
  struct Cmd *current = headNode;
  while(current != NULL){
    printf("%d", current->sequenceNumber);
    printf(" [");
    printf("%d",current->exitStatus);
    printf("] ");
    printf("%s ",current->cmd);
    current = current->next;
    printf("\n");
  }
  free(current);
}
