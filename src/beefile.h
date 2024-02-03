#include <stdio.h>
#include <stdarg.h>

#ifndef _BEEFILE_H
#define _BEEFILE_H

/*
   The following attempts to abstract away (to some degree) the differences
   between the control characters used by the bee (char 13, for example,
   being '\n') and the standards of the underlying OS.  Basically, these
   functions operate just like their counterparts in the C standard library,
   except that in text mode they translate to/from the microbee standard,
   not the standard of the underlying OS.

   The function fix_bee_filename(char *filename, char replch) will scan
   through the string filename and replace any illegal chars (ie. not
   allowed by the filesystem) with the character replch.

   FIXME: The way \n is dealt with is dubious (crlf issues).
*/

typedef struct
{
    FILE *rawfile;
    int is_readmode;
    int is_txtmode; /* If set then translate \n etc. to bee txt */
    int nextread;   /* All reads are pre-recced and stored here */
}
BEE_FILE;

#define PC_FILE FILE

int       bee_fclose(BEE_FILE *bee_stream);
int       bee_feof(BEE_FILE *bee_stream);
int       bee_fgetc(BEE_FILE *bee_stream);
BEE_FILE *bee_fopen(const char *_filename, const char *bee_mode);
int       bee_fputc(int bee_c, BEE_FILE *bee_stream);
int       bee_fprintf(BEE_FILE *_stream, const char *_format, ...);

void fix_bee_filename(char *filename, char replch);


int      pc_fclose(PC_FILE *pc_stream);
int      pc_feof(PC_FILE *pc_stream);
int      pc_fgetc(PC_FILE *pc_stream);
char    *pc_fgets(char *s, int n, PC_FILE *pc_stream);
PC_FILE *pc_fopen(const char *_filename, const char *pc_mode);
int      pc_fputc(int pc_c, PC_FILE *pc_stream);

#endif

