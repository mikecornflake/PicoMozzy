
#include "beefile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define BEEFPRINTFBUFSIZE       2048

typedef struct
{
    char c_char;
    int bee_equiv;
}
beetxttran;

void bee_fskip(BEE_FILE *bee_stream);

beetxttran transtab[] = { { '\a' , 0x007 },
                          { '\b' , 0x008 },
                          { '\t' , 0x009 },
                          { '\n' , 0x00a },
                          { '\v' , 0x00b },
                          { '\f' , 0x00c },
                          { '\n' , 0x00d }, /* *cough* *hack* *splutter* */
                          { '\r' , 0x00d },
                          { ' '  , 0x020 },
                          { '!'  , 0x021 },
                          { '\"' , 0x022 },
                          { '#'  , 0x023 },
                          { '$'  , 0x024 },
                          { '%'  , 0x025 },
                          { '&'  , 0x026 },
                          { '\'' , 0x027 },
                          { '('  , 0x028 },
                          { ')'  , 0x029 },
                          { '*'  , 0x02a },
                          { '+'  , 0x02b },
                          { ','  , 0x02c },
                          { '-'  , 0x02d },
                          { '.'  , 0x02e },
                          { '/'  , 0x02f },
                          { '0'  , 0x030 },
                          { '1'  , 0x031 },
                          { '2'  , 0x032 },
                          { '3'  , 0x033 },
                          { '4'  , 0x034 },
                          { '5'  , 0x035 },
                          { '6'  , 0x036 },
                          { '7'  , 0x037 },
                          { '8'  , 0x038 },
                          { '9'  , 0x039 },
                          { ':'  , 0x03a },
                          { ';'  , 0x03b },
                          { '<'  , 0x03c },
                          { '='  , 0x03d },
                          { '>'  , 0x03e },
                          { '\?' , 0x03f },
                          { '@'  , 0x040 },
                          { 'A'  , 0x041 },
                          { 'B'  , 0x042 },
                          { 'C'  , 0x043 },
                          { 'D'  , 0x044 },
                          { 'E'  , 0x045 },
                          { 'F'  , 0x046 },
                          { 'G'  , 0x047 },
                          { 'H'  , 0x048 },
                          { 'I'  , 0x049 },
                          { 'J'  , 0x04a },
                          { 'K'  , 0x04b },
                          { 'L'  , 0x04c },
                          { 'M'  , 0x04d },
                          { 'N'  , 0x04e },
                          { 'O'  , 0x04f },
                          { 'P'  , 0x050 },
                          { 'Q'  , 0x051 },
                          { 'R'  , 0x052 },
                          { 'S'  , 0x053 },
                          { 'T'  , 0x054 },
                          { 'U'  , 0x055 },
                          { 'V'  , 0x056 },
                          { 'W'  , 0x057 },
                          { 'X'  , 0x058 },
                          { 'Y'  , 0x059 },
                          { 'Z'  , 0x05a },
                          { '['  , 0x05b },
                          { '\\' , 0x05c },
                          { ']'  , 0x05d },
                          { '^'  , 0x05e },
                          { '_'  , 0x05f },
                          { '`'  , 0x060 },
                          { 'a'  , 0x061 },
                          { 'b'  , 0x062 },
                          { 'c'  , 0x063 },
                          { 'd'  , 0x064 },
                          { 'e'  , 0x065 },
                          { 'f'  , 0x066 },
                          { 'g'  , 0x067 },
                          { 'h'  , 0x068 },
                          { 'i'  , 0x069 },
                          { 'j'  , 0x06a },
                          { 'k'  , 0x06b },
                          { 'l'  , 0x06c },
                          { 'm'  , 0x06d },
                          { 'n'  , 0x06e },
                          { 'o'  , 0x06f },
                          { 'p'  , 0x070 },
                          { 'q'  , 0x071 },
                          { 'r'  , 0x072 },
                          { 's'  , 0x073 },
                          { 't'  , 0x074 },
                          { 'u'  , 0x075 },
                          { 'v'  , 0x076 },
                          { 'w'  , 0x077 },
                          { 'x'  , 0x078 },
                          { 'y'  , 0x079 },
                          { 'z'  , 0x07a },
                          { '{'  , 0x07b },
                          { '|'  , 0x07c },
                          { '}'  , 0x07d },
                          { '~'  , 0x07e },
                          { ' '  , 0x0ff }  }; /* 0x0ff marks end */

void bee_fskip(BEE_FILE *bee_stream)
{
    int i;
    int temp;

    if ( bee_stream != NULL )
    {
        if ( bee_stream->is_txtmode )
        {
            /*
               Keep going until the end of the file is hit, or a recognised
               txt char is found - whichever happens first.
            */

            bee_stream->nextread = EOF;

            while ( !feof(bee_stream->rawfile) )
            {
                /*
                   Get the character at this position.
                */

                temp = fgetc(bee_stream->rawfile);

                /*
                   Run through translation table.  If found, return.
                   Otherwise do nothing.
                */

                i = 0;

                while ( ( (transtab[i]).bee_equiv != 0x0ff ) && ( bee_stream->nextread == EOF ) )
                {
                    if ( (transtab[i]).c_char == temp )
                    {
                        bee_stream->nextread = (transtab[i]).bee_equiv;
                    }

                    i++;
                }
            }
        }

        else
        {
            bee_stream->nextread = fgetc(bee_stream->rawfile);
        }
    }

    return;
}

