
#include "genmod.h"
#include "u_dtype.h"
#include "debmaloc.h"
#include "modules.h"
#include "beefile.h"
#include <stdlib.h>
#include <strings.h>


/*

                           Generic Modules
                           ===============

*/


/**********************************************************************

Structural module 1: null

**********************************************************************/

void nullmod_nothing(void *what);

module_data *nullmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,0,0,0,1,0);

    return what;
}

int nullmod_init(module_data *what)
{
    DEREF_INFN(what,0) = nullmod_nothing;

    return 0;
}

void nullmod_go(module_data *what)
{
    return;

    what = NULL;
}

void nullmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void nullmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void nullmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *nullmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void nullmod_nothing(void *what)
{
    return;

    what = NULL;
}





/**********************************************************************

Structural module 2: do

**********************************************************************/

#define DOMOD_NUMFNS(what)    DEREF_32VAR(what,0)

void domod_branch(void *what);

module_data *domod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data_varonly(module_name,0,0,0,1,0);

    return what;
}

int domod_init(module_data *what)
{
    int result = 1;

    if ( ( what = gen_module_data_nonvaronly(what,0,0,0,0,1,DOMOD_NUMFNS(what)) ) != NULL )
    {
        DEREF_INFN(what,0) = domod_branch;

        result = 0;
    }

    return result;
}

void domod_go(module_data *what)
{
    return;

    what = NULL;
}

void domod_stop(module_data *what)
{
    return;

    what = NULL;
}

void domod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void domod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *domod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void domod_branch(void *what)
{
    UINT_64 i;

    if ( DOMOD_NUMFNS(what) > 0 )
    {
        for ( i = 0 ; i < DOMOD_NUMFNS(what) ; i++ )
        {
            OUTFNCALL(what,i);
        }
    }

    return;
}




/**********************************************************************

Structural module 3: dowhile

**********************************************************************/

#define DOWHILEMOD_NUMFNS(what) DEREF_32VAR(what,0)

#define DOWHILEMOD_COND8(what)  DEREF_8BUS(what,0)
#define DOWHILEMOD_COND16(what) DEREF_16BUS(what,0)
#define DOWHILEMOD_COND32(what) DEREF_32BUS(what,0)

void dowhilemod_branch8(void *what);
void dowhilemod_branch16(void *what);
void dowhilemod_branch32(void *what);

module_data *dowhilemod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data_varonly(module_name,0,0,0,1,0);

    DOWHILEMOD_NUMFNS(what) = 2;

    return what;
}

int dowhilemod_init(module_data *what)
{
    int result = 1;

    if ( ( what = gen_module_data_nonvaronly(what,0,1,1,1,3,DOWHILEMOD_NUMFNS(what)) ) != NULL )
    {
        DEREF_INFN(what,0) = dowhilemod_branch8;
        DEREF_INFN(what,1) = dowhilemod_branch16;
        DEREF_INFN(what,2) = dowhilemod_branch32;

        result = 0;
    }

    return result;
}

void dowhilemod_go(module_data *what)
{
    return;

    what = NULL;
}

void dowhilemod_stop(module_data *what)
{
    return;

    what = NULL;
}

void dowhilemod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void dowhilemod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *dowhilemod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void dowhilemod_branch8(void *what)
{
    UINT_64 i;

    if ( DOWHILEMOD_NUMFNS(what) > 0 )
    {
        do
        {
            for ( i = 0 ; i < DOWHILEMOD_NUMFNS(what) ; i++ )
            {
                OUTFNCALL(what,i);
            }
        }
        while ( DOWHILEMOD_COND8(what) );
    }

    return;
}

void dowhilemod_branch16(void *what)
{
    UINT_64 i;

    if ( DOWHILEMOD_NUMFNS(what) > 0 )
    {
        do
        {
            for ( i = 0 ; i < DOWHILEMOD_NUMFNS(what) ; i++ )
            {
                OUTFNCALL(what,i);
            }
        }
        while ( DOWHILEMOD_COND16(what) );
    }

    return;
}

void dowhilemod_branch32(void *what)
{
    UINT_64 i;

    if ( DOWHILEMOD_NUMFNS(what) > 0 )
    {
        do
        {
            for ( i = 0 ; i < DOWHILEMOD_NUMFNS(what) ; i++ )
            {
                OUTFNCALL(what,i);
            }
        }
        while ( DOWHILEMOD_COND32(what) );
    }

    return;
}




/**********************************************************************

Structural module 4: table8

**********************************************************************/

#define TABLE8MOD_SEL8(what)    DEREF_8BUS(what,0)
#define TABLE8MOD_SEL16(what)   DEREF_16BUS(what,0)
#define TABLE8MOD_SEL32(what)   DEREF_32BUS(what,0)

void table8mod_branch8(void *what);
void table8mod_branch16(void *what);
void table8mod_branch32(void *what);

module_data *table8mod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,1,1,1,3,256);

    return what;
}

int table8mod_init(module_data *what)
{
    DEREF_INFN(what,0) = table8mod_branch8;
    DEREF_INFN(what,1) = table8mod_branch16;
    DEREF_INFN(what,2) = table8mod_branch32;

    return 0;
}

void table8mod_go(module_data *what)
{
    return;

    what = NULL;
}

void table8mod_stop(module_data *what)
{
    return;

    what = NULL;
}

void table8mod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void table8mod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *table8mod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void table8mod_branch8(void *what)
{
    OUTFNCALL(what,TABLE8MOD_SEL8(what));

    return;
}

void table8mod_branch16(void *what)
{
    OUTFNCALL(what,(TABLE8MOD_SEL16(what) & 0x0ff));

    return;
}

void table8mod_branch32(void *what)
{
    OUTFNCALL(what,(TABLE8MOD_SEL32(what) & 0x0ff));

    return;
}




/**********************************************************************

Structural module 5: lut8

**********************************************************************/

#define LUT8MOD_LUT(what)       REF_8VAR(what)

#define LUT8MOD_RES8(what)      DEREF_8BUS(what,0)
#define LUT8MOD_RES16(what)     DEREF_16BUS(what,0)
#define LUT8MOD_RES32(what)     DEREF_32BUS(what,0)
#define LUT8MOD_SEL(what)       DEREF_8BUS(what,1)

void lut8mod_doit8(void *what);
void lut8mod_doit16(void *what);
void lut8mod_doit32(void *what);

module_data *lut8mod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,256,0,0,0,0,2,1,1,3,0);

    return what;
}

int lut8mod_init(module_data *what)
{
    DEREF_INFN(what,0) = lut8mod_doit8;
    DEREF_INFN(what,1) = lut8mod_doit16;
    DEREF_INFN(what,2) = lut8mod_doit32;

    return 0;
}

void lut8mod_go(module_data *what)
{
    return;

    what = NULL;
}

void lut8mod_stop(module_data *what)
{
    return;

    what = NULL;
}

void lut8mod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void lut8mod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *lut8mod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void lut8mod_doit8(void *what)
{
    LUT8MOD_RES8(what) = LUT8MOD_LUT(what)[LUT8MOD_SEL(what)];

    return;
}

void lut8mod_doit16(void *what)
{
    LUT8MOD_RES16(what) = (UINT_16) (LUT8MOD_LUT(what)[LUT8MOD_SEL(what)]);

    return;
}

void lut8mod_doit32(void *what)
{
    LUT8MOD_RES32(what) = (UINT_32) (LUT8MOD_LUT(what)[LUT8MOD_SEL(what)]);

    return;
}




/**********************************************************************

Functional module 1: bus

**********************************************************************/

#define BUSMOD_8RESTYPE(what)   DEREF_8VAR(what,0)
#define BUSMOD_16RESTYPE(what)  DEREF_8VAR(what,1)
#define BUSMOD_32RESTYPE(what)  DEREF_8VAR(what,2)
#define BUSMOD_8RESVAL(what)    DEREF_8VAR(what,3)
#define BUSMOD_16RESVAL(what)   DEREF_16VAR(what,0)
#define BUSMOD_32RESVAL(what)   DEREF_32VAR(what,0)

#define BUSMOD_BUS_8BIT(what)   DEREF_8MEM(what,0)
#define BUSMOD_BUS_16BIT(what)  DEREF_16MEM(what,0)
#define BUSMOD_BUS_32BIT(what)  DEREF_32MEM(what,0)

void busmod_reset(void *what);

void busmod_inc8(void *what);
void busmod_dec8(void *what);
void busmod_sr8(void *what);
void busmod_sl8(void *what);
void busmod_rr8(void *what);
void busmod_rl8(void *what);
void busmod_inc16(void *what);
void busmod_dec16(void *what);
void busmod_sr16(void *what);
void busmod_sl16(void *what);
void busmod_rr16(void *what);
void busmod_rl16(void *what);
void busmod_inc32(void *what);
void busmod_dec32(void *what);
void busmod_sr32(void *what);
void busmod_sl32(void *what);
void busmod_rr32(void *what);
void busmod_rl32(void *what);

void busmod_8set0(void *what);
void busmod_8set1(void *what);
void busmod_8set2(void *what);
void busmod_8set3(void *what);
void busmod_8set4(void *what);
void busmod_8set5(void *what);
void busmod_8set6(void *what);
void busmod_8set7(void *what);

void busmod_16set0(void *what);
void busmod_16set1(void *what);
void busmod_16set2(void *what);
void busmod_16set3(void *what);
void busmod_16set4(void *what);
void busmod_16set5(void *what);
void busmod_16set6(void *what);
void busmod_16set7(void *what);
void busmod_16set8(void *what);
void busmod_16set9(void *what);
void busmod_16seta(void *what);
void busmod_16setb(void *what);
void busmod_16setc(void *what);
void busmod_16setd(void *what);
void busmod_16sete(void *what);
void busmod_16setf(void *what);

void busmod_32set00(void *what);
void busmod_32set01(void *what);
void busmod_32set02(void *what);
void busmod_32set03(void *what);
void busmod_32set04(void *what);
void busmod_32set05(void *what);
void busmod_32set06(void *what);
void busmod_32set07(void *what);
void busmod_32set08(void *what);
void busmod_32set09(void *what);
void busmod_32set0a(void *what);
void busmod_32set0b(void *what);
void busmod_32set0c(void *what);
void busmod_32set0d(void *what);
void busmod_32set0e(void *what);
void busmod_32set0f(void *what);
void busmod_32set10(void *what);
void busmod_32set11(void *what);
void busmod_32set12(void *what);
void busmod_32set13(void *what);
void busmod_32set14(void *what);
void busmod_32set15(void *what);
void busmod_32set16(void *what);
void busmod_32set17(void *what);
void busmod_32set18(void *what);
void busmod_32set19(void *what);
void busmod_32set1a(void *what);
void busmod_32set1b(void *what);
void busmod_32set1c(void *what);
void busmod_32set1d(void *what);
void busmod_32set1e(void *what);
void busmod_32set1f(void *what);

void busmod_preset00(void *what);
void busmod_preset01(void *what);
void busmod_preset02(void *what);
void busmod_preset03(void *what);
void busmod_preset04(void *what);
void busmod_preset05(void *what);
void busmod_preset06(void *what);
void busmod_preset07(void *what);
void busmod_preset08(void *what);
void busmod_preset09(void *what);
void busmod_preset0a(void *what);
void busmod_preset0b(void *what);
void busmod_preset0c(void *what);
void busmod_preset0d(void *what);
void busmod_preset0e(void *what);
void busmod_preset0f(void *what);
void busmod_preset10(void *what);
void busmod_preset11(void *what);
void busmod_preset12(void *what);
void busmod_preset13(void *what);
void busmod_preset14(void *what);
void busmod_preset15(void *what);
void busmod_preset16(void *what);
void busmod_preset17(void *what);
void busmod_preset18(void *what);
void busmod_preset19(void *what);
void busmod_preset1a(void *what);
void busmod_preset1b(void *what);
void busmod_preset1c(void *what);
void busmod_preset1d(void *what);
void busmod_preset1e(void *what);
void busmod_preset1f(void *what);
void busmod_preset20(void *what);
void busmod_preset21(void *what);
void busmod_preset22(void *what);
void busmod_preset23(void *what);
void busmod_preset24(void *what);
void busmod_preset25(void *what);
void busmod_preset26(void *what);
void busmod_preset27(void *what);
void busmod_preset28(void *what);
void busmod_preset29(void *what);
void busmod_preset2a(void *what);
void busmod_preset2b(void *what);
void busmod_preset2c(void *what);
void busmod_preset2d(void *what);
void busmod_preset2e(void *what);
void busmod_preset2f(void *what);
void busmod_preset30(void *what);
void busmod_preset31(void *what);
void busmod_preset32(void *what);
void busmod_preset33(void *what);
void busmod_preset34(void *what);
void busmod_preset35(void *what);
void busmod_preset36(void *what);
void busmod_preset37(void *what);
void busmod_preset38(void *what);
void busmod_preset39(void *what);
void busmod_preset3a(void *what);
void busmod_preset3b(void *what);
void busmod_preset3c(void *what);
void busmod_preset3d(void *what);
void busmod_preset3e(void *what);
void busmod_preset3f(void *what);
void busmod_preset40(void *what);
void busmod_preset41(void *what);
void busmod_preset42(void *what);
void busmod_preset43(void *what);
void busmod_preset44(void *what);
void busmod_preset45(void *what);
void busmod_preset46(void *what);
void busmod_preset47(void *what);
void busmod_preset48(void *what);
void busmod_preset49(void *what);
void busmod_preset4a(void *what);
void busmod_preset4b(void *what);
void busmod_preset4c(void *what);
void busmod_preset4d(void *what);
void busmod_preset4e(void *what);
void busmod_preset4f(void *what);
void busmod_preset50(void *what);
void busmod_preset51(void *what);
void busmod_preset52(void *what);
void busmod_preset53(void *what);
void busmod_preset54(void *what);
void busmod_preset55(void *what);
void busmod_preset56(void *what);
void busmod_preset57(void *what);
void busmod_preset58(void *what);
void busmod_preset59(void *what);
void busmod_preset5a(void *what);
void busmod_preset5b(void *what);
void busmod_preset5c(void *what);
void busmod_preset5d(void *what);
void busmod_preset5e(void *what);
void busmod_preset5f(void *what);
void busmod_preset60(void *what);
void busmod_preset61(void *what);
void busmod_preset62(void *what);
void busmod_preset63(void *what);
void busmod_preset64(void *what);
void busmod_preset65(void *what);
void busmod_preset66(void *what);
void busmod_preset67(void *what);
void busmod_preset68(void *what);
void busmod_preset69(void *what);
void busmod_preset6a(void *what);
void busmod_preset6b(void *what);
void busmod_preset6c(void *what);
void busmod_preset6d(void *what);
void busmod_preset6e(void *what);
void busmod_preset6f(void *what);
void busmod_preset70(void *what);
void busmod_preset71(void *what);
void busmod_preset72(void *what);
void busmod_preset73(void *what);
void busmod_preset74(void *what);
void busmod_preset75(void *what);
void busmod_preset76(void *what);
void busmod_preset77(void *what);
void busmod_preset78(void *what);
void busmod_preset79(void *what);
void busmod_preset7a(void *what);
void busmod_preset7b(void *what);
void busmod_preset7c(void *what);
void busmod_preset7d(void *what);
void busmod_preset7e(void *what);
void busmod_preset7f(void *what);
void busmod_preset80(void *what);
void busmod_preset81(void *what);
void busmod_preset82(void *what);
void busmod_preset83(void *what);
void busmod_preset84(void *what);
void busmod_preset85(void *what);
void busmod_preset86(void *what);
void busmod_preset87(void *what);
void busmod_preset88(void *what);
void busmod_preset89(void *what);
void busmod_preset8a(void *what);
void busmod_preset8b(void *what);
void busmod_preset8c(void *what);
void busmod_preset8d(void *what);
void busmod_preset8e(void *what);
void busmod_preset8f(void *what);
void busmod_preset90(void *what);
void busmod_preset91(void *what);
void busmod_preset92(void *what);
void busmod_preset93(void *what);
void busmod_preset94(void *what);
void busmod_preset95(void *what);
void busmod_preset96(void *what);
void busmod_preset97(void *what);
void busmod_preset98(void *what);
void busmod_preset99(void *what);
void busmod_preset9a(void *what);
void busmod_preset9b(void *what);
void busmod_preset9c(void *what);
void busmod_preset9d(void *what);
void busmod_preset9e(void *what);
void busmod_preset9f(void *what);
void busmod_preseta0(void *what);
void busmod_preseta1(void *what);
void busmod_preseta2(void *what);
void busmod_preseta3(void *what);
void busmod_preseta4(void *what);
void busmod_preseta5(void *what);
void busmod_preseta6(void *what);
void busmod_preseta7(void *what);
void busmod_preseta8(void *what);
void busmod_preseta9(void *what);
void busmod_presetaa(void *what);
void busmod_presetab(void *what);
void busmod_presetac(void *what);
void busmod_presetad(void *what);
void busmod_presetae(void *what);
void busmod_presetaf(void *what);
void busmod_presetb0(void *what);
void busmod_presetb1(void *what);
void busmod_presetb2(void *what);
void busmod_presetb3(void *what);
void busmod_presetb4(void *what);
void busmod_presetb5(void *what);
void busmod_presetb6(void *what);
void busmod_presetb7(void *what);
void busmod_presetb8(void *what);
void busmod_presetb9(void *what);
void busmod_presetba(void *what);
void busmod_presetbb(void *what);
void busmod_presetbc(void *what);
void busmod_presetbd(void *what);
void busmod_presetbe(void *what);
void busmod_presetbf(void *what);
void busmod_presetc0(void *what);
void busmod_presetc1(void *what);
void busmod_presetc2(void *what);
void busmod_presetc3(void *what);
void busmod_presetc4(void *what);
void busmod_presetc5(void *what);
void busmod_presetc6(void *what);
void busmod_presetc7(void *what);
void busmod_presetc8(void *what);
void busmod_presetc9(void *what);
void busmod_presetca(void *what);
void busmod_presetcb(void *what);
void busmod_presetcc(void *what);
void busmod_presetcd(void *what);
void busmod_presetce(void *what);
void busmod_presetcf(void *what);
void busmod_presetd0(void *what);
void busmod_presetd1(void *what);
void busmod_presetd2(void *what);
void busmod_presetd3(void *what);
void busmod_presetd4(void *what);
void busmod_presetd5(void *what);
void busmod_presetd6(void *what);
void busmod_presetd7(void *what);
void busmod_presetd8(void *what);
void busmod_presetd9(void *what);
void busmod_presetda(void *what);
void busmod_presetdb(void *what);
void busmod_presetdc(void *what);
void busmod_presetdd(void *what);
void busmod_presetde(void *what);
void busmod_presetdf(void *what);
void busmod_presete0(void *what);
void busmod_presete1(void *what);
void busmod_presete2(void *what);
void busmod_presete3(void *what);
void busmod_presete4(void *what);
void busmod_presete5(void *what);
void busmod_presete6(void *what);
void busmod_presete7(void *what);
void busmod_presete8(void *what);
void busmod_presete9(void *what);
void busmod_presetea(void *what);
void busmod_preseteb(void *what);
void busmod_presetec(void *what);
void busmod_preseted(void *what);
void busmod_presetee(void *what);
void busmod_presetef(void *what);
void busmod_presetf0(void *what);
void busmod_presetf1(void *what);
void busmod_presetf2(void *what);
void busmod_presetf3(void *what);
void busmod_presetf4(void *what);
void busmod_presetf5(void *what);
void busmod_presetf6(void *what);
void busmod_presetf7(void *what);
void busmod_presetf8(void *what);
void busmod_presetf9(void *what);
void busmod_presetfa(void *what);
void busmod_presetfb(void *what);
void busmod_presetfc(void *what);
void busmod_presetfd(void *what);
void busmod_presetfe(void *what);
void busmod_presetff(void *what);

module_data *busmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,4,1,1,0,0,1,1,1,331,0);

    BUSMOD_8RESTYPE(what)  = 2;
    BUSMOD_16RESTYPE(what) = 2;
    BUSMOD_32RESTYPE(what) = 2;
    BUSMOD_8RESVAL(what)   = 0;
    BUSMOD_16RESVAL(what)  = 0;
    BUSMOD_32RESVAL(what)  = 0;

    return what;
}

