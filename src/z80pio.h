
#include "u_dtype.h"
#include "modules.h"

#ifndef _z80pio_h
#define _z80pio_h

/*
   +-------+
  -|       |-            
  -|       |-            
  -|       |-            
  -|       |-      \\            __________
  -|       | ====== \\          /  / \    /\
  -|                 \\        /   \ /   /--`
  -|                  \\      / >{|||}  /
  -| z80pio Emulator   \\    |   / \   |
  -|                    _  +-------------+
  -| Alistair Shilton  |_|=|        .... |
  -| apsh@ecr.mu.oz.au     |_____________|
  -| http://           //   \___________/
  -|                  //
  -|                 // 
  -|       | ====== //   
  -|       |-      //    
  -|       |-            
  -|       |-            
  -|       |-            
  -|       |-
   +-------+

                         Z80 PIO Emulation
                         =================

The following code emulates the functionality of the Zilog Z80 PIO.



Module details
==============

8  bit variables: none
16 bit variables: none
32 bit variables: none

8  bit buses: bus0  signal bus - ieo.                           outfn2
              bus1  signal bus - iei.                           infn11
              bus2  data bus - port A data write.               infn1
              bus3  data bus - port A control write.            infn2
              bus4  data bus - port B data write.               infn3
              bus5  data bus - port B control write.            infn4
              bus6  data bus - port A data read.                infn5
              bus7  data bus - port A control read.             infn6
              bus8  data bus - port B data read.                infn7
              bus9  data bus - port B control read.             infn8
              bus10 vector bus - interupt acknowledge.          infn12
              bus11 ready bus - port A.                         infn9, outfn1
              bus12 strobe bus - port A.                        infn9, outfn1
              bus13 outdata bus - port A.                       infn9, outfn1
              bus14 ready bus - port B.                         infn10,outfn2
              bus15 strobe bus - port B.                        infn10,outfn2
              bus16 outdata bus - port B.                       infn10,outfn2
16 bit buses: none
32 bit buses: none

incoming functions: infn0  reset the z80pio state.
                    infn1  write bus2 to the z80pio A data port.
                    infn2  write bus3 to the z80pio A control port.
                    infn3  write bus4 to the z80pio B data port.
                    infn4  write bus5 to the z80pio B control port.
                    infn5  read bus6 from the z80pio A data port.
                    infn6  read bus7 from the z80pio A control port.
                    infn7  read bus8 from the z80pio B data port.
                    infn8  read bus9 from the z80pio B control port.
                    infn9  signal possible change in the external state of
                           port A.  bus11, bus12 and bus13 will be checked
                           and if changes have occured then appropriate
                           action will be taken.
                    infn10 signal possible change in the external state of
                           port B.  bus14, bus15 and bus16 will be checked
                           and if changes have occured then appropriate
                           action will be taken.
                    infn11 signal possible change in IEI signal.  bus1 will
                           be checked, and if it has changed (either become 0
                           from nz, or vice-versa) then appropriate action
                           will be taken.
                    infn12 acknowledge interupt signal.  Any relevant vector
                           is assumed to be on bus10.
                    infn13 called to indicate presence of RETI on data bus.

outgoing functions: outfn0 signal possible change in the external state of
                           port A, caused by this module.  One of bus11,
                           bus12 or bus13 may have changed.
                    outfn1 signal possible change in the external state of
                           port B, caused by this module.  One of bus14,
                           bus15 or bus16 may have changed.
                    outfn2 called to indicate a change in the IEO signal,
                           which is on bus0.
                    outfn3 signal interupt.  Interupts typically function
                           as follows:
                           1. the pio sends the interupt by calling outfn3.
                           2. this is passed to the z80 cpu module, which
                              will (if interupts are enabled etc) call back
                              to acknowledge the interupt using infn13,
                              putting the appropriate vector on the data bus
                              if required, and return.
                           3. Once the interupt routine is finished, it may
                              end in reti.  Upon (or near) when the reti is
                              got from memory the function infn14 should be
                              called to finish the interupt cycle.

Module is unclocked.

*/


module_data *z80pio_alloc(const char *module_name);
int          z80pio_init(module_data *what);
void         z80pio_go(module_data *what);
void         z80pio_stop(module_data *what);
void         z80pio_remove(module_data *what);
void         z80pio_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *z80pio_getinf(module_data *what);

#endif
