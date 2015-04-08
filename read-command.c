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
const int  OPERATOR_LEN =16;
const int  COMMAND_LEN =16;
const int WORD_LEN =16;

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

//auxiliary functions declarations
bool isWord (char c);
bool isSpecial (char c);
bool isNotValid (char c);
enum command_type cmd_type(char *cc);
command_t init_command(enum command_type type);
command_stream_t init_stream();
command_t store_simple_command (char *c, int *i, size_t size);
int precedence (char *operator);
char* get_next_word (char *c, int *i, size_t size);
void init_op_stack();
void init_cmd_stack();


//global variable
int line_number =0;
bool wait_input =false; //sign for <
bool wait_output =false; // >
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
enum command_type cmd_type(char *cc)
{
    if(!strcmp(cc,"&&")) return AND_COMMAND;
    if(!strcmp(cc,"||")) return OR_COMMAND;
    if(!strcmp(cc,"|")) return PIPE_COMMAND;
    if(!strcmp(cc,";")) return SEQUENCE_COMMAND;
    if(!strcmp(cc,"(")) return SUBSHELL_COMMAND;
    else {
        err(1, "invalid type");
        return SIMPLE_COMMAND;
    }
    
}
command_t init_command(enum command_type type)
{
    command_t command=(command_t)checked_malloc(sizeof(struct command));
    command->type=type;
    command->status=-1;
    command->input=NULL;
    command->output=NULL;
    command->u.word=NULL;
    command->u.command[0]=NULL;
    command->u.command[1]=NULL;
    command->u.subshell_command=NULL;

    /* put this inside build command tree
    if(type == SUBSHELL_COMMAND)
    {
        command->u.subshell_command= command_t checked_malloc(sizeof(struct command));
    }
*/
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

char* get_next_wordd (char *c, int *i, size_t size  )
{
    int starting=*i;
    int size_word=0;
    while (isWord(c[*i]) && (*i)<size)
    {
        size_word++;
        (*i)++;
    }
    char *word=(char *)checked_malloc(sizeof(char)*(size_word+1) );
    if (size_word == 0)
        return NULL;
    int j=0;
    while (j<size_word)
    {
        word[j]=c[starting+j];
        j++;
    }
    word[size_word]='\0';
    return word;
}

//create simple command (consists of one or more words)
command_t store_simple_command (char *c, int *i, size_t size ) // to avoid similar name with build-in function
{
    command_t command= init_command(SIMPLE_COMMAND);
    int size_word=1;
    int count_w=0;
    command->u.word=(char**)malloc(sizeof(char*)*size_word);
    
   // store words instead of store pointers
    for(int j=0;j<size_word;j++) // j instead of i
        command->u.word[j]=(char*)malloc(sizeof(char)*size);
 
    
    while ((*i)<size)
    {
        char tmp_c=c[*i];
        //if meet operator, then return command
        if (tmp_c==' ' || tmp_c=='\t')
        {
            (*i)++;
        }
        else if (isWord(tmp_c))
        {
            char* w= get_next_word(c, i, size);
            if (count_w==size_word)
            {
                size_word=size_word*2;
                command->u.word=realloc(command->u.word,sizeof(char*)*size_word);
            }
            command->u.word[count_w]=w;//entire word
            count_w++;
            command->u.word[count_w]=NULL;
        }
        else if (isNotValid(tmp_c))
        {
            fprintf(stderr,"line %d: character, '%c', is not valid. \n", line_number, tmp_c );
            exit(1);
        }
        else//meet an operator of any sort or others ...i.e. \n, < > #
        {
            (*i)--;//can deal with it later in make_comamnd_tree function
            break;
        }
    }
    return command;
}


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
//***********************************************************************************


//build command tree


//stack def
struct op_stack{
    char** operator;
    int len;
    int top;
    
}op_s; // global var

void init_op_stack()
{
    
    op_s.len=OPERATOR_LEN;
    op_s.top=0;
    op_s.operator=(char**)checked_malloc(op_s.len*sizeof(char*));
    for(int i=0;i<op_s.len;i++)
    {
        op_s.operator[i]=(char*)checked_malloc(OPERATOR_LEN*sizeof(char));
    }

}

struct cmd_stack{
    command_t* command;
    int len;
    int top;
}cmd_s;
void init_cmd_stack(){
    cmd_s.len=COMMAND_LEN;
    cmd_s.top=0;
    cmd_s.command=(command_t*)checked_malloc(cmd_s.len * sizeof(command_t));
    for (int i=0 ; i<cmd_s.len; i++) {
        cmd_s.command[i]=(command_t)checked_malloc(sizeof(struct command));
    }
}


void push_op(char* cc)
{
    if (op_s.len==(op_s.top))  // get larger space
        /*
        tmp = (char**)checked_malloc(op_s.len*sizeof(char*));
        for(int i=0;i<op_s.len;i++)
            tmp[i]=op_s.operater[i];
            
        int l=op_s.len;
        free(op_s);// ?
        op_s=(char**)checked_grow_alloc(op_s.len*sizeof(char*));
        op_s.len=2*l;
        for (int i=0; i<op_s.len; i++) {
            op_s[i]=(char*) checked_malloc(OPERATOR_LEN*sizeof(char));
            if(i<l)
            strcpy(op_s.operator[i],tmp[i]);
        }
        free(tmp);//
        op_s.top=l;
    }*/
    {
        op_s.operator=(char**) realloc(op_s.operator, op_s.len*2*sizeof(char*));
        for (int i=op_s.len; i<2*op_s.len; i++)
            op_s.operator[i]=(char*) checked_malloc(OPERATOR_LEN * sizeof(char) ) ;
        op_s.len= 2 * op_s.len;
    }
    op_s.top++;
    strcpy(op_s.operator[op_s.top-1],cc);
}
void pop_op(char* cc)
{
    if (op_s.top==0) {
        error(1, 0, "%d: syntax error operater stack invalid pop", line_number);
    }
    else{
        cc=op_s.operator[op_s.top-1];
        op_s.top--;
    }
}

void push_cmd( command_t cc)
{
    if (cmd_s.len==cmd_s.top) { // get larger space
        
    /*tmp = (command_t*)checked_malloc(cmd_s.len*sizeof(command_t));
    for (int i=0; i<cmd_s.len; i++)
        tmp[i]=cmd_s.commmand[i];
    int l=cmd_s.len;
    cmd_s=(command_t*)checked_grow_alloc(cmd_s.len*sizeof(command_t));
        cmd.s.len=2*l;
    for (int i=0; i<l; i++) {
        cmd_s.command[i]=tmp[i];
    }
    free(tmp);
    cmd_s.top=l;*/
    cmd_s.command=(command_t*)realloc(cmd_s.command, 2*cmd_s.len*sizeof(command_t));// realloc store previous content
            for (int i=cmd_s.len; i<2*cmd_s.len; i++)
            cmd_s.command[i]=(command_t) checked_malloc(sizeof(struct command));
        cmd_s.len=2*cmd_s.len;
        
}
    cmd_s.top++;
    cmd_s.command[cmd_s.top-1]=cc; // copy store
}

void pop_cmd(command_t cc)
{
    if (cmd_s.top==0) {
        error(1, 0, "%d: syntax error command stack invalid pop", line_number);    }
    else{
        cc=cmd_s.command[cmd_s.top-1];
        cmd_s.top--;
    }
}

//********************************************************

command_t build_command_t(char* buff, int* index, size_t ssize)// size=buff--read_size
{
    // init operator stack and command stack
    
    init_op_stack();
    init_cmd_stack();
    
    while ((buff[*index])!=EOF)
    {
        int next= *index+1;// used to detect || &&
        switch (buff[*index]){
            case '(' :
                push_op("(");
                break;
            case ';' :
                // ;;
                if (op_s.top>=cmd_s.top) {
                    error(1, 0, "%d: syntax error dismatched operator and command", line_number);
                }
                // \n ;
                int i=(*index)-1;
                while ((buff[i]!=' ') && (i>=0)) { // skip all ' ' to find if the previous is \n : || b
                    i--;
                }
                if (buff[i]=='\n') {
                    error(1, 0, "%d: syntax error \n appears in wrong place", line_number);
                }

                
                if (op_s.top ==0)
                push_op(";");
                else{
                    while ( (precedence(op_s.operator[op_s.top-1])>=precedence(";")) &&(strcmp(op_s.operator[op_s.top-1],"(")!=0))
                    {
                        char* tmp_op;
                        pop_op(tmp_op);
                        command_t tmp=init_command(cmd_type(tmp_op));
                        pop_cmd(  tmp->u.command[1]);
                        pop_cmd( tmp->u.command[0]);
                        push_cmd( tmp);
                        free (tmp);
                    }
                    push_op( ";");
                    
                }
                // ;\n
                if (buff[next]=='\n') { // regard \n after; as a ;
                    *index=(*index)+1;
                }
            case '|':

                if (buff[next]=='|')  // case: ||
                {
                    *index=(*index)+1;
                    next=(*index)+1;
                    if (buff[next]=='\n') // a ||
                        error(1, 0, "%d: syntax error \n happens with dismatched operator and command", line_number);
                    int i=(*index)-1;
                    while ((buff[i]!=' ') && (i>=0)) { // skip all ' ' to find if the previous is \n : || b
                        i--;
                    }
                    if (buff[i]=='\n') {
                        error(1, 0, "%d: syntax error \n appears in wrong place", line_number);
                    }
                    if (op_s.top>=cmd_s.top) {
                        error(1, 0, "%d: syntax error: dismatched operator and command",line_number);
                    }
                    if (op_s. top==0)
                        push_op("||");
                    else{
                        while ( (precedence(op_s.operator[op_s.top-1])>=precedence("||")) &&(strcmp(op_s.operator[op_s.top-1],"||")!=0))
                        {
                            char* tmp_op;
                            pop_op(tmp_op);
                            command_t tmp=init_command(cmd_type(tmp_op));
                            pop_cmd( tmp->u.command[1]);
                            pop_cmd( tmp->u.command[0]);
                            push_cmd( tmp);
                            free(tmp);
                        }
                        push_op( "||");
                    }
                }
                else{ // case |
                    // check valid
                    int i=(*index)-1;
                    while ((buff[i]!=' ') && (i>=0)) { // skip all ' ' to find if the previous is \n : | b
                        i--;
                    }
                    if (buff[i]=='\n') {
                        error(1, 0, "%d: syntax error \n appears in wrong place", line_number);
                    }
                    if (op_s.top>=cmd_s.top) {
                        error(1, 0, "%d: syntax error: dismatched operator and command",line_number);
                    }
                    
                    if (op_s.top ==0)
                        push_op( "|");
                    else{
                        while ( (precedence(op_s.operator[op_s.top-1])>=precedence("|")) &&(strcmp(op_s.operator[op_s.top-1],"|")!=0))
                        {
                            char* tmp_op;
                            pop_op( tmp_op);
                            command_t tmp=init_command(cmd_type(tmp_op));
                            pop_cmd( tmp->u.command[1]);
                            pop_cmd(tmp->u.command[0]);
                            push_cmd( tmp);
                            free(tmp);
                        }
                        push_op("|");
                    }
                }
                // && ||
                break;
           case '&':
                if(buff[next]=='&')
                {
                    *index=(*index)+1;
                    if (op_s.top>=cmd_s.top) {
                        error(1, 0, "%d: syntax error: dismatched operator and command",line_number);
                    }
                    //valid
                    if (op_s.top ==0)
                        push_op( "&&");
                    else{
                        while ( (precedence(op_s.operator[op_s.top-1])>=precedence("&&")) &&(strcmp(op_s.operator[op_s.top-1],"&&")!=0))
                        {
                            char* tmp_op;
                            pop_op(tmp_op);
                            command_t tmp=init_command(cmd_type(tmp_op));
                            pop_cmd( tmp->u.command[1]);
                            pop_cmd( tmp->u.command[0]);
                            push_cmd( tmp);
                            free(tmp);
                        }
                        push_op( "&&");
                    }
                }
                else
                    error(1, 0, "%d: syntax error invalid & ", line_number);
                break;
                
           case ')':
                if (op_s.top>=cmd_s.top) {
                    error(1, 0, "%d: syntax error: dismatched operator and command",line_number);
                }

                {
                    while ((strcmp(op_s.operator[op_s.top-1],"(")!=0))
                    {
                        char* tmp_op=NULL;
                        pop_op(tmp_op);
                        command_t tmp=init_command(cmd_type(tmp_op));
                        pop_cmd( tmp->u.command[1]);
                        pop_cmd( tmp->u.command[0]);
                        push_cmd( tmp);
                        free (tmp);
                    }
                    command_t tmp=init_command(SUBSHELL_COMMAND);
                    tmp->u.subshell_command= (command_t) checked_malloc(sizeof(struct command));
                    pop_cmd(tmp->u.subshell_command);
                    push_cmd( tmp);
                    free (tmp);
                }
                break;
            case '<':
                if (op_s.top>=cmd_s.top) {
                    error(1, 0, "%d: syntax error: dismatched operator and command",line_number);
                }
                if(buff[next]=='\n')
                    error(1, 0, "%d: syntax error < followed by newline ", line_number);
                //command_t tmp=store_simple_command();// things needed
                //push_cmd(tmp);
                wait_input=true;
                break;
                
            case '>':
                if (op_s.top>=cmd_s.top) {
                    error(1, 0, "%d: syntax error: dismatched operator and command",line_number);
                }
                if(buff[next]=='\n')
                    error(1, 0, "%d: syntax error > followed by newline ", line_number);
                //command_t tmp=store_simple_command();// things needed
                //push_cmd(tmp);
                wait_output=true;
                break;

            case '#': // skip comments
                *index=(*index)+1;
                while (buff[*index] !='\n') {
                    *index=(*index)+1;
                }
                line_number++;
                break;
            case ' ':
                       break;
            case '\n':
                 line_number++;
                // more details !!!!
                if (op_s.top>=cmd_s.top) // a || \n
                    error(1, 0, "%d: syntax error /n happens with dismatched operator and command", line_number);
                if(((buff[next]!='(')||buff[next]!=')')&&(isSpecial(buff[next])))
                    error(1, 0, "%d: syntax error /n is before specials other than ( )",line_number);

                       break;
            default: // simple command
                       //check valid input
                       if(isNotValid(buff[*index]))
                       error(1, 0, "%d: syntax error invalid input", line_number);
                       if (wait_input && wait_output)
                       error (1, 0, "%d: syntax error input output dismatch", line_number);
                
                //
                       if(wait_input)
                       {
                           *index=(*index)+1;
                           char* text= get_next_wordd(buff, index, ssize);
                           int len = sizeof(text)/sizeof(char);
                           cmd_s.command[cmd_s.top-1]->input = (char*) checked_malloc(sizeof(char)*len);
                           
                           strcpy(cmd_s.command[cmd_s.top-1]->input, text); // undefined text
                           
                           wait_input=false;
                           
                           break;
                       }
                       if(wait_output)
                       {
                           *index=(*index)+1;
                           char* text= get_next_word(buff, index, ssize);
                           int len = sizeof(text)/sizeof(char);
                           cmd_s.command[cmd_s.top-1]->output = (char*) checked_malloc(sizeof(char)*len);
                           strcpy(cmd_s.command[cmd_s.top-1]->output, text); // undefined text
                           wait_output =false;
                           break;
                       }
                // simple command
                       *index=(*index)+1;
                       command_t tmp=store_simple_command(buff, index, ssize);// things needed
                       push_cmd( tmp);
                       break;
        }
        *index=(*index)+1;
    }
    // pop all operators and commands
    if (op_s.top>=cmd_s.top) {
        error(1, 0, "%d: syntax error /n happens with dismatched operator and command", line_number);
    }
    while (op_s.top>=0 && cmd_s.top>=0) {
        char* tmp_op;
        pop_op( tmp_op);
        command_t tmp=init_command(cmd_type(tmp_op));
        pop_cmd( tmp->u.command[1]);
        pop_cmd(tmp->u.command[0]);
        push_cmd( tmp);
        free(tmp);
    }
    if ((op_s.top!=0)||(cmd_s.top!=1)) {
        error(1, 0, "%d: syntax error dismatched operator and command", line_number);
    }
    return cmd_s.command[0];
    
}
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    size_t buf_size=2048;
    size_t read_size=0;
    int c;
    char *buf=checked_malloc(buf_size);
    
    while ((c=get_next_byte(get_next_byte_argument))!=EOF)
    {
        buf[read_size++]=c;
        if(read_size==buf_size)
        {
            buf_size=buf_size*2;
            buf=(char*)checked_grow_alloc(buf,&buf_size );
        }
    }
    
    command_stream_t stream=init_stream();
    struct command_node* tmp_node= (struct command_node*) checked_malloc(sizeof(struct command_node));
    command_t tmp=NULL;
    int index=0 ;
    tmp_node->c=build_command_t(buf,&index,read_size );
    tmp_node->next=NULL;
    
    stream->head=tmp_node;
    stream->cursor=tmp_node;
    stream->tail=tmp_node;
    tmp=build_command_t(buf, &index, read_size);
    while(tmp!=NULL){
        stream->cursor->next =(struct command_node*) checked_malloc(sizeof(struct command_node));
        tmp_node->c  = tmp;
        tmp_node->next = NULL;
        // space alloc???
        stream->cursor->next = tmp_node;
        stream->cursor= stream->cursor->next;
        tmp=build_command_t(buf, &index, read_size);
    }
    stream->tail=stream->cursor;
    stream->cursor=stream->head;
          
          
    return stream;
    
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    
    
  //error (1, 0, "command reading not yet implemented");
  //return 0;
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