int busmod_init(module_data *what)
{
    if ( ( BUSMOD_BUS_8BIT(what)  = (UINT_8 *)  DEBMALLOC(sizeof(UINT_8))  ) == NULL ) { return 1; }
    if ( ( BUSMOD_BUS_16BIT(what) = (UINT_16 *) DEBMALLOC(sizeof(UINT_16)) ) == NULL ) { return 1; }
    if ( ( BUSMOD_BUS_32BIT(what) = (UINT_32 *) DEBMALLOC(sizeof(UINT_32)) ) == NULL ) { return 1; }

    DEREF_INFN(what,0x000) = busmod_preset00;
    DEREF_INFN(what,0x001) = busmod_preset01;
    DEREF_INFN(what,0x002) = busmod_preset02;
    DEREF_INFN(what,0x003) = busmod_preset03;
    DEREF_INFN(what,0x004) = busmod_preset04;
    DEREF_INFN(what,0x005) = busmod_preset05;
    DEREF_INFN(what,0x006) = busmod_preset06;
    DEREF_INFN(what,0x007) = busmod_preset07;
    DEREF_INFN(what,0x008) = busmod_preset08;
    DEREF_INFN(what,0x009) = busmod_preset09;
    DEREF_INFN(what,0x00a) = busmod_preset0a;
    DEREF_INFN(what,0x00b) = busmod_preset0b;
    DEREF_INFN(what,0x00c) = busmod_preset0c;
    DEREF_INFN(what,0x00d) = busmod_preset0d;
    DEREF_INFN(what,0x00e) = busmod_preset0e;
    DEREF_INFN(what,0x00f) = busmod_preset0f;
    DEREF_INFN(what,0x010) = busmod_preset10;
    DEREF_INFN(what,0x011) = busmod_preset11;
    DEREF_INFN(what,0x012) = busmod_preset12;
    DEREF_INFN(what,0x013) = busmod_preset13;
    DEREF_INFN(what,0x014) = busmod_preset14;
    DEREF_INFN(what,0x015) = busmod_preset15;
    DEREF_INFN(what,0x016) = busmod_preset16;
    DEREF_INFN(what,0x017) = busmod_preset17;
    DEREF_INFN(what,0x018) = busmod_preset18;
    DEREF_INFN(what,0x019) = busmod_preset19;
    DEREF_INFN(what,0x01a) = busmod_preset1a;
    DEREF_INFN(what,0x01b) = busmod_preset1b;
    DEREF_INFN(what,0x01c) = busmod_preset1c;
    DEREF_INFN(what,0x01d) = busmod_preset1d;
    DEREF_INFN(what,0x01e) = busmod_preset1e;
    DEREF_INFN(what,0x01f) = busmod_preset1f;
    DEREF_INFN(what,0x020) = busmod_preset20;
    DEREF_INFN(what,0x021) = busmod_preset21;
    DEREF_INFN(what,0x022) = busmod_preset22;
    DEREF_INFN(what,0x023) = busmod_preset23;
    DEREF_INFN(what,0x024) = busmod_preset24;
    DEREF_INFN(what,0x025) = busmod_preset25;
    DEREF_INFN(what,0x026) = busmod_preset26;
    DEREF_INFN(what,0x027) = busmod_preset27;
    DEREF_INFN(what,0x028) = busmod_preset28;
    DEREF_INFN(what,0x029) = busmod_preset29;
    DEREF_INFN(what,0x02a) = busmod_preset2a;
    DEREF_INFN(what,0x02b) = busmod_preset2b;
    DEREF_INFN(what,0x02c) = busmod_preset2c;
    DEREF_INFN(what,0x02d) = busmod_preset2d;
    DEREF_INFN(what,0x02e) = busmod_preset2e;
    DEREF_INFN(what,0x02f) = busmod_preset2f;
    DEREF_INFN(what,0x030) = busmod_preset30;
    DEREF_INFN(what,0x031) = busmod_preset31;
    DEREF_INFN(what,0x032) = busmod_preset32;
    DEREF_INFN(what,0x033) = busmod_preset33;
    DEREF_INFN(what,0x034) = busmod_preset34;
    DEREF_INFN(what,0x035) = busmod_preset35;
    DEREF_INFN(what,0x036) = busmod_preset36;
    DEREF_INFN(what,0x037) = busmod_preset37;
    DEREF_INFN(what,0x038) = busmod_preset38;
    DEREF_INFN(what,0x039) = busmod_preset39;
    DEREF_INFN(what,0x03a) = busmod_preset3a;
    DEREF_INFN(what,0x03b) = busmod_preset3b;
    DEREF_INFN(what,0x03c) = busmod_preset3c;
    DEREF_INFN(what,0x03d) = busmod_preset3d;
    DEREF_INFN(what,0x03e) = busmod_preset3e;
    DEREF_INFN(what,0x03f) = busmod_preset3f;
    DEREF_INFN(what,0x040) = busmod_preset40;
    DEREF_INFN(what,0x041) = busmod_preset41;
    DEREF_INFN(what,0x042) = busmod_preset42;
    DEREF_INFN(what,0x043) = busmod_preset43;
    DEREF_INFN(what,0x044) = busmod_preset44;
    DEREF_INFN(what,0x045) = busmod_preset45;
    DEREF_INFN(what,0x046) = busmod_preset46;
    DEREF_INFN(what,0x047) = busmod_preset47;
    DEREF_INFN(what,0x048) = busmod_preset48;
    DEREF_INFN(what,0x049) = busmod_preset49;
    DEREF_INFN(what,0x04a) = busmod_preset4a;
    DEREF_INFN(what,0x04b) = busmod_preset4b;
    DEREF_INFN(what,0x04c) = busmod_preset4c;
    DEREF_INFN(what,0x04d) = busmod_preset4d;
    DEREF_INFN(what,0x04e) = busmod_preset4e;
    DEREF_INFN(what,0x04f) = busmod_preset4f;
    DEREF_INFN(what,0x050) = busmod_preset50;
    DEREF_INFN(what,0x051) = busmod_preset51;
    DEREF_INFN(what,0x052) = busmod_preset52;
    DEREF_INFN(what,0x053) = busmod_preset53;
    DEREF_INFN(what,0x054) = busmod_preset54;
    DEREF_INFN(what,0x055) = busmod_preset55;
    DEREF_INFN(what,0x056) = busmod_preset56;
    DEREF_INFN(what,0x057) = busmod_preset57;
    DEREF_INFN(what,0x058) = busmod_preset58;
    DEREF_INFN(what,0x059) = busmod_preset59;
    DEREF_INFN(what,0x05a) = busmod_preset5a;
    DEREF_INFN(what,0x05b) = busmod_preset5b;
    DEREF_INFN(what,0x05c) = busmod_preset5c;
    DEREF_INFN(what,0x05d) = busmod_preset5d;
    DEREF_INFN(what,0x05e) = busmod_preset5e;
    DEREF_INFN(what,0x05f) = busmod_preset5f;
    DEREF_INFN(what,0x060) = busmod_preset60;
    DEREF_INFN(what,0x061) = busmod_preset61;
    DEREF_INFN(what,0x062) = busmod_preset62;
    DEREF_INFN(what,0x063) = busmod_preset63;
    DEREF_INFN(what,0x064) = busmod_preset64;
    DEREF_INFN(what,0x065) = busmod_preset65;
    DEREF_INFN(what,0x066) = busmod_preset66;
    DEREF_INFN(what,0x067) = busmod_preset67;
    DEREF_INFN(what,0x068) = busmod_preset68;
    DEREF_INFN(what,0x069) = busmod_preset69;
    DEREF_INFN(what,0x06a) = busmod_preset6a;
    DEREF_INFN(what,0x06b) = busmod_preset6b;
    DEREF_INFN(what,0x06c) = busmod_preset6c;
    DEREF_INFN(what,0x06d) = busmod_preset6d;
    DEREF_INFN(what,0x06e) = busmod_preset6e;
    DEREF_INFN(what,0x06f) = busmod_preset6f;
    DEREF_INFN(what,0x070) = busmod_preset70;
    DEREF_INFN(what,0x071) = busmod_preset71;
    DEREF_INFN(what,0x072) = busmod_preset72;
    DEREF_INFN(what,0x073) = busmod_preset73;
    DEREF_INFN(what,0x074) = busmod_preset74;
    DEREF_INFN(what,0x075) = busmod_preset75;
    DEREF_INFN(what,0x076) = busmod_preset76;
    DEREF_INFN(what,0x077) = busmod_preset77;
    DEREF_INFN(what,0x078) = busmod_preset78;
    DEREF_INFN(what,0x079) = busmod_preset79;
    DEREF_INFN(what,0x07a) = busmod_preset7a;
    DEREF_INFN(what,0x07b) = busmod_preset7b;
    DEREF_INFN(what,0x07c) = busmod_preset7c;
    DEREF_INFN(what,0x07d) = busmod_preset7d;
    DEREF_INFN(what,0x07e) = busmod_preset7e;
    DEREF_INFN(what,0x07f) = busmod_preset7f;
    DEREF_INFN(what,0x080) = busmod_preset80;
    DEREF_INFN(what,0x081) = busmod_preset81;
    DEREF_INFN(what,0x082) = busmod_preset82;
    DEREF_INFN(what,0x083) = busmod_preset83;
    DEREF_INFN(what,0x084) = busmod_preset84;
    DEREF_INFN(what,0x085) = busmod_preset85;
    DEREF_INFN(what,0x086) = busmod_preset86;
    DEREF_INFN(what,0x087) = busmod_preset87;
    DEREF_INFN(what,0x088) = busmod_preset88;
    DEREF_INFN(what,0x089) = busmod_preset89;
    DEREF_INFN(what,0x08a) = busmod_preset8a;
    DEREF_INFN(what,0x08b) = busmod_preset8b;
    DEREF_INFN(what,0x08c) = busmod_preset8c;
    DEREF_INFN(what,0x08d) = busmod_preset8d;
    DEREF_INFN(what,0x08e) = busmod_preset8e;
    DEREF_INFN(what,0x08f) = busmod_preset8f;
    DEREF_INFN(what,0x090) = busmod_preset90;
    DEREF_INFN(what,0x091) = busmod_preset91;
    DEREF_INFN(what,0x092) = busmod_preset92;
    DEREF_INFN(what,0x093) = busmod_preset93;
    DEREF_INFN(what,0x094) = busmod_preset94;
    DEREF_INFN(what,0x095) = busmod_preset95;
    DEREF_INFN(what,0x096) = busmod_preset96;
    DEREF_INFN(what,0x097) = busmod_preset97;
    DEREF_INFN(what,0x098) = busmod_preset98;
    DEREF_INFN(what,0x099) = busmod_preset99;
    DEREF_INFN(what,0x09a) = busmod_preset9a;
    DEREF_INFN(what,0x09b) = busmod_preset9b;
    DEREF_INFN(what,0x09c) = busmod_preset9c;
    DEREF_INFN(what,0x09d) = busmod_preset9d;
    DEREF_INFN(what,0x09e) = busmod_preset9e;
    DEREF_INFN(what,0x09f) = busmod_preset9f;
    DEREF_INFN(what,0x0a0) = busmod_preseta0;
    DEREF_INFN(what,0x0a1) = busmod_preseta1;
    DEREF_INFN(what,0x0a2) = busmod_preseta2;
    DEREF_INFN(what,0x0a3) = busmod_preseta3;
    DEREF_INFN(what,0x0a4) = busmod_preseta4;
    DEREF_INFN(what,0x0a5) = busmod_preseta5;
    DEREF_INFN(what,0x0a6) = busmod_preseta6;
    DEREF_INFN(what,0x0a7) = busmod_preseta7;
    DEREF_INFN(what,0x0a8) = busmod_preseta8;
    DEREF_INFN(what,0x0a9) = busmod_preseta9;
    DEREF_INFN(what,0x0aa) = busmod_presetaa;
    DEREF_INFN(what,0x0ab) = busmod_presetab;
    DEREF_INFN(what,0x0ac) = busmod_presetac;
    DEREF_INFN(what,0x0ad) = busmod_presetad;
    DEREF_INFN(what,0x0ae) = busmod_presetae;
    DEREF_INFN(what,0x0af) = busmod_presetaf;
    DEREF_INFN(what,0x0b0) = busmod_presetb0;
    DEREF_INFN(what,0x0b1) = busmod_presetb1;
    DEREF_INFN(what,0x0b2) = busmod_presetb2;
    DEREF_INFN(what,0x0b3) = busmod_presetb3;
    DEREF_INFN(what,0x0b4) = busmod_presetb4;
    DEREF_INFN(what,0x0b5) = busmod_presetb5;
    DEREF_INFN(what,0x0b6) = busmod_presetb6;
    DEREF_INFN(what,0x0b7) = busmod_presetb7;
    DEREF_INFN(what,0x0b8) = busmod_presetb8;
    DEREF_INFN(what,0x0b9) = busmod_presetb9;
    DEREF_INFN(what,0x0ba) = busmod_presetba;
    DEREF_INFN(what,0x0bb) = busmod_presetbb;
    DEREF_INFN(what,0x0bc) = busmod_presetbc;
    DEREF_INFN(what,0x0bd) = busmod_presetbd;
    DEREF_INFN(what,0x0be) = busmod_presetbe;
    DEREF_INFN(what,0x0bf) = busmod_presetbf;
    DEREF_INFN(what,0x0c0) = busmod_presetc0;
    DEREF_INFN(what,0x0c1) = busmod_presetc1;
    DEREF_INFN(what,0x0c2) = busmod_presetc2;
    DEREF_INFN(what,0x0c3) = busmod_presetc3;
    DEREF_INFN(what,0x0c4) = busmod_presetc4;
    DEREF_INFN(what,0x0c5) = busmod_presetc5;
    DEREF_INFN(what,0x0c6) = busmod_presetc6;
    DEREF_INFN(what,0x0c7) = busmod_presetc7;
    DEREF_INFN(what,0x0c8) = busmod_presetc8;
    DEREF_INFN(what,0x0c9) = busmod_presetc9;
    DEREF_INFN(what,0x0ca) = busmod_presetca;
    DEREF_INFN(what,0x0cb) = busmod_presetcb;
    DEREF_INFN(what,0x0cc) = busmod_presetcc;
    DEREF_INFN(what,0x0cd) = busmod_presetcd;
    DEREF_INFN(what,0x0ce) = busmod_presetce;
    DEREF_INFN(what,0x0cf) = busmod_presetcf;
    DEREF_INFN(what,0x0d0) = busmod_presetd0;
    DEREF_INFN(what,0x0d1) = busmod_presetd1;
    DEREF_INFN(what,0x0d2) = busmod_presetd2;
    DEREF_INFN(what,0x0d3) = busmod_presetd3;
    DEREF_INFN(what,0x0d4) = busmod_presetd4;
    DEREF_INFN(what,0x0d5) = busmod_presetd5;
    DEREF_INFN(what,0x0d6) = busmod_presetd6;
    DEREF_INFN(what,0x0d7) = busmod_presetd7;
    DEREF_INFN(what,0x0d8) = busmod_presetd8;
    DEREF_INFN(what,0x0d9) = busmod_presetd9;
    DEREF_INFN(what,0x0da) = busmod_presetda;
    DEREF_INFN(what,0x0db) = busmod_presetdb;
    DEREF_INFN(what,0x0dc) = busmod_presetdc;
    DEREF_INFN(what,0x0dd) = busmod_presetdd;
    DEREF_INFN(what,0x0de) = busmod_presetde;
    DEREF_INFN(what,0x0df) = busmod_presetdf;
    DEREF_INFN(what,0x0e0) = busmod_presete0;
    DEREF_INFN(what,0x0e1) = busmod_presete1;
    DEREF_INFN(what,0x0e2) = busmod_presete2;
    DEREF_INFN(what,0x0e3) = busmod_presete3;
    DEREF_INFN(what,0x0e4) = busmod_presete4;
    DEREF_INFN(what,0x0e5) = busmod_presete5;
    DEREF_INFN(what,0x0e6) = busmod_presete6;
    DEREF_INFN(what,0x0e7) = busmod_presete7;
    DEREF_INFN(what,0x0e8) = busmod_presete8;
    DEREF_INFN(what,0x0e9) = busmod_presete9;
    DEREF_INFN(what,0x0ea) = busmod_presetea;
    DEREF_INFN(what,0x0eb) = busmod_preseteb;
    DEREF_INFN(what,0x0ec) = busmod_presetec;
    DEREF_INFN(what,0x0ed) = busmod_preseted;
    DEREF_INFN(what,0x0ee) = busmod_presetee;
    DEREF_INFN(what,0x0ef) = busmod_presetef;
    DEREF_INFN(what,0x0f0) = busmod_presetf0;
    DEREF_INFN(what,0x0f1) = busmod_presetf1;
    DEREF_INFN(what,0x0f2) = busmod_presetf2;
    DEREF_INFN(what,0x0f3) = busmod_presetf3;
    DEREF_INFN(what,0x0f4) = busmod_presetf4;
    DEREF_INFN(what,0x0f5) = busmod_presetf5;
    DEREF_INFN(what,0x0f6) = busmod_presetf6;
    DEREF_INFN(what,0x0f7) = busmod_presetf7;
    DEREF_INFN(what,0x0f8) = busmod_presetf8;
    DEREF_INFN(what,0x0f9) = busmod_presetf9;
    DEREF_INFN(what,0x0fa) = busmod_presetfa;
    DEREF_INFN(what,0x0fb) = busmod_presetfb;
    DEREF_INFN(what,0x0fc) = busmod_presetfc;
    DEREF_INFN(what,0x0fd) = busmod_presetfd;
    DEREF_INFN(what,0x0fe) = busmod_presetfe;
    DEREF_INFN(what,0x0ff) = busmod_presetff;

    DEREF_INFN(what,256) = busmod_reset;

    DEREF_INFN(what,257) = busmod_inc8;
    DEREF_INFN(what,258) = busmod_dec8;
    DEREF_INFN(what,259) = busmod_sr8;
    DEREF_INFN(what,260) = busmod_sl8;
    DEREF_INFN(what,261) = busmod_rr8;
    DEREF_INFN(what,262) = busmod_rl8;
    DEREF_INFN(what,263) = busmod_inc16;
    DEREF_INFN(what,264) = busmod_dec16;
    DEREF_INFN(what,265) = busmod_sr16;
    DEREF_INFN(what,266) = busmod_sl16;
    DEREF_INFN(what,267) = busmod_rr16;
    DEREF_INFN(what,268) = busmod_rl16;
    DEREF_INFN(what,269) = busmod_inc32;
    DEREF_INFN(what,270) = busmod_dec32;
    DEREF_INFN(what,271) = busmod_sr32;
    DEREF_INFN(what,272) = busmod_sl32;
    DEREF_INFN(what,273) = busmod_rr32;
    DEREF_INFN(what,274) = busmod_rl32;

    DEREF_INFN(what,275) = busmod_8set0;
    DEREF_INFN(what,276) = busmod_8set1;
    DEREF_INFN(what,277) = busmod_8set2;
    DEREF_INFN(what,278) = busmod_8set3;
    DEREF_INFN(what,279) = busmod_8set4;
    DEREF_INFN(what,280) = busmod_8set5;
    DEREF_INFN(what,281) = busmod_8set6;
    DEREF_INFN(what,282) = busmod_8set7;

    DEREF_INFN(what,283) = busmod_16set0;
    DEREF_INFN(what,284) = busmod_16set1;
    DEREF_INFN(what,285) = busmod_16set2;
    DEREF_INFN(what,286) = busmod_16set3;
    DEREF_INFN(what,287) = busmod_16set4;
    DEREF_INFN(what,288) = busmod_16set5;
    DEREF_INFN(what,289) = busmod_16set6;
    DEREF_INFN(what,290) = busmod_16set7;
    DEREF_INFN(what,291) = busmod_16set8;
    DEREF_INFN(what,292) = busmod_16set9;
    DEREF_INFN(what,293) = busmod_16seta;
    DEREF_INFN(what,294) = busmod_16setb;
    DEREF_INFN(what,295) = busmod_16setc;
    DEREF_INFN(what,296) = busmod_16setd;
    DEREF_INFN(what,297) = busmod_16sete;
    DEREF_INFN(what,298) = busmod_16setf;

    DEREF_INFN(what,299) = busmod_32set00;
    DEREF_INFN(what,300) = busmod_32set01;
    DEREF_INFN(what,301) = busmod_32set02;
    DEREF_INFN(what,302) = busmod_32set03;
    DEREF_INFN(what,303) = busmod_32set04;
    DEREF_INFN(what,304) = busmod_32set05;
    DEREF_INFN(what,305) = busmod_32set06;
    DEREF_INFN(what,306) = busmod_32set07;
    DEREF_INFN(what,307) = busmod_32set08;
    DEREF_INFN(what,308) = busmod_32set09;
    DEREF_INFN(what,309) = busmod_32set0a;
    DEREF_INFN(what,310) = busmod_32set0b;
    DEREF_INFN(what,311) = busmod_32set0c;
    DEREF_INFN(what,312) = busmod_32set0d;
    DEREF_INFN(what,313) = busmod_32set0e;
    DEREF_INFN(what,314) = busmod_32set0f;
    DEREF_INFN(what,315) = busmod_32set10;
    DEREF_INFN(what,316) = busmod_32set11;
    DEREF_INFN(what,317) = busmod_32set12;
    DEREF_INFN(what,318) = busmod_32set13;
    DEREF_INFN(what,319) = busmod_32set14;
    DEREF_INFN(what,320) = busmod_32set15;
    DEREF_INFN(what,321) = busmod_32set16;
    DEREF_INFN(what,322) = busmod_32set17;
    DEREF_INFN(what,323) = busmod_32set18;
    DEREF_INFN(what,324) = busmod_32set19;
    DEREF_INFN(what,325) = busmod_32set1a;
    DEREF_INFN(what,326) = busmod_32set1b;
    DEREF_INFN(what,327) = busmod_32set1c;
    DEREF_INFN(what,328) = busmod_32set1d;
    DEREF_INFN(what,329) = busmod_32set1e;
    DEREF_INFN(what,330) = busmod_32set1f;

    return 0;
}

void busmod_go(module_data *what)
{
    busmod_reset(what);

    return;
}

void busmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void busmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        if ( BUSMOD_BUS_8BIT(what) != NULL )
        {
            DEBFREE(BUSMOD_BUS_8BIT(what));
        }

        if ( BUSMOD_BUS_16BIT(what) != NULL )
        {
            DEBFREE(BUSMOD_BUS_16BIT(what));
        }

        if ( BUSMOD_BUS_32BIT(what) != NULL )
        {
            DEBFREE(BUSMOD_BUS_32BIT(what));
        }

        free_module_data(what);
    }

    return;
}

void busmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *busmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void busmod_reset(void *what)
{
    if ( BUSMOD_8RESTYPE(what) == 1 )
    {
        *BUSMOD_BUS_8BIT(what) = (UINT_8) rand();
    }

    else if ( BUSMOD_8RESTYPE(what) == 2 )
    {
        *BUSMOD_BUS_8BIT(what) = BUSMOD_8RESVAL(what);
    }

    if ( BUSMOD_16RESTYPE(what) == 1 )
    {
        *BUSMOD_BUS_16BIT(what) = (UINT_16) rand();
    }

    else if ( BUSMOD_16RESTYPE(what) == 2 )
    {
        *BUSMOD_BUS_16BIT(what) = BUSMOD_16RESVAL(what);
    }

    if ( BUSMOD_32RESTYPE(what) == 1 )
    {
        *BUSMOD_BUS_32BIT(what) = (UINT_32) rand();
    }

    else if ( BUSMOD_32RESTYPE(what) == 2 )
    {
        *BUSMOD_BUS_32BIT(what) = BUSMOD_32RESVAL(what);
    }

    return;
}

void busmod_8set0(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x001;

    return;
}

void busmod_8set1(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x002;

    return;
}

void busmod_8set2(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x004;

    return;
}

void busmod_8set3(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x008;

    return;
}

void busmod_8set4(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x010;

    return;
}

void busmod_8set5(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x020;

    return;
}

void busmod_8set6(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x040;

    return;
}

void busmod_8set7(void *what)
{
    (*BUSMOD_BUS_8BIT(what)) |= 0x080;

    return;
}


void busmod_16set0(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00001;

    return;
}

void busmod_16set1(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00002;

    return;
}

void busmod_16set2(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00004;

    return;
}

void busmod_16set3(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00008;

    return;
}

void busmod_16set4(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00010;

    return;
}

void busmod_16set5(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00020;

    return;
}

void busmod_16set6(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00040;

    return;
}

void busmod_16set7(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00080;

    return;
}

void busmod_16set8(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00100;

    return;
}

void busmod_16set9(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00200;

    return;
}

void busmod_16seta(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00400;

    return;
}

void busmod_16setb(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x00800;

    return;
}

void busmod_16setc(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x01000;

    return;
}

void busmod_16setd(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x02000;

    return;
}

void busmod_16sete(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x04000;

    return;
}

void busmod_16setf(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x08000;

    return;
}

void busmod_32set00(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000001;

    return;
}

void busmod_32set01(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000002;

    return;
}

void busmod_32set02(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000004;

    return;
}

void busmod_32set03(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000008;

    return;
}

void busmod_32set04(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000010;

    return;
}

void busmod_32set05(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000020;

    return;
}

void busmod_32set06(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000040;

    return;
}

void busmod_32set07(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000080;

    return;
}

void busmod_32set08(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000100;

    return;
}

void busmod_32set09(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000200;

    return;
}

void busmod_32set0a(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000400;

    return;
}

void busmod_32set0b(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000000800;

    return;
}

void busmod_32set0c(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000001000;

    return;
}

void busmod_32set0d(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000002000;

    return;
}

void busmod_32set0e(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000004000;

    return;
}

void busmod_32set0f(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000008000;

    return;
}

void busmod_32set10(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000010000;

    return;
}

void busmod_32set11(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000020000;

    return;
}

void busmod_32set12(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000040000;

    return;
}

void busmod_32set13(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000080000;

    return;
}

void busmod_32set14(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000100000;

    return;
}

void busmod_32set15(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000200000;

    return;
}

void busmod_32set16(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000400000;

    return;
}

void busmod_32set17(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x000800000;

    return;
}

void busmod_32set18(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x001000000;

    return;
}

void busmod_32set19(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x002000000;

    return;
}

void busmod_32set1a(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x004000000;

    return;
}

void busmod_32set1b(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x008000000;

    return;
}

void busmod_32set1c(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x010000000;

    return;
}

void busmod_32set1d(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x020000000;

    return;
}

void busmod_32set1e(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x040000000;

    return;
}

void busmod_32set1f(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) |= 0x080000000;

    return;
}

void busmod_inc8(void *what)
{
    (*BUSMOD_BUS_8BIT(what))++;

    return;
}

void busmod_inc16(void *what)
{
    (*BUSMOD_BUS_16BIT(what))++;

    return;
}

void busmod_inc32(void *what)
{
    (*BUSMOD_BUS_32BIT(what))++;

    return;
}

void busmod_dec8(void *what)
{
    (*BUSMOD_BUS_8BIT(what))--;

    return;
}

void busmod_dec16(void *what)
{
    (*BUSMOD_BUS_16BIT(what))--;

    return;
}

void busmod_dec32(void *what)
{
    (*BUSMOD_BUS_32BIT(what))--;

    return;
}

void busmod_sr8(void *what)
{
    (*BUSMOD_BUS_8BIT(what))  = ( (*BUSMOD_BUS_8BIT(what))  >> 1 ) & 0x07f;

    return;
}

void busmod_sr16(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) = ( (*BUSMOD_BUS_16BIT(what)) >> 1 ) & 0x07fff;

    return;
}

void busmod_sr32(void *what)
{
    (*BUSMOD_BUS_32BIT(what)) = ( (*BUSMOD_BUS_32BIT(what)) >> 1 ) & 0x07fffffff;

    return;
}

void busmod_sl8(void *what)
{
    (*BUSMOD_BUS_8BIT(what))  = (*BUSMOD_BUS_8BIT(what))  << 1;

    return;
}

void busmod_sl16(void *what)
{
    (*BUSMOD_BUS_16BIT(what)) = (*BUSMOD_BUS_16BIT(what)) << 1;

    return;
}

void busmod_sl32(void *what)
{
    (*BUSMOD_BUS_32BIT(what)) = (*BUSMOD_BUS_32BIT(what)) << 1;

    return;
}

void busmod_rr8(void *what)
{
    if ( (*BUSMOD_BUS_8BIT(what))  & 0x001 )
    {
        (*BUSMOD_BUS_8BIT(what))   = ( (*BUSMOD_BUS_8BIT(what))  >> 1 ) & 0x07f;
        (*BUSMOD_BUS_8BIT(what))  |= 0x080;
    }

    else
    {
        (*BUSMOD_BUS_8BIT(what))   = ( (*BUSMOD_BUS_8BIT(what))  >> 1 ) & 0x07f;
    }

    return;
}

void busmod_rr16(void *what)
{
    if ( (*BUSMOD_BUS_16BIT(what)) & 0x001 )
    {
        (*BUSMOD_BUS_16BIT(what))  = ( (*BUSMOD_BUS_16BIT(what)) >> 1 ) & 0x07fff;
        (*BUSMOD_BUS_16BIT(what)) |= 0x08000;
    }

    else
    {
        (*BUSMOD_BUS_16BIT(what))  = ( (*BUSMOD_BUS_16BIT(what)) >> 1 ) & 0x07fff;
    }

    return;
}

void busmod_rr32(void *what)
{
    if ( (*BUSMOD_BUS_32BIT(what)) & 0x001 )
    {
        (*BUSMOD_BUS_32BIT(what))  = ( (*BUSMOD_BUS_32BIT(what)) >> 1 ) & 0x07fffffff;
        (*BUSMOD_BUS_32BIT(what)) |= 0x080000000;
    }

    else
    {
        (*BUSMOD_BUS_32BIT(what))  = ( (*BUSMOD_BUS_32BIT(what)) >> 1 ) & 0x07fffffff;
    }

    return;
}

void busmod_rl8(void *what)
{
    if ( (*BUSMOD_BUS_8BIT(what))  & 0x080 )
    {
        (*BUSMOD_BUS_8BIT(what))   = (*BUSMOD_BUS_8BIT(what))  << 1;
        (*BUSMOD_BUS_8BIT(what))  |= 0x001;
    }

    else
    {
        (*BUSMOD_BUS_8BIT(what))   = (*BUSMOD_BUS_8BIT(what))  << 1;
    }

    return;
}

void busmod_rl16(void *what)
{
    if ( (*BUSMOD_BUS_16BIT(what)) & 0x08000 )
    {
        (*BUSMOD_BUS_16BIT(what))  = (*BUSMOD_BUS_16BIT(what)) << 1;
        (*BUSMOD_BUS_16BIT(what)) |= 0x001;
    }

    else
    {
        (*BUSMOD_BUS_16BIT(what))  = (*BUSMOD_BUS_16BIT(what)) << 1;
    }

    return;
}

void busmod_rl32(void *what)
{
    if ( (*BUSMOD_BUS_32BIT(what)) & 0x080000000 )
    {
        (*BUSMOD_BUS_32BIT(what))  = (*BUSMOD_BUS_32BIT(what)) << 1;
        (*BUSMOD_BUS_32BIT(what)) |= 0x001;
    }

    else
    {
        (*BUSMOD_BUS_32BIT(what))  = (*BUSMOD_BUS_32BIT(what)) << 1;
    }

    return;
}



