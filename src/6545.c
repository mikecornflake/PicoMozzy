
#include "6545.h"
#include "u_dtype.h"
#include "debmaloc.h"
#include "modules.h"
#include "beefile.h"
#include <strings.h>


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



                      6545 detailed internal operation
                      ================================

The 6545 has two ports, namely data and address.  The address port controls
what is present at the data port (when written), but may also be read to
obtain information about the status of the 6545 and what attached stuff
is doing (vertical blanking and lpen).  The data register may be read only,
read write or write only depending on what address is being used.



Known Unknowns and ToDo
=======================

- In row/column mode (R8_adm = 1), MA0-7 gives the column address and MA8-13
  the row address.  MA0-7 is offset by R13_ca, and MA8-13 by R13_ra.  I have
  assumed that this is equivalent to offsetting MA_bus by R12_13_.  A
  possible bug with this interpretation is that, under the scheme used here,
  carry from the MSB of MA0-7 to the LSB of MA8-13 is possible during
  offsetting.  None of the docs seem to say if this carry should be masked
  or not, so this may cause issues in some cases (possibly games that rely
  on roll-over?).

- Documentation on the lightpen, update addressing and memory address modes
  (and there interactions) of the 6545 is sparse to say the least.  This
  emulator is based to a large degree on the Synertec datasheet and a
  document I found on the web which covering many of these issues in some
  detail.  Unfortunately, I have no idea who actually wrote the latter
  document, as I failed to record this information - sorry :(.

- Cursor wrap disabled (search for IS_CURSOR = 0 when it should be 1) - this
  is a quick hack.  I known cursor wrap works on the bee, but I need to
  double check to see how exactly, because what I had here wasn't working
  at all.

- Need to made active memory access possible.


The address port
================

Write     +-+-+-+-+-+-+-+-+
          |7|6|5|4|3|2|1|0|
          +-+-+-+-+-+-+-+-+
           | | | | | | | |
           | | | +-+-+-+-+-- Address appearing on the data port.
           | | |
           +-+-+------------ Ignored.

Read:     +-+-+-+-+-+-+-+-+
          |7|6|5|4|3|2|1|0|
          +-+-+-+-+-+-+-+-+
           | | | | | | | |
           | | | +-+-+-+-+-- Not used.
           | | |
           | | +------------ VERTICAL BLANKING
           | |               0 = Scan currently not in vert blanking time.
           | |               1 = Scan currently in vertical blanking time.
           | |
           | +-------------- LPEN_REGISTER_FULL
           |                 0 = Goes 0 when either R16 or R17 is read.
           |                 1 = Goes 1 when an LPEN strobe occurs.
           |
           +---------------- UPDATE_READY
                             0 = Goes 0 when R31 is read or written.
                             1 = Goes 1 when an update strobe occurs.



The data port
=============

 w R0     ________ Horizontal total chars (-1).
 w R1     ________ Horizontal display chars.
 w R2     ________ Horizontal sync position.
 w R3     vvvvhhhh Vertical and horizontal sync widths:
                   bits 0-3: horiz sync width in char times (0-15).
                   bits 4-7: vert sync width in rasterlines (0-15).
                             0 translates to 16 rasterlines.
 w R4     x_______ Vertical total character rows (-1).
 w R5     xxx_____ Vertical total adjust rasterlines.
 w R6     x_______ Vertical displayed character lines.
 w R7     x_______ Vertical sync position.
 w R8     ________ Mode: bit 0,1: ignore for simplicity (interlace control).
                         bit 2: Addressing mode:
                                0 = straight binary addressing.
                                1 = row/column addressing.
                         bit 3: Refresh RAM access:
                                0 = shared memory access.
                                1 = transparent memory access.
                         bit 4,5: Skew control (simplified version):
                                0,0 = no skew.
                                0,1 = cursor effectively 1 char right.
                                1,0 = cursor effectively 1 char left.
                                1,1 = no effective skew.
                         bit 6: Update strobe (transparent mode):
                                0 = pin 34 acts as scanline address RA4.
                                1 = pin 34 acts as update strobe.
                         bit 7: Update/Read mode:
                                0 = update occurs during horiz/vert blank.
                                1 = update interleaves during Phi2 clock.
 w R9     xxx_____ Number of rasterlines per character row (-1).
 w R10    xBB_____ Cursor start rasterline (low 5 bits).
                   BB: +---------+-----------------------------+
                       |   BIT   |                             |
                       +----+----+ CURSOR MODE                 |
                       |  6 |  5 |                             |
                       +----+----+-----------------------------+
                       |  0 |  0 | No blinking                 |
                       |  0 |  1 | No cursor                   |
                       |  1 |  0 | Blink at 1/16 field rate    |
                       |  1 |  1 | Blink at 1/32 field rate    |
                       +----+----+-----------------------------+
 w R11    xxx_____ Cursor end rasterline (if < start, wrap and invert).
 w R12    xx______ Display start address high.
 w R13    ________ Display start address low.
rw R14    xx______ Cursor address high.
rw R15    ________ Cursor address low.
r  R16    xx______ Lightpen address high.
r  R17    ________ Lightpen address low.
 w R18    xx______ Update address high.
 w R19    ________ Update address low.
rw R20-30 xxxxxxxx Nonexistant (read 0, write to nowhere).
rw R31    xxxxxxxx Read or write this port to tell the 6545 to perform an
                   update cycle (if enabled) at the soonest possible
                   oportunity, and make UPDATE READY go to 0.  After this
                   update cycle is completed, the value in the update
                   register will be incremented and UPDATE READY set to 1.

Reading from x gives 0's.  Writing to x has no affect.






What gets displayed where
=========================

Consider an 80 * 24 example.  If R8_adm == 0, the CRTC sees the screen as
follows:

              +----------------- Horizontal total R0 = 59h ----------------+
              |                                                            |
              +------- Horiz displayed R1 = 50h ------+                    |
              |                                       |                    |
                00   01   02  .... ....  4D   4E   4F    50   51  ....  59
             +-----------------------------------------+
 +----+----- |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  00h ||0000|0001|0002|....|....|004D|004E|004F||0050|0051|....|0059|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  01h ||0050|0051|0052|....|....|009D|009E|009F||00A0|00A1|....|00A9|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  02h ||00A0|00A1|00A2|....|....|00ED|00EE|00EF||00F0|00F1|....|00F9|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  ..  ||....|....|....|....|....|....|....|....||....|....|....|....|
 | Vertical  |+----+----+----+----+----+----+----+----+|----+----+----+----+
 | displayed ||....|....|....|....|....|....|....|....||....|....|....|....|
 | R6 = 18h  |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  ..  ||....|....|....|....|....|....|....|....||....|....|....|....|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  16h ||06E0|06E1|06E2|....|....|072D|072E|072F||0730|0731|....|0739|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  17h ||0730|0731|0732|....|....|077D|077E|077F||0780|0781|....|0789|
 |    +----- |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |           +-----------------------------------------+
 |       18h  |0780|0781|0782|....|....|07CD|07CE|07CF| 07D0|07D1|....|07D9|
Vertical      +----+----+----+----+----+----+----+----+ ----+----+----+----+
total    19h  |07D0|07D1|07D2|....|....|081D|081E|081F| 0820|0821|....|0829|
R4 = 21h      +----+----+----+----+----+----+----+----+ ----+----+----+----+
 |       ..   |....|....|....|....|....|....|....|....| ....|....|....|....|
 |            +----+----+----+----+----+----+----+----+ ----+----+----+----+
 |       21h  |0A50|0A51|0A52|....|....|0A9D|0A9E|0A9F| 0AA0|0AA1|....|0AA9|
 +----------  +----+----+----+----+----+----+----+----+ ----+----+----+----+
   R5_ blank  ========================================= ====================
   scanlines  ========================================= ====================

The number given inside the grid gives the offset from the display start
address R12_13_ which is displayed in that part of the grid.  Thus the
character in the upper left corner is at address R12_13_ + 00h in memory (as
seen by the CRTC).  R12_13_ + offset is what appears on the MA bus.  Each
character is R9_+1 rasterlines high by 8 pixels wide.


For each character on the screen:

                                        76543210
                                       +--------+
                                       |********| 00h
   +----+                              |********| 01h
   |0052|    ---- expands to ----->    |********| 02h 
   +----+                              |********| 03h
                                       |  ....  | ...
                                       |********| R9_
                                       +--------+

Where: The *'s represent the pixel pattern of the char at memory 0052.  For
       example:


 Char memory address        Byte            Binary          Character shape
                                                            +--------+
(sy6545_char_memory[0052]).lines[0x000]    10011100        |x..xxx..|
(sy6545_char_memory[0052]).lines[0x001]    10110010        |x.xx..x.|
(sy6545_char_memory[0052]).lines[0x002]    10110010        |x.xx..x.|
(sy6545_char_memory[0052]).lines[0x003]    01100111        |.xx..xxx|
(sy6545_char_memory[0052]).lines[0x004]    00100101        |..x..x.x|
(sy6545_char_memory[0052]).lines[0x005]    00100101        |..x..x.x|
(sy6545_char_memory[0052]).lines[0x006]    00111101        |..xxxx.x|
(sy6545_char_memory[0052]).lines[0x007]    00011001        |...xx..x|
(sy6545_char_memory[0052]).lines[0x008]    00011000        |...xx...|
(sy6545_char_memory[0052]).lines[0x009]    01100110        |.xx..xx.|
(sy6545_char_memory[0052]).lines[0x00A]    01100110        |.xx..xx.|
(sy6545_char_memory[0052]).lines[0x00B]    01000010        |.x....x.|
(sy6545_char_memory[0052]).lines[0x00C]    10000001        |x......x|
(sy6545_char_memory[0052]).lines[0x00D]    11100111        |xxx..xxx|
(sy6545_char_memory[0052]).lines[0x00E]    10100101        |x.x..x.x|
(sy6545_char_memory[0052]).lines[0x00F]    10100101        |x.x..x.x|
                                                             |        |
                                   :            :            |   :    |
                                   :            :            |   :    |
                                                             +--------+

In this diagram, x has color ((sy6545_char_memory[0052]).fore_colour)[0]
while . has color ((sy6545_char_memory[0052]).back_colour)[0].

If R8_adm = 1 then things are different.  The above diagram becomes:

              +----------------- Horizontal total R0 = 59h ----------------+
              |                                                            |
              +------- Horiz displayed R1 = 50h ------+                    |
              |                                       |                    |
                00   01   02  .... ....  4D   4E   4F    50   51  ....  59
             +-----------------------------------------+
 +----+----- |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  00h ||0000|0001|0002|....|....|004D|004E|004F||0050|0051|....|0059|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  01h ||0100|0101|0102|....|....|014D|014E|014F||0150|0151|....|0159|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  02h ||0200|0201|0202|....|....|024D|024E|024F||0250|0251|....|0259|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  ..  ||....|....|....|....|....|....|....|....||....|....|....|....|
 | Vertical  |+----+----+----+----+----+----+----+----+|----+----+----+----+
 | displayed ||....|....|....|....|....|....|....|....||....|....|....|....|
 | R6 = 18h  |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  ..  ||....|....|....|....|....|....|....|....||....|....|....|....|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  16h ||1600|1601|1602|....|....|164D|164E|164F||1650|1651|....|1659|
 |    |      |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |    |  17h ||1700|1701|1702|....|....|174D|174E|174F||1750|1751|....|1759|
 |    +----- |+----+----+----+----+----+----+----+----+|----+----+----+----+
 |           +-----------------------------------------+
 |       18h  |1800|1801|1802|....|....|184D|184E|184F| 1850|1851|....|1859|
Vertical      +----+----+----+----+----+----+----+----+ ----+----+----+----+
total    19h  |1900|1901|1902|....|....|194D|194E|194F| 1950|1951|....|1959|
R4 = 21h      +----+----+----+----+----+----+----+----+ ----+----+----+----+
 |       ..   |....|....|....|....|....|....|....|....| ....|....|....|....|
 |            +----+----+----+----+----+----+----+----+ ----+----+----+----+
 |       21h  |2100|2101|2102|....|....|214D|214E|214F| 2150|2151|....|2159|
 +----------  +----+----+----+----+----+----+----+----+ ----+----+----+----+
   R5_ blank  ========================================= ====================
   scanlines  ========================================= ====================

So, MA0-7 is the column address, and MA8-13 the row address.  Note that the
address on MA0-7 is offset by R13_ca, and the address on MA8-13 is offset by
R13_ra.  Equivalently (and I *think* this is what actually happens), the
MA_bus just has R12_13_ added to it, which is identical to the other mode of
operation.  The only difference between the two is how carry from MA0-7 to
MA8-13 is treated.  In the first scheme, it is prevented, whereas the second
(used here) allows carry.  This may be incorrect.


Horizontal syncing begins at character R2_ and continues until character
R2_+R3_h-1, inclusive (*may be zero width*).  Therefore, at the start of
each scanline, there will be a visible blank area of width (chars):

sy6545_left_margin = R0_-(R2_+R3_h-1)
                     = R0_-R2_-R3_h+1


Likewise, vertical syncing occurs on the R7_'th character, and will go for
R3_v+1 scanlines.  Thus there will be a blank area at the top of the screen
of height (scanlines):

sy6545_top_margin = R5_+((R4_-R7_+1)*(R9_+1))-R3_v






The Cursor
==========

The cursor is displayed at whatever position the MA bus gives the same
address as written to R14_15_ and DISPEN == 1 (but allow for skew).  The
MA_bus address where the cursor is shown is:

   +--------+--------------------------+------------------------+
   | R8_csk | MA_bus when cursor shown | Additional conditions  |
   +--------+--------------------------+------------------------+
   | 00xxxx | R14_15_                  | none                   |
   | 01xxxx | R14_15_-1                | HORIZ_CHAR_COUNT > 0   |
   | 10xxxx | R14_15_+1                | HORIZ_CHAR_COUNT < R0_ |
   | 11xxxx | R14_15_                  | none                   |
   +--------+--------+-----------------+------------------------+

The cursor begins at scanline R10_cs and finishes at R11_.  If
R10_cs > R11_ then the cursor rolls over, so covering all starting from
R10_cs, going to the end of the char, re-starting at the top and finishing
at R11_.  For a given character and scanline, the following conditions must
hold for the cursor signal to be activated:

1. CURSOR_ON_OFF == 1
2. DISPEN == 1
3. Conditions in above table met
4. Either:
   a. ( R11_ >= R10_cs ) and (     ( VERT_SCAN_COUNT-1 >= R10_cs )
                             ( and ( VERT_SCAN_COUNT-1 <= R11_   ) )
   b. ( R11_ <  R10_cs ) and (     ( VERT_SCAN_COUNT-1 <= R11_   )
                                or ( VERT_SCAN_COUNT-1 >= R10_cs ) )

CURSOR_ON_OFF is set as follows

   +--------+-----------------------------------------------------------+
   | R10_bb | CURSOR_ON_OFF = 1 if:                                     |
   +--------+-----------------------------------------------------------+
   |   00   | always                                                    |
   |   01   | never                                                     |
   |   10   | ( 0 <= FRAME_COUNT <= 15 ) or ( 32 <= FRAME_COUNT <= 47 ) |
   |   11   | ( 0 <= FRAME_COUNT <= 31 )                                |
   +--------+-----------------------------------------------------------+

The following internal variables are used:

HORIZ_CHAR_COUNT: horizontal position on screen (characters, start at 0).
VERT_SCAN_COUNT:  scanline of current char being drawn (pixels, start at 0).
DISPEN:           1 during visible display, 0 otherwise.
FRAME_COUNT:      Counts frames, starts at 0, rolls over at 63.




Update Strobing (source: Synertek datasheet SY6545)
===================================================

The contents of MA_bus and CR_bus (and in particular, CR4_state) are not
necessarily just screen positions.  First, note that the MSB of the CR_bus
may be used for an "update strobe" by setting R8_us = 1.  Thus:

if R8_us = 0 then CR_bus = ( VERT_SCAN_COUNT & 0x01F )
if R8_us = 1 then CR_bus = ( VERT_SCAN_COUNT & 0x00F ) + ( UP_STB * 0x010 )

From a source I have but can't recall where I found (largely corroborated
by the Synertek SY6545 datasheet, which is the best datasheet I can find,
although it is not complete):


   Shared and Transparent Addressing (Rockwell 6545 only)
   ------------------------------------------------------

   This mode is set with R8 bit 3.  Writing a 0 sets the shared adressing
   mode.  In this mode the CRTC assumes that the CPU has an independent means
   of accessing the video memory - sharing the memory.  A very common method
   is to switch the memory addresses from CRTC during Phi2/E low to CPU
   during Phi2/E high.

   More interesting is the transparent mode that is set with R8, bit 3=1.
   In this case the CPU cannot directly access the video RAM.  The CRTC has
   to generate the address for the CPU.  This is done via the write-only
   registers

   R18     Update Register high (6 bit)
   R19     Update Register low (8 bit)

   When R8 bit 7=1 then the CRTC puts the display memory address on MA0-13
   during Phi2/E low (first), and the update address from R18/R19 during
   Phi2/E high, mimicking interleaved CPU access [I would assume this to be
   quite difficult if CCLK is not connected to Phi2/E].  In this mode it can
   be assumed that the CPU hardware knows when to access the memory.  When R8
   bit 7=0, then the CRTC waits for the horizontal and vertical retrace
   times to put the update address from R18/R19 on the address lines MA0-13.
   With R8 bit 6=1 pin 34 can be programmed to give a high pulse when the
   update address is valid.  External latches might be necessary to store
   the data between initiating the access and receiving it.

   The CRTC docs do not say anything about the read/write control, so this
   has to be set up with external hardware.

   After each update access the address in the update register is
   incremented by 1.  How does the CRTC know when an update has to be done?
   Status register bit 7 gives the answer.  Reading or writing the -
   otherwise nonexisting - register R31 tells the CRTC to perform an update.


The update strobes UP_STB(_clow) and MA_bus(_clow) are both dependant on
the update mode, and in particular R8_rma, R8_smde, R18_19_, R18_ra, R19_ca
and R31_pnd.




                        Local data:
                        ===========

Raddr   - last value written to the address port (5 bits).
R0_     - horizontal total (-1) (8 bits).
R1_     - horizontal displayed (8 bits).
R2_     - horizontal sync pos (8 bits).
R3_h    - horizontal sync width (can be zero?) (4 bits).
R3_v    - vertical sync width (-1) (4 bits).
R4_     - vertical total (-1) (7 bits).
R5_     - vertical total adjust (5 bits).
R6_     - vertical displayed (7 bits).
R7_     - vertical sync pos (7 bits).
R8_     - masked version of R8 (adm,rma,csk,us and smde are left).
R8_adm  - addressing mode (1 bit). (0x004 by default)
R8_rma  - refresh memory access mode (1 bit).
R8_csk  - cursor skew mode (2 bits). (bbxxxx, where bb are skew)
R8_us   - update strobe mode (1 bit).
R8_smde - update strobe mode (1 bit).
R9_     - rasterlines per character row (-1) (5 bits).
R10_bb  - cursor mode (2 bits).
R10_cs  - cursor start rasterline (5 bits).
R11_    - cursor end rasterline (5 bits).
R12_13_ - display start address (14 bits).
R12_ra  - display start row (6 bits).
R13_ca  - display start column (8 bits).
R14_15_ - cursor address (14 bits).
R14_ra  - cursor row (6 bits).
R15_ca  - cursor column (8 bits).
R16_17_ - lightpen address (14 bits).
R16_ra  - lightpen row (6 bits).
R17_ca  - lightpen column (8 bits).
R18_19_ - update address (14 bits).
R18_ra  - update row (6 bits).
R19_ca  - update column (8 bits).

Note: - R3_v is obtained by subtracting 1 from the value written to the
        MSB of R3 (unless 0, in which case R3_ = 15).





                        Local derived data
                        ==================

UINT_16 sy6545_left_margin = 8*(R0_-R2_-R3_h+1)               (pixels)
UINT_16 sy6545_top_margin  = R5_+((R4_-R7_+1)*(R9_+1))-R3_v   (pixels)

UINT_16 sy6545_screen_width  = 8*R1_                          (pixels)
UINT_16 sy6545_screen_height = R6_*(R9_+1)                    (pixels)

UINT_16 sy6545_right_margin  = 8*(R2_-R1_)                    (pixels)
UINT_16 sy6545_bottom_margin = (R7_-R6_)*(R9_+1)              (pixels)





                        Local counters
                        ==============

The following counters are used, in the ranges given:

HORIZ_CHAR_COUNT: x position in characters (>= 0, <= R0_)
VERT_CHAR_COUNT:  y position in characters (>= 0, <= R4_+1)
VERT_SCAN_COUNT:  in character scanline counter (>= 0, <= R9_)
                  (if VERT_CHAR_COUNT == R4_+1, VERT_SCAN_COUNT <= R5_-1
                  this introduces some complexity if R5_ == 0)
VERT_SYNC_COUNT:  counter for vertical sync width (>= 0, <= R3_v)
FRAME_COUNT:      frame counter (>= 0, <= 63)

Scan goes left to right, top to bottom.  Hence HORIZ_CHAR_COUNT will loop
first, then VERT_SCAN_COUNT, then VERT_CHAR_COUNT and finally FRAME_COUNT.
VERT_SYNC_COUNT only starts being inced when VERT_CHAR_COUNT == R7_ (and
VERT_SCAN_COUNT == 0 to ensure that it only loops once), and keeps going
until it reaches R3_v, where it goes back to zero and stays until the next
vertical sync starts.




                        Local signals and global buses
                        ==============================

The following bus signals are 0 by default, 1 if condition met:

DISPEN:      ( HORIZ_CHAR_COUNT <  R1_      ) and
             ( VERT_CHAR_COUNT  <  R6_      )
IS_HSYNC:    ( HORIZ_CHAR_COUNT >= R2_      ) and
             ( HORIZ_CHAR_COUNT <  R2_+R3_h )
IS_VSYNC:  ( ( VERT_CHAR_COUNT  == R7_      ) and
             ( VERT_SCAN_COUNT  == 0        )     ) or
             ( VERT_SYNC_COUNT  >  0        )
IS_CURSOR: see later.

Likewise, local data is 0 by default, 1 if relevant condition met:

VBLANK:             ( VERT_CHAR_COUNT  >= R6_ )   (0x020 == 1)
HBLANK:             ( HORIZ_CHAR_COUNT >= R1_ )
LPEN_REGISTER_FULL: Goes 0x000 when either R16 or R17 is read.
                    Goes 0x040 when an LPEN strobe occurs.
UPDATE_READY:       Goes 0x000 when R31 is read or written.
                    Goes 0x080 when an update strobe occurs.





                Screen memory and character RAM organisation
                ============================================

The 6545 can address 2^14 bytes of VDU RAM.  Each memory address specifies
a character number (out of 2^16 possible character, each 8*32 pixels in
size), background colour (8 bits) and foreground colour (8 bits).




                        6545 Character storage structure
                        ================================

Defines a 8*32 pixel character.  The character is specified by the contents
of the lines array.  The top line of the character is given by lines[0],
the next by lines[1], etc.  For example:

Byte            Binary          Character shape
                                +--------+
lines[0x000]    10011100        |x  xxx  |
lines[0x001]    10110010        |x xx  x |
lines[0x002]    10110010        |x xx  x |
lines[0x003]    01100111        | xx  xxx|
lines[0x004]    00100101        |  x  x x|
lines[0x005]    00100101        |  x  x x|
lines[0x006]    00111101        |  xxxx x|
lines[0x007]    00011001        |   xx  x|
lines[0x008]    00011000        |   xx   |
lines[0x009]    01100110        | xx  xx |
lines[0x00A]    01100110        | xx  xx |
lines[0x00B]    01000010        | x    x |
lines[0x00C]    10000001        |x      x|
lines[0x00D]    11100111        |xxx  xxx|
lines[0x00E]    10100101        |x x  x x|
lines[0x00F]    10100101        |x x  x x|
                                |        |
      :            :            |   :    |
      :            :            |   :    |
                                +--------+

first_occur points to the first occurrence of the character on the current
screen.  If there are no occurences of this character onscreen then this
will be NULL.




                6545 Screen location storage structure
                ======================================

As well as information specified previously, each bit of vdu must specify
it's position in a character chain (bidirectionally linked list of chars
of the same type, so when this char is changed all occurrences can be
located and changed quickly), as well as a map of all lines that have
changed in this char since it was last drawn.  Thus if the relevant bit
is on, the scanline of the relevant char will be updated and the bit set
to zero once more.


                      Clock Division and Timing
                      =========================

