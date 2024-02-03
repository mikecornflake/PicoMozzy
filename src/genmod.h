
#include "u_dtype.h"
#include "modules.h"

#ifndef _genmod_h
#define _genmod_h

/*

                           Generic Modules
                           ===============

Structural module 1: null
=========================

This module the dev/null module.  There is one incoming function provided,
which does nothing.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: none
16 bit buses: none
32 bit buses: none

incoming functions: infn0 - do nothing
outgoing functions: none

Module is unclocked.


Structural module 2: do
=======================

When called, will call n other functions.

8  bit variables: none
16 bit variables: none
32 bit variables: varc0 - number of outfn's (default 2)

module pointers: none

8  bit buses: none
16 bit buses: none
32 bit buses: none

incoming functions: infn0 - initiate call sequence
outgoing functions: outfn0     - first function called in response to infn0.
                    outfn1     - second function called in response to infn0.
                    ...
                    outfn(n-1) - nth function called in response to infn0.
                                 (n is set by varc0 at initialisation time).

Module is unclocked.


Structural module 3: dowhile
============================

When called, will call n other functions.  It will then test to see if
busa0, busb0 or busc0 is nonzero, and repeat if this is true.

8  bit variables: none
16 bit variables: none
32 bit variables: varc0 - number of outfn's (default 2)

module pointers: none

8  bit buses: busa0 - 8  bit condition bus
16 bit buses: busb0 - 16 bit condition bus
32 bit buses: busc0 - 32 bit condition bus

incoming functions: infn0  - initiate dowhile loop using busa0.
                    infn1  - initiate dowhile loop using busb0.
                    infn2  - initiate dowhile loop using busc0.
outgoing functions: outfn0     - first function called in response to infnx.
                    outfn1     - second function called in response to infnx.
                    ...
                    outfn(n-1) - nth function called in response to infnx.
                                 (n is set by varc0 at initialisation time).

Module is unclocked.


Structural module 4: table8
===========================

When called, will call one of 256 possible functions.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - selects which function will be called.
16 bit buses: busb0 - selects which function will be called.
32 bit buses: busc0 - selects which function will be called.

incoming functions: infn0  - initiate call sequence using busa0
                    infn1  - initiate call sequence using busb0
                    infn2  - initiate call sequence using busc0
outgoing functions: outfn0 - function called if busx = 0
                    outfn1 - function called if busx = 1
                    outfn2 - function called if busx = 2
                    ...
                    outfn255 - function called if busx = 255

Module is unclocked.


Structural module 5: lut8
=========================

When called, will call set busa0, busb0 or busc0 to the value of the LUT
given by busa1, where the lut is maintained in vara0, vara1, ... vara255

8  bit variables: LUT goes here (256 elements)
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - bus to be set by infn0.
              busa1 - selector (ranges from 0 to 255).
16 bit buses: busb0 - bus to be set by infn1.
32 bit buses: busc0 - bus to be set by infn2.

incoming functions: infn0 - cause busa0 to be set according to busa1
                    infn1 - cause busb0 to be set according to busa1
                    infn2 - cause busc0 to be set according to busa1
outgoing functions: none

Module is unclocked.


Functional module 1: bus
========================

Basic "bus" module.  Provides 3 buses.

8  bit variables: vara0 - 8-bit  reset type: 0 = reset has no effect.
                                             1 = reset to random.
                                             2 = reset to vara3 (default).
                  vara1 - 16-bit reset type: 0 = reset has no effect.
                                             1 = reset to random.
                                             2 = reset to varb0 (default).
                  vara2 - 32-bit reset type: 0 = reset has no effect.
                                             1 = reset to random.
                                             2 = reset to varc0 (default).
                  vara3 - reset value for 8-bit  (if vara0 = 2) (default 0).
16 bit variables: varb0 - reset value for 16-bit (if vara1 = 2) (default 0).
32 bit variables: varc0 - reset value for 32-bit (if vara2 = 2) (default 0).

module pointers: none

8  bit buses: (out) busa0 - 8-bit  bus
16 bit buses: (out) busb0 - 16-bit bus
32 bit buses: (out) busc0 - 32-bit bus

incoming functions: infn0   - set busa0 = 0
                    infn1   - set busa0 = 1
                    infn2   - set busa0 = 2
                    ...
                    infn255 - set busa0 = 255
                    infn256 - reset function.
                    infn257 - increment busa0
                    infn258 - decrement busa0
                    infn259 - sr busa0 (msb = 0)
                    infn260 - sl busa0 (lsb = 0)
                    infn261 - rr busa0 (msb = lsb)
                    infn262 - rl busa0 (lsb = msb)
                    infn263 - increment busb0
                    infn264 - decrement busb0
                    infn265 - sr busb0 (msb = 0)
                    infn266 - sl busb0 (lsb = 0)
                    infn267 - rr busb0 (msb = lsb)
                    infn268 - rl busb0 (lsb = msb)
                    infn269 - increment busc0
                    infn270 - decrement busc0
                    infn271 - sr busc0 (msb = 0)
                    infn272 - sl busc0 (lsb = 0)
                    infn273 - rr busc0 (msb = lsb)
                    infn274 - rl busc0 (lsb = msb)
                    infn275 - set bit 0 of busa0
                    infn276 - set bit 1 of busa0
                    infn277 - set bit 2 of busa0
                    infn278 - set bit 3 of busa0
                    infn279 - set bit 4 of busa0
                    infn280 - set bit 5 of busa0
                    infn281 - set bit 6 of busa0
                    infn282 - set bit 7 of busa0
                    infn283 - set bit 0 of busb0
                    infn284 - set bit 1 of busb0
                    infn285 - set bit 2 of busb0
                    infn286 - set bit 3 of busb0
                    infn287 - set bit 4 of busb0
                    infn288 - set bit 5 of busb0
                    infn289 - set bit 6 of busb0
                    infn290 - set bit 7 of busb0
                    infn291 - set bit 8 of busb0
                    infn292 - set bit 9 of busb0
                    infn293 - set bit a of busb0
                    infn294 - set bit b of busb0
                    infn295 - set bit c of busb0
                    infn296 - set bit d of busb0
                    infn297 - set bit e of busb0
                    infn298 - set bit f of busb0
                    infn299 - set bit 00 of busc0
                    infn300 - set bit 01 of busc0
                    infn301 - set bit 02 of busc0
                    infn302 - set bit 03 of busc0
                    infn303 - set bit 04 of busc0
                    infn304 - set bit 05 of busc0
                    infn305 - set bit 06 of busc0
                    infn306 - set bit 07 of busc0
                    infn307 - set bit 08 of busc0
                    infn308 - set bit 09 of busc0
                    infn309 - set bit 0a of busc0
                    infn310 - set bit 0b of busc0
                    infn311 - set bit 0c of busc0
                    infn312 - set bit 0d of busc0
                    infn313 - set bit 0e of busc0
                    infn314 - set bit 0f of busc0
                    infn315 - set bit 10 of busc0
                    infn316 - set bit 11 of busc0
                    infn317 - set bit 12 of busc0
                    infn318 - set bit 13 of busc0
                    infn319 - set bit 14 of busc0
                    infn320 - set bit 15 of busc0
                    infn321 - set bit 16 of busc0
                    infn322 - set bit 17 of busc0
                    infn323 - set bit 18 of busc0
                    infn324 - set bit 19 of busc0
                    infn325 - set bit 1a of busc0
                    infn326 - set bit 1b of busc0
                    infn327 - set bit 1c of busc0
                    infn328 - set bit 1d of busc0
                    infn329 - set bit 1e of busc0
                    infn330 - set bit 1f of busc0
outgoing functions: none

Module is unclocked.


Functional module 2: mem
========================

To use as a ROM module, the "name" string should be of the format:

module_name:filename

where module_name will be trimmed and used as is, and the contents of the
module will be loaded from the file "filename".  Note that this will cause
var1 to default to 0 (reset has no effect) rather than 2.


8  bit variables: vara0 reset type: 0 = reset has no effect.
                                    1 = reset to random.
                                    2 = reset to vara1 (default).
                  vara1 reset value (if vara0 = 2) (default 0).
16 bit variables: none
32 bit variables: varc0 memory size-1 (NB assumed to be 2^n -1) (default 0).
                  varc1 memory mask used (ANDed with address) (default 0).

string variables: svar0 if nonempty, the memory module will act as ROM and
                        the file specified by this string will be loaded.
                        In this mode var1 will default to zero, not 2.

module pointers: none

8  bit buses: busa data bus
              busb 8-bit address bus (starts at 0)
              (out) bus0 points to memory address 0 (memory is sequential)
              (out) bus1 points to memory address 1
                    ...
16 bit buses: busc 16-bit address bus (starts at 0)
32 bit buses: busd 32-bit address bus (starts at 0)

incoming functions: infn0  reset the memory module
                    infn1  write to memory using 8-bit address
                    infn2  write to memory using 16-bit address
                    infn3  write to memory using 32-bit address
                    infn4  read from memory using 8-bit address
                    infn5  read from memory using 16-bit address
                    infn6  read from memory using 32-bit address
                    infn7  increment memory using 8-bit address
                    infn8  increment memory using 16-bit address
                    infn9  increment memory using 32-bit address
                    infn10 decrement memory using 8-bit address
                    infn11 decrement memory using 16-bit address
                    infn12 decrement memory using 32-bit address

outgoing functions: none

Module is unclocked.


Functional module 3: setbus
===========================

Allows buses to be re-directed when required.

8  bit variables: none
16 bit variables: none
32 bit variables: var0 - number of 8-bit  buses to be selected from (deft 0).
                  var1 - number of 16-bit buses to be selected from (deft 0).
                  var2 - number of 32-bit buses to be selected from (deft 0).
                  var3 - 8bit  bus in mod0 that is being selled (deft 0).
                  var4 - 16bit bus in mod1 that is being selled (deft 0).
                  var5 - 32bit bus in mod2 that is being selled (deft 0).

module pointers: mod0 - 8-bit  module where bus is being selected.
                 mod1 - 16-bit module where bus is being selected.
                 mod2 - 32-bit module where bus is being selected.

8  bit buses: busa0 - 8-bit  selector bus for 8-bit  bus module (start at 0)
              busa1 - 8-bit  selector bus for 16-bit bus module (start at 0)
              busa2 - 8-bit  selector bus for 32-bit bus module (start at 0)
              busb0 - 8-bit  select bus 0
              busb1 - 8-bit  select bus 1
              ....
              busbm - 8-bit  select bus var0
16 bit buses: busc0 - 16-bit selector bus for 8-bit  bus module (start at 0)
              busc1 - 16-bit selector bus for 16-bit bus module (start at 0)
              busc2 - 16-bit selector bus for 32-bit bus module (start at 0)
              busd0 - 16-bit select bus 0
              busd1 - 16-bit select bus 1
              ....
              busdn - 16-bit select bus var1
32 bit buses: buse0 - 32-bit selector bus for 8-bit  bus module (start at 0)
              buse1 - 32-bit selector bus for 16-bit bus module (start at 0)
              buse2 - 32-bit selector bus for 32-bit bus module (start at 0)
              busf0 - 32-bit select bus 0
              busf1 - 32-bit select bus 1
              ....
              busfp - 32-bit select bus var2

incoming functions: infn0 - set the 8-bit  bus in mod0 using 8-bit  selector
                    infn1 - set the 16-bit bus in mod1 using 8-bit  selector
                    infn2 - set the 32-bit bus in mod2 using 8-bit  selector
                    infn3 - set the 8-bit  bus in mod0 using 16-bit selector
                    infn4 - set the 16-bit bus in mod1 using 16-bit selector
                    infn5 - set the 32-bit bus in mod2 using 16-bit selector
                    infn6 - set the 8-bit  bus in mod0 using 32-bit selector
                    infn7 - set the 16-bit bus in mod1 using 32-bit selector
                    infn8 - set the 32-bit bus in mod2 using 32-bit selector
outgoing functions: none

Module is unclocked.


Branch module 1: istrue
=======================

This module is a basic "IF A != 0" logic module.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
16 bit buses: busb0 - 16-bit left arg
32 bit buses: busc0 - 32-bit left arg

incoming functions: infn0 - incoming call (8-bit)
                    infn1 - incoming call (16-bit)
                    infn2 - incoming call (32-bit)
outgoing functions: outfn0 - called if busa0 != 0 when infn0 called
                    outfn1 - called if busa0 == 0 when infn0 called
                    outfn2 - called if busb0 != 0 when infn1 called
                    outfn3 - called if busb0 == 0 when infn1 called
                    outfn4 - called if busc0 != 0 when infn2 called
                    outfn5 - called if busc0 == 0 when infn2 called

Module is unclocked.


Branch module 2: equals
=======================

This module is a basic "IF A == B" logic module.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
              busa1 - 8-bit  right arg
16 bit buses: busb0 - 16-bit left arg
              busb1 - 16-bit right arg
32 bit buses: busc0 - 32-bit left arg
              busc1 - 32-bit right arg

incoming functions: infn0 - incoming call (8-bit)
                    infn1 - incoming call (16-bit)
                    infn2 - incoming call (32-bit)
outgoing functions: outfn0 - called if busa0 == busa1 when infn0 called
                    outfn1 - called if busa0 != busa1 when infn0 called
                    outfn2 - called if busb0 == busb1 when infn1 called
                    outfn3 - called if busb0 != busb1 when infn1 called
                    outfn4 - called if busc0 == busc1 when infn2 called
                    outfn5 - called if busc0 != busc1 when infn2 called

Module is unclocked.


Branch module 3: greater
========================

This module is a basic "IF A > B" logic module.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
              busa1 - 8-bit  right arg
16 bit buses: busb0 - 16-bit left arg
              busb1 - 16-bit right arg
32 bit buses: busc0 - 32-bit left arg
              busc1 - 32-bit right arg

incoming functions: infn0 - incoming call (8-bit)
                    infn1 - incoming call (16-bit)
                    infn2 - incoming call (32-bit)
outgoing functions: outfn0 - called if busa0 >  busa1 when infn0 called
                    outfn1 - called if busa0 <= busa1 when infn0 called
                    outfn2 - called if busb0 >  busb1 when infn1 called
                    outfn3 - called if busb0 <= busb1 when infn1 called
                    outfn4 - called if busc0 >  busc1 when infn2 called
                    outfn5 - called if busc0 <= busc1 when infn2 called

Module is unclocked.


Branch module 4: less
=====================

This module is a basic "IF A < B" logic module.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
              busa1 - 8-bit  right arg
16 bit buses: busb0 - 16-bit left arg
              busb1 - 16-bit right arg
32 bit buses: busc0 - 32-bit left arg
              busc1 - 32-bit right arg

incoming functions: infn0 - incoming call (8-bit)
                    infn1 - incoming call (16-bit)
                    infn2 - incoming call (32-bit)
outgoing functions: outfn0 - called if busa0 <  busa1 when infn0 called
                    outfn1 - called if busa0 >= busa1 when infn0 called
                    outfn2 - called if busb0 <  busb1 when infn1 called
                    outfn3 - called if busb0 >= busb1 when infn1 called
                    outfn4 - called if busc0 <  busc1 when infn2 called
                    outfn5 - called if busc0 >= busc1 when infn2 called

Module is unclocked.


Branch module 5: equalsconst
============================

This module is a basic "IF A == B" logic module.

8  bit variables: vara0 - 8-bit  right arg (default 0)
16 bit variables: varb0 - 16-bit right arg (default 0)
32 bit variables: varc0 - 32-bit right arg (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
16 bit buses: busb0 - 16-bit left arg
32 bit buses: busc0 - 32-bit left arg

incoming functions: infn0 - incoming call (8-bit)
                    infn1 - incoming call (16-bit)
                    infn2 - incoming call (32-bit)
outgoing functions: outfn0 - called if busa0 == vara0 when infn0 called
                    outfn1 - called if busa0 != vara0 when infn0 called
                    outfn2 - called if busb0 == varb0 when infn1 called
                    outfn3 - called if busb0 != varb0 when infn1 called
                    outfn4 - called if busc0 == varc0 when infn2 called
                    outfn5 - called if busc0 != varc0 when infn2 called

Module is unclocked.


Branch module 6: greaterconst
=============================

This module is a basic "IF A > B" logic module.

8  bit variables: vara0 - 8-bit  right arg (default 0)
16 bit variables: varb0 - 16-bit right arg (default 0)
32 bit variables: varc0 - 32-bit right arg (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
16 bit buses: busb0 - 16-bit left arg
32 bit buses: busc0 - 32-bit left arg

incoming functions: infn0 - incoming call (8-bit)
                    infn1 - incoming call (16-bit)
                    infn2 - incoming call (32-bit)
outgoing functions: outfn0 - called if busa0 >  vara0 when infn0 called
                    outfn1 - called if busa0 <= vara0 when infn0 called
                    outfn2 - called if busb0 >  varb0 when infn1 called
                    outfn3 - called if busb0 <= varb0 when infn1 called
                    outfn4 - called if busc0 >  varc0 when infn2 called
                    outfn5 - called if busc0 <= varc0 when infn2 called

Module is unclocked.


Branch module 7: lessconst
==========================

This module is a basic "IF A < B" logic module.

8  bit variables: vara0 - 8-bit  right arg (default 0)
16 bit variables: varb0 - 16-bit right arg (default 0)
32 bit variables: varc0 - 32-bit right arg (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  left arg
16 bit buses: busb0 - 16-bit left arg
32 bit buses: busc0 - 32-bit left arg

incoming functions: infn0  - incoming call (8-bit)
                    infn1  - incoming call (16-bit)
                    infn2  - incoming call (32-bit)
outgoing functions: outfn1 - called if busa0 <  vara0 when infn0 called
                    outfn2 - called if busa0 >= vara0 when infn0 called
                    outfn3 - called if busb0 <  varb0 when infn1 called
                    outfn4 - called if busb0 >= varb0 when infn1 called
                    outfn5 - called if busc0 <  varc0 when infn2 called
                    outfn6 - called if busc0 >= varc0 when infn2 called

Module is unclocked.


ALU module 1: assign
====================

This module provides for the copying of buses.  When the source and dest
sizes not equal, a number of operations are supported.

1. If source size exceeds destination size then only part of the source
   will be copied.  In the following, lower refers to the least significant
   n bits (where n is (bits in source)/2).  Hence if the source is 32 bits
   and the destination 16, then lower means bits 0-15 of the source and
   upper bits 16-31 of the source.  If the destination is 8 bits, then
   lower-lower means bits 0-7 of the source, lower-upper bits 8-15,
   upper-lower bits 16-23 and upper-upper bits 24-31, and so-on.
2. If the destination size is bigger, then same nomenclature applies,
   except that it refers to which parts of the destination are to be
   overwritten, not which parts of the source are to be overwritten.  Those
   bits not overwritten may be either ignored (ND (non-destructive)
   functions) or overwritten with zero (D (destructive) functions).

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  dest bus
              busa1 - 8-bit  source bus
16 bit buses: busb0 - 16-bit dest bus
              busb1 - 16-bit source bus
32 bit buses: busc0 - 32-bit dest bus
              busc1 - 32-bit source bus

incoming functions: infn0  - sets busa0 = busa1 (ND)
                    infn1  - sets busa0 = busb1 (ND) (lower 8-bits copied)
                    infn2  - sets busa0 = busb1 (ND) (upper 8-bits copied)
                    infn3  - sets busa0 = busc1 (ND) (lower-lower 8-bits copied)
                    infn4  - sets busa0 = busc1 (ND) (lower-upper 8-bits copied)
                    infn5  - sets busa0 = busc1 (ND) (upper-lower 8-bits copied)
                    infn6  - sets busa0 = busc1 (ND) (upper-upper 8-bits copied)
                    infn7  - sets busb0 = busa1 (ND) (copy to lower 8-bits)
                    infn8  - sets busb0 = busa1 (ND) (copy to upper 8-bits)
                    infn9  - sets busb0 = busb1 (ND)
                    infn10 - sets busb0 = busc1 (ND) (lower 16-bits copied)
                    infn11 - sets busb0 = busc1 (ND) (upper 16-bits copied)
                    infn12 - sets busc0 = busa1 (ND) (copy to lower-lower 8-bits)
                    infn13 - sets busc0 = busa1 (ND) (copy to lower-upper 8-bits)
                    infn14 - sets busc0 = busa1 (ND) (copy to upper-lower 8-bits)
                    infn15 - sets busc0 = busa1 (ND) (copy to upper-upper 8-bits)
                    infn16 - sets busc0 = busb1 (ND) (copy to lower 16-bits)
                    infn17 - sets busc0 = busb1 (ND) (copy to upper 16-bits)
                    infn18 - sets busc0 = busc1 (ND)
                    infn19 - sets busa0 = busa1 (D)
                    infn20 - sets busa0 = busb1 (D) (lower 8-bits copied)
                    infn21 - sets busa0 = busb1 (D) (upper 8-bits copied)
                    infn22 - sets busa0 = busc1 (D) (lower-lower 8-bits copied)
                    infn23 - sets busa0 = busc1 (D) (lower-upper 8-bits copied)
                    infn24 - sets busa0 = busc1 (D) (upper-lower 8-bits copied)
                    infn25 - sets busa0 = busc1 (D) (upper-upper 8-bits copied)
                    infn26 - sets busb0 = busa1 (D) (copy to lower 8-bits)
                    infn27 - sets busb0 = busa1 (D) (copy to upper 8-bits)
                    infn28 - sets busb0 = busb1 (D)
                    infn29 - sets busb0 = busc1 (D) (lower 16-bits copied)
                    infn30 - sets busb0 = busc1 (D) (upper 16-bits copied)
                    infn31 - sets busc0 = busa1 (D) (copy to lower-lower 8-bits)
                    infn32 - sets busc0 = busa1 (D) (copy to lower-upper 8-bits)
                    infn33 - sets busc0 = busa1 (D) (copy to upper-lower 8-bits)
                    infn34 - sets busc0 = busa1 (D) (copy to upper-upper 8-bits)
                    infn35 - sets busc0 = busb1 (D) (copy to lower 16-bits)
                    infn36 - sets busc0 = busb1 (D) (copy to upper 16-bits)
                    infn37 - sets busc0 = busc1 (D)
outgoing functions: none

Module is unclocked.



ALU module 2: not
=================

This module provides for the inversion of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = inv(busa1) (8  bit inverse)
                    infn1 - sets busb0 = inv(busb1) (16 bit inverse)
                    infn2 - sets busc0 = inv(busc1) (32 bit inverse)
outgoing functions: none

Module is unclocked.


ALU module 3: twos
==================

This module provides for the 2's complement of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = 2's(busa1) (8  bit 2's)
                    infn1 - sets busb0 = 2's(busb1) (16 bit 2's)
                    infn2 - sets busc0 = 2's(busc1) (32 bit 2's)
outgoing functions: none

Module is unclocked.


ALU module 4: add
=================

This module provides for the addition of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busb2 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1+busa2 (8-bit  add)
                    infn1 - sets busb0 = busb1+busb2 (16-bit add)
                    infn2 - sets busc0 = busc1+busc2 (32-bit add)
outgoing functions: none

Module is unclocked.


ALU module 5: sub
=================

This module provides for the subtraction of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busa0 - 16-bit result bus
              busa1 - 16-bit arg1 bus
              busa2 - 16-bit arg2 bus
32 bit buses: busa0 - 32-bit result bus
              busa1 - 32-bit arg1 bus
              busa2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1-busa2 (8-bit  sub)
                    infn1 - sets busb0 = busb1-busb2 (16-bit sub)
                    infn2 - sets busc0 = busc1-busc2 (32-bit sub)
outgoing functions: none

Module is unclocked.


ALU module 6: mul
=================

This module provides for the multiplication of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busb2 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1*busa2 (8-bit  mul)
                    infn1 - sets busb0 = busb1*busb2 (16-bit mul)
                    infn2 - sets busc0 = busc1*busc2 (32-bit mul)
outgoing functions: none

Module is unclocked.


ALU module 7: div
=================

This module provides for the integer division of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busb2 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1/busa2 (8-bit  div)
                    infn1 - sets busb0 = busb1/busb2 (16-bit div)
                    infn2 - sets busc0 = busc1/busc2 (32-bit div)
outgoing functions: none

Module is unclocked.


ALU module 8: mod
=================

This module provides for the modulus of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busb1 - 8-bit  arg1 bus
              busc2 - 8-bit  arg2 bus
16 bit buses: busa0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busc2 - 16-bit arg2 bus
32 bit buses: busa0 - 32-bit result bus
              busb1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1%busa2 (8-bit  mod)
                    infn1 - sets busb0 = busb1%busb2 (16-bit mod)
                    infn2 - sets busc0 = busc1%busc2 (32-bit mod)
outgoing functions: none

Module is unclocked.


ALU module 9: and
=================

This module provides for the ANDing of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busb2 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1&busa2 (8-bit  and)
                    infn1 - sets busb0 = busb1&busb2 (16-bit and)
                    infn2 - sets busc0 = busc1&busc2 (32-bit and)
outgoing functions: none

Module is unclocked.


ALU module 10: or
=================

This module provides for the ORing of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busb2 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1|busa2 (8-bit  or)
                    infn1 - sets busb0 = busb1|busb2 (16-bit or)
                    infn2 - sets busc0 = busc1|busc2 (32-bit or)
                    infn3 - sets busb0 = busb1|busa2 (mixed mode or)
outgoing functions: none

Module is unclocked.


ALU module 11: xor
==================

This module provides for the XORing of buses.

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
              busa2 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
              busb2 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus
              busc2 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = busa1^busa2 (8-bit  xor)
                    infn1 - sets busb0 = busb1^busb2 (16-bit xor)
                    infn2 - sets busc0 = busc1^busc2 (32-bit xor)
outgoing functions: none

Module is unclocked.


ALU module 12: rr
=================

This module provides for the right rotation of buses (LSB goes to MSB).

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
              busa2 - 8  bit count bus
              busa3 - 16 bit count bus
              busa4 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1>.>busa2 (8-bit  rr)
                    infn1 - sets busb0 = busb1>.>busa3 (16-bit rr)
                    infn2 - sets busc0 = busc1>.>busa4 (32-bit rr)
outgoing functions: none

Module is unclocked.


ALU module 13: rl
=================

This module provides for the left rotation of buses (MSB goes to LSB).

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
              busa2 - 8  bit count bus
              busa3 - 16 bit count bus
              busa4 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1<.<busa2 (8-bit  rl)
                    infn1 - sets busb0 = busb1<.<busa3 (16-bit rl)
                    infn2 - sets busc0 = busc1<.<busa4 (32-bit rl)
outgoing functions: none

Module is unclocked.


ALU module 14: sr
=================

This module provides for the right shift of buses (MSB = 0).

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
              busa2 - 8  bit count bus
              busa3 - 16 bit count bus
              busa4 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1>>busa2 (8-bit  sr)
                    infn1 - sets busb0 = busb1>>busa3 (16-bit sr)
                    infn2 - sets busc0 = busc1>>busa4 (32-bit sr)
outgoing functions: none

Module is unclocked.


ALU module 15: sl
=================

This module provides for the left shift of buses (LSB = 0).

8  bit variables: none
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
              busa2 - 8  bit count bus
              busa3 - 16 bit count bus
              busa4 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1<<busa2 (8-bit  sl)
                    infn1 - sets busb0 = busb1<<busa3 (16-bit sl)
                    infn2 - sets busc0 = busc1<<busa4 (32-bit sl)
outgoing functions: none

Module is unclocked.


ALU module 16: assignconst
==========================

This module provides for the copying of buses.  When the source and dest
sizes not equal, a number of operations are supported.

1. If source size exceeds destination size then only part of the source
   will be copied.  In the following, lower refers to the least significant
   n bits (where n is (bits in source)/2).  Hence if the source is 32 bits
   and the destination 16, then lower means bits 0-15 of the source and
   upper bits 16-31 of the source.  If the destination is 8 bits, then
   lower-lower means bits 0-7 of the source, lower-upper bits 8-15,
   upper-lower bits 16-23 and upper-upper bits 24-31, and so-on.
2. If the destination size is bigger, then same nomenclature applies,
   except that it refers to which parts of the destination are to be
   overwritten, not which parts of the source are to be overwritten.  Those
   bits not overwritten may be either ignored (ND (non-destructive)
   functions) or overwritten with zero (D (destructive) functions).

8  bit variables: vara0 - 8-bit  source (default 0)
16 bit variables: varb0 - 16-bit source (default 0)
32 bit variables: varc0 - 32-bit source (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  dest bus
16 bit buses: busb0 - 16-bit dest bus
32 bit buses: busc0 - 32-bit dest bus

incoming functions: infn0  - sets busa0 = vara0 (ND)
                    infn1  - sets busa0 = varb0 (ND) (lower 8-bits copied)
                    infn2  - sets busa0 = varb0 (ND) (upper 8-bits copied)
                    infn3  - sets busa0 = varc0 (ND) (lower-lower 8-bits copied)
                    infn4  - sets busa0 = varc0 (ND) (lower-upper 8-bits copied)
                    infn5  - sets busa0 = varc0 (ND) (upper-lower 8-bits copied)
                    infn6  - sets busa0 = varc0 (ND) (upper-upper 8-bits copied)
                    infn7  - sets busb0 = vara0 (ND) (copy to lower 8-bits)
                    infn8  - sets busb0 = vara0 (ND) (copy to upper 8-bits)
                    infn9  - sets busb0 = varb0 (ND)
                    infn10 - sets busb0 = varc0 (ND) (lower 16-bits copied)
                    infn11 - sets busb0 = varc0 (ND) (upper 16-bits copied)
                    infn12 - sets busc0 = vara0 (ND) (copy to lower-lower 8-bits)
                    infn13 - sets busc0 = vara0 (ND) (copy to lower-upper 8-bits)
                    infn14 - sets busc0 = vara0 (ND) (copy to upper-lower 8-bits)
                    infn15 - sets busc0 = vara0 (ND) (copy to upper-upper 8-bits)
                    infn16 - sets busc0 = varb0 (ND) (copy to lower 16-bits)
                    infn17 - sets busc0 = varb0 (ND) (copy to upper 16-bits)
                    infn18 - sets busc0 = varc0 (ND)
                    infn19 - sets busa0 = vara0 (D)
                    infn20 - sets busa0 = varb0 (D) (lower 8-bits copied)
                    infn21 - sets busa0 = varb0 (D) (upper 8-bits copied)
                    infn22 - sets busa0 = varc0 (D) (lower-lower 8-bits copied)
                    infn23 - sets busa0 = varc0 (D) (lower-upper 8-bits copied)
                    infn24 - sets busa0 = varc0 (D) (upper-lower 8-bits copied)
                    infn25 - sets busa0 = varc0 (D) (upper-upper 8-bits copied)
                    infn26 - sets busb0 = vara0 (D) (copy to lower 8-bits)
                    infn27 - sets busb0 = vara0 (D) (copy to upper 8-bits)
                    infn28 - sets busb0 = varb0 (D)
                    infn29 - sets busb0 = varc0 (D) (lower 16-bits copied)
                    infn30 - sets busb0 = varc0 (D) (upper 16-bits copied)
                    infn31 - sets busc0 = vara0 (D) (copy to lower-lower 8-bits)
                    infn32 - sets busc0 = vara0 (D) (copy to lower-upper 8-bits)
                    infn33 - sets busc0 = vara0 (D) (copy to upper-lower 8-bits)
                    infn34 - sets busc0 = vara0 (D) (copy to upper-upper 8-bits)
                    infn35 - sets busc0 = varb0 (D) (copy to lower 16-bits)
                    infn36 - sets busc0 = varb0 (D) (copy to upper 16-bits)
                    infn37 - sets busc0 = varc0 (D)
outgoing functions: none

Module is unclocked.



ALU module 17: addconst
=======================

This module provides for the addition of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1+vara0 (8-bit  add)
                    infn1 - sets busb0 = busb1+varb0 (16-bit add)
                    infn2 - sets busc0 = busc1+varc0 (32-bit add)
outgoing functions: none

Module is unclocked.


ALU module 18: subconsta
========================

This module provides for the subtraction of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1-vara0 (8-bit  sub)
                    infn1 - sets busb0 = busb1-varb0 (16-bit sub)
                    infn2 - sets busc0 = busc1-varc0 (32-bit sub)
outgoing functions: none

Module is unclocked.


ALU module 19: subconstb
========================

This module provides for the subtraction of buses.

8  bit variables: vara0 - 8-bit  arg1 bus (default 0)
16 bit variables: vara0 - 16-bit arg1 bus (default 0)
32 bit variables: vara0 - 32-bit arg1 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = vara0-busa1 (8-bit  sub)
                    infn1 - sets busb0 = varb0-busb1 (16-bit sub)
                    infn2 - sets busc0 = varc0-busc1 (32-bit sub)
outgoing functions: none

Module is unclocked.


ALU module 20: mulconst
=======================

This module provides for the multiplication of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1*vara0 (8-bit  mul)
                    infn1 - sets busb0 = busb1*varb0 (16-bit mul)
                    infn2 - sets busc0 = busc1*varc0 (32-bit mul)
outgoing functions: none

Module is unclocked.


ALU module 21: divconsta
========================

This module provides for the integer division of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1/vara0 (8-bit  div)
                    infn1 - sets busb0 = busb1/varb0 (16-bit div)
                    infn2 - sets busc0 = busc1/varc0 (32-bit div)
outgoing functions: none

Module is unclocked.


ALU module 22: divconstb
========================

This module provides for the integer division of buses.

8  bit variables: vara0 - 8-bit  arg1 bus (default 0)
16 bit variables: vara0 - 16-bit arg1 bus (default 0)
32 bit variables: vara0 - 32-bit arg1 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = vara0/busa1 (8-bit  div)
                    infn1 - sets busb0 = varb0/busb1 (16-bit div)
                    infn2 - sets busc0 = varc0/busc1 (32-bit div)
outgoing functions: none

Module is unclocked.


ALU module 23: modconsta
========================

This module provides for the modulus of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1%vara0 (8-bit  mod)
                    infn1 - sets busb0 = busb1%varb0 (16-bit mod)
                    infn2 - sets busc0 = busc1%varc0 (32-bit mod)
outgoing functions: none

Module is unclocked.


ALU module 24: modconstb
========================

This module provides for the modulus of buses.

8  bit variables: vara0 - 8-bit  arg1 bus (default 0)
16 bit variables: vara0 - 16-bit arg1 bus (default 0)
32 bit variables: vara0 - 32-bit arg1 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg2 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg2 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg2 bus

incoming functions: infn0 - sets busa0 = vara0%busa1 (8-bit  mod)
                    infn1 - sets busb0 = varb0%busb1 (16-bit mod)
                    infn2 - sets busc0 = varc0%busc1 (32-bit mod)
outgoing functions: none

Module is unclocked.


ALU module 25: andconst
=======================

This module provides for the ANDing of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1&vara0 (8-bit  and)
                    infn1 - sets busb0 = busb1&varb0 (16-bit and)
                    infn2 - sets busc0 = busc1&varc0 (32-bit and)
outgoing functions: none

Module is unclocked.


ALU module 26: orconst
======================

This module provides for the ORing of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1|vara0 (8-bit  or)
                    infn1 - sets busb0 = busb1|varb0 (16-bit or)
                    infn2 - sets busc0 = busc1|varc0 (32-bit or)
outgoing functions: none

Module is unclocked.


ALU module 27: xorconst
=======================

This module provides for the XORing of buses.

8  bit variables: vara0 - 8-bit  arg2 bus (default 0)
16 bit variables: varb0 - 16-bit arg2 bus (default 0)
32 bit variables: varc0 - 32-bit arg2 bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  arg1 bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit arg1 bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit arg1 bus

incoming functions: infn0 - sets busa0 = busa1^vara0 (8-bit  xor)
                    infn1 - sets busb0 = busb1^varb0 (16-bit xor)
                    infn2 - sets busc0 = busc1^varc0 (32-bit xor)
outgoing functions: none

Module is unclocked.


ALU module 28: rrconsta
=======================

This module provides for the right rotation of buses (LSB goes to MSB).

8  bit variables: vara0 - 8-bit  count bus (default 0)
                  vara1 - 16-bit count bus (default 0)
                  vara2 - 32-bit count bus (default 0)
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1>.>vara0 (8-bit  rr)
                    infn1 - sets busb0 = busb1>.>vara1 (16-bit rr)
                    infn2 - sets busc0 = busc1>.>vara2 (32-bit rr)
outgoing functions: none

Module is unclocked.


ALU module 29: rrconstb
=======================

This module provides for the right rotation of buses (LSB goes to MSB).

8  bit variables: vara0 - 8-bit  argument bus (default 0)
16 bit variables: varb0 - 16-bit argument bus (default 0)
32 bit variables: varc0 - 32-bit argument bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8  bit count bus
              busa2 - 16 bit count bus
              busa3 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
32 bit buses: busc0 - 32-bit result bus

incoming functions: infn0 - sets busa0 = vara0>.>busa1 (8-bit  rr)
                    infn1 - sets busb0 = varb0>.>busa2 (16-bit rr)
                    infn2 - sets busc0 = varc0>.>busa3 (32-bit rr)
outgoing functions: none

Module is unclocked.


ALU module 30: rlconsta
=======================

This module provides for the left rotation of buses (MSB goes to LSB).

8  bit variables: vara0 - 8-bit  count bus (default 0)
                  vara1 - 16-bit count bus (default 0)
                  vara2 - 32-bit count bus (default 0)
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1<.<vara0 (8-bit  rl)
                    infn1 - sets busb0 = busb1<.<vara1 (16-bit rl)
                    infn2 - sets busc0 = busc1<.<vara2 (32-bit rl)
outgoing functions: none

Module is unclocked.


ALU module 31: rlconstb
=======================

This module provides for the left rotation of buses (MSB goes to LSB).

8  bit variables: vara0 - 8-bit  argument bus (default 0)
16 bit variables: varb0 - 16-bit argument bus (default 0)
32 bit variables: varc0 - 32-bit argument bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8  bit count bus
              busa2 - 16 bit count bus
              busa3 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
32 bit buses: busc0 - 32-bit result bus

incoming functions: infn0 - sets busa0 = vara0<.<busa1 (8-bit  rl)
                    infn1 - sets busb0 = varb0<.<busa2 (16-bit rl)
                    infn2 - sets busc0 = varc0<.<busa3 (32-bit rl)
outgoing functions: none

Module is unclocked.


ALU module 32: srconsta
=======================

This module provides for the right shift of buses (MSB = 0).

8  bit variables: vara0 - 8-bit  count bus (default 0)
                  vara1 - 16-bit count bus (default 0)
                  vara2 - 32-bit count bus (default 0)
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1>>vara0 (8-bit  sr)
                    infn1 - sets busb0 = busb1>>vara1 (16-bit sr)
                    infn2 - sets busc0 = busc1>>vara2 (32-bit sr)
outgoing functions: none

Module is unclocked.


ALU module 33: srconstb
=======================

This module provides for the right shift of buses (MSB = 0).

8  bit variables: vara0 - 8-bit  argument bus (default 0)
16 bit variables: varb0 - 16-bit argument bus (default 0)
32 bit variables: varc0 - 32-bit argument bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8  bit count bus
              busa2 - 16 bit count bus
              busa3 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
32 bit buses: busc0 - 32-bit result bus

incoming functions: infn0 - sets busa0 = vara0>>busa1 (8-bit  sr)
                    infn1 - sets busb0 = varb0>>busa2 (16-bit sr)
                    infn2 - sets busc0 = varc0>>busa3 (32-bit sr)
outgoing functions: none

Module is unclocked.


ALU module 34: slconsta
=======================

This module provides for the left shift of buses (LSB = 0).

8  bit variables: vara0 - 8-bit  count bus (default 0)
                  vara1 - 16-bit count bus (default 0)
                  vara2 - 32-bit count bus (default 0)
16 bit variables: none
32 bit variables: none

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8-bit  argument bus
16 bit buses: busb0 - 16-bit result bus
              busb1 - 16-bit argument bus
32 bit buses: busc0 - 32-bit result bus
              busc1 - 32-bit argument bus

incoming functions: infn0 - sets busa0 = busa1<<vara0 (8-bit  sl)
                    infn1 - sets busb0 = busb1<<vara1 (16-bit sl)
                    infn2 - sets busc0 = busc1<<vara2 (32-bit sl)
outgoing functions: none

Module is unclocked.


ALU module 35: slconstb
=======================

This module provides for the left shift of buses (LSB = 0).

8  bit variables: vara0 - 8-bit  argument bus (default 0)
16 bit variables: varb0 - 16-bit argument bus (default 0)
32 bit variables: varc0 - 32-bit argument bus (default 0)

module pointers: none

8  bit buses: busa0 - 8-bit  result bus
              busa1 - 8  bit count bus
              busa2 - 16 bit count bus
              busa3 - 23 bit count bus
16 bit buses: busb0 - 16-bit result bus
32 bit buses: busc0 - 32-bit result bus

incoming functions: infn0 - sets busa0 = vara0<<busa1 (8-bit  sl)
                    infn1 - sets busb0 = varb0<<busa2 (16-bit sl)
                    infn2 - sets busc0 = varc0<<busa3 (32-bit sl)
outgoing functions: none

Module is unclocked.


*/