void busmod_presetf0(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f0; return; }
void busmod_presetf1(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f1; return; }
void busmod_presetf2(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f2; return; }
void busmod_presetf3(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f3; return; }
void busmod_presetf4(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f4; return; }
void busmod_presetf5(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f5; return; }
void busmod_presetf6(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f6; return; }
void busmod_presetf7(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f7; return; }
void busmod_presetf8(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f8; return; }
void busmod_presetf9(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0f9; return; }
void busmod_presetfa(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0fa; return; }
void busmod_presetfb(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0fb; return; }
void busmod_presetfc(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0fc; return; }
void busmod_presetfd(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0fd; return; }
void busmod_presetfe(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0fe; return; }
void busmod_presetff(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ff; return; }
void busmod_presete0(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e0; return; }
void busmod_presete1(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e1; return; }
void busmod_presete2(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e2; return; }
void busmod_presete3(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e3; return; }
void busmod_presete4(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e4; return; }
void busmod_presete5(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e5; return; }
void busmod_presete6(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e6; return; }
void busmod_presete7(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e7; return; }
void busmod_presete8(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e8; return; }
void busmod_presete9(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0e9; return; }
void busmod_presetea(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ea; return; }
void busmod_preseteb(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0eb; return; }
void busmod_presetec(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ec; return; }
void busmod_preseted(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ed; return; }
void busmod_presetee(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ee; return; }
void busmod_presetef(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ef; return; }
void busmod_presetd0(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d0; return; }
void busmod_presetd1(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d1; return; }
void busmod_presetd2(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d2; return; }
void busmod_presetd3(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d3; return; }
void busmod_presetd4(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d4; return; }
void busmod_presetd5(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d5; return; }
void busmod_presetd6(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d6; return; }
void busmod_presetd7(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d7; return; }
void busmod_presetd8(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d8; return; }
void busmod_presetd9(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0d9; return; }
void busmod_presetda(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0da; return; }
void busmod_presetdb(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0db; return; }
void busmod_presetdc(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0dc; return; }
void busmod_presetdd(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0dd; return; }
void busmod_presetde(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0de; return; }
void busmod_presetdf(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0df; return; }
void busmod_presetc0(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c0; return; }
void busmod_presetc1(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c1; return; }
void busmod_presetc2(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c2; return; }
void busmod_presetc3(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c3; return; }
void busmod_presetc4(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c4; return; }
void busmod_presetc5(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c5; return; }
void busmod_presetc6(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c6; return; }
void busmod_presetc7(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c7; return; }
void busmod_presetc8(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c8; return; }
void busmod_presetc9(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0c9; return; }
void busmod_presetca(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ca; return; }
void busmod_presetcb(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0cb; return; }
void busmod_presetcc(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0cc; return; }
void busmod_presetcd(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0cd; return; }
void busmod_presetce(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ce; return; }
void busmod_presetcf(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0cf; return; }
void busmod_presetb0(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b0; return; }
void busmod_presetb1(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b1; return; }
void busmod_presetb2(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b2; return; }
void busmod_presetb3(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b3; return; }
void busmod_presetb4(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b4; return; }
void busmod_presetb5(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b5; return; }
void busmod_presetb6(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b6; return; }
void busmod_presetb7(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b7; return; }
void busmod_presetb8(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b8; return; }
void busmod_presetb9(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0b9; return; }
void busmod_presetba(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ba; return; }
void busmod_presetbb(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0bb; return; }
void busmod_presetbc(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0bc; return; }
void busmod_presetbd(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0bd; return; }
void busmod_presetbe(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0be; return; }
void busmod_presetbf(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0bf; return; }
void busmod_preseta0(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a0; return; }
void busmod_preseta1(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a1; return; }
void busmod_preseta2(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a2; return; }
void busmod_preseta3(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a3; return; }
void busmod_preseta4(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a4; return; }
void busmod_preseta5(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a5; return; }
void busmod_preseta6(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a6; return; }
void busmod_preseta7(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a7; return; }
void busmod_preseta8(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a8; return; }
void busmod_preseta9(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0a9; return; }
void busmod_presetaa(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0aa; return; }
void busmod_presetab(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ab; return; }
void busmod_presetac(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ac; return; }
void busmod_presetad(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ad; return; }
void busmod_presetae(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0ae; return; }
void busmod_presetaf(void *what) { *BUSMOD_BUS_8BIT(what) = 0x0af; return; }
void busmod_preset90(void *what) { *BUSMOD_BUS_8BIT(what) = 0x090; return; }
void busmod_preset91(void *what) { *BUSMOD_BUS_8BIT(what) = 0x091; return; }
void busmod_preset92(void *what) { *BUSMOD_BUS_8BIT(what) = 0x092; return; }
void busmod_preset93(void *what) { *BUSMOD_BUS_8BIT(what) = 0x093; return; }
void busmod_preset94(void *what) { *BUSMOD_BUS_8BIT(what) = 0x094; return; }
void busmod_preset95(void *what) { *BUSMOD_BUS_8BIT(what) = 0x095; return; }
void busmod_preset96(void *what) { *BUSMOD_BUS_8BIT(what) = 0x096; return; }
void busmod_preset97(void *what) { *BUSMOD_BUS_8BIT(what) = 0x097; return; }
void busmod_preset98(void *what) { *BUSMOD_BUS_8BIT(what) = 0x098; return; }
void busmod_preset99(void *what) { *BUSMOD_BUS_8BIT(what) = 0x099; return; }
void busmod_preset9a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x09a; return; }
void busmod_preset9b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x09b; return; }
void busmod_preset9c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x09c; return; }
void busmod_preset9d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x09d; return; }
void busmod_preset9e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x09e; return; }
void busmod_preset9f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x09f; return; }
void busmod_preset80(void *what) { *BUSMOD_BUS_8BIT(what) = 0x080; return; }
void busmod_preset81(void *what) { *BUSMOD_BUS_8BIT(what) = 0x081; return; }
void busmod_preset82(void *what) { *BUSMOD_BUS_8BIT(what) = 0x082; return; }
void busmod_preset83(void *what) { *BUSMOD_BUS_8BIT(what) = 0x083; return; }
void busmod_preset84(void *what) { *BUSMOD_BUS_8BIT(what) = 0x084; return; }
void busmod_preset85(void *what) { *BUSMOD_BUS_8BIT(what) = 0x085; return; }
void busmod_preset86(void *what) { *BUSMOD_BUS_8BIT(what) = 0x086; return; }
void busmod_preset87(void *what) { *BUSMOD_BUS_8BIT(what) = 0x087; return; }
void busmod_preset88(void *what) { *BUSMOD_BUS_8BIT(what) = 0x088; return; }
void busmod_preset89(void *what) { *BUSMOD_BUS_8BIT(what) = 0x089; return; }
void busmod_preset8a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x08a; return; }
void busmod_preset8b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x08b; return; }
void busmod_preset8c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x08c; return; }
void busmod_preset8d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x08d; return; }
void busmod_preset8e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x08e; return; }
void busmod_preset8f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x08f; return; }
void busmod_preset70(void *what) { *BUSMOD_BUS_8BIT(what) = 0x070; return; }
void busmod_preset71(void *what) { *BUSMOD_BUS_8BIT(what) = 0x071; return; }
void busmod_preset72(void *what) { *BUSMOD_BUS_8BIT(what) = 0x072; return; }
void busmod_preset73(void *what) { *BUSMOD_BUS_8BIT(what) = 0x073; return; }
void busmod_preset74(void *what) { *BUSMOD_BUS_8BIT(what) = 0x074; return; }
void busmod_preset75(void *what) { *BUSMOD_BUS_8BIT(what) = 0x075; return; }
void busmod_preset76(void *what) { *BUSMOD_BUS_8BIT(what) = 0x076; return; }
void busmod_preset77(void *what) { *BUSMOD_BUS_8BIT(what) = 0x077; return; }
void busmod_preset78(void *what) { *BUSMOD_BUS_8BIT(what) = 0x078; return; }
void busmod_preset79(void *what) { *BUSMOD_BUS_8BIT(what) = 0x079; return; }
void busmod_preset7a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x07a; return; }
void busmod_preset7b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x07b; return; }
void busmod_preset7c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x07c; return; }
void busmod_preset7d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x07d; return; }
void busmod_preset7e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x07e; return; }
void busmod_preset7f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x07f; return; }
void busmod_preset60(void *what) { *BUSMOD_BUS_8BIT(what) = 0x060; return; }
void busmod_preset61(void *what) { *BUSMOD_BUS_8BIT(what) = 0x061; return; }
void busmod_preset62(void *what) { *BUSMOD_BUS_8BIT(what) = 0x062; return; }
void busmod_preset63(void *what) { *BUSMOD_BUS_8BIT(what) = 0x063; return; }
void busmod_preset64(void *what) { *BUSMOD_BUS_8BIT(what) = 0x064; return; }
void busmod_preset65(void *what) { *BUSMOD_BUS_8BIT(what) = 0x065; return; }
void busmod_preset66(void *what) { *BUSMOD_BUS_8BIT(what) = 0x066; return; }
void busmod_preset67(void *what) { *BUSMOD_BUS_8BIT(what) = 0x067; return; }
void busmod_preset68(void *what) { *BUSMOD_BUS_8BIT(what) = 0x068; return; }
void busmod_preset69(void *what) { *BUSMOD_BUS_8BIT(what) = 0x069; return; }
void busmod_preset6a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x06a; return; }
void busmod_preset6b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x06b; return; }
void busmod_preset6c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x06c; return; }
void busmod_preset6d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x06d; return; }
void busmod_preset6e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x06e; return; }
void busmod_preset6f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x06f; return; }
void busmod_preset50(void *what) { *BUSMOD_BUS_8BIT(what) = 0x050; return; }
void busmod_preset51(void *what) { *BUSMOD_BUS_8BIT(what) = 0x051; return; }
void busmod_preset52(void *what) { *BUSMOD_BUS_8BIT(what) = 0x052; return; }
void busmod_preset53(void *what) { *BUSMOD_BUS_8BIT(what) = 0x053; return; }
void busmod_preset54(void *what) { *BUSMOD_BUS_8BIT(what) = 0x054; return; }
void busmod_preset55(void *what) { *BUSMOD_BUS_8BIT(what) = 0x055; return; }
void busmod_preset56(void *what) { *BUSMOD_BUS_8BIT(what) = 0x056; return; }
void busmod_preset57(void *what) { *BUSMOD_BUS_8BIT(what) = 0x057; return; }
void busmod_preset58(void *what) { *BUSMOD_BUS_8BIT(what) = 0x058; return; }
void busmod_preset59(void *what) { *BUSMOD_BUS_8BIT(what) = 0x059; return; }
void busmod_preset5a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x05a; return; }
void busmod_preset5b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x05b; return; }
void busmod_preset5c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x05c; return; }
void busmod_preset5d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x05d; return; }
void busmod_preset5e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x05e; return; }
void busmod_preset5f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x05f; return; }
void busmod_preset40(void *what) { *BUSMOD_BUS_8BIT(what) = 0x040; return; }
void busmod_preset41(void *what) { *BUSMOD_BUS_8BIT(what) = 0x041; return; }
void busmod_preset42(void *what) { *BUSMOD_BUS_8BIT(what) = 0x042; return; }
void busmod_preset43(void *what) { *BUSMOD_BUS_8BIT(what) = 0x043; return; }
void busmod_preset44(void *what) { *BUSMOD_BUS_8BIT(what) = 0x044; return; }
void busmod_preset45(void *what) { *BUSMOD_BUS_8BIT(what) = 0x045; return; }
void busmod_preset46(void *what) { *BUSMOD_BUS_8BIT(what) = 0x046; return; }
void busmod_preset47(void *what) { *BUSMOD_BUS_8BIT(what) = 0x047; return; }
void busmod_preset48(void *what) { *BUSMOD_BUS_8BIT(what) = 0x048; return; }
void busmod_preset49(void *what) { *BUSMOD_BUS_8BIT(what) = 0x049; return; }
void busmod_preset4a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x04a; return; }
void busmod_preset4b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x04b; return; }
void busmod_preset4c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x04c; return; }
void busmod_preset4d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x04d; return; }
void busmod_preset4e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x04e; return; }
void busmod_preset4f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x04f; return; }
void busmod_preset30(void *what) { *BUSMOD_BUS_8BIT(what) = 0x030; return; }
void busmod_preset31(void *what) { *BUSMOD_BUS_8BIT(what) = 0x031; return; }
void busmod_preset32(void *what) { *BUSMOD_BUS_8BIT(what) = 0x032; return; }
void busmod_preset33(void *what) { *BUSMOD_BUS_8BIT(what) = 0x033; return; }
void busmod_preset34(void *what) { *BUSMOD_BUS_8BIT(what) = 0x034; return; }
void busmod_preset35(void *what) { *BUSMOD_BUS_8BIT(what) = 0x035; return; }
void busmod_preset36(void *what) { *BUSMOD_BUS_8BIT(what) = 0x036; return; }
void busmod_preset37(void *what) { *BUSMOD_BUS_8BIT(what) = 0x037; return; }
void busmod_preset38(void *what) { *BUSMOD_BUS_8BIT(what) = 0x038; return; }
void busmod_preset39(void *what) { *BUSMOD_BUS_8BIT(what) = 0x039; return; }
void busmod_preset3a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x03a; return; }
void busmod_preset3b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x03b; return; }
void busmod_preset3c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x03c; return; }
void busmod_preset3d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x03d; return; }
void busmod_preset3e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x03e; return; }
void busmod_preset3f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x03f; return; }
void busmod_preset20(void *what) { *BUSMOD_BUS_8BIT(what) = 0x020; return; }
void busmod_preset21(void *what) { *BUSMOD_BUS_8BIT(what) = 0x021; return; }
void busmod_preset22(void *what) { *BUSMOD_BUS_8BIT(what) = 0x022; return; }
void busmod_preset23(void *what) { *BUSMOD_BUS_8BIT(what) = 0x023; return; }
void busmod_preset24(void *what) { *BUSMOD_BUS_8BIT(what) = 0x024; return; }
void busmod_preset25(void *what) { *BUSMOD_BUS_8BIT(what) = 0x025; return; }
void busmod_preset26(void *what) { *BUSMOD_BUS_8BIT(what) = 0x026; return; }
void busmod_preset27(void *what) { *BUSMOD_BUS_8BIT(what) = 0x027; return; }
void busmod_preset28(void *what) { *BUSMOD_BUS_8BIT(what) = 0x028; return; }
void busmod_preset29(void *what) { *BUSMOD_BUS_8BIT(what) = 0x029; return; }
void busmod_preset2a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x02a; return; }
void busmod_preset2b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x02b; return; }
void busmod_preset2c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x02c; return; }
void busmod_preset2d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x02d; return; }
void busmod_preset2e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x02e; return; }
void busmod_preset2f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x02f; return; }
void busmod_preset10(void *what) { *BUSMOD_BUS_8BIT(what) = 0x010; return; }
void busmod_preset11(void *what) { *BUSMOD_BUS_8BIT(what) = 0x011; return; }
void busmod_preset12(void *what) { *BUSMOD_BUS_8BIT(what) = 0x012; return; }
void busmod_preset13(void *what) { *BUSMOD_BUS_8BIT(what) = 0x013; return; }
void busmod_preset14(void *what) { *BUSMOD_BUS_8BIT(what) = 0x014; return; }
void busmod_preset15(void *what) { *BUSMOD_BUS_8BIT(what) = 0x015; return; }
void busmod_preset16(void *what) { *BUSMOD_BUS_8BIT(what) = 0x016; return; }
void busmod_preset17(void *what) { *BUSMOD_BUS_8BIT(what) = 0x017; return; }
void busmod_preset18(void *what) { *BUSMOD_BUS_8BIT(what) = 0x018; return; }
void busmod_preset19(void *what) { *BUSMOD_BUS_8BIT(what) = 0x019; return; }
void busmod_preset1a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x01a; return; }
void busmod_preset1b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x01b; return; }
void busmod_preset1c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x01c; return; }
void busmod_preset1d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x01d; return; }
void busmod_preset1e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x01e; return; }
void busmod_preset1f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x01f; return; }
void busmod_preset00(void *what) { *BUSMOD_BUS_8BIT(what) = 0x000; return; }
void busmod_preset01(void *what) { *BUSMOD_BUS_8BIT(what) = 0x001; return; }
void busmod_preset02(void *what) { *BUSMOD_BUS_8BIT(what) = 0x002; return; }
void busmod_preset03(void *what) { *BUSMOD_BUS_8BIT(what) = 0x003; return; }
void busmod_preset04(void *what) { *BUSMOD_BUS_8BIT(what) = 0x004; return; }
void busmod_preset05(void *what) { *BUSMOD_BUS_8BIT(what) = 0x005; return; }
void busmod_preset06(void *what) { *BUSMOD_BUS_8BIT(what) = 0x006; return; }
void busmod_preset07(void *what) { *BUSMOD_BUS_8BIT(what) = 0x007; return; }
void busmod_preset08(void *what) { *BUSMOD_BUS_8BIT(what) = 0x008; return; }
void busmod_preset09(void *what) { *BUSMOD_BUS_8BIT(what) = 0x009; return; }
void busmod_preset0a(void *what) { *BUSMOD_BUS_8BIT(what) = 0x00a; return; }
void busmod_preset0b(void *what) { *BUSMOD_BUS_8BIT(what) = 0x00b; return; }
void busmod_preset0c(void *what) { *BUSMOD_BUS_8BIT(what) = 0x00c; return; }
void busmod_preset0d(void *what) { *BUSMOD_BUS_8BIT(what) = 0x00d; return; }
void busmod_preset0e(void *what) { *BUSMOD_BUS_8BIT(what) = 0x00e; return; }
void busmod_preset0f(void *what) { *BUSMOD_BUS_8BIT(what) = 0x00f; return; }





/**********************************************************************

Functional module 2: mem

**********************************************************************/

typedef struct
{
    UINT_8  alias_8;
    UINT_16 alias_16;
    UINT_32 alias_32;
}
memmod_state;

#define MEMMOD_RESET_TYPE(what) DEREF_8VAR(what,0)
#define MEMMOD_RESET_VAL(what)  DEREF_8VAR(what,1)
#define MEMMOD_RAWSIZE(what)    DEREF_32VAR(what,0)
#define MEMMOD_RAWMASK(what)    DEREF_32VAR(what,1)

#define MEMMOD_DATA(what)       DEREF_8BUS(what,0)
#define MEMMOD_ADDR_8(what)     DEREF_8BUS(what,1)
#define MEMMOD_ADDR_16(what)    DEREF_16BUS(what,0)
#define MEMMOD_ADDR_32(what)    DEREF_32BUS(what,0)

#define MEMMOD_MASK_8(what)     (((memmod_state *) DEREF_INTERNAL(what))->alias_8)
#define MEMMOD_MASK_16(what)    (((memmod_state *) DEREF_INTERNAL(what))->alias_16)
#define MEMMOD_MASK_32(what)    (((memmod_state *) DEREF_INTERNAL(what))->alias_32)

#define MEMMOD_MEMCONTENT(what) DEREF_8MEM(what,2)
#define MEMMOD_MEMCON(what,i)   DEREF_8MEM(what,2+i)

#define MEMMOD_ROMNAME(what)    DEREF_STRGVAR(what,0)

void memmod_reset(void *what);

void memmod_and_write8(void *what);
void memmod_and_write16(void *what);
void memmod_and_write32(void *what);

void memmod_and_read8(void *what);
void memmod_and_read16(void *what);
void memmod_and_read32(void *what);

void memmod_and_inc8(void *what);
void memmod_and_inc16(void *what);
void memmod_and_inc32(void *what);

void memmod_and_dec8(void *what);
void memmod_and_dec16(void *what);
void memmod_and_dec32(void *what);

module_data *memmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data_varonly(module_name,0,2,0,2,1);

    if ( ( DEREF_INTERNAL(what) = (void *) DEBMALLOC(sizeof(memmod_state)) ) == NULL )
    {
        return NULL;
    }

    MEMMOD_RESET_TYPE(what) = 2;
    MEMMOD_RESET_VAL(what)  = 0;
    MEMMOD_RAWSIZE(what)    = 0;
    MEMMOD_RAWMASK(what)    = 0;

    return what;
}

int memmod_init(module_data *what)
{
    int result = 1;
    UINT_64 i;
    PC_FILE *fp;

    if ( ( what = gen_module_data_nonvaronly(what,0,2+((UINT_64) MEMMOD_RAWSIZE(what)),1,1,13,0) ) != NULL )
    {
        DEREF_INFN(what,0)  = memmod_reset;
        DEREF_INFN(what,1)  = memmod_and_write8;
        DEREF_INFN(what,2)  = memmod_and_write16;
        DEREF_INFN(what,3)  = memmod_and_write32;
        DEREF_INFN(what,4)  = memmod_and_read8;
        DEREF_INFN(what,5)  = memmod_and_read16;
        DEREF_INFN(what,6)  = memmod_and_read32;
        DEREF_INFN(what,7)  = memmod_and_inc8;
        DEREF_INFN(what,8)  = memmod_and_inc16;
        DEREF_INFN(what,9)  = memmod_and_inc32;
        DEREF_INFN(what,10) = memmod_and_dec8;
        DEREF_INFN(what,11) = memmod_and_dec16;
        DEREF_INFN(what,12) = memmod_and_dec32;

        if ( ( MEMMOD_RAWSIZE(what) == 0x000000000 ) ||   /* 0  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000000001 ) ||   /* 1  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000000003 ) ||   /* 2  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000000007 ) ||   /* 3  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00000000f ) ||   /* 4  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00000001f ) ||   /* 5  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00000003f ) ||   /* 6  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00000007f ) ||   /* 7  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0000000ff ) ||   /* 8  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0000001ff ) ||   /* 9  bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0000003ff ) ||   /* 10 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0000007ff ) ||   /* 11 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000000fff ) ||   /* 12 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000001fff ) ||   /* 13 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000003fff ) ||   /* 14 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000007fff ) ||   /* 15 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00000ffff ) ||   /* 16 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00001ffff ) ||   /* 17 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00003ffff ) ||   /* 18 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00007ffff ) ||   /* 19 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0000fffff ) ||   /* 20 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0001fffff ) ||   /* 21 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0003fffff ) ||   /* 22 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0007fffff ) ||   /* 23 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x000ffffff ) ||   /* 24 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x001ffffff ) ||   /* 25 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x003ffffff ) ||   /* 26 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x007ffffff ) ||   /* 27 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x00fffffff ) ||   /* 28 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x01fffffff ) ||   /* 29 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x03fffffff ) ||   /* 30 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x07fffffff ) ||   /* 31 bit address */
             ( MEMMOD_RAWSIZE(what) == 0x0ffffffff )    ) /* 32 bit address */
        {
            MEMMOD_MASK_8(what)  = (UINT_8)  (MEMMOD_RAWSIZE(what) & MEMMOD_RAWMASK(what) & 0x0000000ff);
            MEMMOD_MASK_16(what) = (UINT_16) (MEMMOD_RAWSIZE(what) & MEMMOD_RAWMASK(what) & 0x00000ffff);
            MEMMOD_MASK_32(what) = (UINT_32) (MEMMOD_RAWSIZE(what) & MEMMOD_RAWMASK(what) & 0x0ffffffff);

            if ( ( MEMMOD_MEMCONTENT(what) = (UINT_8 *) DEBMALLOC(((((UINT_64) MEMMOD_RAWSIZE(what))+0x10))*sizeof(UINT_8)) ) != NULL )
            {
                for ( i = 0 ; i < MEMMOD_RAWSIZE(what) ; i++ )
                {
                    MEMMOD_MEMCON(what,i) = &(DEBDEREF((MEMMOD_MEMCONTENT(what)),i));
                }

                /*
                   Test to see if ROM
                */

                if ( strlen(MEMMOD_ROMNAME(what)) > 0 )
                {
                    /*
                       This is a ROM type memory block.
                    */

                    MEMMOD_RESET_TYPE(what) = 0; /* reset has no effect on ROM */

                    if ( ( fp = pc_fopen(MEMMOD_ROMNAME(what),"rb") ) != NULL )
                    {
                        for ( i = 0 ; i < MEMMOD_RAWSIZE(what) ; i++ )
                        {
                            DEBDEREF((MEMMOD_MEMCONTENT(what)),i) = pc_fgetc(fp);
                        }

                        pc_fclose(fp);

                        result = 0;
                    }
                }

                else
                {
                    result = 0;
                }
            }
        }
    }

    return result;
}

void memmod_go(module_data *what)
{
    memmod_reset(what);

    return;
}

void memmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void memmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        if ( DEREF_INTERNAL(what) != NULL )
        {
            if ( MEMMOD_MEMCONTENT(what) != NULL )
            {
                DEBFREE(MEMMOD_MEMCONTENT(what));
            }

            DEBFREE(DEREF_INTERNAL(what));
        }

        free_module_data(what);
    }

    return;
}

void memmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *memmod_getinf(module_data *what)
{
    char *dest;
    char test[80];
    UINT_64 i,j,k;

    if ( ( dest = (char *) DEBMALLOC((((((MEMMOD_RAWSIZE(what)+1)/0x10)+1)*50)+70)*sizeof(char)) ) != NULL )
    {
        sprintf(dest,"  ==== Memory dump (%02x,%02x,%04x) ====  \n", (int) MEMMOD_RESET_TYPE(what), (int) MEMMOD_RESET_VAL(what), (int) MEMMOD_RAWSIZE(what));

        for ( i = 0 ; i < MEMMOD_RAWSIZE(what) ; i += 0x10 )
        {
            /*
               Note that i+0x10 may overflow, so use sub on rightside to
               ensure this doesn't happen.  This however may cause
               underflow, which the second condition will catch.
            */

            if ( ( i <= MEMMOD_RAWSIZE(what)-0x10 ) || ( MEMMOD_RAWSIZE(what) < 0x10 ) )
            {
                k = 0x10;
            }

            else
            {
                k = MEMMOD_RAWSIZE(what)-i;
            }

            for ( j = 0 ; j < k ; j++ )
            {
                sprintf(test,"%02x ",(int) DEBDEREF(MEMMOD_MEMCONTENT(what),(i+j)));
                strcat(dest,test);
            }

            strcat(dest,"\n");
        }

        {
            if ( ( i <= MEMMOD_RAWSIZE(what)-0x10 ) || ( MEMMOD_RAWSIZE(what) < 0x10 ) )
            {
                k = 0x10;
            }

            else
            {
                k = MEMMOD_RAWSIZE(what)-i;
            }

            for ( j = 0 ; j < k ; j++ )
            {
                sprintf(test,"%02x ",(int) DEBDEREF(MEMMOD_MEMCONTENT(what),(i+j)));
                strcat(dest,test);
            }

            strcat(dest,"\n");
        }
    }

    return dest;
}

void memmod_reset(void *what)
{
    UINT_64 i;

    switch ( MEMMOD_RESET_TYPE(what) )
    {
        case 1:
        {
            for ( i = 0 ; i < MEMMOD_RAWSIZE(what) ; i++ )
            {
                DEBDEREF(MEMMOD_MEMCONTENT(what),i) = (UINT_8) rand();
            }

            {
                DEBDEREF(MEMMOD_MEMCONTENT(what),i) = (UINT_8) rand();
            }

            break;
        }

        case 2:
        {
            for ( i = 0 ; i < MEMMOD_RAWSIZE(what) ; i++ )
            {
                DEBDEREF(MEMMOD_MEMCONTENT(what),i) = MEMMOD_RESET_VAL(what);
            }

            {
                DEBDEREF(MEMMOD_MEMCONTENT(what),i) = MEMMOD_RESET_VAL(what);
            }

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void memmod_and_write8(void *what)
{
    DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_8(what)&MEMMOD_MASK_8(what))) = MEMMOD_DATA(what);

    return;
}

void memmod_and_write16(void *what)
{
    DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_16(what)&MEMMOD_MASK_16(what))) = MEMMOD_DATA(what);

    return;
}

void memmod_and_write32(void *what)
{
    DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_32(what)&MEMMOD_MASK_32(what))) = MEMMOD_DATA(what);

    return;
}

void memmod_and_read8(void *what)
{
    MEMMOD_DATA(what) = DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_8(what)&MEMMOD_MASK_8(what)));

    return;
}

void memmod_and_read16(void *what)
{
    MEMMOD_DATA(what) = DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_16(what)&MEMMOD_MASK_16(what)));

    return;
}

void memmod_and_read32(void *what)
{
    MEMMOD_DATA(what) = DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_32(what)&MEMMOD_MASK_32(what)));

    return;
}

void memmod_and_inc8(void *what)
{
    (DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_8(what)&MEMMOD_MASK_8(what))))++;

    return;
}

void memmod_and_inc16(void *what)
{
    (DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_16(what)&MEMMOD_MASK_16(what))))++;

    return;
}

void memmod_and_inc32(void *what)
{
    (DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_32(what)&MEMMOD_MASK_32(what))))++;

    return;
}

void memmod_and_dec8(void *what)
{
    (DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_8(what)&MEMMOD_MASK_8(what))))--;

    return;
}

void memmod_and_dec16(void *what)
{
    (DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_16(what)&MEMMOD_MASK_16(what))))--;

    return;
}

void memmod_and_dec32(void *what)
{
    (DEBDEREF(MEMMOD_MEMCONTENT(what),(MEMMOD_ADDR_32(what)&MEMMOD_MASK_32(what))))--;

    return;
}





/**********************************************************************

Functional module 3: setbus

**********************************************************************/

/* Number of busses available to choose from. */

#define SETBUSMOD_NUM8(what)    DEREF_32VAR(what,0)
#define SETBUSMOD_NUM16(what)   DEREF_32VAR(what,1)
#define SETBUSMOD_NUM32(what)   DEREF_32VAR(what,2)

/* Which bus number in the relevant module is to be set. */

#define SETBUSMOD_MARK8(what)   DEREF_32VAR(what,3)
#define SETBUSMOD_MARK16(what)  DEREF_32VAR(what,4)
#define SETBUSMOD_MARK32(what)  DEREF_32VAR(what,5)

/* Bus which tells us which bus should be selected.
   xbussely means "using this x-bit bus, choose the y-bit bus" */

#define SETBUSMOD_8BUSEL8(what)    DEREF_8BUS(what,0)
#define SETBUSMOD_8BUSEL16(what)   DEREF_8BUS(what,1)
#define SETBUSMOD_8BUSEL32(what)   DEREF_8BUS(what,2)

#define SETBUSMOD_16BUSEL8(what)   DEREF_16BUS(what,0)
#define SETBUSMOD_16BUSEL16(what)  DEREF_16BUS(what,1)
#define SETBUSMOD_16BUSEL32(what)  DEREF_16BUS(what,2)

#define SETBUSMOD_32BUSEL8(what)   DEREF_32BUS(what,0)
#define SETBUSMOD_32BUSEL16(what)  DEREF_32BUS(what,1)
#define SETBUSMOD_32BUSEL32(what)  DEREF_32BUS(what,2)

/* Pointers to the bus being selected. */

#define SETBUSMOD_BUSTOSET8(what)  DEREF_8MEM(DEREF_MODPTR(what,0),SETBUSMOD_MARK8(what))
#define SETBUSMOD_BUSTOSET16(what) DEREF_16MEM(DEREF_MODPTR(what,1),SETBUSMOD_MARK16(what))
#define SETBUSMOD_BUSTOSET32(what) DEREF_32MEM(DEREF_MODPTR(what,2),SETBUSMOD_MARK32(what))

/* Pointers to buses selected by SETBUSMOD_xBUSELy */

#define SETBUSMOD_8BUSVAL8(what)   DEREF_8MEM(what,3+SETBUSMOD_8BUSEL8(what))
#define SETBUSMOD_8BUSVAL16(what)  DEREF_16MEM(what,3+SETBUSMOD_8BUSEL16(what))
#define SETBUSMOD_8BUSVAL32(what)  DEREF_32MEM(what,3+SETBUSMOD_8BUSEL32(what))

#define SETBUSMOD_16BUSVAL8(what)  DEREF_8MEM(what,3+SETBUSMOD_16BUSEL8(what))
#define SETBUSMOD_16BUSVAL16(what) DEREF_16MEM(what,3+SETBUSMOD_16BUSEL16(what))
#define SETBUSMOD_16BUSVAL32(what) DEREF_32MEM(what,3+SETBUSMOD_16BUSEL32(what))

#define SETBUSMOD_32BUSVAL8(what)  DEREF_8MEM(what,3+SETBUSMOD_32BUSEL8(what))
#define SETBUSMOD_32BUSVAL16(what) DEREF_16MEM(what,3+SETBUSMOD_32BUSEL16(what))
#define SETBUSMOD_32BUSVAL32(what) DEREF_32MEM(what,3+SETBUSMOD_32BUSEL32(what))

void setbusmod_8setbus8(void *what);
void setbusmod_8setbus16(void *what);
void setbusmod_8setbus32(void *what);
void setbusmod_16setbus8(void *what);
void setbusmod_16setbus16(void *what);
void setbusmod_16setbus32(void *what);
void setbusmod_32setbus8(void *what);
void setbusmod_32setbus16(void *what);
void setbusmod_32setbus32(void *what);

module_data *setbusmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data_varonly(module_name,0,0,0,6,0);

    return what;
}

int setbusmod_init(module_data *what)
{
    int result = 1;

    if ( ( what = gen_module_data_nonvaronly(what,3,3+SETBUSMOD_NUM8(what),3+SETBUSMOD_NUM16(what),3+SETBUSMOD_NUM32(what),9,0) ) != NULL )
    {
        DEREF_INFN(what,0) = setbusmod_8setbus8;
        DEREF_INFN(what,1) = setbusmod_8setbus16;
        DEREF_INFN(what,2) = setbusmod_8setbus32;
        DEREF_INFN(what,3) = setbusmod_16setbus8;
        DEREF_INFN(what,4) = setbusmod_16setbus16;
        DEREF_INFN(what,5) = setbusmod_16setbus32;
        DEREF_INFN(what,6) = setbusmod_32setbus8;
        DEREF_INFN(what,7) = setbusmod_32setbus16;
        DEREF_INFN(what,8) = setbusmod_32setbus32;

        result = 0;
    }

    return result;
}

void setbusmod_go(module_data *what)
{
    return;

    what = NULL;
}

void setbusmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void setbusmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void setbusmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *setbusmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void setbusmod_8setbus8(void *what)
{
    if ( SETBUSMOD_8BUSEL8(what) < SETBUSMOD_NUM8(what) )
    {
        SETBUSMOD_BUSTOSET8(what) = SETBUSMOD_8BUSVAL8(what);
    }

    return;
}

void setbusmod_8setbus16(void *what)
{
    if ( SETBUSMOD_8BUSEL16(what) < SETBUSMOD_NUM16(what) )
    {
        SETBUSMOD_BUSTOSET16(what) = SETBUSMOD_8BUSVAL16(what);
    }

    return;
}

void setbusmod_8setbus32(void *what)
{
    if ( SETBUSMOD_8BUSEL16(what) < SETBUSMOD_NUM16(what) )
    {
        SETBUSMOD_BUSTOSET32(what) = SETBUSMOD_8BUSVAL32(what);
    }

    return;
}

void setbusmod_16setbus8(void *what)
{
    if ( SETBUSMOD_16BUSEL8(what) < SETBUSMOD_NUM8(what) )
    {
        SETBUSMOD_BUSTOSET8(what) = SETBUSMOD_16BUSVAL8(what);
    }

    return;
}

void setbusmod_16setbus16(void *what)
{
    if ( SETBUSMOD_16BUSEL16(what) < SETBUSMOD_NUM16(what) )
    {
        SETBUSMOD_BUSTOSET16(what) = SETBUSMOD_16BUSVAL16(what);
    }

    return;
}

void setbusmod_16setbus32(void *what)
{
    if ( SETBUSMOD_16BUSEL16(what) < SETBUSMOD_NUM16(what) )
    {
        SETBUSMOD_BUSTOSET32(what) = SETBUSMOD_16BUSVAL32(what);
    }

    return;
}

void setbusmod_32setbus8(void *what)
{
    if ( SETBUSMOD_32BUSEL8(what) < SETBUSMOD_NUM8(what) )
    {
        SETBUSMOD_BUSTOSET8(what) = SETBUSMOD_32BUSVAL8(what);
    }

    return;
}

