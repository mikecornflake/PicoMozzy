# PicoMozzy
(2005) A Microbee 32k Emulator v1.25.09.2005 (beta)

## 2024 Notes

This win95/98/DOS project recently resurfaced, and I think the project is worth preserving and making available.  I had no part of it's development, so I contacted Alistair Shilton and obtained permission to upload to github.   
[Original webpage](docs/Microbee_page.md) was retrieved from the Wayback Machine by @Chickenman

Mike Thompson (Feb 2024)

The below is modified from the original [readme.txt](docs/readme.txt) (which can be found in the docs folder)

## PicoMozzy - a Microbee 32k Emulator v1.25.09.2005 (beta)

By Alistair Shilton
apsh@ee.unimelb.edu.au

http://www2.ee.mu.oz.au/pgrad/apsh/microbee/microbee.html (dead link, but cached [here](docs/Microbee_page.md))

### Standard disclaimer

I do not accept any responsibility for any effects that this code may
have on you, your computer, your sanity, or anything else.  Use at own
risk.

### Introduction

PicoMozzy is an emulator for the Microbee platform.  Eventually, I plan
to support most of the Microbee line of computers from the old 32k to the
256TC (and beyond... it all depends on documentation).

Anyhow, I hope you enjoy what's here already.  If you find any major unknown
bugs (see list of known bugs below), please send details to
apsh@ee.unimelb.edu.au.  To lessen the chance that your mail might get
mistaken for spam (that account gets a lot), make the first word in the
header of you email "PicoMozzy".

### Updates in V1.25.09.2005 (beta)
+ Sound now available in Windows version.  Yay!

### Updates in v1.14.09.2005 (beta)
+ Minor bugfixes, improved keyboard response.

### Updates in V1.19.06.2005 (beta)
+ Major re-write of tape interface.  Several major bugs removed.
+ Improvements made to keyboard interface.  The shift key should now work
  more reliably.
+ Sound temporarily removed from the windows version.

### Updates in V1.05.06.2005 (beta)
+ Sound added for windows version (still not perfect, unfortunately).
+ Added support for tape interface.  Programs can now be loaded using the
  tape commands, without resorting to hacks required previously (wave file
  support coming soon...).


### Loading .COM files (OBSOLETE in v1.05.06.2005, USE TAPE INTERFACE)
The following process may be used to run (some) z80 .COM files:

1. Hard reset BASIC (reset-esc with emulation running, where reset=PGUP on
   the PC keyboard).
2. From the menu system, load the binary file comload2.bin into the memory 
   range 0x0f000 to 0x0ffff.
3. In BASIC, type usr(61440) and press enter.
4. From the menu system, load the .COM file into the memory range 0x00100 
   to 0x07fff.
5. Return to the emulation.  If things have worked properly, the .COM
   program will now be running.

The .COM files included in the zip package have all been tested using this
method.

### Updates in v1.02.05.2005 (beta)
+ Major re-write of keyboard emulation.
+ Acceleration of piping to keyboard.
+ Added option to grab the contents of the screen.

### Updates in v1.11.04.2005 (beta)
+ Runs in a window by default.
+ Various bugs fixed.

### Updates in v1.27.03.2005 (beta)
+ Added ability to pipe direct to the keyboard.

### Updates in v1.22.03.2005 (beta)
+ Improved parallel port support.
+ Changed key to enter the GUI from PAUSE to PAGE DOWN.
+ Ported to windows.
+ Fixed many bugs.

### Updates since last version (pre 21-02-2005)
+ Totally re-wrote config file system.
+ Control keys remapped to allow for 256tc keyboard (see splash-screen for
  details).
+ New menuing system added (accessed using PAUSE key).
+ Partial parallel support now available (output only).
+ Improved support for various graphics modes.
+ CPU state and step mode options available.
+ + more (see splash screen and menus).

### Minimum requirements
Processor: Pentium II 233
Memory:    128MB RAM (although less would probably be fine too)
Harddrive: Not much.
OS:        - dos with extender loaded (dos version).
        or - win95 or win-more-recent (windows version).

### Copyright
PicoMozzy is gift-ware.  I hope you enjoy using it.  You can use,
redistribute, hack and modify it in any way you like.  For good karma, if
you redistribute PicoMozzy (or some product based on PicoMozzy), you could
acknowledge my input.  For even better karma, make a donation to a charity
of your choice and put something in your licence to encourage others to do
the same.  Finally, for maximum good karma, publish your source code ;).

### Known bugs
+ Sound - it's not perfect.  Shorter notes may have the wrong tone or be
  missed entirely.
+ Colour - the current version is a hack.  The foreground LUT needs to be
  corrected, and basic fails to detect the colour option (current hack fix
  is to start RAM with 0x099 written with 0x0ff (to indicate colour).  This
  is crude and will only work in basic).
+ Cursor - IIRC the 6545 allowed the cursor to wrap around.  This is
  currently disabled as a temporary fix for a related bug.
+ The DOS fork, if run under windows, won't work unless (a) the OS is
  win95 or win98 or (b) the program is run in full screen mode.  Even in
  full screen mode, resolutions higher than 640x480 are likely to fail in
  windows versions more recent than win98.
