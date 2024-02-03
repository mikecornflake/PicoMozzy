
#include "z80cpu.h"
#include "u_dtype.h"
#include "debmaloc.h"
#include "modules.h"

/*

       \\\              ///
      /~~~~~~~~~\      ///
      \______   /     ///
         \\\/  ####  /####
          \/  #    # #//  #
          /  /#    #/#/   #   VZ80 - Virtual Z80 for PC
         /  /\ #### /#    #   32-bit z80 emulator
        /  /\\#    #/#    #   Coding: Alistair Shilton
       /  /  \#\  /#/#    #           apsh@ee.unimelb.edu.au
      /   ~~~~~####/  ####
      \_________////
               \\//
                \/

*/


/***********************************************************************/
/***********************************************************************/
/***********************************************************************/


/*

                Stuff in z80cpu.asm
                ===================


Introduction
============

The z80 is started by calling the function z80_enter().  This passes control
to the z80 emulator, which will then call call back when external data/stuff
is required.  The following functions must be provided to the emulator:


z80_sig_error()  - called by the emulation if an error occurs.  After an
                   error, the cpu will reset itself.
z80_ack_reset()  - acknowledge receipt of reset.
z80_ack_busrq()  - acknowledge receipt of bus request.  The caller can now
                   do operation and put wait states on z80_wait_bus (clock
                   cycles).
z80_ack_halt()   - acknowledge excecution of HALT opcode.
z80_ack_NMI()    - acknowledge NMI.  The contents of the PC are put on the
                   z80_addr_bus, and R on the z80_rfsh_bus.
z80_ack_INT()    - acknowledge INT.  The contents of the PC are put on the
                   z80_addr_bus, and the responce data can be put on the
                   z80_data_bus.  Wait states can also be inserted using the
                   z80_wait_bus.
z80_opfetch()    - call to fetch opcode.  The contents of the PC are put on
                   the z80_addr_bus, and R on z80_rfsh_bus.  The responce
                   data must be put on the z80_data_bus and wait states can
                   be inserted using the z80_wait_bus.
z80_sync_clock() - call to delay to align clock cycles with elapsed time.
                   The number of accumulated clock cycles is put on the
                   z80_clk_bus.  The function may only wait for some, and
                   put the difference on the z80_clk_bus before returning,
                   or otherwise put 0 on the z80_clock_bus otherwise.
z80_wr_mem()     - call to write to memory.  Data on z80_data_bus, address
                   on z80_addr_bus.  Wait states may be inserted in the
                   usual manner.
z80_rd_mem()     - call to read from memory.  Addr on z80_addr_bus, data
                   must be loaded onto z80_data_bus.  Wait states may be
                   inserted in the usual manner.
z80_wr_io()      - write value to memory.  Same as z80_wr_mem(), but io.
z80_rd_io()      - read value from memory.  Same as z80_rd_mem(), but io.


The signal functions are:


z80_set_reset() - reset the CPU.  Once the current operation is completed,
                  the emulator will call z80_ack_reset and the following
                  values are written to the registers:

                  A,F,B,C,D,E,H,L,Ax,Fx,Bx,Cx,Dx,Ex,Hx,Lx - 0x0ff
                  IX,IY,SP                                - 0x0ffff
                  PC                                      - 0x00000
                  I,R                                     - 0x000
                  st1_,st2_                               - 0x000
                  doff                                    - 0x000

z80_set_busrq() - initiate bus request sequence.  Once the current operation
                  is complete, the emulator will call z80_ack_busrq.
z80_set_NMI()   - initiate NMI sequence.  Emulator will complete the current
                  operation and call z80_ack_NMI.
z80_set_INT()   - set INT pin.  This pin will remain set until either the
                  interupt is processed (resulting in a call from the
                  emulator to z80_ack_INT) or z80_res_INT is called to
                  remove the interupting signal (recall the the INT pin
                  on z80's is level sensitive, and the z80 will ignore
                  it unless interupts are enabled, and even then a long
                  sequence of DDFD type stuff can hold back processing of
                  the instruction indefinitely).
z80_res_INT()   - reset INT pin.
z80_set_wait()  - this function can be called (with the number of wait
                  states being inserted placed on the z80_wait_bus) at
                  any time to insert wait states in the operation.  It is
                  equivalent to the WAIT pin on the z80.


Emulator Buses
==============

All data transfer to and from the emulator is done using the following
buses:


z80_wait_bus - wait state bus
z80_rfsh_bus - refresh byte bus
z80_data_bus - data bus
z80_addr_bus - address bus
z80_clk_bus  - clock state bus


The Emulator State Bytes
========================

The emulator state bytes are as follows:

st1_: 7 - mode bit 2
      6 - mode bit 1
      5 - mode bit 0
      4 - IM1
      3 - IM0
      2 - IFF2
      1 - IFF1
      0 - DI - set if last opcode was EI,DI or a prefix (CB,DD,FD,ED).
               If this bit is set then both NMI and INT signals will be
               ignored.

The mode control bits indicate which part of an opcode is being processed.
Specifically, they indicate whether the opcode is standard, prefixed or
double prefixed, and what prefix (if any) it is.  The mode is:

Mode (210): 000 = normal (no prefix)
            001 = DD prefix
            010 = FD prefix
            100 = CB prefix
            111 = ED prefix
            101 = DDCB double prefix
            110 = FDCB double prefix

st2_: 7 - power  - unused (0).
      6 - reset  - set by z80_set_reset, causes reset & z80_ack_reset call.
      5 - busrq  - set by z80_set_busrq, causes z80_ack_busrq call.
      4 - NMI    - set by z80_set_NMI, may cause z80_ack_NMI call.
      3 - INT    - set by z80_set_INT, may cause z80_ack_INT call.
      2 - speed  - unused (0).
      1 - halted - set by HALT opcode, reset by interupts.
      0 - INTOP  - set to indicate presence of an interupt, which can
                   modify read operations.  eg.  a LB A,n op in the correct
                   interupt mode must not increment PC, even though this
                   read is indistinguishable from a standard opcode data
                   read.

Note that bits 7,6,5,4,3,2 are set by signal functions.  Bits 7,6,5,4,3
are transient (will be reset sometime near when the relevant operation is
performed - but AFTER the reset operation, which is important).


Memory operation control data
=============================

Memory is divided into 256 pages (ie. the upper 8 bytes of the address
bus select the table).  For each page, read, write and opread modes may
be set using the following functions:

void z80_set_mem_write_none()
void z80_set_mem_write_direct()
void z80_set_mem_write_indirect()

void z80_set_mem_read_none()
void z80_set_mem_read_direct()
void z80_set_mem_read_indirect()

void z80_set_mem_opread_none()
void z80_set_mem_opread_direct()
void z80_set_mem_opread_indirect()

void z80_set_mem_write_none_naw()
void z80_set_mem_write_direct_naw()
void z80_set_mem_write_indirect_naw()

void z80_set_mem_read_none_naw()
void z80_set_mem_read_direct_naw()
void z80_set_mem_read_indirect_naw()

void z80_set_mem_opread_none_naw()
void z80_set_mem_opread_direct_naw()
void z80_set_mem_opread_indirect_naw()

The methods are: none     - write nothing, read 0
                 direct   - read/write memory directly, using the page of 256
                            packed bytes for the relevant page.  This is set
                            by putting the address of the page on the
                            z80_tab_addr_bus when calling the function to set
                            direct memory access mode
                 indirect - call the relevant z80 function (provided to the
                            z80 emulator).
                 naw      - "no auto wait".  Does not automatically insert
                            wait states (but waits put on the wait bus during
                            indirect will be inserted).

When any of these functions are called, the following buses are used:

UINT_8  z80_tab_num_bus     - number of page which is to be modified.
UINT_8 *z80_tab_addr_bus    - page address to use for direct access.
UINT_16 z80_tab_wr_wait_bus - number of wait states for mem write this page.
UINT_16 z80_tab_rd_wait_bus - number of wait states for mem/op rd this page.

NB. Memory reads from PC when dealing with IM0 will all call external
    read function, regardless of setting here.  Also, wait states here
    will be ignored in this case.



External architecture support
=============================

The PIO (and possibly other devices) need to be able to detect the
processing of the RETI instruction to operate correctly. The z80_reti_counter
counter allows this.  It is incremented whenever the RETI instruction is
encountered, allowing the z80 pio to detect the op (by detecting the change
in the counter).



****************************************************************************/