void setbusmod_32setbus16(void *what)
{
    if ( SETBUSMOD_32BUSEL16(what) < SETBUSMOD_NUM16(what) )
    {
        SETBUSMOD_BUSTOSET16(what) = SETBUSMOD_32BUSVAL16(what);
    }

    return;
}

void setbusmod_32setbus32(void *what)
{
    if ( SETBUSMOD_32BUSEL16(what) < SETBUSMOD_NUM16(what) )
    {
        SETBUSMOD_BUSTOSET32(what) = SETBUSMOD_32BUSVAL32(what);
    }

    return;
}




/**********************************************************************

Branch module 1: istrue

**********************************************************************/

#define ISTRUEMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define ISTRUEMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define ISTRUEMOD_LEFT32(what)    DEREF_32BUS(what,0)

void istruemod_branch8(void *what);
void istruemod_branch16(void *what);
void istruemod_branch32(void *what);

module_data *istruemod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,1,1,1,3,6);

    return what;
}

int istruemod_init(module_data *what)
{
    DEREF_INFN(what,0) = istruemod_branch8;
    DEREF_INFN(what,1) = istruemod_branch16;
    DEREF_INFN(what,2) = istruemod_branch32;

    return 0;
}

void istruemod_go(module_data *what)
{
    return;

    what = NULL;
}

void istruemod_stop(module_data *what)
{
    return;

    what = NULL;
}

void istruemod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void istruemod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *istruemod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void istruemod_branch8(void *what)
{
    if ( ISTRUEMOD_LEFT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void istruemod_branch16(void *what)
{
    if ( ISTRUEMOD_LEFT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void istruemod_branch32(void *what)
{
    if ( ISTRUEMOD_LEFT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}




/**********************************************************************

Branch module 2: equals

**********************************************************************/

#define EQUALSMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define EQUALSMOD_RIGHT8(what)    DEREF_8BUS(what,1)
#define EQUALSMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define EQUALSMOD_RIGHT16(what)   DEREF_16BUS(what,1)
#define EQUALSMOD_LEFT32(what)    DEREF_32BUS(what,0)
#define EQUALSMOD_RIGHT32(what)   DEREF_32BUS(what,1)

void equalsmod_branch8(void *what);
void equalsmod_branch16(void *what);
void equalsmod_branch32(void *what);

module_data *equalsmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,2,2,2,3,6);

    return what;
}

int equalsmod_init(module_data *what)
{
    DEREF_INFN(what,0) = equalsmod_branch8;
    DEREF_INFN(what,1) = equalsmod_branch16;
    DEREF_INFN(what,2) = equalsmod_branch32;

    return 0;
}

void equalsmod_go(module_data *what)
{
    return;

    what = NULL;
}

void equalsmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void equalsmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void equalsmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *equalsmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void equalsmod_branch8(void *what)
{
    if ( EQUALSMOD_LEFT8(what) == EQUALSMOD_RIGHT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void equalsmod_branch16(void *what)
{
    if ( EQUALSMOD_LEFT16(what) == EQUALSMOD_RIGHT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void equalsmod_branch32(void *what)
{
    if ( EQUALSMOD_LEFT32(what) == EQUALSMOD_RIGHT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}




/**********************************************************************

Branch module 3: greater

**********************************************************************/

#define GREATERMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define GREATERMOD_RIGHT8(what)    DEREF_8BUS(what,1)
#define GREATERMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define GREATERMOD_RIGHT16(what)   DEREF_16BUS(what,1)
#define GREATERMOD_LEFT32(what)    DEREF_32BUS(what,0)
#define GREATERMOD_RIGHT32(what)   DEREF_32BUS(what,1)

void greatermod_branch8(void *what);
void greatermod_branch16(void *what);
void greatermod_branch32(void *what);

module_data *greatermod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,2,2,2,3,6);

    return what;
}

int greatermod_init(module_data *what)
{
    DEREF_INFN(what,0) = greatermod_branch8;
    DEREF_INFN(what,1) = greatermod_branch16;
    DEREF_INFN(what,2) = greatermod_branch32;

    return 0;
}

void greatermod_go(module_data *what)
{
    return;

    what = NULL;
}

void greatermod_stop(module_data *what)
{
    return;

    what = NULL;
}

void greatermod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void greatermod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *greatermod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void greatermod_branch8(void *what)
{
    if ( GREATERMOD_LEFT8(what) > GREATERMOD_RIGHT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void greatermod_branch16(void *what)
{
    if ( GREATERMOD_LEFT16(what) > GREATERMOD_RIGHT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void greatermod_branch32(void *what)
{
    if ( GREATERMOD_LEFT32(what) > GREATERMOD_RIGHT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}



/**********************************************************************

Branch module 4: less

**********************************************************************/

#define LESSMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define LESSMOD_RIGHT8(what)    DEREF_8BUS(what,1)
#define LESSMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define LESSMOD_RIGHT16(what)   DEREF_16BUS(what,1)
#define LESSMOD_LEFT32(what)    DEREF_32BUS(what,0)
#define LESSMOD_RIGHT32(what)   DEREF_32BUS(what,1)

void lessmod_branch8(void *what);
void lessmod_branch16(void *what);
void lessmod_branch32(void *what);

module_data *lessmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,2,2,2,3,6);

    return what;
}

int lessmod_init(module_data *what)
{
    DEREF_INFN(what,0) = lessmod_branch8;
    DEREF_INFN(what,1) = lessmod_branch16;
    DEREF_INFN(what,2) = lessmod_branch32;

    return 0;
}

void lessmod_go(module_data *what)
{
    return;

    what = NULL;
}

void lessmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void lessmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void lessmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *lessmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void lessmod_branch8(void *what)
{
    if ( LESSMOD_LEFT8(what) < LESSMOD_RIGHT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void lessmod_branch16(void *what)
{
    if ( LESSMOD_LEFT16(what) < LESSMOD_RIGHT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void lessmod_branch32(void *what)
{
    if ( LESSMOD_LEFT32(what) < LESSMOD_RIGHT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}



/**********************************************************************

Branch module 5: equalsconst

**********************************************************************/

#define EQUALSCONSTMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define EQUALSCONSTMOD_RIGHT8(what)    DEREF_8VAR(what,0)
#define EQUALSCONSTMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define EQUALSCONSTMOD_RIGHT16(what)   DEREF_16VAR(what,0)
#define EQUALSCONSTMOD_LEFT32(what)    DEREF_32BUS(what,0)
#define EQUALSCONSTMOD_RIGHT32(what)   DEREF_32VAR(what,0)

void equalsconstmod_branch8(void *what);
void equalsconstmod_branch16(void *what);
void equalsconstmod_branch32(void *what);

module_data *equalsconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,1,1,1,3,6);

    EQUALSCONSTMOD_RIGHT8(what)  = 0;
    EQUALSCONSTMOD_RIGHT16(what) = 0;
    EQUALSCONSTMOD_RIGHT32(what) = 0;

    return what;
}

int equalsconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = equalsconstmod_branch8;
    DEREF_INFN(what,1) = equalsconstmod_branch16;
    DEREF_INFN(what,2) = equalsconstmod_branch32;

    return 0;
}

void equalsconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void equalsconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void equalsconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void equalsconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *equalsconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void equalsconstmod_branch8(void *what)
{
    if ( EQUALSCONSTMOD_LEFT8(what) == EQUALSCONSTMOD_RIGHT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void equalsconstmod_branch16(void *what)
{
    if ( EQUALSCONSTMOD_LEFT16(what) == EQUALSCONSTMOD_RIGHT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void equalsconstmod_branch32(void *what)
{
    if ( EQUALSCONSTMOD_LEFT32(what) == EQUALSCONSTMOD_RIGHT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}




/**********************************************************************

Branch module 6: greaterconst

**********************************************************************/

#define GREATERCONSTMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define GREATERCONSTMOD_RIGHT8(what)    DEREF_8VAR(what,0)
#define GREATERCONSTMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define GREATERCONSTMOD_RIGHT16(what)   DEREF_16VAR(what,0)
#define GREATERCONSTMOD_LEFT32(what)    DEREF_32BUS(what,0)
#define GREATERCONSTMOD_RIGHT32(what)   DEREF_32VAR(what,0)

void greaterconstmod_branch8(void *what);
void greaterconstmod_branch16(void *what);
void greaterconstmod_branch32(void *what);

module_data *greaterconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,1,1,1,3,6);

    GREATERCONSTMOD_RIGHT8(what)  = 0;
    GREATERCONSTMOD_RIGHT16(what) = 0;
    GREATERCONSTMOD_RIGHT32(what) = 0;

    return what;
}

int greaterconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = greaterconstmod_branch8;
    DEREF_INFN(what,1) = greaterconstmod_branch16;
    DEREF_INFN(what,2) = greaterconstmod_branch32;

    return 0;
}

void greaterconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void greaterconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void greaterconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void greaterconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *greaterconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void greaterconstmod_branch8(void *what)
{
    if ( GREATERCONSTMOD_LEFT8(what) > GREATERCONSTMOD_RIGHT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void greaterconstmod_branch16(void *what)
{
    if ( GREATERCONSTMOD_LEFT16(what) > GREATERCONSTMOD_RIGHT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void greaterconstmod_branch32(void *what)
{
    if ( GREATERCONSTMOD_LEFT32(what) > GREATERCONSTMOD_RIGHT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}



/**********************************************************************

Branch module 7: lessconst

**********************************************************************/

#define LESSCONSTMOD_LEFT8(what)     DEREF_8BUS(what,0)
#define LESSCONSTMOD_RIGHT8(what)    DEREF_8VAR(what,0)
#define LESSCONSTMOD_LEFT16(what)    DEREF_16BUS(what,0)
#define LESSCONSTMOD_RIGHT16(what)   DEREF_16VAR(what,0)
#define LESSCONSTMOD_LEFT32(what)    DEREF_32BUS(what,0)
#define LESSCONSTMOD_RIGHT32(what)   DEREF_32VAR(what,0)

void lessconstmod_branch8(void *what);
void lessconstmod_branch16(void *what);
void lessconstmod_branch32(void *what);

module_data *lessconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,1,1,1,3,6);

    LESSCONSTMOD_RIGHT8(what)  = 0;
    LESSCONSTMOD_RIGHT16(what) = 0;
    LESSCONSTMOD_RIGHT32(what) = 0;

    return what;
}

int lessconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = lessconstmod_branch8;
    DEREF_INFN(what,1) = lessconstmod_branch16;
    DEREF_INFN(what,2) = lessconstmod_branch32;

    return 0;
}

void lessconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void lessconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void lessconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void lessconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *lessconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void lessconstmod_branch8(void *what)
{
    if ( LESSCONSTMOD_LEFT8(what) < LESSCONSTMOD_RIGHT8(what) )
    {
        OUTFNCALL(what,0);
    }

    else
    {
        OUTFNCALL(what,1);
    }

    return;
}

void lessconstmod_branch16(void *what)
{
    if ( LESSCONSTMOD_LEFT16(what) < LESSCONSTMOD_RIGHT16(what) )
    {
        OUTFNCALL(what,2);
    }

    else
    {
        OUTFNCALL(what,3);
    }

    return;
}

void lessconstmod_branch32(void *what)
{
    if ( LESSCONSTMOD_LEFT32(what) < LESSCONSTMOD_RIGHT32(what) )
    {
        OUTFNCALL(what,4);
    }

    else
    {
        OUTFNCALL(what,5);
    }

    return;
}






/**********************************************************************

ALU module 1: assign

**********************************************************************/

#define ASSIGNMOD_DEST8(what)     DEREF_8BUS(what,0)
#define ASSIGNMOD_SRC8(what)      DEREF_8BUS(what,1)
#define ASSIGNMOD_DEST16(what)    DEREF_16BUS(what,0)
#define ASSIGNMOD_SRC16(what)     DEREF_16BUS(what,1)
#define ASSIGNMOD_DEST32(what)    DEREF_32BUS(what,0)
#define ASSIGNMOD_SRC32(what)     DEREF_32BUS(what,1)

void assignmod_nd_08xx_08xx(void *what);
void assignmod_nd_08xx_16l_(void *what);
void assignmod_nd_08xx_16h_(void *what);
void assignmod_nd_08xx_32ll(void *what);
void assignmod_nd_08xx_32lh(void *what);
void assignmod_nd_08xx_32hl(void *what);
void assignmod_nd_08xx_32hh(void *what);
void assignmod_nd_16l__08xx(void *what);
void assignmod_nd_16h__08xx(void *what);
void assignmod_nd_16xx_16xx(void *what);
void assignmod_nd_16xx_32l_(void *what);
void assignmod_nd_16xx_32h_(void *what);
void assignmod_nd_32ll_08xx(void *what);
void assignmod_nd_32lh_08xx(void *what);
void assignmod_nd_32hl_08xx(void *what);
void assignmod_nd_32hh_08xx(void *what);
void assignmod_nd_32l__16xx(void *what);
void assignmod_nd_32h__16xx(void *what);
void assignmod_nd_32xx_32xx(void *what);

void assignmod_d_08xx_08xx(void *what);
void assignmod_d_08xx_16l_(void *what);
void assignmod_d_08xx_16h_(void *what);
void assignmod_d_08xx_32ll(void *what);
void assignmod_d_08xx_32lh(void *what);
void assignmod_d_08xx_32hl(void *what);
void assignmod_d_08xx_32hh(void *what);
void assignmod_d_16l__08xx(void *what);
void assignmod_d_16h__08xx(void *what);
void assignmod_d_16xx_16xx(void *what);
void assignmod_d_16xx_32l_(void *what);
void assignmod_d_16xx_32h_(void *what);
void assignmod_d_32ll_08xx(void *what);
void assignmod_d_32lh_08xx(void *what);
void assignmod_d_32hl_08xx(void *what);
void assignmod_d_32hh_08xx(void *what);
void assignmod_d_32l__16xx(void *what);
void assignmod_d_32h__16xx(void *what);
void assignmod_d_32xx_32xx(void *what);

module_data *assignmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,2,2,2,38,0);

    return what;
}

int assignmod_init(module_data *what)
{
    DEREF_INFN(what,0)  = assignmod_nd_08xx_08xx;
    DEREF_INFN(what,1)  = assignmod_nd_08xx_16l_;
    DEREF_INFN(what,2)  = assignmod_nd_08xx_16h_;
    DEREF_INFN(what,3)  = assignmod_nd_08xx_32ll;
    DEREF_INFN(what,4)  = assignmod_nd_08xx_32lh;
    DEREF_INFN(what,5)  = assignmod_nd_08xx_32hl;
    DEREF_INFN(what,6)  = assignmod_nd_08xx_32hh;
    DEREF_INFN(what,7)  = assignmod_nd_16l__08xx;
    DEREF_INFN(what,8)  = assignmod_nd_16h__08xx;
    DEREF_INFN(what,9)  = assignmod_nd_16xx_16xx;
    DEREF_INFN(what,10) = assignmod_nd_16xx_32l_;
    DEREF_INFN(what,11) = assignmod_nd_16xx_32h_;
    DEREF_INFN(what,12) = assignmod_nd_32ll_08xx;
    DEREF_INFN(what,13) = assignmod_nd_32lh_08xx;
    DEREF_INFN(what,14) = assignmod_nd_32hl_08xx;
    DEREF_INFN(what,15) = assignmod_nd_32hh_08xx;
    DEREF_INFN(what,16) = assignmod_nd_32l__16xx;
    DEREF_INFN(what,17) = assignmod_nd_32h__16xx;
    DEREF_INFN(what,18) = assignmod_nd_32xx_32xx;
    DEREF_INFN(what,19) = assignmod_d_08xx_08xx;
    DEREF_INFN(what,20) = assignmod_d_08xx_16l_;
    DEREF_INFN(what,21) = assignmod_d_08xx_16h_;
    DEREF_INFN(what,22) = assignmod_d_08xx_32ll;
    DEREF_INFN(what,23) = assignmod_d_08xx_32lh;
    DEREF_INFN(what,24) = assignmod_d_08xx_32hl;
    DEREF_INFN(what,25) = assignmod_d_08xx_32hh;
    DEREF_INFN(what,26) = assignmod_d_16l__08xx;
    DEREF_INFN(what,27) = assignmod_d_16h__08xx;
    DEREF_INFN(what,28) = assignmod_d_16xx_16xx;
    DEREF_INFN(what,29) = assignmod_d_16xx_32l_;
    DEREF_INFN(what,30) = assignmod_d_16xx_32h_;
    DEREF_INFN(what,31) = assignmod_d_32ll_08xx;
    DEREF_INFN(what,32) = assignmod_d_32lh_08xx;
    DEREF_INFN(what,33) = assignmod_d_32hl_08xx;
    DEREF_INFN(what,34) = assignmod_d_32hh_08xx;
    DEREF_INFN(what,35) = assignmod_d_32l__16xx;
    DEREF_INFN(what,36) = assignmod_d_32h__16xx;
    DEREF_INFN(what,37) = assignmod_d_32xx_32xx;

    return 0;
}

void assignmod_go(module_data *what)
{
    return;

    what = NULL;
}

void assignmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void assignmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void assignmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *assignmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void assignmod_nd_08xx_08xx(void *what)
{
    ASSIGNMOD_DEST8(what) = ASSIGNMOD_SRC8(what);

    return;
}

void assignmod_nd_08xx_16l_(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ASSIGNMOD_SRC16(what) & 0x0ff );

    return;
}

void assignmod_nd_08xx_16h_(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC16(what) >> 8 ) & 0x0ff );

    return;
}

void assignmod_nd_08xx_32ll(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ASSIGNMOD_SRC32(what) & 0x0ff );

    return;
}

void assignmod_nd_08xx_32lh(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC32(what) >> 8 ) & 0x0ff );

    return;
}

void assignmod_nd_08xx_32hl(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC32(what) >> 16 ) & 0x0ff );

    return;
}

void assignmod_nd_08xx_32hh(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC32(what) >> 24 ) & 0x0ff );

    return;
}

void assignmod_nd_16l__08xx(void *what)
{
    ASSIGNMOD_DEST16(what) &= 0x0ff00;
    ASSIGNMOD_DEST16(what) |= ( (UINT_16) ASSIGNMOD_SRC8(what) ) & 0x000ff;

    return;
}

void assignmod_nd_16h__08xx(void *what)
{
    ASSIGNMOD_DEST16(what) &= 0x000ff;
    ASSIGNMOD_DEST16(what) |= ( ( (UINT_16) ASSIGNMOD_SRC8(what) ) << 8 ) & 0x0ff00;

    return;
}

void assignmod_nd_16xx_16xx(void *what)
{
    ASSIGNMOD_DEST16(what) = ASSIGNMOD_SRC16(what);

    return;
}

void assignmod_nd_16xx_32l_(void *what)
{
    ASSIGNMOD_DEST16(what) = (UINT_16) ( ASSIGNMOD_SRC32(what) & 0x0ffff );

    return;
}

void assignmod_nd_16xx_32h_(void *what)
{
    ASSIGNMOD_DEST16(what) = (UINT_16) ( ( ASSIGNMOD_SRC32(what) >> 16 ) & 0x0ffff );

    return;
}

void assignmod_nd_32ll_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) &= 0x0ffffff00;
    ASSIGNMOD_DEST32(what) |= ( (UINT_32) ASSIGNMOD_SRC8(what) ) & 0x0000000ff;

    return;
}

void assignmod_nd_32lh_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) &= 0x0ffff00ff;
    ASSIGNMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNMOD_SRC8(what) ) << 8 ) & 0x00000ff00;

    return;
}

void assignmod_nd_32hl_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) &= 0x0ff00ffff;
    ASSIGNMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNMOD_SRC8(what) ) << 16 ) & 0x000ff0000;

    return;
}

void assignmod_nd_32hh_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) &= 0x000ffffff;
    ASSIGNMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNMOD_SRC8(what) ) << 24 ) & 0x0ff000000;

    return;
}

void assignmod_nd_32l__16xx(void *what)
{
    ASSIGNMOD_DEST32(what) &= 0x0ffff0000;
    ASSIGNMOD_DEST32(what) |= ( (UINT_32) ASSIGNMOD_SRC16(what) ) & 0x00000ffff;

    return;
}

void assignmod_nd_32h__16xx(void *what)
{
    ASSIGNMOD_DEST32(what) &= 0x00000ffff;
    ASSIGNMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNMOD_SRC16(what) ) << 16 ) & 0x0ffff0000;

    return;
}

void assignmod_nd_32xx_32xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ASSIGNMOD_SRC32(what);

    return;
}

void assignmod_d_08xx_08xx(void *what)
{
    ASSIGNMOD_DEST8(what) = ASSIGNMOD_SRC8(what);

    return;
}

void assignmod_d_08xx_16l_(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ASSIGNMOD_SRC16(what) & 0x0ff );

    return;
}

void assignmod_d_08xx_16h_(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC16(what) >> 8 ) & 0x0ff );

    return;
}

void assignmod_d_08xx_32ll(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ASSIGNMOD_SRC32(what) & 0x0ff );

    return;
}

void assignmod_d_08xx_32lh(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC32(what) >> 8 ) & 0x0ff );

    return;
}

void assignmod_d_08xx_32hl(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC32(what) >> 16 ) & 0x0ff );

    return;
}

void assignmod_d_08xx_32hh(void *what)
{
    ASSIGNMOD_DEST8(what) = (UINT_8) ( ( ASSIGNMOD_SRC32(what) >> 24 ) & 0x0ff );

    return;
}

void assignmod_d_16l__08xx(void *what)
{
    ASSIGNMOD_DEST16(what) = ( (UINT_16) ASSIGNMOD_SRC8(what) ) & 0x000ff;

    return;
}

void assignmod_d_16h__08xx(void *what)
{
    ASSIGNMOD_DEST16(what) = ( ( (UINT_16) ASSIGNMOD_SRC8(what) ) << 8 ) & 0x0ff00;

    return;
}

void assignmod_d_16xx_16xx(void *what)
{
    ASSIGNMOD_DEST16(what) = ASSIGNMOD_SRC16(what);

    return;
}

void assignmod_d_16xx_32l_(void *what)
{
    ASSIGNMOD_DEST16(what) = (UINT_16) ( ASSIGNMOD_SRC32(what) & 0x0ffff );

    return;
}

void assignmod_d_16xx_32h_(void *what)
{
    ASSIGNMOD_DEST16(what) = (UINT_16) ( ( ASSIGNMOD_SRC32(what) >> 16 ) & 0x0ffff );

    return;
}

void assignmod_d_32ll_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ( (UINT_32) ASSIGNMOD_SRC8(what) ) & 0x0000000ff;

    return;
}

void assignmod_d_32lh_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ( ( (UINT_32) ASSIGNMOD_SRC8(what) ) << 8 ) & 0x00000ff00;

    return;
}

void assignmod_d_32hl_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ( ( (UINT_32) ASSIGNMOD_SRC8(what) ) << 16 ) & 0x000ff0000;

    return;
}

void assignmod_d_32hh_08xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ( ( (UINT_32) ASSIGNMOD_SRC8(what) ) << 24 ) & 0x0ff000000;

    return;
}

void assignmod_d_32l__16xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ( (UINT_32) ASSIGNMOD_SRC16(what) ) & 0x00000ffff;

    return;
}

void assignmod_d_32h__16xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ( ( (UINT_32) ASSIGNMOD_SRC16(what) ) << 16 ) & 0x0ffff0000;

    return;
}

void assignmod_d_32xx_32xx(void *what)
{
    ASSIGNMOD_DEST32(what) = ASSIGNMOD_SRC32(what);

    return;
}




/**********************************************************************

ALU module 2: not

**********************************************************************/

#define NOTMOD_RES8(what)      DEREF_8BUS(what,0)
#define NOTMOD_ARG8(what)      DEREF_8BUS(what,1)
#define NOTMOD_RES16(what)     DEREF_16BUS(what,0)
#define NOTMOD_ARG16(what)     DEREF_16BUS(what,1)
#define NOTMOD_RES32(what)     DEREF_32BUS(what,0)
#define NOTMOD_ARG32(what)     DEREF_32BUS(what,1)

void notmod_8(void *what);
void notmod_16(void *what);
void notmod_32(void *what);

module_data *notmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,2,2,2,3,0);

    return what;
}

int notmod_init(module_data *what)
{
    DEREF_INFN(what,0) = notmod_8;
    DEREF_INFN(what,1) = notmod_16;
    DEREF_INFN(what,2) = notmod_32;

    return 0;
}

void notmod_go(module_data *what)
{
    return;

    what = NULL;
}

void notmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void notmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void notmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *notmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void notmod_8(void *what)
{
    NOTMOD_RES8(what) = NOTMOD_ARG8(what) ^ 0x0ff;

    return;
}

void notmod_16(void *what)
{
    NOTMOD_RES16(what) = NOTMOD_ARG16(what) ^ 0x0ffff;

    return;
}

void notmod_32(void *what)
{
    NOTMOD_RES32(what) = NOTMOD_ARG32(what) ^ 0x0ffffffff;

    return;
}







/**********************************************************************

ALU module 3: twos

**********************************************************************/

#define TWOSMOD_RES8(what)      DEREF_8BUS(what,0)
#define TWOSMOD_ARG8(what)      DEREF_8BUS(what,1)
#define TWOSMOD_RES16(what)     DEREF_16BUS(what,0)
#define TWOSMOD_ARG16(what)     DEREF_16BUS(what,1)
#define TWOSMOD_RES32(what)     DEREF_32BUS(what,0)
#define TWOSMOD_ARG32(what)     DEREF_32BUS(what,1)

void twosmod_8(void *what);
void twosmod_16(void *what);
void twosmod_32(void *what);

module_data *twosmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,2,2,2,3,0);

    return what;
}

int twosmod_init(module_data *what)
{
    DEREF_INFN(what,0) = twosmod_8;
    DEREF_INFN(what,1) = twosmod_16;
    DEREF_INFN(what,2) = twosmod_32;

    return 0;
}

void twosmod_go(module_data *what)
{
    return;

    what = NULL;
}

void twosmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void twosmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void twosmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *twosmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void twosmod_8(void *what)
{
    TWOSMOD_RES8(what) = ( ( ( ( TWOSMOD_ARG8(what) ) ^ 0x0ff ) + 1 ) & 0x0ff );

    return;
}

void twosmod_16(void *what)
{
    TWOSMOD_RES16(what) = ( ( ( ( TWOSMOD_ARG16(what) ) ^ 0x0ffff ) + 1 ) & 0x0ffff );

    return;
}

void twosmod_32(void *what)
{
    TWOSMOD_RES32(what) = ( ( ( ( TWOSMOD_ARG32(what) ) ^ 0x0ffffffff ) + 1 ) & 0x0ffffffff );

    return;
}







/**********************************************************************

ALU module 4: add

**********************************************************************/

#define ADDMOD_RES8(what)      DEREF_8BUS(what,0)
#define ADDMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define ADDMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define ADDMOD_RES16(what)     DEREF_16BUS(what,0)
#define ADDMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define ADDMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define ADDMOD_RES32(what)     DEREF_32BUS(what,0)
#define ADDMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define ADDMOD_ARGR32(what)    DEREF_32BUS(what,2)

void addmod_8(void *what);
void addmod_16(void *what);
void addmod_32(void *what);

module_data *addmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int addmod_init(module_data *what)
{
    DEREF_INFN(what,0) = addmod_8;
    DEREF_INFN(what,1) = addmod_16;
    DEREF_INFN(what,2) = addmod_32;

    return 0;
}

void addmod_go(module_data *what)
{
    return;

    what = NULL;
}

void addmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void addmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void addmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *addmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void addmod_8(void *what)
{
    ADDMOD_RES8(what) = ADDMOD_ARGL8(what) + ADDMOD_ARGR8(what);

    return;
}

void addmod_16(void *what)
{
    ADDMOD_RES16(what) = ADDMOD_ARGL16(what) + ADDMOD_ARGR16(what);

    return;
}