For speed reasons, it may be desirable to run the 6545 CRTC emulator
through less cycles than would happen in reality.  If this is done, it may
be noted that the cursor will flash proportionally slower than normal.  This
is controlled by the data pointed to be sy6545_frame_multiply, which contains
this clock division factor.  The emulator then increments the frame counter
by this, thereby speeding up the cursor rate sufficiently to exactly
compensate for the slowing that may otherwise be seen.










Control functions
=================

void sy6545_reset(what)   - reset function
void sy6545_refresh(what) - called to tell the 6545 to redraw the screen.

void sy6545_addr_wr(what) - pio address write
void sy6545_data_wr(what) - pio data write

void sy6545_addr_rd(what) - pio address read
void sy6545_data_rd(what) - pio data read



Internal Memory Map
===================

The 6545 CRTC emulator maintains an internal version of VDU, character and
colour RAM, which are write-only from a user perspective.  This enables the
emulator to update screen refresh operations.  Each character displayed is
specified by 3 characteristics, namely foreground colour (used to draw the
"set" dots), background colour (the non-set dots) and the character number.
Each character is defined by 32 bytes (8-bit) thusly:

byte 0: defines the top row.
byte 1: defines the 2nd row.
...
byte 31: defines the bottom row.

For each byte, each bit is a pixel, with the leftmost being the MSB and the
rightmost the LSB, where 1 means a foreground pixel and 0 a background pixel.
If the height of the character is less than defined here then rows will be
cut from the base of the character.

The following functions are used to write to this memory:

void sy6545_write_char_mem(what)    - write char memory.
void sy6545_write_vdu_mem(what)     - write VDU memory.
void sy6545_write_fore_colour(what) - write fore colour memory.
void sy6545_write_back_colour(what) - write back colour memory.



External Functionality - the Lightpen
=====================================

In an atual 6545, if a 0-1 transition is detected on the lightpen, ie.:

   1: IS_LPEN(previous) = 0 and IS_LPEN_clow = 1
   2: IS_LPEN_clow = 0 and IS_LPEN = 1

then the value stored that would be on the MA_bus in the next cycle (but
not, apparently, the update address, if relevant) is stored in the lpen
registers (R16_17_, R16_ra and R17_ca) and LPEN_REGISTER_FULL is set to 1.

To emulate this functionality, the 6545 CRTC emulator needs to obtain the
state of the lpen form both the lower and high part of the clock cycle.  It
does this in two steps.

Firstly, for each half-cycle, it examines the sy6545_lpen_call_mask_bus (32
bit).  In particular, it will look at one bit of this mask based on the
following table:

+-----+----------------------------------------+
|     |  6545 signal combination to test bit   |
|     +------+--------+-------+-------+--------+
| bit | CR4* | DISPEN | HSYNC | VSYNC | CURSOR |
+-----+------+--------+-------+-------+--------+
|  0  |  0   |   0    |   0   |   0   |   0    |
|  1  |  0   |   0    |   0   |   0   |   1    |
|  2  |  0   |   0    |   0   |   1   |   0    |
|  3  |  0   |   0    |   0   |   1   |   1    |
|  4  |  0   |   0    |   1   |   0   |   0    |
|  5  |  0   |   0    |   1   |   0   |   1    |
|  6  |  0   |   0    |   1   |   1   |   0    |
|  7  |  0   |   0    |   1   |   1   |   1    |
|  8  |  0   |   1    |   0   |   0   |   0    |
|  9  |  0   |   1    |   0   |   0   |   1    |
| 10  |  0   |   1    |   0   |   1   |   0    |
| 11  |  0   |   1    |   0   |   1   |   1    |
| 12  |  0   |   1    |   1   |   0   |   0    |
| 13  |  0   |   1    |   1   |   0   |   1    |
| 14  |  0   |   1    |   1   |   1   |   0    |
| 15  |  0   |   1    |   1   |   1   |   1    |
| 16  |  1   |   0    |   0   |   0   |   0    |
| 17  |  1   |   0    |   0   |   0   |   1    |
| 18  |  1   |   0    |   0   |   1   |   0    |
| 19  |  1   |   0    |   0   |   1   |   1    |
| 20  |  1   |   0    |   1   |   0   |   0    |
| 21  |  1   |   0    |   1   |   0   |   1    |
| 22  |  1   |   0    |   1   |   1   |   0    |
| 23  |  1   |   0    |   1   |   1   |   1    |
| 24  |  1   |   1    |   0   |   0   |   0    |
| 25  |  1   |   1    |   0   |   0   |   1    |
| 26  |  1   |   1    |   0   |   1   |   0    |
| 27  |  1   |   1    |   0   |   1   |   1    |
| 28  |  1   |   1    |   1   |   0   |   0    |
| 29  |  1   |   1    |   1   |   0   |   1    |
| 30  |  1   |   1    |   1   |   1   |   0    |
| 31  |  1   |   1    |   1   |   1   |   1    |
+-----+------+--------+-------+-------+--------+

NB: * CR4 is what is physically on pin CR4 of the 6545, which may of may
      not be bit 5 of the CR bus (it can be programmed to be the update
      strobe).

If this bit is set, then the process may continue.  Otherwise, lpen is
assumed to be low for the relevant half cycle.  In the former case, the
emulator will use the contents of MA_bus (or MA_bus_clow for the relevant
half-cycle) as an index to lookup one of the tables:

UINT_8 *sy6545_pole_lpen_table;
UINT_8 *sy6545_pole_lpen_clow_table;

The result of this lookup is the state of the lpen pin for this half-cycle.


External Functionality - the Lightpen
=====================================

In an atual 6545, if a 0-1 transition is detected on the lightpen, ie.:

   1: IS_LPEN(previous) = 0 and IS_LPEN_clow = 1
   2: IS_LPEN_clow = 0 and IS_LPEN = 1

then the value stored that would be on the MA_bus in the next cycle (but
not, apparently, the update address, if relevant) is stored in the lpen
registers (R16_17_, R16_ra and R17_ca) and LPEN_REGISTER_FULL is set to 1.

To emulate this functionality, the 6545 CRTC emulator needs to obtain the
state of the lpen form both the lower and high part of the clock cycle.  It
does this in two steps.

Firstly, for each half-cycle, it examines the sy6545_lpen_call_mask_bus (32
bit).  In particular, it will look at one bit of this mask based on the
following table:

+-----+----------------------------------------+
|     |  6545 signal combination to test bit   |
|     +------+--------+-------+-------+--------+
| bit | CR4* | DISPEN | HSYNC | VSYNC | CURSOR |
+-----+------+--------+-------+-------+--------+
|  0  |  0   |   0    |   0   |   0   |   0    |
|  1  |  0   |   0    |   0   |   0   |   1    |
|  2  |  0   |   0    |   0   |   1   |   0    |
|  3  |  0   |   0    |   0   |   1   |   1    |
|  4  |  0   |   0    |   1   |   0   |   0    |
|  5  |  0   |   0    |   1   |   0   |   1    |
|  6  |  0   |   0    |   1   |   1   |   0    |
|  7  |  0   |   0    |   1   |   1   |   1    |
|  8  |  0   |   1    |   0   |   0   |   0    |
|  9  |  0   |   1    |   0   |   0   |   1    |
| 10  |  0   |   1    |   0   |   1   |   0    |
| 11  |  0   |   1    |   0   |   1   |   1    |
| 12  |  0   |   1    |   1   |   0   |   0    |
| 13  |  0   |   1    |   1   |   0   |   1    |
| 14  |  0   |   1    |   1   |   1   |   0    |
| 15  |  0   |   1    |   1   |   1   |   1    |
| 16  |  1   |   0    |   0   |   0   |   0    |
| 17  |  1   |   0    |   0   |   0   |   1    |
| 18  |  1   |   0    |   0   |   1   |   0    |
| 19  |  1   |   0    |   0   |   1   |   1    |
| 20  |  1   |   0    |   1   |   0   |   0    |
| 21  |  1   |   0    |   1   |   0   |   1    |
| 22  |  1   |   0    |   1   |   1   |   0    |
| 23  |  1   |   0    |   1   |   1   |   1    |
| 24  |  1   |   1    |   0   |   0   |   0    |
| 25  |  1   |   1    |   0   |   0   |   1    |
| 26  |  1   |   1    |   0   |   1   |   0    |
| 27  |  1   |   1    |   0   |   1   |   1    |
| 28  |  1   |   1    |   1   |   0   |   0    |
| 29  |  1   |   1    |   1   |   0   |   1    |
| 30  |  1   |   1    |   1   |   1   |   0    |
| 31  |  1   |   1    |   1   |   1   |   1    |
+-----+------+--------+-------+-------+--------+

NB: * CR4 is what is physically on pin CR4 of the 6545, which may of may
      not be bit 5 of the CR bus (it can be programmed to be the update
      strobe).

If this bit is set, then the process may continue.  Otherwise, lpen is
assumed to be low for the relevant half cycle.  In the former case, the
emulator will use the contents of MA_bus (or MA_bus_clow for the relevant
half-cycle) as an index to lookup one of the tables:

UINT_8 *sy6545_pole_lpen_table;
UINT_8 *sy6545_pole_lpen_clow_table;

The result of this lookup is the state of the lpen pin for this half-cycle.


Known Unknowns
==============

- In row/column mode (R8_adm = 1), MA0-7 gives the column address and MA8-13
  the row address.  MA0-7 is offset by R13_ca, and MA8-13 by R13_ra.  I have
  assumed that this is equivalent to offsetting MA_bus by R12_13_.  A
  possible bug with this interpretation is that, under the scheme used here,
  carry from the MSB of MA0-7 to the LSB of MA8-13 is possible during
  offsetting.  None of the docs seem to say if this carry should be masked
  or not, so this may cause issues in some cases (possibly games that rely
  on roll-over?).

