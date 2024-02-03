
#include <stdio.h>
#include <string.h>
#include "configer.h"
#include "u_dtype.h"
#include "beefile.h"



int load_config_file(const char configfile[], SetupData **info)
{
    PC_FILE *gp;
    char buffera[CONFIG_BUFFER_LEN+1];
    char bufferb[CONFIG_BUFFER_LEN+1];
    int i,j,k,l;
    unsigned long m;
    long n;
    char c;
    int result = 0;

    if ( ( gp = pc_fopen(configfile,"rt") ) == NULL )
    {
        result = 1;

        goto exit_point;
    }

    while ( !pc_feof(gp) )
    {
        if ( pc_fgets(buffera,CONFIG_BUFFER_LEN,gp) == NULL )
        {
            /* For some reason, this detects eof before the above. */

            goto exit_point;
        }

        i = 1;

        while ( info[i-1] != NULL )
        {
            j = 1;

            while ( (info[i-1][j-1]).data_addr != NULL )
            {
                if ( buffera[0] != '%' )
                {
                    if ( buffera[strlen((info[i-1][j-1]).data_name)] == ' ' )
                    {
                        if ( strncmp(buffera,(info[i-1][j-1]).data_name,strlen((info[i-1][j-1]).data_name)) == 0 )
                        {
                            switch ( (info[i-1][j-1]).data_type )
                            {
                                case 0:
                                {
                                    sscanf(buffera,"%s %c %lu",bufferb,&c,&m);

                                    if ( m > (info[i-1][j-1]).upper_bound ) { m = (info[i-1][j-1]).upper_bound; }
                                    if ( m < (info[i-1][j-1]).lower_bound ) { m = (info[i-1][j-1]).lower_bound; }

                                    *((UINT_8 *) (info[i-1][j-1]).data_addr) = (UINT_8) m;

                                    break;
                                }

                                case 1:
                                {
                                    sscanf(buffera,"%s %c %lu",bufferb,&c,&m);

                                    if ( m > (info[i-1][j-1]).upper_bound ) { m = (info[i-1][j-1]).upper_bound; }
                                    if ( m < (info[i-1][j-1]).lower_bound ) { m = (info[i-1][j-1]).lower_bound; }

                                    *((UINT_16 *) (info[i-1][j-1]).data_addr) = (UINT_16) m;

                                    break;
                                }

                                case 2:
                                {
                                    sscanf(buffera,"%s %c %lu",bufferb,&c,&m);

                                    if ( m > (info[i-1][j-1]).upper_bound ) { m = (info[i-1][j-1]).upper_bound; }
                                    if ( m < (info[i-1][j-1]).lower_bound ) { m = (info[i-1][j-1]).lower_bound; }

                                    *((UINT_32 *) (info[i-1][j-1]).data_addr) = (UINT_32) m;

                                    break;
                                }

                                case 3:
                                {
                                    sscanf(buffera,"%s %c %ld",bufferb,&c,&n);

                                    if ( n > (info[i-1][j-1]).upper_bound ) { n = (info[i-1][j-1]).upper_bound; }
                                    if ( n < (info[i-1][j-1]).lower_bound ) { n = (info[i-1][j-1]).lower_bound; }

                                    *((SINT_8 *) (info[i-1][j-1]).data_addr) = (SINT_8) n;

                                    break;
                                }

                                case 4:
                                {
                                    sscanf(buffera,"%s %c %ld",bufferb,&c,&n);

                                    if ( n > (info[i-1][j-1]).upper_bound ) { n = (info[i-1][j-1]).upper_bound; }
                                    if ( n < (info[i-1][j-1]).lower_bound ) { n = (info[i-1][j-1]).lower_bound; }

                                    *((SINT_16 *) (info[i-1][j-1]).data_addr) = (SINT_16) n;

                                    break;
                                }

                                case 5:
                                {
                                    sscanf(buffera,"%s %c %ld",bufferb,&c,&n);

                                    if ( n > (info[i-1][j-1]).upper_bound ) { n = (info[i-1][j-1]).upper_bound; }
                                    if ( n < (info[i-1][j-1]).lower_bound ) { n = (info[i-1][j-1]).lower_bound; }

                                    *((SINT_32 *) (info[i-1][j-1]).data_addr) = (SINT_32) n;

                                    break;
                                }

                                case 6:
                                {
                                    sscanf(buffera,"%s %c %d",bufferb,&c,(int *) ((info[i-1][j-1]).data_addr));

                                    if ( (*((int *) ((info[i-1][j-1]).data_addr))) > (info[i-1][j-1]).upper_bound ) { (*((int *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).upper_bound; }
                                    if ( (*((int *) ((info[i-1][j-1]).data_addr))) < (info[i-1][j-1]).lower_bound ) { (*((int *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).lower_bound; }

                                    break;
                                }

                                case 7:
                                {
                                    l = -1;

                                    ((char *) (info[i-1][j-1]).data_addr)[0] = '\0';

                                    if ( strlen((info[i-1][j-1]).data_name)+1 < strlen(buffera) )
                                    {
                                        for ( k = strlen((info[i-1][j-1]).data_name)+1 ; ((unsigned int) k) < strlen(buffera) ; k++ )
                                        {
                                            switch ( l )
                                            {
                                                case -1:
                                                {
                                                    if ( buffera[k-1] == '=' )
                                                    {
                                                        l = 0;
                                                    }

                                                    break;
                                                }

                                                case 0:
                                                {
                                                    if ( ( buffera[k-1] != ' '  ) &&
                                                         ( buffera[k-1] != '\t' )    )
                                                    {
                                                        l = 1;

                                                        goto default_proceed;
                                                    }

                                                    break;
                                                }

                                                default:
                                                {
                                                    default_proceed:

                                                    if ( l < CONFIG_BUFFER_LEN )
                                                    {
                                                        ((char *) (info[i-1][j-1]).data_addr)[l-1] = buffera[k-1];
                                                        ((char *) (info[i-1][j-1]).data_addr)[l]   = '\0';
                                                    }

                                                    l++;

                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    break;
                                }

                                case 8:
                                {
                                    sscanf(buffera,"%s %c %ld",bufferb,&c,(long *) ((info[i-1][j-1]).data_addr));

                                    if ( (*((long *) ((info[i-1][j-1]).data_addr))) > (info[i-1][j-1]).upper_bound ) { (*((long *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).upper_bound; }
                                    if ( (*((long *) ((info[i-1][j-1]).data_addr))) < (info[i-1][j-1]).lower_bound ) { (*((long *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).lower_bound; }

                                    break;
                                }

                                case 9:
                                {
                                    sscanf(buffera,"%s %c %Lux",bufferb,&c,(UINT_64 *) ((info[i-1][j-1]).data_addr));

                                    if ( (*((UINT_64 *) ((info[i-1][j-1]).data_addr))) > (info[i-1][j-1]).upper_bound ) { (*((UINT_64 *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).upper_bound; }
                                    if ( (*((UINT_64 *) ((info[i-1][j-1]).data_addr))) < (info[i-1][j-1]).lower_bound ) { (*((UINT_64 *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).lower_bound; }

                                    break;
                                }

                                case 10:
                                {
                                    sscanf(buffera,"%s %c %Ldx",bufferb,&c,(SINT_64 *) ((info[i-1][j-1]).data_addr));

                                    if ( (*((SINT_64 *) ((info[i-1][j-1]).data_addr))) > (info[i-1][j-1]).upper_bound ) { (*((SINT_64 *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).upper_bound; }
                                    if ( (*((SINT_64 *) ((info[i-1][j-1]).data_addr))) < (info[i-1][j-1]).lower_bound ) { (*((SINT_64 *) ((info[i-1][j-1]).data_addr))) = (info[i-1][j-1]).lower_bound; }

                                    break;
                                }

                                default:
                                {
                                    result = 4;

                                    goto exit_point;

                                    break;
                                }
                            }
                        }
                    }
                }

                j++;
            }

            i++;
        }
    }

    exit_point:

    pc_fclose(gp);

    return result;
}