void addmod_32(void *what)
{
    ADDMOD_RES32(what) = ADDMOD_ARGL32(what) + ADDMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 5: sub

**********************************************************************/

#define SUBMOD_RES8(what)      DEREF_8BUS(what,0)
#define SUBMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define SUBMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define SUBMOD_RES16(what)     DEREF_16BUS(what,0)
#define SUBMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define SUBMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define SUBMOD_RES32(what)     DEREF_32BUS(what,0)
#define SUBMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define SUBMOD_ARGR32(what)    DEREF_32BUS(what,2)

void submod_8(void *what);
void submod_16(void *what);
void submod_32(void *what);

module_data *submod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int submod_init(module_data *what)
{
    DEREF_INFN(what,0) = submod_8;
    DEREF_INFN(what,1) = submod_16;
    DEREF_INFN(what,2) = submod_32;

    return 0;
}

void submod_go(module_data *what)
{
    return;

    what = NULL;
}

void submod_stop(module_data *what)
{
    return;

    what = NULL;
}

void submod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void submod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *submod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void submod_8(void *what)
{
    SUBMOD_RES8(what) = SUBMOD_ARGL8(what) - SUBMOD_ARGR8(what);

    return;
}

void submod_16(void *what)
{
    SUBMOD_RES16(what) = SUBMOD_ARGL16(what) - SUBMOD_ARGR16(what);

    return;
}

void submod_32(void *what)
{
    SUBMOD_RES32(what) = SUBMOD_ARGL32(what) - SUBMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 6: mul

**********************************************************************/

#define MULMOD_RES8(what)      DEREF_8BUS(what,0)
#define MULMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define MULMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define MULMOD_RES16(what)     DEREF_16BUS(what,0)
#define MULMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define MULMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define MULMOD_RES32(what)     DEREF_32BUS(what,0)
#define MULMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define MULMOD_ARGR32(what)    DEREF_32BUS(what,2)

void mulmod_8(void *what);
void mulmod_16(void *what);
void mulmod_32(void *what);

module_data *mulmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int mulmod_init(module_data *what)
{
    DEREF_INFN(what,0) = mulmod_8;
    DEREF_INFN(what,1) = mulmod_16;
    DEREF_INFN(what,2) = mulmod_32;

    return 0;
}

void mulmod_go(module_data *what)
{
    return;

    what = NULL;
}

void mulmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void mulmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void mulmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *mulmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void mulmod_8(void *what)
{
    MULMOD_RES8(what) = MULMOD_ARGL8(what) * MULMOD_ARGR8(what);

    return;
}

void mulmod_16(void *what)
{
    MULMOD_RES16(what) = MULMOD_ARGL16(what) * MULMOD_ARGR16(what);

    return;
}

void mulmod_32(void *what)
{
    MULMOD_RES32(what) = MULMOD_ARGL32(what) * MULMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 7: div

**********************************************************************/

#define DIVMOD_RES8(what)      DEREF_8BUS(what,0)
#define DIVMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define DIVMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define DIVMOD_RES16(what)     DEREF_16BUS(what,0)
#define DIVMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define DIVMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define DIVMOD_RES32(what)     DEREF_32BUS(what,0)
#define DIVMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define DIVMOD_ARGR32(what)    DEREF_32BUS(what,2)

void divmod_8(void *what);
void divmod_16(void *what);
void divmod_32(void *what);

module_data *divmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int divmod_init(module_data *what)
{
    DEREF_INFN(what,0) = divmod_8;
    DEREF_INFN(what,1) = divmod_16;
    DEREF_INFN(what,2) = divmod_32;

    return 0;
}

void divmod_go(module_data *what)
{
    return;

    what = NULL;
}

void divmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void divmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void divmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *divmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void divmod_8(void *what)
{
    DIVMOD_RES8(what) = DIVMOD_ARGL8(what) / DIVMOD_ARGR8(what);

    return;
}

void divmod_16(void *what)
{
    DIVMOD_RES16(what) = DIVMOD_ARGL16(what) / DIVMOD_ARGR16(what);

    return;
}

void divmod_32(void *what)
{
    DIVMOD_RES32(what) = DIVMOD_ARGL32(what) / DIVMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 8: mod

**********************************************************************/

#define MODMOD_RES8(what)      DEREF_8BUS(what,0)
#define MODMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define MODMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define MODMOD_RES16(what)     DEREF_16BUS(what,0)
#define MODMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define MODMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define MODMOD_RES32(what)     DEREF_32BUS(what,0)
#define MODMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define MODMOD_ARGR32(what)    DEREF_32BUS(what,2)

void modmod_8(void *what);
void modmod_16(void *what);
void modmod_32(void *what);

module_data *modmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int modmod_init(module_data *what)
{
    DEREF_INFN(what,0) = modmod_8;
    DEREF_INFN(what,1) = modmod_16;
    DEREF_INFN(what,2) = modmod_32;

    return 0;
}

void modmod_go(module_data *what)
{
    return;

    what = NULL;
}

void modmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void modmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void modmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *modmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void modmod_8(void *what)
{
    MODMOD_RES8(what) = MODMOD_ARGL8(what) % MODMOD_ARGR8(what);

    return;
}

void modmod_16(void *what)
{
    MODMOD_RES16(what) = MODMOD_ARGL16(what) % MODMOD_ARGR16(what);

    return;
}

void modmod_32(void *what)
{
    MODMOD_RES32(what) = MODMOD_ARGL32(what) % MODMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 9: and

**********************************************************************/

#define ANDMOD_RES8(what)      DEREF_8BUS(what,0)
#define ANDMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define ANDMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define ANDMOD_RES16(what)     DEREF_16BUS(what,0)
#define ANDMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define ANDMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define ANDMOD_RES32(what)     DEREF_32BUS(what,0)
#define ANDMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define ANDMOD_ARGR32(what)    DEREF_32BUS(what,2)

void andmod_8(void *what);
void andmod_16(void *what);
void andmod_32(void *what);

module_data *andmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int andmod_init(module_data *what)
{
    DEREF_INFN(what,0) = andmod_8;
    DEREF_INFN(what,1) = andmod_16;
    DEREF_INFN(what,2) = andmod_32;

    return 0;
}

void andmod_go(module_data *what)
{
    return;

    what = NULL;
}

void andmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void andmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void andmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *andmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void andmod_8(void *what)
{
    ANDMOD_RES8(what) = ANDMOD_ARGL8(what) & ANDMOD_ARGR8(what);

    return;
}

void andmod_16(void *what)
{
    ANDMOD_RES16(what) = ANDMOD_ARGL16(what) & ANDMOD_ARGR16(what);

    return;
}

void andmod_32(void *what)
{
    ANDMOD_RES32(what) = ANDMOD_ARGL32(what) & ANDMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 10: or

**********************************************************************/

#define ORMOD_RES8(what)      DEREF_8BUS(what,0)
#define ORMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define ORMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define ORMOD_RES16(what)     DEREF_16BUS(what,0)
#define ORMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define ORMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define ORMOD_RES32(what)     DEREF_32BUS(what,0)
#define ORMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define ORMOD_ARGR32(what)    DEREF_32BUS(what,2)

void ormod_8(void *what);
void ormod_16(void *what);
void ormod_32(void *what);
void ormod_mix(void *what);

module_data *ormod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,4,0);

    return what;
}

int ormod_init(module_data *what)
{
    DEREF_INFN(what,0) = ormod_8;
    DEREF_INFN(what,1) = ormod_16;
    DEREF_INFN(what,2) = ormod_32;
    DEREF_INFN(what,3) = ormod_mix;

    return 0;
}

void ormod_go(module_data *what)
{
    return;

    what = NULL;
}

void ormod_stop(module_data *what)
{
    return;

    what = NULL;
}

void ormod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void ormod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *ormod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void ormod_8(void *what)
{
    ORMOD_RES8(what) = ORMOD_ARGL8(what) | ORMOD_ARGR8(what);

    return;
}

void ormod_16(void *what)
{
    ORMOD_RES16(what) = ORMOD_ARGL16(what) | ORMOD_ARGR16(what);

    return;
}

void ormod_32(void *what)
{
    ORMOD_RES32(what) = ORMOD_ARGL32(what) | ORMOD_ARGR32(what);

    return;
}

void ormod_mix(void *what)
{
    ORMOD_RES16(what) = ORMOD_ARGL16(what) | ORMOD_ARGR8(what);

    return;
}







/**********************************************************************

ALU module 11: xor

**********************************************************************/

#define XORMOD_RES8(what)      DEREF_8BUS(what,0)
#define XORMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define XORMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define XORMOD_RES16(what)     DEREF_16BUS(what,0)
#define XORMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define XORMOD_ARGR16(what)    DEREF_16BUS(what,2)
#define XORMOD_RES32(what)     DEREF_32BUS(what,0)
#define XORMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define XORMOD_ARGR32(what)    DEREF_32BUS(what,2)

void xormod_8(void *what);
void xormod_16(void *what);
void xormod_32(void *what);

module_data *xormod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,3,3,3,3,0);

    return what;
}

int xormod_init(module_data *what)
{
    DEREF_INFN(what,0) = xormod_8;
    DEREF_INFN(what,1) = xormod_16;
    DEREF_INFN(what,2) = xormod_32;

    return 0;
}

void xormod_go(module_data *what)
{
    return;

    what = NULL;
}

void xormod_stop(module_data *what)
{
    return;

    what = NULL;
}

void xormod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void xormod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *xormod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void xormod_8(void *what)
{
    XORMOD_RES8(what) = XORMOD_ARGL8(what) ^ XORMOD_ARGR8(what);

    return;
}

void xormod_16(void *what)
{
    XORMOD_RES16(what) = XORMOD_ARGL16(what) ^ XORMOD_ARGR16(what);

    return;
}

void xormod_32(void *what)
{
    XORMOD_RES32(what) = XORMOD_ARGL32(what) ^ XORMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 12: rr

**********************************************************************/

#define RRMOD_RES8(what)      DEREF_8BUS(what,0)
#define RRMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define RRMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define RRMOD_RES16(what)     DEREF_16BUS(what,0)
#define RRMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define RRMOD_ARGR16(what)    DEREF_8BUS(what,3)
#define RRMOD_RES32(what)     DEREF_32BUS(what,0)
#define RRMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define RRMOD_ARGR32(what)    DEREF_8BUS(what,4)

void rrmod_8(void *what);
void rrmod_16(void *what);
void rrmod_32(void *what);

module_data *rrmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,5,2,2,3,0);

    return what;
}

int rrmod_init(module_data *what)
{
    DEREF_INFN(what,0) = rrmod_8;
    DEREF_INFN(what,1) = rrmod_16;
    DEREF_INFN(what,2) = rrmod_32;

    return 0;
}

void rrmod_go(module_data *what)
{
    return;

    what = NULL;
}

void rrmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void rrmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void rrmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *rrmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void rrmod_8(void *what)
{
    switch ( RRMOD_ARGR8(what)%8 )
    {
        case 0:  { RRMOD_RES8(what) =     RRMOD_ARGL8(what);                                                           break; }
        case 1:  { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 1 ) & 0x07f ) | ( ( RRMOD_ARGL8(what) << 7 ) & 0x080 ); break; }
        case 2:  { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 2 ) & 0x03f ) | ( ( RRMOD_ARGL8(what) << 6 ) & 0x0c0 ); break; }
        case 3:  { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 3 ) & 0x01f ) | ( ( RRMOD_ARGL8(what) << 5 ) & 0x0e0 ); break; }
        case 4:  { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 4 ) & 0x00f ) | ( ( RRMOD_ARGL8(what) << 4 ) & 0x0f0 ); break; }
        case 5:  { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 5 ) & 0x007 ) | ( ( RRMOD_ARGL8(what) << 3 ) & 0x0f8 ); break; }
        case 6:  { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 6 ) & 0x003 ) | ( ( RRMOD_ARGL8(what) << 2 ) & 0x0fc ); break; }
        default: { RRMOD_RES8(what) = ( ( RRMOD_ARGL8(what) >> 7 ) & 0x001 ) | ( ( RRMOD_ARGL8(what) << 1 ) & 0x0fe ); break; }
    }

    return;
}

void rrmod_16(void *what)
{
    switch ( RRMOD_ARGR16(what)%16 )
    {
        case  0: { RRMOD_RES16(what) =     RRMOD_ARGL16(what);                                                                  break; }
        case  1: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  1 ) & 0x07fff ) | ( ( RRMOD_ARGL16(what) << 15 ) & 0x08000 ); break; }
        case  2: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  2 ) & 0x03fff ) | ( ( RRMOD_ARGL16(what) << 14 ) & 0x0c000 ); break; }
        case  3: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  3 ) & 0x01fff ) | ( ( RRMOD_ARGL16(what) << 13 ) & 0x0e000 ); break; }
        case  4: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  4 ) & 0x00fff ) | ( ( RRMOD_ARGL16(what) << 12 ) & 0x0f000 ); break; }
        case  5: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  5 ) & 0x007ff ) | ( ( RRMOD_ARGL16(what) << 11 ) & 0x0f800 ); break; }
        case  6: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  6 ) & 0x003ff ) | ( ( RRMOD_ARGL16(what) << 10 ) & 0x0fc00 ); break; }
        case  7: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  7 ) & 0x001ff ) | ( ( RRMOD_ARGL16(what) <<  9 ) & 0x0fe00 ); break; }
        case  8: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  8 ) & 0x000ff ) | ( ( RRMOD_ARGL16(what) <<  8 ) & 0x0ff00 ); break; }
        case  9: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >>  9 ) & 0x0007f ) | ( ( RRMOD_ARGL16(what) <<  7 ) & 0x0ff80 ); break; }
        case 10: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >> 10 ) & 0x0003f ) | ( ( RRMOD_ARGL16(what) <<  6 ) & 0x0ffc0 ); break; }
        case 11: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >> 11 ) & 0x0001f ) | ( ( RRMOD_ARGL16(what) <<  5 ) & 0x0ffe0 ); break; }
        case 12: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >> 12 ) & 0x0000f ) | ( ( RRMOD_ARGL16(what) <<  4 ) & 0x0fff0 ); break; }
        case 13: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >> 13 ) & 0x00007 ) | ( ( RRMOD_ARGL16(what) <<  3 ) & 0x0fff8 ); break; }
        case 14: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >> 14 ) & 0x00003 ) | ( ( RRMOD_ARGL16(what) <<  2 ) & 0x0fffc ); break; }
        default: { RRMOD_RES16(what) = ( ( RRMOD_ARGL16(what) >> 15 ) & 0x00001 ) | ( ( RRMOD_ARGL16(what) <<  1 ) & 0x0fffe ); break; }
    }

    return;
}

void rrmod_32(void *what)
{
    switch ( RRMOD_ARGR32(what)%32 )
    {
        case  0: { RRMOD_RES32(what) =     RRMOD_ARGL32(what);                                                                          break; }
        case  1: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  1 ) & 0x07fffffff ) | ( ( RRMOD_ARGL32(what) << 31 ) & 0x080000000 ); break; }
        case  2: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  2 ) & 0x03fffffff ) | ( ( RRMOD_ARGL32(what) << 30 ) & 0x0c0000000 ); break; }
        case  3: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  3 ) & 0x01fffffff ) | ( ( RRMOD_ARGL32(what) << 29 ) & 0x0e0000000 ); break; }
        case  4: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  4 ) & 0x00fffffff ) | ( ( RRMOD_ARGL32(what) << 28 ) & 0x0f0000000 ); break; }
        case  5: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  5 ) & 0x007ffffff ) | ( ( RRMOD_ARGL32(what) << 27 ) & 0x0f8000000 ); break; }
        case  6: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  6 ) & 0x003ffffff ) | ( ( RRMOD_ARGL32(what) << 26 ) & 0x0fc000000 ); break; }
        case  7: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  7 ) & 0x001ffffff ) | ( ( RRMOD_ARGL32(what) << 25 ) & 0x0fe000000 ); break; }
        case  8: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  8 ) & 0x000ffffff ) | ( ( RRMOD_ARGL32(what) << 24 ) & 0x0ff000000 ); break; }
        case  9: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >>  9 ) & 0x0007fffff ) | ( ( RRMOD_ARGL32(what) << 23 ) & 0x0ff800000 ); break; }
        case 10: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 10 ) & 0x0003fffff ) | ( ( RRMOD_ARGL32(what) << 22 ) & 0x0ffc00000 ); break; }
        case 11: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 11 ) & 0x0001fffff ) | ( ( RRMOD_ARGL32(what) << 21 ) & 0x0ffe00000 ); break; }
        case 12: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 12 ) & 0x0000fffff ) | ( ( RRMOD_ARGL32(what) << 20 ) & 0x0fff00000 ); break; }
        case 13: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 13 ) & 0x00007ffff ) | ( ( RRMOD_ARGL32(what) << 19 ) & 0x0fff80000 ); break; }
        case 14: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 14 ) & 0x00003ffff ) | ( ( RRMOD_ARGL32(what) << 18 ) & 0x0fffc0000 ); break; }
        case 15: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 15 ) & 0x00001ffff ) | ( ( RRMOD_ARGL32(what) << 17 ) & 0x0fffe0000 ); break; }
        case 16: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 16 ) & 0x00000ffff ) | ( ( RRMOD_ARGL32(what) << 16 ) & 0x0ffff0000 ); break; }
        case 17: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 17 ) & 0x000007fff ) | ( ( RRMOD_ARGL32(what) << 15 ) & 0x0ffff8000 ); break; }
        case 18: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 18 ) & 0x000003fff ) | ( ( RRMOD_ARGL32(what) << 14 ) & 0x0ffffc000 ); break; }
        case 19: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 19 ) & 0x000001fff ) | ( ( RRMOD_ARGL32(what) << 13 ) & 0x0ffffe000 ); break; }
        case 20: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 20 ) & 0x000000fff ) | ( ( RRMOD_ARGL32(what) << 12 ) & 0x0fffff000 ); break; }
        case 21: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 21 ) & 0x0000007ff ) | ( ( RRMOD_ARGL32(what) << 11 ) & 0x0fffff800 ); break; }
        case 22: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 22 ) & 0x0000003ff ) | ( ( RRMOD_ARGL32(what) << 10 ) & 0x0fffffc00 ); break; }
        case 23: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 23 ) & 0x0000001ff ) | ( ( RRMOD_ARGL32(what) <<  9 ) & 0x0fffffe00 ); break; }
        case 24: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 24 ) & 0x0000000ff ) | ( ( RRMOD_ARGL32(what) <<  8 ) & 0x0ffffff00 ); break; }
        case 25: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 25 ) & 0x00000007f ) | ( ( RRMOD_ARGL32(what) <<  7 ) & 0x0ffffff80 ); break; }
        case 26: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 26 ) & 0x00000003f ) | ( ( RRMOD_ARGL32(what) <<  6 ) & 0x0ffffffc0 ); break; }
        case 27: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 27 ) & 0x00000001f ) | ( ( RRMOD_ARGL32(what) <<  5 ) & 0x0ffffffe0 ); break; }
        case 28: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 28 ) & 0x00000000f ) | ( ( RRMOD_ARGL32(what) <<  4 ) & 0x0fffffff0 ); break; }
        case 29: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 29 ) & 0x000000007 ) | ( ( RRMOD_ARGL32(what) <<  3 ) & 0x0fffffff8 ); break; }
        case 30: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 30 ) & 0x000000003 ) | ( ( RRMOD_ARGL32(what) <<  2 ) & 0x0fffffffc ); break; }
        default: { RRMOD_RES32(what) = ( ( RRMOD_ARGL32(what) >> 31 ) & 0x000000001 ) | ( ( RRMOD_ARGL32(what) <<  1 ) & 0x0fffffffe ); break; }
    }

    return;
}







/**********************************************************************

ALU module 13: rl

**********************************************************************/

#define RLMOD_RES8(what)      DEREF_8BUS(what,0)
#define RLMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define RLMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define RLMOD_RES16(what)     DEREF_16BUS(what,0)
#define RLMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define RLMOD_ARGR16(what)    DEREF_8BUS(what,3)
#define RLMOD_RES32(what)     DEREF_32BUS(what,0)
#define RLMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define RLMOD_ARGR32(what)    DEREF_8BUS(what,4)

void rlmod_8(void *what);
void rlmod_16(void *what);
void rlmod_32(void *what);

module_data *rlmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,5,2,2,3,0);

    return what;
}

int rlmod_init(module_data *what)
{
    DEREF_INFN(what,0) = rlmod_8;
    DEREF_INFN(what,1) = rlmod_16;
    DEREF_INFN(what,2) = rlmod_32;

    return 0;
}

void rlmod_go(module_data *what)
{
    return;

    what = NULL;
}

void rlmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void rlmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void rlmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *rlmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void rlmod_8(void *what)
{
    switch ( RLMOD_ARGR8(what)%8 )
    {
        case 0:  { RLMOD_RES8(what) =     RLMOD_ARGL8(what);                                                           break; }
        case 1:  { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 7 ) & 0x001 ) | ( ( RLMOD_ARGL8(what) << 1 ) & 0x0fe ); break; }
        case 2:  { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 6 ) & 0x003 ) | ( ( RLMOD_ARGL8(what) << 2 ) & 0x0fc ); break; }
        case 3:  { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 5 ) & 0x007 ) | ( ( RLMOD_ARGL8(what) << 3 ) & 0x0f8 ); break; }
        case 4:  { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 4 ) & 0x00f ) | ( ( RLMOD_ARGL8(what) << 4 ) & 0x0f0 ); break; }
        case 5:  { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 3 ) & 0x01f ) | ( ( RLMOD_ARGL8(what) << 5 ) & 0x0e0 ); break; }
        case 6:  { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 2 ) & 0x03f ) | ( ( RLMOD_ARGL8(what) << 6 ) & 0x0c0 ); break; }
        default: { RLMOD_RES8(what) = ( ( RLMOD_ARGL8(what) >> 1 ) & 0x07f ) | ( ( RLMOD_ARGL8(what) << 7 ) & 0x080 ); break; }
    }

    return;
}

void rlmod_16(void *what)
{
    switch ( RLMOD_ARGR16(what)%16 )
    {
        case  0: { RLMOD_RES16(what) =     RLMOD_ARGL16(what);                                                                  break; }
        case  1: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >> 15 ) & 0x00001 ) | ( ( RLMOD_ARGL16(what) <<  1 ) & 0x0fffe ); break; }
        case  2: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >> 14 ) & 0x00003 ) | ( ( RLMOD_ARGL16(what) <<  2 ) & 0x0fffc ); break; }
        case  3: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >> 13 ) & 0x00007 ) | ( ( RLMOD_ARGL16(what) <<  3 ) & 0x0fff8 ); break; }
        case  4: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >> 12 ) & 0x0000f ) | ( ( RLMOD_ARGL16(what) <<  4 ) & 0x0fff0 ); break; }
        case  5: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >> 11 ) & 0x0001f ) | ( ( RLMOD_ARGL16(what) <<  5 ) & 0x0ffe0 ); break; }
        case  6: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >> 10 ) & 0x0003f ) | ( ( RLMOD_ARGL16(what) <<  6 ) & 0x0ffc0 ); break; }
        case  7: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  9 ) & 0x0007f ) | ( ( RLMOD_ARGL16(what) <<  7 ) & 0x0ff80 ); break; }
        case  8: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  8 ) & 0x000ff ) | ( ( RLMOD_ARGL16(what) <<  8 ) & 0x0ff00 ); break; }
        case  9: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  7 ) & 0x001ff ) | ( ( RLMOD_ARGL16(what) <<  9 ) & 0x0fe00 ); break; }
        case 10: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  6 ) & 0x003ff ) | ( ( RLMOD_ARGL16(what) << 10 ) & 0x0fc00 ); break; }
        case 11: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  5 ) & 0x007ff ) | ( ( RLMOD_ARGL16(what) << 11 ) & 0x0f800 ); break; }
        case 12: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  4 ) & 0x00fff ) | ( ( RLMOD_ARGL16(what) << 12 ) & 0x0f000 ); break; }
        case 13: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  3 ) & 0x01fff ) | ( ( RLMOD_ARGL16(what) << 13 ) & 0x0e000 ); break; }
        case 14: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  2 ) & 0x03fff ) | ( ( RLMOD_ARGL16(what) << 14 ) & 0x0c000 ); break; }
        default: { RLMOD_RES16(what) = ( ( RLMOD_ARGL16(what) >>  1 ) & 0x07fff ) | ( ( RLMOD_ARGL16(what) << 15 ) & 0x08000 ); break; }
    }

    return;
}

void rlmod_32(void *what)
{
    switch ( RLMOD_ARGR32(what)%32 )
    {
        case  0: { RLMOD_RES32(what) =     RLMOD_ARGL32(what);                                                                          break; }
        case  1: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 31 ) & 0x000000001 ) | ( ( RLMOD_ARGL32(what) <<  1 ) & 0x0fffffffe ); break; }
        case  2: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 30 ) & 0x000000003 ) | ( ( RLMOD_ARGL32(what) <<  2 ) & 0x0fffffffc ); break; }
        case  3: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 29 ) & 0x000000007 ) | ( ( RLMOD_ARGL32(what) <<  3 ) & 0x0fffffff8 ); break; }
        case  4: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 28 ) & 0x00000000f ) | ( ( RLMOD_ARGL32(what) <<  4 ) & 0x0fffffff0 ); break; }
        case  5: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 27 ) & 0x00000001f ) | ( ( RLMOD_ARGL32(what) <<  5 ) & 0x0ffffffe0 ); break; }
        case  6: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 26 ) & 0x00000003f ) | ( ( RLMOD_ARGL32(what) <<  6 ) & 0x0ffffffc0 ); break; }
        case  7: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 25 ) & 0x00000007f ) | ( ( RLMOD_ARGL32(what) <<  7 ) & 0x0ffffff80 ); break; }
        case  8: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 24 ) & 0x0000000ff ) | ( ( RLMOD_ARGL32(what) <<  8 ) & 0x0ffffff00 ); break; }
        case  9: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 23 ) & 0x0000001ff ) | ( ( RLMOD_ARGL32(what) <<  9 ) & 0x0fffffe00 ); break; }
        case 10: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 22 ) & 0x0000003ff ) | ( ( RLMOD_ARGL32(what) << 10 ) & 0x0fffffc00 ); break; }
        case 11: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 21 ) & 0x0000007ff ) | ( ( RLMOD_ARGL32(what) << 11 ) & 0x0fffff800 ); break; }
        case 12: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 20 ) & 0x000000fff ) | ( ( RLMOD_ARGL32(what) << 12 ) & 0x0fffff000 ); break; }
        case 13: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 19 ) & 0x000001fff ) | ( ( RLMOD_ARGL32(what) << 13 ) & 0x0ffffe000 ); break; }
        case 14: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 18 ) & 0x000003fff ) | ( ( RLMOD_ARGL32(what) << 14 ) & 0x0ffffc000 ); break; }
        case 15: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 17 ) & 0x000007fff ) | ( ( RLMOD_ARGL32(what) << 15 ) & 0x0ffff8000 ); break; }
        case 16: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 16 ) & 0x00000ffff ) | ( ( RLMOD_ARGL32(what) << 16 ) & 0x0ffff0000 ); break; }
        case 17: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 15 ) & 0x00001ffff ) | ( ( RLMOD_ARGL32(what) << 17 ) & 0x0fffe0000 ); break; }
        case 18: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 14 ) & 0x00003ffff ) | ( ( RLMOD_ARGL32(what) << 18 ) & 0x0fffc0000 ); break; }
        case 19: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 13 ) & 0x00007ffff ) | ( ( RLMOD_ARGL32(what) << 19 ) & 0x0fff80000 ); break; }
        case 20: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 12 ) & 0x0000fffff ) | ( ( RLMOD_ARGL32(what) << 20 ) & 0x0fff00000 ); break; }
        case 21: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 11 ) & 0x0001fffff ) | ( ( RLMOD_ARGL32(what) << 21 ) & 0x0ffe00000 ); break; }
        case 22: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >> 10 ) & 0x0003fffff ) | ( ( RLMOD_ARGL32(what) << 22 ) & 0x0ffc00000 ); break; }
        case 23: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  9 ) & 0x0007fffff ) | ( ( RLMOD_ARGL32(what) << 23 ) & 0x0ff800000 ); break; }
        case 24: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  8 ) & 0x000ffffff ) | ( ( RLMOD_ARGL32(what) << 24 ) & 0x0ff000000 ); break; }
        case 25: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  7 ) & 0x001ffffff ) | ( ( RLMOD_ARGL32(what) << 25 ) & 0x0fe000000 ); break; }
        case 26: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  6 ) & 0x003ffffff ) | ( ( RLMOD_ARGL32(what) << 26 ) & 0x0fc000000 ); break; }
        case 27: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  5 ) & 0x007ffffff ) | ( ( RLMOD_ARGL32(what) << 27 ) & 0x0f8000000 ); break; }
        case 28: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  4 ) & 0x00fffffff ) | ( ( RLMOD_ARGL32(what) << 28 ) & 0x0f0000000 ); break; }
        case 29: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  3 ) & 0x01fffffff ) | ( ( RLMOD_ARGL32(what) << 29 ) & 0x0e0000000 ); break; }
        case 30: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  2 ) & 0x03fffffff ) | ( ( RLMOD_ARGL32(what) << 30 ) & 0x0c0000000 ); break; }
        default: { RLMOD_RES32(what) = ( ( RLMOD_ARGL32(what) >>  1 ) & 0x07fffffff ) | ( ( RLMOD_ARGL32(what) << 31 ) & 0x080000000 ); break; }
    }

    return;
}







/**********************************************************************

ALU module 14: sr

**********************************************************************/

#define SRMOD_RES8(what)      DEREF_8BUS(what,0)
#define SRMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define SRMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define SRMOD_RES16(what)     DEREF_16BUS(what,0)
#define SRMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define SRMOD_ARGR16(what)    DEREF_8BUS(what,3)
#define SRMOD_RES32(what)     DEREF_32BUS(what,0)
#define SRMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define SRMOD_ARGR32(what)    DEREF_8BUS(what,4)

void srmod_8(void *what);
void srmod_16(void *what);
void srmod_32(void *what);

module_data *srmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,5,2,2,3,0);

    return what;
}

int srmod_init(module_data *what)
{
    DEREF_INFN(what,0) = srmod_8;
    DEREF_INFN(what,1) = srmod_16;
    DEREF_INFN(what,2) = srmod_32;

    return 0;
}

void srmod_go(module_data *what)
{
    return;

    what = NULL;
}

void srmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void srmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void srmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *srmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void srmod_8(void *what)
{
    switch ( SRMOD_ARGR8(what) )
    {
        case 0: { SRMOD_RES8(what) =   SRMOD_ARGL8(what);                break; }
        case 1: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 1 ) & 0x07f; break; }
        case 2: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 2 ) & 0x03f; break; }
        case 3: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 3 ) & 0x01f; break; }
        case 4: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 4 ) & 0x00f; break; }
        case 5: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 5 ) & 0x007; break; }
        case 6: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 6 ) & 0x003; break; }
        case 7: { SRMOD_RES8(what) = ( SRMOD_ARGL8(what) >> 7 ) & 0x001; break; }

        default:
        {
            SRMOD_RES8(what) = 0;

            break;
        }
    }

    return;
}

void srmod_16(void *what)
{
    switch ( SRMOD_ARGR16(what) )
    {
        case  0: { SRMOD_RES16(what) =   SRMOD_ARGL16(what);                   break; }
        case  1: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  1 ) & 0x07fff; break; }
        case  2: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  2 ) & 0x03fff; break; }
        case  3: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  3 ) & 0x01fff; break; }
        case  4: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  4 ) & 0x00fff; break; }
        case  5: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  5 ) & 0x007ff; break; }
        case  6: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  6 ) & 0x003ff; break; }
        case  7: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  7 ) & 0x001ff; break; }
        case  8: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  8 ) & 0x000ff; break; }
        case  9: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >>  9 ) & 0x0007f; break; }
        case 10: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >> 10 ) & 0x0003f; break; }
        case 11: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >> 11 ) & 0x0001f; break; }
        case 12: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >> 12 ) & 0x0000f; break; }
        case 13: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >> 13 ) & 0x00007; break; }
        case 14: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >> 14 ) & 0x00003; break; }
        case 15: { SRMOD_RES16(what) = ( SRMOD_ARGL16(what) >> 15 ) & 0x00001; break; }

        default:
        {
            SRMOD_RES16(what) = 0;

            break;
        }
    }

    return;
}

void srmod_32(void *what)
{
    switch ( SRMOD_ARGR32(what) )
    {
        case  0: { SRMOD_RES32(what) =   SRMOD_ARGL32(what);                       break; }
        case  1: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  1 ) & 0x07fffffff; break; }
        case  2: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  2 ) & 0x03fffffff; break; }
        case  3: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  3 ) & 0x01fffffff; break; }
        case  4: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  4 ) & 0x00fffffff; break; }
        case  5: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  5 ) & 0x007ffffff; break; }
        case  6: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  6 ) & 0x003ffffff; break; }
        case  7: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  7 ) & 0x001ffffff; break; }
        case  8: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  8 ) & 0x000ffffff; break; }
        case  9: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >>  9 ) & 0x0007fffff; break; }
        case 10: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 10 ) & 0x0003fffff; break; }
        case 11: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 11 ) & 0x0001fffff; break; }
        case 12: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 12 ) & 0x0000fffff; break; }
        case 13: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 13 ) & 0x00007ffff; break; }
        case 14: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 14 ) & 0x00003ffff; break; }
        case 15: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 15 ) & 0x00001ffff; break; }
        case 16: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 16 ) & 0x00000ffff; break; }
        case 17: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 17 ) & 0x000007fff; break; }
        case 18: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 18 ) & 0x000003fff; break; }
        case 19: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 19 ) & 0x000001fff; break; }
        case 20: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 20 ) & 0x000000fff; break; }
        case 21: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 21 ) & 0x0000007ff; break; }
        case 22: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 22 ) & 0x0000003ff; break; }
        case 23: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 23 ) & 0x0000001ff; break; }
        case 24: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 24 ) & 0x0000000ff; break; }
        case 25: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 25 ) & 0x00000007f; break; }
        case 26: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 26 ) & 0x00000003f; break; }
        case 27: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 27 ) & 0x00000001f; break; }
        case 28: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 28 ) & 0x00000000f; break; }
        case 29: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 29 ) & 0x000000007; break; }
        case 30: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 30 ) & 0x000000003; break; }
        case 31: { SRMOD_RES32(what) = ( SRMOD_ARGL32(what) >> 31 ) & 0x000000001; break; }

        default:
        {
            SRMOD_RES32(what) = 0;

            break;
        }
    }

    return;
}






/**********************************************************************

ALU module 15: sl

**********************************************************************/

#define SLMOD_RES8(what)      DEREF_8BUS(what,0)
#define SLMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define SLMOD_ARGR8(what)     DEREF_8BUS(what,2)
#define SLMOD_RES16(what)     DEREF_16BUS(what,0)
#define SLMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define SLMOD_ARGR16(what)    DEREF_8BUS(what,3)
#define SLMOD_RES32(what)     DEREF_32BUS(what,0)
#define SLMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define SLMOD_ARGR32(what)    DEREF_8BUS(what,4)

void slmod_8(void *what);
void slmod_16(void *what);
void slmod_32(void *what);

module_data *slmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,0,0,0,0,0,5,2,2,3,0);

    return what;
}

int slmod_init(module_data *what)
{
    DEREF_INFN(what,0) = slmod_8;
    DEREF_INFN(what,1) = slmod_16;
    DEREF_INFN(what,2) = slmod_32;

    return 0;
}

void slmod_go(module_data *what)
{
    return;

    what = NULL;
}

void slmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void slmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void slmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *slmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void slmod_8(void *what)
{
    SLMOD_RES8(what) = SLMOD_ARGL8(what) << SLMOD_ARGR8(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}

void slmod_16(void *what)
{
    SLMOD_RES16(what) = SLMOD_ARGL16(what) << SLMOD_ARGR16(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}

void slmod_32(void *what)
{
    SLMOD_RES32(what) = SLMOD_ARGL32(what) << SLMOD_ARGR32(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}









/**********************************************************************

ALU module 16: assignconst

**********************************************************************/

#define ASSIGNCONSTMOD_DEST8(what)     DEREF_8BUS(what,0)
#define ASSIGNCONSTMOD_SRC8(what)      DEREF_8VAR(what,0)
#define ASSIGNCONSTMOD_DEST16(what)    DEREF_16BUS(what,0)
#define ASSIGNCONSTMOD_SRC16(what)     DEREF_16VAR(what,0)
#define ASSIGNCONSTMOD_DEST32(what)    DEREF_32BUS(what,0)
#define ASSIGNCONSTMOD_SRC32(what)     DEREF_32VAR(what,0)

void assignconstmod_nd_08xx_08xx(void *what);
void assignconstmod_nd_08xx_16l_(void *what);
void assignconstmod_nd_08xx_16h_(void *what);
void assignconstmod_nd_08xx_32ll(void *what);
void assignconstmod_nd_08xx_32lh(void *what);
void assignconstmod_nd_08xx_32hl(void *what);
void assignconstmod_nd_08xx_32hh(void *what);
void assignconstmod_nd_16l__08xx(void *what);
void assignconstmod_nd_16h__08xx(void *what);
void assignconstmod_nd_16xx_16xx(void *what);
void assignconstmod_nd_16xx_32l_(void *what);
void assignconstmod_nd_16xx_32h_(void *what);
void assignconstmod_nd_32ll_08xx(void *what);
void assignconstmod_nd_32lh_08xx(void *what);
void assignconstmod_nd_32hl_08xx(void *what);
void assignconstmod_nd_32hh_08xx(void *what);
void assignconstmod_nd_32l__16xx(void *what);
void assignconstmod_nd_32h__16xx(void *what);
void assignconstmod_nd_32xx_32xx(void *what);

void assignconstmod_d_08xx_08xx(void *what);
void assignconstmod_d_08xx_16l_(void *what);
void assignconstmod_d_08xx_16h_(void *what);
void assignconstmod_d_08xx_32ll(void *what);
void assignconstmod_d_08xx_32lh(void *what);
void assignconstmod_d_08xx_32hl(void *what);
void assignconstmod_d_08xx_32hh(void *what);
void assignconstmod_d_16l__08xx(void *what);
void assignconstmod_d_16h__08xx(void *what);
void assignconstmod_d_16xx_16xx(void *what);
void assignconstmod_d_16xx_32l_(void *what);
void assignconstmod_d_16xx_32h_(void *what);
void assignconstmod_d_32ll_08xx(void *what);
void assignconstmod_d_32lh_08xx(void *what);
void assignconstmod_d_32hl_08xx(void *what);
void assignconstmod_d_32hh_08xx(void *what);
void assignconstmod_d_32l__16xx(void *what);
void assignconstmod_d_32h__16xx(void *what);
void assignconstmod_d_32xx_32xx(void *what);

module_data *assignconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,1,1,1,38,0);

    ASSIGNCONSTMOD_SRC8(what)  = 0;
    ASSIGNCONSTMOD_SRC16(what) = 0;
    ASSIGNCONSTMOD_SRC32(what) = 0;

    return what;
}

int assignconstmod_init(module_data *what)
{
    DEREF_INFN(what,0)  = assignconstmod_nd_08xx_08xx;
    DEREF_INFN(what,1)  = assignconstmod_nd_08xx_16l_;
    DEREF_INFN(what,2)  = assignconstmod_nd_08xx_16h_;
    DEREF_INFN(what,3)  = assignconstmod_nd_08xx_32ll;
    DEREF_INFN(what,4)  = assignconstmod_nd_08xx_32lh;
    DEREF_INFN(what,5)  = assignconstmod_nd_08xx_32hl;
    DEREF_INFN(what,6)  = assignconstmod_nd_08xx_32hh;
    DEREF_INFN(what,7)  = assignconstmod_nd_16l__08xx;
    DEREF_INFN(what,8)  = assignconstmod_nd_16h__08xx;
    DEREF_INFN(what,9)  = assignconstmod_nd_16xx_16xx;
    DEREF_INFN(what,10) = assignconstmod_nd_16xx_32l_;
    DEREF_INFN(what,11) = assignconstmod_nd_16xx_32h_;
    DEREF_INFN(what,12) = assignconstmod_nd_32ll_08xx;
    DEREF_INFN(what,13) = assignconstmod_nd_32lh_08xx;
    DEREF_INFN(what,14) = assignconstmod_nd_32hl_08xx;
    DEREF_INFN(what,15) = assignconstmod_nd_32hh_08xx;
    DEREF_INFN(what,16) = assignconstmod_nd_32l__16xx;
    DEREF_INFN(what,17) = assignconstmod_nd_32h__16xx;
    DEREF_INFN(what,18) = assignconstmod_nd_32xx_32xx;
    DEREF_INFN(what,19) = assignconstmod_d_08xx_08xx;
    DEREF_INFN(what,20) = assignconstmod_d_08xx_16l_;
    DEREF_INFN(what,21) = assignconstmod_d_08xx_16h_;
    DEREF_INFN(what,22) = assignconstmod_d_08xx_32ll;
    DEREF_INFN(what,23) = assignconstmod_d_08xx_32lh;
    DEREF_INFN(what,24) = assignconstmod_d_08xx_32hl;
    DEREF_INFN(what,25) = assignconstmod_d_08xx_32hh;
    DEREF_INFN(what,26) = assignconstmod_d_16l__08xx;
    DEREF_INFN(what,27) = assignconstmod_d_16h__08xx;
    DEREF_INFN(what,28) = assignconstmod_d_16xx_16xx;
    DEREF_INFN(what,29) = assignconstmod_d_16xx_32l_;
    DEREF_INFN(what,30) = assignconstmod_d_16xx_32h_;
    DEREF_INFN(what,31) = assignconstmod_d_32ll_08xx;
    DEREF_INFN(what,32) = assignconstmod_d_32lh_08xx;
    DEREF_INFN(what,33) = assignconstmod_d_32hl_08xx;
    DEREF_INFN(what,34) = assignconstmod_d_32hh_08xx;
    DEREF_INFN(what,35) = assignconstmod_d_32l__16xx;
    DEREF_INFN(what,36) = assignconstmod_d_32h__16xx;
    DEREF_INFN(what,37) = assignconstmod_d_32xx_32xx;

    return 0;
}

void assignconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void assignconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void assignconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void assignconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *assignconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void assignconstmod_nd_08xx_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = ASSIGNCONSTMOD_SRC8(what);

    return;
}

void assignconstmod_nd_08xx_16l_(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ASSIGNCONSTMOD_SRC16(what) & 0x0ff );

    return;
}

void assignconstmod_nd_08xx_16h_(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC16(what) >> 8 ) & 0x0ff );

    return;
}

void assignconstmod_nd_08xx_32ll(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ASSIGNCONSTMOD_SRC32(what) & 0x0ff );

    return;
}

void assignconstmod_nd_08xx_32lh(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC32(what) >> 8 ) & 0x0ff );

    return;
}

void assignconstmod_nd_08xx_32hl(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC32(what) >> 16 ) & 0x0ff );

    return;
}

void assignconstmod_nd_08xx_32hh(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC32(what) >> 24 ) & 0x0ff );

    return;
}

void assignconstmod_nd_16l__08xx(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) &= 0x0ff00;
    ASSIGNCONSTMOD_DEST16(what) |= ( (UINT_16) ASSIGNCONSTMOD_SRC8(what) ) & 0x000ff;

    return;
}

void assignconstmod_nd_16h__08xx(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) &= 0x000ff;
    ASSIGNCONSTMOD_DEST16(what) |= ( ( (UINT_16) ASSIGNCONSTMOD_SRC8(what) ) << 8 ) & 0x0ff00;

    return;
}

void assignconstmod_nd_16xx_16xx(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = ASSIGNCONSTMOD_SRC16(what);

    return;
}

void assignconstmod_nd_16xx_32l_(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = (UINT_16) ( ASSIGNCONSTMOD_SRC32(what) & 0x0ffff );

    return;
}

void assignconstmod_nd_16xx_32h_(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = (UINT_16) ( ( ASSIGNCONSTMOD_SRC32(what) >> 16 ) & 0x0ffff );

    return;
}

void assignconstmod_nd_32ll_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) &= 0x0ffffff00;
    ASSIGNCONSTMOD_DEST32(what) |= ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) & 0x0000000ff;

    return;
}

void assignconstmod_nd_32lh_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) &= 0x0ffff00ff;
    ASSIGNCONSTMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) << 8 ) & 0x00000ff00;

    return;
}

void assignconstmod_nd_32hl_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) &= 0x0ff00ffff;
    ASSIGNCONSTMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) << 16 ) & 0x000ff0000;

    return;
}

void assignconstmod_nd_32hh_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) &= 0x000ffffff;
    ASSIGNCONSTMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) << 24 ) & 0x0ff000000;

    return;
}

void assignconstmod_nd_32l__16xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) &= 0x0ffff0000;
    ASSIGNCONSTMOD_DEST32(what) |= ( (UINT_32) ASSIGNCONSTMOD_SRC16(what) ) & 0x00000ffff;

    return;
}

void assignconstmod_nd_32h__16xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) &= 0x00000ffff;
    ASSIGNCONSTMOD_DEST32(what) |= ( ( (UINT_32) ASSIGNCONSTMOD_SRC16(what) ) << 16 ) & 0x0ffff0000;

    return;
}

void assignconstmod_nd_32xx_32xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ASSIGNCONSTMOD_SRC32(what);

    return;
}

void assignconstmod_d_08xx_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = ASSIGNCONSTMOD_SRC8(what);

    return;
}

void assignconstmod_d_08xx_16l_(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ASSIGNCONSTMOD_SRC16(what) & 0x0ff );

    return;
}

void assignconstmod_d_08xx_16h_(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC16(what) >> 8 ) & 0x0ff );

    return;
}

void assignconstmod_d_08xx_32ll(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ASSIGNCONSTMOD_SRC32(what) & 0x0ff );

    return;
}

void assignconstmod_d_08xx_32lh(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC32(what) >> 8 ) & 0x0ff );

    return;
}

void assignconstmod_d_08xx_32hl(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC32(what) >> 16 ) & 0x0ff );

    return;
}

void assignconstmod_d_08xx_32hh(void *what)
{
    ASSIGNCONSTMOD_DEST8(what) = (UINT_8) ( ( ASSIGNCONSTMOD_SRC32(what) >> 24 ) & 0x0ff );

    return;
}

void assignconstmod_d_16l__08xx(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = ( (UINT_16) ASSIGNCONSTMOD_SRC8(what) ) & 0x000ff;

    return;
}

void assignconstmod_d_16h__08xx(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = ( ( (UINT_16) ASSIGNCONSTMOD_SRC8(what) ) << 8 ) & 0x0ff00;

    return;
}

void assignconstmod_d_16xx_16xx(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = ASSIGNCONSTMOD_SRC16(what);

    return;
}

void assignconstmod_d_16xx_32l_(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = (UINT_16) ( ASSIGNCONSTMOD_SRC32(what) & 0x0ffff );

    return;
}

void assignconstmod_d_16xx_32h_(void *what)
{
    ASSIGNCONSTMOD_DEST16(what) = (UINT_16) ( ( ASSIGNCONSTMOD_SRC32(what) >> 16 ) & 0x0ffff );

    return;
}

void assignconstmod_d_32ll_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) & 0x0000000ff;

    return;
}

void assignconstmod_d_32lh_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ( ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) << 8 ) & 0x00000ff00;

    return;
}

void assignconstmod_d_32hl_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ( ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) << 16 ) & 0x000ff0000;

    return;
}

void assignconstmod_d_32hh_08xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ( ( (UINT_32) ASSIGNCONSTMOD_SRC8(what) ) << 24 ) & 0x0ff000000;

    return;
}

void assignconstmod_d_32l__16xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ( (UINT_32) ASSIGNCONSTMOD_SRC16(what) ) & 0x00000ffff;

    return;
}

void assignconstmod_d_32h__16xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ( ( (UINT_32) ASSIGNCONSTMOD_SRC16(what) ) << 16 ) & 0x0ffff0000;

    return;
}

void assignconstmod_d_32xx_32xx(void *what)
{
    ASSIGNCONSTMOD_DEST32(what) = ASSIGNCONSTMOD_SRC32(what);

    return;
}




/**********************************************************************

ALU module 17: addconst

**********************************************************************/

#define ADDCONSTMOD_RES8(what)      DEREF_8BUS(what,0)
#define ADDCONSTMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define ADDCONSTMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define ADDCONSTMOD_RES16(what)     DEREF_16BUS(what,0)
#define ADDCONSTMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define ADDCONSTMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define ADDCONSTMOD_RES32(what)     DEREF_32BUS(what,0)
#define ADDCONSTMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define ADDCONSTMOD_ARGR32(what)    DEREF_32VAR(what,0)

void addconstmod_8(void *what);
void addconstmod_16(void *what);
void addconstmod_32(void *what);

module_data *addconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    ADDCONSTMOD_ARGR8(what)  = 0;
    ADDCONSTMOD_ARGR16(what) = 0;
    ADDCONSTMOD_ARGR32(what) = 0;

    return what;
}

int addconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = addconstmod_8;
    DEREF_INFN(what,1) = addconstmod_16;
    DEREF_INFN(what,2) = addconstmod_32;

    return 0;
}

void addconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void addconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void addconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void addconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *addconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void addconstmod_8(void *what)
{
    ADDCONSTMOD_RES8(what) = ADDCONSTMOD_ARGL8(what) + ADDCONSTMOD_ARGR8(what);

    return;
}

void addconstmod_16(void *what)
{
    ADDCONSTMOD_RES16(what) = ADDCONSTMOD_ARGL16(what) + ADDCONSTMOD_ARGR16(what);

    return;
}

void addconstmod_32(void *what)
{
    ADDCONSTMOD_RES32(what) = ADDCONSTMOD_ARGL32(what) + ADDCONSTMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 18: subconsta

**********************************************************************/

#define SUBCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define SUBCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define SUBCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define SUBCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define SUBCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define SUBCONSTAMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define SUBCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define SUBCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define SUBCONSTAMOD_ARGR32(what)    DEREF_32VAR(what,0)

void subconstamod_8(void *what);
void subconstamod_16(void *what);
void subconstamod_32(void *what);

module_data *subconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    SUBCONSTAMOD_ARGR8(what)  = 0;
    SUBCONSTAMOD_ARGR16(what) = 0;
    SUBCONSTAMOD_ARGR32(what) = 0;

    return what;
}

int subconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = subconstamod_8;
    DEREF_INFN(what,1) = subconstamod_16;
    DEREF_INFN(what,2) = subconstamod_32;

    return 0;
}

void subconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void subconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void subconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void subconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *subconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void subconstamod_8(void *what)
{
    SUBCONSTAMOD_RES8(what) = SUBCONSTAMOD_ARGL8(what) - SUBCONSTAMOD_ARGR8(what);

    return;
}

void subconstamod_16(void *what)
{
    SUBCONSTAMOD_RES16(what) = SUBCONSTAMOD_ARGL16(what) - SUBCONSTAMOD_ARGR16(what);

    return;
}

void subconstamod_32(void *what)
{
    SUBCONSTAMOD_RES32(what) = SUBCONSTAMOD_ARGL32(what) - SUBCONSTAMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 19: subconstb

**********************************************************************/

#define SUBCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define SUBCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define SUBCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define SUBCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define SUBCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define SUBCONSTBMOD_ARGR16(what)    DEREF_16BUS(what,1)
#define SUBCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define SUBCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define SUBCONSTBMOD_ARGR32(what)    DEREF_32BUS(what,1)

void subconstbmod_8(void *what);
void subconstbmod_16(void *what);
void subconstbmod_32(void *what);

module_data *subconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    SUBCONSTBMOD_ARGL8(what)  = 0;
    SUBCONSTBMOD_ARGL16(what) = 0;
    SUBCONSTBMOD_ARGL32(what) = 0;

    return what;
}

int subconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = subconstbmod_8;
    DEREF_INFN(what,1) = subconstbmod_16;
    DEREF_INFN(what,2) = subconstbmod_32;

    return 0;
}

void subconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void subconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void subconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void subconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *subconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void subconstbmod_8(void *what)
{
    SUBCONSTBMOD_RES8(what) = SUBCONSTBMOD_ARGL8(what) - SUBCONSTBMOD_ARGR8(what);

    return;
}

void subconstbmod_16(void *what)
{
    SUBCONSTBMOD_RES16(what) = SUBCONSTBMOD_ARGL16(what) - SUBCONSTBMOD_ARGR16(what);

    return;
}

void subconstbmod_32(void *what)
{
    SUBCONSTBMOD_RES32(what) = SUBCONSTBMOD_ARGL32(what) - SUBCONSTBMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 20: mulconst

**********************************************************************/

#define MULCONSTMOD_RES8(what)      DEREF_8BUS(what,0)
#define MULCONSTMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define MULCONSTMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define MULCONSTMOD_RES16(what)     DEREF_16BUS(what,0)
#define MULCONSTMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define MULCONSTMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define MULCONSTMOD_RES32(what)     DEREF_32BUS(what,0)
#define MULCONSTMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define MULCONSTMOD_ARGR32(what)    DEREF_32VAR(what,0)

void mulconstmod_8(void *what);
void mulconstmod_16(void *what);
void mulconstmod_32(void *what);

module_data *mulconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    MULCONSTMOD_ARGR8(what)  = 0;
    MULCONSTMOD_ARGR16(what) = 0;
    MULCONSTMOD_ARGR32(what) = 0;

    return what;
}

int mulconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = mulconstmod_8;
    DEREF_INFN(what,1) = mulconstmod_16;
    DEREF_INFN(what,2) = mulconstmod_32;

    return 0;
}

void mulconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void mulconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void mulconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void mulconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *mulconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void mulconstmod_8(void *what)
{
    MULCONSTMOD_RES8(what) = MULCONSTMOD_ARGL8(what) * MULCONSTMOD_ARGR8(what);

    return;
}

void mulconstmod_16(void *what)
{
    MULCONSTMOD_RES16(what) = MULCONSTMOD_ARGL16(what) * MULCONSTMOD_ARGR16(what);

    return;
}

void mulconstmod_32(void *what)
{
    MULCONSTMOD_RES32(what) = MULCONSTMOD_ARGL32(what) * MULCONSTMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 21: divconsta

**********************************************************************/

#define DIVCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define DIVCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define DIVCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define DIVCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define DIVCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define DIVCONSTAMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define DIVCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define DIVCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define DIVCONSTAMOD_ARGR32(what)    DEREF_32VAR(what,0)

void divconstamod_8(void *what);
void divconstamod_16(void *what);
void divconstamod_32(void *what);

module_data *divconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    DIVCONSTAMOD_ARGR8(what)  = 0;
    DIVCONSTAMOD_ARGR16(what) = 0;
    DIVCONSTAMOD_ARGR32(what) = 0;

    return what;
}

int divconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = divconstamod_8;
    DEREF_INFN(what,1) = divconstamod_16;
    DEREF_INFN(what,2) = divconstamod_32;

    return 0;
}

void divconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void divconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void divconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void divconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *divconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void divconstamod_8(void *what)
{
    DIVCONSTAMOD_RES8(what) = DIVCONSTAMOD_ARGL8(what) / DIVCONSTAMOD_ARGR8(what);

    return;
}

void divconstamod_16(void *what)
{
    DIVCONSTAMOD_RES16(what) = DIVCONSTAMOD_ARGL16(what) / DIVCONSTAMOD_ARGR16(what);

    return;
}

void divconstamod_32(void *what)
{
    DIVCONSTAMOD_RES32(what) = DIVCONSTAMOD_ARGL32(what) / DIVCONSTAMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 22: divconstb

**********************************************************************/

#define DIVCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define DIVCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define DIVCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define DIVCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define DIVCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define DIVCONSTBMOD_ARGR16(what)    DEREF_16BUS(what,1)
#define DIVCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define DIVCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define DIVCONSTBMOD_ARGR32(what)    DEREF_32BUS(what,1)

void divconstbmod_8(void *what);
void divconstbmod_16(void *what);
void divconstbmod_32(void *what);

module_data *divconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    DIVCONSTBMOD_ARGL8(what)  = 0;
    DIVCONSTBMOD_ARGL16(what) = 0;
    DIVCONSTBMOD_ARGL32(what) = 0;

    return what;
}

int divconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = divconstbmod_8;
    DEREF_INFN(what,1) = divconstbmod_16;
    DEREF_INFN(what,2) = divconstbmod_32;

    return 0;
}

void divconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void divconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void divconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void divconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *divconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void divconstbmod_8(void *what)
{
    DIVCONSTBMOD_RES8(what) = DIVCONSTBMOD_ARGL8(what) / DIVCONSTBMOD_ARGR8(what);

    return;
}

void divconstbmod_16(void *what)
{
    DIVCONSTBMOD_RES16(what) = DIVCONSTBMOD_ARGL16(what) / DIVCONSTBMOD_ARGR16(what);

    return;
}

void divconstbmod_32(void *what)
{
    DIVCONSTBMOD_RES32(what) = DIVCONSTBMOD_ARGL32(what) / DIVCONSTBMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 23: modconsta

**********************************************************************/

#define MODCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define MODCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define MODCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define MODCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define MODCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define MODCONSTAMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define MODCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define MODCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define MODCONSTAMOD_ARGR32(what)    DEREF_32VAR(what,0)

void modconstamod_8(void *what);
void modconstamod_16(void *what);
void modconstamod_32(void *what);

module_data *modconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    MODCONSTAMOD_ARGR8(what)  = 0;
    MODCONSTAMOD_ARGR16(what) = 0;
    MODCONSTAMOD_ARGR32(what) = 0;

    return what;
}

int modconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = modconstamod_8;
    DEREF_INFN(what,1) = modconstamod_16;
    DEREF_INFN(what,2) = modconstamod_32;

    return 0;
}

void modconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void modconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void modconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void modconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *modconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void modconstamod_8(void *what)
{
    MODCONSTAMOD_RES8(what) = MODCONSTAMOD_ARGL8(what) % MODCONSTAMOD_ARGR8(what);

    return;
}

void modconstamod_16(void *what)
{
    MODCONSTAMOD_RES16(what) = MODCONSTAMOD_ARGL16(what) % MODCONSTAMOD_ARGR16(what);

    return;
}

void modconstamod_32(void *what)
{
    MODCONSTAMOD_RES32(what) = MODCONSTAMOD_ARGL32(what) % MODCONSTAMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 24: modconstb

**********************************************************************/

#define MODCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define MODCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define MODCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define MODCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define MODCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define MODCONSTBMOD_ARGR16(what)    DEREF_16BUS(what,1)
#define MODCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define MODCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define MODCONSTBMOD_ARGR32(what)    DEREF_32BUS(what,1)

void modconstbmod_8(void *what);
void modconstbmod_16(void *what);
void modconstbmod_32(void *what);

module_data *modconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    MODCONSTBMOD_ARGL8(what)  = 0;
    MODCONSTBMOD_ARGL16(what) = 0;
    MODCONSTBMOD_ARGL32(what) = 0;

    return what;
}

int modconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = modconstbmod_8;
    DEREF_INFN(what,1) = modconstbmod_16;
    DEREF_INFN(what,2) = modconstbmod_32;

    return 0;
}

void modconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void modconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void modconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void modconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *modconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void modconstbmod_8(void *what)
{
    MODCONSTBMOD_RES8(what) = MODCONSTBMOD_ARGL8(what) % MODCONSTBMOD_ARGR8(what);

    return;
}

void modconstbmod_16(void *what)
{
    MODCONSTBMOD_RES16(what) = MODCONSTBMOD_ARGL16(what) % MODCONSTBMOD_ARGR16(what);

    return;
}