- Documentation on the lightpen, update addressing and memory address modes
  (and there interactions) of the 6545 is sparse to say the least.  This
  emulator is based to a large degree on the Synertec datasheet and a
  document I found on the web which covering many of these issues in some
  detail.  Unfortunately, I have no idea who actually wrote the latter
  document, as I failed to record this information - sorry :(.

- Cursor wrap disabled (search for IS_CURSOR = 0 when it should be 1) - this
  is a quick hack.  I known cursor wrap works on the bee, but I need to
  double check to see how exactly, because what I had here wasn't working
  at all.


*/






void sy6545_reset(void *what);
void sy6545_refresh(void *what);

void sy6545_addr_wr(void *what);
void sy6545_data_wr(void *what);

void sy6545_addr_rd(void *what);
void sy6545_data_rd(void *what);

void sy6545_write_char_mem(void *what);
void sy6545_write_vdu_mem(void *what);
void sy6545_write_fore_colour(void *what);
void sy6545_write_back_colour(void *what);













#define C6545_VDU_MEM_SIZE      0x04000 /* VDU memory size == 2^14   */
#define C6545_CHAR_MEM_SIZE     0x10000 /* char memory size = 2^16   */
#define C6545_MAX_CHR_HEIGHT    0x020   /* max scanlines / char = 32 */
#define C6545_DEFAULT_CHAR      0x000   /* default fill character    */
#define C6545_MAX_SCN_HEIGHT    0x100   /* max screen width (chars)  */
#define C6545_MAX_SCN_WIDTH     0x100   /* max screen height (chars) */


/*
   Video character struct (INTERNAL)
   =================================

   The following struct defines the relevant information for a contents of
   a particular character position on the 6545 screen.
*/

typedef struct sy6545_vdu_point
{
    /*
       Character number
       ================

       char_num: defines the character number displayed here.
    */

    UINT_16 char_num;

    /*
       Colour attributes
       =================

       fore_colour[0] = foreground colour
       back_colour[1] = foreground colour
       fore_colour[1] = background colour
       back_colour[0] = background colour
    */

    UINT_8 fore_colour[2];
    UINT_8 back_colour[2];

    /*
       Character modification mask
       ===========================

       line_change_mask: each bit of this mask refers to a line in the
                         character (the LSB being the uppermost).  If a bit
                         is set then the relevant row has changed since the
                         last time it was drawn, and so must be redrawn at
                         some point.
    */

    UINT_32 line_change_mask;

    /*
       Character linking
       =================

       Characters with the same char_num are arranged into a linked list
       using the following pointers (which point to the next element in the
       list and the previous one, respectively).  In this way, if a character
       is changed then the line_change_masks relevant to this change can be
       quickly updated by following the linked list for the char_num in
       question.
    */

    struct sy6545_vdu_point *prev_same_char;
    struct sy6545_vdu_point *next_same_char;

    /*
       Position information
       ====================

       char_x_coord,char_y_coord define the position of the character on
       the screen.  If the character is offscreen the value stored here is
       undefined.
    */

    UINT_8 char_x_coord;
    UINT_8 char_y_coord;

    /*
       Screen Information struct pointers
       ==================================

       These point to the relevant elements of the screen informatinos struct
       pointers to allow for quick updating of these structs when the
       contents of the screen are changed.  If NULL then the character is
       not onscreen.
    */

    UINT_8 *char_pointer; /* NULL if char is not onscreen */
    UINT_8 *fore_pointer; /* NULL if char is not onscreen */
    UINT_8 *back_pointer; /* NULL if char is not onscreen */
}
C6545_vdu_point;

/*
   PCG definition struct (INTERNAL)
   ================================

   This struct defines a programmable character.
*/

typedef struct
{
    /*
       Character lines
       ===============

       Best illustrated by example:

                                       +--------+
       lines[0x000] =  10011100        |x..xxx..|
       lines[0x001] =  10110010        |x.xx..x.|
       lines[0x002] =  10110010        |x.xx..x.|
       lines[0x003] =  01100111        |.xx..xxx|
       lines[0x004] =  00100101        |..x..x.x|
       lines[0x005] =  00100101        |..x..x.x|
       lines[0x006] =  00111101        |..xxxx.x|
       lines[0x007] =  00011001        |...xx..x|
       lines[0x008] =  00011000        |...xx...|
       lines[0x009] =  01100110        |.xx..xx.|
       lines[0x00A] =  01100110        |.xx..xx.|
       lines[0x00B] =  01000010        |.x....x.|
       lines[0x00C] =  10000001        |x......x|
       lines[0x00D] =  11100111        |xxx..xxx|
       lines[0x00E] =  10100101        |x.x..x.x|
       lines[0x00F] =  10100101        |x.x..x.x|
                                       |        |
             :            :            |   :    |
             :            :            |   :    |
                                       +--------+
    */

    UINT_8 lines[C6545_MAX_CHR_HEIGHT];

    /*
       Helper pointer
       ==============

       first_occur: points to the first occurrence of this character in
                    VDU ram.  Thus if the character is changed, you can
                    follow this link to the first occurance on the screen,
                    and thereby the linked list of *all* occurences of this
                    character on the screen, to update the line update
                    masks quickly, without searching.
    */

    struct sy6545_vdu_point *first_occur;
}
C6545_char;







/*
   6545 Object Instance (nominally read only)
   ==========================================
*/

typedef struct
{
    /*
       Raw 6545 data section (register contents).
       ==========================================
    */

    UINT_8  reg_Raddr;
    UINT_8  reg_R0_;
    UINT_8  reg_R1_;
    UINT_8  reg_R2_;
    UINT_8  reg_R3_;
    UINT_8  reg_R3_h;
    UINT_8  reg_R3_v;
    UINT_8  reg_R4_;
    UINT_8  reg_R5_;
    UINT_8  reg_R6_;
    UINT_8  reg_R7_;
    UINT_8  reg_R8_;
    UINT_8  reg_R8_adm;
    UINT_8  reg_R8_csk;
    UINT_8  reg_R9_;
    UINT_8  reg_R10_bb;
    UINT_8  reg_R10_cs;
    UINT_8  reg_R11_;
    UINT_16 reg_R12_13_;
    UINT_8  reg_R12_ra;
    UINT_8  reg_R13_ca;
    UINT_16 reg_R14_15_;
    UINT_8  reg_R14_ra;
    UINT_8  reg_R15_ca;
    UINT_16 reg_R16_17_;
    UINT_8  reg_R16_ra;
    UINT_8  reg_R17_ca;
    UINT_16 reg_R18_19_;
    UINT_8  reg_R18_ra;
    UINT_8  reg_R19_ca;

    /*
       VDU RAM, character RAM and tables
       =================================

       vdu_memory  - arranged in the order seen by the 6545 memory bus.
       char_memory - character RAM.
    */

    C6545_vdu_point *vdu_memory;
    C6545_char      *char_memory;

    /*
       Screen display data
       ===================

       scn_map: defines the content of the screen.  scn_map[i][j] gives
                the char at position i,j on the screen (0 start).
                array size: C6545_MAX_SCN_WIDTH*C6545_MAX_SCN_HEIGHT
       col_map: defines the fore and background colour content of the
                screen.  col_map[i][j][0] is the foreground colour for
                the char at position i,j on the screen (0 start),
                col_map[i][j][1] the background colour of same.
                array size: C6545_MAX_SCN_WIDTH*C6545_MAX_SCN_HEIGHT*2
    */

    UINT_8 **scn_map;
    UINT_8 ***col_map;

    /*
       6545 data section
       =================
    */

    UINT_16 horiz_char_count;    /* horizontal character counter */
    UINT_16 vert_char_count;     /* vertical character counter   */
    UINT_16 vert_scan_count;     /* vertical scanline counter    */
    UINT_8  vert_sync_count;     /* vertical oddity counter      */
    UINT_8  frame_count;         /* frame counter                */

    UINT_8  vblank;              /* 0x020 if in vertical blanking      */
    UINT_8  hblank;              /* 0x001 if in vertical blanking      */
    UINT_8  lpen_register_full;  /* 0x040 if lightpen register is full */
    UINT_8  update_ready;        /* 0x080 if update blah               */

    UINT_16 left_margin;         /* left margin on screen  */
    UINT_16 screen_width;        /* visible screen width   */
    UINT_16 right_margin;        /* right margin on screen */

    UINT_16 top_margin;          /* top margin on screen    */
    UINT_16 screen_height;       /* visible screen height   */
    UINT_16 bottom_margin;       /* bottom margin on screen */

    UINT_16 MA_bus_no_update;    /* MA bus contents (raw form)   */
    UINT_16 MA_bus_clow;         /* MA bus contents (clock low)  */
    UINT_16 MA_bus;              /* MA bus contents (clock high) */
    UINT_8  CR_bus_clow;         /* CR bus contents (clock low)  */
    UINT_8  CR_bus;              /* CR bus contents (clock high) */

    UINT_8  is_cursor;           /* CURSOR signal (00h/01h) */
    UINT_8  is_vsync;            /* VSYNC  signal (00h/02h) */
    UINT_8  is_hsync;            /* HSYNC  signal (00h/04h) */
    UINT_8  dispen;              /* DISPEN signal (00h/08h) */

    UINT_8  update_dispen_count; /* internal counter (update cycle)  */
    UINT_32 line_up_mask;        /* modded internal line update mask */

    /*
       Lightpen stuff
       ==============

       is_lpen_clow:       used to store the lightpen state (low)
       is_lpen:            used to store the lightpen state (high)
       previous_lpen_clow: previous state of is_lpen_clow
       previous_lpen:      previous state of is_lpen
       latch_lpen:         set makes llpen latch on next cycle (must be
                           the next cycle, not this one, to put the correct
                           address into the update register etc.

       lpen_feedback_addr:           temp store of lpen feedback address
       lpen_feedback_addr_type:      set if addr is refresh addr
       lpen_feedback_addr_type_clow: set if addr is refresh addr
    */

    UINT_8  is_lpen_clow;
    UINT_8  is_lpen;
    UINT_8  previous_lpen_clow;
    UINT_8  previous_lpen;
    UINT_8  latch_lpen;

    UINT_16 lpen_feedback_addr;
    UINT_8  lpen_feedback_addr_type;
    UINT_8  lpen_feedback_addr_type_clow;

    /*
       Temporary variables
       ===================
    */

    weird_pointer_jive_wargs lpen_feedback_dxfn;
    void *lpen_feedback_dxargs;
    UINT_16 *lpen_feed_addrx;

    /*
       Redraw control bits
       ===================
    */

    UINT_8  redraw_bit;

    /*
       Stuff to control automatic ROM loading.
    */

    const char *filename;
    char *canfreeme;
    int isrommode;
}
sy6545_state;






















/*
   Macro section
*/

#define DEFAULT_Raddr   0x000
#define DEFAULT_R0_     0x06B
#define DEFAULT_R1_     0x040
#define DEFAULT_R2_     0x051
#define DEFAULT_R3_     0x027
#define DEFAULT_R3_h    0x007
#define DEFAULT_R3_v    0x002
#define DEFAULT_R4_     0x012
#define DEFAULT_R5_     0x009
#define DEFAULT_R6_     0x010
#define DEFAULT_R7_     0x012
#define DEFAULT_R8_     0x0c8
#define DEFAULT_R8_adm  0x000
#define DEFAULT_R8_csk  0x000
#define DEFAULT_R9_     0x00F
#define DEFAULT_R10_bb  0x001
#define DEFAULT_R10_cs  0x00F
#define DEFAULT_R11_    0x00F
#define DEFAULT_R12_13_ 0x00000
#define DEFAULT_R12_ra  0x000
#define DEFAULT_R13_ca  0x000
#define DEFAULT_R14_15_ 0x00000
#define DEFAULT_R14_ra  0x000
#define DEFAULT_R15_ca  0x000
#define DEFAULT_R16_17_ 0x00000
#define DEFAULT_R16_ra  0x000
#define DEFAULT_R17_ca  0x000
#define DEFAULT_R18_19_ 0x00000
#define DEFAULT_R18_ra  0x000
#define DEFAULT_R19_ca  0x000


#define DEFAULT_FORE_COLOUR 255
#define DEFAULT_BACK_COLOUR 0

#define C6545_REDIR(what) ((sy6545_state *) DEREF_INTERNAL(what))


#define RADDR(what)                              (C6545_REDIR(what)->reg_Raddr)
#define R0_(what)                                (C6545_REDIR(what)->reg_R0_)
#define R1_(what)                                (C6545_REDIR(what)->reg_R1_)
#define R2_(what)                                (C6545_REDIR(what)->reg_R2_)
#define R3_(what)                                (C6545_REDIR(what)->reg_R3_)
#define R3_H(what)                               (C6545_REDIR(what)->reg_R3_h)
#define R3_V(what)                               (C6545_REDIR(what)->reg_R3_v)
#define R4_(what)                                (C6545_REDIR(what)->reg_R4_)
#define R5_(what)                                (C6545_REDIR(what)->reg_R5_)
#define R6_(what)                                (C6545_REDIR(what)->reg_R6_)
#define R7_(what)                                (C6545_REDIR(what)->reg_R7_)
#define R8_(what)                                (C6545_REDIR(what)->reg_R8_)
#define R8_ADM(what)                             (C6545_REDIR(what)->reg_R8_adm)
#define R8_CSK(what)                             (C6545_REDIR(what)->reg_R8_csk)
#define R9_(what)                                (C6545_REDIR(what)->reg_R9_)
#define R10_BB(what)                             (C6545_REDIR(what)->reg_R10_bb)
#define R10_CS(what)                             (C6545_REDIR(what)->reg_R10_cs)
#define R11_(what)                               (C6545_REDIR(what)->reg_R11_)
#define R12_13_(what)                            (C6545_REDIR(what)->reg_R12_13_)
#define R12_RA(what)                             (C6545_REDIR(what)->reg_R12_ra)
#define R13_CA(what)                             (C6545_REDIR(what)->reg_R13_ca)
#define R14_15_(what)                            (C6545_REDIR(what)->reg_R14_15_)
#define R14_RA(what)                             (C6545_REDIR(what)->reg_R14_ra)
#define R15_CA(what)                             (C6545_REDIR(what)->reg_R15_ca)
#define R16_17_(what)                            (C6545_REDIR(what)->reg_R16_17_)
#define R16_RA(what)                             (C6545_REDIR(what)->reg_R16_ra)
#define R17_CA(what)                             (C6545_REDIR(what)->reg_R17_ca)
#define R18_19_(what)                            (C6545_REDIR(what)->reg_R18_19_)
#define R18_RA(what)                             (C6545_REDIR(what)->reg_R18_ra)
#define R19_CA(what)                             (C6545_REDIR(what)->reg_R19_ca)

#define sy6545_VDU_MEMORY(what)                  (C6545_REDIR(what)->vdu_memory)
#define sy6545_CHAR_MEMORY(what)                 (C6545_REDIR(what)->char_memory)

#define sy6545_SCN_MAP(what)                     (C6545_REDIR(what)->scn_map)
#define sy6545_COL_MAP(what)                     (C6545_REDIR(what)->col_map)

#define sy6545_HORIZ_CHAR_COUNT(what)            (C6545_REDIR(what)->horiz_char_count)
#define sy6545_VERT_CHAR_COUNT(what)             (C6545_REDIR(what)->vert_char_count)
#define sy6545_VERT_SCAN_COUNT(what)             (C6545_REDIR(what)->vert_scan_count)
#define sy6545_VERT_SYNC_COUNT(what)             (C6545_REDIR(what)->vert_sync_count)
#define sy6545_FRAME_COUNT(what)                 (C6545_REDIR(what)->frame_count)

#define sy6545_VBLANK(what)                      (C6545_REDIR(what)->vblank)
#define sy6545_HBLANK(what)                      (C6545_REDIR(what)->hblank)
#define sy6545_LPEN_REGISTER_FULL(what)          (C6545_REDIR(what)->lpen_register_full)
#define sy6545_UPDATE_READY(what)                (C6545_REDIR(what)->update_ready)

#define sy6545_LEFT_MARGIN(what)                 (C6545_REDIR(what)->left_margin)
#define sy6545_SCREEN_WIDTH(what)                (C6545_REDIR(what)->screen_width)
#define sy6545_RIGHT_MARGIN(what)                (C6545_REDIR(what)->right_margin)

#define sy6545_TOP_MARGIN(what)                  (C6545_REDIR(what)->top_margin)
#define sy6545_SCREEN_HEIGHT(what)               (C6545_REDIR(what)->screen_height)
#define sy6545_BOTTOM_MARGIN(what)               (C6545_REDIR(what)->bottom_margin)

#define sy6545_MA_BUS_CLOW(what)                 (C6545_REDIR(what)->MA_bus_clow)
#define sy6545_MA_BUS(what)                      (C6545_REDIR(what)->MA_bus)
#define sy6545_CR_BUS_CLOW(what)                 (C6545_REDIR(what)->CR_bus_clow)
#define sy6545_CR_BUS(what)                      (C6545_REDIR(what)->CR_bus)

#define sy6545_REDRAW_BIT(what)                  (C6545_REDIR(what)->redraw_bit)





#define sy6545_LATCH_LPEN(what)                  (C6545_REDIR(what)->latch_lpen)
#define sy6545_MA_BUS_NO_UPDATE(what)            (C6545_REDIR(what)->MA_bus_no_update)
#define sy6545_UPDATE_DISPEN_COUNT(what)         (C6545_REDIR(what)->update_dispen_count)
#define sy6545_LINE_UP_MASK(what)                (C6545_REDIR(what)->line_up_mask)
#define sy6545_IS_CURSOR(what)                   (C6545_REDIR(what)->is_cursor)
#define sy6545_DISPEN(what)                      (C6545_REDIR(what)->dispen)
#define sy6545_IS_HSYNC(what)                    (C6545_REDIR(what)->is_hsync)
#define sy6545_IS_VSYNC(what)                    (C6545_REDIR(what)->is_vsync)
#define sy6545_IS_LPEN_CLOW(what)                (C6545_REDIR(what)->is_lpen_clow)
#define sy6545_IS_LPEN(what)                     (C6545_REDIR(what)->is_lpen)
#define sy6545_PREVIOUS_LPEN_CLOW(what)          (C6545_REDIR(what)->previous_lpen_clow)
#define sy6545_PREVIOUS_LPEN(what)               (C6545_REDIR(what)->previous_lpen)

#define sy6545_ASSUMED_ROMCHAR_HEIGHT(what)      DEREF_8VAR(what,0)
#define sy6545_FILENAME(what)                    DEREF_STRGVAR(what,0)

#define sy6545_COMMS_DATA_BUS_A(what)            DEREF_8BUS(what,0)
#define sy6545_COMMS_DATA_BUS_B(what)            DEREF_8BUS(what,1)
#define sy6545_COMMS_DATA_BUS_C(what)            DEREF_8BUS(what,2)
#define sy6545_COMMS_DATA_BUS_D(what)            DEREF_8BUS(what,3)
#define sy6545_VIDEO_CHAR_LINE_BUS(what)         DEREF_8BUS(what,4)
#define sy6545_IS_FORE_BUS(what)                 DEREF_8BUS(what,5)
#define sy6545_FORE_COLOUR_BUS(what)             DEREF_8BUS(what,6)
#define sy6545_BACK_COLOUR_BUS(what)             DEREF_8BUS(what,7)
#define sy6545_INV__COLOUR_BUS(what)             DEREF_8BUS(what,8)
#define sy6545_LPEN_DATA_BUS(what)               DEREF_8BUS(what,9)
#define sy6545_LPEN_DATA_BUS_CLOW(what)          DEREF_8BUS(what,10)

#define sy6545_GEOMETRY_BUS_L(what)              DEREF_16BUS(what,0)
#define sy6545_GEOMETRY_BUS_W(what)              DEREF_16BUS(what,1)
#define sy6545_GEOMETRY_BUS_R(what)              DEREF_16BUS(what,2)
#define sy6545_GEOMETRY_BUS_T(what)              DEREF_16BUS(what,3)
#define sy6545_GEOMETRY_BUS_H(what)              DEREF_16BUS(what,4)
#define sy6545_GEOMETRY_BUS_B(what)              DEREF_16BUS(what,5)
#define sy6545_VIDEO_MEM_ADDR_BUS_C(what)        DEREF_16BUS(what,6)
#define sy6545_VIDEO_MEM_ADDR_BUS_V(what)        DEREF_16BUS(what,7)
#define sy6545_VIDEO_MEM_ADDR_BUS_FC(what)       DEREF_16BUS(what,8)
#define sy6545_VIDEO_MEM_ADDR_BUS_BC(what)       DEREF_16BUS(what,9)
#define sy6545_VIDEO_DATA_BUS_C(what)            DEREF_16BUS(what,10)
#define sy6545_VIDEO_DATA_BUS_V(what)            DEREF_16BUS(what,11)
#define sy6545_VIDEO_DATA_BUS_FC(what)           DEREF_16BUS(what,12)
#define sy6545_VIDEO_DATA_BUS_BC(what)           DEREF_16BUS(what,13)
#define sy6545_XPOS_BUS(what)                    DEREF_16BUS(what,14)
#define sy6545_YPOS_BUS(what)                    DEREF_16BUS(what,15)
#define sy6545_LPEN_ADDR_BUS(what)               DEREF_16BUS(what,16)
#define sy6545_LPEN_ADDR_BUS_CLOW(what)          DEREF_16BUS(what,17)
#define sy6545_LPEN_ADDR_BUS_BACK(what)          DEREF_16MEM(what,18)
#define sy6545_LPEN_ADDR_BUS_BACK_CLOW(what)     DEREF_16MEM(what,19)
#define sy6545_LPEN_ADDR_BUS_RFSH(what)          DEREF_16MEM(what,20)
#define sy6545_LPEN_ADDR_BUS_RFSH_CLOW(what)     DEREF_16MEM(what,21)

#define sy6545_LPEN_RESET_COUNTER_BUS(what)      DEREF_32BUS(what,0)
#define sy6545_UPDATE_RESET_COUNTER_BUS(what)    DEREF_32BUS(what,1)
#define sy6545_LPEN_CALL_MASK_BUS(what)          DEREF_32BUS(what,2)

#define sy6545_CHANGE_LEFT_MARGIN(what)          OUTFNCALL(what,0)
#define sy6545_CHANGE_SCREEN_WIDTH(what)         OUTFNCALL(what,1)
#define sy6545_CHANGE_RIGHT_MARGIN(what)         OUTFNCALL(what,2)
#define sy6545_CHANGE_TOP_MARGIN(what)           OUTFNCALL(what,3)
#define sy6545_CHANGE_SCREEN_HEIGHT(what)        OUTFNCALL(what,4)
#define sy6545_CHANGE_BOTTOM_MARGIN(what)        OUTFNCALL(what,5)
#define sy6545_PIXEL_DRAWER(what)                OUTFNCALL(what,6)
#define sy6545_POLE_LPEN_TABLE_READ(what)        OUTFNCALL(what,7)
#define sy6545_POLE_LPEN_TABLE_READ_CLOW(what)   OUTFNCALL(what,8)
#define sy6545_FEEDBACK_INCER(what)              OUTFNCALL(what,9)
#define sy6545_FEEDBACK_INCER_CLOW(what)         OUTFNCALL(what,10)
#define sy6545_FEEDRFSH_INCER(what)              OUTFNCALL(what,11)
#define sy6545_FEEDRFSH_INCER_CLOW(what)         OUTFNCALL(what,12)

#define sy6545_FEEDBACK_INCER_dxfn(what)         DEREF_OUTFN(what,9)
#define sy6545_FEEDBACK_INCER_dxfn_CLOW(what)    DEREF_OUTFN(what,10)
#define sy6545_FEEDRFSH_INCER_dxfn(what)         DEREF_OUTFN(what,11)
#define sy6545_FEEDRFSH_INCER_dxfn_CLOW(what)    DEREF_OUTFN(what,12)
#define sy6545_FEEDBACK_INCER_dxargs(what)       DEREF_OUTARGS(what,9)
#define sy6545_FEEDBACK_INCER_dxargs_CLOW(what)  DEREF_OUTARGS(what,10)
#define sy6545_FEEDRFSH_INCER_dxargs(what)       DEREF_OUTARGS(what,11)
#define sy6545_FEEDRFSH_INCER_dxargs_CLOW(what)  DEREF_OUTARGS(what,12)

#define sy6545_LPEN_FEEDBACK_ADDR(what)          (C6545_REDIR(what)->lpen_feedback_addr)
#define sy6545_LPEN_FEEDBACK_ADDR_TYPE(what)     (C6545_REDIR(what)->lpen_feedback_addr_type)
#define sy6545_LPEN_FEEDBACK_ADDR_TYPE_CLW(what) (C6545_REDIR(what)->lpen_feedback_addr_type_clow)

#define sy6545_FEEDx_INCER_dxfn_SEL(what)        (C6545_REDIR(what)->lpen_feedback_dxfn)
#define sy6545_FEEDx_INCER_dxargs_SEL(what)      (C6545_REDIR(what)->lpen_feedback_dxargs)
#define sy6545_FEEDx_INCER_dx_SEL(what)          (C6545_REDIR(what)->lpen_feedback_dxfn)(C6545_REDIR(what)->lpen_feedback_dxargs)
#define sy6545_LPEN_ADDR_BUS_SEL(what)           (C6545_REDIR(what)->lpen_feed_addrx)


#define SET_C6545_SCREEN_WIDTH(what)    sy6545_SCREEN_WIDTH(what)    = 8*((UINT_16) R1_(what));
#define SET_C6545_SCREEN_HEIGHT(what)   sy6545_SCREEN_HEIGHT(what)   = ((UINT_16) (R6_(what)))*((((UINT_16) R9_(what))+1));

#define SET_C6545_LEFT_MARGIN(what)     sy6545_LEFT_MARGIN(what)     = 8*((((UINT_16) R0_(what))-((UINT_16) R2_(what))-((UINT_16) R3_H(what))+1));
#define SET_C6545_TOP_MARGIN(what)      sy6545_TOP_MARGIN(what)      = (((UINT_16) (R5_(what)))+(((UINT_16) (((UINT_16) R4_(what))-((UINT_16) R7_(what))+1))*((UINT_16) (((UINT_16) R9_(what))+1)))-((UINT_16) R3_V(what)));

#define SET_C6545_RIGHT_MARGIN(what)    sy6545_RIGHT_MARGIN(what)    = 8*((UINT_16) (R2_(what)-R1_(what)));
#define SET_C6545_BOTTOM_MARGIN(what)   sy6545_BOTTOM_MARGIN(what)   = ( ((UINT_16) (R7_(what)-R6_(what))) * ((UINT_16) (((UINT_16) R9_(what))+1)) );







void sy6545_fix_coordspoint(module_data *what);





























/*
Function: sy6545_alloc()
Operation: see modules.h
*/

module_data *sy6545_alloc(const char *module_name)
{
    module_data *what;

    what = gen_module_data(module_name,1,1,0,0,1,0,11,22,3,10,13);

    sy6545_ASSUMED_ROMCHAR_HEIGHT(what) = 16;

    if ( ( DEREF_INTERNAL(what) = (void *) DEBMALLOC(sizeof(sy6545_state)) ) == NULL )
    {
        return NULL;
    }

    return what;
}



/*
Function: sy6545_init()
Operation: see modules.h
*/

int sy6545_init(module_data *what)
{
    UINT_32 i,j;
    PC_FILE *fp;

    DEREF_INFN(what,0) = sy6545_reset;
    DEREF_INFN(what,1) = sy6545_refresh;
    DEREF_INFN(what,2) = sy6545_addr_wr;
    DEREF_INFN(what,3) = sy6545_data_wr;
    DEREF_INFN(what,4) = sy6545_addr_rd;
    DEREF_INFN(what,5) = sy6545_data_rd;
    DEREF_INFN(what,6) = sy6545_write_char_mem;
    DEREF_INFN(what,7) = sy6545_write_vdu_mem;
    DEREF_INFN(what,8) = sy6545_write_fore_colour;
    DEREF_INFN(what,9) = sy6545_write_back_colour;

    {
        /*
           Allocate video memory maps and clear.
        */

        if ( ( sy6545_SCN_MAP(what) = (UINT_8 **) DEBMALLOC(C6545_MAX_SCN_WIDTH*sizeof(UINT_8 *)) ) == NULL )
        {
            return 1;
        }

        for ( i = 0 ; i < C6545_MAX_SCN_WIDTH ; i++ )
        {
            if ( ( (sy6545_SCN_MAP(what))[i] = (UINT_8 *) DEBMALLOC(C6545_MAX_SCN_HEIGHT*sizeof(UINT_8)) ) == NULL )
            {
                return 2;
            }

            for ( j = 1 ; j < C6545_MAX_SCN_HEIGHT ; j++ )
            {
                (sy6545_SCN_MAP(what))[i][j] = 0;
            }
        }

        if ( ( sy6545_COL_MAP(what) = (UINT_8 ***) DEBMALLOC(C6545_MAX_SCN_WIDTH*sizeof(UINT_8 **)) ) == NULL )
        {
            return 3;
        }

        for ( i = 0 ; i < C6545_MAX_SCN_WIDTH ; i++ )
        {
            if ( ( (sy6545_COL_MAP(what))[i] = (UINT_8 **) DEBMALLOC(C6545_MAX_SCN_HEIGHT*sizeof(UINT_8 *)) ) == NULL )
            {
                return 4;
            }

            for ( j = 0 ; j < C6545_MAX_SCN_HEIGHT ; j++ )
            {
                if ( ( (sy6545_COL_MAP(what))[i][j] = (UINT_8 *) malloc(2*sizeof(UINT_8)) ) == NULL )
                {
                    return 5;
                }

                (sy6545_COL_MAP(what))[i][j][0] = 0;
                (sy6545_COL_MAP(what))[i][j][1] = 0;
            }
        }

        if ( ( sy6545_VDU_MEMORY(what) = (C6545_vdu_point *) DEBMALLOC(C6545_VDU_MEM_SIZE*sizeof(C6545_vdu_point)) ) == NULL )
        {
            return 6;
        }

        if ( ( sy6545_CHAR_MEMORY(what) = (C6545_char *) DEBMALLOC(C6545_CHAR_MEM_SIZE*sizeof(C6545_char)) ) == NULL )
        {
            return 7;
        }

        /*
           Test to see if ROM
        */

        if ( strlen(sy6545_FILENAME(what)) > 0 )
        {
            if ( ( fp = pc_fopen(sy6545_FILENAME(what),"rb") ) != NULL )
            {
                for ( i = 0 ; i < C6545_CHAR_MEM_SIZE ; i++ )
                {
                    for ( j = 0 ; j < C6545_MAX_CHR_HEIGHT ; j++ )
                    {
                        if ( !feof(fp) && ( j < sy6545_ASSUMED_ROMCHAR_HEIGHT(what) ) )
                        {
                            (((sy6545_CHAR_MEMORY(what))[i]).lines)[j] = pc_fgetc(fp);
                        }

                        else
                        {
                            (((sy6545_CHAR_MEMORY(what))[i]).lines)[j] = 0x000;
                        }
                    }

                    ((sy6545_CHAR_MEMORY(what))[i]).first_occur = NULL;
                }

                pc_fclose(fp);
            }

            else
            {
                return 55;
            }
        }

        else
        {
            for ( i = 0 ; i < C6545_CHAR_MEM_SIZE ; i++ )
            {
                for ( j = 0 ; j < C6545_MAX_CHR_HEIGHT ; j++ )
                {
                    (((sy6545_CHAR_MEMORY(what))[i]).lines)[j] = 0x000;
                }

                ((sy6545_CHAR_MEMORY(what))[i]).first_occur = NULL;
            }
        }

        ((sy6545_CHAR_MEMORY(what))[C6545_DEFAULT_CHAR]).first_occur = sy6545_VDU_MEMORY(what);

        for ( i = 0 ; i < C6545_VDU_MEM_SIZE ; i++ )
        {
            ((sy6545_VDU_MEMORY(what))[i]).char_num = C6545_DEFAULT_CHAR;

            (((sy6545_VDU_MEMORY(what))[i]).fore_colour)[0] = DEFAULT_FORE_COLOUR;
            (((sy6545_VDU_MEMORY(what))[i]).back_colour)[0] = DEFAULT_BACK_COLOUR;

            (((sy6545_VDU_MEMORY(what))[i]).fore_colour)[1] = DEFAULT_BACK_COLOUR;
            (((sy6545_VDU_MEMORY(what))[i]).back_colour)[1] = DEFAULT_FORE_COLOUR;

            ((sy6545_VDU_MEMORY(what))[i]).line_change_mask = 0x0FFFFFFFF;

            switch ( i )
            {
                case 0:
                {
                    ((sy6545_VDU_MEMORY(what))[i]).prev_same_char = NULL;
                    ((sy6545_VDU_MEMORY(what))[i]).next_same_char = &((sy6545_VDU_MEMORY(what))[i+1]);

                    break;
                }

                case C6545_VDU_MEM_SIZE-1:
                {
                    ((sy6545_VDU_MEMORY(what))[i]).prev_same_char = &((sy6545_VDU_MEMORY(what))[i-1]);
                    ((sy6545_VDU_MEMORY(what))[i]).next_same_char = NULL;

                    break;
                }

                default:
                {
                    ((sy6545_VDU_MEMORY(what))[i]).prev_same_char = &((sy6545_VDU_MEMORY(what))[i-1]);
                    ((sy6545_VDU_MEMORY(what))[i]).next_same_char = &((sy6545_VDU_MEMORY(what))[i+1]);

                    break;
                }
            }

            ((sy6545_VDU_MEMORY(what))[i]).char_x_coord = 0;
            ((sy6545_VDU_MEMORY(what))[i]).char_y_coord = 0;

            ((sy6545_VDU_MEMORY(what))[i]).char_pointer = 0;
            ((sy6545_VDU_MEMORY(what))[i]).fore_pointer = 0;
            ((sy6545_VDU_MEMORY(what))[i]).back_pointer = 0;
        }
    }

    return 0;
}

void sy6545_go(module_data *what)
{
    sy6545_reset((void *) what);

    sy6545_LATCH_LPEN(what)          = 0;
    sy6545_MA_BUS_NO_UPDATE(what)    = 0;
    sy6545_UPDATE_DISPEN_COUNT(what) = 0;
    sy6545_LINE_UP_MASK(what)        = 0;
    sy6545_IS_CURSOR(what)           = 0;
    sy6545_DISPEN(what)              = 0;
    sy6545_IS_HSYNC(what)            = 0;
    sy6545_IS_VSYNC(what)            = 0;
    sy6545_IS_LPEN_CLOW(what)        = 0;
    sy6545_IS_LPEN(what)             = 0;
    sy6545_PREVIOUS_LPEN(what)       = 0;
    sy6545_PREVIOUS_LPEN_CLOW(what)  = 0;

    return;
}




/*
Function: sy6545_reset()
Operation: Reset the internal state of the 6545 to power on defaults.
*/

void sy6545_reset(void *what)
{
    #ifdef DEBUGMODE
    fprintf(stderr,"reset start\n");
    #endif

    RADDR(what)       = DEFAULT_Raddr;
    R0_(what)         = DEFAULT_R0_;
    R1_(what)         = DEFAULT_R1_;
    R2_(what)         = DEFAULT_R2_;
    R3_(what)         = DEFAULT_R3_;
    R3_H(what)        = DEFAULT_R3_h;
    R3_V(what)        = DEFAULT_R3_v;
    R4_(what)         = DEFAULT_R4_;
    R5_(what)         = DEFAULT_R5_;
    R6_(what)         = DEFAULT_R6_;
    R7_(what)         = DEFAULT_R7_;
    R8_(what)         = DEFAULT_R8_;
    R8_ADM(what)      = DEFAULT_R8_adm;
    R8_CSK(what)      = DEFAULT_R8_csk;
    R9_(what)         = DEFAULT_R9_;
    R10_BB(what)      = DEFAULT_R10_bb;
    R10_CS(what)      = DEFAULT_R10_cs;
    R11_(what)        = DEFAULT_R11_;
    R12_13_(what)     = DEFAULT_R12_13_;
    R12_RA(what)      = DEFAULT_R12_ra;
    R13_CA(what)      = DEFAULT_R13_ca;
    R14_15_(what)     = DEFAULT_R14_15_;
    R14_RA(what)      = DEFAULT_R14_ra;
    R15_CA(what)      = DEFAULT_R15_ca;
    R16_17_(what)     = DEFAULT_R16_17_;
    R16_RA(what)      = DEFAULT_R16_ra;
    R17_CA(what)      = DEFAULT_R17_ca;
    R18_19_(what)     = DEFAULT_R18_19_;
    R18_RA(what)      = DEFAULT_R18_ra;
    R19_CA(what)      = DEFAULT_R19_ca;

    sy6545_HORIZ_CHAR_COUNT(what) = 0;
    sy6545_VERT_CHAR_COUNT(what)  = 0;
    sy6545_VERT_SCAN_COUNT(what)  = 0;
    sy6545_VERT_SYNC_COUNT(what)  = 0;
    sy6545_FRAME_COUNT(what)      = 0;

    sy6545_VBLANK(what)             = 0;
    sy6545_HBLANK(what)             = 0;
    sy6545_LPEN_REGISTER_FULL(what) = 0;
    sy6545_UPDATE_READY(what)       = 0x080;

    #ifdef DEBUGMODE
    fprintf(stderr,"set margins\n");
    #endif

    SET_C6545_LEFT_MARGIN(what);
    SET_C6545_SCREEN_WIDTH(what);
    SET_C6545_RIGHT_MARGIN(what);

    SET_C6545_TOP_MARGIN(what);
    SET_C6545_SCREEN_HEIGHT(what);
    SET_C6545_BOTTOM_MARGIN(what);

    #ifdef DEBUGMODE
    fprintf(stderr,"call externals to set margins there\n");
    #endif

    sy6545_GEOMETRY_BUS_L(what) = sy6545_LEFT_MARGIN(what);  sy6545_CHANGE_LEFT_MARGIN(what);
    sy6545_GEOMETRY_BUS_W(what) = sy6545_SCREEN_WIDTH(what); sy6545_CHANGE_SCREEN_WIDTH(what);
    sy6545_GEOMETRY_BUS_R(what) = sy6545_RIGHT_MARGIN(what); sy6545_CHANGE_RIGHT_MARGIN(what);

    sy6545_GEOMETRY_BUS_T(what) = sy6545_TOP_MARGIN(what);    sy6545_CHANGE_TOP_MARGIN(what);
    sy6545_GEOMETRY_BUS_H(what) = sy6545_SCREEN_HEIGHT(what); sy6545_CHANGE_SCREEN_HEIGHT(what);
    sy6545_GEOMETRY_BUS_B(what) = sy6545_BOTTOM_MARGIN(what); sy6545_CHANGE_BOTTOM_MARGIN(what);

    #ifdef DEBUGMODE
    fprintf(stderr,"reset extended pointers\n");
    #endif

    sy6545_LPEN_FEEDBACK_ADDR(what) = 0;

    sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDBACK_INCER_dxfn(what);
    sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDBACK_INCER_dxargs(what);
    sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_BACK(what);

    #ifdef DEBUGMODE
    fprintf(stderr,"set to redraw\n");
    #endif

    sy6545_REDRAW_BIT(what) |= 0x02;

    #ifdef DEBUGMODE
    fprintf(stderr,"fix details\n");
    #endif

    #ifdef DEBUGMODE
    fprintf(stderr,"start map\n");
    #endif

    sy6545_fix_coordspoint(what);

    #ifdef DEBUGMODE
    fprintf(stderr,"fix cursor\n");
    #endif

    return;
}


/*
Function: sy6545_refresh()
Operation: Reset the internal state of the 6545 to power on defaults.
*/

void sy6545_refresh(void *what)
{
    sy6545_REDRAW_BIT(what) |= 0x02;

    return;
}



void sy6545_stop(module_data *what)
{
     return;

     what = NULL;
}


/*
Function: sy6545_remove()
Operation: Remove 6545 emulator.
*/

void sy6545_remove(module_data *what)
{
    UINT_32 i,j;

    if ( what != NULL )
    {
        if ( DEREF_INTERNAL(what) != NULL )
        {
            if ( (sy6545_SCN_MAP(what)) != NULL )
            {
                for ( i = 0 ; i < C6545_MAX_SCN_WIDTH ; i++ )
                {
                    if ( (sy6545_SCN_MAP(what))[i] != NULL )
                    {
                        DEBFREE((sy6545_SCN_MAP(what))[i]);
                    }
                }

                DEBFREE(sy6545_SCN_MAP(what));
            }

            if ( (sy6545_COL_MAP(what)) != NULL )
            {
                for ( i = 0 ; i < C6545_MAX_SCN_WIDTH ; i++ )
                {
                    if ( (sy6545_COL_MAP(what))[i] != NULL )
                    {
                        for ( j = 0 ; j < C6545_MAX_SCN_HEIGHT ; j++ )
                        {
                            if ( (sy6545_COL_MAP(what))[i][j] != NULL )
                            {
                                free((sy6545_COL_MAP(what))[i][j]);
                            }
                        }

                        DEBFREE((sy6545_COL_MAP(what))[i]);
                    }
                }

                DEBFREE(sy6545_COL_MAP(what));
            }

            if ( sy6545_VDU_MEMORY(what) != NULL )
            {
                DEBFREE(sy6545_VDU_MEMORY(what));
            }

            if ( sy6545_CHAR_MEMORY(what) != NULL )
            {
                DEBFREE(sy6545_CHAR_MEMORY(what));
            }

            DEBFREE(DEREF_INTERNAL(what));
        }

        free_module_data(what);
    }

    return;
}


#define SY6545_MESSAGE_SIZE 1250

char *sy6545_getinf(module_data *what)
{
    char *dest;

    dest = DEBMALLOC((SY6545_MESSAGE_SIZE+1)*sizeof(char));

    sprintf(dest,"      ===== SY6545 Status Summary =====      " "\n"
                 "                                             " "\n"
                 " Addr Reg: %02x           " "|" " Vert blank:    %d   "  "\n"
                 " Stat Reg: %02x           " "|" " Horiz blank:   %d   "  "\n"
                 " R00: %02x (HTotal-1)     " "|" " Lpen loaded:   %d   "  "\n"
                 " R01: %02x (HDisplay)     " "|" " Update done:   %d   "  "\n"
                 " R02: %02x (HSyncPos)     " "|" " Cursor mode:   %02x  " "\n"
                 " R03: %02x (SyncSize)     " "|" " Line Length:   %02x  " "\n"
                 " R04: %02x (VTotal-1)     " "|" " row/col mode:  %d   "  "\n"
                 " R05: %02x (VAdjRast)     " "|" " Trans mem on:  %d   "  "\n"
                 " R06: %02x (VDisplay)     " "|" " RA4 is strb:   %d   "  "\n"
                 " R07: %02x (VSyncPos)     " "|" " Update interl: %d   "  "\n"
                 " R08: %02x (Mode)         " "|" " Display Addr:  %04x"   "\n"
                 " R09: %02x (CharHgt-1)    " "|" " Cursor Addr:   %04x"   "\n"
                 " R0A: %02x (CursorBeg)    " "|" " Lpen Addr:     %04x"   "\n"
                 " R0B: %02x (CursorEnd)    " "|" " Update Addr:   %04x"   "\n"
                 " R0C: %02x (DispAdd)h     " "|" "                    "   "\n"
                 " R0D: %02x (DispAdd)l     " "|" "                    "   "\n"
                 " R0E: %02x (CursAdd)h     " "|" "                    "   "\n"
                 " R0F: %02x (CursAdd)l     " "|" "                    "   "\n"
                 " R10: %02x (LpenAdd)h     " "|" "                    "   "\n"
                 " R11: %02x (LpenAdd)l     " "|" "                    "   "\n"
                 " R12: %02x (UpdtAdd)h     " "|" "                    "   "\n"
                 " R13: %02x (UpdtAdd)l     " "|" "                    "   "\n"

                 ,RADDR(what)              ,(sy6545_VBLANK(what)             == 0x020)?1:0
                 ,(sy6545_VBLANK(what)|sy6545_HBLANK(what)|sy6545_LPEN_REGISTER_FULL(what)|sy6545_UPDATE_READY(what))  ,(sy6545_HBLANK(what) == 0x001)?1:0
                 ,R0_(what)                ,(sy6545_LPEN_REGISTER_FULL(what) == 0x040)?1:0
                 ,R1_(what)                ,(sy6545_UPDATE_READY(what)       == 0x080)?1:0
                 ,R2_(what)                ,R10_BB(what)
                 ,R3_(what)                ,R1_(what)
                 ,R4_(what)                ,(R8_(what) & 0x004)?1:0
                 ,R5_(what)                ,(R8_(what) & 0x008)?1:0
                 ,R6_(what)                ,(R8_(what) & 0x040)?1:0
                 ,R7_(what)                ,(R8_(what) & 0x080)?1:0
                 ,(R8_(what)|R8_ADM(what)) ,R12_13_(what)
                 ,R9_(what)                ,R14_15_(what)
                 ,R10_CS(what)             ,R16_17_(what)
                 ,R11_(what)               ,R18_19_(what)
                 ,R12_RA(what)
                 ,R13_CA(what)
                 ,R14_RA(what)
                 ,R15_CA(what)
                 ,R16_RA(what)
                 ,R17_CA(what)
                 ,R18_RA(what)
                 ,R19_CA(what));

    return dest;
}




/*
Function: sy6545_addr_wr(void)
Operation: Write to the 6545 address register.
*/

void sy6545_addr_wr(void *what)
{
    RADDR(what) = sy6545_COMMS_DATA_BUS_A(what) & 0x01F;

    return;
}




/*
Function: void sy6545_addr_rd(void)
Operation: Read from the 6545 address register.
*/

void sy6545_addr_rd(void *what)
{
    sy6545_COMMS_DATA_BUS_C(what) = sy6545_VBLANK(what) | sy6545_LPEN_REGISTER_FULL(what) | sy6545_UPDATE_READY(what);

    return;
}





/*
Function: sy6545_data_wr(void)
Operation: Write to the 6545 data register.
*/

void sy6545_data_wr(void *what)
{
    UINT_8 i;

    switch ( RADDR(what) )
    {
        case 0x000:
        {
            if ( R0_(what) != sy6545_COMMS_DATA_BUS_B(what) )
            {
                R0_(what) = sy6545_COMMS_DATA_BUS_B(what);

                SET_C6545_LEFT_MARGIN(what);
                sy6545_GEOMETRY_BUS_L(what) = sy6545_LEFT_MARGIN(what);
                sy6545_CHANGE_LEFT_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x001:
        {
            if ( R1_(what) != sy6545_COMMS_DATA_BUS_B(what) )
            {
                R1_(what) = sy6545_COMMS_DATA_BUS_B(what);

                SET_C6545_SCREEN_WIDTH(what);
                sy6545_GEOMETRY_BUS_W(what) = sy6545_SCREEN_WIDTH(what);
                sy6545_CHANGE_SCREEN_WIDTH(what);

                SET_C6545_RIGHT_MARGIN(what);
                sy6545_GEOMETRY_BUS_R(what) = sy6545_RIGHT_MARGIN(what);
                sy6545_CHANGE_RIGHT_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;

                sy6545_fix_coordspoint(what);
            }

            break;
        }

        case 0x002:
        {
            if ( R2_(what) != sy6545_COMMS_DATA_BUS_B(what) )
            {
                R2_(what) = sy6545_COMMS_DATA_BUS_B(what);

                SET_C6545_LEFT_MARGIN(what);
                sy6545_GEOMETRY_BUS_L(what) = sy6545_LEFT_MARGIN(what);
                sy6545_CHANGE_LEFT_MARGIN(what);

                SET_C6545_RIGHT_MARGIN(what);
                sy6545_GEOMETRY_BUS_R(what) = sy6545_RIGHT_MARGIN(what);
                sy6545_CHANGE_RIGHT_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x003:
        {
            R3_(what) = sy6545_COMMS_DATA_BUS_B(what);

            i = sy6545_COMMS_DATA_BUS_B(what) & 0x00F;

            if ( R3_H(what) != i )
            {
                R3_H(what) = i;

                SET_C6545_TOP_MARGIN(what);
                sy6545_GEOMETRY_BUS_T(what) = sy6545_TOP_MARGIN(what);
                sy6545_CHANGE_TOP_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            i = ( sy6545_COMMS_DATA_BUS_B(what) >> 4 ) & 0x00F;

            if ( i > 0 ) { i--;       }
            else         { i = 0x00F; }

            if ( R3_V(what) != i )
            {
                R3_V(what) = i;

                SET_C6545_LEFT_MARGIN(what);
                sy6545_GEOMETRY_BUS_L(what) = sy6545_LEFT_MARGIN(what);
                sy6545_CHANGE_LEFT_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x004:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x07F;

            if ( R4_(what) != i )
            {
                R4_(what) = i;

                SET_C6545_TOP_MARGIN(what);
                sy6545_GEOMETRY_BUS_T(what) = sy6545_TOP_MARGIN(what);
                sy6545_CHANGE_TOP_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x005:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x01F;

            if ( R5_(what) != i )
            {
                R5_(what) = i;

                SET_C6545_TOP_MARGIN(what);
                sy6545_GEOMETRY_BUS_T(what) = sy6545_TOP_MARGIN(what);
                sy6545_CHANGE_TOP_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x006:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x07F;

            if ( R6_(what) != i )
            {
                R6_(what) = i;

                SET_C6545_SCREEN_HEIGHT(what);
                sy6545_GEOMETRY_BUS_H(what) = sy6545_SCREEN_HEIGHT(what);
                sy6545_CHANGE_SCREEN_HEIGHT(what);

                SET_C6545_BOTTOM_MARGIN(what);
                sy6545_GEOMETRY_BUS_B(what) = sy6545_BOTTOM_MARGIN(what);
                sy6545_CHANGE_BOTTOM_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;

                sy6545_fix_coordspoint(what);
            }

            break;
        }

        case 0x007:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x07F;

            if ( R7_(what) != i )
            {
                R7_(what) = i;

                SET_C6545_TOP_MARGIN(what);
                sy6545_GEOMETRY_BUS_T(what) = sy6545_TOP_MARGIN(what);
                sy6545_CHANGE_TOP_MARGIN(what);

                SET_C6545_BOTTOM_MARGIN(what);
                sy6545_GEOMETRY_BUS_B(what) = sy6545_BOTTOM_MARGIN(what);
                sy6545_CHANGE_BOTTOM_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x008:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x004;

            if ( R8_ADM(what) != i )
            {
                R8_ADM(what) = i;

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            R8_CSK(what) = sy6545_COMMS_DATA_BUS_B(what) & 0x030;
            R8_(what)    = sy6545_COMMS_DATA_BUS_B(what) & 0x0FC;

            sy6545_fix_coordspoint(what);

            break;
        }

        case 0x009:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x01F;

            if ( R9_(what) != i )
            {
                R9_(what) = i;

                SET_C6545_TOP_MARGIN(what);
                sy6545_GEOMETRY_BUS_T(what) = sy6545_TOP_MARGIN(what);
                sy6545_CHANGE_TOP_MARGIN(what);

                SET_C6545_SCREEN_HEIGHT(what);
                sy6545_GEOMETRY_BUS_H(what) = sy6545_SCREEN_HEIGHT(what);
                sy6545_CHANGE_SCREEN_HEIGHT(what);

                SET_C6545_BOTTOM_MARGIN(what);
                sy6545_GEOMETRY_BUS_B(what) = sy6545_BOTTOM_MARGIN(what);
                sy6545_CHANGE_BOTTOM_MARGIN(what);

                sy6545_REDRAW_BIT(what) |= 0x02;
            }

            break;
        }

        case 0x00A:
        {
            R10_CS(what) = sy6545_COMMS_DATA_BUS_B(what) & 0x01F;
            R10_BB(what) = ( sy6545_COMMS_DATA_BUS_B(what) >> 5 ) & 0x003;

            break;
        }

        case 0x00B:
        {
            R11_(what) = sy6545_COMMS_DATA_BUS_B(what) & 0x01F;

            break;
        }

        case 0x00C:
        {
            i = sy6545_COMMS_DATA_BUS_B(what) & 0x03F;

            if ( R12_RA(what) != i )
            {
                R12_RA(what)  = i;
                R12_13_(what) = ( R12_RA(what) << 8 ) | R13_CA(what);

                sy6545_REDRAW_BIT(what) |= 0x02;

                sy6545_fix_coordspoint(what);
            }

            break;
        }

        case 0x00D:
        {
            if ( R13_CA(what) != sy6545_COMMS_DATA_BUS_B(what) )
            {
                R13_CA(what)  = sy6545_COMMS_DATA_BUS_B(what);
                R12_13_(what) = ( R12_RA(what) << 8 ) | R13_CA(what);

                sy6545_REDRAW_BIT(what) |= 0x02;

                sy6545_fix_coordspoint(what);
            }

            break;
        }

        case 0x00E:
        {
            R14_RA(what)  = sy6545_COMMS_DATA_BUS_B(what) & 0x03F;
            R14_15_(what) = ( R14_RA(what) << 8 ) | R15_CA(what);

            break;
        }

        case 0x00F:
        {
            R15_CA(what)  = sy6545_COMMS_DATA_BUS_B(what);
            R14_15_(what) = ( R14_RA(what) << 8 ) | R15_CA(what);

            break;
        }

        case 0x012:
        {
            R18_RA(what)  = sy6545_COMMS_DATA_BUS_B(what) & 0x03F;
            R18_19_(what) = ( R18_RA(what) << 8 ) | R19_CA(what);

            break;
        }

        case 0x013:
        {
            R19_CA(what)  = sy6545_COMMS_DATA_BUS_B(what);
            R18_19_(what) = ( R18_RA(what) << 8 ) | R19_CA(what);

            break;
        }

        case 0x01F:
        {
            sy6545_UPDATE_READY(what) = 0;

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}





/*
Function: void sy6545_data_rd(void)
Operation: Read from the 6545 data register.
*/

void sy6545_data_rd(void *what)
{
    sy6545_COMMS_DATA_BUS_D(what) = 0;

    switch ( RADDR(what) )
    {
        case 0x00E:
        {
            sy6545_COMMS_DATA_BUS_D(what) = R14_RA(what);

            break;
        }

        case 0x00F:
        {
            sy6545_COMMS_DATA_BUS_D(what) = R15_CA(what);

            break;
        }

        case 0x010:
        {
            sy6545_LPEN_REGISTER_FULL(what) = 0;

            sy6545_COMMS_DATA_BUS_D(what) = R16_RA(what);

            (sy6545_LPEN_RESET_COUNTER_BUS(what))++;

            break;
        }

        case 0x011:
        {
            sy6545_LPEN_REGISTER_FULL(what) = 0;

            sy6545_COMMS_DATA_BUS_D(what) = R17_CA(what);

            (sy6545_LPEN_RESET_COUNTER_BUS(what))++;

            break;
        }

        case 0x01F:
        {
            sy6545_UPDATE_READY(what) = 0;

            (sy6545_UPDATE_RESET_COUNTER_BUS(what))++;

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}






/*
Function: void sy6545_write_char_mem(void)
Operation: Write to character memory
*/

void sy6545_write_char_mem(void *what)
{
    UINT_32 i;
    C6545_vdu_point *temp_vdu;

    /*
       Do nothing unless something has changed.
    */

    if ( sy6545_VIDEO_DATA_BUS_C(what) != (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_C(what)]).lines)[sy6545_VIDEO_CHAR_LINE_BUS(what)] )
    {
        sy6545_XPOS_BUS(what) = sy6545_VIDEO_MEM_ADDR_BUS_C(what);
        sy6545_YPOS_BUS(what) = sy6545_VIDEO_CHAR_LINE_BUS(what);

        (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_C(what)]).lines)[sy6545_VIDEO_CHAR_LINE_BUS(what)] = sy6545_VIDEO_DATA_BUS_C(what);

        i   = 1;
        i <<= sy6545_VIDEO_CHAR_LINE_BUS(what);

        temp_vdu = ((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_C(what)]).first_occur;

        /*
           Go through list of screen positions where this character can
           be found and set the line mask bit of each appropriately.
        */

        while ( temp_vdu != NULL )
        {
            temp_vdu->line_change_mask |= i;

            temp_vdu = temp_vdu->next_same_char;
        }
    }

    return;
}








/*
Function: void sy6545_write_vdu_mem(void)
Operation: Write to VDU memory
*/

void sy6545_write_vdu_mem(void *what)
{
    /*
       Do nothing unless the data has changed.
    */

    if ( sy6545_VIDEO_DATA_BUS_V(what) != ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num )
    {
        sy6545_XPOS_BUS(what) = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_x_coord;
        sy6545_YPOS_BUS(what) = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_y_coord;

        (sy6545_SCN_MAP(what))[sy6545_XPOS_BUS(what)][sy6545_YPOS_BUS(what)] = sy6545_VIDEO_DATA_BUS_V(what);

        /* update the update mask */

        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x000] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x000] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000001; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x001] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x001] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000002; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x002] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x002] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000004; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x003] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x003] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000008; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x004] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x004] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000010; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x005] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x005] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000020; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x006] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x006] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000040; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x007] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x007] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000080; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x008] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x008] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000100; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x009] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x009] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000200; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x00a] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x00a] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000400; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x00b] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x00b] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000000800; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x00c] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x00c] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000001000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x00d] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x00d] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000002000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x00e] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x00e] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000004000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x00f] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x00f] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000008000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x010] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x010] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000010000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x011] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x011] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000020000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x012] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x012] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000040000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x013] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x013] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000080000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x014] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x014] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000100000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x015] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x015] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000200000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x016] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x016] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000400000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x017] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x017] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x000800000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x018] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x018] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x001000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x019] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x019] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x002000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x01a] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x01a] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x004000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x01b] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x01b] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x008000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x01c] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x01c] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x010000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x01d] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x01d] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x020000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x01e] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x01e] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x040000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).lines)[0x01f] != (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).lines)[0x01f] ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).line_change_mask |= 0x080000000; }

        /* remove from current char linked list */

        if ( ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).prev_same_char == NULL )
        {
            if ( ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char == NULL )
            {
                /* only one in list */

                ((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).first_occur = NULL;
            }

            else
            {
                /* first one in list */

                ((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num]).first_occur = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char;
                (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char)->prev_same_char = NULL;
            }
        }

        else
        {
            if ( ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char == NULL )
            {
                /* last one in list */

                (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).prev_same_char)->next_same_char = NULL;
            }

            else
            {
                /* somewhere in middle of list */

                (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).prev_same_char)->next_same_char = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char;
                (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char)->prev_same_char = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).prev_same_char;
            }
        }

        ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).char_num = sy6545_VIDEO_DATA_BUS_V(what);

        /* put at start of new list */

        if ( ((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).first_occur == NULL )
        {
            /* start new list */

            ((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).first_occur = &((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]);

            ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).prev_same_char = NULL;
            ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char = NULL;
        }

        else
        {
            /* add to start of existing list */

            (((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).first_occur)->prev_same_char = &((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]);

            ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).next_same_char = ((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).first_occur;
            ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]).prev_same_char = NULL;

            ((sy6545_CHAR_MEMORY(what))[sy6545_VIDEO_DATA_BUS_V(what)]).first_occur = &((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_V(what)]);
        }
    }

    return;
}




