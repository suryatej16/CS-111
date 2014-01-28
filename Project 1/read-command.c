// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

struct command_stream
{
  int full_command_size;
  int full_command_position;
  char **full_commands;
}stream;

static command_stream_t stream_t = &stream;


//Function helps determine if char is valid, but not a special token
int is_valid_character(int input)
{
  if((input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z') || (input >= '0' && input <= '9') || (input == '!') || (input == '%') || (input == '+') || (input ==',') | (input == '-') || (input == '.') || (input == '/') || (input == ':') || (input == '@') || (input == '^') || (input == '_'))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

//Function helps check left side of special tokens for validity
int check_left_valid(char* start, char* end) {
  while(end != start) {
    end--;
    if(*end == ' ' || *end == '\t') 
      continue;
    else if(*end == ')' || is_valid_character(*end)) 
      return 1;
    else
      return 0;
  }
  return 0;
}


//Function helps check right side of special tokens for validity
int check_right_valid(char* start, int redirect) {
  int newlines = 0;
  if(start != NULL && *start == '\n')
    newlines++;
  while(start != NULL) {
    start++;
    if(*start == ' ' || *start == '\t' || (!redirect && *start == '\n')) {
      if(*start == '\n') {
        newlines++;
        continue;
      }
    }
    else if(*start == '(' || is_valid_character(*start))
      if(newlines ==0)
        return 99;
      else 
        return newlines;
    else
      return 0;
  }
  return 0;
}

//Function checks if newline is actually a complete command
int complete_command(char* start, char* nline, int line, int parentheses_open) {
  nline++;
  //Check if EOF
  if(*nline == NULL ) 
    return 3;
  //Check if previous character was a redirect
  nline--;
  while(nline != start) {
    nline--;
    if(nline != NULL) {
      if(*nline == ' ' || *nline == '\t')
        continue;
      else if(*nline == ')') {
        return 1;
      }
      else if(*nline == ';')  
        return 1;
      else if(is_valid_character(*nline)) {
        if(parentheses_open) {
          nline++;
          if(*nline == '\n')
            *nline = ';';
        } 
        return 1;
      }
      /*  nline++;
//If valid char before new line, check after new line for another new line 
        while(*(nline++) != '\0') {
          char x = *nline;
          if(x == '\n')
            return 1;
          else if( x == ' ' || x == '\t') 
            continue;
          else if(x == '(' || x == ')' || is_valid_character(x))
            return 0;*/
      else if(*nline == '<' || *nline == '>') {
        fprintf(stderr, "%i: Syntax error new line after redirection", line);
        exit(1);
      }
      else
        return 0;  
    }
  }
  return 0;
}
  
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  char* whole_file;
  int next_byte;
  int count = 0;
  int line = 1;
  int max_size = 50;
  int complete_new = 0;
  whole_file = malloc(max_size*sizeof(char));
  stream_t->full_command_size = malloc(sizeof(int));
  stream_t->full_command_position = malloc(sizeof(int));
  stream_t->full_command_size = 10;
  stream_t->full_command_position = 0;
  stream_t->full_commands = malloc(stream_t->full_command_size*sizeof(char*));
  
  while((next_byte=get_next_byte(get_next_byte_argument)) >= 0)
    {
         if(count == max_size ) {
           whole_file = realloc(whole_file, max_size*2);
           max_size *=2;
         }
         whole_file[count] = next_byte;
         count++;
    }
  if(count == max_size) {
    whole_file = realloc(whole_file, max_size+2);
  }
  whole_file[count] = '\n';
  count++;
  whole_file[count+1] = '\0';
//Read in whole file as a string and add null byte to end
  int beginning = 0; //delete this
  int i = 0;
  int second_token = 0;
  char* start_ptr = whole_file;
  int parentheses_open = 0;
  int comment = 0;
  int result;
  int comment_command = 0;
  char* comment_ptr = NULL;
  char* end_comment = NULL;
  for(; i<count; i++ ) {
   //Going character by character, skip comments
     second_token = 0;
     if(whole_file[i] == ' ' || whole_file[i] == '\t')
       continue;
//REach a new line, check to see if its part of a group of new lines (in a row)
//if this is the end of a comment
//or if this is the end of a full command
     if(whole_file[i] == '\n'){
       if(*start_ptr == whole_file[i]) //delete this
	 {
	   start_ptr++;
       line++;
	   continue;
	 }
       if(complete_new)
         continue;
       if(comment) { //fix this later too
         if(comment_ptr != start_ptr) {
           comment_command = 1;
           result = complete_command(start_ptr, comment_ptr, line, parentheses_open);
           end_comment = &whole_file[i];
         } else {
           line++;
           comment = 0;
           comment_ptr = NULL;
           start_ptr = &whole_file[i];
           while(*start_ptr == '\n') 
             start_ptr++;
           while(whole_file[i+1] == '\n')
             i++;
           continue;
         }
       }
       if(comment_command != 1) {
         result = complete_command(start_ptr, whole_file+i, line, parentheses_open);
       }
//Based on result, if it returns > 0 that means it is a complete command
       if(result >0) {
         if(parentheses_open) {
         if(i == count-1) {
           fprintf(stderr, "%i: Syntax error no matching parentheses", line);
           exit(1);
         }
          continue;
         }
         line+=result;
         int j = 0;
         int prev_space = 0;
         if(stream_t->full_command_position == stream_t->full_command_size)
         {
           stream_t->full_command_size += 10;
	       stream_t->full_commands = realloc(stream_t->full_commands, stream_t->full_command_size*(sizeof(char*)));
         }

//Create space in full_command array to hold new command
         if(comment_command != 1) {
           stream_t->full_commands[stream_t->full_command_position] = malloc(1+(&whole_file[i]-start_ptr)*sizeof(char));
           while(start_ptr != &whole_file[i]) {
             if(*start_ptr == '\n') {
               start_ptr++;
               prev_space = 0;
               continue;
             } else
             if(*start_ptr == ' ') {
               if(prev_space) {
                 start_ptr++;
                 continue;
               }
               else {
                 prev_space= 1;
               }
             } else {
               prev_space = 0;
             }
             stream_t->full_commands[stream_t->full_command_position][j] = *start_ptr;
             j++;
             start_ptr++;
             }
         } else {
           stream_t->full_commands[stream_t->full_command_position] = malloc(1+(comment_ptr-start_ptr)*sizeof(char));
           while(start_ptr != comment_ptr) {
             if(*start_ptr == '\n') {
               start_ptr++;
               prev_space = 0;
               continue;
             } else
             if(*start_ptr == ' ') {
               if(prev_space) {
                 start_ptr++;
                 continue;
               }
               else {
                 prev_space= 1;
               }
             } else {
               prev_space = 0;
             }
             stream_t->full_commands[stream_t->full_command_position][j] = *start_ptr;
             j++;
             start_ptr++;
           }
         }
         stream_t->full_commands[stream_t->full_command_position][j] = '\0';
         stream_t->full_command_position++;
         complete_new = 1;
         if(result != 3) {
           if(comment_command == 1) {
             start_ptr = end_comment;
             comment_command = 0;
             comment_ptr = NULL;
             end_comment = NULL;
             comment = 0;
           }
           while(*start_ptr == '\n') {
             start_ptr++;
           }
           while(whole_file[i+1] == '\n') {
             i++;
           }
           continue;
         } else {
           stream_t->full_command_position = 0;
           return stream_t;
         }
       }
      }
//If you are in a comment, ignore characters 
     if(comment){
       whole_file[i] = ' '; //delete this later too
       continue;
     }
//If you hit a hash sign, this is a start of a comment
     else if(whole_file[i] == '#') {
       comment = 1;
       complete_new = 0;
       comment_ptr = &whole_file[i];
       whole_file[i] = ' '; //delete this later too
       continue;
     }
//If this is a character, continue
     else if(is_valid_character(whole_file[i]))  {
       complete_new = 0;
       continue;
     }
//If this is redirect, check the left and right side for validity
     else if(whole_file[i] == '<' || whole_file[i] == '>') {
       if(!check_left_valid(start_ptr, (whole_file+i))) {
         fprintf(stderr,"%i: Syntax Error for left side of %c", line, whole_file[i]);
         exit(1);
       }
       int newlines = check_right_valid(whole_file+i, 1);
       if(newlines == 0) {
         fprintf(stderr, "%i: Syntax Error for right side of %c", line, whole_file[i]);
         exit(1);
       }
     }
//If this is AND or OR check left and right as well
     else if(whole_file[i] == '&' || whole_file[i] == '|') {
       if(!check_left_valid(start_ptr, (whole_file+i))) {
         fprintf(stderr,"%i: Syntax Error for left side of %c",line, whole_file[i]);
         exit(1);
       }
       if((whole_file[i] == '&' || ((i+1 != count) && whole_file[i+1] == '|'))) {
         second_token = 1;
       }
       int newlines = check_right_valid((whole_file+i+second_token), 0);
       if(newlines == 0) {
         fprintf(stderr,"%i: Syntax Error for right side of %c",line, whole_file[i]);
         exit(1);
       }
       if(second_token)
         i++;
       if(newlines>0 && newlines != 99) 
         i+=newlines;
     }
//Signal that you are looking for the end of subshell
     else if(whole_file[i] == '(') {
       parentheses_open ++;
       continue;
     }
//Signal that you have reached the end of a subshell
     else if(whole_file[i] == ')') {
       parentheses_open --;
       continue;
     }
//Something must be on the left side of the semicolon
     else if(whole_file[i] == ';') {
       if(!check_left_valid(start_ptr, (whole_file+i))) {
         fprintf(stderr, "%i: Syntax Error left side of ;", line);
         exit(1);
       }
     } 
     else {
       fprintf(stderr, "%i: Syntax Error unidentified character", line);
       exit(1);
     }
   }
  stream_t->full_command_position = 0; 
  return stream_t;
}

//This helper function removes beginning and trailing spaces, between 
//the pointers given in the parameters.
char*
read_part_command(char* start, char* end) {
  char* return_ptr;
  while(*start == ' ')
    start++;
  end--;
  while(*end == ' ')
    end--;
  int size = end - start;
  return_ptr = (char*) malloc(size+2);
  strncpy(return_ptr, start, size+1);
  return_ptr[size+1] = '\0';
  //printf("return_ptr %sE\n", return_ptr);
  return return_ptr;
}

//Majority of work to find commands found here
//Priority goes (), <>, |, &&||, ;, nothing.
//Therefore, search in reverse order and recursively parse the commands.
command_t
parse_command_stream (char* test) {
  if(test == NULL)
    return NULL;
  char* pipe = strrchr(test, '|');
  char* start_ptr = test;
  char* pos_ptr = start_ptr;
  char* end_ptr = test+strlen(test);
  char* left;
  char* right;
  
  command_t cmd = ((command_t) malloc(sizeof(command_t)));
  cmd->u.word = (malloc(1*sizeof(char*)));
  *cmd->u.word = malloc(sizeof(start_ptr));
  
  //Start searching for the tokens
  char* and_token = strrchr(pos_ptr, '&');
  char* temp_or = strstr(pos_ptr, "||");

  //OR token must not be mistaken for PIPE, so look for second |
  char* or_token = NULL;
  while(temp_or != NULL) {
    or_token = temp_or;
    temp_or = strstr(temp_or+2, "||");
  }
  char* in_token = strrchr(pos_ptr, '>');
  char* out_token = strrchr(pos_ptr, '<');
  char* semi_token = strrchr(pos_ptr, ';');
  char* subshell_token = strrchr(pos_ptr, '(');
  char* end_subshell_token = strrchr(pos_ptr, ')');
  

  if(subshell_token != NULL) {
    if(and_token - subshell_token > 0 && (and_token - end_subshell_token < 0) ) 
      and_token = NULL;
    if(or_token - subshell_token > 0 && (or_token - end_subshell_token < 0 ) )
      or_token = NULL;
    if(pipe - subshell_token > 0 && (pipe - end_subshell_token < 0 ) )
      pipe = NULL;
    if(in_token - subshell_token > 0 && (in_token - end_subshell_token < 0 ) )
      in_token = NULL;
    if(out_token - subshell_token > 0 && (out_token - end_subshell_token < 0 ) )
      out_token = NULL;
    if(semi_token - subshell_token > 0  && (semi_token - end_subshell_token < 0 ))
      semi_token = NULL;
  }
//If no tokens are found, this is a simple command.
  if(pipe == NULL && and_token == NULL && or_token == NULL && in_token == NULL 
                  && out_token == NULL && semi_token == NULL 
                                       && subshell_token == NULL) {
    cmd->type = SIMPLE_COMMAND;
    strncpy(*cmd->u.word, start_ptr, strlen(start_ptr)); 
  } else 

//Search for sequence commands next.
  if(semi_token != NULL) {
    cmd->type = SEQUENCE_COMMAND;
    left = read_part_command(start_ptr, semi_token);
    right = read_part_command(semi_token+1, end_ptr);
    cmd->u.command[0] = parse_command_stream(left);
    cmd->u.command[1] = parse_command_stream(right);
  } else 

//Search for ANDS and ORS at the same time, then go from right-left order priority
if(and_token != NULL || or_token != NULL) {
    if(and_token == NULL || (and_token != NULL && or_token != NULL 
                                       && ((and_token - or_token) < 0))) {
      left = read_part_command(start_ptr, or_token);
      right = read_part_command(or_token+2, end_ptr);
      cmd->type = OR_COMMAND;
    } else {
      left = read_part_command(start_ptr, and_token-1);
      right = read_part_command(and_token+1, end_ptr);
      cmd->type = AND_COMMAND;
    }
    cmd->u.command[0] = parse_command_stream(left);
    cmd->u.command[1] = parse_command_stream(right);
  } else 

//Search for pipes
if(pipe != NULL) {
    left = read_part_command(start_ptr, pipe);
    right = read_part_command(pipe+1, end_ptr);
    cmd->type = PIPE_COMMAND;
    cmd->status = -1;
    cmd->u.command[0] = parse_command_stream(left);
    cmd->u.command[1] = parse_command_stream(right);
  } else 

//I/O Redirection, look for both and right-left priority
if((in_token != NULL || out_token != NULL) && subshell_token == NULL) {
    cmd->type = SIMPLE_COMMAND;
    if(in_token != NULL && out_token != NULL) {
      left = read_part_command(start_ptr, out_token);
      right = read_part_command(out_token+1, in_token);
      cmd->input = malloc(sizeof(right));
      strncpy(cmd->input, right, strlen(right));
      right = read_part_command(in_token+1, end_ptr);
      cmd->output = malloc(sizeof(right));
      strncpy(cmd->output, right, strlen(right));
    } else
    if(in_token == NULL) {
      left = read_part_command(start_ptr, out_token);
      right = read_part_command(out_token+1, end_ptr);
      cmd->input = malloc(sizeof(start_ptr));
      strncpy(cmd->input, right, strlen(right));
    } else {
      left = read_part_command(start_ptr, in_token);
      right = read_part_command(in_token+1, end_ptr);
      cmd->output = malloc(sizeof(start_ptr));
      strncpy(cmd->output, right, strlen(right));
    }
    strncpy(*cmd->u.word, left, strlen(left));
  } else 

//Subshell commands are highest priority so they are found last so they are added to the top of the tree
if(subshell_token != NULL) {
  cmd->type = SUBSHELL_COMMAND;
     //>
  if(in_token != NULL && out_token != NULL) {
    left = read_part_command(start_ptr, out_token);
    right = read_part_command(out_token+1, subshell_token);
    cmd->input = malloc(sizeof(right));
    strncpy(cmd->input, right, strlen(right));
    right = read_part_command(in_token+1, end_ptr);
    cmd->output = malloc(sizeof(right));
    strncpy(cmd->output, right, strlen(right));
  } 
  if(in_token != NULL) {
    right = read_part_command(in_token+1, end_ptr);
    cmd->output = malloc(sizeof(start_ptr));
    strncpy(cmd->output, right, strlen(right));
  }
  left = read_part_command(++subshell_token, end_subshell_token);
  cmd->u.subshell_command = malloc(sizeof(command_t));
  cmd->u.subshell_command = parse_command_stream(left);
}
  return cmd; 
}

command_t
read_command_stream (command_stream_t s)
{
  command_t cmd;
  if( s->full_command_position < s->full_command_size) {
    cmd = parse_command_stream(s->full_commands[s->full_command_position]);
    s->full_command_position++;
  } else {
    cmd = NULL;
  }
  return cmd;
}