void modconstbmod_32(void *what)
{
    MODCONSTBMOD_RES32(what) = MODCONSTBMOD_ARGL32(what) % MODCONSTBMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 25: andconst

**********************************************************************/

#define ANDCONSTMOD_RES8(what)      DEREF_8BUS(what,0)
#define ANDCONSTMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define ANDCONSTMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define ANDCONSTMOD_RES16(what)     DEREF_16BUS(what,0)
#define ANDCONSTMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define ANDCONSTMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define ANDCONSTMOD_RES32(what)     DEREF_32BUS(what,0)
#define ANDCONSTMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define ANDCONSTMOD_ARGR32(what)    DEREF_32VAR(what,0)

void andconstmod_8(void *what);
void andconstmod_16(void *what);
void andconstmod_32(void *what);

module_data *andconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    ANDCONSTMOD_ARGR8(what)  = 0;
    ANDCONSTMOD_ARGR16(what) = 0;
    ANDCONSTMOD_ARGR32(what) = 0;

    return what;
}

int andconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = andconstmod_8;
    DEREF_INFN(what,1) = andconstmod_16;
    DEREF_INFN(what,2) = andconstmod_32;

    return 0;
}

void andconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void andconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void andconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void andconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *andconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void andconstmod_8(void *what)
{
    ANDCONSTMOD_RES8(what) = ANDCONSTMOD_ARGL8(what) & ANDCONSTMOD_ARGR8(what);

    return;
}

void andconstmod_16(void *what)
{
    ANDCONSTMOD_RES16(what) = ANDCONSTMOD_ARGL16(what) & ANDCONSTMOD_ARGR16(what);

    return;
}

void andconstmod_32(void *what)
{
    ANDCONSTMOD_RES32(what) = ANDCONSTMOD_ARGL32(what) & ANDCONSTMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 26: orconst

**********************************************************************/

#define ORCONSTMOD_RES8(what)      DEREF_8BUS(what,0)
#define ORCONSTMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define ORCONSTMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define ORCONSTMOD_RES16(what)     DEREF_16BUS(what,0)
#define ORCONSTMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define ORCONSTMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define ORCONSTMOD_RES32(what)     DEREF_32BUS(what,0)
#define ORCONSTMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define ORCONSTMOD_ARGR32(what)    DEREF_32VAR(what,0)

void orconstmod_8(void *what);
void orconstmod_16(void *what);
void orconstmod_32(void *what);

module_data *orconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    ORCONSTMOD_ARGR8(what)  = 0;
    ORCONSTMOD_ARGR16(what) = 0;
    ORCONSTMOD_ARGR32(what) = 0;

    return what;
}

int orconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = orconstmod_8;
    DEREF_INFN(what,1) = orconstmod_16;
    DEREF_INFN(what,2) = orconstmod_32;

    return 0;
}

void orconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void orconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void orconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void orconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *orconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void orconstmod_8(void *what)
{
    ORCONSTMOD_RES8(what) = ORCONSTMOD_ARGL8(what) | ORCONSTMOD_ARGR8(what);

    return;
}

void orconstmod_16(void *what)
{
    ORCONSTMOD_RES16(what) = ORCONSTMOD_ARGL16(what) | ORCONSTMOD_ARGR16(what);

    return;
}

void orconstmod_32(void *what)
{
    ORCONSTMOD_RES32(what) = ORCONSTMOD_ARGL32(what) | ORCONSTMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 27: xorconst

**********************************************************************/

#define XORCONSTMOD_RES8(what)      DEREF_8BUS(what,0)
#define XORCONSTMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define XORCONSTMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define XORCONSTMOD_RES16(what)     DEREF_16BUS(what,0)
#define XORCONSTMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define XORCONSTMOD_ARGR16(what)    DEREF_16VAR(what,0)
#define XORCONSTMOD_RES32(what)     DEREF_32BUS(what,0)
#define XORCONSTMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define XORCONSTMOD_ARGR32(what)    DEREF_32VAR(what,0)

void xorconstmod_8(void *what);
void xorconstmod_16(void *what);
void xorconstmod_32(void *what);

module_data *xorconstmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,2,2,2,3,0);

    XORCONSTMOD_ARGR8(what)  = 0;
    XORCONSTMOD_ARGR16(what) = 0;
    XORCONSTMOD_ARGR32(what) = 0;

    return what;
}

int xorconstmod_init(module_data *what)
{
    DEREF_INFN(what,0) = xorconstmod_8;
    DEREF_INFN(what,1) = xorconstmod_16;
    DEREF_INFN(what,2) = xorconstmod_32;

    return 0;
}

void xorconstmod_go(module_data *what)
{
    return;

    what = NULL;
}

void xorconstmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void xorconstmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void xorconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *xorconstmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void xorconstmod_8(void *what)
{
    XORCONSTMOD_RES8(what) = XORCONSTMOD_ARGL8(what) ^ XORCONSTMOD_ARGR8(what);

    return;
}

void xorconstmod_16(void *what)
{
    XORCONSTMOD_RES16(what) = XORCONSTMOD_ARGL16(what) ^ XORCONSTMOD_ARGR16(what);

    return;
}

void xorconstmod_32(void *what)
{
    XORCONSTMOD_RES32(what) = XORCONSTMOD_ARGL32(what) ^ XORCONSTMOD_ARGR32(what);

    return;
}







/**********************************************************************

ALU module 28: rrconsta

**********************************************************************/

#define RRCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define RRCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define RRCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define RRCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define RRCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define RRCONSTAMOD_ARGR16(what)    DEREF_8VAR(what,1)
#define RRCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define RRCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define RRCONSTAMOD_ARGR32(what)    DEREF_8VAR(what,2)

void rrconstamod_8(void *what);
void rrconstamod_16(void *what);
void rrconstamod_32(void *what);

module_data *rrconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,3,0,0,0,0,2,2,2,3,0);

    RRCONSTAMOD_ARGR8(what)  = 0;
    RRCONSTAMOD_ARGR16(what) = 0;
    RRCONSTAMOD_ARGR32(what) = 0;

    return what;
}

int rrconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = rrconstamod_8;
    DEREF_INFN(what,1) = rrconstamod_16;
    DEREF_INFN(what,2) = rrconstamod_32;

    return 0;
}

void rrconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void rrconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void rrconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void rrconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *rrconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void rrconstamod_8(void *what)
{
    switch ( RRCONSTAMOD_ARGR8(what)%8 )
    {
        case 0:  { RRCONSTAMOD_RES8(what) =     RRCONSTAMOD_ARGL8(what);                                                           break; }
        case 1:  { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 1 ) & 0x07f ) | ( ( RRCONSTAMOD_ARGL8(what) << 7 ) & 0x080 ); break; }
        case 2:  { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 2 ) & 0x03f ) | ( ( RRCONSTAMOD_ARGL8(what) << 6 ) & 0x0c0 ); break; }
        case 3:  { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 3 ) & 0x01f ) | ( ( RRCONSTAMOD_ARGL8(what) << 5 ) & 0x0e0 ); break; }
        case 4:  { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 4 ) & 0x00f ) | ( ( RRCONSTAMOD_ARGL8(what) << 4 ) & 0x0f0 ); break; }
        case 5:  { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 5 ) & 0x007 ) | ( ( RRCONSTAMOD_ARGL8(what) << 3 ) & 0x0f8 ); break; }
        case 6:  { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 6 ) & 0x003 ) | ( ( RRCONSTAMOD_ARGL8(what) << 2 ) & 0x0fc ); break; }
        default: { RRCONSTAMOD_RES8(what) = ( ( RRCONSTAMOD_ARGL8(what) >> 7 ) & 0x001 ) | ( ( RRCONSTAMOD_ARGL8(what) << 1 ) & 0x0fe ); break; }
    }

    return;
}

void rrconstamod_16(void *what)
{
    switch ( RRCONSTAMOD_ARGR16(what)%16 )
    {
        case  0: { RRCONSTAMOD_RES16(what) =     RRCONSTAMOD_ARGL16(what);                                                                  break; }
        case  1: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  1 ) & 0x07fff ) | ( ( RRCONSTAMOD_ARGL16(what) << 15 ) & 0x08000 ); break; }
        case  2: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  2 ) & 0x03fff ) | ( ( RRCONSTAMOD_ARGL16(what) << 14 ) & 0x0c000 ); break; }
        case  3: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  3 ) & 0x01fff ) | ( ( RRCONSTAMOD_ARGL16(what) << 13 ) & 0x0e000 ); break; }
        case  4: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  4 ) & 0x00fff ) | ( ( RRCONSTAMOD_ARGL16(what) << 12 ) & 0x0f000 ); break; }
        case  5: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  5 ) & 0x007ff ) | ( ( RRCONSTAMOD_ARGL16(what) << 11 ) & 0x0f800 ); break; }
        case  6: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  6 ) & 0x003ff ) | ( ( RRCONSTAMOD_ARGL16(what) << 10 ) & 0x0fc00 ); break; }
        case  7: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  7 ) & 0x001ff ) | ( ( RRCONSTAMOD_ARGL16(what) <<  9 ) & 0x0fe00 ); break; }
        case  8: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  8 ) & 0x000ff ) | ( ( RRCONSTAMOD_ARGL16(what) <<  8 ) & 0x0ff00 ); break; }
        case  9: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >>  9 ) & 0x0007f ) | ( ( RRCONSTAMOD_ARGL16(what) <<  7 ) & 0x0ff80 ); break; }
        case 10: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >> 10 ) & 0x0003f ) | ( ( RRCONSTAMOD_ARGL16(what) <<  6 ) & 0x0ffc0 ); break; }
        case 11: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >> 11 ) & 0x0001f ) | ( ( RRCONSTAMOD_ARGL16(what) <<  5 ) & 0x0ffe0 ); break; }
        case 12: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >> 12 ) & 0x0000f ) | ( ( RRCONSTAMOD_ARGL16(what) <<  4 ) & 0x0fff0 ); break; }
        case 13: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >> 13 ) & 0x00007 ) | ( ( RRCONSTAMOD_ARGL16(what) <<  3 ) & 0x0fff8 ); break; }
        case 14: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >> 14 ) & 0x00003 ) | ( ( RRCONSTAMOD_ARGL16(what) <<  2 ) & 0x0fffc ); break; }
        default: { RRCONSTAMOD_RES16(what) = ( ( RRCONSTAMOD_ARGL16(what) >> 15 ) & 0x00001 ) | ( ( RRCONSTAMOD_ARGL16(what) <<  1 ) & 0x0fffe ); break; }
    }

    return;
}

void rrconstamod_32(void *what)
{
    switch ( RRCONSTAMOD_ARGR32(what)%32 )
    {
        case  0: { RRCONSTAMOD_RES32(what) =     RRCONSTAMOD_ARGL32(what);                                                                          break; }
        case  1: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  1 ) & 0x07fffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 31 ) & 0x080000000 ); break; }
        case  2: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  2 ) & 0x03fffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 30 ) & 0x0c0000000 ); break; }
        case  3: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  3 ) & 0x01fffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 29 ) & 0x0e0000000 ); break; }
        case  4: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  4 ) & 0x00fffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 28 ) & 0x0f0000000 ); break; }
        case  5: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  5 ) & 0x007ffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 27 ) & 0x0f8000000 ); break; }
        case  6: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  6 ) & 0x003ffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 26 ) & 0x0fc000000 ); break; }
        case  7: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  7 ) & 0x001ffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 25 ) & 0x0fe000000 ); break; }
        case  8: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  8 ) & 0x000ffffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 24 ) & 0x0ff000000 ); break; }
        case  9: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >>  9 ) & 0x0007fffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 23 ) & 0x0ff800000 ); break; }
        case 10: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 10 ) & 0x0003fffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 22 ) & 0x0ffc00000 ); break; }
        case 11: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 11 ) & 0x0001fffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 21 ) & 0x0ffe00000 ); break; }
        case 12: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 12 ) & 0x0000fffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 20 ) & 0x0fff00000 ); break; }
        case 13: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 13 ) & 0x00007ffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 19 ) & 0x0fff80000 ); break; }
        case 14: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 14 ) & 0x00003ffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 18 ) & 0x0fffc0000 ); break; }
        case 15: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 15 ) & 0x00001ffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 17 ) & 0x0fffe0000 ); break; }
        case 16: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 16 ) & 0x00000ffff ) | ( ( RRCONSTAMOD_ARGL32(what) << 16 ) & 0x0ffff0000 ); break; }
        case 17: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 17 ) & 0x000007fff ) | ( ( RRCONSTAMOD_ARGL32(what) << 15 ) & 0x0ffff8000 ); break; }
        case 18: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 18 ) & 0x000003fff ) | ( ( RRCONSTAMOD_ARGL32(what) << 14 ) & 0x0ffffc000 ); break; }
        case 19: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 19 ) & 0x000001fff ) | ( ( RRCONSTAMOD_ARGL32(what) << 13 ) & 0x0ffffe000 ); break; }
        case 20: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 20 ) & 0x000000fff ) | ( ( RRCONSTAMOD_ARGL32(what) << 12 ) & 0x0fffff000 ); break; }
        case 21: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 21 ) & 0x0000007ff ) | ( ( RRCONSTAMOD_ARGL32(what) << 11 ) & 0x0fffff800 ); break; }
        case 22: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 22 ) & 0x0000003ff ) | ( ( RRCONSTAMOD_ARGL32(what) << 10 ) & 0x0fffffc00 ); break; }
        case 23: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 23 ) & 0x0000001ff ) | ( ( RRCONSTAMOD_ARGL32(what) <<  9 ) & 0x0fffffe00 ); break; }
        case 24: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 24 ) & 0x0000000ff ) | ( ( RRCONSTAMOD_ARGL32(what) <<  8 ) & 0x0ffffff00 ); break; }
        case 25: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 25 ) & 0x00000007f ) | ( ( RRCONSTAMOD_ARGL32(what) <<  7 ) & 0x0ffffff80 ); break; }
        case 26: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 26 ) & 0x00000003f ) | ( ( RRCONSTAMOD_ARGL32(what) <<  6 ) & 0x0ffffffc0 ); break; }
        case 27: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 27 ) & 0x00000001f ) | ( ( RRCONSTAMOD_ARGL32(what) <<  5 ) & 0x0ffffffe0 ); break; }
        case 28: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 28 ) & 0x00000000f ) | ( ( RRCONSTAMOD_ARGL32(what) <<  4 ) & 0x0fffffff0 ); break; }
        case 29: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 29 ) & 0x000000007 ) | ( ( RRCONSTAMOD_ARGL32(what) <<  3 ) & 0x0fffffff8 ); break; }
        case 30: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 30 ) & 0x000000003 ) | ( ( RRCONSTAMOD_ARGL32(what) <<  2 ) & 0x0fffffffc ); break; }
        default: { RRCONSTAMOD_RES32(what) = ( ( RRCONSTAMOD_ARGL32(what) >> 31 ) & 0x000000001 ) | ( ( RRCONSTAMOD_ARGL32(what) <<  1 ) & 0x0fffffffe ); break; }
    }

    return;
}







/**********************************************************************

ALU module 29: rrconstb

**********************************************************************/

#define RRCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define RRCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define RRCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define RRCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define RRCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define RRCONSTBMOD_ARGR16(what)    DEREF_8BUS(what,2)
#define RRCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define RRCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define RRCONSTBMOD_ARGR32(what)    DEREF_8BUS(what,3)

void rrconstbmod_8(void *what);
void rrconstbmod_16(void *what);
void rrconstbmod_32(void *what);

module_data *rrconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,4,1,1,3,0);

    RRCONSTBMOD_ARGL8(what)  = 0;
    RRCONSTBMOD_ARGL16(what) = 0;
    RRCONSTBMOD_ARGL32(what) = 0;

    return what;
}

int rrconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = rrconstbmod_8;
    DEREF_INFN(what,1) = rrconstbmod_16;
    DEREF_INFN(what,2) = rrconstbmod_32;

    return 0;
}

void rrconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void rrconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void rrconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void rrconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *rrconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void rrconstbmod_8(void *what)
{
    switch ( RRCONSTBMOD_ARGR8(what)%8 )
    {
        case 0:  { RRCONSTBMOD_RES8(what) =     RRCONSTBMOD_ARGL8(what);                                                           break; }
        case 1:  { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 1 ) & 0x07f ) | ( ( RRCONSTBMOD_ARGL8(what) << 7 ) & 0x080 ); break; }
        case 2:  { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 2 ) & 0x03f ) | ( ( RRCONSTBMOD_ARGL8(what) << 6 ) & 0x0c0 ); break; }
        case 3:  { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 3 ) & 0x01f ) | ( ( RRCONSTBMOD_ARGL8(what) << 5 ) & 0x0e0 ); break; }
        case 4:  { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 4 ) & 0x00f ) | ( ( RRCONSTBMOD_ARGL8(what) << 4 ) & 0x0f0 ); break; }
        case 5:  { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 5 ) & 0x007 ) | ( ( RRCONSTBMOD_ARGL8(what) << 3 ) & 0x0f8 ); break; }
        case 6:  { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 6 ) & 0x003 ) | ( ( RRCONSTBMOD_ARGL8(what) << 2 ) & 0x0fc ); break; }
        default: { RRCONSTBMOD_RES8(what) = ( ( RRCONSTBMOD_ARGL8(what) >> 7 ) & 0x001 ) | ( ( RRCONSTBMOD_ARGL8(what) << 1 ) & 0x0fe ); break; }
    }

    return;
}

void rrconstbmod_16(void *what)
{
    switch ( RRCONSTBMOD_ARGR16(what)%16 )
    {
        case  0: { RRCONSTBMOD_RES16(what) =     RRCONSTBMOD_ARGL16(what);                                                                  break; }
        case  1: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  1 ) & 0x07fff ) | ( ( RRCONSTBMOD_ARGL16(what) << 15 ) & 0x08000 ); break; }
        case  2: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  2 ) & 0x03fff ) | ( ( RRCONSTBMOD_ARGL16(what) << 14 ) & 0x0c000 ); break; }
        case  3: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  3 ) & 0x01fff ) | ( ( RRCONSTBMOD_ARGL16(what) << 13 ) & 0x0e000 ); break; }
        case  4: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  4 ) & 0x00fff ) | ( ( RRCONSTBMOD_ARGL16(what) << 12 ) & 0x0f000 ); break; }
        case  5: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  5 ) & 0x007ff ) | ( ( RRCONSTBMOD_ARGL16(what) << 11 ) & 0x0f800 ); break; }
        case  6: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  6 ) & 0x003ff ) | ( ( RRCONSTBMOD_ARGL16(what) << 10 ) & 0x0fc00 ); break; }
        case  7: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  7 ) & 0x001ff ) | ( ( RRCONSTBMOD_ARGL16(what) <<  9 ) & 0x0fe00 ); break; }
        case  8: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  8 ) & 0x000ff ) | ( ( RRCONSTBMOD_ARGL16(what) <<  8 ) & 0x0ff00 ); break; }
        case  9: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >>  9 ) & 0x0007f ) | ( ( RRCONSTBMOD_ARGL16(what) <<  7 ) & 0x0ff80 ); break; }
        case 10: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >> 10 ) & 0x0003f ) | ( ( RRCONSTBMOD_ARGL16(what) <<  6 ) & 0x0ffc0 ); break; }
        case 11: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >> 11 ) & 0x0001f ) | ( ( RRCONSTBMOD_ARGL16(what) <<  5 ) & 0x0ffe0 ); break; }
        case 12: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >> 12 ) & 0x0000f ) | ( ( RRCONSTBMOD_ARGL16(what) <<  4 ) & 0x0fff0 ); break; }
        case 13: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >> 13 ) & 0x00007 ) | ( ( RRCONSTBMOD_ARGL16(what) <<  3 ) & 0x0fff8 ); break; }
        case 14: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >> 14 ) & 0x00003 ) | ( ( RRCONSTBMOD_ARGL16(what) <<  2 ) & 0x0fffc ); break; }
        default: { RRCONSTBMOD_RES16(what) = ( ( RRCONSTBMOD_ARGL16(what) >> 15 ) & 0x00001 ) | ( ( RRCONSTBMOD_ARGL16(what) <<  1 ) & 0x0fffe ); break; }
    }

    return;
}

void rrconstbmod_32(void *what)
{
    switch ( RRCONSTBMOD_ARGR32(what)%32 )
    {
        case  0: { RRCONSTBMOD_RES32(what) =     RRCONSTBMOD_ARGL32(what);                                                                          break; }
        case  1: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  1 ) & 0x07fffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 31 ) & 0x080000000 ); break; }
        case  2: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  2 ) & 0x03fffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 30 ) & 0x0c0000000 ); break; }
        case  3: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  3 ) & 0x01fffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 29 ) & 0x0e0000000 ); break; }
        case  4: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  4 ) & 0x00fffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 28 ) & 0x0f0000000 ); break; }
        case  5: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  5 ) & 0x007ffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 27 ) & 0x0f8000000 ); break; }
        case  6: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  6 ) & 0x003ffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 26 ) & 0x0fc000000 ); break; }
        case  7: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  7 ) & 0x001ffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 25 ) & 0x0fe000000 ); break; }
        case  8: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  8 ) & 0x000ffffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 24 ) & 0x0ff000000 ); break; }
        case  9: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >>  9 ) & 0x0007fffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 23 ) & 0x0ff800000 ); break; }
        case 10: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 10 ) & 0x0003fffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 22 ) & 0x0ffc00000 ); break; }
        case 11: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 11 ) & 0x0001fffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 21 ) & 0x0ffe00000 ); break; }
        case 12: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 12 ) & 0x0000fffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 20 ) & 0x0fff00000 ); break; }
        case 13: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 13 ) & 0x00007ffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 19 ) & 0x0fff80000 ); break; }
        case 14: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 14 ) & 0x00003ffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 18 ) & 0x0fffc0000 ); break; }
        case 15: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 15 ) & 0x00001ffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 17 ) & 0x0fffe0000 ); break; }
        case 16: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 16 ) & 0x00000ffff ) | ( ( RRCONSTBMOD_ARGL32(what) << 16 ) & 0x0ffff0000 ); break; }
        case 17: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 17 ) & 0x000007fff ) | ( ( RRCONSTBMOD_ARGL32(what) << 15 ) & 0x0ffff8000 ); break; }
        case 18: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 18 ) & 0x000003fff ) | ( ( RRCONSTBMOD_ARGL32(what) << 14 ) & 0x0ffffc000 ); break; }
        case 19: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 19 ) & 0x000001fff ) | ( ( RRCONSTBMOD_ARGL32(what) << 13 ) & 0x0ffffe000 ); break; }
        case 20: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 20 ) & 0x000000fff ) | ( ( RRCONSTBMOD_ARGL32(what) << 12 ) & 0x0fffff000 ); break; }
        case 21: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 21 ) & 0x0000007ff ) | ( ( RRCONSTBMOD_ARGL32(what) << 11 ) & 0x0fffff800 ); break; }
        case 22: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 22 ) & 0x0000003ff ) | ( ( RRCONSTBMOD_ARGL32(what) << 10 ) & 0x0fffffc00 ); break; }
        case 23: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 23 ) & 0x0000001ff ) | ( ( RRCONSTBMOD_ARGL32(what) <<  9 ) & 0x0fffffe00 ); break; }
        case 24: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 24 ) & 0x0000000ff ) | ( ( RRCONSTBMOD_ARGL32(what) <<  8 ) & 0x0ffffff00 ); break; }
        case 25: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 25 ) & 0x00000007f ) | ( ( RRCONSTBMOD_ARGL32(what) <<  7 ) & 0x0ffffff80 ); break; }
        case 26: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 26 ) & 0x00000003f ) | ( ( RRCONSTBMOD_ARGL32(what) <<  6 ) & 0x0ffffffc0 ); break; }
        case 27: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 27 ) & 0x00000001f ) | ( ( RRCONSTBMOD_ARGL32(what) <<  5 ) & 0x0ffffffe0 ); break; }
        case 28: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 28 ) & 0x00000000f ) | ( ( RRCONSTBMOD_ARGL32(what) <<  4 ) & 0x0fffffff0 ); break; }
        case 29: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 29 ) & 0x000000007 ) | ( ( RRCONSTBMOD_ARGL32(what) <<  3 ) & 0x0fffffff8 ); break; }
        case 30: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 30 ) & 0x000000003 ) | ( ( RRCONSTBMOD_ARGL32(what) <<  2 ) & 0x0fffffffc ); break; }
        default: { RRCONSTBMOD_RES32(what) = ( ( RRCONSTBMOD_ARGL32(what) >> 31 ) & 0x000000001 ) | ( ( RRCONSTBMOD_ARGL32(what) <<  1 ) & 0x0fffffffe ); break; }
    }

    return;
}







/**********************************************************************

ALU module 30: rlconsta

**********************************************************************/

#define RLCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define RLCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define RLCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define RLCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define RLCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define RLCONSTAMOD_ARGR16(what)    DEREF_8VAR(what,1)
#define RLCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define RLCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define RLCONSTAMOD_ARGR32(what)    DEREF_8VAR(what,2)

void rlconstamod_8(void *what);
void rlconstamod_16(void *what);
void rlconstamod_32(void *what);

module_data *rlconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,3,0,0,0,0,2,2,2,3,0);

    RLCONSTAMOD_ARGR8(what)  = 0;
    RLCONSTAMOD_ARGR16(what) = 0;
    RLCONSTAMOD_ARGR32(what) = 0;

    return what;
}

int rlconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = rlconstamod_8;
    DEREF_INFN(what,1) = rlconstamod_16;
    DEREF_INFN(what,2) = rlconstamod_32;

    return 0;
}

void rlconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void rlconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void rlconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void rlconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *rlconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void rlconstamod_8(void *what)
{
    switch ( RLCONSTAMOD_ARGR8(what)%8 )
    {
        case 0:  { RLCONSTAMOD_RES8(what) =     RLCONSTAMOD_ARGL8(what);                                                           break; }
        case 1:  { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 7 ) & 0x001 ) | ( ( RLCONSTAMOD_ARGL8(what) << 1 ) & 0x0fe ); break; }
        case 2:  { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 6 ) & 0x003 ) | ( ( RLCONSTAMOD_ARGL8(what) << 2 ) & 0x0fc ); break; }
        case 3:  { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 5 ) & 0x007 ) | ( ( RLCONSTAMOD_ARGL8(what) << 3 ) & 0x0f8 ); break; }
        case 4:  { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 4 ) & 0x00f ) | ( ( RLCONSTAMOD_ARGL8(what) << 4 ) & 0x0f0 ); break; }
        case 5:  { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 3 ) & 0x01f ) | ( ( RLCONSTAMOD_ARGL8(what) << 5 ) & 0x0e0 ); break; }
        case 6:  { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 2 ) & 0x03f ) | ( ( RLCONSTAMOD_ARGL8(what) << 6 ) & 0x0c0 ); break; }
        default: { RLCONSTAMOD_RES8(what) = ( ( RLCONSTAMOD_ARGL8(what) >> 1 ) & 0x07f ) | ( ( RLCONSTAMOD_ARGL8(what) << 7 ) & 0x080 ); break; }
    }

    return;
}

void rlconstamod_16(void *what)
{
    switch ( RLCONSTAMOD_ARGR16(what)%16 )
    {
        case  0: { RLCONSTAMOD_RES16(what) =     RLCONSTAMOD_ARGL16(what);                                                                  break; }
        case  1: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >> 15 ) & 0x00001 ) | ( ( RLCONSTAMOD_ARGL16(what) <<  1 ) & 0x0fffe ); break; }
        case  2: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >> 14 ) & 0x00003 ) | ( ( RLCONSTAMOD_ARGL16(what) <<  2 ) & 0x0fffc ); break; }
        case  3: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >> 13 ) & 0x00007 ) | ( ( RLCONSTAMOD_ARGL16(what) <<  3 ) & 0x0fff8 ); break; }
        case  4: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >> 12 ) & 0x0000f ) | ( ( RLCONSTAMOD_ARGL16(what) <<  4 ) & 0x0fff0 ); break; }
        case  5: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >> 11 ) & 0x0001f ) | ( ( RLCONSTAMOD_ARGL16(what) <<  5 ) & 0x0ffe0 ); break; }
        case  6: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >> 10 ) & 0x0003f ) | ( ( RLCONSTAMOD_ARGL16(what) <<  6 ) & 0x0ffc0 ); break; }
        case  7: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  9 ) & 0x0007f ) | ( ( RLCONSTAMOD_ARGL16(what) <<  7 ) & 0x0ff80 ); break; }
        case  8: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  8 ) & 0x000ff ) | ( ( RLCONSTAMOD_ARGL16(what) <<  8 ) & 0x0ff00 ); break; }
        case  9: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  7 ) & 0x001ff ) | ( ( RLCONSTAMOD_ARGL16(what) <<  9 ) & 0x0fe00 ); break; }
        case 10: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  6 ) & 0x003ff ) | ( ( RLCONSTAMOD_ARGL16(what) << 10 ) & 0x0fc00 ); break; }
        case 11: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  5 ) & 0x007ff ) | ( ( RLCONSTAMOD_ARGL16(what) << 11 ) & 0x0f800 ); break; }
        case 12: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  4 ) & 0x00fff ) | ( ( RLCONSTAMOD_ARGL16(what) << 12 ) & 0x0f000 ); break; }
        case 13: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  3 ) & 0x01fff ) | ( ( RLCONSTAMOD_ARGL16(what) << 13 ) & 0x0e000 ); break; }
        case 14: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  2 ) & 0x03fff ) | ( ( RLCONSTAMOD_ARGL16(what) << 14 ) & 0x0c000 ); break; }
        default: { RLCONSTAMOD_RES16(what) = ( ( RLCONSTAMOD_ARGL16(what) >>  1 ) & 0x07fff ) | ( ( RLCONSTAMOD_ARGL16(what) << 15 ) & 0x08000 ); break; }
    }

    return;
}