/*
Function: void sy6545_write_fore_colour(void)
Operation: Write to foreground colour memory
*/

void sy6545_write_fore_colour(void *what)
{
    if ( sy6545_VIDEO_DATA_BUS_FC(what) != (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).fore_colour)[0] )
    {
        sy6545_XPOS_BUS(what) = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_x_coord;
        sy6545_YPOS_BUS(what) = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_y_coord;
        (sy6545_COL_MAP(what))[sy6545_XPOS_BUS(what)][sy6545_YPOS_BUS(what)][0] = sy6545_VIDEO_DATA_BUS_FC(what);


        ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).fore_colour[0] = sy6545_VIDEO_DATA_BUS_FC(what);
        ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).back_colour[1] = sy6545_VIDEO_DATA_BUS_FC(what);

        /* update the update mask */

        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x000] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000001; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x001] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000002; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x002] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000004; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x003] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000008; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x004] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000010; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x005] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000020; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x006] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000040; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x007] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000080; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x008] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000100; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x009] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000200; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x00a] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000400; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x00b] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000000800; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x00c] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000001000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x00d] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000002000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x00e] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000004000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x00f] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000008000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x010] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000010000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x011] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000020000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x012] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000040000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x013] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000080000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x014] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000100000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x015] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000200000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x016] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000400000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x017] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x000800000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x018] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x001000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x019] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x002000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x01a] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x004000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x01b] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x008000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x01c] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x010000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x01d] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x020000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x01e] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x040000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).char_num]).lines)[0x01f] != 0x000 ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_FC(what)]).line_change_mask |= 0x080000000; }
    }

    return;
}







