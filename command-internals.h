// UCLA CS 111 Lab 1 command internals
#include<unistd.h>
#include<sys/types.h>

enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
  };

//each command/operator/others as token
//precedence low to high
/*
 
 lowest --> highest
 ; \n
 &&/||
 |

 */
enum token
{
    SEMICOLON,
    NEWLINE,
    AND,
    OR,
    PIPE,
    LEFTPAREN,
    RIGHTPAREN,
    LESSTHAN,
    GREATERTHAN,
    WORD
};

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};



//added


//each node of the command tree
struct command_node
{
    struct command *c;
    struct command_node *next;

};

//linked list of the command tree
struct command_stream
{
    struct command_node* head;
    struct command_node* tail;
    struct command_node* cursor;
};
/*********time travel structs*****/

struct graph_node
{
    command_t command;
    struct graph_node** before;
    pid_t pid;
    //
    int size_before_list;//
    
};

struct queue_node
{
    struct graph_node *g;
    struct queue_node *next;
};

struct queue //linked list of queue node
{
    struct queue_node* head;
    struct queue_node* cursor;
    struct queue_node* tail;
};

//create a list node {graph node, RL, WL}? where should i put RL, WL?

struct dependency_graph
{
    //need to implement a queue ourself first . linked list of graph node
    
    struct queue* no_dependency;
    struct queue* dependency;
};

// linked list to save wl rl
struct list_node{ // to save graphnode wl rl for each command tree
    
    struct graph_node *graph_node;
    char ** read_list;
    char ** write_list;
    struct list_node* next;
    
};
struct list_stream{
    struct list_node* head;
    struct list_node* cursor;
    struct list_node* tail;
};
