
#ifndef _modules_h
#define _modules_h

#include "u_dtype.h"
#include "configer.h"

/*#define VOLATILITY      volatile*/
#define VOLATILITY

/*
   All modules provide the following functions:

   module_data *modname_alloc(void);
   int  modname_init(module_data *what);
   void modname_go(module_data *what);
   void modname_remove(module_data *what);
   void modname_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
   char *modname_getinf(module_data *what);

   Initialisation is to be done as follows.

   1. The process is started by calling modname_alloc() creates an instance
      of the modname_alloc data structure with memory allocated (but not
      defined as yet).
   2. The caller must then fill in the var arrays with relevant data.
   3. Then modname_init() must be called.  This will malloc internal_data
      and fill it in using the var_*bit arrays now available.  It will also
      fill in any outgoing bus arrays and outof functions.
   4. The caller must then fill in all other relevant (incoming) arrays
      to provide the buses and functions and buses required by the module.
   5. Finally, modname_go() must be called to finish the process.

   When the module is no longer required, modname_remove() should be called
   to free all relevant memory arrays and generally clean up as required.

   The other function provided by the module (appart from the internal
   functions provided in the module_data structure) is:

   modname_cycle() - this function will be called regularly to put the
                     module through the given number of clock cycles.
                     clock_div > 1 indicates that the clock "rate" the
                     module is being run at has been reduced (divided) by
                     this amount.  The module should ignore this for the
                     most part, unless a visible effect (eg. the blinking
                     rate of a cursor is slowed) occurs, in which case
                     minimal effort should be made to ameliorate the
                     visible effect.

   Finally, modname_getinf will return a string containing details of the
   module state in ascii form, for display/debugging purposes.

*/


typedef struct Module_Data
{
    /*
       Module initialisation is done by passing a pointer to this struct,
       which must contain any data required by the module.  The data
       contained in the following arrays:

       var_*bit - * bit variables defining the behaviour of the module.
       bus_*bit - * bit buses used to communicate with the modules.

       sig_calls_into_module  - functions provided by the module for comms.
       sig_calls_outof_module - functions provided to the module for comms.
       sig_calls_outof_args   - arguments passed to sig_calls_outof_module.

       With the exception of sig_calls_into_module, the variables must be
       provided to the initialiser).

       The number of elements in these arrays is defined by the following
       variables: num_var_*bit, num_bus_*bit, 
       num_sig_calls_into_module and num_sig_calls_outof_module.  All of
       these counts will be provided by the module initialiser.
    */

    UINT_8  *var_8bit;
    UINT_16 *var_16bit;
    UINT_32 *var_32bit;

    char **stringvars;

    struct Module_Data **modptrs;

    VOLATILITY UINT_8  **bus_8bit;
    VOLATILITY UINT_16 **bus_16bit;
    VOLATILITY UINT_32 **bus_32bit;

    weird_pointer_jive_wargs  *sig_calls_into_module;
    weird_pointer_jive_wargs  *sig_calls_outof_module;
    void                     **sig_calls_outof_args;

    UINT_64 num_var_8bit;
    UINT_64 num_var_16bit;
    UINT_64 num_var_32bit;

    UINT_64 num_stringvars;

    UINT_64 num_modptrs;

    UINT_64 num_bus_8bit;
    UINT_64 num_bus_16bit;
    UINT_64 num_bus_32bit;

    UINT_64 num_sig_calls_into_module;
    UINT_64 num_sig_calls_outof_module;

    /*
       The following indicates whether clocking is needed (NZ) or not (Z)
    */

    int mod_clocked;

    /*
       Indentifier string for module
    */

    const char *module_name;

    /*
       Config information pointer (see configer.h for details)
    */

    SetupData *config_data;

    /*
       The internal state of the module is kept in an object pointed to
       by the following variable.
    */

    void *internal_data;
}
module_data;


module_data *gen_module_data(const char *module_name,
                             int is_mod_clocked,
                             UINT_64 num_var_8bit,
                             UINT_64 num_var_16bit,
                             UINT_64 num_var_32bit,
                             UINT_64 num_stringvars,
                             UINT_64 num_modptrs,
                             UINT_64 num_bus_8bit,
                             UINT_64 num_bus_16bit,
                             UINT_64 num_bus_32bit,
                             UINT_64 num_sig_calls_into_module,
                             UINT_64 num_sig_calls_outof_module);

module_data *gen_module_data_varonly(const char *module_name,
                                     int is_mod_clocked,
                                     UINT_64 num_var_8bit,
                                     UINT_64 num_var_16bit,
                                     UINT_64 num_var_32bit,
                                     UINT_64 num_stringvars);

module_data *gen_module_data_nonvaronly(module_data *result,
                                        UINT_64 num_modptrs,
                                        UINT_64 num_bus_8bit,
                                        UINT_64 num_bus_16bit,
                                        UINT_64 num_bus_32bit,
                                        UINT_64 num_sig_calls_into_module,
                                        UINT_64 num_sig_calls_outof_module);

void free_module_data(module_data *what);


#define REF_8VAR(what)          (((module_data *) (what))->var_8bit)
#define REF_16VAR(what)         (((module_data *) (what))->var_16bit)
#define REF_32VAR(what)         (((module_data *) (what))->var_32bit)

#define REF_STRINGVAR(what)     (((module_data *) (what))->stringvar)

#define DEREF_8VAR(what,num)    ((((module_data *) (what))->var_8bit)[(num)])
#define DEREF_16VAR(what,num)   ((((module_data *) (what))->var_16bit)[(num)])
#define DEREF_32VAR(what,num)   ((((module_data *) (what))->var_32bit)[(num)])

#define DEREF_STRGVAR(what,num) ((((module_data *) (what))->stringvars)[(num)])

#define DEREF_MODPTR(what,num)  ((((module_data *) (what))->modptrs)[(num)])

#define DEREF_8BUS(what,num)    (*((((module_data *) (what))->bus_8bit)[(num)]))
#define DEREF_16BUS(what,num)   (*((((module_data *) (what))->bus_16bit)[(num)]))
#define DEREF_32BUS(what,num)   (*((((module_data *) (what))->bus_32bit)[(num)]))

#define DEREF_8MEM(what,num)    ((((module_data *) (what))->bus_8bit)[(num)])
#define DEREF_16MEM(what,num)   ((((module_data *) (what))->bus_16bit)[(num)])
#define DEREF_32MEM(what,num)   ((((module_data *) (what))->bus_32bit)[(num)])

#define DEREF_INFN(what,num)    ((((module_data *) (what))->sig_calls_into_module)[(num)])
#define DEREF_OUTFN(what,num)   ((((module_data *) (what))->sig_calls_outof_module)[(num)])
#define DEREF_OUTARGS(what,num) ((((module_data *) (what))->sig_calls_outof_args)[(num)])

#define OUTFNCALL(what,num)     ((((module_data *) (what))->sig_calls_outof_module)[(num)])((((module_data *) (what))->sig_calls_outof_args)[(num)])

#define DEREF_MODNAME(what)     (((module_data *) (what))->module_name)
#define DEREF_INTERNAL(what)    (((module_data *) (what))->internal_data)

#endif