/*
Function: void sy6545_write_back_colour(void)
Operation: Write to background colour memory
*/

void sy6545_write_back_colour(void *what)
{
    if ( sy6545_VIDEO_DATA_BUS_BC(what) != (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).back_colour)[0] )
    {
        sy6545_XPOS_BUS(what) = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_x_coord;
        sy6545_YPOS_BUS(what) = ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_y_coord;
        (sy6545_COL_MAP(what))[sy6545_XPOS_BUS(what)][sy6545_YPOS_BUS(what)][1] = sy6545_VIDEO_DATA_BUS_BC(what);


        (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).back_colour)[0] = sy6545_VIDEO_DATA_BUS_BC(what);
        (((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).fore_colour)[1] = sy6545_VIDEO_DATA_BUS_BC(what);

        /* update the update mask */

        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x000] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000001; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x001] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000002; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x002] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000004; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x003] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000008; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x004] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000010; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x005] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000020; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x006] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000040; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x007] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000080; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x008] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000100; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x009] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000200; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x00a] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000400; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x00b] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000000800; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x00c] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000001000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x00d] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000002000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x00e] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000004000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x00f] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000008000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x010] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000010000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x011] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000020000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x012] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000040000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x013] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000080000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x014] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000100000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x015] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000200000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x016] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000400000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x017] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x000800000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x018] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x001000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x019] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x002000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x01a] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x004000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x01b] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x008000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x01c] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x010000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x01d] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x020000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x01e] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x040000000; }
        if ( (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).char_num]).lines)[0x01f] != 0x0FF ) { ((sy6545_VDU_MEMORY(what))[sy6545_VIDEO_MEM_ADDR_BUS_BC(what)]).line_change_mask |= 0x080000000; }
    }

    return;
}