module_data *nullmod_alloc(const char *module_name);
int          nullmod_init(module_data *what);
void         nullmod_go(module_data *what);
void         nullmod_stop(module_data *what);
void         nullmod_remove(module_data *what);
void         nullmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *nullmod_getinf(module_data *what);

module_data *domod_alloc(const char *module_name);
int          domod_init(module_data *what);
void         domod_go(module_data *what);
void         domod_stop(module_data *what);
void         domod_remove(module_data *what);
void         domod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *domod_getinf(module_data *what);

module_data *dowhilemod_alloc(const char *module_name);
int          dowhilemod_init(module_data *what);
void         dowhilemod_go(module_data *what);
void         dowhilemod_stop(module_data *what);
void         dowhilemod_remove(module_data *what);
void         dowhilemod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *dowhilemod_getinf(module_data *what);

module_data *table8mod_alloc(const char *module_name);
int          table8mod_init(module_data *what);
void         table8mod_go(module_data *what);
void         table8mod_stop(module_data *what);
void         table8mod_remove(module_data *what);
void         table8mod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *table8mod_getinf(module_data *what);

module_data *lut8mod_alloc(const char *module_name);
int          lut8mod_init(module_data *what);
void         lut8mod_go(module_data *what);
void         lut8mod_stop(module_data *what);
void         lut8mod_remove(module_data *what);
void         lut8mod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *lut8mod_getinf(module_data *what);