/* Emulator functions */

void z80_init(void *z80block);
void z80_cycle(void *z80block);

void z80_set_reset(void *z80block);
void z80_set_busrq(void *z80block);
void z80_set_NMI(void *z80block);
void z80_set_INT(void *z80block);
void z80_res_INT(void *z80block);
void z80_set_wait(void *z80block);

void z80_set_mem_write_none(void *z80block);
void z80_set_mem_write_direct(void *z80block);
void z80_set_mem_write_indirect(void *z80block);

void z80_set_mem_read_none(void *z80block);
void z80_set_mem_read_direct(void *z80block);
void z80_set_mem_read_indirect(void *z80block);

void z80_set_mem_opread_none(void *z80block);
void z80_set_mem_opread_direct(void *z80block);
void z80_set_mem_opread_indirect(void *z80block);

void z80_set_mem_write_none_naw(void *z80block);
void z80_set_mem_write_direct_naw(void *z80block);
void z80_set_mem_write_indirect_naw(void *z80block);

void z80_set_mem_read_none_naw(void *z80block);
void z80_set_mem_read_direct_naw(void *z80block);
void z80_set_mem_read_indirect_naw(void *z80block);

void z80_set_mem_opread_none_naw(void *z80block);
void z80_set_mem_opread_direct_naw(void *z80block);
void z80_set_mem_opread_indirect_naw(void *z80block);



