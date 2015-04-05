// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include "alloc.h"


#ifdef __APPLE__
#include <err.h>
#define error(args...) errc(args)
#else
#include <error.h>
#endif

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

//additional includes
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//end of additional includes

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */


//global variable
int line_number;

//auxiliary functions
bool isWord (char c)
{
    return (isalpha(c) || isdigit(c) || (c == '!') || (c == '%') || (c == '+') || (c == ',') ||
            (c == '-') || (c == '.') || (c == '/') || (c == ':') || (c == '@') || (c == '^') || (c == '_') );
    
}

bool isSpecial (char c)
{
    return ( (c == ';') || (c == '|') || (c == '(') || (c == ')') || (c == '<') || (c == '>') );
}

bool isNotValid (char c)
{
    return ( (c!=' ') && (c!='#') && (c!='\n') && (!isWord(c)) && (!isSpecial(c)) );
}
//end of auxiliary functions

//initialize command
command_t init_command(enum command_type type)
{
    command_t command=(command_t)checked_malloc(sizeof(struct command));
    command->type=type;
    command->status=-1;
    command->input=NULL;
    command->output=NULL;
    return command;
}


//initialize command stream
command_stream_t init_stream()
{
    command_stream_t stream=(command_stream_t) checked_malloc(sizeof(struct command_stream));
    stream->head=NULL;
    stream->tail=NULL;
    stream->cursor=NULL;
    return stream;
}

//create simple command
command_t store_simple_command ()
{
    command_t command= init_command(SIMPLE_COMMAND);
    return command;
}

//create subshell command
command_t store_subshell_command()
{
    command_t command= init_command(SUBSHELL_COMMAND);
    return command;
}

//create compound command
command_t store_compound_command()
{
    return NULL;
}

//create the tree. operator stack and command stack

//precedance
int precedence (char *operator)
{
 /*
  lowest --> highest
  ; \n    1
  &&/||   2
  |       3
*/
    if (strcmp(operator,";")==0)
        return 1;
    else if (strcmp(operator,"\n")==0)
        return 1;
    else if (strcmp(operator,"&&")==0)
        return 2;
    else if (strcmp(operator,"||")==0)
        return 2;
    else if (strcmp(operator,"|")==0)
        return 3;
    else
        return -1;
}

char *get_next_word (char *c, int *i, const int size)
{
    while ( ((c[*i]==' ') || (c[*i]=='\t') || (c[*i]=='#')) && (*c<size) )
    {
        if (c[*i]=='#')
        {
            while (*c<size)
            {
                if (c[*i]=='\n')
                {
                    line_number++;
                    break;
                }
                (*i)++;
            }
        }
        (*i)++;
    }
    
    int skipped=*i;
    
    while (isWord(c[*i]) && *i<size)
        (*i)++;
    int size_word=(*i)-skipped;
    
    if (size_word == 0)
        return NULL;
    char *word=(char *)malloc(sizeof(char)*(size_word+1) );
    
    int t=0;
    while (t<size_word)
    {
        word[t]=c[skipped+t];
        t++;
    }
    word[size_word]='\0';
    
    if (c[*i]=='#' || c[*i]=='<' || c[*i]=='>' || c[*i]=='\n' || isSpecial(c[*i]) )
        (*i)--;
    
    return word;
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
    
command_t
read_command_stream (command_stream_t s)
{
    if (s->cursor==NULL)
    {
        return NULL;
    }
    command_t tmp=s->cursor->c;
    s->cursor=s->cursor->next;
    return tmp;
    
  
}