module_data *busmod_alloc(const char *module_name);
int          busmod_init(module_data *what);
void         busmod_go(module_data *what);
void         busmod_stop(module_data *what);
void         busmod_remove(module_data *what);
void         busmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *busmod_getinf(module_data *what);

module_data *memmod_alloc(const char *module_name);
int          memmod_init(module_data *what);
void         memmod_go(module_data *what);
void         memmod_stop(module_data *what);
void         memmod_remove(module_data *what);
void         memmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *memmod_getinf(module_data *what);

module_data *setbusmod_alloc(const char *module_name);
int          setbusmod_init(module_data *what);
void         setbusmod_go(module_data *what);
void         setbusmod_stop(module_data *what);
void         setbusmod_remove(module_data *what);
void         setbusmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *setbusmod_getinf(module_data *what);

module_data *istruemod_alloc(const char *module_name);
int          istruemod_init(module_data *what);
void         istruemod_go(module_data *what);
void         istruemod_stop(module_data *what);
void         istruemod_remove(module_data *what);
void         istruemod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *istruemod_getinf(module_data *what);

module_data *equalsmod_alloc(const char *module_name);
int          equalsmod_init(module_data *what);
void         equalsmod_go(module_data *what);
void         equalsmod_stop(module_data *what);
void         equalsmod_remove(module_data *what);
void         equalsmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *equalsmod_getinf(module_data *what);