/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

void z80cpu_set_reset(void *what);
void z80cpu_set_busrq(void *what);
void z80cpu_set_NMI(void *what);
void z80cpu_set_INT(void *what);
void z80cpu_res_INT(void *what);
void z80cpu_set_wait(void *what);

void z80cpu_set_mem_write_none(void *what);
void z80cpu_set_mem_write_direct(void *what);
void z80cpu_set_mem_write_indirect(void *what);

void z80cpu_set_mem_read_none(void *what);
void z80cpu_set_mem_read_direct(void *what);
void z80cpu_set_mem_read_indirect(void *what);

void z80cpu_set_mem_opread_none(void *what);
void z80cpu_set_mem_opread_direct(void *what);
void z80cpu_set_mem_opread_indirect(void *what);

void z80cpu_set_mem_write_none_naw(void *what);
void z80cpu_set_mem_write_direct_naw(void *what);
void z80cpu_set_mem_write_indirect_naw(void *what);

void z80cpu_set_mem_read_none_naw(void *what);
void z80cpu_set_mem_read_direct_naw(void *what);
void z80cpu_set_mem_read_indirect_naw(void *what);

void z80cpu_set_mem_opread_none_naw(void *what);
void z80cpu_set_mem_opread_direct_naw(void *what);
void z80cpu_set_mem_opread_indirect_naw(void *what);




void z80_sig_error(void *backref);
void z80_ack_reset(void *backref);
void z80_ack_busrq(void *backref);
void z80_ack_halt(void *backref);
void z80_ack_NMI(void *backref);
void z80_ack_INT(void *backref);
void z80_opfetch(void *backref);
void z80_wr_mem(void *backref);
void z80_rd_mem(void *backref);
void z80_wr_io(void *backref);
void z80_rd_io(void *backref);