void sy6545_fix_coordspoint(module_data *what)
{
    UINT_32 i,k;
    UINT_8 x,y;

    for ( i = 0x00000 ; i < C6545_VDU_MEM_SIZE ; i++ )
    {
        ((sy6545_VDU_MEMORY(what))[i]).char_x_coord = 0;
        ((sy6545_VDU_MEMORY(what))[i]).char_y_coord = 0;

        ((sy6545_VDU_MEMORY(what))[i]).char_pointer = NULL;
        ((sy6545_VDU_MEMORY(what))[i]).back_pointer = NULL;
        ((sy6545_VDU_MEMORY(what))[i]).fore_pointer = NULL;
    }

    if ( !R8_ADM(what) )
    {
        /*
           linear display mode
        */

        for ( i = 0x00000 ; i < R1_(what)*R6_(what) ; i++ )
        {
            k = ( i + R12_13_(what) ) & 0x0FFFF;

            x = (UINT_8) ( i % R1_(what) );
            y = (UINT_8) ( i / R1_(what) );

            ((sy6545_VDU_MEMORY(what))[k]).char_x_coord = x;
            ((sy6545_VDU_MEMORY(what))[k]).char_y_coord = y;

            ((sy6545_VDU_MEMORY(what))[k]).char_pointer = &((sy6545_SCN_MAP(what))[x][y]);
            ((sy6545_VDU_MEMORY(what))[k]).back_pointer = &((sy6545_COL_MAP(what))[x][y][0]);
            ((sy6545_VDU_MEMORY(what))[k]).fore_pointer = &((sy6545_COL_MAP(what))[x][y][1]);

            *(((sy6545_VDU_MEMORY(what))[k]).char_pointer) = (((sy6545_VDU_MEMORY(what))[k]).char_num);
            *(((sy6545_VDU_MEMORY(what))[k]).back_pointer) = (((sy6545_VDU_MEMORY(what))[k]).back_colour)[0];
            *(((sy6545_VDU_MEMORY(what))[k]).fore_pointer) = (((sy6545_VDU_MEMORY(what))[k]).fore_colour)[0];
        }
    }

    else
    {
        /*
           row/column mode
        */

        for ( y = 0x000 ; y < R6_(what) ; y++ )
        {
            for ( x = 0x000 ; x < R1_(what) ; x++ )
            {
                k = ( ( ( y * 0x0100 ) + x ) + R12_13_(what) ) & 0x0FFFF;

                ((sy6545_VDU_MEMORY(what))[k]).char_x_coord = x;
                ((sy6545_VDU_MEMORY(what))[k]).char_y_coord = y;

                ((sy6545_VDU_MEMORY(what))[k]).char_pointer = &((sy6545_SCN_MAP(what))[x][y]);
                ((sy6545_VDU_MEMORY(what))[k]).back_pointer = &((sy6545_COL_MAP(what))[x][y][0]);
                ((sy6545_VDU_MEMORY(what))[k]).fore_pointer = &((sy6545_COL_MAP(what))[x][y][1]);

                *(((sy6545_VDU_MEMORY(what))[k]).char_pointer) = (((sy6545_VDU_MEMORY(what))[k]).char_num);
                *(((sy6545_VDU_MEMORY(what))[k]).back_pointer) = (((sy6545_VDU_MEMORY(what))[k]).back_colour)[0];
                *(((sy6545_VDU_MEMORY(what))[k]).fore_pointer) = (((sy6545_VDU_MEMORY(what))[k]).fore_colour)[0];
            }
        }
    }

    return;
}



/*
Function: sy6545_cycle()
Operation: see modules.h
*/

/*
   Note: Assumed here that carry from column to row is allowed

   Note: In the latch_lpen stuff above, where R16_17_ is loaded, the specs
         say that this is set according to the contents of the "internal
         scan counter" on the next negative going clock edge after the
         lpen transition (low-high) occurs.  I've followed this, and it
         seems to work for the MicroBee.
         BUT: the diagram in same datasheet (SY6545) clearly indicates
              that the contents of MA_bus should be loaded on the negative
              going clock edge.  This would be sy6545_MA_bus.  The fact
              that the dispen based update lasts 3 clock cycles also
              indicates that this should be true (ie. update register
              loaded, not the scan address).  I've chosen the internal bus
              method, but I'm not entirely convinced either way.

   Update counters for 1 clock cycle.  In list of priority, the counters
   are:

        sy6545_horiz_char_count: ranges from 0 to R0_
        sy6545_vert_scan_count:  ranges from 0 to R9_ (R5_-1 if sy6545_vert_char_count == R4_+1)
        sy6545_vert_char_count:  ranges from 0 to R4_ (R4_+1 if R5_ > 0)
        sy6545_frame_count:      ranges from 0 to 63

   there is also the sy6545_vert_sync_count, as described elsewhere.
*/

#define INCR_UPD_ADDR                                                   \
{                                                                       \
    R18_19_(what) = ( ( R18_19_(what) + 1 ) & 0x03FFF );                \
    R19_CA(what)  = R18_19_(what) & 0x0FF;                              \
    R18_RA(what)  = (R18_19_(what) >> 8) & 0x03F;                       \
}


/*
For speed reasons, the do_cycles while loop is enclosed in a switch statement
to avoid too many repetetive decisions inside the loop.  Thus, rather than
one big loop full of small decisions, there are many small loops only one
of which will be excecuted.  This is done with macros.
Additionally, putting the dispen condition outside most of the operations
(so there are separate macros for dispen and overscan/flyback) resulted in
a significant speedup.
*/

/*
FIRST_PART_DO_CYCLE: This part of the loop works out the horizonal blanking,
                     vertical blanking, display enable, vertical sync and
                     horizontal sync signals, as well as the default state
                     of the CR bus.

                     vblank = 0x000 or 0x020
                     hblank = 0x000 or 0x001
                     dispen = 0x000 or 0x008

AMID_PART_DO_CYCLE: This masks (and modifies if CR4 is an update strobe) the
                    CR bus, and modifies the MA buses if required.  It also
                    sets the update state (internal) of the 6545 and the
                    update strobing.

BMID_PART_DO_CYCLE: set sy6545_line_up_mask (bit mask, 1 bit set,
                    indicating that the relevant line is being drawn.

CMID_PART_DO_CYCLE: Ignoring the position on the screen, work out if the
                    cursor is on or off for this frame.

DMID_PART_DO_CYCLE: Ignoring the position on the screen, work out if the
                    cursor is on or off for this frame.
*/



#define GENERIC_SETUP_PART                                              \
                                                                        \
    sy6545_DISPEN(what) = ( ~( ( sy6545_VBLANK(what) >> 2 ) | ( sy6545_HBLANK(what) << 3 ) ) ) & 0x008; \
                                                                        \
    sy6545_LPEN_FEEDBACK_ADDR_TYPE_CLW(what) = 0;                       \
    sy6545_LPEN_FEEDBACK_ADDR_TYPE(what)     = 0;

#define GENERIC_FINAL_PART                                              \
                                                                        \
    if ( sy6545_LATCH_LPEN(what) )                                      \
    {                                                                   \
        sy6545_LATCH_LPEN(what) = 0;                                    \
                                                                        \
        *sy6545_LPEN_ADDR_BUS_SEL(what) = sy6545_LPEN_FEEDBACK_ADDR(what); \
                                                                        \
        sy6545_FEEDx_INCER_dx_SEL(what);                                \
                                                                        \
        R16_17_(what) = 0x000;                                          \
        R16_RA(what)  = 0x000;                                          \
        R17_CA(what)  = 0x000;                                          \
                                                                        \
        sy6545_LPEN_REGISTER_FULL(what) = 0x040;                        \
    }                                                                   \
                                                                        \
    if ( ( sy6545_PREVIOUS_LPEN(what) < sy6545_IS_LPEN_CLOW(what) ) ||   \
         ( sy6545_IS_LPEN_CLOW(what)  < sy6545_IS_LPEN(what)      )    ) \
    {                                                                   \
        sy6545_LATCH_LPEN(what) = 1;                                    \
    }                                                                   \
                                                                        \
    sy6545_PREVIOUS_LPEN(what)      = sy6545_IS_LPEN(what);             \
    sy6545_PREVIOUS_LPEN_CLOW(what) = sy6545_IS_LPEN_CLOW(what);        \
                                                                        \
    if ( ++sy6545_HORIZ_CHAR_COUNT(what) == R1_(what) )                 \
    {                                                                   \
        sy6545_HBLANK(what) = 0x001;                                    \
    }                                                                   \
                                                                        \
    if ( sy6545_HORIZ_CHAR_COUNT(what) == R2_(what) )                   \
    {                                                                   \
        sy6545_IS_HSYNC(what) = 0x004;                                  \
    }                                                                   \
                                                                        \
    else if ( sy6545_HORIZ_CHAR_COUNT(what) == R2_(what)+R3_H(what) )   \
    {                                                                   \
        sy6545_IS_HSYNC(what) = 0x000;                                  \
    }                                                                   \
                                                                        \
    if ( sy6545_HORIZ_CHAR_COUNT(what) > R0_(what) )                    \
    {                                                                   \
        sy6545_IS_HSYNC(what) = 0x000;                                  \
        sy6545_HBLANK(what)   = 0x000;                                  \
                                                                        \
        sy6545_HORIZ_CHAR_COUNT(what) = 0;                              \
                                                                        \
        if ( ( ( sy6545_VERT_CHAR_COUNT(what) == R7_(what) ) && ( sy6545_VERT_SCAN_COUNT(what) == 0 ) ) ||     \
               ( sy6545_VERT_SYNC_COUNT(what) > 0                                                    )      ) \
        {                                                               \
            sy6545_VERT_SYNC_COUNT(what)++;                             \
            sy6545_IS_VSYNC(what) = 0x002;                              \
                                                                        \
            if ( sy6545_VERT_SYNC_COUNT(what) > R3_V(what) )            \
            {                                                           \
                sy6545_VERT_SYNC_COUNT(what) = 0;                       \
                sy6545_IS_VSYNC(what) = 0x000;                          \
            }                                                           \
        }                                                               \
                                                                        \
        sy6545_VERT_SCAN_COUNT(what)++;                                 \
                                                                        \
        if ( ( ( sy6545_VERT_CHAR_COUNT(what) <= R4_(what)   ) && ( sy6545_VERT_SCAN_COUNT(what) > R9_(what)   ) ) ||   \
             ( ( sy6545_VERT_CHAR_COUNT(what) == R4_(what)+1 ) && ( sy6545_VERT_SCAN_COUNT(what) > R5_(what)-1 ) )    ) \
        {                                                               \
            sy6545_VERT_SCAN_COUNT(what) = 0;                           \
                                                                        \
            if ( ++sy6545_VERT_CHAR_COUNT(what) == R6_(what) )          \
            {                                                           \
                sy6545_VBLANK(what) = 0x020;                            \
            }                                                           \
                                                                        \
            if ( ( ( sy6545_VERT_CHAR_COUNT(what) >  R4_(what)+1 )                           ) ||   \
                 ( ( sy6545_VERT_CHAR_COUNT(what) == R4_(what)+1 ) && ( R5_(what) ==     0 ) )    ) \
            {                                                           \
                sy6545_VBLANK(what) = 0x000;                            \
                                                                        \
                sy6545_VERT_CHAR_COUNT(what) = 0;                       \
                                                                        \
                if ( sy6545_REDRAW_BIT(what) & 0x03 )                   \
                {                                                       \
                    if ( sy6545_REDRAW_BIT(what) & 0x02 )               \
                    {                                                   \
                        sy6545_REDRAW_BIT(what) &= 0x04;                \
                        sy6545_REDRAW_BIT(what) |= 0x01;                \
                    }                                                   \
                                                                        \
                    else if ( sy6545_REDRAW_BIT(what) & 0x01 )          \
                    {                                                   \
                        sy6545_REDRAW_BIT(what) &= 0x04;                \
                    }                                                   \
                }                                                       \
                                                                        \
                sy6545_FRAME_COUNT(what) += clock_div;                  \
                                                                        \
                if ( sy6545_FRAME_COUNT(what) > 63 )                    \
                {                                                       \
                    sy6545_FRAME_COUNT(what) = 0;                       \
                }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    num_cycles--;






#define FIRST_PART_DO_CYCLE_LINEAR(CR_mask)                             \
                                                                        \
    sy6545_CR_BUS_CLOW(what) = sy6545_VERT_SCAN_COUNT(what) & CR_mask;  \
    sy6545_CR_BUS(what)      = sy6545_CR_BUS_CLOW(what);                \
                                                                        \
    sy6545_MA_BUS_NO_UPDATE(what) = ((sy6545_VERT_CHAR_COUNT(what)*R1_(what))+sy6545_HORIZ_CHAR_COUNT(what)+R12_13_(what)) & 0x03FFF; \
                                                                        \
    sy6545_MA_BUS_CLOW(what) = sy6545_MA_BUS_NO_UPDATE(what);           \
    sy6545_MA_BUS(what)      = sy6545_MA_BUS_NO_UPDATE(what);

#define FIRST_PART_DO_CYCLE_ROWCOL(CR_mask)                             \
                                                                        \
    sy6545_CR_BUS_CLOW(what) = sy6545_VERT_SCAN_COUNT(what) & CR_mask;  \
    sy6545_CR_BUS(what)      = sy6545_CR_BUS_CLOW(what);                \
                                                                        \
    sy6545_MA_BUS_NO_UPDATE(what) = ((sy6545_VERT_CHAR_COUNT(what)<<8)+sy6545_HORIZ_CHAR_COUNT(what)+R12_13_(what)) & 0x03FFF; \
                                                                        \
    sy6545_MA_BUS_CLOW(what) = sy6545_MA_BUS_NO_UPDATE(what);           \
    sy6545_MA_BUS(what)      = sy6545_MA_BUS_NO_UPDATE(what);

#define AMID_DO_NOTHING

#define AMID_PART_DO_CYCLE001xq                                         \
                                                                        \
    if ( sy6545_UPDATE_DISPEN_COUNT(what) )                             \
    {                                                                   \
        sy6545_MA_BUS(what) = R18_19_(what);                            \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) = 1;                       \
                                                                        \
        switch ( sy6545_UPDATE_DISPEN_COUNT(what) )                     \
        {                                                               \
            case 1:                                                     \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 2;                   \
                                                                        \
                break;                                                  \
            }                                                           \
                                                                        \
            default:                                                    \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 0;                   \
                                                                        \
                sy6545_UPDATE_READY(what) = 0x080;                      \
                                                                        \
                INCR_UPD_ADDR;                                          \
                                                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
    }

#define AMID_PART_DO_CYCLE011xq                                         \
                                                                        \
    if ( sy6545_UPDATE_DISPEN_COUNT(what) )                             \
    {                                                                   \
        sy6545_MA_BUS(what) = R18_19_(what);                            \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) = 1;                       \
                                                                        \
        switch ( sy6545_UPDATE_DISPEN_COUNT(what) )                     \
        {                                                               \
            case 1:                                                     \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 2;                   \
                                                                        \
                sy6545_CR_BUS(what) |= 0x010;                           \
                                                                        \
                break;                                                  \
            }                                                           \
                                                                        \
            default:                                                    \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 0;                   \
                                                                        \
                sy6545_UPDATE_READY(what) = 0x080;                      \
                                                                        \
                INCR_UPD_ADDR;                                          \
                                                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
    }

#define AMID_PART_DO_CYCLE101xq                                         \
                                                                        \
    if ( !sy6545_UPDATE_READY(what) )                                   \
    {                                                                   \
        sy6545_MA_BUS(what) = R18_19_(what);                            \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) = 1;                       \
                                                                        \
        sy6545_UPDATE_READY(what) = 0x080;                              \
                                                                        \
        INCR_UPD_ADDR;                                                  \
    }

#define AMID_PART_DO_CYCLE111xq                                         \
                                                                        \
    if ( !sy6545_UPDATE_READY(what) )                                   \
    {                                                                   \
        sy6545_MA_BUS(what) = R18_19_(what);                            \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) = 1;                       \
                                                                        \
        sy6545_UPDATE_READY(what) = 0x080;                              \
                                                                        \
        INCR_UPD_ADDR;                                                  \
                                                                        \
        sy6545_CR_BUS(what) |= 0x010;                                   \
    }

#define BMID_DO_NOTHING

#define BMID_PART_DO_CYCLE                                              \
                                                                        \
    sy6545_IS_CURSOR(what) = 0;                                         \
                                                                        \
    sy6545_LINE_UP_MASK(what) = 1 << sy6545_VERT_SCAN_COUNT(what);


#define CMID_DO_NOTHING

#define CMID_PART_DO_CYCLE00q                                           \
                                                                        \
    {

#define CMID_PART_DO_CYCLE10q                                           \
                                                                        \
    if ( sy6545_FRAME_COUNT(what) & 0x010 )                             \
    {

#define CMID_PART_DO_CYCLE11q                                           \
                                                                        \
    if ( sy6545_FRAME_COUNT(what) & 0x020 )                             \
    {

#define DMID_DO_NOTHING

#define DMID_PART_DO_CYCLExxq                                           \
                                                                        \
        if ( sy6545_MA_BUS_NO_UPDATE(what) == R14_15_(what) )           \
        {                                                               \
            if ( R11_(what) >= R10_CS(what) )                           \
            {                                                           \
                if ( ( sy6545_VERT_SCAN_COUNT(what) >= R10_CS(what) ) &&   \
                     ( sy6545_VERT_SCAN_COUNT(what) <= R11_(what)   )    ) \
                {                                                       \
                    sy6545_IS_CURSOR(what) = 1;                         \
                                                                        \
                    (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).line_change_mask) |= sy6545_LINE_UP_MASK(what); \
                }                                                       \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                if ( ( sy6545_VERT_SCAN_COUNT(what) <= R10_CS(what) ) ||   \
                     ( sy6545_VERT_SCAN_COUNT(what) >= R11_(what)   )    ) \
                {                                                       \
                    sy6545_IS_CURSOR(what) = 0;                         \
                }                                                       \
            }                                                           \
        }                                                               \
    }

#define DMID_PART_DO_CYCLE01q                                           \
                                                                        \
        if ( ( sy6545_MA_BUS_NO_UPDATE(what) == R14_15_(what)-1 ) && ( sy6545_HORIZ_CHAR_COUNT(what) > 0 ) ) \
        {                                                               \
            if ( R11_(what) >= R10_CS(what) )                           \
            {                                                           \
                if ( ( sy6545_VERT_SCAN_COUNT(what) >= R10_CS(what) ) &&   \
                     ( sy6545_VERT_SCAN_COUNT(what) <= R11_(what)   )    ) \
                {                                                       \
                    sy6545_IS_CURSOR(what) = 1;                         \
                                                                        \
                    (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).line_change_mask) |= sy6545_LINE_UP_MASK(what); \
                }                                                       \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                if ( ( sy6545_VERT_SCAN_COUNT(what) <= R10_CS(what) ) ||   \
                     ( sy6545_VERT_SCAN_COUNT(what) >= R11_(what)   )    ) \
                {                                                       \
                    sy6545_IS_CURSOR(what) = 0;                         \
                }                                                       \
            }                                                           \
        }                                                               \
    }