module_data *greatermod_alloc(const char *module_name);
int          greatermod_init(module_data *what);
void         greatermod_go(module_data *what);
void         greatermod_stop(module_data *what);
void         greatermod_remove(module_data *what);
void         greatermod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *greatermod_getinf(module_data *what);

module_data *lessmod_alloc(const char *module_name);
int          lessmod_init(module_data *what);
void         lessmod_go(module_data *what);
void         lessmod_stop(module_data *what);
void         lessmod_remove(module_data *what);
void         lessmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *lessmod_getinf(module_data *what);

module_data *equalsconstmod_alloc(const char *module_name);
int          equalsconstmod_init(module_data *what);
void         equalsconstmod_go(module_data *what);
void         equalsconstmod_stop(module_data *what);
void         equalsconstmod_remove(module_data *what);
void         equalsconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *equalsconstmod_getinf(module_data *what);

module_data *greaterconstmod_alloc(const char *module_name);
int          greaterconstmod_init(module_data *what);
void         greaterconstmod_go(module_data *what);
void         greaterconstmod_stop(module_data *what);
void         greaterconstmod_remove(module_data *what);
void         greaterconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *greaterconstmod_getinf(module_data *what);

module_data *lessconstmod_alloc(const char *module_name);
int          lessconstmod_init(module_data *what);
void         lessconstmod_go(module_data *what);
void         lessconstmod_stop(module_data *what);
void         lessconstmod_remove(module_data *what);
void         lessconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *lessconstmod_getinf(module_data *what);

