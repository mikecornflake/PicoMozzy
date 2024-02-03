#include <stdio.h>
#include <stdlib.h>

#include "u_dtype.h"
#include "modules.h"
#include "debmaloc.h"


UINT_8  global_8dummyvar  = 0;
UINT_16 global_16dummyvar = 0;
UINT_32 global_32dummyvar = 0;

UINT_8  *global_8dummyptr  = &global_8dummyvar;
UINT_16 *global_16dummyptr = &global_16dummyvar;
UINT_32 *global_32dummyptr = &global_32dummyvar;

char global_strdummy[] = "";

SetupData default_setdat[] =
{
   { "", NULL, 0, 0, 0 }
};

void global_nothingfn(void *what);

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
                             UINT_64 num_sig_calls_outof_module)
{
    module_data *result;
    UINT_64 i;

    if ( ( result = DEBMALLOC(sizeof(module_data)) ) != NULL )
    {
        result->module_name = module_name;
        result->config_data = default_setdat;

        result->num_var_8bit  = num_var_8bit;
        result->num_var_16bit = num_var_16bit;
        result->num_var_32bit = num_var_32bit;

        result->num_stringvars = num_stringvars;

        result->num_modptrs = num_modptrs;

        result->num_bus_8bit  = num_bus_8bit;
        result->num_bus_16bit = num_bus_16bit;
        result->num_bus_32bit = num_bus_32bit;

        result->num_sig_calls_into_module  = num_sig_calls_into_module;
        result->num_sig_calls_outof_module = num_sig_calls_outof_module;

        result->var_8bit  = NULL;
        result->var_16bit = NULL;
        result->var_32bit = NULL;

        result->stringvars = NULL;

        result->modptrs = NULL;

        result->bus_8bit  = NULL;
        result->bus_16bit = NULL;
        result->bus_32bit = NULL;

        result->sig_calls_into_module  = NULL;
        result->sig_calls_outof_module = NULL;
        result->sig_calls_outof_args   = NULL;

        result->internal_data = NULL;

        result->mod_clocked = is_mod_clocked;

        if ( num_var_8bit  > 0 ) { if ( ( result->var_8bit  = (UINT_8  *) DEBMALLOC(num_var_8bit *sizeof(UINT_8 )) ) == NULL ) { return NULL; } }
        if ( num_var_16bit > 0 ) { if ( ( result->var_16bit = (UINT_16 *) DEBMALLOC(num_var_16bit*sizeof(UINT_16)) ) == NULL ) { return NULL; } }
        if ( num_var_32bit > 0 ) { if ( ( result->var_32bit = (UINT_32 *) DEBMALLOC(num_var_32bit*sizeof(UINT_32)) ) == NULL ) { return NULL; } }

        if ( num_stringvars > 0 ) { if ( ( result->stringvars = (char **) DEBMALLOC(num_stringvars*sizeof(char *)) ) == NULL ) { return NULL; } }

        if ( num_modptrs > 0 ) { if ( ( result->modptrs = (module_data **) DEBMALLOC(num_modptrs*sizeof(module_data *)) ) == NULL ) { return NULL; } }

        if ( num_bus_8bit  > 0 ) { if ( ( result->bus_8bit  = (UINT_8  **) DEBMALLOC(num_bus_8bit *sizeof(UINT_8  *)) ) == NULL ) { return NULL; } }
        if ( num_bus_16bit > 0 ) { if ( ( result->bus_16bit = (UINT_16 **) DEBMALLOC(num_bus_16bit*sizeof(UINT_16 *)) ) == NULL ) { return NULL; } }
        if ( num_bus_32bit > 0 ) { if ( ( result->bus_32bit = (UINT_32 **) DEBMALLOC(num_bus_32bit*sizeof(UINT_32 *)) ) == NULL ) { return NULL; } }

        if ( num_sig_calls_into_module  > 0 ) { if ( ( result->sig_calls_into_module  = (weird_pointer_jive_wargs *) DEBMALLOC(num_sig_calls_into_module *sizeof(weird_pointer_jive_wargs)) ) == NULL ) { return NULL; } }
        if ( num_sig_calls_outof_module > 0 ) { if ( ( result->sig_calls_outof_module = (weird_pointer_jive_wargs *) DEBMALLOC(num_sig_calls_outof_module*sizeof(weird_pointer_jive_wargs)) ) == NULL ) { return NULL; } }
        if ( num_sig_calls_outof_module > 0 ) { if ( ( result->sig_calls_outof_args   = (void **)                    DEBMALLOC(num_sig_calls_outof_module*sizeof(void *                  )) ) == NULL ) { return NULL; } }

        if ( num_var_8bit  > 0 ) { for ( i = 0 ; i < num_var_8bit  ; i++ ) { DEBDEREF((result->var_8bit),i)  = global_8dummyvar;  } }
        if ( num_var_16bit > 0 ) { for ( i = 0 ; i < num_var_16bit ; i++ ) { DEBDEREF((result->var_16bit),i) = global_16dummyvar; } }
        if ( num_var_32bit > 0 ) { for ( i = 0 ; i < num_var_32bit ; i++ ) { DEBDEREF((result->var_32bit),i) = global_32dummyvar; } }

        if ( num_stringvars > 0 ) { for ( i = 0 ; i < num_stringvars ; i++ ) { DEBDEREF((result->stringvars),i) = global_strdummy; } }

        if ( num_modptrs > 0 ) { for ( i = 0 ; i < num_modptrs ; i++ ) { DEBDEREF((result->modptrs),i) = NULL; } }

        if ( num_bus_8bit  > 0 ) { for ( i = 0 ; i < num_bus_8bit  ; i++ ) { DEBDEREF((result->bus_8bit),i)  = global_8dummyptr;  } }
        if ( num_bus_16bit > 0 ) { for ( i = 0 ; i < num_bus_16bit ; i++ ) { DEBDEREF((result->bus_16bit),i) = global_16dummyptr; } }
        if ( num_bus_32bit > 0 ) { for ( i = 0 ; i < num_bus_32bit ; i++ ) { DEBDEREF((result->bus_32bit),i) = global_32dummyptr; } }

        if ( num_sig_calls_outof_module > 0 ) { for ( i = 0 ; i < num_sig_calls_outof_module ; i++ ) { DEBDEREF((result->sig_calls_outof_module),i) = global_nothingfn; } }
        if ( num_sig_calls_outof_module > 0 ) { for ( i = 0 ; i < num_sig_calls_outof_module ; i++ ) { DEBDEREF((result->sig_calls_outof_args),i)   = NULL;             } }
    }

    return result;
}

