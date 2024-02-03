
#include "u_dtype.h"

#ifndef configer_h
#define configer_h

#define MAX_DATA_NAME_LEN       128
#define CONFIG_BUFFER_LEN       1024

/*
   data_name:   string used in config file.
   data_addr:   points to data object to be read.
   data_type:   what the data is.
   upper_bound: max value if not a string (round to this if above).
   lower_bound: min value if not a string (round to this if below).

   Data types are:

   0  - UINT_8
   1  - UINT_16
   2  - UINT_32
   3  - SINT_8
   4  - SINT_16
   5  - SINT_32
   6  - int
   7  - string
   8  - long
   9  - UINT_64
   10 - SINT_64
*/

typedef struct
{
    char data_name[MAX_DATA_NAME_LEN];
    void *data_addr;
    int data_type;
    UINT_64 lower_bound;
    SINT_64 upper_bound;
}
SetupData;


int load_config_file(const char configfile[], SetupData **info);

#endif