module_data *assignmod_alloc(const char *module_name);
int          assignmod_init(module_data *what);
void         assignmod_go(module_data *what);
void         assignmod_stop(module_data *what);
void         assignmod_remove(module_data *what);
void         assignmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *assignmod_getinf(module_data *what);

module_data *notmod_alloc(const char *module_name);
int          notmod_init(module_data *what);
void         notmod_go(module_data *what);
void         notmod_stop(module_data *what);
void         notmod_remove(module_data *what);
void         notmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *notmod_getinf(module_data *what);

module_data *twosmod_alloc(const char *module_name);
int          twosmod_init(module_data *what);
void         twosmod_go(module_data *what);
void         twosmod_stop(module_data *what);
void         twosmod_remove(module_data *what);
void         twosmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *twosmod_getinf(module_data *what);

module_data *addmod_alloc(const char *module_name);
int          addmod_init(module_data *what);
void         addmod_go(module_data *what);
void         addmod_stop(module_data *what);
void         addmod_remove(module_data *what);
void         addmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *addmod_getinf(module_data *what);

module_data *submod_alloc(const char *module_name);
int          submod_init(module_data *what);
void         submod_go(module_data *what);
void         submod_stop(module_data *what);
void         submod_remove(module_data *what);
void         submod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *submod_getinf(module_data *what);