#define Z80CPU_WAIT_BUS(what)   DEREF_8BUS(what,0)
#define Z80CPU_RFSH_BUS(what)   DEREF_8BUS(what,1)
#define Z80CPU_DATA_BUS(what)   DEREF_8BUS(what,2)
#define Z80CPU_TNUM_BUS(what)   DEREF_8BUS(what,3)
#define Z80CPU_TEND_BUS(what)   DEREF_8BUS(what,4)
#define Z80CPU_TDAC_BUS(what)   DEREF_8MEM(what,5)
#define Z80CPU_ADDR_BUS(what)   DEREF_16BUS(what,0)
#define Z80CPU_TWRW_BUS(what)   DEREF_16BUS(what,1)
#define Z80CPU_TRDW_BUS(what)   DEREF_16BUS(what,2)
#define Z80CPU_RETI_BUS(what)   DEREF_32BUS(what,0)


#define Z80CPU_TNUM_LOCAL(what) (*((UINT_8  *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0000])))
#define Z80CPU_TDAC_LOCAL(what) (*((UINT_8 **) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0001])))
#define Z80CPU_TWRW_LOCAL(what) (*((UINT_16 *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0002])))
#define Z80CPU_TRDW_LOCAL(what) (*((UINT_16 *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0003])))
#define Z80CPU_WAIT_LOCAL(what) (*((UINT_8  *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0004])))
#define Z80CPU_RFSH_LOCAL(what) (*((UINT_8  *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0005])))
#define Z80CPU_DATA_LOCAL(what) (*((UINT_8  *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0006])))
#define Z80CPU_ADDR_LOCAL(what) (*((UINT_16 *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0007])))
#define Z80CPU_CLK__LOCAL(what) (*((UINT_16 *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0009])))
#define Z80CPU_RETI_LOCAL(what) (*((UINT_32 *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x000A])))


#define Z80CPU_CLK_COUNT(what)  (*((UINT_32 *) &(((UINT_32 *) DEREF_INTERNAL(what))[0x0544])))


#define Z80CPU_SIGERR_OUT(what) OUTFNCALL(what,0)
#define Z80CPU_RESACK_OUT(what) OUTFNCALL(what,1)
#define Z80CPU_BRQACK_OUT(what) OUTFNCALL(what,2)
#define Z80CPU_HLTACK_OUT(what) OUTFNCALL(what,3)
#define Z80CPU_NMIACK_OUT(what) OUTFNCALL(what,4)
#define Z80CPU_INTACK_OUT(what) OUTFNCALL(what,5)
#define Z80CPU_OPFTCH_OUT(what) OUTFNCALL(what,6)
#define Z80CPU_MEM_WR_OUT(what) OUTFNCALL(what,7)
#define Z80CPU_MEM_RD_OUT(what) OUTFNCALL(what,8)
#define Z80CPU_IO__WR_OUT(what) OUTFNCALL(what,9)
#define Z80CPU_IO__RD_OUT(what) OUTFNCALL(what,10)


/* here be dragons */
#define Z80CPU_GET_MODULE(what) ((module_data *) (((UINT_32 **) (what))[0x0540]))
#define Z80CPU_SCRATCHPAD(what) DEREF_INTERNAL(what)




module_data *z80cpu_alloc(const char *module_name)
{
    module_data *result;

    result = gen_module_data(module_name,1,0,0,0,0,0,6,3,1,24,11);

    return result;
}

