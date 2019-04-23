// header file for the history functionality
// author: AJ Trantham

//Define the layout of a single entry in the history array
struct Cmd{
  struct Cmd *next;
  char* cmd;        //A saved copy of the user's command string
  int exitStatus;   //The exit sstatus from the user's cmd
  int sequenceNumber; //represents the id number of a cmd
};

//Define the max number of history records
#define MAXHISTORY 10 //Smash will save the history of the last ten cmds

// Function prototypes for the gistory functionality
void init_history(void);                      //Builds data structure for recording the cmd init_history
void add_history(char *cmd, int exitStatus, int SequenceNumber);  //Adds an entry to the add_history
void clear_history(void);                     // Frees all malloc'd memory
void print_history(void);  //Prints the history to stdout