module_data *mulmod_alloc(const char *module_name);
int          mulmod_init(module_data *what);
void         mulmod_go(module_data *what);
void         mulmod_stop(module_data *what);
void         mulmod_remove(module_data *what);
void         mulmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *mulmod_getinf(module_data *what);

module_data *divmod_alloc(const char *module_name);
int          divmod_init(module_data *what);
void         divmod_go(module_data *what);
void         divmod_stop(module_data *what);
void         divmod_remove(module_data *what);
void         divmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *divmod_getinf(module_data *what);

module_data *modmod_alloc(const char *module_name);
int          modmod_init(module_data *what);
void         modmod_go(module_data *what);
void         modmod_stop(module_data *what);
void         modmod_remove(module_data *what);
void         modmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *modmod_getinf(module_data *what);

module_data *andmod_alloc(const char *module_name);
int          andmod_init(module_data *what);
void         andmod_go(module_data *what);
void         andmod_stop(module_data *what);
void         andmod_remove(module_data *what);
void         andmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *andmod_getinf(module_data *what);

module_data *ormod_alloc(const char *module_name);
int          ormod_init(module_data *what);
void         ormod_go(module_data *what);
void         ormod_stop(module_data *what);
void         ormod_remove(module_data *what);
void         ormod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *ormod_getinf(module_data *what);