int z80cpu_init(module_data *what)
{
    int result = 1;

    DEREF_INFN(what,0)  = z80cpu_set_reset;
    DEREF_INFN(what,1)  = z80cpu_set_busrq;
    DEREF_INFN(what,2)  = z80cpu_set_NMI;
    DEREF_INFN(what,3)  = z80cpu_set_INT;
    DEREF_INFN(what,4)  = z80cpu_res_INT;
    DEREF_INFN(what,5)  = z80cpu_set_wait;
    DEREF_INFN(what,6)  = z80cpu_set_mem_write_none;
    DEREF_INFN(what,7)  = z80cpu_set_mem_write_direct;
    DEREF_INFN(what,8)  = z80cpu_set_mem_write_indirect;
    DEREF_INFN(what,9)  = z80cpu_set_mem_read_none;
    DEREF_INFN(what,10) = z80cpu_set_mem_read_direct;
    DEREF_INFN(what,11) = z80cpu_set_mem_read_indirect;
    DEREF_INFN(what,12) = z80cpu_set_mem_opread_none;
    DEREF_INFN(what,13) = z80cpu_set_mem_opread_direct;
    DEREF_INFN(what,14) = z80cpu_set_mem_opread_indirect;
    DEREF_INFN(what,15) = z80cpu_set_mem_write_none_naw;
    DEREF_INFN(what,16) = z80cpu_set_mem_write_direct_naw;
    DEREF_INFN(what,17) = z80cpu_set_mem_write_indirect_naw;
    DEREF_INFN(what,18) = z80cpu_set_mem_read_none_naw;
    DEREF_INFN(what,19) = z80cpu_set_mem_read_direct_naw;
    DEREF_INFN(what,20) = z80cpu_set_mem_read_indirect_naw;
    DEREF_INFN(what,21) = z80cpu_set_mem_opread_none_naw;
    DEREF_INFN(what,22) = z80cpu_set_mem_opread_direct_naw;
    DEREF_INFN(what,23) = z80cpu_set_mem_opread_indirect_naw;

    if ( ( Z80CPU_SCRATCHPAD(what) = DEBMALLOC(0x01600) ) != NULL )
    {
        result = 0;
    }

    ((void **) Z80CPU_SCRATCHPAD(what))[0x0540] = (void *) what;

    return result;
}

void z80cpu_go(module_data *what)
{
    z80_init(Z80CPU_SCRATCHPAD(what));

    Z80CPU_CLK_COUNT(what) = 0x00fffffff;

    return;
}

void z80cpu_stop(module_data *what)
{
    return;

    what = NULL;
}

void z80cpu_remove(module_data *what)
{
    if ( what != NULL )
    {
        if ( Z80CPU_SCRATCHPAD(what) != NULL )
        {
            DEBFREE(Z80CPU_SCRATCHPAD(what));
        }

        free_module_data(what);
    }

    return;
}

void z80cpu_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    Z80CPU_CLK_COUNT(what)  += num_cycles;
    Z80CPU_CLK__LOCAL(what)  = 0;

    while ( Z80CPU_CLK_COUNT(what) > 0x00fffffff )
    {
        z80_cycle(Z80CPU_SCRATCHPAD(what));

        Z80CPU_CLK_COUNT(what)  -= Z80CPU_CLK__LOCAL(what);
        Z80CPU_CLK__LOCAL(what)  = 0;
    }

    return;

    clock_div = 0;
    lsync_point = 0;
}

char *z80cpu_getinf(module_data *what)
{
    char *dest;

    dest = DEBMALLOC(10*sizeof(UINT_8));

    sprintf(dest,"\n");

    return dest;

    what = NULL;
}


void z80cpu_set_reset(void *what)
{
    z80_set_reset(Z80CPU_SCRATCHPAD(what));

    return;
}

void z80cpu_set_busrq(void *what)
{
    z80_set_busrq(Z80CPU_SCRATCHPAD(what));

    return;
}

void z80cpu_set_NMI(void *what)
{
    z80_set_NMI(Z80CPU_SCRATCHPAD(what));

    return;
}

void z80cpu_set_INT(void *what)
{
    z80_set_INT(Z80CPU_SCRATCHPAD(what));

    return;
}

void z80cpu_res_INT(void *what)
{
    z80_res_INT(Z80CPU_SCRATCHPAD(what));

    return;
}

void z80cpu_set_wait(void *what)
{
    Z80CPU_WAIT_LOCAL(what) = Z80CPU_WAIT_BUS(what);

    z80_set_wait(Z80CPU_SCRATCHPAD(what));

    return;
}

