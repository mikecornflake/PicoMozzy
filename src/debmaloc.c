
#include <stdio.h>
#include <stdlib.h>
#include "debmaloc.h"

#define MAX_MALL_BUFSIZE 204800

int num_malled = 0;

void **malled_addr;
int   *malled_size;

FILE *malloc_error_log;

void *deb_malloc(int size, const char *mess, int line)
{
    void *result;

    if ( num_malled == 0 )
    {
        if ( ( malloc_error_log = fopen("mallerr.log","wt") ) == NULL )
        {
            return NULL;
        }

        fprintf(malloc_error_log,"Starting debug run.\n");
        fprintf(malloc_error_log,"===================\n\n");

        fflush(malloc_error_log);

        malled_addr = (void **) malloc(MAX_MALL_BUFSIZE*sizeof(void *));
        malled_size = (int *)   malloc(MAX_MALL_BUFSIZE*sizeof(int));

        if ( ( malled_size == NULL ) || ( malled_addr == NULL ) )
        {
            exit(1);
        }
    }

    if ( ( result = malloc(size) ) != NULL )
    {
        if ( num_malled < MAX_MALL_BUFSIZE-1 )
        {
            malled_addr[num_malled] = result;
            malled_size[num_malled] = size;

            num_malled++;
        }

        else
        {
            fprintf(malloc_error_log,"malloc buffer full at %d\n",(int) num_malled);

            fflush(malloc_error_log);
        }
    }

    else
    {
        fprintf(malloc_error_log,"Malloc failed[%08x]: %s line %d\n",size,mess,line);

        fflush(malloc_error_log);
    }

    return result;
}

int deb_deref(void *what, int where, const char *mess, int line)
{
    int i,j;

    if ( num_malled > 0 )
    {
        j = -1;

        if ( num_malled > 0 )
        {
            for ( i = 0 ; i < num_malled ; i++ )
            {
                if ( malled_addr[i] == what )
                {
                    j = i;
                }
            }
        }

        if ( j > -1 )
        {
            if ( where < 0 )
            {
                fprintf(malloc_error_log,"Deref with -ve index[%08x]: %s line %d\n",where,mess,line);

                fflush(malloc_error_log);
            }

            if ( where > malled_size[j]-1 )
            {
                fprintf(malloc_error_log,"Deref over range[%08x > %08x]: %s line %d\n",where,malled_size[j]-1,mess,line);

                fflush(malloc_error_log);
            }
        }

        else
        {
            goto deref_error;
        }
    }

    else
    {
        deref_error:

        fprintf(malloc_error_log,"Deref non-allocced pointer[%08x]: %s line %d\n",where,mess,line);

        fflush(malloc_error_log);
    }

    return 0;
}

void deb_free(void *what, const char *mess, int line)
{
    int i,j;

    if ( num_malled > 0 )
    {
        j = -1;

        if ( num_malled > 0 )
        {
            for ( i = 0 ; i < num_malled ; i++ )
            {
                if ( malled_addr[i] == what )
                {
                    j = i;
                }
            }
        }

        if ( j > -1 )
        {
            free(what);

            num_malled--;

            if ( j < num_malled )
            {
                for ( i = j ; i < num_malled ; i++ )
                {
                    malled_addr[i] = malled_addr[i+1];
                    malled_size[i] = malled_size[i+1];
                }
            }
        }

        else
        {
            goto free_error;
        }
    }

    else
    {
        free_error:

        fprintf(malloc_error_log,"Free non-allocced pointer: %s line %d\n",mess,line);

        fflush(malloc_error_log);
    }

    return;
}


void report_error(const char *mess, int line)
{
    fprintf(malloc_error_log,"Failed assertion: %s line %d\n",mess,line);

    fflush(malloc_error_log);

    return;
}