module_data *xormod_alloc(const char *module_name);
int          xormod_init(module_data *what);
void         xormod_go(module_data *what);
void         xormod_stop(module_data *what);
void         xormod_remove(module_data *what);
void         xormod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *xormod_getinf(module_data *what);

module_data *rrmod_alloc(const char *module_name);
int          rrmod_init(module_data *what);
void         rrmod_go(module_data *what);
void         rrmod_stop(module_data *what);
void         rrmod_remove(module_data *what);
void         rrmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *rrmod_getinf(module_data *what);

module_data *rlmod_alloc(const char *module_name);
int          rlmod_init(module_data *what);
void         rlmod_go(module_data *what);
void         rlmod_stop(module_data *what);
void         rlmod_remove(module_data *what);
void         rlmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *rlmod_getinf(module_data *what);

module_data *srmod_alloc(const char *module_name);
int          srmod_init(module_data *what);
void         srmod_go(module_data *what);
void         srmod_stop(module_data *what);
void         srmod_remove(module_data *what);
void         srmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *srmod_getinf(module_data *what);

module_data *slmod_alloc(const char *module_name);
int          slmod_init(module_data *what);
void         slmod_go(module_data *what);
void         slmod_stop(module_data *what);
void         slmod_remove(module_data *what);
void         slmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *slmod_getinf(module_data *what);