module_data *gen_module_data_varonly(const char *module_name,
                                     int is_mod_clocked,
                                     UINT_64 num_var_8bit,
                                     UINT_64 num_var_16bit,
                                     UINT_64 num_var_32bit,
                                     UINT_64 num_stringvars)
{
    UINT_64 i;

    module_data *result;

    if ( ( result = DEBMALLOC(sizeof(module_data)) ) != NULL )
    {
        result->module_name = module_name;
        result->config_data = default_setdat;

        result->num_var_8bit  = num_var_8bit;
        result->num_var_16bit = num_var_16bit;
        result->num_var_32bit = num_var_32bit;

        result->num_stringvars = num_stringvars;

        result->var_8bit  = NULL;
        result->var_16bit = NULL;
        result->var_32bit = NULL;

        result->stringvars = NULL;

        result->modptrs = NULL;

        result->bus_8bit  = NULL;
        result->bus_16bit = NULL;
        result->bus_32bit = NULL;

        result->sig_calls_into_module  = NULL;
        result->sig_calls_outof_module = NULL;
        result->sig_calls_outof_args   = NULL;

        result->internal_data = NULL;

        result->mod_clocked = is_mod_clocked;

        if ( num_var_8bit  > 0 ) { if ( ( result->var_8bit  = (UINT_8  *) DEBMALLOC(num_var_8bit *sizeof(UINT_8 )) ) == NULL ) { return NULL; } }
        if ( num_var_16bit > 0 ) { if ( ( result->var_16bit = (UINT_16 *) DEBMALLOC(num_var_16bit*sizeof(UINT_16)) ) == NULL ) { return NULL; } }
        if ( num_var_32bit > 0 ) { if ( ( result->var_32bit = (UINT_32 *) DEBMALLOC(num_var_32bit*sizeof(UINT_32)) ) == NULL ) { return NULL; } }

        if ( num_stringvars > 0 ) { if ( ( result->stringvars = (char **) DEBMALLOC(num_stringvars*sizeof(char *)) ) == NULL ) { return NULL; } }

        if ( num_var_8bit  > 0 ) { for ( i = 0 ; i < num_var_8bit  ; i++ ) { DEBDEREF((result->var_8bit),i)  = global_8dummyvar;  } }
        if ( num_var_16bit > 0 ) { for ( i = 0 ; i < num_var_16bit ; i++ ) { DEBDEREF((result->var_16bit),i) = global_16dummyvar; } }
        if ( num_var_32bit > 0 ) { for ( i = 0 ; i < num_var_32bit ; i++ ) { DEBDEREF((result->var_32bit),i) = global_32dummyvar; } }

        if ( num_stringvars > 0 ) { for ( i = 0 ; i < num_stringvars ; i++ ) { DEBDEREF((result->stringvars),i) = global_strdummy; } }
    }

    return result;
}

        
module_data *gen_module_data_nonvaronly(module_data *result,
                                        UINT_64 num_modptrs,
                                        UINT_64 num_bus_8bit,
                                        UINT_64 num_bus_16bit,
                                        UINT_64 num_bus_32bit,
                                        UINT_64 num_sig_calls_into_module,
                                        UINT_64 num_sig_calls_outof_module)
{
    UINT_64 i;

    if ( result != NULL )
    {
        result->num_modptrs = num_modptrs;

        result->num_bus_8bit  = num_bus_8bit;
        result->num_bus_16bit = num_bus_16bit;
        result->num_bus_32bit = num_bus_32bit;

        result->num_sig_calls_into_module  = num_sig_calls_into_module;
        result->num_sig_calls_outof_module = num_sig_calls_outof_module;

        if ( num_modptrs > 0 ) { if ( ( result->modptrs = (module_data **) DEBMALLOC(num_modptrs*sizeof(module_data *)) ) == NULL ) { return NULL; } }

        if ( num_bus_8bit  > 0 ) { if ( ( result->bus_8bit  = (UINT_8  **) DEBMALLOC(num_bus_8bit *sizeof(UINT_8  *)) ) == NULL ) { return NULL; } }
        if ( num_bus_16bit > 0 ) { if ( ( result->bus_16bit = (UINT_16 **) DEBMALLOC(num_bus_16bit*sizeof(UINT_16 *)) ) == NULL ) { return NULL; } }
        if ( num_bus_32bit > 0 ) { if ( ( result->bus_32bit = (UINT_32 **) DEBMALLOC(num_bus_32bit*sizeof(UINT_32 *)) ) == NULL ) { return NULL; } }

        if ( num_sig_calls_into_module  > 0 ) { if ( ( result->sig_calls_into_module  = (weird_pointer_jive_wargs *) DEBMALLOC(num_sig_calls_into_module *sizeof(weird_pointer_jive_wargs)) ) == NULL ) { return NULL; } }
        if ( num_sig_calls_outof_module > 0 ) { if ( ( result->sig_calls_outof_module = (weird_pointer_jive_wargs *) DEBMALLOC(num_sig_calls_outof_module*sizeof(weird_pointer_jive_wargs)) ) == NULL ) { return NULL; } }
        if ( num_sig_calls_outof_module > 0 ) { if ( ( result->sig_calls_outof_args   = (void **)                    DEBMALLOC(num_sig_calls_outof_module*sizeof(void *                  )) ) == NULL ) { return NULL; } }

        if ( num_modptrs > 0 ) { for ( i = 0 ; i < num_modptrs ; i++ ) { DEBDEREF((result->modptrs),i) = NULL; } }

        if ( num_bus_8bit  > 0 ) { for ( i = 0 ; i < num_bus_8bit  ; i++ ) { DEBDEREF((result->bus_8bit),i)  = global_8dummyptr;  } }
        if ( num_bus_16bit > 0 ) { for ( i = 0 ; i < num_bus_16bit ; i++ ) { DEBDEREF((result->bus_16bit),i) = global_16dummyptr; } }
        if ( num_bus_32bit > 0 ) { for ( i = 0 ; i < num_bus_32bit ; i++ ) { DEBDEREF((result->bus_32bit),i) = global_32dummyptr; } }

        if ( num_sig_calls_outof_module > 0 ) { for ( i = 0 ; i < num_sig_calls_outof_module ; i++ ) { DEBDEREF((result->sig_calls_outof_module),i) = global_nothingfn; } }
        if ( num_sig_calls_outof_module > 0 ) { for ( i = 0 ; i < num_sig_calls_outof_module ; i++ ) { DEBDEREF((result->sig_calls_outof_args),i)   = NULL;             } }
    }

    return result;
}

void global_nothingfn(void *what)
{
    return;

    what = NULL;
}

void free_module_data(module_data *what)
{
    if ( what != NULL )
    {
        if ( what->var_8bit  != NULL ) { DEBFREE(what->var_8bit);  }
        if ( what->var_16bit != NULL ) { DEBFREE(what->var_16bit); }
        if ( what->var_32bit != NULL ) { DEBFREE(what->var_32bit); }

        if ( what->stringvars != NULL ) { DEBFREE(what->stringvars); }

        if ( what->modptrs != NULL ) { DEBFREE(what->modptrs); }

        if ( what->bus_8bit  != NULL ) { DEBFREE(what->bus_8bit);  }
        if ( what->bus_16bit != NULL ) { DEBFREE(what->bus_16bit); }
        if ( what->bus_32bit != NULL ) { DEBFREE(what->bus_32bit); }

        if ( what->sig_calls_into_module  != NULL ) { DEBFREE(what->sig_calls_into_module);  }
        if ( what->sig_calls_outof_module != NULL ) { DEBFREE(what->sig_calls_outof_module); }
        if ( what->sig_calls_outof_args   != NULL ) { DEBFREE(what->sig_calls_outof_args);   }
    }

    return;
}

