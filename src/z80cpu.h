
#include "u_dtype.h"
#include "modules.h"

#ifndef _z80cpu_h
#define _z80cpu_h

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

                         Z80 CPU Emulation
                         =================

The following code emulates the functionality of the Zilog Z80 CPU.



Module details
==============

8  bit variables: none
16 bit variables: none
32 bit variables: none

8  bit buses: busa0  wait bus (used to insert wait states)
              busa1  refresh bus (refresh address put here)
              busa2  data bus
              busa3  table bus: table # start (upper 8 bits of mem).
              busa4  table bus: table # finish (upper 8 bits of mem).
              busa5  table bus: memory access address
16 bit buses: busb0 address bus
              busb1 table bus: write waits inserted for this memory
              busb2 table bus: read waits inserted for this memory
32 bit buses: busc0 reti counter bus (incremented when reti opcode executed)

incoming functions: infn0  send reset signal.
                    infn1  send bus request signal.
                    infn2  send NMI (non-maskable interupt) signal.
                    infn3  set INT pin (maskable interupt).
                    infn4  reset INT pin (maskable interupt).
                    infn5  insert wait states (# on busa0).
                       === the remainder of the functions control    ===
                       === memory access.  write, read and opread    ===
                       === are all controlled independently. busa3,  ===
                       === busa4 and busa5 select the range of mem   ===
                       === blocks (method used is selected by the    ===
                       === upper 8 bits of the address bus,          ===
                       === giving 256 independent memory blocks).    ===
                       === busa5 should be set to point to the mem   ===
                       === to be used for the direct access method.  ===
                       === busb1  and busb2 set the number of wait   ===
                       === states added for the relevant operation   ===
                       === (if used).                                ===
                    infn6  wr: redirect memory writes to dev/null.
                    infn7  wr: write directly to memory using busa5.
                    infn8  wr: write to memory using outfn8 (default).
                    infn9  rd: memory reads will be zero.
                    infn10 rd: read memory directly using busa5.
                    infn11 rd: read memory using outfn9 (default).
                    infn12 op: opreads will be zero.
                    infn13 op: opread memory directly using busa5.
                    infn14 op: opread memory using outfn6 (default).
                    infn15 wr: infn6, but no waits inserted.
                    infn16 wr: infn7, but no waits inserted.
                    infn17 wr: infn8, but no waits inserted.
                    infn18 rd: infn9, but no waits inserted.
                    infn19 rd: infn10, but no waits inserted.
                    infn20 rd: infn11, but no waits inserted.
                    infn21 op: infn12, but no waits inserted.
                    infn22 op: infn13, but no waits inserted.
                    infn23 op: infn14, but no waits inserted.

outgoing functions: outfn0  indicates an emulation error.
                    outfn1  acknowledges reset.
                    outfn2  acknowledges bus request.  The number of waits to
                            be inserted during this request should be placed
                            on busa0 before returning.
                    outfn3  acknowledges halt.
                    outfn4  acknowledges NMI.  Both the refresh (busa1) and
                            address (busb0) will be set appropriately.
                    outfn5  acknowledges INT.  The address bus (busb0) will
                            be set appropriately.  Any data to be read should
                            be placed on busa2 before returning.  The data
                            bus will be reset before calling this function.
                    outfn6  opfetch call.  The address and refresh buses are
                            filled (busb0 and busa0), and the function called
                            should set the data bus (busa2).  The data bus
                            will be reset before calling this function.
                    outfn7  memory write call.  The address and data buses
                            are filled (busb0 and busa2).
                    outfn8  memory read call.  The address bus is filled
                            (busb0), and the function called should set the
                            data bus (busa2).  The data bus will be reset
                            before calling this function.
                    outfn9  io write call.  The address and data buses
                            are filled (busb0 and busa2).
                    outfn10 io read call.  The address bus is filled
                            (busb0), and the function called should set the
                            data bus (busa2).  The data bus will be reset
                            before calling this function.

Module is clocked.

*/


module_data *z80cpu_alloc(const char *module_name);
int          z80cpu_init(module_data *what);
void         z80cpu_go(module_data *what);
void         z80cpu_stop(module_data *what);
void         z80cpu_remove(module_data *what);
void         z80cpu_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *z80cpu_getinf(module_data *what);

#endif