module_data *assignconstmod_alloc(const char *module_name);
int          assignconstmod_init(module_data *what);
void         assignconstmod_go(module_data *what);
void         assignconstmod_stop(module_data *what);
void         assignconstmod_remove(module_data *what);
void         assignconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *assignconstmod_getinf(module_data *what);

module_data *addconstmod_alloc(const char *module_name);
int          addconstmod_init(module_data *what);
void         addconstmod_go(module_data *what);
void         addconstmod_stop(module_data *what);
void         addconstmod_remove(module_data *what);
void         addconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *addconstmod_getinf(module_data *what);

module_data *subconstamod_alloc(const char *module_name);
int          subconstamod_init(module_data *what);
void         subconstamod_go(module_data *what);
void         subconstamod_stop(module_data *what);
void         subconstamod_remove(module_data *what);
void         subconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *subconstamod_getinf(module_data *what);

module_data *subconstbmod_alloc(const char *module_name);
int          subconstbmod_init(module_data *what);
void         subconstbmod_go(module_data *what);
void         subconstbmod_stop(module_data *what);
void         subconstbmod_remove(module_data *what);
void         subconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *subconstbmod_getinf(module_data *what);

module_data *mulconstmod_alloc(const char *module_name);
int          mulconstmod_init(module_data *what);
void         mulconstmod_go(module_data *what);
void         mulconstmod_stop(module_data *what);
void         mulconstmod_remove(module_data *what);
void         mulconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *mulconstmod_getinf(module_data *what);

module_data *divconstamod_alloc(const char *module_name);
int          divconstamod_init(module_data *what);
void         divconstamod_go(module_data *what);
void         divconstamod_stop(module_data *what);
void         divconstamod_remove(module_data *what);
void         divconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *divconstamod_getinf(module_data *what);

module_data *divconstbmod_alloc(const char *module_name);
int          divconstbmod_init(module_data *what);
void         divconstbmod_go(module_data *what);
void         divconstbmod_stop(module_data *what);
void         divconstbmod_remove(module_data *what);
void         divconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *divconstbmod_getinf(module_data *what);

module_data *modconstamod_alloc(const char *module_name);
int          modconstamod_init(module_data *what);
void         modconstamod_go(module_data *what);
void         modconstamod_stop(module_data *what);
void         modconstamod_remove(module_data *what);
void         modconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *modconstamod_getinf(module_data *what);

module_data *modconstbmod_alloc(const char *module_name);
int          modconstbmod_init(module_data *what);
void         modconstbmod_go(module_data *what);
void         modconstbmod_stop(module_data *what);
void         modconstbmod_remove(module_data *what);
void         modconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *modconstbmod_getinf(module_data *what);

module_data *andconstmod_alloc(const char *module_name);
int          andconstmod_init(module_data *what);
void         andconstmod_go(module_data *what);
void         andconstmod_stop(module_data *what);
void         andconstmod_remove(module_data *what);
void         andconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *andconstmod_getinf(module_data *what);

module_data *orconstmod_alloc(const char *module_name);
int          orconstmod_init(module_data *what);
void         orconstmod_go(module_data *what);
void         orconstmod_stop(module_data *what);
void         orconstmod_remove(module_data *what);
void         orconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *orconstmod_getinf(module_data *what);

module_data *xorconstmod_alloc(const char *module_name);
int          xorconstmod_init(module_data *what);
void         xorconstmod_go(module_data *what);
void         xorconstmod_stop(module_data *what);
void         xorconstmod_remove(module_data *what);
void         xorconstmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *xorconstmod_getinf(module_data *what);

module_data *rrconstamod_alloc(const char *module_name);
int          rrconstamod_init(module_data *what);
void         rrconstamod_go(module_data *what);
void         rrconstamod_stop(module_data *what);
void         rrconstamod_remove(module_data *what);
void         rrconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *rrconstamod_getinf(module_data *what);

module_data *rrconstbmod_alloc(const char *module_name);
int          rrconstbmod_init(module_data *what);
void         rrconstbmod_go(module_data *what);
void         rrconstbmod_stop(module_data *what);
void         rrconstbmod_remove(module_data *what);
void         rrconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *rrconstbmod_getinf(module_data *what);

module_data *rlconstamod_alloc(const char *module_name);
int          rlconstamod_init(module_data *what);
void         rlconstamod_go(module_data *what);
void         rlconstamod_stop(module_data *what);
void         rlconstamod_remove(module_data *what);
void         rlconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *rlconstamod_getinf(module_data *what);

module_data *rlconstbmod_alloc(const char *module_name);
int          rlconstbmod_init(module_data *what);
void         rlconstbmod_go(module_data *what);
void         rlconstbmod_stop(module_data *what);
void         rlconstbmod_remove(module_data *what);
void         rlconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *rlconstbmod_getinf(module_data *what);

module_data *srconstamod_alloc(const char *module_name);
int          srconstamod_init(module_data *what);
void         srconstamod_go(module_data *what);
void         srconstamod_stop(module_data *what);
void         srconstamod_remove(module_data *what);
void         srconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *srconstamod_getinf(module_data *what);

module_data *srconstbmod_alloc(const char *module_name);
int          srconstbmod_init(module_data *what);
void         srconstbmod_go(module_data *what);
void         srconstbmod_stop(module_data *what);
void         srconstbmod_remove(module_data *what);
void         srconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *srconstbmod_getinf(module_data *what);

module_data *slconstamod_alloc(const char *module_name);
int          slconstamod_init(module_data *what);
void         slconstamod_go(module_data *what);
void         slconstamod_stop(module_data *what);
void         slconstamod_remove(module_data *what);
void         slconstamod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *slconstamod_getinf(module_data *what);

module_data *slconstbmod_alloc(const char *module_name);
int          slconstbmod_init(module_data *what);
void         slconstbmod_go(module_data *what);
void         slconstbmod_stop(module_data *what);
void         slconstbmod_remove(module_data *what);
void         slconstbmod_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point);
char        *slconstbmod_getinf(module_data *what);

#endif