void z80cpu_set_mem_write_none(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_write_none(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_write_direct(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_write_direct(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_write_indirect(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_write_indirect(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_read_none(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_read_none(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_read_direct(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_read_direct(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_read_indirect(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_read_indirect(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_opread_none(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_opread_none(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_opread_direct(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_opread_direct(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_opread_indirect(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);
    Z80CPU_TWRW_LOCAL(what) = Z80CPU_TWRW_BUS(what);
    Z80CPU_TRDW_LOCAL(what) = Z80CPU_TRDW_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_opread_indirect(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_write_none_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_write_none_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_write_direct_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_write_direct_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_write_indirect_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_write_indirect_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_read_none_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_read_none_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_read_direct_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_read_direct_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_read_indirect_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_read_indirect_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_opread_none_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_opread_none_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_opread_direct_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_opread_direct_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80cpu_set_mem_opread_indirect_naw(void *what)
{
    UINT_32 i;

    Z80CPU_TDAC_LOCAL(what) = Z80CPU_TDAC_BUS(what);

    for ( i = Z80CPU_TNUM_BUS(what) ; i <= Z80CPU_TEND_BUS(what) ; i++ )
    {
        Z80CPU_TNUM_LOCAL(what) = i;

        z80_set_mem_opread_indirect_naw(Z80CPU_SCRATCHPAD(what));

        Z80CPU_TDAC_LOCAL(what) += 0x100;
    }

    return;
}

void z80_sig_error(void *backref)
{
    Z80CPU_SIGERR_OUT(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_ack_reset(void *backref)
{
    Z80CPU_RESACK_OUT(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_ack_busrq(void *backref)
{
    Z80CPU_BRQACK_OUT(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_ack_halt(void *backref)
{
    Z80CPU_HLTACK_OUT(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_ack_NMI(void *backref)
{
    Z80CPU_RFSH_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_RFSH_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));

    Z80CPU_NMIACK_OUT(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_ack_INT(void *backref)
{
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref)) = 0;

    Z80CPU_INTACK_OUT(Z80CPU_GET_MODULE(backref));

    Z80CPU_WAIT_LOCAL(Z80CPU_GET_MODULE(backref)) = 0;
    Z80CPU_DATA_LOCAL(Z80CPU_GET_MODULE(backref)) = Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_opfetch(void *backref)
{
    Z80CPU_RFSH_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_RFSH_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref)) = 0;

    Z80CPU_OPFTCH_OUT(Z80CPU_GET_MODULE(backref));

    Z80CPU_WAIT_LOCAL(Z80CPU_GET_MODULE(backref)) = 0;
    Z80CPU_DATA_LOCAL(Z80CPU_GET_MODULE(backref)) = Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_wr_mem(void *backref)
{
    Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_DATA_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));

    Z80CPU_MEM_WR_OUT(Z80CPU_GET_MODULE(backref));

    Z80CPU_WAIT_LOCAL(Z80CPU_GET_MODULE(backref)) = 0;

    return;
}

void z80_rd_mem(void *backref)
{
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref)) = 0;

    Z80CPU_MEM_RD_OUT(Z80CPU_GET_MODULE(backref));

    Z80CPU_WAIT_LOCAL(Z80CPU_GET_MODULE(backref)) = 0;
    Z80CPU_DATA_LOCAL(Z80CPU_GET_MODULE(backref)) = Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref));

    return;
}

void z80_wr_io(void *backref)
{
    Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_DATA_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));

    Z80CPU_IO__WR_OUT(Z80CPU_GET_MODULE(backref));

    Z80CPU_WAIT_LOCAL(Z80CPU_GET_MODULE(backref)) = 0;

    return;
}

void z80_rd_io(void *backref)
{
    Z80CPU_ADDR_BUS(Z80CPU_GET_MODULE(backref)) = Z80CPU_ADDR_LOCAL(Z80CPU_GET_MODULE(backref));
    Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref)) = 0;

    Z80CPU_IO__RD_OUT(Z80CPU_GET_MODULE(backref));

    Z80CPU_WAIT_LOCAL(Z80CPU_GET_MODULE(backref)) = 0;
    Z80CPU_DATA_LOCAL(Z80CPU_GET_MODULE(backref)) = Z80CPU_DATA_BUS(Z80CPU_GET_MODULE(backref));

    return;
}