void rlconstamod_32(void *what)
{
    switch ( RLCONSTAMOD_ARGR32(what)%32 )
    {
        case  0: { RLCONSTAMOD_RES32(what) =     RLCONSTAMOD_ARGL32(what);                                                                          break; }
        case  1: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 31 ) & 0x000000001 ) | ( ( RLCONSTAMOD_ARGL32(what) <<  1 ) & 0x0fffffffe ); break; }
        case  2: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 30 ) & 0x000000003 ) | ( ( RLCONSTAMOD_ARGL32(what) <<  2 ) & 0x0fffffffc ); break; }
        case  3: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 29 ) & 0x000000007 ) | ( ( RLCONSTAMOD_ARGL32(what) <<  3 ) & 0x0fffffff8 ); break; }
        case  4: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 28 ) & 0x00000000f ) | ( ( RLCONSTAMOD_ARGL32(what) <<  4 ) & 0x0fffffff0 ); break; }
        case  5: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 27 ) & 0x00000001f ) | ( ( RLCONSTAMOD_ARGL32(what) <<  5 ) & 0x0ffffffe0 ); break; }
        case  6: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 26 ) & 0x00000003f ) | ( ( RLCONSTAMOD_ARGL32(what) <<  6 ) & 0x0ffffffc0 ); break; }
        case  7: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 25 ) & 0x00000007f ) | ( ( RLCONSTAMOD_ARGL32(what) <<  7 ) & 0x0ffffff80 ); break; }
        case  8: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 24 ) & 0x0000000ff ) | ( ( RLCONSTAMOD_ARGL32(what) <<  8 ) & 0x0ffffff00 ); break; }
        case  9: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 23 ) & 0x0000001ff ) | ( ( RLCONSTAMOD_ARGL32(what) <<  9 ) & 0x0fffffe00 ); break; }
        case 10: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 22 ) & 0x0000003ff ) | ( ( RLCONSTAMOD_ARGL32(what) << 10 ) & 0x0fffffc00 ); break; }
        case 11: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 21 ) & 0x0000007ff ) | ( ( RLCONSTAMOD_ARGL32(what) << 11 ) & 0x0fffff800 ); break; }
        case 12: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 20 ) & 0x000000fff ) | ( ( RLCONSTAMOD_ARGL32(what) << 12 ) & 0x0fffff000 ); break; }
        case 13: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 19 ) & 0x000001fff ) | ( ( RLCONSTAMOD_ARGL32(what) << 13 ) & 0x0ffffe000 ); break; }
        case 14: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 18 ) & 0x000003fff ) | ( ( RLCONSTAMOD_ARGL32(what) << 14 ) & 0x0ffffc000 ); break; }
        case 15: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 17 ) & 0x000007fff ) | ( ( RLCONSTAMOD_ARGL32(what) << 15 ) & 0x0ffff8000 ); break; }
        case 16: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 16 ) & 0x00000ffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 16 ) & 0x0ffff0000 ); break; }
        case 17: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 15 ) & 0x00001ffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 17 ) & 0x0fffe0000 ); break; }
        case 18: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 14 ) & 0x00003ffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 18 ) & 0x0fffc0000 ); break; }
        case 19: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 13 ) & 0x00007ffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 19 ) & 0x0fff80000 ); break; }
        case 20: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 12 ) & 0x0000fffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 20 ) & 0x0fff00000 ); break; }
        case 21: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 11 ) & 0x0001fffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 21 ) & 0x0ffe00000 ); break; }
        case 22: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >> 10 ) & 0x0003fffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 22 ) & 0x0ffc00000 ); break; }
        case 23: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  9 ) & 0x0007fffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 23 ) & 0x0ff800000 ); break; }
        case 24: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  8 ) & 0x000ffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 24 ) & 0x0ff000000 ); break; }
        case 25: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  7 ) & 0x001ffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 25 ) & 0x0fe000000 ); break; }
        case 26: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  6 ) & 0x003ffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 26 ) & 0x0fc000000 ); break; }
        case 27: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  5 ) & 0x007ffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 27 ) & 0x0f8000000 ); break; }
        case 28: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  4 ) & 0x00fffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 28 ) & 0x0f0000000 ); break; }
        case 29: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  3 ) & 0x01fffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 29 ) & 0x0e0000000 ); break; }
        case 30: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  2 ) & 0x03fffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 30 ) & 0x0c0000000 ); break; }
        default: { RLCONSTAMOD_RES32(what) = ( ( RLCONSTAMOD_ARGL32(what) >>  1 ) & 0x07fffffff ) | ( ( RLCONSTAMOD_ARGL32(what) << 31 ) & 0x080000000 ); break; }
    }

    return;
}







/**********************************************************************

ALU module 31: rlconstb

**********************************************************************/

#define RLCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define RLCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define RLCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define RLCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define RLCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define RLCONSTBMOD_ARGR16(what)    DEREF_8BUS(what,2)
#define RLCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define RLCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define RLCONSTBMOD_ARGR32(what)    DEREF_8BUS(what,3)

void rlconstbmod_8(void *what);
void rlconstbmod_16(void *what);
void rlconstbmod_32(void *what);

module_data *rlconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,4,1,1,3,0);

    return what;
}

int rlconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = rlconstbmod_8;
    DEREF_INFN(what,1) = rlconstbmod_16;
    DEREF_INFN(what,2) = rlconstbmod_32;

    return 0;
}

void rlconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void rlconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void rlconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void rlconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *rlconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void rlconstbmod_8(void *what)
{
    switch ( RLCONSTBMOD_ARGR8(what)%8 )
    {
        case 0:  { RLCONSTBMOD_RES8(what) =     RLCONSTBMOD_ARGL8(what);                                                           break; }
        case 1:  { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 7 ) & 0x001 ) | ( ( RLCONSTBMOD_ARGL8(what) << 1 ) & 0x0fe ); break; }
        case 2:  { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 6 ) & 0x003 ) | ( ( RLCONSTBMOD_ARGL8(what) << 2 ) & 0x0fc ); break; }
        case 3:  { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 5 ) & 0x007 ) | ( ( RLCONSTBMOD_ARGL8(what) << 3 ) & 0x0f8 ); break; }
        case 4:  { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 4 ) & 0x00f ) | ( ( RLCONSTBMOD_ARGL8(what) << 4 ) & 0x0f0 ); break; }
        case 5:  { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 3 ) & 0x01f ) | ( ( RLCONSTBMOD_ARGL8(what) << 5 ) & 0x0e0 ); break; }
        case 6:  { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 2 ) & 0x03f ) | ( ( RLCONSTBMOD_ARGL8(what) << 6 ) & 0x0c0 ); break; }
        default: { RLCONSTBMOD_RES8(what) = ( ( RLCONSTBMOD_ARGL8(what) >> 1 ) & 0x07f ) | ( ( RLCONSTBMOD_ARGL8(what) << 7 ) & 0x080 ); break; }
    }

    return;
}

void rlconstbmod_16(void *what)
{
    switch ( RLCONSTBMOD_ARGR16(what)%16 )
    {
        case  0: { RLCONSTBMOD_RES16(what) =     RLCONSTBMOD_ARGL16(what);                                                                  break; }
        case  1: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >> 15 ) & 0x00001 ) | ( ( RLCONSTBMOD_ARGL16(what) <<  1 ) & 0x0fffe ); break; }
        case  2: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >> 14 ) & 0x00003 ) | ( ( RLCONSTBMOD_ARGL16(what) <<  2 ) & 0x0fffc ); break; }
        case  3: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >> 13 ) & 0x00007 ) | ( ( RLCONSTBMOD_ARGL16(what) <<  3 ) & 0x0fff8 ); break; }
        case  4: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >> 12 ) & 0x0000f ) | ( ( RLCONSTBMOD_ARGL16(what) <<  4 ) & 0x0fff0 ); break; }
        case  5: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >> 11 ) & 0x0001f ) | ( ( RLCONSTBMOD_ARGL16(what) <<  5 ) & 0x0ffe0 ); break; }
        case  6: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >> 10 ) & 0x0003f ) | ( ( RLCONSTBMOD_ARGL16(what) <<  6 ) & 0x0ffc0 ); break; }
        case  7: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  9 ) & 0x0007f ) | ( ( RLCONSTBMOD_ARGL16(what) <<  7 ) & 0x0ff80 ); break; }
        case  8: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  8 ) & 0x000ff ) | ( ( RLCONSTBMOD_ARGL16(what) <<  8 ) & 0x0ff00 ); break; }
        case  9: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  7 ) & 0x001ff ) | ( ( RLCONSTBMOD_ARGL16(what) <<  9 ) & 0x0fe00 ); break; }
        case 10: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  6 ) & 0x003ff ) | ( ( RLCONSTBMOD_ARGL16(what) << 10 ) & 0x0fc00 ); break; }
        case 11: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  5 ) & 0x007ff ) | ( ( RLCONSTBMOD_ARGL16(what) << 11 ) & 0x0f800 ); break; }
        case 12: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  4 ) & 0x00fff ) | ( ( RLCONSTBMOD_ARGL16(what) << 12 ) & 0x0f000 ); break; }
        case 13: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  3 ) & 0x01fff ) | ( ( RLCONSTBMOD_ARGL16(what) << 13 ) & 0x0e000 ); break; }
        case 14: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  2 ) & 0x03fff ) | ( ( RLCONSTBMOD_ARGL16(what) << 14 ) & 0x0c000 ); break; }
        default: { RLCONSTBMOD_RES16(what) = ( ( RLCONSTBMOD_ARGL16(what) >>  1 ) & 0x07fff ) | ( ( RLCONSTBMOD_ARGL16(what) << 15 ) & 0x08000 ); break; }
    }

    return;
}

void rlconstbmod_32(void *what)
{
    switch ( RLCONSTBMOD_ARGR32(what)%32 )
    {
        case  0: { RLCONSTBMOD_RES32(what) =     RLCONSTBMOD_ARGL32(what);                                                                          break; }
        case  1: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 31 ) & 0x000000001 ) | ( ( RLCONSTBMOD_ARGL32(what) <<  1 ) & 0x0fffffffe ); break; }
        case  2: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 30 ) & 0x000000003 ) | ( ( RLCONSTBMOD_ARGL32(what) <<  2 ) & 0x0fffffffc ); break; }
        case  3: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 29 ) & 0x000000007 ) | ( ( RLCONSTBMOD_ARGL32(what) <<  3 ) & 0x0fffffff8 ); break; }
        case  4: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 28 ) & 0x00000000f ) | ( ( RLCONSTBMOD_ARGL32(what) <<  4 ) & 0x0fffffff0 ); break; }
        case  5: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 27 ) & 0x00000001f ) | ( ( RLCONSTBMOD_ARGL32(what) <<  5 ) & 0x0ffffffe0 ); break; }
        case  6: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 26 ) & 0x00000003f ) | ( ( RLCONSTBMOD_ARGL32(what) <<  6 ) & 0x0ffffffc0 ); break; }
        case  7: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 25 ) & 0x00000007f ) | ( ( RLCONSTBMOD_ARGL32(what) <<  7 ) & 0x0ffffff80 ); break; }
        case  8: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 24 ) & 0x0000000ff ) | ( ( RLCONSTBMOD_ARGL32(what) <<  8 ) & 0x0ffffff00 ); break; }
        case  9: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 23 ) & 0x0000001ff ) | ( ( RLCONSTBMOD_ARGL32(what) <<  9 ) & 0x0fffffe00 ); break; }
        case 10: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 22 ) & 0x0000003ff ) | ( ( RLCONSTBMOD_ARGL32(what) << 10 ) & 0x0fffffc00 ); break; }
        case 11: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 21 ) & 0x0000007ff ) | ( ( RLCONSTBMOD_ARGL32(what) << 11 ) & 0x0fffff800 ); break; }
        case 12: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 20 ) & 0x000000fff ) | ( ( RLCONSTBMOD_ARGL32(what) << 12 ) & 0x0fffff000 ); break; }
        case 13: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 19 ) & 0x000001fff ) | ( ( RLCONSTBMOD_ARGL32(what) << 13 ) & 0x0ffffe000 ); break; }
        case 14: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 18 ) & 0x000003fff ) | ( ( RLCONSTBMOD_ARGL32(what) << 14 ) & 0x0ffffc000 ); break; }
        case 15: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 17 ) & 0x000007fff ) | ( ( RLCONSTBMOD_ARGL32(what) << 15 ) & 0x0ffff8000 ); break; }
        case 16: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 16 ) & 0x00000ffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 16 ) & 0x0ffff0000 ); break; }
        case 17: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 15 ) & 0x00001ffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 17 ) & 0x0fffe0000 ); break; }
        case 18: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 14 ) & 0x00003ffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 18 ) & 0x0fffc0000 ); break; }
        case 19: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 13 ) & 0x00007ffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 19 ) & 0x0fff80000 ); break; }
        case 20: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 12 ) & 0x0000fffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 20 ) & 0x0fff00000 ); break; }
        case 21: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 11 ) & 0x0001fffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 21 ) & 0x0ffe00000 ); break; }
        case 22: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >> 10 ) & 0x0003fffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 22 ) & 0x0ffc00000 ); break; }
        case 23: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  9 ) & 0x0007fffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 23 ) & 0x0ff800000 ); break; }
        case 24: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  8 ) & 0x000ffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 24 ) & 0x0ff000000 ); break; }
        case 25: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  7 ) & 0x001ffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 25 ) & 0x0fe000000 ); break; }
        case 26: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  6 ) & 0x003ffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 26 ) & 0x0fc000000 ); break; }
        case 27: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  5 ) & 0x007ffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 27 ) & 0x0f8000000 ); break; }
        case 28: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  4 ) & 0x00fffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 28 ) & 0x0f0000000 ); break; }
        case 29: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  3 ) & 0x01fffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 29 ) & 0x0e0000000 ); break; }
        case 30: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  2 ) & 0x03fffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 30 ) & 0x0c0000000 ); break; }
        default: { RLCONSTBMOD_RES32(what) = ( ( RLCONSTBMOD_ARGL32(what) >>  1 ) & 0x07fffffff ) | ( ( RLCONSTBMOD_ARGL32(what) << 31 ) & 0x080000000 ); break; }
    }

    return;
}







/**********************************************************************

ALU module 32: srconsta

**********************************************************************/

#define SRCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define SRCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define SRCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define SRCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define SRCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define SRCONSTAMOD_ARGR16(what)    DEREF_8VAR(what,1)
#define SRCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define SRCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define SRCONSTAMOD_ARGR32(what)    DEREF_8VAR(what,2)

void srconstamod_8(void *what);
void srconstamod_16(void *what);
void srconstamod_32(void *what);

module_data *srconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,3,0,0,0,0,2,2,2,3,0);

    return what;
}

int srconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = srconstamod_8;
    DEREF_INFN(what,1) = srconstamod_16;
    DEREF_INFN(what,2) = srconstamod_32;

    return 0;
}

void srconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void srconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void srconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void srconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *srconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void srconstamod_8(void *what)
{
    switch ( SRCONSTAMOD_ARGR8(what) )
    {
        case 0: { SRCONSTAMOD_RES8(what) =   SRCONSTAMOD_ARGL8(what);                break; }
        case 1: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 1 ) & 0x07f; break; }
        case 2: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 2 ) & 0x03f; break; }
        case 3: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 3 ) & 0x01f; break; }
        case 4: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 4 ) & 0x00f; break; }
        case 5: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 5 ) & 0x007; break; }
        case 6: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 6 ) & 0x003; break; }
        case 7: { SRCONSTAMOD_RES8(what) = ( SRCONSTAMOD_ARGL8(what) >> 7 ) & 0x001; break; }

        default:
        {
            SRCONSTAMOD_RES8(what) = 0;

            break;
        }
    }

    return;
}

void srconstamod_16(void *what)
{
    switch ( SRCONSTAMOD_ARGR16(what) )
    {
        case  0: { SRCONSTAMOD_RES16(what) =   SRCONSTAMOD_ARGL16(what);                   break; }
        case  1: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  1 ) & 0x07fff; break; }
        case  2: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  2 ) & 0x03fff; break; }
        case  3: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  3 ) & 0x01fff; break; }
        case  4: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  4 ) & 0x00fff; break; }
        case  5: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  5 ) & 0x007ff; break; }
        case  6: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  6 ) & 0x003ff; break; }
        case  7: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  7 ) & 0x001ff; break; }
        case  8: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  8 ) & 0x000ff; break; }
        case  9: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >>  9 ) & 0x0007f; break; }
        case 10: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >> 10 ) & 0x0003f; break; }
        case 11: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >> 11 ) & 0x0001f; break; }
        case 12: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >> 12 ) & 0x0000f; break; }
        case 13: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >> 13 ) & 0x00007; break; }
        case 14: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >> 14 ) & 0x00003; break; }
        case 15: { SRCONSTAMOD_RES16(what) = ( SRCONSTAMOD_ARGL16(what) >> 15 ) & 0x00001; break; }

        default:
        {
            SRCONSTAMOD_RES16(what) = 0;

            break;
        }
    }

    return;
}

void srconstamod_32(void *what)
{
    switch ( SRCONSTAMOD_ARGR32(what) )
    {
        case  0: { SRCONSTAMOD_RES32(what) =   SRCONSTAMOD_ARGL32(what);                       break; }
        case  1: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  1 ) & 0x07fffffff; break; }
        case  2: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  2 ) & 0x03fffffff; break; }
        case  3: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  3 ) & 0x01fffffff; break; }
        case  4: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  4 ) & 0x00fffffff; break; }
        case  5: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  5 ) & 0x007ffffff; break; }
        case  6: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  6 ) & 0x003ffffff; break; }
        case  7: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  7 ) & 0x001ffffff; break; }
        case  8: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  8 ) & 0x000ffffff; break; }
        case  9: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >>  9 ) & 0x0007fffff; break; }
        case 10: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 10 ) & 0x0003fffff; break; }
        case 11: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 11 ) & 0x0001fffff; break; }
        case 12: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 12 ) & 0x0000fffff; break; }
        case 13: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 13 ) & 0x00007ffff; break; }
        case 14: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 14 ) & 0x00003ffff; break; }
        case 15: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 15 ) & 0x00001ffff; break; }
        case 16: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 16 ) & 0x00000ffff; break; }
        case 17: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 17 ) & 0x000007fff; break; }
        case 18: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 18 ) & 0x000003fff; break; }
        case 19: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 19 ) & 0x000001fff; break; }
        case 20: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 20 ) & 0x000000fff; break; }
        case 21: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 21 ) & 0x0000007ff; break; }
        case 22: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 22 ) & 0x0000003ff; break; }
        case 23: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 23 ) & 0x0000001ff; break; }
        case 24: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 24 ) & 0x0000000ff; break; }
        case 25: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 25 ) & 0x00000007f; break; }
        case 26: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 26 ) & 0x00000003f; break; }
        case 27: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 27 ) & 0x00000001f; break; }
        case 28: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 28 ) & 0x00000000f; break; }
        case 29: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 29 ) & 0x000000007; break; }
        case 30: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 30 ) & 0x000000003; break; }
        case 31: { SRCONSTAMOD_RES32(what) = ( SRCONSTAMOD_ARGL32(what) >> 31 ) & 0x000000001; break; }

        default:
        {
            SRCONSTAMOD_RES32(what) = 0;

            break;
        }
    }

    return;
}






/**********************************************************************

ALU module 33: srconstb

**********************************************************************/

#define SRCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define SRCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define SRCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define SRCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define SRCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define SRCONSTBMOD_ARGR16(what)    DEREF_8BUS(what,2)
#define SRCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define SRCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define SRCONSTBMOD_ARGR32(what)    DEREF_8BUS(what,3)

void srconstbmod_8(void *what);
void srconstbmod_16(void *what);
void srconstbmod_32(void *what);

module_data *srconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,4,1,1,3,0);

    return what;
}

int srconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = srconstbmod_8;
    DEREF_INFN(what,1) = srconstbmod_16;
    DEREF_INFN(what,2) = srconstbmod_32;

    return 0;
}

void srconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void srconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void srconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void srconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *srconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void srconstbmod_8(void *what)
{
    switch ( SRCONSTBMOD_ARGR8(what) )
    {
        case 0: { SRCONSTBMOD_RES8(what) =   SRCONSTBMOD_ARGL8(what);                break; }
        case 1: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 1 ) & 0x07f; break; }
        case 2: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 2 ) & 0x03f; break; }
        case 3: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 3 ) & 0x01f; break; }
        case 4: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 4 ) & 0x00f; break; }
        case 5: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 5 ) & 0x007; break; }
        case 6: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 6 ) & 0x003; break; }
        case 7: { SRCONSTBMOD_RES8(what) = ( SRCONSTBMOD_ARGL8(what) >> 7 ) & 0x001; break; }

        default:
        {
            SRCONSTBMOD_RES8(what) = 0;

            break;
        }
    }

    return;
}

void srconstbmod_16(void *what)
{
    switch ( SRCONSTBMOD_ARGR16(what) )
    {
        case  0: { SRCONSTBMOD_RES16(what) =   SRCONSTBMOD_ARGL16(what);                   break; }
        case  1: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  1 ) & 0x07fff; break; }
        case  2: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  2 ) & 0x03fff; break; }
        case  3: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  3 ) & 0x01fff; break; }
        case  4: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  4 ) & 0x00fff; break; }
        case  5: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  5 ) & 0x007ff; break; }
        case  6: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  6 ) & 0x003ff; break; }
        case  7: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  7 ) & 0x001ff; break; }
        case  8: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  8 ) & 0x000ff; break; }
        case  9: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >>  9 ) & 0x0007f; break; }
        case 10: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >> 10 ) & 0x0003f; break; }
        case 11: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >> 11 ) & 0x0001f; break; }
        case 12: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >> 12 ) & 0x0000f; break; }
        case 13: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >> 13 ) & 0x00007; break; }
        case 14: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >> 14 ) & 0x00003; break; }
        case 15: { SRCONSTBMOD_RES16(what) = ( SRCONSTBMOD_ARGL16(what) >> 15 ) & 0x00001; break; }

        default:
        {
            SRCONSTBMOD_RES16(what) = 0;

            break;
        }
    }

    return;
}

void srconstbmod_32(void *what)
{
    switch ( SRCONSTBMOD_ARGR32(what) )
    {
        case  0: { SRCONSTBMOD_RES32(what) =   SRCONSTBMOD_ARGL32(what);                       break; }
        case  1: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  1 ) & 0x07fffffff; break; }
        case  2: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  2 ) & 0x03fffffff; break; }
        case  3: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  3 ) & 0x01fffffff; break; }
        case  4: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  4 ) & 0x00fffffff; break; }
        case  5: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  5 ) & 0x007ffffff; break; }
        case  6: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  6 ) & 0x003ffffff; break; }
        case  7: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  7 ) & 0x001ffffff; break; }
        case  8: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  8 ) & 0x000ffffff; break; }
        case  9: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >>  9 ) & 0x0007fffff; break; }
        case 10: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 10 ) & 0x0003fffff; break; }
        case 11: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 11 ) & 0x0001fffff; break; }
        case 12: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 12 ) & 0x0000fffff; break; }
        case 13: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 13 ) & 0x00007ffff; break; }
        case 14: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 14 ) & 0x00003ffff; break; }
        case 15: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 15 ) & 0x00001ffff; break; }
        case 16: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 16 ) & 0x00000ffff; break; }
        case 17: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 17 ) & 0x000007fff; break; }
        case 18: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 18 ) & 0x000003fff; break; }
        case 19: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 19 ) & 0x000001fff; break; }
        case 20: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 20 ) & 0x000000fff; break; }
        case 21: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 21 ) & 0x0000007ff; break; }
        case 22: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 22 ) & 0x0000003ff; break; }
        case 23: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 23 ) & 0x0000001ff; break; }
        case 24: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 24 ) & 0x0000000ff; break; }
        case 25: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 25 ) & 0x00000007f; break; }
        case 26: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 26 ) & 0x00000003f; break; }
        case 27: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 27 ) & 0x00000001f; break; }
        case 28: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 28 ) & 0x00000000f; break; }
        case 29: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 29 ) & 0x000000007; break; }
        case 30: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 30 ) & 0x000000003; break; }
        case 31: { SRCONSTBMOD_RES32(what) = ( SRCONSTBMOD_ARGL32(what) >> 31 ) & 0x000000001; break; }

        default:
        {
            SRCONSTBMOD_RES32(what) = 0;

            break;
        }
    }

    return;
}






/**********************************************************************

ALU module 34: slconsta

**********************************************************************/

#define SLCONSTAMOD_RES8(what)      DEREF_8BUS(what,0)
#define SLCONSTAMOD_ARGL8(what)     DEREF_8BUS(what,1)
#define SLCONSTAMOD_ARGR8(what)     DEREF_8VAR(what,0)
#define SLCONSTAMOD_RES16(what)     DEREF_16BUS(what,0)
#define SLCONSTAMOD_ARGL16(what)    DEREF_16BUS(what,1)
#define SLCONSTAMOD_ARGR16(what)    DEREF_8VAR(what,1)
#define SLCONSTAMOD_RES32(what)     DEREF_32BUS(what,0)
#define SLCONSTAMOD_ARGL32(what)    DEREF_32BUS(what,1)
#define SLCONSTAMOD_ARGR32(what)    DEREF_8VAR(what,2)

void slconstamod_8(void *what);
void slconstamod_16(void *what);
void slconstamod_32(void *what);

module_data *slconstamod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,3,0,0,0,0,2,2,2,3,0);

    return what;
}

int slconstamod_init(module_data *what)
{
    DEREF_INFN(what,0) = slconstamod_8;
    DEREF_INFN(what,1) = slconstamod_16;
    DEREF_INFN(what,2) = slconstamod_32;

    return 0;
}

void slconstamod_go(module_data *what)
{
    return;

    what = NULL;
}

void slconstamod_stop(module_data *what)
{
    return;

    what = NULL;
}

void slconstamod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void slconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *slconstamod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void slconstamod_8(void *what)
{
    SLMOD_RES8(what) = SLMOD_ARGL8(what) << SLMOD_ARGR8(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}

void slconstamod_16(void *what)
{
    SLMOD_RES16(what) = SLMOD_ARGL16(what) << SLMOD_ARGR16(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}

void slconstamod_32(void *what)
{
    SLMOD_RES32(what) = SLMOD_ARGL32(what) << SLMOD_ARGR32(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}









/**********************************************************************

ALU module 35: slconstb

**********************************************************************/

#define SLCONSTBMOD_RES8(what)      DEREF_8BUS(what,0)
#define SLCONSTBMOD_ARGL8(what)     DEREF_8VAR(what,0)
#define SLCONSTBMOD_ARGR8(what)     DEREF_8BUS(what,1)
#define SLCONSTBMOD_RES16(what)     DEREF_16BUS(what,0)
#define SLCONSTBMOD_ARGL16(what)    DEREF_16VAR(what,0)
#define SLCONSTBMOD_ARGR16(what)    DEREF_8BUS(what,2)
#define SLCONSTBMOD_RES32(what)     DEREF_32BUS(what,0)
#define SLCONSTBMOD_ARGL32(what)    DEREF_32VAR(what,0)
#define SLCONSTBMOD_ARGR32(what)    DEREF_8BUS(what,3)

void slconstbmod_8(void *what);
void slconstbmod_16(void *what);
void slconstbmod_32(void *what);

module_data *slconstbmod_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,0,1,1,1,0,0,4,1,1,3,0);

    return what;
}

int slconstbmod_init(module_data *what)
{
    DEREF_INFN(what,0) = slconstbmod_8;
    DEREF_INFN(what,1) = slconstbmod_16;
    DEREF_INFN(what,2) = slconstbmod_32;

    return 0;
}

void slconstbmod_go(module_data *what)
{
    return;

    what = NULL;
}

void slconstbmod_stop(module_data *what)
{
    return;

    what = NULL;
}

void slconstbmod_remove(module_data *what)
{
    if ( what != NULL )
    {
        free_module_data(what);
    }

    return;
}

void slconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

char *slconstbmod_getinf(module_data *what)
{
    char *dest;

    if ( ( dest = (char *) DEBMALLOC(sizeof(char)) ) != NULL )
    {
        dest[0] = '\0';
    }

    return dest;

    what = NULL;
}

void slconstbmod_8(void *what)
{
    SLMOD_RES8(what) = SLMOD_ARGL8(what) << SLMOD_ARGR8(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}

void slconstbmod_16(void *what)
{
    SLMOD_RES16(what) = SLMOD_ARGL16(what) << SLMOD_ARGR16(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}

void slconstbmod_32(void *what)
{
    SLMOD_RES32(what) = SLMOD_ARGL32(what) << SLMOD_ARGR32(what);

    /* no need to mask, operation is guaranteed by c standard */

    return;
}