#define DMID_PART_DO_CYCLE10q                                           \
                                                                        \
        if ( ( sy6545_MA_BUS_NO_UPDATE(what) == R14_15_(what)+1 ) && ( sy6545_HORIZ_CHAR_COUNT(what) < R0_(what) ) ) \
        {                                                               \
            if ( R11_(what) >= R10_CS(what) )                           \
            {                                                           \
                if ( ( sy6545_VERT_SCAN_COUNT(what) >= R10_CS(what) ) &&   \
                     ( sy6545_VERT_SCAN_COUNT(what) <= R11_(what)   )    ) \
                {                                                       \
                    sy6545_IS_CURSOR(what) = 1;                         \
                                                                        \
                    (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).line_change_mask) |= sy6545_LINE_UP_MASK(what); \
                }                                                       \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                if ( ( sy6545_VERT_SCAN_COUNT(what) <= R10_CS(what) ) ||   \
                     ( sy6545_VERT_SCAN_COUNT(what) >= R11_(what)   )    ) \
                {                                                       \
                    sy6545_IS_CURSOR(what) = 0;                         \
                }                                                       \
            }                                                           \
        }                                                               \
    }

#define LAST_PART_DO_CYCLE                                              \
                                                                        \
    if ( ( (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).line_change_mask) & sy6545_LINE_UP_MASK(what) ) || sy6545_REDRAW_BIT(what) ) \
    {                                                                   \
        sy6545_FORE_COLOUR_BUS(what) = (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).fore_colour)[sy6545_IS_CURSOR(what)]; \
        sy6545_BACK_COLOUR_BUS(what) = (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).back_colour)[sy6545_IS_CURSOR(what)]; \
        sy6545_INV__COLOUR_BUS(what) = sy6545_IS_CURSOR(what);          \
                                                                        \
        sy6545_XPOS_BUS(what) = sy6545_HORIZ_CHAR_COUNT(what) << 3;  \
        sy6545_YPOS_BUS(what) = ((((UINT_16) R9_(what))+1)*sy6545_VERT_CHAR_COUNT(what))+sy6545_VERT_SCAN_COUNT(what); \
                                                                        \
        sy6545_IS_FORE_BUS(what) = (((sy6545_CHAR_MEMORY(what))[((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).char_num]).lines)[sy6545_VERT_SCAN_COUNT(what)]; \
                                                                        \
        sy6545_PIXEL_DRAWER(what);                                      \
                                                                        \
        if ( !sy6545_IS_CURSOR(what) )                                  \
        {                                                               \
            sy6545_LINE_UP_MASK(what) ^= 0x0FFFFFFFF;                   \
                                                                        \
            (((sy6545_VDU_MEMORY(what))[sy6545_MA_BUS_CLOW(what)]).line_change_mask) &= sy6545_LINE_UP_MASK(what); \
        }                                                               \
    }                                                                   \
                                                                        \
    sy6545_IS_LPEN_CLOW(what) = 0;                                      \
    sy6545_IS_LPEN(what)      = 0;                                      \
                                                                        \
    if ( (1<<((sy6545_CR_BUS(what)      & 0x010)|sy6545_DISPEN(what)|sy6545_IS_HSYNC(what)|sy6545_IS_VSYNC(what)|sy6545_IS_CURSOR(what))) & sy6545_LPEN_CALL_MASK_BUS(what) ) \
    {                                                                   \
        sy6545_LPEN_ADDR_BUS(what)      = sy6545_MA_BUS(what);          \
        sy6545_POLE_LPEN_TABLE_READ(what);                              \
        sy6545_IS_LPEN(what)      = sy6545_LPEN_DATA_BUS(what);         \
                                                                        \
        if ( sy6545_IS_LPEN(what) )                                     \
        {                                                               \
            sy6545_LPEN_FEEDBACK_ADDR(what) = sy6545_MA_BUS(what);      \
                                                                        \
            if ( sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) )                 \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDRFSH_INCER_dxfn(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDRFSH_INCER_dxargs(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_RFSH(what);    \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDBACK_INCER_dxfn(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDBACK_INCER_dxargs(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_BACK(what);    \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    if ( (1<<((sy6545_CR_BUS_CLOW(what) & 0x010)|sy6545_DISPEN(what)|sy6545_IS_HSYNC(what)|sy6545_IS_VSYNC(what)|sy6545_IS_CURSOR(what))) & sy6545_LPEN_CALL_MASK_BUS(what) ) \
    {                                                                   \
        sy6545_LPEN_ADDR_BUS_CLOW(what) = sy6545_MA_BUS_CLOW(what);     \
        sy6545_POLE_LPEN_TABLE_READ_CLOW(what);                         \
        sy6545_IS_LPEN_CLOW(what) = sy6545_LPEN_DATA_BUS_CLOW(what);    \
                                                                        \
        if ( sy6545_IS_LPEN_CLOW(what))                                 \
        {                                                               \
            sy6545_LPEN_FEEDBACK_ADDR(what) = sy6545_MA_BUS_CLOW(what); \
                                                                        \
            if ( sy6545_LPEN_FEEDBACK_ADDR_TYPE_CLW(what) )             \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDRFSH_INCER_dxfn_CLOW(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDRFSH_INCER_dxargs_CLOW(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_RFSH_CLOW(what);    \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDBACK_INCER_dxfn_CLOW(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDBACK_INCER_dxargs_CLOW(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_BACK_CLOW(what);    \
            }                                                           \
        }                                                               \
    }

/*
NOTE: the ordering of the above (first lpen_table, then lpen_clow_table) is
      important (although not in the microbee context).  The 6545 cycle is
      first clock_low, then clock_high.  Now, we want to capture the address
      of the *earliest* low-high transition of the lpen pin and store the
      address corresponding to this in sy6545_LPEN_FEEDBACK_ADDR(what).  The
      first event is the clow event, hence we put it second so that the
      value written will override later events.
*/




#define FIRST_PART_DO_CYCLE_LINEAR_NODISP(CR_mask)                      \
                                                                        \
    sy6545_CR_BUS_CLOW(what) = 0x000;                                   \
    sy6545_CR_BUS(what)      = 0x000;                                   \
                                                                        \
    sy6545_MA_BUS_CLOW(what) = 0x000;                                   \
    sy6545_MA_BUS(what)      = 0x000;

#define FIRST_PART_DO_CYCLE_ROWCOL_NODISP(CR_mask)                      \
                                                                        \
    sy6545_CR_BUS_CLOW(what) = 0x000;                                   \
    sy6545_CR_BUS(what)      = 0x000;                                   \
                                                                        \
    sy6545_MA_BUS_CLOW(what) = 0x000;                                   \
    sy6545_MA_BUS(what)      = 0x000;

#define AMID_DO_NOTHING_NODISP

#define AMID_PART_DO_CYCLE001xq_NODISP                                  \
                                                                        \
    if ( !sy6545_UPDATE_READY(what) || sy6545_UPDATE_DISPEN_COUNT(what) ) \
    {                                                                   \
        sy6545_MA_BUS_CLOW(what) = R18_19_(what);                       \
        sy6545_MA_BUS(what)      = R18_19_(what);                       \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE_CLW(what) = 1;                   \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what)     = 1;                   \
                                                                        \
        switch ( sy6545_UPDATE_DISPEN_COUNT(what) )                     \
        {                                                               \
            case 0:                                                     \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 1;                   \
                                                                        \
                break;                                                  \
            }                                                           \
                                                                        \
            case 1:                                                     \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 2;                   \
                                                                        \
                break;                                                  \
            }                                                           \
                                                                        \
            default:                                                    \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 0;                   \
                                                                        \
                sy6545_UPDATE_READY(what) = 0x080;                      \
                                                                        \
                INCR_UPD_ADDR;                                          \
                                                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
    }

#define AMID_PART_DO_CYCLE011xq_NODISP                                  \
                                                                        \
    if ( !sy6545_UPDATE_READY(what) || sy6545_UPDATE_DISPEN_COUNT(what) ) \
    {                                                                   \
        sy6545_MA_BUS_CLOW(what) = R18_19_(what);                       \
        sy6545_MA_BUS(what)      = R18_19_(what);                       \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE_CLW(what) = 1;                   \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what)     = 1;                   \
                                                                        \
        switch ( sy6545_UPDATE_DISPEN_COUNT(what) )                     \
        {                                                               \
            case 0:                                                     \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 1;                   \
                                                                        \
                break;                                                  \
            }                                                           \
                                                                        \
            case 1:                                                     \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 2;                   \
                                                                        \
                sy6545_CR_BUS_CLOW(what) = 0x010;                       \
                sy6545_CR_BUS(what)      = 0x010;                       \
                                                                        \
                break;                                                  \
            }                                                           \
                                                                        \
            default:                                                    \
            {                                                           \
                sy6545_UPDATE_DISPEN_COUNT(what) = 0;                   \
                                                                        \
                sy6545_UPDATE_READY(what) = 0x080;                      \
                                                                        \
                INCR_UPD_ADDR;                                          \
                                                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
    }

#define AMID_PART_DO_CYCLE101xq_NODISP                                  \
                                                                        \
    if ( !sy6545_UPDATE_READY(what) )                                   \
    {                                                                   \
        sy6545_MA_BUS(what) = R18_19_(what);                            \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) = 1;                       \
                                                                        \
        sy6545_UPDATE_READY(what) = 0x080;                              \
                                                                        \
        INCR_UPD_ADDR;                                                  \
    }

#define AMID_PART_DO_CYCLE111xq_NODISP                                  \
                                                                        \
    if ( !sy6545_UPDATE_READY(what) )                                   \
    {                                                                   \
        sy6545_MA_BUS(what) = R18_19_(what);                            \
                                                                        \
        sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) = 1;                       \
                                                                        \
        sy6545_UPDATE_READY(what) = 0x080;                              \
                                                                        \
        INCR_UPD_ADDR;                                                  \
                                                                        \
        sy6545_CR_BUS(what) = 0x010;                                     \
    }

#define BMID_DO_NOTHING_NODISP
#define BMID_PART_DO_CYCLE_NODISP

#define CMID_DO_NOTHING_NODISP
#define CMID_PART_DO_CYCLE00q_NODISP
#define CMID_PART_DO_CYCLE10q_NODISP
#define CMID_PART_DO_CYCLE11q_NODISP

#define DMID_DO_NOTHING_NODISP
#define DMID_PART_DO_CYCLExxq_NODISP
#define DMID_PART_DO_CYCLE01q_NODISP
#define DMID_PART_DO_CYCLE10q_NODISP

#define LAST_PART_DO_CYCLE_NODISP                                       \
                                                                        \
    sy6545_IS_LPEN_CLOW(what) = 0;                                      \
    sy6545_IS_LPEN(what)      = 0;                                      \
                                                                        \
    if ( (1<<(sy6545_CR_BUS(what)     |sy6545_IS_HSYNC(what)|sy6545_IS_VSYNC(what))) & sy6545_LPEN_CALL_MASK_BUS(what) ) \
    {                                                                   \
        sy6545_LPEN_ADDR_BUS(what)      = sy6545_MA_BUS(what);          \
        sy6545_POLE_LPEN_TABLE_READ(what);                              \
        sy6545_IS_LPEN(what)      = sy6545_LPEN_DATA_BUS(what);         \
                                                                        \
        if ( sy6545_IS_LPEN(what) )                                     \
        {                                                               \
            sy6545_LPEN_FEEDBACK_ADDR(what) = sy6545_MA_BUS(what);      \
                                                                        \
            if ( sy6545_LPEN_FEEDBACK_ADDR_TYPE(what) )                 \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDRFSH_INCER_dxfn(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDRFSH_INCER_dxargs(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_RFSH(what);    \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDBACK_INCER_dxfn(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDBACK_INCER_dxargs(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_BACK(what);    \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    if ( (1<<(sy6545_CR_BUS_CLOW(what)|sy6545_IS_HSYNC(what)|sy6545_IS_VSYNC(what))) & sy6545_LPEN_CALL_MASK_BUS(what) ) \
    {                                                                   \
        sy6545_LPEN_ADDR_BUS_CLOW(what) = sy6545_MA_BUS_CLOW(what);     \
        sy6545_POLE_LPEN_TABLE_READ_CLOW(what);                         \
        sy6545_IS_LPEN_CLOW(what) = sy6545_LPEN_DATA_BUS_CLOW(what);    \
                                                                        \
        if ( sy6545_IS_LPEN_CLOW(what) )                                \
        {                                                               \
            sy6545_LPEN_FEEDBACK_ADDR(what) = sy6545_MA_BUS_CLOW(what); \
                                                                        \
            if ( sy6545_LPEN_FEEDBACK_ADDR_TYPE_CLW(what) )             \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDRFSH_INCER_dxfn_CLOW(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDRFSH_INCER_dxargs_CLOW(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_RFSH_CLOW(what);    \
            }                                                           \
                                                                        \
            else                                                        \
            {                                                           \
                sy6545_FEEDx_INCER_dxfn_SEL(what)   = sy6545_FEEDBACK_INCER_dxfn_CLOW(what);   \
                sy6545_FEEDx_INCER_dxargs_SEL(what) = sy6545_FEEDBACK_INCER_dxargs_CLOW(what); \
                sy6545_LPEN_ADDR_BUS_SEL(what)      = sy6545_LPEN_ADDR_BUS_BACK_CLOW(what);    \
            }                                                           \
        }                                                               \
    }                                                                   \

void sy6545_do_cycles_00(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_01(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_02(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_03(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_04(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_05(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_06(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_07(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_08(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_09(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_0a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_0b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_0c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_0d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_0e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_0f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_10(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_11(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_12(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_13(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_14(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_15(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_16(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_17(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_18(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_19(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_1a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_1b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_1c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_1d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_1e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_1f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_20(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_21(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_22(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_23(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_24(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_25(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_26(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_27(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_28(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_29(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_2a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_2b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_2c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_2d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_2e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_2f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_30(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_31(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_32(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_33(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_34(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_35(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_36(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_37(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_38(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_39(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_3a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_3b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_3c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_3d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_3e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_3f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_40(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_41(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_42(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_43(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_44(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_45(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_46(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_47(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_48(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_49(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_4a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_4b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_4c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_4d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_4e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_4f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_50(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_51(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_52(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_53(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_54(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_55(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_56(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_57(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_58(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_59(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_5a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_5b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_5c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_5d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_5e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_5f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_60(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_61(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_62(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_63(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_64(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_65(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_66(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_67(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_68(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_69(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_6a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_6b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_6c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_6d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_6e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_6f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_70(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_71(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_72(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_73(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_74(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_75(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_76(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);
void sy6545_do_cycles_77(module_data *what, UINT_16 num_cycles, UINT_8 clock_div);



void sy6545_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    switch ( R8_(what) | R10_BB(what) )
    {
        case 0x000: /* 00xx00xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x080: /* 10xx00xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x030: /* 00xx00xx */ /* xx11xxxx */ /* xxxxxx00 */
        case 0x0B0: /* 10xx00xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_00(what,num_cycles,clock_div);

            break;
        }

        case 0x010: /* 00xx00xx */ /* xx01xxxx */ /* xxxxxx00 */
        case 0x090: /* 10xx00xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_01(what,num_cycles,clock_div);

            break;
        }

        case 0x020: /* 00xx00xx */ /* xx10xxxx */ /* xxxxxx00 */
        case 0x0A0: /* 10xx00xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_02(what,num_cycles,clock_div);

            break;
        }

        case 0x040: /* 01xx00xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x0C0: /* 11xx00xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x070: /* 01xx00xx */ /* xx11xxxx */ /* xxxxxx00 */
        case 0x0F0: /* 11xx00xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_03(what,num_cycles,clock_div);

            break;
        }

        case 0x050: /* 01xx00xx */ /* xx01xxxx */ /* xxxxxx00 */
        case 0x0D0: /* 11xx00xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_04(what,num_cycles,clock_div);

            break;
        }

        case 0x060: /* 01xx00xx */ /* xx10xxxx */ /* xxxxxx00 */
        case 0x0E0: /* 11xx00xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_05(what,num_cycles,clock_div);

            break;
        }

        case 0x004: /* 00xx01xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x084: /* 10xx01xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x034: /* 00xx01xx */ /* xx11xxxx */ /* xxxxxx00 */
        case 0x0B4: /* 10xx01xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_06(what,num_cycles,clock_div);

            break;
        }

        case 0x014: /* 00xx01xx */ /* xx01xxxx */ /* xxxxxx00 */
        case 0x094: /* 10xx01xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_07(what,num_cycles,clock_div);

            break;
        }

        case 0x024: /* 00xx01xx */ /* xx10xxxx */ /* xxxxxx00 */
        case 0x0A4: /* 10xx01xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_08(what,num_cycles,clock_div);

            break;
        }

        case 0x044: /* 01xx01xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x0C4: /* 11xx01xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x074: /* 01xx01xx */ /* xx11xxxx */ /* xxxxxx00 */
        case 0x0F4: /* 11xx01xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_09(what,num_cycles,clock_div);

            break;
        }

        case 0x054: /* 01xx01xx */ /* xx01xxxx */ /* xxxxxx00 */
        case 0x0D4: /* 11xx01xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_0a(what,num_cycles,clock_div);

            break;
        }

        case 0x064: /* 01xx01xx */ /* xx10xxxx */ /* xxxxxx00 */
        case 0x0E4: /* 11xx01xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_0b(what,num_cycles,clock_div);

            break;
        }

        case 0x008: /* 00xx10xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x038: /* 00xx10xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_0c(what,num_cycles,clock_div);

            break;
        }

        case 0x018: /* 00xx10xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_0d(what,num_cycles,clock_div);

            break;
        }

        case 0x028: /* 00xx10xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_0e(what,num_cycles,clock_div);

            break;
        }

        case 0x048: /* 01xx10xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x078: /* 01xx10xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_0f(what,num_cycles,clock_div);

            break;
        }

        case 0x058: /* 01xx10xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_10(what,num_cycles,clock_div);

            break;
        }

        case 0x068: /* 01xx10xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_11(what,num_cycles,clock_div);

            break;
        }

        case 0x00C: /* 00xx11xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x03C: /* 00xx11xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_12(what,num_cycles,clock_div);

            break;
        }

        case 0x01C: /* 00xx11xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_13(what,num_cycles,clock_div);

            break;
        }

        case 0x02C: /* 00xx11xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_14(what,num_cycles,clock_div);

            break;
        }

        case 0x04C: /* 01xx11xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x07C: /* 01xx11xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_15(what,num_cycles,clock_div);

            break;
        }

        case 0x05C: /* 01xx11xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_16(what,num_cycles,clock_div);

            break;
        }

        case 0x06C: /* 01xx11xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_17(what,num_cycles,clock_div);

            break;
        }

        case 0x088: /* 10xx10xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x0B8: /* 10xx10xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_18(what,num_cycles,clock_div);

            break;
        }

        case 0x098: /* 10xx10xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_19(what,num_cycles,clock_div);

            break;
        }

        case 0x0A8: /* 10xx10xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_1a(what,num_cycles,clock_div);

            break;
        }

        case 0x0C8: /* 11xx10xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x0F8: /* 11xx10xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_1b(what,num_cycles,clock_div);

            break;
        }

        case 0x0D8: /* 11xx10xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_1c(what,num_cycles,clock_div);

            break;
        }

        case 0x0E8: /* 11xx10xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_1d(what,num_cycles,clock_div);

            break;
        }

        case 0x08C: /* 10xx11xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x0BC: /* 10xx11xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_1e(what,num_cycles,clock_div);

            break;
        }

        case 0x09C: /* 10xx11xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_1f(what,num_cycles,clock_div);

            break;
        }

        case 0x0AC: /* 10xx11xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_20(what,num_cycles,clock_div);

            break;
        }

        case 0x0CC: /* 11xx11xx */ /* xx00xxxx */ /* xxxxxx00 */
        case 0x0FC: /* 11xx11xx */ /* xx11xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_21(what,num_cycles,clock_div);

            break;
        }

        case 0x0DC: /* 11xx11xx */ /* xx01xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_22(what,num_cycles,clock_div);

            break;
        }

        case 0x0EC: /* 11xx11xx */ /* xx10xxxx */ /* xxxxxx00 */
        {
            sy6545_do_cycles_23(what,num_cycles,clock_div);

            break;
        }


        case 0x001: /* 00xx00xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x081: /* 10xx00xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x031: /* 00xx00xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x0B1: /* 10xx00xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x011: /* 00xx00xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x091: /* 10xx00xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x021: /* 00xx00xx */ /* xx10xxxx */ /* xxxxxx01 */
        case 0x0A1: /* 10xx00xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_24(what,num_cycles,clock_div);

            break;
        }

        case 0x041: /* 01xx00xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x0C1: /* 11xx00xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x071: /* 01xx00xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x0F1: /* 11xx00xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x051: /* 01xx00xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x0D1: /* 11xx00xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x061: /* 01xx00xx */ /* xx10xxxx */ /* xxxxxx01 */
        case 0x0E1: /* 11xx00xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_25(what,num_cycles,clock_div);

            break;
        }

        case 0x005: /* 00xx01xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x085: /* 10xx01xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x035: /* 00xx01xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x0B5: /* 10xx01xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x015: /* 00xx01xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x095: /* 10xx01xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x025: /* 00xx01xx */ /* xx10xxxx */ /* xxxxxx01 */
        case 0x0A5: /* 10xx01xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_26(what,num_cycles,clock_div);

            break;
        }

        case 0x045: /* 01xx01xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x0C5: /* 11xx01xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x075: /* 01xx01xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x0F5: /* 11xx01xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x055: /* 01xx01xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x0D5: /* 11xx01xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x065: /* 01xx01xx */ /* xx10xxxx */ /* xxxxxx01 */
        case 0x0E5: /* 11xx01xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_27(what,num_cycles,clock_div);

            break;
        }

        case 0x009: /* 00xx10xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x039: /* 00xx10xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x019: /* 00xx10xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x029: /* 00xx10xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_28(what,num_cycles,clock_div);

            break;
        }

        case 0x049: /* 01xx10xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x079: /* 01xx10xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x059: /* 01xx10xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x069: /* 01xx10xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_29(what,num_cycles,clock_div);

            break;
        }

        case 0x00D: /* 00xx11xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x03D: /* 00xx11xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x01D: /* 00xx11xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x02D: /* 00xx11xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_2a(what,num_cycles,clock_div);

            break;
        }

        case 0x04D: /* 01xx11xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x07D: /* 01xx11xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x05D: /* 01xx11xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x06D: /* 01xx11xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_2b(what,num_cycles,clock_div);

            break;
        }

        case 0x089: /* 10xx10xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x0B9: /* 10xx10xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x099: /* 10xx10xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x0A9: /* 10xx10xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_2c(what,num_cycles,clock_div);

            break;
        }

        case 0x0C9: /* 11xx10xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x0F9: /* 11xx10xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x0D9: /* 11xx10xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x0E9: /* 11xx10xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_2d(what,num_cycles,clock_div);

            break;
        }

        case 0x08D: /* 10xx11xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x0BD: /* 10xx11xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x09D: /* 10xx11xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x0AD: /* 10xx11xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_2e(what,num_cycles,clock_div);

            break;
        }

        case 0x0CD: /* 11xx11xx */ /* xx00xxxx */ /* xxxxxx01 */
        case 0x0FD: /* 11xx11xx */ /* xx11xxxx */ /* xxxxxx01 */
        case 0x0DD: /* 11xx11xx */ /* xx01xxxx */ /* xxxxxx01 */
        case 0x0ED: /* 11xx11xx */ /* xx10xxxx */ /* xxxxxx01 */
        {
            sy6545_do_cycles_2f(what,num_cycles,clock_div);

            break;
        }

        case 0x002: /* 00xx00xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x082: /* 10xx00xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x032: /* 00xx00xx */ /* xx11xxxx */ /* xxxxxx10 */
        case 0x0B2: /* 10xx00xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_30(what,num_cycles,clock_div);

            break;
        }

        case 0x012: /* 00xx00xx */ /* xx01xxxx */ /* xxxxxx10 */
        case 0x092: /* 10xx00xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_31(what,num_cycles,clock_div);

            break;
        }

        case 0x022: /* 00xx00xx */ /* xx10xxxx */ /* xxxxxx10 */
        case 0x0A2: /* 10xx00xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_32(what,num_cycles,clock_div);

            break;
        }

        case 0x042: /* 01xx00xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x0C2: /* 11xx00xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x072: /* 01xx00xx */ /* xx11xxxx */ /* xxxxxx10 */
        case 0x0F2: /* 11xx00xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_33(what,num_cycles,clock_div);

            break;
        }

        case 0x052: /* 01xx00xx */ /* xx01xxxx */ /* xxxxxx10 */
        case 0x0D2: /* 11xx00xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_34(what,num_cycles,clock_div);

            break;
        }

        case 0x062: /* 01xx00xx */ /* xx10xxxx */ /* xxxxxx10 */
        case 0x0E2: /* 11xx00xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_35(what,num_cycles,clock_div);

            break;
        }

        case 0x006: /* 00xx01xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x086: /* 10xx01xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x036: /* 00xx01xx */ /* xx11xxxx */ /* xxxxxx10 */
        case 0x0B6: /* 10xx01xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_36(what,num_cycles,clock_div);

            break;
        }

        case 0x016: /* 00xx01xx */ /* xx01xxxx */ /* xxxxxx10 */
        case 0x096: /* 10xx01xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_37(what,num_cycles,clock_div);

            break;
        }

        case 0x026: /* 00xx01xx */ /* xx10xxxx */ /* xxxxxx10 */
        case 0x0A6: /* 10xx01xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_38(what,num_cycles,clock_div);

            break;
        }

        case 0x046: /* 01xx01xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x0C6: /* 11xx01xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x076: /* 01xx01xx */ /* xx11xxxx */ /* xxxxxx10 */
        case 0x0F6: /* 11xx01xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_39(what,num_cycles,clock_div);

            break;
        }

        case 0x056: /* 01xx01xx */ /* xx01xxxx */ /* xxxxxx10 */
        case 0x0D6: /* 11xx01xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_3a(what,num_cycles,clock_div);

            break;
        }

        case 0x066: /* 01xx01xx */ /* xx10xxxx */ /* xxxxxx10 */
        case 0x0E6: /* 11xx01xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_3b(what,num_cycles,clock_div);

            break;
        }

        case 0x00A: /* 00xx10xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x03A: /* 00xx10xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_3c(what,num_cycles,clock_div);

            break;
        }

        case 0x01A: /* 00xx10xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_3d(what,num_cycles,clock_div);

            break;
        }

        case 0x02A: /* 00xx10xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_3e(what,num_cycles,clock_div);

            break;
        }

        case 0x04A: /* 01xx10xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x07A: /* 01xx10xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_3f(what,num_cycles,clock_div);

            break;
        }

        case 0x05A: /* 01xx10xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_40(what,num_cycles,clock_div);

            break;
        }

        case 0x06A: /* 01xx10xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_41(what,num_cycles,clock_div);

            break;
        }

        case 0x00E: /* 00xx11xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x03E: /* 00xx11xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_42(what,num_cycles,clock_div);

            break;
        }

        case 0x01E: /* 00xx11xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_43(what,num_cycles,clock_div);

            break;
        }

        case 0x02E: /* 00xx11xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_44(what,num_cycles,clock_div);

            break;
        }

        case 0x04E: /* 01xx11xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x07E: /* 01xx11xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_45(what,num_cycles,clock_div);

            break;
        }

        case 0x05E: /* 01xx11xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_46(what,num_cycles,clock_div);

            break;
        }

        case 0x06E: /* 01xx11xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_47(what,num_cycles,clock_div);

            break;
        }

        case 0x08A: /* 10xx10xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x0BA: /* 10xx10xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_48(what,num_cycles,clock_div);

            break;
        }

        case 0x09A: /* 10xx10xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_49(what,num_cycles,clock_div);

            break;
        }

        case 0x0AA: /* 10xx10xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_4a(what,num_cycles,clock_div);

            break;
        }

        case 0x0CA: /* 11xx10xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x0FA: /* 11xx10xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_4b(what,num_cycles,clock_div);

            break;
        }

        case 0x0DA: /* 11xx10xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_4c(what,num_cycles,clock_div);

            break;
        }

        case 0x0EA: /* 11xx10xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_4d(what,num_cycles,clock_div);

            break;
        }

        case 0x08E: /* 10xx11xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x0BE: /* 10xx11xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_4e(what,num_cycles,clock_div);

            break;
        }

        case 0x09E: /* 10xx11xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_4f(what,num_cycles,clock_div);

            break;
        }

        case 0x0AE: /* 10xx11xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_50(what,num_cycles,clock_div);

            break;
        }

        case 0x0CE: /* 11xx11xx */ /* xx00xxxx */ /* xxxxxx10 */
        case 0x0FE: /* 11xx11xx */ /* xx11xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_51(what,num_cycles,clock_div);

            break;
        }

        case 0x0DE: /* 11xx11xx */ /* xx01xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_52(what,num_cycles,clock_div);

            break;
        }

        case 0x0EE: /* 11xx11xx */ /* xx10xxxx */ /* xxxxxx10 */
        {
            sy6545_do_cycles_53(what,num_cycles,clock_div);

            break;
        }


        case 0x003: /* 00xx00xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x083: /* 10xx00xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x033: /* 00xx00xx */ /* xx11xxxx */ /* xxxxxx11 */
        case 0x0B3: /* 10xx00xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_54(what,num_cycles,clock_div);

            break;
        }

        case 0x013: /* 00xx00xx */ /* xx01xxxx */ /* xxxxxx11 */
        case 0x093: /* 10xx00xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_55(what,num_cycles,clock_div);

            break;
        }

        case 0x023: /* 00xx00xx */ /* xx10xxxx */ /* xxxxxx11 */
        case 0x0A3: /* 10xx00xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_56(what,num_cycles,clock_div);

            break;
        }

        case 0x043: /* 01xx00xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x0C3: /* 11xx00xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x073: /* 01xx00xx */ /* xx11xxxx */ /* xxxxxx11 */
        case 0x0F3: /* 11xx00xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_57(what,num_cycles,clock_div);

            break;
        }

        case 0x053: /* 01xx00xx */ /* xx01xxxx */ /* xxxxxx11 */
        case 0x0D3: /* 11xx00xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_58(what,num_cycles,clock_div);

            break;
        }

        case 0x063: /* 01xx00xx */ /* xx10xxxx */ /* xxxxxx11 */
        case 0x0E3: /* 11xx00xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_59(what,num_cycles,clock_div);

            break;
        }

        case 0x007: /* 00xx01xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x087: /* 10xx01xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x037: /* 00xx01xx */ /* xx11xxxx */ /* xxxxxx11 */
        case 0x0B7: /* 10xx01xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_5a(what,num_cycles,clock_div);

            break;
        }

        case 0x017: /* 00xx01xx */ /* xx01xxxx */ /* xxxxxx11 */
        case 0x097: /* 10xx01xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_5b(what,num_cycles,clock_div);

            break;
        }

        case 0x027: /* 00xx01xx */ /* xx10xxxx */ /* xxxxxx11 */
        case 0x0A7: /* 10xx01xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_5c(what,num_cycles,clock_div);

            break;
        }

        case 0x047: /* 01xx01xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x0C7: /* 11xx01xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x077: /* 01xx01xx */ /* xx11xxxx */ /* xxxxxx11 */
        case 0x0F7: /* 11xx01xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_5d(what,num_cycles,clock_div);

            break;
        }

        case 0x057: /* 01xx01xx */ /* xx01xxxx */ /* xxxxxx11 */
        case 0x0D7: /* 11xx01xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_5e(what,num_cycles,clock_div);

            break;
        }

        case 0x067: /* 01xx01xx */ /* xx10xxxx */ /* xxxxxx11 */
        case 0x0E7: /* 11xx01xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_5f(what,num_cycles,clock_div);

            break;
        }

        case 0x00B: /* 00xx10xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x03B: /* 00xx10xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_60(what,num_cycles,clock_div);

            break;
        }

        case 0x01B: /* 00xx10xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_61(what,num_cycles,clock_div);

            break;
        }

        case 0x02B: /* 00xx10xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_62(what,num_cycles,clock_div);

            break;
        }

        case 0x04B: /* 01xx10xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x07B: /* 01xx10xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_63(what,num_cycles,clock_div);

            break;
        }

        case 0x05B: /* 01xx10xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_64(what,num_cycles,clock_div);

            break;
        }

        case 0x06B: /* 01xx10xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_65(what,num_cycles,clock_div);

            break;
        }

        case 0x00F: /* 00xx11xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x03F: /* 00xx11xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_66(what,num_cycles,clock_div);

            break;
        }

        case 0x01F: /* 00xx11xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_67(what,num_cycles,clock_div);

            break;
        }

        case 0x02F: /* 00xx11xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_68(what,num_cycles,clock_div);

            break;
        }

        case 0x04F: /* 01xx11xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x07F: /* 01xx11xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_69(what,num_cycles,clock_div);

            break;
        }

        case 0x05F: /* 01xx11xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_6a(what,num_cycles,clock_div);

            break;
        }

        case 0x06F: /* 01xx11xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_6b(what,num_cycles,clock_div);

            break;
        }

        case 0x08B: /* 10xx10xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x0BB: /* 10xx10xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_6c(what,num_cycles,clock_div);

            break;
        }

        case 0x09B: /* 10xx10xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_6d(what,num_cycles,clock_div);

            break;
        }

        case 0x0AB: /* 10xx10xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_6e(what,num_cycles,clock_div);

            break;
        }

        case 0x0CB: /* 11xx10xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x0FB: /* 11xx10xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_6f(what,num_cycles,clock_div);

            break;
        }

        case 0x0DB: /* 11xx10xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_70(what,num_cycles,clock_div);

            break;
        }

        case 0x0EB: /* 11xx10xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_71(what,num_cycles,clock_div);

            break;
        }

        case 0x08F: /* 10xx11xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x0BF: /* 10xx11xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_72(what,num_cycles,clock_div);

            break;
        }

        case 0x09F: /* 10xx11xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_73(what,num_cycles,clock_div);

            break;
        }

        case 0x0AF: /* 10xx11xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_74(what,num_cycles,clock_div);

            break;
        }

        case 0x0CF: /* 11xx11xx */ /* xx00xxxx */ /* xxxxxx11 */
        case 0x0FF: /* 11xx11xx */ /* xx11xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_75(what,num_cycles,clock_div);

            break;
        }

        case 0x0DF: /* 11xx11xx */ /* xx01xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_76(what,num_cycles,clock_div);

            break;
        }

        default: /* 11xx11xx */ /* xx10xxxx */ /* xxxxxx11 */
        {
            sy6545_do_cycles_77(what,num_cycles,clock_div);

            break;
        }
    }

    return;

    lsync_point = 0;
}

void sy6545_do_cycles_00(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_01(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_02(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_03(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_04(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_05(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_06(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_07(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_08(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_09(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_0a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_0b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_0c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_0d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_0e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_0f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_10(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_11(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_12(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_13(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_14(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_15(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_16(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_17(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_18(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_19(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_1a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_1b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_1c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_1d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_1e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_1f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_20(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_21(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_22(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_23(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE00q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE00q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_24(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_25(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_26(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_27(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_28(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_29(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_2a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_2b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_2c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_2d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_2e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_2f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_DO_NOTHING
            DMID_DO_NOTHING
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_DO_NOTHING_NODISP
            DMID_DO_NOTHING_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_30(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_31(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_32(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_33(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_34(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_35(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_36(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_37(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_38(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_39(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_3a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_3b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_3c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_3d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_3e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_3f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_40(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_41(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_42(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_43(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_44(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_45(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_46(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_47(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_48(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_49(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_4a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_4b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_4c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_4d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_4e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_4f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_50(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_51(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_52(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_53(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE10q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE10q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_54(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_55(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_56(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_57(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_58(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_59(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_5a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_5b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_5c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_5d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_5e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_5f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_DO_NOTHING
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_DO_NOTHING_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_60(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_61(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_62(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_63(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_64(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_65(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_66(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_67(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_68(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE001xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE001xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_69(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_6a(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_6b(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE011xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE011xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_6c(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_6d(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_6e(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_6f(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_70(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_71(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_LINEAR(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_LINEAR_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_72(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_73(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_74(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x01F)
            AMID_PART_DO_CYCLE101xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x01F)
            AMID_PART_DO_CYCLE101xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_75(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLExxq
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLExxq_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_76(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE01q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE01q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}

void sy6545_do_cycles_77(module_data *what, UINT_16 num_cycles, UINT_8 clock_div)
{
    while ( num_cycles > 0 )
    {
        GENERIC_SETUP_PART

        if ( sy6545_DISPEN(what) )
        {
            FIRST_PART_DO_CYCLE_ROWCOL(0x00F)
            AMID_PART_DO_CYCLE111xq
            BMID_PART_DO_CYCLE
            CMID_PART_DO_CYCLE11q
            DMID_PART_DO_CYCLE10q
            LAST_PART_DO_CYCLE
        }

        else
        {
            FIRST_PART_DO_CYCLE_ROWCOL_NODISP(0x00F)
            AMID_PART_DO_CYCLE111xq_NODISP
            BMID_PART_DO_CYCLE_NODISP
            CMID_PART_DO_CYCLE11q_NODISP
            DMID_PART_DO_CYCLE10q_NODISP
            LAST_PART_DO_CYCLE_NODISP
        }

        GENERIC_FINAL_PART
    }

    return;
}
