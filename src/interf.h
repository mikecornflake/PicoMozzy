
#include "u_dtype.h"
#include "configer.h"
#include "modules.h"

#ifdef IS_DJGPP
#include <allegro.h>
#endif
#ifdef IS_CYGWIN
#include <allegro.h>
#endif

#ifndef _interf_h
#define _interf_h

/*
   Microbee emulator => pc interface layer
   =======================================


Module details
==============

8  bit variables: none
16 bit variables: none
32 bit variables: none

string variables: svar0 name of file implementation dependent data

8  bit buses: == graphics section ==
              busa0  pixel bus - pixel drawer.                  infn6
              busa1  colour bus (foreground) - pixel drawer.    infn6
              busa2  colour bus (background) - pixel drawer.    infn6
              busa3  inversion bus - pixel drawer.              infn6
              == keyboard control buses ==
              busa4  lightpen table address.
              busa5  lightpen feedback table address.
              busa6  lightpen feedrfsh table address.
              == parallel port control buses ==
              busa7  data bus   - parallel port input.          outfn7
              busa8  strobe bus - parallel port input.          outfn7
              busa9  ready bus  - parallel port input.          outfn7
              busa10 data bus   - parallel port output.         infn7
              busa11 strobe bus - parallel port output.         infn7
              busa12 ready bus  - parallel port output.         infn7
              == speaker port control buses ==
              busa13 state bus - speaker port.                  infn8
              == tape interface buses ==
              busa14 state bus - tape input.                    outfn8
              busa15 state bus - tape output.                   infn9

16 bit buses: == graphics section ==
              busb0  width bus - left margin.                   infn0
              busb1  width bus - used screen width.             infn1
              busb2  width bus - right screen.                  infn2
              busb3  width bus - top margin.                    infn3
              busb4  width bus - used screen height.            infn4
              busb5  width bus - bottom margin.                 infn5
              busb6  x position bus - pixel drawer.             infn6
              busb7  y position bus - pixel drawer.             infn6
32 bit buses: == keyboard control buses ==
              busc0  lpen reset counter bus.
              busc1  lpen update counter bus.

incoming functions: == graphics functions ==
                    infn0  change width of left margin to busb0 pixels.
                    infn1  change displayed screen width to busb1 pixels.
                    infn2  change width of left margin to busb2 pixels.
                    infn3  change width of top margin to busb3 pixels.
                    infn4  change displayed screen height to busb4 pixels.
                    infn5  change width of bottom margin to busb5 pixels.
                    infn6  write pixels to the screen.  This function draws
                           8 pixels onto the screen at position (x,y) =
                           (busb6,busb7), which is relative to the upper
                           left-hand corner of the screen, starting at the
                           margins given above (i.e. if (x,y) = (0,0) then
                           the first pixel will be at (x,y) =
                           (left_margin,top_margin), the second pixel will be
                           at (x,y) = (left_margin+1,top_margin) and so on).
                           The actual pixels are specified by busa0, where
                           the lsb of busa0 is the leftmost (first) pixel and
                           the msb is the rightmost (eighth) pixel.
                           The colour of each pixel (if used) is the same,
                           and is given by busa1 (foreground, if the bit is
                           1) and busa2 (background, if 0).
                           If the pixel drawing function ignores the colour
                           values here (ie. in black/write or greenscreen
                           emulation) then busa3 must also be considered.
                           Specifically, if busa3 != 0 then busa0 should be
                           inverted before use (busa3 should be ignored if
                           colour is used).
                    == parallel port functions ==
                    infn7  check parallel port buses (busa12, busa13 and
                           busa14) and take appropriate action if required.
                    == speaker port functions ==
                    infn8  check state of speaker port bus (busa15) and
                           take appropriate action if required.
                    == tape interface functions ==
                    infn9  check state of tape output bus (busa17) and take
                           appropriate action if required.


outgoing functions: == graphics functions ==
                    outfn0  called to trigger a screen refresh.
                    == miscellaneous control functions ==
                    outfn1  called to tell the emulator to stop and exit.
                    outfn2  called to turn speed emulation on.
                    outfn3  called to turn speed emulation off.
                    outfn4  called to set the reset flag.
                    outfn5  called to reset the reset flag.
                    outfn6  called to pause emulation.
                    outfn7  called to restart emulation.
                    == parallel port interface functions ==
                    outfn8  called to indicate a (possible) change in the
                            state of the parallel port buses (busa9, busa10
                            and busa11).
                    == tape interface functions ==
                    outfn9  called to indicate a (possible) change in the
                            state of the tape input state bus (busa16).



Module is clocked.

*/


module_data *interf_alloc(const char *module_name);
int          interf_init(module_data *what);
void         interf_go(module_data *what);
void         interf_stop(module_data *what);
void         interf_remove(module_data *what);
void         interf_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *interf_getinf(module_data *what);


#ifdef IS_WEB

/*
   These macros are needed by the emulator for timing.  The emulation needs
   a timer with ~1 millisecond granularity.  Timers are installed using
   either:

   int install_int_ex(void (*proc)(), SECS_TO_TIMER(x)) - call function proc
        every x seconds.  Returns 0 on success.
   int install_int_ex(void (*proc)(), MSEC_TO_TIMER(x)) - call function proc
        every x milliseconds.  Returns 0 on success.

   void remove_int(void (*proc)()) - stop calling the interupt function.
*/

#define END_OF_FUNCTION(x)
#define LOCK_FUNCTION(x)
#define LOCK_VARIABLE(x)
#define END_OF_MAIN()
#define TIMERS_PER_SECOND     1193181L
#define SECS_TO_TIMER(x)      ((long)(x) * TIMERS_PER_SECOND)
#define MSEC_TO_TIMER(x)      ((long)(x) * (TIMERS_PER_SECOND / 1000))
#define BPS_TO_TIMER(x)       (TIMERS_PER_SECOND / (long)(x))
#define BPM_TO_TIMER(x)       ((60 * TIMERS_PER_SECOND) / (long)(x))
int install_int_ex(void (*proc)(), int speed);
void remove_int(void (*proc)());

#endif

#endif