int bee_fclose(BEE_FILE *bee_stream)
{
    int result;

    if ( bee_stream != NULL )
    {
        if ( bee_stream->rawfile != NULL )
        {
            result = fclose(bee_stream->rawfile);

            free(bee_stream);

            return result;
        }

        return EOF;
    }

    return EOF;
}

int bee_feof(BEE_FILE *bee_stream)
{
    if ( bee_stream != NULL )
    {
        if ( bee_stream->nextread != EOF )
        {
            return 0;
        }
    }

    return 1;
}

int bee_fgetc(BEE_FILE *bee_stream)
{
    int result;

    if ( bee_stream != NULL )
    {
        result = bee_stream->nextread;

        bee_fskip(bee_stream);

        return result;
    }

    return EOF;
}

int bee_fputc(int bee_c, BEE_FILE *bee_stream)
{
    int trans_c;
    int i;

    if ( bee_stream != NULL )
    {
        if ( bee_stream->is_txtmode )
        {
            trans_c = EOF;

            i = 0;

            while ( ( (transtab[i]).bee_equiv != 0x0ff ) && ( trans_c == EOF ) )
            {
                if ( (transtab[i]).bee_equiv == bee_c )
                {
                    trans_c = (transtab[i]).c_char;
                }

                i++;
            }
        }

        else
        {
            trans_c = bee_c;
        }

        if ( trans_c != EOF )
        {
            if ( fputc(trans_c,bee_stream->rawfile) != trans_c )
            {
                return EOF;
            }
        }

        return bee_c;
    }

    return EOF;
}

BEE_FILE *bee_fopen(const char *bee_filename, const char *bee_mode)
{
    BEE_FILE *result;
    int i,j;

    if ( ( result = (BEE_FILE *) malloc(sizeof(BEE_FILE)) ) != NULL )
    {
        /*
           First set the microbee filetype.  Mode is txt by default, but
           binary can be indicated by a 'b' in the bee_mode string.
        */

        result->is_readmode = 0;
        result->is_txtmode  = 1;

        if ( ( j = strlen(bee_mode) ) > 0 )
        {
            for ( i = 0 ; i < j ; i++ )
            {
                if ( bee_mode[i] == 'b' )
                {
                    result->is_txtmode = 0;
                }

                else if ( bee_mode[i] == 'r' )
                {
                    result->is_readmode = 1;
                }
            }
        }

        /*
           Then open the file.
        */

        if ( ( result->rawfile = fopen(bee_filename,bee_mode) ) == NULL )
        {
            free(result);

            result = NULL;
        }

        else if ( result->is_readmode )
        {
            /*
               Get char if reading (all reads are pre-read).
            */

            bee_fskip(result);
        }
    }

    return result;
}

int bee_fprintf(BEE_FILE *bee_stream, const char *fmt, ...)
{
    char buffer[BEEFPRINTFBUFSIZE];
    int buflen;
    int i;
    va_list args;

    va_start(args,fmt);
    buflen = vsprintf(buffer,fmt,args);
    va_end(args);

    if ( buflen > 0 )
    {
        for ( i = 0 ; i < buflen ; i++ )
        {
            if ( bee_fputc(buffer[i],bee_stream) == EOF )
            {
                return -1;
            }
        }
    }

    return buflen;
}


void fix_bee_filename(char *filename, char replch)
{
    int i,j;

    if ( ( j = strlen(filename) ) > 0 )
    {
        for ( i = 0 ; i < j ; i++ )
        {
            if ( !( ( filename[i] >= ' '  ) &&
                    ( filename[i] <= '~'  ) &&
                    ( filename[i] != '\\' ) &&
                    ( filename[i] != '/'  ) &&
                    ( filename[i] != ':'  ) &&
                    ( filename[i] != '*'  ) &&
                    ( filename[i] != '\?' ) &&
                    ( filename[i] != '\"' ) &&
                    ( filename[i] != '<'  ) &&
                    ( filename[i] != '>'  ) &&
                    ( filename[i] != '|'  )    ) )
            {
                filename[i] = replch;
            }
        }
    }

    return;
}


int pc_fclose(PC_FILE *pc_stream)
{
    return fclose(pc_stream);
}

int pc_feof(PC_FILE *pc_stream)
{
    return feof(pc_stream);
}

int pc_fgetc(PC_FILE *pc_stream)
{
    return fgetc(pc_stream);
}

char *pc_fgets(char *s, int n, PC_FILE *pc_stream)
{
    return fgets(s,n,pc_stream);
}

PC_FILE *pc_fopen(const char *_filename, const char *pc_mode)
{
    return fopen(_filename,pc_mode);
}

int pc_fputc(int pc_c, PC_FILE *pc_stream)
{
    return fputc(pc_c,pc_stream);
}

