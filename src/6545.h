/*FIXME: buses marked * should be 8 bit, not 16
  ALSO: - add VDU memory write version for 8-bit address (therefore have both
          8-bit and 16-bit versions of this bus, so the 32k can use the 8bit
          version and be fast, and when extended PCG is present can use the
          16bit version as required).
        - colour: rather than one colour map for each of foreground and
                  background, make it so the colour is the foreground byte
                  for the relevant address, ANDed with a mask, and then ORed
                  with a universal byte for the whole screen.  Then, when
                  background intensity is changed, rather than updating
                  the whole background map, just update this byte and then
                  tell the screen to refresh.
*/

#include "u_dtype.h"
#include "modules.h"

#ifndef _6545_h
#define _6545_h

/*

   +-------+
  -|       |-            +-----------------------------------------+
  -|       |-            |+----+----+----+----+----+----+----+----+|
  -|       |-            ||0000|0001|0002|....|....|004D|004E|004F||
  -|       |-      \\    |+----+----+----+----+----+----+----+----+|
  -|       | ====== \\   ||0050|0051|0052|....|....|009D|009E|009F||
  -|                 \\  |+----+----+----+-       -+----+----+----+|
  -|                  \\ ||00A0|00A1|00A             0ED|00EE|00EF||
  -| 6545 Emulator     \ |+----+----+--       _       --+----+----+|
  -|                     ||....|....|..      / \      ..|....|....||
  -| Alistair Shilton    |+----+----+-       \ /       -+----+----+|
  -| apsh@ecr.mu.oz.au   ||....|....|.     >{|||}      .|....|....||
  -| http://           / |+----+----+--      / \      --+----+----+|
  -|                  // ||....|....|..      \_/      ..|....|....||
  -|                 //  |+----+----+---             ---+----+----+|
  -|       | ====== //   ||06E0|06E1|06E2|.       .|072D|072E|072F||
  -|       |-      //    |+----+----+----+----+----+----+----+----+|
  -|       |-            ||0730|0731|0732|....|....|077D|077E|077F||
  -|       |-            |+----+----+----+----+----+----+----+----+|
  -|       |-            +-----------------------------------------+
  -|       |-
   +-------+



                        6545 CRTC Emulation
                        ===================

The following code emulates the functionality of the Synertek 6545 CRT
controller (CRTC).  In the MicroBee, in addition to controlling the video
display, the 6545 also acts as an interface to the keyboard.

To preload character ROM (height given by vara0 below) follow the same syntax
as was used for the mem module in genmod.h.


Module details
==============

8  bit variables: vara0: character height when loading char ROM (deft 16).
16 bit variables: none
32 bit variables: none

string variables: svar0 preloaded character ROM (leave "" if none).

8  bit buses: busa0  data bus - 6545 port address write.        infn2
              busa1  data bus - 6545 port data write.           infn3
              busa2  data bus - 6545 port address read.         infn4
              busa3  data bus - 6545 port data read.            infn5
              busa4  line bus - character memory write.         infn6
              busa5  pixel bus - pixel drawer.                  outfn6
              busa6  colour bus (foreground) - pixel drawer.    outfn6
              busa7  colour bus (background) - pixel drawer.    outfn6
              busa8  inversion bus - pixel drawer.              outfn6
              busa9  data bus - lpen table access (clk 1).      outfn7
              busa10 data bus - lpen table access (clk 0).      outfn8

16 bit buses: busb0  width bus - left margin.                   outfn0
              busb1  width bus - used screen width.             outfn1
              busb2  width bus - right screen.                  outfn2
              busb3  width bus - top margin.                    outfn3
              busb4  width bus - used screen height.            outfn4
              busb5  width bus - bottom margin.                 outfn5
              busb6  address bus - character memory write.      infn6
              busb7  address bus - video memory write.          infn7
              busb8  address bus - colour (fore) memory write.  infn8
              busb9  address bus - colour (back) memory write.  infn9
*              busb10 data bus - character memory write.         infn6
              busb11 data bus - video memory write.             infn7
*              busb12 data bus - colour (fore) memory write.     infn8
*              busb13 data bus - colour (back) memory write.     infn9
              busb14 x-position bus - pixel drawer.             outfn6
              busb15 y-position bus - pixel drawer.             outfn6
              busb16 address bus - lpen table access (clk 1).   outfn7
              busb17 address bus - lpen table access (clk 0).   outfn8
              busb18 address bus - lpen strobe random (clk 1).  outfn9
              busb19 address bus - lpen strobe random (clk 0).  outfn10
              busb20 address bus - lpen strobe refresh (clk 1). outfn11
              busb21 address bus - lpen strobe refresh (clk 1). outfn12

32 bit buses: busc0 lpen reset counter bus - this bus will be incremented
                    by the 6545 module whenever port 16 or 17 is read.
              busc1 update counter bus - this bus will be incremented by
                    the 6545 module whenever port 31 is read.
              busc2 lightpen call mask bus - the bits present on this bus
                    are used to mask the lightpen.  When the lightpen is
                    to be strobed, an address is calculated as:
                           +------+--------+-------+-------+--------+
                    signal | CR4* | DISPEN | HSYNC | VSYNC | CURSOR |
                    bit    | 4    | 3      | 2     | 1     | 0      |
                           +------+--------+-------+-------+--------+
                    this address specifies a bit position on this bus
                    (ie. LSB if address is 0, MSB if address is 31).  If
                    this bit is zero then the lightpen will be read as zero.
                    Otherwise, the lightpen will be determined by lookup
                    of memory using outfn7 or outfn8.

incoming functions: infn0 reset the 6545 state.
                    infn1 refresh (ie. redraw) the screen.  Usually the
                          6545 only calls the video writing functions when
                          the contents of geometry of the screen are changed.
                          Calling this function will override this behaviour
                          to completely rewrite the next frame.
                    infn2 write busa0 to the 6545 address port.
                    infn3 write busa1 to the 6545 data port.
                    infn4 read busa2 from the 6545 address port.
                    infn5 read busa3 from the 6545 data port.
                    infn6 write to character memory.  busb6 defines which
                          character is being changed, and busa4 says which
                          line of that character is being written (0 being
                          the uppermost line).  The data on busb10 is written
                          to here.
                    infn7 write to video memory.  The value busb11 is written
                          to video memory at address busb7.
                    infn8 write to colour (foreground) memory.  The value
                          busb12 is written to video memory at address busb8.
                    infn9 write to colour (foreground) memory.  The value
                          busb13 is written to video memory at address busb9.

outgoing functions: outfn0  change width of left margin to busb0 pixels.
                    outfn1  change displayed screen width to busb1 pixels.
                    outfn2  change width of right margin to busb2 pixels.
                    outfn3  change width of top margin to busb3 pixels.
                    outfn1  change displayed screen height to busb4 pixels.
                    outfn5  change width of bottom margin to busb5 pixels.
                    outfn6  write pixels to the screen.  This function must
                            draw 8 pixels onto the screen at position
                            (x,y) = (busb14,busb15), which is relative to the
                            upper left-hand corner of the screen, starting
                            at the margins given above (i.e. if (x,y) = (0,0)
                            then the first pixel will be at (x,y) = 
                            (left_margin,top_margin), the second pixel will
                            be at (x,y) = (left_margin+1,top_margin) and so
                            on).
                            The actual pixels are specified by busa5, where
                            the lsb of busa5 is the leftmost (first) pixel
                            and the msb is the rightmost (eighth) pixel.
                            The colour of each pixel (if used) is the same,
                            and is given by busa6 (foreground, if the bit is
                            1) and busa7 (background, if 0).
                            If the pixel drawing function ignores the colour
                            values here (ie. in black/write or greenscreen
                            emulation) then busa8 must also be considered.
                            Specifically, if busa8 != 0 then busa5 should be
                            inverted before use (busa8 should be ignored if
                            colour is used).
                    outfn7  get lpen matrix state, busc2 allowing (see
                            before) when the dotclock is 1.  The contents of
                            the lpen matrix (either 0 or nz) at address
                            busb16 should be placed on busa9.
                    outfn8  get lpen matrix state, busc2 allowing (see
                            before) when the dotclock is 0.  The contents of
                            the lpen matrix (either 0 or nz) at address
                            busb17 should be placed on busa10.
                    outfn9  called when a non-zero return from outfn7
                            triggers a successful strobing of the relevent
                            element, and this occurs due to a random screen
                            scan (not specifically refresh driven).  The
                            address leading to a strobe is placed on busb18.
                    outfn10 called when a non-zero return from outfn8
                            triggers a successful strobing of the relevent
                            element, and this occurs due to a random screen
                            scan (not specifically refresh driven).  The
                            address leading to a strobe is placed on busb19.
                    outfn11 called when a non-zero return from outfn7
                            triggers a successful strobing of the relevent
                            element, and this occurs due to a targetted
                            refresh test (i.e. not random).  The address
                            leading to a strobe is placed on busb20.
                    outfn12 called when a non-zero return from outfn8
                            triggers a successful strobing of the relevent
                            element, and this occurs due to a targetted
                            refresh test (i.e. not random).  The address
                            leading to a strobe is placed on busb21.

Module is clocked.

*/


module_data *sy6545_alloc(const char *module_name);
int          sy6545_init(module_data *what);
void         sy6545_go(module_data *what);
void         sy6545_stop(module_data *what);
void         sy6545_remove(module_data *what);
void         sy6545_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *sy6545_getinf(module_data *what);

#endif
