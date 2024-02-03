
/*
   Microbee emulator => pc interface layer
   =======================================
*/


/*
   These macros define what is available in this environment:

   PARA_ACCESS_BIOS: parallel port can be accessed using the bios.
   PARA_ACCESS_HARD: parallel port can be accessed directly (hardware).

   SOUND_PCSPEAKER: audio is produced using the pc speaker
   SOUND_SOUNDCARD: audio is produced using the soundcard, via allegro.
*/

#ifdef IS_DJGPP
#define IS_ALLEGRO
#define PARA_ACCESS_BIOS
#define PARA_ACCESS_HARD
#ifdef DOSTEST_SOUNDCARD
#define SOUND_SOUNDCARD
#endif
#ifndef DOSTEST_SOUNDCARD
#define SOUND_PCSPEAKER
#endif
#endif

#ifdef IS_CYGWIN
#define IS_ALLEGRO
/*#define PARA_ACCESS_BIOS*/
/*#define PARA_ACCESS_HARD*/
#define SOUND_SOUNDCARD
#endif

#ifdef IS_WEB
/*#define PARA_ACCESS_BIOS*/
/*#define PARA_ACCESS_HARD*/
#endif


#define DEFAULT_STRLEN  1800


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef IS_CYGWIN
#include <conio.h>
#endif
#ifdef PARA_ACCESS_HARD
#include <pc.h>
#include <sys/movedata.h>
#endif
#ifdef PARA_ACCESS_BIOS
#include <bios.h>
#endif
#ifdef IS_ALLEGRO
#include <allegro.h>
#endif
#include "configer.h"
#include "debmaloc.h"
#include "interf.h"
#include "u_dtype.h"
#include "beefile.h"




/*
   Unlike other modules, there can be only one valid instance of this
   module.  The following var is set when this instance is allocated.
*/

int interf_is_alloced = 0;

/*
   As there is only one instance anyway, can just keep a global pointer for
   ease of use.  The two versions are identical - the volatile one is needed
   for use in an interupt context, the non-volatile one elsewhere to avoid
   warnings ;).
*/

volatile void *interf_indir = NULL;
void *interf_indir_nonvol   = NULL;

/*
   Macros are used to access data contained in the module structure so that
   modules can be reorganised easily without large-scale recoding.
*/

#define INTERF_GFX_PIXEL_BUS(what)      DEREF_8BUS(what,0)
#define INTERF_GFX_COL_BUS_FORE(what)   DEREF_8BUS(what,1)
#define INTERF_GFX_COL_BUS_BACK(what)   DEREF_8BUS(what,2)
#define INTERF_GFX_INVERSION_BUS(what)  DEREF_8BUS(what,3)
#define INTERF_KEY_LPEN_TABLE(what)     DEREF_8MEM(what,4)
#define INTERF_KEY_FEEDBACK_TABLE(what) DEREF_8MEM(what,5)
#define INTERF_KEY_FEEDRFSH_TABLE(what) DEREF_8MEM(what,6)
#define INTERF_PARA_DATA_BUS_IN(what)   DEREF_8BUS(what,7)
#define INTERF_PARA_STRB_BUS_IN(what)   DEREF_8BUS(what,8)
#define INTERF_PARA_RDY_BUS_IN(what)    DEREF_8BUS(what,9)
#define INTERF_PARA_DATA_BUS_OUT(what)  DEREF_8BUS(what,10)
#define INTERF_PARA_STRB_BUS_OUT(what)  DEREF_8BUS(what,11)
#define INTERF_PARA_RDY_BUS_OUT(what)   DEREF_8BUS(what,12)
#define INTERF_SND_BITSTATE(what)       DEREF_8BUS(what,13)
#define INTERF_TAPE_INSTATE(what)       DEREF_8BUS(what,14)
#define INTERF_TAPE_OUTSTATE(what)      DEREF_8BUS(what,15)

#define INTERF_GFX_LEFT_MARGIN(what)    DEREF_16BUS(what,0)
#define INTERF_GFX_SCRN_WIDTH(what)     DEREF_16BUS(what,1)
#define INTERF_GFX_RIGHT_MARGIN(what)   DEREF_16BUS(what,2)
#define INTERF_GFX_TOP_MARGIN(what)     DEREF_16BUS(what,3)
#define INTERF_GFX_SCRN_HEIGHT(what)    DEREF_16BUS(what,4)
#define INTERF_GFX_BOTTOM_MARGIN(what)  DEREF_16BUS(what,5)
#define INTERF_GFX_X_POS(what)          DEREF_16BUS(what,6)
#define INTERF_GFX_Y_POS(what)          DEREF_16BUS(what,7)

#define INTERF_KEY_RSET_COUNTER(what)   DEREF_32BUS(what,0)
#define INTERF_KEY_UPDAT_COUNTER(what)  DEREF_32BUS(what,1)

#define INTERF_SCRN_RFSH(what)          OUTFNCALL(what,0)
#define INTERF_CTRL_EXIT(what)          OUTFNCALL(what,1)
#define INTERF_CTRL_SPEEDCTRL_ON(what)  OUTFNCALL(what,2)
#define INTERF_CTRL_SPEEDCTRL_OFF(what) OUTFNCALL(what,3)
#define INTERF_CTRL_RESET_ON(what)      OUTFNCALL(what,4)
#define INTERF_CTRL_RESET_OFF(what)     OUTFNCALL(what,5)
#define INTERF_CTRL_PAUSE_EMU(what)     OUTFNCALL(what,6)
#define INTERF_CTRL_RESTART_EMU(what)   OUTFNCALL(what,7)
#define INTERF_PARA_STROBE(what)        OUTFNCALL(what,8)
#define INTERF_TAPE_STROBE(what)        OUTFNCALL(what,9)


/*
   Splashscreen specs
   ==================

   SPLASH_CHAR_HEIGHT: height of characters on the splashscreen.
   SPLASH_BOUNDARY: characters left around splash screen boundary.
*/

#define SPLASH_CHAR_HEIGHT      9
#define SPLASH_BOUNDARY         2


/*
   System specific options
   =======================

   interf_keyboard_pollmode: set if keyboard must be polled.
   interf_mouse_pollmode:    set if mouse must be polled.

   interf_snd_toggle_flag:  set to toggle sound emulation.
   interf_sig_rsetdwn_flag: set to start emulator reset.
   interf_sig_rsetup_flag:  set to end emulator reset.
   interf_scrn_rfsh_flag:   set to refresh the screen.
   interf_speedtoggle_flag: set to toggle speed emulation.
   interf_key_lowcall_flag: set to register keypress for keyboard emulation.
   interf_prtscr_flag:      set to cause the screen to be printed.
   interf_interact_flag:    set to enter interactive mode.

*/

int interf_keyboard_pollmode = 0;
int interf_mouse_pollmode    = 0;

volatile int interf_snd_toggle_flag  = 0;
volatile int interf_sig_rsetdwn_flag = 0;
volatile int interf_sig_rsetup_flag  = 0;
volatile int interf_scrn_rfsh_flag   = 0;
volatile int interf_speedtoggle_flag = 0;
volatile int interf_key_lowcall_flag = 0;
volatile int interf_prtscr_flag      = 0;
volatile int interf_interact_flag    = 0;

int interf_is_in_menu_mode = 0;
int interf_speed_emu_on    = 0;


#ifdef IS_WEB

/*
   Allegro KEY_* replacement macros
   ================================

   These are copied directly out of allegro.h.  It may be convenient to
   redefine these (values not important).  Also define the array key[]
   here.
*/


#define KB_NORMAL             1

#define KEY_A                 1
#define KEY_B                 2
#define KEY_C                 3
#define KEY_D                 4
#define KEY_E                 5
#define KEY_F                 6
#define KEY_G                 7
#define KEY_H                 8
#define KEY_I                 9
#define KEY_J                 10
#define KEY_K                 11
#define KEY_L                 12
#define KEY_M                 13
#define KEY_N                 14
#define KEY_O                 15
#define KEY_P                 16
#define KEY_Q                 17
#define KEY_R                 18
#define KEY_S                 19
#define KEY_T                 20
#define KEY_U                 21
#define KEY_V                 22
#define KEY_W                 23
#define KEY_X                 24
#define KEY_Y                 25
#define KEY_Z                 26
#define KEY_0                 27
#define KEY_1                 28
#define KEY_2                 29
#define KEY_3                 30
#define KEY_4                 31
#define KEY_5                 32
#define KEY_6                 33
#define KEY_7                 34
#define KEY_8                 35
#define KEY_9                 36
#define KEY_0_PAD             37
#define KEY_1_PAD             38
#define KEY_2_PAD             39
#define KEY_3_PAD             40
#define KEY_4_PAD             41
#define KEY_5_PAD             42
#define KEY_6_PAD             43
#define KEY_7_PAD             44
#define KEY_8_PAD             45
#define KEY_9_PAD             46
#define KEY_F1                47
#define KEY_F2                48
#define KEY_F3                49
#define KEY_F4                50
#define KEY_F5                51
#define KEY_F6                52
#define KEY_F7                53
#define KEY_F8                54
#define KEY_F9                55
#define KEY_F10               56
#define KEY_F11               57
#define KEY_F12               58
#define KEY_ESC               59
#define KEY_TILDE             60
#define KEY_MINUS             61
#define KEY_EQUALS            62
#define KEY_BACKSPACE         63
#define KEY_TAB               64
#define KEY_OPENBRACE         65
#define KEY_CLOSEBRACE        66
#define KEY_ENTER             67
#define KEY_COLON             68
#define KEY_QUOTE             69
#define KEY_BACKSLASH         70
#define KEY_BACKSLASH2        71
#define KEY_COMMA             72
#define KEY_STOP              73
#define KEY_SLASH             74
#define KEY_SPACE             75
#define KEY_INSERT            76
#define KEY_DEL               77
#define KEY_HOME              78
#define KEY_END               79
#define KEY_PGUP              80
#define KEY_PGDN              81
#define KEY_LEFT              82
#define KEY_RIGHT             83
#define KEY_UP                84
#define KEY_DOWN              85
#define KEY_SLASH_PAD         86
#define KEY_ASTERISK          87
#define KEY_MINUS_PAD         88
#define KEY_PLUS_PAD          89
#define KEY_DEL_PAD           90
#define KEY_ENTER_PAD         91
#define KEY_PRTSCR            92
#define KEY_PAUSE             93
#define KEY_ABNT_C1           94
#define KEY_YEN               95
#define KEY_KANA              96
#define KEY_CONVERT           97
#define KEY_NOCONVERT         98
#define KEY_AT                99
#define KEY_CIRCUMFLEX        100
#define KEY_COLON2            101
#define KEY_KANJI             102
#define KEY_MODIFIERS         103
#define KEY_LSHIFT            103
#define KEY_RSHIFT            104
#define KEY_LCONTROL          105
#define KEY_RCONTROL          106
#define KEY_ALT               107
#define KEY_ALTGR             108
#define KEY_LWIN              109
#define KEY_RWIN              110
#define KEY_MENU              111
#define KEY_SCRLOCK           112
#define KEY_NUMLOCK           113
#define KEY_CAPSLOCK          114
#define KEY_MAX               115

volatile char key[KEY_MAX];

#endif

/*
   Macros
   ======

   INTERF_INTERACT_KEY: key to be used to enter the microbee emulation menu
        system (interactive mode).  Should be KEY_PGDN.
   INTERF_SCRNDUMP_KEY: key to be used to initiate screen capture.  I would
        like to use KEY_PRTSCR, but this seems to be broken in dos.
        Screendump also possible through interactive menu.
   INTERF_RESET_KEY: mapping for the microbee's "reset" key.  Should be
        KEY_PGDN.
   INTERF_SCRNRFSH_KEY: key to be used to force the 6545 emulator to redraw
        the screen.  This should be KEY_PAUSE.  Currently disabled due to
        KEY_PAUSE weirdness.
   INTERF_SPEEDCTRL_KEY: key to toggle the emulator between "bee speed" (ie.
        emulated to run at 3.141MHz" or "as fast as possible".  Should be
        KEY_SCRLOCK to line up with the use of the scroll lock led to
        indicate speed state.
   INTERF_SOUNDCNTRL_KEY: key to toggle sound emulation off and on.  Should
        be KEY_NUMLOCK to line up with the use of the num lock led to
        indicate whether sound emulation is on or off.
   DEFAULT_SCREENDUMP_FILE: default screendump filename (this is the file
        created by using the prtscr button when in menu mode).
*/

#define INTERF_INTERACT_KEY     KEY_PGDN
#define INTERF_SCRNDUMP_KEY     KEY_PRTSCR
#define INTERF_RESET_KEY        KEY_PGUP
#define INTERF_SCRNRFSH_KEY     KEY_PAUSE
#define INTERF_SPEEDCTRL_KEY    KEY_SCRLOCK
#define INTERF_SOUNDCNTRL_KEY   KEY_NUMLOCK
#define DEFAULT_SCREENDUMP_FILE "screen.bmp"




/*
   Setup variables
   ===============

   The following variables are loaded when interf_init is called.  They
   are stored in interf_setdat for this reason (loading is done by passing
   this variable to load_config_file() (see configer.h for details).


   interf_scrn_video_mode: Defines the prefered video mode to be used under
        allegro.
         +--------+------------+------------+--------------+----------+
         | Screen |   Screen   | Vertically | Horizontally | Use line |
         |  mode  | resolution | stretched  |  stretched   | doubling |
         +--------+------------+------------+--------------+----------+
         |   0    | 640x480    | no         | no           | n/a      |
         |   1    | 800x600    | yes        | no           | yes      |
         |   2    | 1024x768   | yes        | no           | yes      |
         |   3    | 1280x1024  | yes        | yes          | yes      |
         |   4    | 800x600    | yes        | no           | no       |
         |   5    | 1024x768   | yes        | no           | no       |
         |   6    | 1280x1024  | yes        | yes          | no       |
         |   7    | 800x600    | no         | no           | n/a      |
         |   8    | 1024x768   | no         | no           | n/a      |
         |   9    | 1280x1024  | no         | no           | n/a      |
         +--------+------------+------------+--------------+----------+
        If the specified mode is no available will default to mode 0
        (640x480), which must be present.  
   interf_scrn_pref_fullscrn: If 0 then use a window if possible for the
        screen mode specified.  Otherwise (ie. nz) use fullscreen mode if
        possible.  Note that this only defines preference.  If the
        preferred option is not available, will default to whatever can
        be used to achieve the video mode required.
   interf_scrn_monitor_type: Defines the monitor type.
         +---------+---------+------------+------------+
         | Monitor | Colour? | Foreground | Background |
         |   Type  |         |   Colour   |   Colour   |
         +---------+---------+------------+------------+
         |    0    |   Yes   |    n/a     |    n/a     |
         |    1    |   No    |   green    |   black    |
         |    2    |   No    |   amber    |   black    |
         |    3    |   No    |   white    |   black    |
         |    4    |   No    |   black    |   green    |
         |    5    |   No    |   black    |   amber    |
         |    6    |   No    |   black    |   white    |
         +---------+---------+------------+------------+
   interf_scrn_bright: sets default brightness (0 min, 255 max).
   interf_scrn_contrast: sets the contrast (0 min, 255 max).

   interf_snd_sndon: If nz then sound will be emulated.
   interf_snd_sndclk_period: Clock period (ns) upon which all frequency
        estimations are based (if estimation is used).
   interf_snd_minfreq: If frequency estimation is being used, this is the
        lowest frequency which will be played.
   interf_snd_clkcnterr_numer: \ To minimise frequency jitter, if a sound
   interf_snd_clkcnterr_denom: / is being played then the frequency will be
        fixed unless there is a significant change in the estimated
        underlying frequency.  Specifically, unless the period of the sound
        (in underlying clock cycles) is more than numer*first_count/denom
        different from first_cound (the cycle count when the frequency
        started being played) the frequency being played will not change.
   interf_snd_clkcnt_max:   \ If the clock count exceeds clkcnt_max between
   interf_snd_clkcnt_fback: / speaker state changes then it will be reset
        to clkcnt_fback.


   interf_key_rfsh_cycles: number of clock cycles between keyboard updates.
   interf_key_clkcnt_max: When a key is pressed, a counter associated with
        it is set to this value.  This counter is then decremented every
        keyboard update, and when it is 0 and the key has been lifted the
        emulated key will be lifted (the key can also be lifted if feedback
        is received, via the count tables discussed elsewhere, to say that
        the keystroke has been registered).  This effectively makes very
        short keypresses "stick" until registered (via feedback method),
        but not for too long (where this defines how many clock cycles "too
        long" is).


   interf_tape_autosave_mode: defines how tape output is dealt with by
        default.  If 0 then any output will be dealt with in the same was
        as a pipe operation (useful to dump headers in save operations).
        Otherwise, the output type will be autodetected and dynamically
        redirected to the file given by the filename and extension in the
        tape header.


   interf_para_lptnum: Parallel port number (bios number).
   interf_para_lprport: This may be used to directly select the base io
        address in direct hardware access mode.  If this is 0 on entry
        then it will be set according to interf_para_lptnum.  Otherwise
        direct accesses will be relative to the base io port specified
        by this.
   interf_para_sim_pulse: Controls strobe pulse emulation in direct hardware
        modes.  Specifically, if this is 0 then parallel port pins directly
        reflect what they would on a really old microbee - no fancy stuff
        done.  Pin assignments on the PC are as follows:
                   +------------+
                   |  PC pin #  |
                   | (parallel) |
                   +------------+-------------------------+
                   |     1      | microbee pin 6   ARDY   |
                   |     2      | microbee pin 5   DATA 0 |
                   |     3      | microbee pin 12  DATA 1 |
                   |     4      | microbee pin 4   DATA 2 |
                   |     5      | microbee pin 11  DATA 3 |
                   |     6      | microbee pin 3   DATA 4 |
                   |     7      | microbee pin 10  DATA 5 |
                   |     8      | microbee pin 2   DATA 6 |
                   |     9      | microbee pin 7   DATA 7 |
                   |     10     | microbee pin 15  ASTRB  |
                   |     11-13  | unused                  |
                   |     14     | logic high              |
                   |     15     | unused                  |
                   |     16-17  | logic high              |
                   |     18-25  | 0V                      |
                   +------------+-------------------------+
        If this is nonzero then pins are set as they would be on a later
        microbee, with simulated printer cable.  This is what you want if
        you want to be able to connect directly to a printer.  Pin
        assignments on the PC are as follows:
                   +------------+
                   |  PC pin #  |
                   | (parallel) |
                   +------------+-------------------------+
                   |     1      | microbee pin 6   PSTB   |
                   |            | (n/c on early bees)     |
                   |     2      | microbee pin 5   DATA 0 |
                   |     3      | microbee pin 12  DATA 1 |
                   |     4      | microbee pin 4   DATA 2 |
                   |     5      | microbee pin 11  DATA 3 |
                   |     6      | microbee pin 3   DATA 4 |
                   |     7      | microbee pin 10  DATA 5 |
                   |     8      | microbee pin 2   DATA 6 |
                   |     9      | microbee pin 7   DATA 7 |
                   |     10     | microbee pin 15  ASTRB  |
                   |            | (pulsed)                |
                   |     11-13  | unused                  |
                   |     14     | logic high              |
                   |     15     | unused                  |
                   |     16-17  | logic high              |
                   |     18-25  | 0V                      |
                   +------------+-------------------------+
   interf_para_pulse_len: length (in timer_period ms, +/- 1 unit) of ready
        pulse, if emulated (direct hardware mode only).
   interf_para_responsetime_out: # z80 cycles "wait" before z80 out is read.
   interf_para_responsetime_in: # z80 cycles "wait" before z80 input writes.
   interf_para_strobe_time: # z80 cycles for a strobe signal.
   interf_para_readgranularity: # calls to interf_cycle between checks
        (direct hardware mode (mode 0) only).
*/

int    interf_scrn_video_mode    = 0;
int    interf_scrn_pref_fullscrn = 1;
int    interf_scrn_monitor_type  = 0;
UINT_8 interf_scrn_bright        = 255;
UINT_8 interf_scrn_contrast      = 75;

int     interf_snd_sndon           = 1;
UINT_32 interf_snd_sndclk_period   = 298;
UINT_32 interf_snd_minfreq         = 200;
UINT_32 interf_snd_clkcnterr_numer = 1;
UINT_32 interf_snd_clkcnterr_denom = 2;
UINT_64 interf_snd_clkcnt_max      = 0x0dfffffff;
UINT_64 interf_snd_clkcnt_fback    = 0x00dffffff;

UINT_8  interf_key_clkcnt_max  = 15;
UINT_32 interf_key_rfsh_cycles = 200;

int interf_tape_autosave_mode = 1;

int     interf_para_lptnum           = 1;
int     interf_para_lptport          = 0;
int     interf_para_sim_pulse        = 1;
int     interf_para_pulse_len        = 2;
UINT_32 interf_para_responsetime_out = 5;
UINT_32 interf_para_responsetime_in  = 500000;
UINT_32 interf_para_strobe_time      = 15;
UINT_32 interf_para_readgranularity  = 50;

SetupData interf_setdat[] =
{
    { "screen_mode",                &interf_scrn_video_mode,       6, 0,   9           },
    { "prefer_fullscreen",          &interf_scrn_pref_fullscrn,    6, 0,   1           },
    { "mono_type",                  &interf_scrn_monitor_type,     6, 0,   6           },
    { "bright",                     &interf_scrn_bright,           0, 0,   255         },
    { "contrast",                   &interf_scrn_contrast,         0, 0,   255         },
    { "do_sound",                   &interf_snd_sndon,             6, 0,   1           },
    { "snd_clock_period_snd",       &interf_snd_sndclk_period,     2, 1,   10000       },
    { "snd_min_freq",               &interf_snd_minfreq,           2, 1,   20000       },
    { "snd_freq_count_error_numer", &interf_snd_clkcnterr_numer,   2, 1,   255         },
    { "snd_freq_count_error_denom", &interf_snd_clkcnterr_denom,   2, 1,   255         },
    { "snd_max_clock_cnnt",         &interf_snd_clkcnt_max,        9, 1,   0x0ffffffff },
    { "snd_max_clock_cnnt_reset",   &interf_snd_clkcnt_fback,      9, 1,   0x0ffffffff },
    { "key_count_start",            &interf_key_clkcnt_max,        0, 0,   1024        },
    { "key_refresh_cycles",         &interf_key_rfsh_cycles,       2, 0,   1024        },
    { "tape_autosave",              &interf_tape_autosave_mode,    6, 0,   1           },
    { "pc_lpt_num",                 &interf_para_lptnum,           6, 0,   255         },
    { "pc_lpt_port",                &interf_para_lptport,          6, 0,   255         },
    { "simulate_lpt_pulse",         &interf_para_sim_pulse,        6, 0,   1           },
    { "pc_lpt_pulse_len",           &interf_para_pulse_len,        6, 0,   1024        },
    { "Parallel_response_time_out", &interf_para_responsetime_out, 2, 0,   65535       },
    { "Parallel_response_time_in",  &interf_para_responsetime_in,  2, 0,   0x0ffffffff },
    { "Parallel_strobe_time",       &interf_para_strobe_time,      2, 0,   1024        },
    { "Parallel_read_granularity",  &interf_para_readgranularity,  2, 1,   1024        },
    { "", NULL, 0, 0, 0 }
};




/*
   Screen Position and size
   ========================

   The size and geometry of the screen is:

      (0,0)-------(bw,0)-----------(bw+sw,0)--------(PSW,0)
        |                                               |
      (0,bh)      (bw,bh)----------(bw+sw,bh)       (PSW,bh)
        |            |*******************|              |
        |            |****Bee screen*****|              |
        |            |*******************|              |
      (0,bh+sh)   (bw,bh+sh)-------(bw+sw,bh+sh)   (PSW,bh+sh)
        |                                               |
      (0,PSH)-----(bw,PSH)---------(bw+sw,PSH)------(PSW,PSH)


   where: bh = interf_scrn_l_top_offset*interf_scrn_vert_line_mult
          bw = interf_scrn_l_left_offset*interf_scrn_horiz_line_mult
          sh = interf_scrn_l_s_height*interf_scrn_vert_line_mult
          sw = interf_scrn_l_s_width*interf_scrn_horiz_line_mult

          PSW = interf_scrn_physical_width-1
          PSH = interf_scrn_physical_height-1


   COLOUR PALLETTE
   ===============

   Colour depth is 8 bits.  These colours are programmed in the pallette as
   follows.  First, bit 7 is always set (allegro considers 00 see-through).
   Otherwise, the low 6 bits are the standard microbee colour selections:

   bit 0: RED   intensity
   bit 1: GREEN intensity
   bit 2: BLUE  intensity
   bit 3: RED
   bit 4: GREEN
   bit 5: BLUE

   The following table shows the explicit colour definitions.

         Red gun + Green gun + Blue gun     = colour (if defined)

    80 -                                    = black
    81 -                                    = black
    82 -                                    = black
    83 -                                    = black
    84 -                                    = black
    85 -                                    = black
    86 -                                    = black
    87 -                                    = black
    88 -                       Blue half    = blue
    89 -                       Blue full    = blue II
    8A -                       Blue half    = blue
    8B -                       Blue full    = blue II
    8C -                       Blue half    = blue
    8D -                       Blue full    = blue II
    8E -                       Blue half    = blue
    8F -                       Blue full    = blue II
    90 -           Green half               = green
    91 -           Green half               = green
    92 -           Green full               = green II
    93 -           Green full               = green II
    94 -           Green half               = green
    95 -           Green half               = green
    96 -           Green full               = green II
    97 -           Green full               = green II
    98 -           Green half, Blue half    = cyan
    99 -           Green half, Blue full
    9A -           Green full, Blue half    
    9B -           Green full, Blue full    = cyan II
    9C -           Green half, Blue half    = cyan
    9D -           Green half, Blue full
    9E -           Green full, Blue half    
    9F -           Green full, Blue full    = cyan II
    A0 - Red half                           = red
    A1 - Red half                           = red
    A2 - Red half                           = red
    A3 - Red half                           = red
    A4 - Red full                           = red II
    A5 - Red full                           = red II
    A6 - Red full                           = red II
    A7 - Red full                           = red II
    A8 - Red half,             Blue half    = magenta
    A9 - Red half,             Blue full
    AA - Red half,             Blue half    = magenta
    AB - Red half,             Blue full
    AC - Red full,             Blue half    
    AD - Red full,             Blue full    = magenta II
    AE - Red full,             Blue half    
    AF - Red full,             Blue full    = magenta II
    B0 - Red half, Green half               = yellow
    B1 - Red half, Green half               = yellow
    B2 - Red half, Green full
    B3 - Red half, Green full
    B4 - Red full, Green half
    B5 - Red full, Green half
    B6 - Red full, Green full               = yellow II
    B7 - Red full, Green full               = yellow II
    B8 - Red half, Green half, Blue half    = white
    B9 - Red half, Green half, Blue full
    BA - Red half, Green full, Blue half
    BB - Red half, Green full, Blue full
    BC - Red full, Green half, Blue half
    BD - Red full, Green half, Blue full
    BE - Red full, Green full, Blue half
    BF - Red full, Green full, Blue full    = white II


   Screen Emulator Data
   ====================

   INTERF_SCRN_COLOUR_BLACK:  default background colour in monochrome mode.
   INTERF_SCRN_COLOUR_GREEN:  default foreground colour in greenscreen mode.
   INTERF_SCRN_COLOUR_AMBER:  default foreground colour in amberscreen mode.
   INTERF_SCRN_COLOUR_WHITE:  default foreground colour in b/w mode.
   INTERF_SCRN_COLOUR_OFFSET: amount by which "microbee" colours must be
        offset to get colours in the allegro pallete.  This basically just
        makes sure that bit 7 is set, preventing the accidental use of
        "see-through" colours.

   INTERF_SCRN_MAX_SCRNWIDTH_BEE: maximum screen width supported by the 6545
   INTERF_SCRN_MAX_SCRNHIGHT_BEE: maximum screen height supported by the 6545


   interf_scrn_mono_forecolour: colour for "foreground" pixels in mono.
   interf_scrn_mono_backcolour: colour for "background" pixels in mono.

   interf_scrn_physical_width:  width of the physical pc screen.
   interf_scrn_physical_height: height of the physical pc screen.

   interf_scrn_horiz_line_mult: 1 microbee col -> this # physical cols
   interf_scrn_vert_line_mult:  1 microbee row -> this # physical rows
   interf_scrn_multip_fill:     if set then rows are duplicated for the
        multipliers.  Otherwise, only 1 line is drawn for every multiplier.

   interf_scrn_bee_screen:   A local copy of the screen is kept here.
   interf_scrn_palette[256]: Colour pallette.

   interf_scrn_l_left_offset:  # pixels on screen to left of 6545 screen.
   interf_scrn_l_s_width:      width of bee screen addressed by the 6545.
   interf_scrn_l_right_offset: # pixels on screen to right of 6545 screen.

   interf_scrn_l_top_offset:    # pixels on screen to top of 6545 screen.
   interf_scrn_l_s_height:      height of bee scrn addressed by the 6545.
   interf_scrn_l_bottom_offset: # pixels on scrn to bottom of 6545 screen.

   interf_scrn_left_correct: "0"
   interf_scrn_top_correct:  "0"

   interf_scrn_lsource: proof that documenting as you go is a good idea.
   interf_scrn_rsource: proof that documenting as you go is a good idea.
   interf_scrn_tsource: proof that documenting as you go is a good idea.
   interf_scrn_bsource: proof that documenting as you go is a good idea.

   interf_scrn_stepmode: set if in step mode (returns to menu whenever the
        cycle function is called).

   interf_scrn_colour_full: full colour intensity level (half->63)
   interf_scrn_colour_half: half colour intensity level (back->full)
   interf_scrn_colour_back: back colour intensity level (0->half)

   interf_scrn_colour_***: (where * = o or l or h). Colour definition where
        guns RGB are given by ***, where o represents background intensity,
        l represents half intensity and h full intensity, as given by the
        above intensity levels.
   interf_scrn_full_colour_***: like interf_scrn_colour_***, except that
        levels 0,l and h are set permanently to 0,31 and 63, respectively.


   Screen Emulator Functions
   =========================

   interf_scrn_set_left_margin:   infn0
   interf_scrn_set_screen_width:  infn1
   interf_scrn_set_right_margin:  infn2
   interf_scrn_set_top_margin:    infn3
   interf_scrn_set_screen_height: infn4
   interf_scrn_set_bottom_margin: infn5
   interf_scrn_8pixel_draw:       infn6

   interf_scrn_blank_screen:   clear the screen.
   interf_scrn_refresh_screen: refresh (redraw) the screen
   interf_scrn_set_bright:     set brightness (0 min, 255 max)
   interf_scrn_set_contrast:   set contrast (0 min, 255 max)

   interf_scrn_fix_palette: sets interf_scrn_colour_*** appropriately using
        interf_scrn_colour_full/half/back.
   interf_scrn_correct_scrnsize: correct for change in size of the physical
        screen.

   interf_scrn_set_gfxmode: set graphics mode to match setting (if possible).
   interf_scrn_screenshot: save contents of microbee screen to a file.
   interf_scrn_set_vidmode: set video mode variables.
*/

#ifdef IS_ALLEGRO
#define INTERF_SCRN_COLOUR_BLACK        0x000
#define INTERF_SCRN_COLOUR_GREEN        0x011
#define INTERF_SCRN_COLOUR_AMBER        0x031
#define INTERF_SCRN_COLOUR_WHITE        0x038
#define INTERF_SCRN_COLOUR_OFFSET       0x080
#endif

#ifdef IS_WEB
#define INTERF_SCRN_COLOUR_BLACK        0x000
#define INTERF_SCRN_COLOUR_GREEN        0x011
#define INTERF_SCRN_COLOUR_AMBER        0x031
#define INTERF_SCRN_COLOUR_WHITE        0x038
#define INTERF_SCRN_COLOUR_OFFSET       0x080
#endif

#define INTERF_SCRN_MAX_SCRNWIDTH_BEE   2048
#define INTERF_SCRN_MAX_SCRNHIGHT_BEE   4096

UINT_8 interf_scrn_mono_forecolour = 0;
UINT_8 interf_scrn_mono_backcolour = 0;

UINT_16 interf_scrn_physical_width  = 0;
UINT_16 interf_scrn_physical_height = 0;

UINT_16 interf_scrn_horiz_line_mult = 0;
UINT_16 interf_scrn_vert_line_mult  = 0;
int     interf_scrn_multip_fill     = 0;

#ifdef IS_ALLEGRO
BITMAP *interf_scrn_bee_screen;
RGB     interf_scrn_palette[256];
#endif

UINT_16 interf_scrn_l_left_offset  = 0;
UINT_16 interf_scrn_l_s_width      = 1;
UINT_16 interf_scrn_l_right_offset = 0;

UINT_16 interf_scrn_l_top_offset    = 0;
UINT_16 interf_scrn_l_s_height      = 1;
UINT_16 interf_scrn_l_bottom_offset = 0;

UINT_16 interf_scrn_left_correct = INTERF_SCRN_MAX_SCRNWIDTH_BEE/2;
UINT_16 interf_scrn_top_correct  = INTERF_SCRN_MAX_SCRNHIGHT_BEE/2;

UINT_16 interf_scrn_lsource = 0;
UINT_16 interf_scrn_rsource = 0;
UINT_16 interf_scrn_tsource = 0;
UINT_16 interf_scrn_bsource = 0;

int interf_scrn_stepmode = 0;

UINT_8 interf_scrn_colour_full = 63;
UINT_8 interf_scrn_colour_half = 31;
UINT_8 interf_scrn_colour_back = 0;

#ifdef IS_ALLEGRO
RGB interf_scrn_colour_ooo = { 0,  0,  0  ,0 };
RGB interf_scrn_colour_ool = { 0,  0,  31 ,0 };
RGB interf_scrn_colour_ooh = { 0,  0,  63 ,0 };
RGB interf_scrn_colour_olo = { 0,  31, 0  ,0 };
RGB interf_scrn_colour_oll = { 0,  31, 31 ,0 };
RGB interf_scrn_colour_olh = { 0,  31, 63 ,0 };
RGB interf_scrn_colour_oho = { 0,  63, 0  ,0 };
RGB interf_scrn_colour_ohl = { 0,  63, 31 ,0 };
RGB interf_scrn_colour_ohh = { 0,  63, 63 ,0 };
RGB interf_scrn_colour_loo = { 31, 0,  0  ,0 };
RGB interf_scrn_colour_lol = { 31, 0,  31 ,0 };
RGB interf_scrn_colour_loh = { 31, 0,  63 ,0 };
RGB interf_scrn_colour_llo = { 31, 31, 0  ,0 };
RGB interf_scrn_colour_lll = { 31, 31, 31 ,0 };
RGB interf_scrn_colour_llh = { 31, 31, 63 ,0 };
RGB interf_scrn_colour_lho = { 31, 63, 0  ,0 };
RGB interf_scrn_colour_lhl = { 31, 63, 31 ,0 };
RGB interf_scrn_colour_lhh = { 31, 63, 63 ,0 };
RGB interf_scrn_colour_hoo = { 63, 0,  0  ,0 };
RGB interf_scrn_colour_hol = { 63, 0,  31 ,0 };
RGB interf_scrn_colour_hoh = { 63, 0,  63 ,0 };
RGB interf_scrn_colour_hlo = { 63, 31, 0  ,0 };
RGB interf_scrn_colour_hll = { 63, 31, 31 ,0 };
RGB interf_scrn_colour_hlh = { 63, 31, 63 ,0 };
RGB interf_scrn_colour_hho = { 63, 63, 0  ,0 };
RGB interf_scrn_colour_hhl = { 63, 63, 31 ,0 };
RGB interf_scrn_colour_hhh = { 63, 63, 63 ,0 };

RGB interf_scrn_full_colour_ooo = { 0,  0,  0  ,0 };
RGB interf_scrn_full_colour_lll = { 31, 31, 31 ,0 };
RGB interf_scrn_full_colour_hlo = { 63, 31, 0  ,0 };
RGB interf_scrn_full_colour_hhh = { 63, 63, 63 ,0 };
RGB interf_scrn_full_colour_xxx = { 15, 15, 15 ,0 };
#endif


void interf_scrn_set_left_margin(void *what);
void interf_scrn_set_screen_width(void *what);
void interf_scrn_set_right_margin(void *what);
void interf_scrn_set_top_margin(void *what);
void interf_scrn_set_screen_height(void *what);
void interf_scrn_set_bottom_margin(void *what);
void interf_scrn_8pixel_draw(void *what);

void interf_scrn_blank_screen(void);
void interf_scrn_refresh_screen(void);
void interf_scrn_set_bright(UINT_8 what);
void interf_scrn_set_contrast(UINT_8 what);

void interf_scrn_fix_palette(void);
void interf_scrn_correct_scrnsize(void);

int  interf_scrn_set_gfxmode(void);
int  interf_scrn_screenshot(const char shotname[]);
int  interf_scrn_set_vidmode(int what);





/*
   Parallel Port Emulation
   =======================

   interf_para_mode: defines parallel port operation mode.  Available modes
        depend on the available hardware, but are a subset of:
         +------+--------------------------------------------------+
         | mode | definition                                       |
         +------+--------------------------------------------------+
         |  0   | output to pc parallel port (direct to hardware). |
         |  1   | output to file.                                  |
         |  2   | output to /dev/null.                             |
         |  3   | input from file.                                 |
         |  4   | unconnected.                                     |
         |  5   | output to pc parallel port (via bios functions). |
         +------+--------------------------------------------------+

   interf_para_state: defines the state of the parallel port emulation.  The
        meaning of this int is dependent on the mode of operation of the
        parallel port.  This byte is not relevant to mode 0, as this takes
        all information direct to/from the parallel port pins itelf.  In
        modes 1,2,5 (ie. output modes), states are:
         +-------+------------------------------------+
         | state |                                    |
         +-------+------------------------------------+
         |   0   | waiting for data from pio.         |
         |   1   | data received, waiting to respond. |
         |   2   | responded, waiting to end strobe.  |
         +-------+------------------------------------+
        In modes 3,4 (input modes), states are:
         +-------+-------------------------------------------+
         | state |                                           |
         +-------+-------------------------------------------+
         |   0   | waiting for pio to give the all clear.    |
         |   1   | all clear received, waiting to start.     |
         |   2   | data given->pio, strobe low, wait to end. |
         +-------+-------------------------------------------+

   interf_para_cycle_cnt: general clock cycle counter variable.
   interf_para_upstat_cnt: counts calls to interf_cycle in mode 0.  When
        this counter exceeds interf_para_readgranularity then checks to
        the pc parallel port are carried out.

   interf_para_dest_fp: file output is to be sent to (mode 1).
   interf_para_src_fp: file input is to be taken from (mode 3).

   interf_para_filename_dest: filename and path for destination file, kept
        global to ensure persistence of the path.
   interf_para_filename_src: filename and path for source file, kept global
        to ensure persistence of the path.

   interf_para_havepulsed:  \ These are used to control pulse emulation when
   interf_para_havestrobed: / directly accessing parallel port hardware on
        the pc.  Usually, both are 0.  When a character is sent to be put
        on the parallel port the strobe is set low and interf_para_havepulsed
        is set.  Next, when the printer (or whatever is attached to the
        parallel port) sends a response, the strobe is set low and
        interf_para_havestrobed is set.  The response is passed back to the
        emulator, and it then responds appropriately, setting both of these
        low.  Thus only 1 pulse is sent per char.


   Functions
   =========

   interf_para_data_written: infn7

   interf_para_set_mode0: set parallel port mode 0.
   interf_para_set_mode1: set parallel port mode 1 (filename given).
   interf_para_set_mode2: set parallel port mode 2.
   interf_para_set_mode3: set parallel port mode 3 (filename given).
   interf_para_set_mode4: set parallel port mode 4.
   interf_para_set_mode5: set parallel port mode 5.


   Windows parallel/serial:

   http://www.geocities.com/firepower_50ae/CodeNote/WinAPIfaq.html

   (mingw32 faq)
*/

int interf_para_mode  = 4;
int interf_para_state = 0;

UINT_32 interf_para_cycle_cnt  = 0;
UINT_8  interf_para_upstat_cnt = 0;

char interf_para_filename_dest[DEFAULT_STRLEN] = "";
char interf_para_filename_src[DEFAULT_STRLEN]  = "";

PC_FILE *interf_para_dest_fp = NULL;
PC_FILE *interf_para_src_fp  = NULL;

int interf_para_havepulsed   = 0;
int interf_para_havestrobed  = 0;
int interf_para_trigpulsecnt = 0;

void interf_para_data_written(void *what);

int interf_para_set_mode0(void);
int interf_para_set_mode1(const char *filename);
int interf_para_set_mode2(void);
int interf_para_set_mode3(const char *filename);
int interf_para_set_mode4(void);
int interf_para_set_mode5(void);



/*
   Keyboard Emulation
   ==================

   (this should be made less microbee specific)

   The microbee keyboard circuitry is connected directly to the 6545 CRTC
   controller and, in particular, the light pen circuitry.  In the microbee
   circuit, the keys are arranged in a grid whose edges are selected by
   MA7,8,9 and MA4,5,6.  The grid is enabled as discussed below.  When the
   grid is enabled, a high goes on the column of the grid selected by
   MA7,8,9. A row is selected by MA4,5,6, and if the key at the intersection
   of these two is selected then the lightpen will be triggered, storing the
   contents of the MA bus in the lightpen register (and updating the
   relevant bits are switched to signal that the read has occured - namely
   LPEN_REGISTER_FULL and UPDATE_READY).

   The grid is set up as follows:



                            +------------------------------------+
    MA7,8,9 ================|                                    |
                            | 74ls156 based open collector demux |
                            | (ie. output pulled low if selected |
                            | by MA7,8,9 and grid_en* = 0).      |
    grid_en* --------------O|                                    |
                            +------------------------------------+
                             O    O    O    O    O    O    O    O
                             |0   |1   |2   |3   |4   |5   |6   |7
      (outputs pulled high)  |    |    |    |    |    |    |    |
                             |    |    |    |    |    |    |    | 
                 +-----+     |    |    |    |    |    |    |    | 
                 |     |---- @`-- H -- P -- X -- 0 -- 8(--ESC   |
                 |     |0    |    |    |    |    |    |    |    |
                 |     |     |    |    |    |    |    |    |    |
                 |     |---- A -- I -- Q -- Y -- 1!-- 9)-- BS--CTRL
    MA4,5,6 =====|     |1    |    |    |    |    |    |    |    |
              sel|     |     |    |    |    |    |    |    |    |
                 |     |---- B -- J -- R -- Z -- 2"-- :*--TAB   |
                 |     |2    |    |    |    |    |    |    |    |
                 |     |     |    |    |    |    |    |    |    |
                 |     |---- C -- K -- S -- [{-- 3#-- ;+-- LF   |
                 | mux |3    |    |    |    |    |    |    |    |
                 |     |     |    |    |    |    |    |    |    |
                 |     |---- D -- L -- T -- \|-- 4$-- ,<-- CR   |
                 |     |4    |    |    |    |    |    |    |    |
    lpen -------O|     |     |    |    |    |    |    |    |    |
    output*      |     |---- E -- M -- U -- ]}-- 5%-- -=--LOCK  |
                 |     |5    |    |    |    |    |    |    |    |
                 |     |     |    |    |    |    |    |    |    |
                 |     |---- F -- N -- V -- ^~-- 6&-- .>--BRK   |
                 |     |6    |    |    |    |    |    |    |    |
                 |     |     |    |    |    |    |    |    |    |
                 |     |---- G -- O -- W --DEL-- 7'-- /?--SPC--SHIFT
                 |     |7
                 +-----+

   Grid enabling is controlled by three signals:

    ROMREAD*:  This signal comes from bit 1 of port 0B, and is also used to
               veiw character ROM.  This is just romread.
    DISPEN:    Comes from the 6545.  It is high during the visible display,
               low during the blanking interval.
    CR4(USTB): The update strobe comes from the 6545.  It will either occur
               when showing char scanlines >= 11h (doesn't happen on the
               microbee under normal conditions) or to initiate an update
               strobe, as a result of an accress to R31 when the state of
               the 6545 is correct.

   These are combined in the following circuit:

                                         +-----+
    CR4(UPDATE STROBE 6545) -------------|     |
                                         |     |        a low here enables
                                         | NOR |------- the grid, which puts
                                         |     |        a high on the column
    DISPEN(6545) -----/\/\/-----+--------|     |        selected by MA7,8,9
                                |        +-----+
                                |
    ROMREAD* ---------|<|-------+

   which has the following truth table:

       ROMREAD* CR4  DISPEN   grid_en*   enabled?
          0      0     0         1         no
          0      0     1         1         no
          0      1     0         0         yes
          0      1     1         0         yes
          1      0     0         1         no
          1      0     1         0         yes
          1      1     0         0         yes
          1      1     1         0         yes

   or, correcting the inverted romread signal:

       ROMREAD  CR4  DISPEN   grid_en    enabled?
          0      0     0         0         no
          0      0     1         1         yes
          0      1     0         1         yes
          0      1     1         1         yes
          1      0     0         0         no
          1      0     1         0         no
          1      1     0         1         yes
          1      1     1         1         yes

   For speed reasons, the calling of the keyboard function is controlled by
   the 6545 emulator.  This relies on the mask c6545_lpen_call_mask, which
   has bits assigned thusly:

   +-----+---------------------------------------+
   |     | 6545 signal combination to test bit   |  Mask given romread state
   |     +-----+--------+-------+-------+--------+
   | bit | CR4 | DISPEN | HSYNC | VSYNC | CURSOR |    ROMREAD0   ROMREAD1
   +-----+-----+--------+-------+-------+--------+
   |  0  |  0  |   0    |   0   |   0   |   0    |       0          0
   |  1  |  0  |   0    |   0   |   0   |   1    |       0          0
   |  2  |  0  |   0    |   0   |   1   |   0    |       0          0
   |  3  |  0  |   0    |   0   |   1   |   1    |       0          0
   |  4  |  0  |   0    |   1   |   0   |   0    |       0          0
   |  5  |  0  |   0    |   1   |   0   |   1    |       0          0
   |  6  |  0  |   0    |   1   |   1   |   0    |       0          0
   |  7  |  0  |   0    |   1   |   1   |   1    |       0          0
   |  8  |  0  |   1    |   0   |   0   |   0    |       1          0
   |  9  |  0  |   1    |   0   |   0   |   1    |       1          0
   | 10  |  0  |   1    |   0   |   1   |   0    |       1          0
   | 11  |  0  |   1    |   0   |   1   |   1    |       1          0
   | 12  |  0  |   1    |   1   |   0   |   0    |       1          0
   | 13  |  0  |   1    |   1   |   0   |   1    |       1          0
   | 14  |  0  |   1    |   1   |   1   |   0    |       1          0
   | 15  |  0  |   1    |   1   |   1   |   1    |       1          0
   | 16  |  1  |   0    |   0   |   0   |   0    |       1          1
   | 17  |  1  |   0    |   0   |   0   |   1    |       1          1
   | 18  |  1  |   0    |   0   |   1   |   0    |       1          1
   | 19  |  1  |   0    |   0   |   1   |   1    |       1          1
   | 20  |  1  |   0    |   1   |   0   |   0    |       1          1
   | 21  |  1  |   0    |   1   |   0   |   1    |       1          1
   | 22  |  1  |   0    |   1   |   1   |   0    |       1          1
   | 23  |  1  |   0    |   1   |   1   |   1    |       1          1
   | 24  |  1  |   1    |   0   |   0   |   0    |       1          1
   | 25  |  1  |   1    |   0   |   0   |   1    |       1          1
   | 26  |  1  |   1    |   0   |   1   |   0    |       1          1
   | 27  |  1  |   1    |   0   |   1   |   1    |       1          1
   | 28  |  1  |   1    |   1   |   0   |   0    |       1          1
   | 29  |  1  |   1    |   1   |   0   |   1    |       1          1
   | 30  |  1  |   1    |   1   |   1   |   0    |       1          1
   | 31  |  1  |   1    |   1   |   1   |   1    |       1          1
   +-----+-----+--------+-------+-------+--------+

    ROMREAD = 0: 11111111111111111111111100000000 = 0x0ffffff00
    ROMREAD = 1: 11111111111111110000000000000000 = 0x0ffff0000

   If the relevant bit for the signals is 1 then the keyboard polling
   function will be called, otherwise not.


   Interpretting the microbee keyboard cct
   =======================================

   If ROMREAD* is low (ie. character ROM is visible) then keyboard operation
   is "normal" (ie. entirely controlled by the 6545).  A value is written to
   the update registers, a strobe is initiated by writing to R31, and if the
   key is down the lightpen is hit and the value stored.  If ROMREAD* is
   high, however, the keyboard is effectively polled continuously (or at
   least whenever the display is not blanked).  If any keys are down they
   will repeatedly trigger the lightpen.  So the contents of the lightpen
   register will be unpredictable, but a keypress will be instantly visible.

   My guess is this: when input is needed by the bee, ROMREAD* is set (or
   more likely, already is) high.  Then cpu then waits, polling the 6545
   until the lightpen is triggered, indicating a keypress.  ROMREAD* is then
   set low, and the update register / update strobe process is used to find
   what key(s) have actually been pressed, including SHIFT, CTRL etc.).


   Microbee key matrix
   ===================

   The rightmost columns of the microbee keyboard matrix are dedicated to
   special function keys, namely:

   [ ESC  ][ unused ]
   [  BS  ][ CTRL   ]
   [ TAB  ][ unused ]
   [  LF  ][ unused ]
   [  CR  ][ unused ]
   [ LOCK ][ unused ]
   [ BRK  ][ unused ]
   [ SPC  ][ SHIFT  ]

   These are assigned to the IBM keyboard as follows:

   ESC   =   key[KEY_ESC]       & KB_NORMAL
   BS    =   key[KEY_BACKSPACE] & KB_NORMAL
   TAB   =   key[KEY_TAB]       & KB_NORMAL
   LF    =   key[KEY_ALTGR]     & KB_NORMAL
   CR    =   key[KEY_ENTER]     & KB_NORMAL
   LOCK  =   key[KEY_CAPSLOCK]  & KB_NORMAL
   BRK   =   key[KEY_RCONROL]   & KB_NORMAL
   SPC   =   key[KEY_SPACE]     & KB_NORMAL
   CTRL  =   key[KEY_LCONROL]   & KB_NORMAL
   SHIFT = ( key[KEY_LSHIFT]    & KB_NORMAL ) || ( key[KEY_RSHIFT] & KB_NORMAL )

   The rest of the matrix depends on whether the shift key is down or not.
   If the (bee) shift key is not down, the matrix is seen by the microbee
   should be:

   [ @ ][ H ][ P ][ X ][ 0 ][ 8 ]
   [ A ][ I ][ Q ][ Y ][ 1 ][ 9 ]
   [ B ][ J ][ R ][ Z ][ 2 ][ : ]
   [ C ][ K ][ S ][ [ ][ 3 ][ ; ]
   [ D ][ L ][ T ][ \ ][ 4 ][ , ]
   [ E ][ M ][ U ][ ] ][ 5 ][ - ]
   [ F ][ N ][ V ][ ^ ][ 6 ][ . ]
   [ G ][ O ][ W ][DEL][ 7 ][ / ]

   corresponding pc keyboard combination to test (default shift up):

   [ shift+2@ ][ H ][ P ][ X        ][ 0) ][ 8*       ]
   [ A        ][ I ][ Q ][ Y        ][ 1! ][ 9(       ]
   [ B        ][ J ][ R ][ Z        ][ 2@ ][ shift+;: ]
   [ C        ][ K ][ S ][ [{       ][ 3# ][ ;:       ]
   [ D        ][ L ][ T ][ \|       ][ 4$ ][ ,<       ]
   [ E        ][ M ][ U ][ ]}       ][ 5% ][ -_       ]
   [ F        ][ N ][ V ][ shift+6^ ][ 6^ ][ .>       ]
   [ G        ][ O ][ W ][ DEL      ][ 7& ][ /?       ]

   If the shift is down, the microbee should see:

   [ ` ][ H ][ P ][ X ][ 0 ][ ( ]
   [ A ][ I ][ Q ][ Y ][ ! ][ ) ]
   [ B ][ J ][ R ][ Z ][ " ][ * ]
   [ C ][ K ][ S ][ { ][ # ][ + ]
   [ D ][ L ][ T ][ | ][ $ ][ < ]
   [ E ][ M ][ U ][ } ][ % ][ = ]
   [ F ][ N ][ V ][ ~ ][ & ][ > ]
   [ G ][ O ][ W ][DEL][ ' ][ ? ]

   corresponding pc keyboard combination to test (default shift down)

   [ noshift+`~ ][ H ][ P ][ X   ][ noshift+0) ][ 9(         ]
   [ A          ][ I ][ Q ][ Y   ][ 1!         ][ 0)         ]
   [ B          ][ J ][ R ][ Z   ][ '"         ][ 8*         ]
   [ C          ][ K ][ S ][ [{  ][ 3#         ][ =+         ]
   [ D          ][ L ][ T ][ \|  ][ 4$         ][ ,<         ]
   [ E          ][ M ][ U ][ ]}  ][ 5%         ][ noshift+=+ ]
   [ F          ][ N ][ V ][ `~  ][ 7&         ][ .>         ]
   [ G          ][ O ][ W ][ DEL ][ noshift+'" ][ /?         ]

   So, the matrix (as read by the microbee scanner to corresponding keys on
   the IBM keyboard) is (if the pc shift is not down):

   [ `~ ][ H ][ P ][ X   ][ 0)       ][ 8*       ]
   [ A  ][ I ][ Q ][ Y   ][ 1!       ][ 9(       ]
   [ B  ][ J ][ R ][ Z   ][ 2@       ][          ]
   [ C  ][ K ][ S ][ [{  ][ 3#       ][ ;:       ]
   [ D  ][ L ][ T ][ \|  ][ 4$       ][ ,<       ]
   [ E  ][ M ][ U ][ ]}  ][ 5%       ][ -_ OR =+ ]
   [ F  ][ N ][ V ][     ][ 6^       ][ .>       ]
   [ G  ][ O ][ W ][ DEL ][ 7& OR '" ][ /?       ]

   If the shift on pc is down:

   [ 2@ ][ H ][ P ][ X        ][    ][ 9(       ]
   [ A  ][ I ][ Q ][ Y        ][ 1! ][ 0)       ]
   [ B  ][ J ][ R ][ Z        ][ '" ][ 8* OR ;: ]
   [ C  ][ K ][ S ][ [{       ][ 3# ][ =+       ]
   [ D  ][ L ][ T ][ \|       ][ 4$ ][ ,<       ]
   [ E  ][ M ][ U ][ ]}       ][ 5% ][          ]
   [ F  ][ N ][ V ][ `~ OR 6^ ][ 7& ][ .>       ]
   [ G  ][ O ][ W ][ DEL      ][    ][ /?       ]

   Usually, the shift intersection of the matrix is just the state of the
   shift key on the pc keyboard.  However, the following combinations of
   shift + another key can cause this to be inverted (ie the intersection
   is the inverse of the shift key on the pc keyboard):

    shift down | shift up
   ------------+----------
       2@      |   `~
       6^      |   '"
       ;:      |   =+
               |   0)


   How the emulation works
   =======================

   When a key is pressed or released, the callback function
   key_lowlevel_function is called with the scancode.  If the scancode is
   relevant to the microbee then a flag is set.  This function will then
   exit.

   When the cycle function is next run, if the flag is detected it will be
   reset, and the lightpen table (which tells the 6545 what addresses (ie.
   keys) will cause a strobe) will be updated.

   If the key has been pressed (was previously up) then the relevant lpen
   bit INTERF_KEY_LPEN_TABLE[i] will be set (well, not quite, but almost -
   see code for use of INTERF_KEY_LPEN_TABLE[i+1] as a temporary scratch
   table to prevent keybounce reseting the downcounter) as well as
   interf_key_worktable[i+1].  Furthermore, a counter byte at
   interf_key_worktable[i+2] is be set to interf_key_clkcnt_max.
   If the key has been released, otoh, interf_key_worktable[i+1]
   will be reset (no other bits will be changed).

   Note that INTERF_KEY_LPEN_TABLE[i] will *not* be reset whenever a key
   is released.  The reason for this is that, at the timescale of keypresses,
   the speed of emulation is jittery, so doing this results in lots of
   missed keypresses that would have worked fine on the microbee.  To prevent
   this keypresses are "stretched" until either (a) they are correctly read
   or acknowledged or (b) the interf_key_worktable[i+2] (which
   is a countdown byte) reaches zero (the latter is to prevent key stickage
   in some games etc. that take over control of the keyboard entirely).

   Mechanism (a) works thusly:  There are two additional tables, namely
   INTERF_KEY_FEEDBACK_TABLE and INTERF_KEY_FEEDRFSH_TABLE which are both
   counters whose elements are incremented if the 6545 fully registers a
   lpen strobe at this address.  The former is specifically for lpen strobes
   using the update register method (ie. directed by code), whereas the
   latter is specifically for lpen strobes that occur during free-running
   scan (ie. undirected).  If either of these exceeds the relevant count
   (interf_key_response_count for the former, interf_key_response_count_rfsh
   the latter) then key stretching ceases - that is, from this point on
   INTERF_KEY_LPEN_TABLE[i] = interf_key_worktable[i+1].

   Mechanism (b) (the timeout method) works thusly: whenever the cumulative
   cycle count for interf_cycle exceeds interf_key_rfsh_cycles the counters
   in interf_key_worktable[i+2] will be decremented.  When this
   counter reaches zero key stretching ceases - ie. from this point on
   INTERF_KEY_LPEN_TABLE[i] = interf_key_worktable[i+1].

   Of course, all of this is designed to only effect very short keystrokes,
   and hence serves a double purpose of debouncing keystrokes (mechanism
   (b) takes care of this) and preventing keystrokes becoming "lost" due to
   emulation speed jitter.

   OBSERVATIONS:

   - I've observed that for most alphanumeric keys (but not shift, ret etc)
     basic will use "refresh" once (and only once) to test which key has
     been pressed for a single keypress.
   - If you hold down one key, it will block all others.  That is, if a
     press (and hold down) 'a', and then also press 'b' while 'a' is held
     down then there is no effect - it is just like 'a' was being held down
     on its own.  From this I *guess* that the keyboard (in basic, at least)
     operates thusly:

     - the 6545 lpen bit is monitored.  When it is tripped, a key is
       presumed to have been pressed.
     - The 6545 update feature is then used to run through the keyboard to
       see which key has been pressed, and also assess the state of the
       modifiers (shift, ctrl etc.).
     - Once the key is assertained, the lpen bit is reset periodically.  If
       it goes back high within some given period of time then it is assumed
       that the same key is being held down and causing this.


   Shift Inversion and Sticky Shifting
   ===================================

   Because some characters ('@' for example) require no shift on the bee,
   but require shift on the pc, for some keystrokes we need a concept of
   shift state inversion - that is, if shift is down on the pc keyboard then
   it should be up on the microbee (emulated) keymap, and vice-versa, for
   some key combinations.  This is known as shift inversion (ie. the state
   of the shift as reported to the emulator is the inverse of the state of
   the shift key on the pc keyboard).  The keys so effected are:

     shift down | shift up
    ------------+----------
        2@      |   `~
        6^      |   '"
        ;:      |   =+
                |   0)

   This seems reasonable.  However, consider the following chain of events
   at the pc keyboard (which ime is quite common when typing at speed):

    1. press SHIFT
    2. press 2@
    3. release SHIFT
    4. release 2@

   Now, this *should* give @ and nothing else.  But if we just naively
   implement the shift inversion scheme outlined earlier, it won't.  The
   problem is that, while 2 and @ are the same key on the pc keyboard, they
   are not the same key on the microbee keyboard.

   What the emulated microbee sees using the naive scheme is

    1. SHIFT goes down.
    2. SHIFT goes up due to shift inversion...
       ...at the same time as @` goes down.
    3. @' goes up...
       ...at the same time as 2" goes down...
    4. 2" goes up.

   which will result in the string @2 being printed.  What *should* happen
   is the following:

    1. SHIFT goes down
    2. SHIFT goes up due to shift inversion...
       ...at the same time as @` goes down.
    3. nothing happens
    4. @' goes up.

   which will result in the character @ being printed, as should be (to be
   consistent with other basic strings).

   To achieve this end, it is necessary to make the pc SHIFT key sticky -
   that is, when one of

    KEY_0
    KEY_2
    KEY_6
    KEY_7
    KEY_9
    KEY_8
    KEY_TILDE
    KEY_QUOTE
    KEY_COLON
    KEY_EQUALS

   is pressed when the pc SHIFT key is down, the pc SHIFT key must remain
   (virtually) held down until that key is released.  Similarly, when one
   of:

    KEY_TILDE
    KEY_0
    KEY_2
    KEY_6
    KEY_7
    KEY_8
    KEY_9
    KEY_QUOTE
    KEY_COLON
    KEY_MINUS
    KEY_EQUALS

   is pressed whilst the pc SHIFT key is NOT held down then pc SHIFT key
   must remain (virtually) NOT held down until the key is released.


   Keyboard Data
   =============

   interf_key_keydown: Set if any element of INTERF_KEY_LPEN_TABLE[i] is 1
        (ie. there is an emulated keypress in operation).
   interf_key_keyfileupdn: toggle variable used when sources keystrokes from
        a file.  This will be set to 0 initially.  A emulated keypress will
        then occur, some timers will start and this will be set to 1.  When
        this is 1 then every cycle the counters will decrease until they all
        reach zero, at which point the keystroke will finish and the process
        will begin again, initiated by setting this to zero.
   interf_key_worktable: See above.
   interf_key_cyclecnt: Cycle counter.
   interf_key_sourcefp: NULL if keyboard operation normal, otherwise a file
        pointer for keystroke source.
   interf_key_sourcefilename: filename and path for keyboard source file.

   interf_key_response_count (0): number of targetted key updates that can
        occur for a given key before key stretching is ceased.
   interf_key_response_count_rfsh (1): number of random (refresh) type key
        updates that can occur for a key before key stretching is ceased.
   interf_key_release_updates_max (1): when reading from file to keyboard,
        the key timing is done based on the number of reads from port 31 of
        the 6545.  This is the number of reads from this port for which a
        key should be held per keystroke, or...
   interf_key_lpenreset_max  (90): \ when the number of reads from port
   interf_key_lpenreset_min (100): / 16/17 is in this range (obscure).

*/

int      interf_key_keydown                        = 0;
int      interf_key_keyfileupdn                    = 0;
UINT_8  *interf_key_worktable                      = NULL;
UINT_64  interf_key_cyclecnt                       = 0;
PC_FILE *interf_key_sourcefp                       = NULL;
char     interf_key_sourcefilename[DEFAULT_STRLEN] = "";

UINT_8  interf_key_response_count      = 0;
UINT_8  interf_key_response_count_rfsh = 1;
UINT_32 interf_key_release_updates_max = 1;
UINT_32 interf_key_lpenreset_max       = 100;
UINT_32 interf_key_lpenreset_min       = 90;

#ifdef IS_ALLEGRO
void interf_key_lowlevel_exit(int scancode);
void interf_key_lowlevel_react(int scancode);
#endif
void interf_key_setfile(const char *interf_key_sourcefilename);
void interf_key_closefile(void);

int interf_key_unshift_0      = 0;
int interf_key_unshift_2      = 0;
int interf_key_unshift_6      = 0;
int interf_key_unshift_7      = 0;
int interf_key_unshift_8      = 0;
int interf_key_unshift_9      = 0;
int interf_key_unshift_tilde  = 0;
int interf_key_unshift_quote  = 0;
int interf_key_unshift_colon  = 0;
int interf_key_unshift_equals = 0;
int interf_key_unshift_minus  = 0;

int interf_key_shift_0      = 0;
int interf_key_shift_2      = 0;
int interf_key_shift_6      = 0;
int interf_key_shift_7      = 0;
int interf_key_shift_8      = 0;
int interf_key_shift_9      = 0;
int interf_key_shift_tilde  = 0;
int interf_key_shift_quote  = 0;
int interf_key_shift_colon  = 0;
int interf_key_shift_equals = 0;
int interf_key_shift_minus  = 0;

int interf_key_effdown_0      = 0;
int interf_key_effdown_2      = 0;
int interf_key_effdown_6      = 0;
int interf_key_effdown_7      = 0;
int interf_key_effdown_8      = 0;
int interf_key_effdown_9      = 0;
int interf_key_effdown_tilde  = 0;
int interf_key_effdown_quote  = 0;
int interf_key_effdown_colon  = 0;
int interf_key_effdown_equals = 0;
int interf_key_effdown_minus  = 0;



/*
                           Sound Emulation
                           ===============

   Microbee Sound
   ==============

   The Microbee hangs it's speaker off bit 6 of port b of the pio.


   Emulation Method - PC Speaker
   =============================

   Sound emulation is done by estimating the output frequency by finding the
   number of clock cycles between 0->1 changes in the state of the speaker.
   This is done by maintaing the counter:

   UINT_32 interf_snd_clkcycle_cnt_a

   which counts the number of clock cycles done by the CPU.  Then, if an
   appropriate state change for the speaker is found (this occurs when the
   pio calls pio_b_rdy_data_out() with bit 6 of port b high where it was
   low on the previous call), the estimated fequency will be:

                                       10^9
   beesnd_freq = -------------------------------------------------- Hz
                 interf_snd_clkcycle_cnt_a*interf_snd_sndclk_period

   This frequency is stored until the next catchup point and then the tone
   (or otherwise) played by the speaker is set correctly.


   Details
   =======

   I tried a few methods.  Averaging resulted in tones bleeding into one
   another, making frogger sound pretty bad - basically the tone of the
   frog "jump" sound was random.

   In the end, I chose to simply base the tone on a single cycle, which
   surprisingly gave the best results.  So the tone played is calculated
   based on the period of the most recent 0->1 transition.  The only caveat
   here is that if the frequency decreases, then the frequency played will
   still be the higher (older) frequency in the current interval.  This
   prevents the spaces between tones being played rather than the tones
   themselves.

   Finally, it is necessary to insert "clicks" to get the sound right.
   For example, if the space between notes falls in a single catchup
   interval then a "click" is needed to make sure the notes are
   differentiable (see, for example, the fanfare in cbrikout.mwb).

   Also, if the tone decreases during an interval then a click is inserted
   after this tone.

   OK, that was confusing - just see the code (search for the functions
   beesnd_sound and interf_snd_nosound, and also the variable
   interf_snd_click_here to locate the relevant bits of the emulator).


   Data
   ====

   interf_snd_tone_here:  0 normally, 1 if there is a tone this sound period.
   interf_snd_click_here: 0 normally, 1 if there is a click during this ".
   interf_snd_click_next: 0 normally, 1 if there should be a click next ".
   interf_snd_click_stop: 0 normally, 1 if click is on that must be stopped.

   speaker_state:     state of pio port b bit 6
   speaker_state_old: previous state of pio port b bit 6

   freq: frequency of tone.

   sound_clock_cycle_countera: number of cycles since last transition
   sound_clock_cycle_counterb: period of last tone.


   Emulation Method - soundcard
   ============================

   The soundcard emulation method is still being developed.  Currently the
   code is a hack to imitate the pc speaker method.  It works, sorta, most
   of the time, but could be improved a lot.  Eventually I want to convert
   it to make wav files on the fly, which should result in perfect sound
   emulation.  Trouble is jitter, and the allegro soundcard stuff seems to
   almost work against doing it this way.

*/

#ifdef SOUND_SOUNDCARD
/* SAMPLE_RATE          byte rate in Hz         */
/* BASE_FREQ_*          frequency at 10000      */
/* CYCLELEN_*           no. cycles in a sample  */
/* CYCLE_LENGTH_*       no. bytes in 1 cycle    */
/* BUFF_SIZE_*          buffer size             */
/* CLICKCYCLES          no cycles/click         */
/* CLICKBUFF_SIZE       click buffer size       */

#define SAMPLE_RATE     44100
#define STRETCH         1
#define SHRINK          2
#define MAXSTRETCH      1
#define MINFREQ         20
#define BASE_FREQ_A     200
#define BASE_FREQ_B     2000
#define BASE_FREQ_C     20000
#define CYCLELEN_A      100
#define CYCLELEN_B      1000
#define CYCLELEN_C      10000
#define CYCLE_LENGTH_A  ((STRETCH*SAMPLE_RATE)/BASE_FREQ_A)
#define CYCLE_LENGTH_B  ((STRETCH*SAMPLE_RATE)/BASE_FREQ_B)
#define CYCLE_LENGTH_C  ((STRETCH*SAMPLE_RATE)/BASE_FREQ_C)
#define BUFF_SIZE_A     (CYCLELEN_A*CYCLE_LENGTH_A)
#define BUFF_SIZE_B     (CYCLELEN_B*CYCLE_LENGTH_B)
#define BUFF_SIZE_C     (CYCLELEN_C*CYCLE_LENGTH_C)
#define CLICKCYCLES     1
#define CLICKBUFF_SIZE  (CLICKCYCLES*CYCLE_LENGTH_A)

SAMPLE *interf_snd_sample_a;
SAMPLE *interf_snd_sample_b;
SAMPLE *interf_snd_sample_c;
SAMPLE *interf_click_sample;
#endif

int interf_snd_cardbad    = 0;
int interf_snd_sound_on_a = 0;
int interf_snd_sound_on_b = 0;
int interf_snd_sound_on_c = 0;
int interf_snd_frq_a = 0;
int interf_snd_frq_b = 0;
int interf_snd_frq_c = 0;

int interf_snd_tone_here  = 0;
int interf_snd_click_here = 0;
int interf_snd_click_next = 0;
int interf_snd_click_stop = 0;

int interf_snd_speaker_state     = 0;
int interf_snd_speaker_state_old = 0;

UINT_32 interf_snd_freq = 0;

UINT_64 interf_snd_clkcycle_cnt_a = 0;
UINT_64 interf_snd_clkcycle_cnt_b = 0;

void interf_speaker_state_change(void *what);
void interf_snd_toggle_snd(void);
void interf_snd_turn_snd_on(void);
void interf_snd_turn_snd_off(void);
void interf_snd_nosound(void);
void interf_snd_sound(int freq);
void interf_snd_soundclick(void);


/*
   Tape i/o Emulation
   ==================

   Modulation Scheme
   =================

   The microbee uses the "Kansas city standard" (thanks to WorkerBee for
   pointing that out) modulation scheme - that is, 0 is 1200Hz and 1 is
   2400Hz.  This is used to modulate a serial data stream with 8 bits, no
   parity and 2 stop bits.  So, each byte transmitted is:

   0.  boundary bit low
   1.  Data bit 0 (LSB)
   2.  Data bit 1
   3.  Data bit 2
   4.  Data bit 3
   5.  Data bit 4
   6.  Data bit 5
   7.  Data bit 6
   8.  Data bit 7 (MSB)
   9.  stop bit high
   10. stop bit high
   (return to low if end of block)

   When no signal is being sent, the output of the pio is held low.


   300 Baud Modulation Details
   ===========================

   Each high bit is: 2400Hz modulated.
                     starts high, ends low.
                     8 cycles long.
                     characterised by the duty cycle:

                                  - first peak is 840 clocks wide
    +-+ +-+ +-+ +-+ +-+ +-+ +-+   - last 7 peaks are 711 z80 clocks wide
      | | | | | | | | | | | | |
      +-+ +-+ +-+ +-+ +-+ +-+ +-+ - each trough is 684 z80 clocks wide


   Each low bit is: 1200Hz modulated.
                    starts high, ends low.
                    4 cycles long.
                    characterised by the duty cycle:

                              - first peak is 1546->1613 clocks wide
    +--+  +--+  +--+  +--+    - last 3 peaks are 1413 z80 clocks wide
       |  |  |  |  |  |  |
       +--+  +--+  +--+  +--+ - each trough is 1386 z80 clocks wide


   1200 Baud Modulation Details
   ============================

   Each high bit is: 2400Hz modulated.
                     starts high, ends low.
                     2 cycles long.
                     characterised by the duty cycle:

    +-+ +-+   - first peak 840, 2nd peak 711 z80 clocks wide
      | | |
      +-+ +-+ - each trough is 684 z80 clocks wide


   Each low bit is: 1200Hz modulated.
                    starts high, ends low.
                    1 cycle long.
                    characterised by the duty cycle:

    +--+    - peak 1582 z80 clocks wide
       |   
       +--+ - trough 1386 z80 clocks wide


   Higher level details - pipe operations
   ======================================

   When data is piped to the tape port (under basic, this means using
   outl#2, out#3 or similar), the format is thus:

   ff     + 
   ff     | First there are 20 ff's (give or take)
   ...    | 
   ff     + 
   2a     - then 2a, which presumably means something
   ll     - the length byte.  If this is 0, 256 bytes of data follow.
            Otherwise ll bytes follow.
   <ll>   - block: ll bytes of data (or 256 if ll = 0)
   crc    - crc byte
            To calculate this: initialise c = 0
                               for each byte d in <ll>: c := (c XOR d) >> 1
   ff     +
   ff     | Some number of ff's (seems random, but at least 1)
   ...    |
   ff     +

   The entire block is either 300 baud or 1200 baud, including all the
   header stuff and the ff buffers.  Thus by timing the length of the
   1200Hz burst at the start (the boundary bit) one can calculate the
   speed of transfer.  The same logic can be used to detect if this is
   a save operation (which will have 00 as a buffer).  Specifically:

   1. If the transmission starts with 1 cycle at 1200Hz, then goes to 2400Hz
      then the transmission must be a 1200 baud pipe operation
   2. If the transmission starts with 4 cycles at 1200Hz and then goes to
      2400Hz then it must be a 300 baud pipe operation.
   3. If the transmission starts with 36 cycles at 1200Hz, and then goes to
      2400Hz it is a tape save operation (see below), which always starts
      at 300baud for the header.


   Higher level details - tape save operations
   ===========================================

   When data is saved to the tape port, the format is thus:

   *** Transmission will begin at 300baud for (at least) the header. ***
   00     + 
   00     | First there are >=16 00's (NULLs) according to the docs
   ...    | ime, this means that there are 64 of these
   00     +
   01     - SOH (start of header) byte
   xx     +
   xx     |
   xx     | 6 byte filename, padded with NULLs (00's)
   xx     |
   xx     |
   xx     +
   ft     - filetype byte.  Could be anything, but the types recognised by
            basic are: - B for BASIC files
                       - M for MACHINE language files
   ll     \ file size 
   hh     / (LSB first)
   ll     \ load address
   hh     / (LSB first)
   ll     \ autoexec address
   hh     / (LSB first)
   ss     - speed - 00 = 300baud, nonzero (01) = 1200baud
   aa     - autoload control - 00 = no autoexec, FF for autoexec
   nu     - unused (zero) byte
   ck     - checksum byte for header
            To calculate this: 1. sum all bytes in the header (starting at
                                  the first byte after the SOH (not including
                                  SOH) and ending with the unused byte.
                               2. take the 2's complement.
                               3. Then the weird bit - if the solution so
                                  far is xy in hex (eg. 6e means x=6, y=e)
                                  then subtract 1 from x, giving the
                                  checksum (x-1)y so, in the above example,
                                  the checksum would be 5e.
   *** There low cycle here is extended to ~1800 clock cycles.  ***
   *** At this point, if speed == 1200Hz (ss = FF) then the     ***
   *** speed will change from 300baud to 1200baud transmission. ***
   <256>  - byte data block            +
   ck     - checksum for data block    | Not present if filesize <= 256
   ...    - repeat so many times       +
   <n>    - n byte data block, where n = filesize % 0x0100
   crc    - crc for data block

   To calculate the checksum for each data block, and the crc for the final
   data block, the following method is used:

   1. Start block with c = 0
   2. For each data byte in block, do c := CPL(d-c) where CPL is the 1's
      complement (inverse) and d-c is done by two's complementing c.

   Behavioural constants
   =====================

   INTERF_TAPE_CLK_MASK: clock counter mask used to prevent overflow in the
        interf_tape_out_elapsed_zclk clock counter.

   INTERF_TAPE_1200_LOW: # interf_tape_*_elapsed_zclk cycles "0" for 1200Hz.
   INTERF_TAPE_1200_HGH: # interf_tape_*_elapsed_zclk cycles "1" for 1200Hz.
   INTERF_TAPE_2400_LOW: # interf_tape_*_elapsed_zclk cycles "0" for 2400Hz.
   INTERF_TAPE_2400_HGH: # interf_tape_*_elapsed_zclk cycles "1" for 2400Hz.

   INTERF_TAPE_1200_LOWER: lower bound of interf_tape_*_elapsed_zclk "1"
        1/2-cycle for a 1200Hz signal (when decoding)
   INTERF_TAPE_1200_UPPER: upper bound of interf_tape_*_elapsed_zclk "1"
        1/2-cycle for a 1200Hz signal (when decoding)
   INTERF_TAPE_2400_LOWER: lower bound of interf_tape_*_elapsed_zclk "1"
        1/2-cycle for a 2400Hz signal (when decoding)
   INTERF_TAPE_2400_UPPER: upper bound of interf_tape_*_elapsed_zclk "1"
        1/2-cycle for a 2400Hz signal (when decoding)

   INTERF_TAPE_300_CNT_LOW: # cycles @ 1200Hz for a 300 baud tape "0".
   INTERF_TAPE_300_CNT_HGH: # cycles @ 2400Hz for a 300 baud tape "1".
   INTERF_TAPE_1200CNT_LOW: # cycles @ 1200Hz for a 1200 baud tape "0".
   INTERF_TAPE_1200CNT_HGH: # cycles @ 2400Hz for a 1200 baud tape "1".

   For convenience, many of the above are combined into some (nominally
   read only) arrays, which allow one refer to a single variable for 
   different frequencies and save speeds.  These are:

   interf_tape_cycles[2][2] = "correct" # z80 clock cycles a signal should
        stay either high or low: [0][0] = at 1200Hz LOW
                                 [0][1] = at 1200Hz HIGH
                                 [1][0] = at 2400Hz LOW
                                 [1][1] = at 2400Hz HIGH

   interf_tape_lower[2] = "minimum" # z80 clock cycles a signal should stay
        high: [0] = at 1200Hz nominal
              [1] = at 2400Hz nominal
   interf_tape_upper[2] = "maximum" # z80 clock cycles a signal should stay
        high: [0] = at 1200Hz nominal
              [1] = at 2400Hz nominal

   interf_tape_count[2][2] = number of cycles for a given signal:
        [0][0] = 300baud "0" (1200Hz) bit
        [0][1] = 300baud "1" (2400Hz) bit
        [1][0] = 1200baud "0" (1200Hz) bit
        [1][1] = 1200baud "1" (2400Hz) bit


   Variables and Control - output
   ==============================

   interf_tape_out_autosave_dir: string giving the current autosave dir.  By
        default, this is "./"
   interf_tape_out_autosave_name: string giving the current autosave name,
        which is the combination of the filename and filetype given in the
        tape header.
   interf_tape_out_file_tapedest: filename all output data is sent to.  This
        is used by both the save and pipe operations.

   interf_tape_out_mode:        1  = stream to data file
                                2  = stream to WAVe file
                                3  = stream to NULL
   interf_tape_out_type:        0  = stream output
                                1  = tape save output
   interf_tape_out_baud:        0  = 300 baud
                                1  = 1200 baud
   interf_tape_out_hl_state:    **** If interf_tape_out_type = 0
                                0  = not doing anything
                                1  = signal detected, ff read
                                2  = 2a read, awaiting length
                                3  = length read, awaiting (or reading) data
                                4  = data finished, awaiting crc
                                **** If interf_tape_out_type = 1
                                0  = not doing anything
                                1  = signal detected, 00 read, awaiting 01
                                2  = 01 read, awaiting filename byte 1
                                3  =          awaiting filename byte 2
                                4  =          awaiting filename byte 3
                                5  =          awaiting filename byte 4
                                6  =          awaiting filename byte 5
                                7  =          awaiting filename byte 6
                                8  = awaiting filetype byte
                                9  = awaiting lsb of filesize word
                                10 = awaiting msb of filesize word
                                11 = awaiting msb of load addr
                                12 = awaiting msb of load addr
                                13 = awaiting msb of autoexec addr
                                14 = awaiting msb of autoexec addr
                                15 = awaiting speed byte
                                16 = awaiting autoload byte
                                17 = awaiting unused byte
                                18 = awaiting checksum byte
                                19 = awaiting (or reading) data
                                20 = awaiting (or reading) data checksum/crc
   interf_tape_out_hl_length:   #  = bytes in block still to be read
   interf_tape_out_hl_overall:  #  = blocks to be read overall, -1
   interf_tape_out_hl_endit:    #  = bytes in final block
   interf_tape_out_filename:    s  = file name string
   interf_tape_out_filetype:    b  = file type byte
   interf_tape_out_filesize:    #  = file size word
   interf_tape_out_fileload:    #  = file load address word
   interf_tape_out_fileexec:    #  = file autoexec address word
   interf_tape_out_filesped:    b  = file speed byte (0 for 300baud)
   interf_tape_out_fileauto:    b  = file autoload byte (0 for non-autoexec)
   interf_tape_out_state_course:0  = nothing happening, waiting to find stuff
                                1  = first boundary bit
                                2  = data bit 0
                                3  = data bit 1
                                4  = data bit 2
                                5  = data bit 3
                                6  = data bit 4
                                7  = data bit 5
                                8  = data bit 6
                                9  = data bit 7
                                10 = first stop bit
                                11 = second stop bit
   interf_tape_out_state_fine:  0  = signal level low
                                !0 = signal level high
   interf_tape_out_tapefreq:    0  = 1200Hz
                                1  = 2400Hz
   interf_tape_out_byte:        b  = actual data byte (used in interfn comms)
   interf_tape_out_crc:         b  = used to calculate the crc
   interf_tape_out_kansascycle: #  = elapsed cycles (at 1200 or 2400Hz)
   interf_tape_out_elapsed_zclk:#  = elapsed z80 cycles since last edge

   interf_tape_out_dest_fp:     file pipe operations are sent to
   interf_tape_out_autosave_fp: file autosave operations are sent to


   Variables and Control - input
   =============================

   interf_tape_in_type:          0  = nothing happening
                                 1  = currently writing from datafile
   interf_tape_in_buffer:        p  = pointer to dword buffer (buffer).
   interf_tape_in_pos_byte:      #  = buffer dword being processed (0 start).
   interf_tape_in_pos_bit:       #  = bit in byte (32 bit mask, not count).
   interf_tape_in_pos_bitcnt:    #  = bit position in buffer
   interf_tape_in_spedchgbitcht: #  = position at which we change from 300 to
                                      1200 baud (used by load function).
   interf_tape_in_bufsize:       #  = size of tape buffer (in bits).
   interf_tape_in_tapesped:      0  = 300 baud
                                 1  = 1200 baud
   interf_tape_in_state_fine:    0  = signal level low
                                 1  = signal level high
   interf_tape_in_bit_fine:      0  = bit is 0 (1200Hz)
                                 1  = bit is 1 (2400Hz)
   interf_tape_in_crc:           b  = used to calculate the crc
   interf_tape_in_kansascycle:   #  = elapsed cycles (at 1200 or 2400Hz)
   interf_tape_in_elapsed_zclk:  #  = elapsed z80 cycles since last edge
*/

#define INTERF_TAPE_1200_LOW    1386
#define INTERF_TAPE_1200_HGH    1413
#define INTERF_TAPE_2400_LOW    684
#define INTERF_TAPE_2400_HGH    711

#define INTERF_TAPE_1200_LOWER  1100
#define INTERF_TAPE_1200_UPPER  2200
#define INTERF_TAPE_2400_LOWER  500
#define INTERF_TAPE_2400_UPPER  900

#define INTERF_TAPE_300_CNT_LOW 4
#define INTERF_TAPE_300_CNT_HGH 8
#define INTERF_TAPE_1200CNT_LOW 1
#define INTERF_TAPE_1200CNT_HGH 2

#define INTERF_TAPE_TIMEOUT_POINT       2300
#define INTERF_TAPE_TIMEOUT_PUSH        2000

#define INTERF_TAPE_CLK_MASK    0x00fffffff

UINT_32 interf_tape_cycles[2][2] = { { INTERF_TAPE_1200_LOW , INTERF_TAPE_1200_HGH } , { INTERF_TAPE_2400_LOW , INTERF_TAPE_2400_HGH } };
UINT_32 interf_tape_lower[2]     = { INTERF_TAPE_1200_LOWER , INTERF_TAPE_2400_LOWER };
UINT_32 interf_tape_upper[2]     = { INTERF_TAPE_1200_UPPER , INTERF_TAPE_2400_UPPER };
UINT_32 interf_tape_count[2][2]  = { { INTERF_TAPE_300_CNT_LOW , INTERF_TAPE_300_CNT_HGH } , { INTERF_TAPE_1200CNT_LOW , INTERF_TAPE_1200CNT_HGH } };

int interf_tape_out_mode = 0;

char interf_tape_out_autosave_dir[DEFAULT_STRLEN]  = "./";
char interf_tape_out_autosave_name[DEFAULT_STRLEN] = "";
char interf_tape_out_file_tapedest[DEFAULT_STRLEN] = "";

int     interf_tape_out_baud         = 0;
int     interf_tape_out_type         = 0;
int     interf_tape_out_state_course = 0;
int     interf_tape_out_tapefreq     = 0;
int     interf_tape_out_state_fine   = 0;
UINT_8  interf_tape_out_byte         = 0;
UINT_8  interf_tape_out_crc          = 0;
UINT_32 interf_tape_out_kansascycle  = 0;
UINT_32 interf_tape_out_elapsed_zclk = 0;
int     interf_tape_out_hl_state     = 0;
UINT_8  interf_tape_out_hl_length    = 0;
UINT_8  interf_tape_out_hl_overall   = 0;
UINT_8  interf_tape_out_hl_endit     = 0;
char    interf_tape_out_filename[7];
UINT_8  interf_tape_out_filetype     = 0;
UINT_16 interf_tape_out_filesize     = 0;
UINT_16 interf_tape_out_fileload     = 0;
UINT_16 interf_tape_out_fileexec     = 0;
UINT_8  interf_tape_out_filesped     = 0;
UINT_8  interf_tape_out_fileauto     = 0;

PC_FILE *interf_tape_out_dest_fp     = NULL;
PC_FILE *interf_tape_out_autosave_fp = NULL;

int      interf_tape_in_type          = 0;
UINT_32 *interf_tape_in_buffer        = NULL;
UINT_32  interf_tape_in_pos_byte      = 0;
UINT_32  interf_tape_in_pos_bit       = 0;
UINT_32  interf_tape_in_pos_bitcnt    = 0;
UINT_32  interf_tape_in_spedchgbitcht = 0;
UINT_32  interf_tape_in_bufsize       = 0;
int      interf_tape_in_tapesped      = 0;
int      interf_tape_in_state_fine    = 0;
int      interf_tape_in_bit_fine      = 0;
UINT_8   interf_tape_in_crc           = 0;
UINT_32  interf_tape_in_kansascycle   = 0;
UINT_32  interf_tape_in_elapsed_zclk  = 0;



void interf_tape_out_reset_state_after_block(void);
void interf_tape_out_reset_state_after_file(void);
void interf_tape_out_reset_state_after_stream(void);
void interf_tape_out_reset_state(void);
void interf_tape_out_send_byte(void);
void interf_tape_out_close_output_stream(void);
void interf_tape_out_close_autosave_file(void);

int interf_tape_out_set_mode_1(char *filename);
int interf_tape_out_set_mode_3(void);

void interf_tape_in_reset_state(void);
char *interf_tape_in_pipe_data(char *filename, int tapesped);
int interf_tape_in_convert_to_memload(UINT_8 head_ft, UINT_8 head_fsl, UINT_8 head_fsh, UINT_8 head_lal, UINT_8 head_lah, UINT_8 head_ael, UINT_8 head_aeh, UINT_8 head_sp, UINT_8 head_ac, UINT_8 head_nu, char *yfilename, UINT_32 filesize, PC_FILE *interf_tape_in_src__fp);
void interf_tape_in_putbyteinbuff(UINT_8 what);
void interf_tape_state_change(void *what);

#define INTERF_TAPE_IN_PUTBITINBUFF(what)                               \
{                                                                       \
    interf_tape_in_buffer[interf_tape_in_pos_byte] &= ( 0x0ffffffff ^ interf_tape_in_pos_bit ); \
                                                                        \
    if ( what )                                                         \
    {                                                                   \
        interf_tape_in_buffer[interf_tape_in_pos_byte] |= interf_tape_in_pos_bit; \
    }                                                                   \
                                                                        \
    if ( interf_tape_in_pos_bit != 0x080000000 )                        \
    {                                                                   \
        interf_tape_in_pos_bit <<= 1;                                   \
    }                                                                   \
                                                                        \
    else                                                                \
    {                                                                   \
        interf_tape_in_pos_bit = 0x000000001;                           \
        interf_tape_in_pos_byte++;                                      \
    }                                                                   \
                                                                        \
    interf_tape_in_pos_bitcnt++;                                        \
}

#define INTERF_TAPE_IN_GETBITFROMBUFF(what)                             \
{                                                                       \
    what = 0;                                                           \
                                                                        \
    if ( interf_tape_in_buffer[interf_tape_in_pos_byte] & interf_tape_in_pos_bit ) \
    {                                                                   \
        what = 1;                                                       \
    }                                                                   \
                                                                        \
    if ( interf_tape_in_pos_bit != 0x080000000 )                        \
    {                                                                   \
        interf_tape_in_pos_bit <<= 1;                                   \
    }                                                                   \
                                                                        \
    else                                                                \
    {                                                                   \
        interf_tape_in_pos_bit = 0x000000001;                           \
        interf_tape_in_pos_byte++;                                      \
    }                                                                   \
                                                                        \
    interf_tape_in_pos_bitcnt++;                                        \
}






/*
   Menu stuff (rest is later)
   ==========================
*/

#ifdef IS_ALLEGRO
int interf_menu_setvidmode0(void);
void interf_menu_enter(void);
#endif














/*
   Code starts here.
   =================
*/


#ifdef IS_ALLEGRO

void interf_key_lowlevel_react(int scancode)
{
    /*
       This function is called (during emulation) whenever a keystroke is
       received (and also for key releases).  It operates in interupt
       context, and hence it just sets a bunch of flags to be dealt with
       elsewhere (but see below for exception, which really should be
       fixed).
    */

    if ( !interf_is_in_menu_mode )
    {
        switch ( scancode )
        {
            case INTERF_SCRNDUMP_KEY:
            {
                /* Save screenshot */

                interf_prtscr_flag = 1;

                break;
            }

            case INTERF_INTERACT_KEY:
            {
                /* Enter interactive mode */

                interf_interact_flag = 1;

                break;
            }

            case INTERF_RESET_KEY:
            {
                /* Reset key pressed */

                interf_sig_rsetdwn_flag = 1;

                break;
            }

            case (INTERF_RESET_KEY | 0x080):
            {
                /* Reset key released */

                interf_sig_rsetup_flag = 1;

                break;
            }

            case INTERF_SCRNRFSH_KEY:
            {
                /* Refresh the screen */

                interf_scrn_rfsh_flag = 1;

                break;
            }

            case INTERF_SPEEDCTRL_KEY:
            {
                /* Toggle speed emulation. */

                interf_speedtoggle_flag = 1;

                break;
            }

            case INTERF_SOUNDCNTRL_KEY:
            {
                /* Toggle sound emulation. */

                interf_snd_toggle_flag = 1;

                break;
            }

            case KEY_ESC:           case (KEY_ESC           | 0x080):

            case KEY_F1:            case (KEY_F1            | 0x080):
            case KEY_F2:            case (KEY_F2            | 0x080):
            case KEY_F3:            case (KEY_F3            | 0x080):
            case KEY_F4:            case (KEY_F4            | 0x080):

            case KEY_F5:            case (KEY_F5            | 0x080):
            case KEY_F6:            case (KEY_F6            | 0x080):
            case KEY_F7:            case (KEY_F7            | 0x080):
            case KEY_F8:            case (KEY_F8            | 0x080):

            case KEY_F9:            case (KEY_F9            | 0x080):
            case KEY_F10:           case (KEY_F10           | 0x080):
            case KEY_F11:           case (KEY_F11           | 0x080):
            case KEY_F12:           case (KEY_F12           | 0x080):

            case KEY_TILDE:         case (KEY_TILDE         | 0x080):
            case KEY_1:             case (KEY_1             | 0x080):
            case KEY_2:             case (KEY_2             | 0x080):
            case KEY_3:             case (KEY_3             | 0x080):
            case KEY_4:             case (KEY_4             | 0x080):
            case KEY_5:             case (KEY_5             | 0x080):
            case KEY_6:             case (KEY_6             | 0x080):
            case KEY_7:             case (KEY_7             | 0x080):
            case KEY_8:             case (KEY_8             | 0x080):
            case KEY_9:             case (KEY_9             | 0x080):
            case KEY_0:             case (KEY_0             | 0x080):
            case KEY_MINUS:         case (KEY_MINUS         | 0x080):
            case KEY_EQUALS:        case (KEY_EQUALS        | 0x080):
            case KEY_BACKSLASH:     case (KEY_BACKSLASH     | 0x080):
            case KEY_BACKSPACE:     case (KEY_BACKSPACE     | 0x080):

            case KEY_TAB:           case (KEY_TAB           | 0x080):
            case KEY_Q:             case (KEY_Q             | 0x080):
            case KEY_W:             case (KEY_W             | 0x080):
            case KEY_E:             case (KEY_E             | 0x080):
            case KEY_R:             case (KEY_R             | 0x080):
            case KEY_T:             case (KEY_T             | 0x080):
            case KEY_Y:             case (KEY_Y             | 0x080):
            case KEY_U:             case (KEY_U             | 0x080):
            case KEY_I:             case (KEY_I             | 0x080):
            case KEY_O:             case (KEY_O             | 0x080):
            case KEY_P:             case (KEY_P             | 0x080):
            case KEY_OPENBRACE:     case (KEY_OPENBRACE     | 0x080):
            case KEY_CLOSEBRACE:    case (KEY_CLOSEBRACE    | 0x080):

            case KEY_CAPSLOCK:      case (KEY_CAPSLOCK      | 0x080):
            case KEY_A:             case (KEY_A             | 0x080):
            case KEY_S:             case (KEY_S             | 0x080):
            case KEY_D:             case (KEY_D             | 0x080):
            case KEY_F:             case (KEY_F             | 0x080):
            case KEY_G:             case (KEY_G             | 0x080):
            case KEY_H:             case (KEY_H             | 0x080):
            case KEY_J:             case (KEY_J             | 0x080):
            case KEY_K:             case (KEY_K             | 0x080):
            case KEY_L:             case (KEY_L             | 0x080):
            case KEY_COLON:         case (KEY_COLON         | 0x080):
            case KEY_QUOTE:         case (KEY_QUOTE         | 0x080):
            case KEY_ENTER:         case (KEY_ENTER         | 0x080):

            case KEY_LSHIFT:        case (KEY_LSHIFT        | 0x080):
            case KEY_Z:             case (KEY_Z             | 0x080):
            case KEY_X:             case (KEY_X             | 0x080):
            case KEY_C:             case (KEY_C             | 0x080):
            case KEY_V:             case (KEY_V             | 0x080):
            case KEY_B:             case (KEY_B             | 0x080):
            case KEY_N:             case (KEY_N             | 0x080):
            case KEY_M:             case (KEY_M             | 0x080):
            case KEY_COMMA:         case (KEY_COMMA         | 0x080):
            case KEY_STOP:          case (KEY_STOP          | 0x080):
            case KEY_SLASH:         case (KEY_SLASH         | 0x080):
            case KEY_RSHIFT:        case (KEY_RSHIFT        | 0x080):

            case KEY_LCONTROL:      case (KEY_LCONTROL      | 0x080):
            case KEY_ALT:           case (KEY_ALT           | 0x080):
            case KEY_SPACE:         case (KEY_SPACE         | 0x080):
            case KEY_ALTGR:         case (KEY_ALTGR         | 0x080):
            case KEY_RCONTROL:      case (KEY_RCONTROL      | 0x080):

            case KEY_INSERT:        case (KEY_INSERT        | 0x080):
            case KEY_HOME:          case (KEY_HOME          | 0x080):
            case KEY_DEL:           case (KEY_DEL           | 0x080):
            case KEY_END:           case (KEY_END           | 0x080):

            case KEY_LEFT:          case (KEY_LEFT          | 0x080):
            case KEY_RIGHT:         case (KEY_RIGHT         | 0x080):
            case KEY_UP:            case (KEY_UP            | 0x080):
            case KEY_DOWN:          case (KEY_DOWN          | 0x080):

            case KEY_SLASH_PAD:     case (KEY_SLASH_PAD     | 0x080):
            case KEY_ASTERISK:      case (KEY_ASTERISK      | 0x080):
            case KEY_MINUS_PAD:     case (KEY_MINUS_PAD     | 0x080):
            case KEY_PLUS_PAD:      case (KEY_PLUS_PAD      | 0x080):
            case KEY_0_PAD:         case (KEY_0_PAD         | 0x080):
            case KEY_1_PAD:         case (KEY_1_PAD         | 0x080):
            case KEY_2_PAD:         case (KEY_2_PAD         | 0x080):
            case KEY_3_PAD:         case (KEY_3_PAD         | 0x080):
            case KEY_4_PAD:         case (KEY_4_PAD         | 0x080):
            case KEY_5_PAD:         case (KEY_5_PAD         | 0x080):
            case KEY_6_PAD:         case (KEY_6_PAD         | 0x080):
            case KEY_7_PAD:         case (KEY_7_PAD         | 0x080):
            case KEY_8_PAD:         case (KEY_8_PAD         | 0x080):
            case KEY_9_PAD:         case (KEY_9_PAD         | 0x080):
            case KEY_DEL_PAD:       case (KEY_DEL_PAD       | 0x080):
            {
                /* Potential "bee" key has been pressed */

                interf_key_lowcall_flag = 1;

                break;
            }

            default:
            {
                break;
            }
        }
    }

    return;
}
END_OF_FUNCTION(interf_key_lowlevel_react)



void interf_key_lowlevel_exit(int scancode)
{
    /*
       This function is temporarily replaces interf_key_lowlevel_react when
       exiting the emulator (until the exit is complete) to prevent problems
       due to stray keystrokes.  It should be redundant (and will be once the
       screendump in interactive mode problem is removed).
    */

    return;

    scancode = 0;
}
END_OF_FUNCTION(interf_key_lowlevel_exit)

#endif


module_data *interf_alloc(const char *module_name)
{
    UINT_16 i;
    module_data *what = NULL;

    /*
       Initialise variables (so repeat calls will work).
    */

    interf_keyboard_pollmode = 0;
    interf_mouse_pollmode    = 0;

    interf_snd_toggle_flag  = 0;
    interf_sig_rsetdwn_flag = 0;
    interf_sig_rsetup_flag  = 0;
    interf_scrn_rfsh_flag   = 0;
    interf_speedtoggle_flag = 0;
    interf_key_lowcall_flag = 0;
    interf_prtscr_flag      = 0;
    interf_interact_flag    = 0;

    interf_is_in_menu_mode = 0;
    interf_speed_emu_on    = 0;

    interf_scrn_mono_forecolour = 0;
    interf_scrn_mono_backcolour = 0;

    interf_scrn_physical_width  = 0;
    interf_scrn_physical_height = 0;

    interf_scrn_horiz_line_mult = 0;
    interf_scrn_vert_line_mult  = 0;
    interf_scrn_multip_fill     = 0;

    interf_scrn_l_left_offset  = 0;
    interf_scrn_l_s_width      = 1;
    interf_scrn_l_right_offset = 0;

    interf_scrn_l_top_offset    = 0;
    interf_scrn_l_s_height      = 1;
    interf_scrn_l_bottom_offset = 0;

    interf_scrn_left_correct = INTERF_SCRN_MAX_SCRNWIDTH_BEE/2;
    interf_scrn_top_correct  = INTERF_SCRN_MAX_SCRNHIGHT_BEE/2;

    interf_scrn_lsource = 0;
    interf_scrn_rsource = 0;
    interf_scrn_tsource = 0;
    interf_scrn_bsource = 0;

    interf_scrn_stepmode = 0;

    interf_scrn_colour_full = 63;
    interf_scrn_colour_half = 31;
    interf_scrn_colour_back = 0;

    interf_para_mode  = 4;
    interf_para_state = 0;

    interf_para_cycle_cnt  = 0;
    interf_para_upstat_cnt = 0;

    interf_para_dest_fp = NULL;
    interf_para_src_fp  = NULL;

    interf_para_havepulsed   = 0;
    interf_para_havestrobed  = 0;
    interf_para_trigpulsecnt = 0;

    interf_key_keydown     = 0;
    interf_key_keyfileupdn = 0;
    interf_key_worktable   = NULL;
    interf_key_cyclecnt    = 0;
    interf_key_sourcefp    = NULL;

    interf_key_response_count      = 0;
    interf_key_response_count_rfsh = 1;
    interf_key_release_updates_max = 1;
    interf_key_lpenreset_max       = 100;
    interf_key_lpenreset_min       = 90;

    interf_key_unshift_0      = 0;
    interf_key_unshift_2      = 0;
    interf_key_unshift_6      = 0;
    interf_key_unshift_7      = 0;
    interf_key_unshift_8      = 0;
    interf_key_unshift_9      = 0;
    interf_key_unshift_tilde  = 0;
    interf_key_unshift_quote  = 0;
    interf_key_unshift_colon  = 0;
    interf_key_unshift_equals = 0;
    interf_key_unshift_minus  = 0;

    interf_key_shift_0      = 0;
    interf_key_shift_2      = 0;
    interf_key_shift_6      = 0;
    interf_key_shift_7      = 0;
    interf_key_shift_8      = 0;
    interf_key_shift_9      = 0;
    interf_key_shift_tilde  = 0;
    interf_key_shift_quote  = 0;
    interf_key_shift_colon  = 0;
    interf_key_shift_equals = 0;
    interf_key_shift_minus  = 0;

    interf_key_effdown_0      = 0;
    interf_key_effdown_2      = 0;
    interf_key_effdown_6      = 0;
    interf_key_effdown_7      = 0;
    interf_key_effdown_8      = 0;
    interf_key_effdown_9      = 0;
    interf_key_effdown_tilde  = 0;
    interf_key_effdown_quote  = 0;
    interf_key_effdown_colon  = 0;
    interf_key_effdown_equals = 0;
    interf_key_effdown_minus  = 0;

    interf_snd_cardbad    = 0;
    interf_snd_sound_on_a = 0;
    interf_snd_sound_on_b = 0;
    interf_snd_sound_on_c = 0;
    interf_snd_frq_a      = 0;
    interf_snd_frq_b      = 0;
    interf_snd_frq_c      = 0;

    interf_snd_tone_here  = 0;
    interf_snd_click_here = 0;
    interf_snd_click_next = 0;
    interf_snd_click_stop = 0;

    interf_snd_speaker_state     = 0;
    interf_snd_speaker_state_old = 0;

    interf_snd_freq = 0;

    interf_snd_clkcycle_cnt_a = 0;
    interf_snd_clkcycle_cnt_b = 0;

    interf_tape_out_mode = 0;

    interf_tape_out_baud         = 0;
    interf_tape_out_type         = 0;
    interf_tape_out_state_course = 0;
    interf_tape_out_tapefreq     = 0;
    interf_tape_out_state_fine   = 0;
    interf_tape_out_byte         = 0;
    interf_tape_out_crc          = 0;
    interf_tape_out_kansascycle  = 0;
    interf_tape_out_elapsed_zclk = 0;
    interf_tape_out_hl_state     = 0;
    interf_tape_out_hl_length    = 0;
    interf_tape_out_hl_overall   = 0;
    interf_tape_out_hl_endit     = 0;
    interf_tape_out_filetype     = 0;
    interf_tape_out_filesize     = 0;
    interf_tape_out_fileload     = 0;
    interf_tape_out_fileexec     = 0;
    interf_tape_out_filesped     = 0;
    interf_tape_out_fileauto     = 0;

    interf_tape_out_dest_fp     = NULL;
    interf_tape_out_autosave_fp = NULL;

    interf_tape_in_type          = 0;
    interf_tape_in_buffer        = NULL;
    interf_tape_in_pos_byte      = 0;
    interf_tape_in_pos_bit       = 0;
    interf_tape_in_pos_bitcnt    = 0;
    interf_tape_in_spedchgbitcht = 0;
    interf_tape_in_bufsize       = 0;
    interf_tape_in_tapesped      = 0;
    interf_tape_in_state_fine    = 0;
    interf_tape_in_bit_fine      = 0;
    interf_tape_in_crc           = 0;
    interf_tape_in_kansascycle   = 0;
    interf_tape_in_elapsed_zclk  = 0;

    /*
       Interface installation routines
    */

    #ifdef IS_ALLEGRO
    {
        /*
           Install allegro
        */

        if ( allegro_init()      ) { return NULL; }
        if ( install_timer()     ) { return NULL; }
        if ( install_mouse() < 0 ) { return NULL; }
        if ( install_keyboard()  ) { return NULL; }

        interf_keyboard_pollmode = keyboard_needs_poll();
        interf_mouse_pollmode    = mouse_needs_poll();

        set_window_title("PicoMozzy");

        /*
           Lock functions and variables that will be used in an interupt
           context.
        */

        LOCK_FUNCTION(interf_key_lowlevel_react);
        LOCK_FUNCTION(interf_key_lowlevel_exit);

        LOCK_VARIABLE(interf_indir);

        LOCK_VARIABLE(interf_snd_toggle_flag);
        LOCK_VARIABLE(interf_sig_rsetdwn_flag);
        LOCK_VARIABLE(interf_sig_rsetup_flag);
        LOCK_VARIABLE(interf_scrn_rfsh_flag);
        LOCK_VARIABLE(interf_speedtoggle_flag);
        LOCK_VARIABLE(interf_key_lowcall_flag);
        LOCK_VARIABLE(interf_prtscr_flag);
        LOCK_VARIABLE(interf_interact_flag);
    }
    #endif

    #ifdef IS_WEB
    {
        /* put whatever needs to run before any emulation occurs here. */

        /*
           Clear the key state table.  key[KEY_A] is set 0 if the key is
           released, 1 if it is depressed (direct reflection of the state
           of the physical key, no debouncing).
        */

        for ( i = 0 ; i < KEY_MAX ; i++ )
        {
            key[i] = 0;
        }
    }
    #endif

    /*
       Initialise interface data
    */

    if ( !interf_is_alloced )
    {
        interf_is_alloced = 1;

        what = gen_module_data(module_name,1,0,0,0,0,0,16,8,2,10,10);

        DEREF_INFN(what,0) = interf_scrn_set_left_margin;
        DEREF_INFN(what,1) = interf_scrn_set_screen_width;
        DEREF_INFN(what,2) = interf_scrn_set_right_margin;
        DEREF_INFN(what,3) = interf_scrn_set_top_margin;
        DEREF_INFN(what,4) = interf_scrn_set_screen_height;
        DEREF_INFN(what,5) = interf_scrn_set_bottom_margin;
        DEREF_INFN(what,6) = interf_scrn_8pixel_draw;
        DEREF_INFN(what,7) = interf_para_data_written;
        DEREF_INFN(what,8) = interf_speaker_state_change;
        DEREF_INFN(what,9) = interf_tape_state_change;

        DEREF_INTERNAL(what) = NULL;

        interf_indir        = (void *) what;
        interf_indir_nonvol = (void *) what;
    }

    /*
       Initialise the keyboard emulation.
    */

    if ( ( interf_key_worktable = (UINT_8 *) DEBMALLOC(0x04003*sizeof(UINT_8)) ) == NULL )
    {
        return NULL;
    }

    for ( i = 0 ; i < 0x04003 ; i++ )
    {
        DEBDEREF(interf_key_worktable,i) = 0;
    }

    return what;
}

int interf_init(module_data *what)
{
    SetupData *all_setdat[2];
    int configerror;
    #ifdef SOUND_SOUNDCARD
    int i,j;
    #endif

    #ifdef IS_DJGPP
    _setcursortype(_NOCURSOR);
    #endif

    /*
       Load configuration file.
    */

    all_setdat[0] = interf_setdat;
    all_setdat[1] = NULL;

    if ( ( configerror = load_config_file(DEREF_STRGVAR(what,0),all_setdat) ) )
    {
        return configerror;
    }

    /*
       Initialise the parallel port.
    */

    #ifdef PARA_ACCESS_HARD
    {
        if ( !interf_para_lptport )
        {
            /*
               As the port number hasn't been over-ridden, find the bios
               port # by direct access to relevant parts of dos memory.
            */

            switch ( interf_para_lptnum )
            {
                case 1:
                {
                    dosmemget(0x0408,2,&interf_para_lptport);

                    break;
                }

                case 2:
                {
                    dosmemget(0x040A,2,&interf_para_lptport);

                    break;
                }

                case 3:
                {
                    dosmemget(0x040C,2,&interf_para_lptport);

                    break;
                }

                case 4:
                {
                    dosmemget(0x040E,2,&interf_para_lptport);

                    break;
                }

                default:
                {
                    break;
                }
            }
        }
    }
    #endif

    /*
       Speaker initialisation routines.
    */

    #ifdef SOUND_SOUNDCARD
    interf_snd_cardbad = install_sound(DIGI_AUTODETECT,MIDI_AUTODETECT,NULL);

    if ( ( interf_snd_sample_a = create_sample(8,0,SAMPLE_RATE,BUFF_SIZE_A) ) == NULL )
    {
        interf_snd_cardbad = 1;
    }

    if ( ( interf_snd_sample_b = create_sample(8,0,SAMPLE_RATE,BUFF_SIZE_B) ) == NULL )
    {
        interf_snd_cardbad = 1;
    }

    if ( ( interf_snd_sample_c = create_sample(8,0,SAMPLE_RATE,BUFF_SIZE_C) ) == NULL )
    {
        interf_snd_cardbad = 1;
    }

    if ( ( interf_click_sample = create_sample(8,0,SAMPLE_RATE,CLICKBUFF_SIZE) ) == NULL )
    {
        interf_snd_cardbad = 1;
    }

    if ( !interf_snd_cardbad )
    {
        {
            j = 1;

            ((UINT_8 *) (interf_snd_sample_a->data))[0] = 0;

            for ( i = 1 ; i < BUFF_SIZE_A ; i++ )
            {
                ((UINT_8 *) (interf_snd_sample_a->data))[i] = ((UINT_8 *) (interf_snd_sample_a->data))[i-1];

                if ( j >= CYCLE_LENGTH_A/2 )
                {
                    ((UINT_8 *) (interf_snd_sample_a->data))[i] = ~(((UINT_8 *) (interf_snd_sample_a->data))[i-1]);

                    j = 0;
                }

                j++;
            }
        }

        {
            j = 1;

            ((UINT_8 *) (interf_snd_sample_b->data))[0] = 0;

            for ( i = 1 ; i < BUFF_SIZE_B ; i++ )
            {
                ((UINT_8 *) (interf_snd_sample_b->data))[i] = ((UINT_8 *) (interf_snd_sample_b->data))[i-1];

                if ( j >= CYCLE_LENGTH_B/2 )
                {
                    ((UINT_8 *) (interf_snd_sample_b->data))[i] = ~(((UINT_8 *) (interf_snd_sample_b->data))[i-1]);

                    j = 0;
                }

                j++;
            }
        }

        {
            j = 1;

            ((UINT_8 *) (interf_snd_sample_c->data))[0] = 0;

            for ( i = 1 ; i < BUFF_SIZE_C ; i++ )
            {
                ((UINT_8 *) (interf_snd_sample_c->data))[i] = ((UINT_8 *) (interf_snd_sample_c->data))[i-1];

                if ( j >= CYCLE_LENGTH_C/2 )
                {
                    ((UINT_8 *) (interf_snd_sample_c->data))[i] = ~(((UINT_8 *) (interf_snd_sample_c->data))[i-1]);

                    j = 0;
                }

                j++;
            }
        }

        {
            j = 1;

            ((UINT_8 *) (interf_click_sample->data))[0] = 0;

            for ( i = 1 ; i < CLICKBUFF_SIZE ; i++ )
            {
                ((UINT_8 *) (interf_click_sample->data))[i] = ((UINT_8 *) (interf_click_sample->data))[i-1];

                if ( j >= CYCLE_LENGTH_A/2 )
                {
                    ((UINT_8 *) (interf_click_sample->data))[i] = ~(((UINT_8 *) (interf_click_sample->data))[i-1]);

                    j = 0;
                }

                j++;
            }
        }
    }
    #endif

    interf_snd_clkcycle_cnt_a = 0;
    interf_snd_clkcycle_cnt_b = 0;

    interf_snd_tone_here  = 0;
    interf_snd_click_here = 0;
    interf_snd_click_next = 0;
    interf_snd_click_stop = 0;

    interf_snd_speaker_state     = 0;
    interf_snd_speaker_state_old = 0;

    interf_snd_freq = 0;

    /*
       Screen initialisation
    */

    {
        switch ( interf_scrn_monitor_type )
        {
            case 0:
            {
                break;
            }

            case 1:
            {
                interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_GREEN;
                interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

                break;
            }

            case 2:
            {
                interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_AMBER;
                interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

                break;
            }

            case 3:
            {
                interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_WHITE;
                interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

                break;
            }

            case 4:
            {
                interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_BLACK;
                interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_GREEN;

                break;
            }

            case 5:
            {
                interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_BLACK;
                interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_AMBER;

                break;
            }

            case 6:
            {
                interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_BLACK;
                interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_WHITE;

                break;
            }

            default:
            {
                return 1;

                break;
            }
        }

        interf_scrn_set_bright(interf_scrn_bright);
        interf_scrn_set_contrast(interf_scrn_contrast);

        if ( interf_scrn_set_vidmode(interf_scrn_video_mode) )
        {
            return 1;
        }

        /*
           Make temporary (blitting) screen
        */

        #ifdef IS_ALLEGRO
        {
            if ( ( interf_scrn_bee_screen = create_bitmap(INTERF_SCRN_MAX_SCRNHIGHT_BEE,INTERF_SCRN_MAX_SCRNWIDTH_BEE) ) == NULL )
            {
                return 1;
            }
        }
        #endif
    }

    /*
       Draw splashscreen
    */

    #ifdef IS_ALLEGRO
    {
        char picosplasha[] = "PicoMozzy COLOUR  - a Microbee 32k Emulator v1.19.06.2005 (beta)";
        char picosplashb[] = "================================================================";
        char picosplashc[] = "                                                                ";
        char picosplashd[] = "             By Alistair Shilton (apsh@ecr.mu.oz.au)            ";
        char picosplashe[] = "    http://www.ee.mu.oz.au/pgrad/apsh/microbee/microbee.html    ";
        char picosplashf[] = "                                                                ";
        char picosplashg[] = "Key mapping: PAGE UP is the reset key.                          ";
        char picosplashh[] = "             RIGHT ALT is the linefeed key.                     ";
        char picosplashi[] = "             RIGHT CTRL is the break key.                       ";
        char picosplashj[] = "                                                                ";
        char picosplashk[] = "Control keys: PAGE DOWN      - emulation control menu.          ";
        char picosplashl[] = "              NUM LOCK       - toggle sound on/off.             ";
        char picosplashm[] = "              SCROLL LOCK    - toggle speed emulation on/off.   ";
        char picosplashn[] = "              SHIFT+PRTSCR   - Save screenshot in screen.bmp.   ";
        char picosplasho[] = "                                                                ";
        char picosplashp[] = "LED meanings: NUM LOCK    - lit if sound emulation is on.       ";
        char picosplashq[] = "              SCROLL LOCK - lit if speed emulation is on.       ";
        char picosplashr[] = "              CAPS LOCK   - lit if colour emulation is on.      ";
        char picosplashs[] = "              (note: may not work on all machines)              ";
        char picosplasht[] = "                                                                ";
        char picosplashu[] = "                                                                ";
        char picosplashv[] = "         Press any key to begin.                                ";
        char picosplashw[] = "";

        char *picosplash[] = { picosplasha, picosplashb, picosplashc,
                               picosplashd, picosplashe, picosplashf,
                               picosplashg, picosplashh, picosplashi,
                               picosplashj, picosplashk, picosplashl,
                               picosplashm, picosplashn, picosplasho,
                               picosplashp, picosplashq, picosplashr,
                               picosplashs, picosplasht, picosplashu,
                               picosplashv, picosplashw, NULL         };

        #ifdef IS_CYGWIN
        int w,h;
        int cw,ch;
        int i;
        RGB black = { 0,  0,  0,  0 };
        RGB green = { 0,  63, 0,  0 };

        cw = strlen(picosplash[0]);
        ch = 0;

        while ( picosplash[ch] != NULL )
        {
            ch++;
        }

        w = SPLASH_CHAR_HEIGHT*(SPLASH_BOUNDARY+SPLASH_BOUNDARY+cw);
        h = SPLASH_CHAR_HEIGHT*(SPLASH_BOUNDARY+SPLASH_BOUNDARY+ch);

        set_color_depth(8);

        if ( set_gfx_mode(GFX_SAFE,w,h,w,h) )
        {
            return 72;
        }

        set_palette(default_palette);

        set_color(1,&black);
        set_color(2,&green);

        set_clip(screen,0,0,w-1,h-1);
        rectfill(screen,0,0,w-1,h-1,1);

        for ( i = 0 ; i < ch ; i++ )
        {
            textout(screen,font,picosplash[i],(SPLASH_CHAR_HEIGHT*SPLASH_BOUNDARY)+1,SPLASH_CHAR_HEIGHT*(SPLASH_BOUNDARY+i)+1,2);
        }
        #endif

        #ifdef IS_DJGPP
        int top_leave;
        int left_leave;

        clrscr();
        gotoxy(1,1);

        left_leave = (ScreenCols()-64)/2;
        top_leave  = (ScreenRows()-25)/2;

        textcolor(LIGHTGRAY);

        gotoxy(left_leave,top_leave+ 1); cprintf(picosplash[0]);
        gotoxy(left_leave,top_leave+ 2); cprintf(picosplash[1]);
        gotoxy(left_leave,top_leave+ 3); cprintf(picosplash[2]);
        gotoxy(left_leave,top_leave+ 4); cprintf(picosplash[3]);
        gotoxy(left_leave,top_leave+ 5); cprintf(picosplash[4]);
        gotoxy(left_leave,top_leave+ 6); cprintf(picosplash[5]);
        gotoxy(left_leave,top_leave+ 7); cprintf(picosplash[6]);
        gotoxy(left_leave,top_leave+ 8); cprintf(picosplash[7]);
        gotoxy(left_leave,top_leave+ 9); cprintf(picosplash[8]);
        gotoxy(left_leave,top_leave+10); cprintf(picosplash[9]);
        gotoxy(left_leave,top_leave+11); cprintf(picosplash[10]);
        gotoxy(left_leave,top_leave+12); cprintf(picosplash[11]);
        gotoxy(left_leave,top_leave+13); cprintf(picosplash[12]);
        gotoxy(left_leave,top_leave+14); cprintf(picosplash[13]);
        gotoxy(left_leave,top_leave+15); cprintf(picosplash[14]);
        gotoxy(left_leave,top_leave+16); cprintf(picosplash[15]);
        gotoxy(left_leave,top_leave+17); cprintf(picosplash[16]);
        gotoxy(left_leave,top_leave+18); cprintf(picosplash[17]);
        gotoxy(left_leave,top_leave+19); cprintf(picosplash[18]);
        gotoxy(left_leave,top_leave+20); cprintf(picosplash[19]);

        textcolor(0x080 | LIGHTRED);

        gotoxy(left_leave,top_leave+21); cprintf("WARNING: DISABLE ANY SCREENSAVERS AND GO TO FULLSCREEN MODE     ");
        gotoxy(left_leave,top_leave+22); cprintf("         (ALT+ENTER IF NEEDED) BEFORE CONTINUING.               ");

        textcolor(LIGHTGRAY);

        gotoxy(left_leave,top_leave+23); cprintf(picosplash[20]);
        gotoxy(left_leave,top_leave+24); cprintf(picosplash[21]);

        textcolor(0x009); gotoxy(left_leave+10,top_leave+1); cprintf("C");
        textcolor(0x00a); gotoxy(left_leave+11,top_leave+1); cprintf("O");
        textcolor(0x00b); gotoxy(left_leave+12,top_leave+1); cprintf("L");
        textcolor(0x00c); gotoxy(left_leave+13,top_leave+1); cprintf("O");
        textcolor(0x00d); gotoxy(left_leave+14,top_leave+1); cprintf("U");
        textcolor(0x00e); gotoxy(left_leave+15,top_leave+1); cprintf("R");
        #endif

        clear_keybuf();
        readkey();

        #ifdef IS_CYGWIN
        set_gfx_mode(GFX_TEXT,0,0,0,0);
        #endif
    }
    #endif

    #ifdef IS_WEB
    {
        /* If there is a splash screen it should go here. */
    }
    #endif

    return 0;
}

void interf_go(module_data *what)
{
    /*
       Redirect keyboard actions to interf_key_lowlevel_react.
    */

    #ifdef IS_ALLEGRO
    {
        /*
           Set keyboard rates, install lowlevel callback function.
        */

        set_keyboard_rate(0,0); /* a windows bug makes this not work */
        clear_keybuf();
        keyboard_lowlevel_callback = interf_key_lowlevel_react;
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           You'll need some function to imitate the operation of the allegro
           keyboard routines here.  Basically when a key is pressed a routine
           following the same basic template as interf_key_lowlevel_react
           should be called to set various flags (see this function for more
           details).
        */
    }
    #endif

    /*
       Prime tape emulation
    */

    interf_tape_in_buffer   = NULL;
    interf_tape_out_dest_fp = NULL;

    interf_tape_out_reset_state();
    interf_tape_in_reset_state();
    interf_tape_out_set_mode_3();

    /*
       Final screen setup code
    */

    interf_scrn_fix_palette();

    #ifdef IS_ALLEGRO
    {
        set_window_title("PicoMozzy");

        /*
           Set graphics mode
        */

        set_color_depth(8);
        interf_scrn_set_gfxmode();
        set_palette(interf_scrn_palette);

        /*
           Clear the screens, go to the microbee screen space
        */

        set_clip(screen,0,0,interf_scrn_physical_width-1,interf_scrn_physical_height-1);
        rectfill(screen,0,0,interf_scrn_physical_width-1,interf_scrn_physical_height-1,interf_scrn_mono_backcolour+INTERF_SCRN_COLOUR_OFFSET);

        set_clip(interf_scrn_bee_screen,0,0,INTERF_SCRN_MAX_SCRNHIGHT_BEE-1,INTERF_SCRN_MAX_SCRNWIDTH_BEE-1);
        rectfill(interf_scrn_bee_screen,0,0,INTERF_SCRN_MAX_SCRNHIGHT_BEE-1,INTERF_SCRN_MAX_SCRNWIDTH_BEE-1,interf_scrn_mono_backcolour+INTERF_SCRN_COLOUR_OFFSET);
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Final screen setup goes here - ie. clear the screen, set the
           palette etc.  After this the screen should be ready to be drawn
           to.  Also, any sound initialisation routines should go here.
        */
    }
    #endif

    /*
       Set emulation speed.
    */

    if ( interf_speed_emu_on ) { INTERF_CTRL_SPEEDCTRL_ON(what);  }
    else                       { INTERF_CTRL_SPEEDCTRL_OFF(what); }

    return;

    what = NULL;
}

void interf_stop(module_data *what)
{
    /*
       Clear tape emulation
    */

    interf_tape_out_reset_state();
    interf_tape_in_reset_state();

    /*
       Redirect keyboard actions to interf_key_lowlevel_exit.
    */

    #ifdef IS_ALLEGRO
    {
        keyboard_lowlevel_callback = interf_key_lowlevel_exit;
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Disable keyboard emulation here.
        */
    }
    #endif

    /*
       Set parallel port mode to 4, which will close all relevant streams
       and finish stuff off.
    */

    interf_para_set_mode4();

    /*
       Stop any tones.
    */

    interf_snd_nosound();

    return;

    what = NULL;
}

void interf_remove(module_data *what)
{
    interf_is_alloced = 0;

    /*
       Kill sound.
    */

    #ifdef SOUND_PCSPEAKER
    interf_snd_nosound();
    #endif

    #ifdef SOUND_SOUNDCARD
    if ( !interf_snd_cardbad )
    {
        destroy_sample(interf_snd_sample_a);
        destroy_sample(interf_snd_sample_b);
        destroy_sample(interf_snd_sample_c);
        destroy_sample(interf_click_sample);
    }
    #endif

    /*
       Free keyboard stuff.
    */

    if ( what != NULL )
    {
        if ( interf_key_worktable != NULL )
        {
            DEBFREE(interf_key_worktable);

            interf_key_worktable = NULL;
        }

        free_module_data(what);

        #ifdef IS_ALLEGRO
        {
            if ( interf_scrn_bee_screen != NULL )
            {
                destroy_bitmap(interf_scrn_bee_screen);
            }
        }
        #endif
    }

    #ifdef IS_DJGPP
    _setcursortype(_NORMALCURSOR);
    #endif

    /*
       Remove allegro stuff
    */

    #ifdef IS_ALLEGRO
    {
        remove_keyboard();
        remove_mouse();
        remove_timer();
        allegro_exit();
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Free memory etc.
        */
    }
    #endif

    return;
}

void interf_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    UINT_16 i;

    /*
       Allow for non-interupt type hardware.
    */

    #ifdef IS_ALLEGRO
    {
        if ( interf_keyboard_pollmode ) { poll_keyboard(); }
        if ( interf_mouse_pollmode    ) { poll_mouse();    }
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           If the keyboard needs to be polled, this is where that should be
           done.  It's easier to do it in an interupt context, as with
           allegro.
        */
    }
    #endif

    /*
       Sound cycling
    */

    if ( interf_snd_sndon )
    {
        /*
           Update interf_snd_clkcycle_cnt_a, which measures the
           length of the current cycle (if there is one).  To prevent
           overflow causing false notes, if the cycle count exceeds the
           overflow level then trim it back.
        */

        interf_snd_clkcycle_cnt_a += num_cycles;

        if ( interf_snd_clkcycle_cnt_a > interf_snd_clkcnt_max )
        {
            interf_snd_clkcycle_cnt_a = interf_snd_clkcnt_fback;
        }

        /*
           FIXME: when the soundcard code is correctly this should be put
                  inside a #ifdef SOUND_PCSPEAKER.
        */

        if ( lsync_point )
        {
            interf_snd_click_stop = 0;

            /*
               OK, timing is synced correctly ish.
            */

            if ( interf_snd_tone_here )
            {
                /*
                   A tone has begun or is continuing.
                */

                if ( interf_snd_freq < interf_snd_minfreq )
                {
                    /*
                       The tonal frequency is too low - this is usually due
                       to frequency miscalculation at the start or end of a
                       tone, which will be heard as a "click".  So we insert
                       a click noise here.
                    */

                    interf_snd_soundclick();

                    interf_snd_click_here = 0;
                    interf_snd_click_next = 0;
                    interf_snd_click_stop = 1;
                    interf_snd_tone_here  = 0;

                    interf_snd_clkcycle_cnt_b = 0;

                    interf_snd_freq = 0;
                }

                else
                {
                    if ( interf_snd_click_here )
                    {
                        /*
                           Restarting the tone will insert an audible "click"
                           into the tone.  This makes sure there are no
                           missed pauses (see cbrikout.mwb, for example).
                        */

                        interf_snd_sound(interf_snd_freq);
                    }

                    interf_snd_click_here = interf_snd_click_next;
                    interf_snd_click_next = 0;

                    /*
                       When a tone finishes, this may be detected by the
                       cycle length becoming very large.  As a rough
                       heuristic, we say a tone has finished if the cycle
                       counter goes for two cycles of the frequency being
                       played.  Then we set the various variables so that
                       it will be shut off at the next sync point.
                    */

                    if ( interf_snd_clkcycle_cnt_a > 2*interf_snd_clkcycle_cnt_b )
                    {
                        interf_snd_tone_here = 0;
                        interf_snd_freq      = 0;
                    }
                }
            }

            else
            {
                /*
                   Either the tone has finished (indicated by
                   interf_snd_clkcycle_cnt_b != 0) or there may be
                   a subsonic "click" that must be stopped (indicated by
                   interf_snd_click_stop != 0).  In either case, turn off any
                   sound.
                */

                if ( interf_snd_clkcycle_cnt_b | interf_snd_click_stop )
                {
                    interf_snd_nosound();

                    interf_snd_click_here = 0;
                    interf_snd_click_next = 0;

                    interf_snd_clkcycle_cnt_b = 0;

                    interf_snd_freq = 0;

                    interf_snd_click_stop = 0;
                }
            }
        }
    }

    /*
       Keyboard cycling
    */

    if ( interf_snd_toggle_flag  ) { interf_snd_toggle_flag  = 0; interf_snd_toggle_snd();         }
    if ( interf_sig_rsetdwn_flag ) { interf_sig_rsetdwn_flag = 0; INTERF_CTRL_RESET_ON(what);      }
    if ( interf_sig_rsetup_flag  ) { interf_sig_rsetup_flag  = 0; INTERF_CTRL_RESET_OFF(what);     }
    if ( interf_scrn_rfsh_flag   ) { interf_scrn_rfsh_flag   = 0; INTERF_SCRN_RFSH(what);          }

    if ( interf_speedtoggle_flag )
    {
        interf_speedtoggle_flag = 0;

        if ( interf_speed_emu_on )
        {
            INTERF_CTRL_SPEEDCTRL_OFF(what);

            interf_speed_emu_on = 0;
        }

        else
        {
            INTERF_CTRL_SPEEDCTRL_ON(what);

            interf_speed_emu_on = 1;
        }
    }

    if ( interf_key_sourcefp == NULL )
    {
        /*
           First off, if a key has been pressed need to update the state
           of the microbee internal key map.
        */

        if ( interf_key_lowcall_flag )
        {
            interf_key_lowcall_flag = 0;

            /*
               The following keys are the same shiftwise between the
               microbee and the pc keyboards - for example, SHIFT-5 will
               give % on either keyboard.  Hence there is no need for any
               fancy stuff with these keys.

               KEY ASSUMPTION: KB_NORMAL = 0x001
            */

                                       /* 0x001 */
            DEBDEREF(interf_key_worktable,0x011) = key[KEY_A]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x021) = key[KEY_B]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x031) = key[KEY_C]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x041) = key[KEY_D]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x051) = key[KEY_E]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x061) = key[KEY_F]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x071) = key[KEY_G]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x081) = key[KEY_H]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x091) = key[KEY_I]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x0A1) = key[KEY_J]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x0B1) = key[KEY_K]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x0C1) = key[KEY_L]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x0D1) = key[KEY_M]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x0E1) = key[KEY_N]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x0F1) = key[KEY_O]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x101) = key[KEY_P]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x111) = key[KEY_Q]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x121) = key[KEY_R]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x131) = key[KEY_S]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x141) = key[KEY_T]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x151) = key[KEY_U]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x161) = key[KEY_V]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x171) = key[KEY_W]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x181) = key[KEY_X]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x191) = key[KEY_Y]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x1A1) = key[KEY_Z]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x1B1) = key[KEY_OPENBRACE]  & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x1C1) = key[KEY_BACKSLASH]  & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x1D1) = key[KEY_CLOSEBRACE] & KB_NORMAL;
                                       /* 0x1E1 */
            DEBDEREF(interf_key_worktable,0x1F1) = key[KEY_DEL]        & KB_NORMAL;
                                       /* 0x201 */
            DEBDEREF(interf_key_worktable,0x211) = key[KEY_1]          & KB_NORMAL;
                                       /* 0x221 */
            DEBDEREF(interf_key_worktable,0x231) = key[KEY_3]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x241) = key[KEY_4]          & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x251) = key[KEY_5]          & KB_NORMAL;
                                       /* 0x261 */
                                       /* 0x271 */
                                       /* 0x281 */
                                       /* 0x291 */
                                       /* 0x2A1 */
                                       /* 0x2B1 */
            DEBDEREF(interf_key_worktable,0x2C1) = key[KEY_COMMA]      & KB_NORMAL;
                                       /* 0x2D1 */
            DEBDEREF(interf_key_worktable,0x2E1) = key[KEY_STOP]       & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x2F1) = key[KEY_SLASH]      & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x301) = key[KEY_ESC]        & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x311) = key[KEY_BACKSPACE]  & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x321) = key[KEY_TAB]        & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x331) = key[KEY_ALTGR]      & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x341) = key[KEY_ENTER]      & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x351) = key[KEY_CAPSLOCK]   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x361) = key[KEY_RCONTROL]   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x371) = key[KEY_SPACE]      & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x381) = 0                   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x391) = key[KEY_LCONTROL]   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x3A1) = 0                   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x3B1) = 0                   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x3C1) = 0                   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x3D1) = 0                   & KB_NORMAL;
            DEBDEREF(interf_key_worktable,0x3E1) = 0                   & KB_NORMAL;
                                       /* 0x3F1 */

            /*
               The rest of the keys, however, need shift inversion, which
               has been described earlier.
            */

            /*
               Clear any sticky responses
            */

            if ( !( key[KEY_0]      & KB_NORMAL ) ) { interf_key_effdown_0      = 0; interf_key_unshift_0      = 0; interf_key_shift_0      = 0; }
            if ( !( key[KEY_2]      & KB_NORMAL ) ) { interf_key_effdown_2      = 0; interf_key_unshift_2      = 0; interf_key_shift_2      = 0; }
            if ( !( key[KEY_6]      & KB_NORMAL ) ) { interf_key_effdown_6      = 0; interf_key_unshift_6      = 0; interf_key_shift_6      = 0; }
            if ( !( key[KEY_7]      & KB_NORMAL ) ) { interf_key_effdown_7      = 0; interf_key_unshift_7      = 0; interf_key_shift_7      = 0; }
            if ( !( key[KEY_8]      & KB_NORMAL ) ) { interf_key_effdown_8      = 0; interf_key_unshift_8      = 0; interf_key_shift_8      = 0; }
            if ( !( key[KEY_9]      & KB_NORMAL ) ) { interf_key_effdown_9      = 0; interf_key_unshift_9      = 0; interf_key_shift_9      = 0; }
            if ( !( key[KEY_TILDE]  & KB_NORMAL ) ) { interf_key_effdown_tilde  = 0; interf_key_unshift_tilde  = 0; interf_key_shift_tilde  = 0; }
            if ( !( key[KEY_QUOTE]  & KB_NORMAL ) ) { interf_key_effdown_quote  = 0; interf_key_unshift_quote  = 0; interf_key_shift_quote  = 0; }
            if ( !( key[KEY_COLON]  & KB_NORMAL ) ) { interf_key_effdown_colon  = 0; interf_key_unshift_colon  = 0; interf_key_shift_colon  = 0; }
            if ( !( key[KEY_EQUALS] & KB_NORMAL ) ) { interf_key_effdown_equals = 0; interf_key_unshift_equals = 0; interf_key_shift_equals = 0; }
            if ( !( key[KEY_MINUS]  & KB_NORMAL ) ) { interf_key_effdown_minus  = 0; interf_key_unshift_minus  = 0; interf_key_shift_minus  = 0; }

            if ( key[KEY_LSHIFT] || key[KEY_RSHIFT] )
            {
                /*
                   Set the effective key states, allowing for stickiness.
                */

                if ( ( key[KEY_0]      & KB_NORMAL ) && !interf_key_unshift_0      ) { interf_key_shift_0      = 1; interf_key_effdown_0      = 1; } else { interf_key_shift_0      = 0; interf_key_effdown_0      = 0; }
                if ( ( key[KEY_2]      & KB_NORMAL ) && !interf_key_unshift_2      ) { interf_key_shift_2      = 1; interf_key_effdown_2      = 1; } else { interf_key_shift_2      = 0; interf_key_effdown_2      = 0; }
                if ( ( key[KEY_6]      & KB_NORMAL ) && !interf_key_unshift_6      ) { interf_key_shift_6      = 1; interf_key_effdown_6      = 1; } else { interf_key_shift_6      = 0; interf_key_effdown_6      = 0; }
                if ( ( key[KEY_7]      & KB_NORMAL ) && !interf_key_unshift_7      ) { interf_key_shift_7      = 1; interf_key_effdown_7      = 1; } else { interf_key_shift_7      = 0; interf_key_effdown_7      = 0; }
                if ( ( key[KEY_8]      & KB_NORMAL ) && !interf_key_unshift_8      ) { interf_key_shift_8      = 1; interf_key_effdown_8      = 1; } else { interf_key_shift_8      = 0; interf_key_effdown_8      = 0; }
                if ( ( key[KEY_9]      & KB_NORMAL ) && !interf_key_unshift_9      ) { interf_key_shift_9      = 1; interf_key_effdown_9      = 1; } else { interf_key_shift_9      = 0; interf_key_effdown_9      = 0; }
                if ( ( key[KEY_TILDE]  & KB_NORMAL ) && !interf_key_unshift_tilde  ) { interf_key_shift_tilde  = 1; interf_key_effdown_tilde  = 1; } else { interf_key_shift_tilde  = 0; interf_key_effdown_tilde  = 0; }
                if ( ( key[KEY_QUOTE]  & KB_NORMAL ) && !interf_key_unshift_quote  ) { interf_key_shift_quote  = 1; interf_key_effdown_quote  = 1; } else { interf_key_shift_quote  = 0; interf_key_effdown_quote  = 0; }
                if ( ( key[KEY_COLON]  & KB_NORMAL ) && !interf_key_unshift_colon  ) { interf_key_shift_colon  = 1; interf_key_effdown_colon  = 1; } else { interf_key_shift_colon  = 0; interf_key_effdown_colon  = 0; }
                if ( ( key[KEY_EQUALS] & KB_NORMAL ) && !interf_key_unshift_equals ) { interf_key_shift_equals = 1; interf_key_effdown_equals = 1; } else { interf_key_shift_equals = 0; interf_key_effdown_equals = 0; }
                if ( ( key[KEY_MINUS]  & KB_NORMAL ) && !interf_key_unshift_minus  ) { interf_key_shift_minus  = 1; interf_key_effdown_minus  = 1; } else { interf_key_shift_minus  = 0; interf_key_effdown_minus  = 0; }

                /*
                   set the microbee keyboard state, using restuck key states.
                */

                DEBDEREF(interf_key_worktable,0x001) = interf_key_effdown_2                             | interf_key_unshift_tilde;
                DEBDEREF(interf_key_worktable,0x1E1) = interf_key_effdown_tilde | interf_key_effdown_6  | 0;
                DEBDEREF(interf_key_worktable,0x201) = 0                                                | interf_key_unshift_0;
                DEBDEREF(interf_key_worktable,0x221) = interf_key_effdown_quote                         | interf_key_unshift_2;
                DEBDEREF(interf_key_worktable,0x261) = interf_key_effdown_7                             | interf_key_unshift_6;
                DEBDEREF(interf_key_worktable,0x271) = 0                                                | interf_key_unshift_7 | interf_key_unshift_quote;
                DEBDEREF(interf_key_worktable,0x281) = interf_key_effdown_9                             | interf_key_unshift_8;
                DEBDEREF(interf_key_worktable,0x291) = interf_key_effdown_0                             | interf_key_unshift_9;
                DEBDEREF(interf_key_worktable,0x2A1) = interf_key_effdown_8 | interf_key_effdown_colon  | 0;
                DEBDEREF(interf_key_worktable,0x2B1) = interf_key_effdown_equals                        | interf_key_unshift_colon;
                DEBDEREF(interf_key_worktable,0x2D1) = 0                                                | interf_key_unshift_minus | interf_key_unshift_equals;

                DEBDEREF(interf_key_worktable,0x3F1) = ( ( key[KEY_2] | key[KEY_6] | key[KEY_COLON] ) & KB_NORMAL ) ^ KB_NORMAL;
            }                                                                   
                                                                        
            else
            {                                                                   
                /*
                   Set the effective key states, allowing for stickiness.
                */

                if ( ( key[KEY_0]      & KB_NORMAL ) && !interf_key_shift_0      ) { interf_key_unshift_0      = 1; interf_key_effdown_0      = 1; } else { interf_key_unshift_0      = 0; interf_key_effdown_0      = 0; }
                if ( ( key[KEY_2]      & KB_NORMAL ) && !interf_key_shift_2      ) { interf_key_unshift_2      = 1; interf_key_effdown_2      = 1; } else { interf_key_unshift_2      = 0; interf_key_effdown_2      = 0; }
                if ( ( key[KEY_6]      & KB_NORMAL ) && !interf_key_shift_6      ) { interf_key_unshift_6      = 1; interf_key_effdown_6      = 1; } else { interf_key_unshift_6      = 0; interf_key_effdown_6      = 0; }
                if ( ( key[KEY_7]      & KB_NORMAL ) && !interf_key_shift_7      ) { interf_key_unshift_7      = 1; interf_key_effdown_7      = 1; } else { interf_key_unshift_7      = 0; interf_key_effdown_7      = 0; }
                if ( ( key[KEY_8]      & KB_NORMAL ) && !interf_key_shift_8      ) { interf_key_unshift_8      = 1; interf_key_effdown_8      = 1; } else { interf_key_unshift_8      = 0; interf_key_effdown_8      = 0; }
                if ( ( key[KEY_9]      & KB_NORMAL ) && !interf_key_shift_9      ) { interf_key_unshift_9      = 1; interf_key_effdown_9      = 1; } else { interf_key_unshift_9      = 0; interf_key_effdown_9      = 0; }
                if ( ( key[KEY_TILDE]  & KB_NORMAL ) && !interf_key_shift_tilde  ) { interf_key_unshift_tilde  = 1; interf_key_effdown_tilde  = 1; } else { interf_key_unshift_tilde  = 0; interf_key_effdown_tilde  = 0; }
                if ( ( key[KEY_QUOTE]  & KB_NORMAL ) && !interf_key_shift_quote  ) { interf_key_unshift_quote  = 1; interf_key_effdown_quote  = 1; } else { interf_key_unshift_quote  = 0; interf_key_effdown_quote  = 0; }
                if ( ( key[KEY_COLON]  & KB_NORMAL ) && !interf_key_shift_colon  ) { interf_key_unshift_colon  = 1; interf_key_effdown_colon  = 1; } else { interf_key_unshift_colon  = 0; interf_key_effdown_colon  = 0; }
                if ( ( key[KEY_EQUALS] & KB_NORMAL ) && !interf_key_shift_equals ) { interf_key_unshift_equals = 1; interf_key_effdown_equals = 1; } else { interf_key_unshift_equals = 0; interf_key_effdown_equals = 0; }
                if ( ( key[KEY_MINUS]  & KB_NORMAL ) && !interf_key_shift_minus  ) { interf_key_unshift_minus  = 1; interf_key_effdown_minus  = 1; } else { interf_key_unshift_minus  = 0; interf_key_effdown_minus  = 0; }

                /*
                   set the microbee keyboard state, using restuck key states
                   and the unshift states.
                */

                DEBDEREF(interf_key_worktable,0x001) = interf_key_effdown_tilde                             | interf_key_shift_2;
                DEBDEREF(interf_key_worktable,0x1E1) = 0                                                    | interf_key_shift_tilde | interf_key_shift_6;
                DEBDEREF(interf_key_worktable,0x201) = interf_key_effdown_0                                 | 0;
                DEBDEREF(interf_key_worktable,0x221) = interf_key_effdown_2                                 | interf_key_shift_quote;
                DEBDEREF(interf_key_worktable,0x261) = interf_key_effdown_6                                 | interf_key_shift_7;
                DEBDEREF(interf_key_worktable,0x271) = interf_key_effdown_7 | interf_key_effdown_quote      | 0;
                DEBDEREF(interf_key_worktable,0x281) = interf_key_effdown_8                                 | interf_key_shift_9;
                DEBDEREF(interf_key_worktable,0x291) = interf_key_effdown_9                                 | interf_key_shift_0;
                DEBDEREF(interf_key_worktable,0x2A1) = 0                                                    | interf_key_shift_8 | interf_key_shift_colon;
                DEBDEREF(interf_key_worktable,0x2B1) = interf_key_effdown_colon                             | interf_key_shift_equals;
                DEBDEREF(interf_key_worktable,0x2D1) = interf_key_effdown_minus | interf_key_effdown_equals | 0;
                                                                    
                DEBDEREF(interf_key_worktable,0x3F1) = ( key[KEY_TILDE] | key[KEY_QUOTE] | key[KEY_EQUALS] | key[KEY_0] ) & KB_NORMAL;
            }

            interf_key_keydown = 0;

            for ( i = 0x00000 ; i <= 0x003F0 ; i += 0x00010 )
            {
                if ( DEBDEREF(interf_key_worktable,i+1) & !DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i) )
                {
                    /* Key has just been pressed. */
                    /* -= start the counter =- */

                    DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i) = 1;
                    DEBDEREF(interf_key_worktable,i+2)      = interf_key_clkcnt_max;

                    DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),i) = 0;
                    DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),i) = 0;
                }

                interf_key_keydown |= DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i);
            }
        }

        /*
           Then do rest of this stuff.
        */

        interf_key_cyclecnt += num_cycles;

        if ( interf_key_cyclecnt > interf_key_rfsh_cycles )
        {
            interf_key_cyclecnt -= interf_key_rfsh_cycles;

            if ( interf_key_keydown )
            {
                interf_key_keydown = 0;

                for ( i = 0x00000 ; i <= 0x003F0 ; i += 0x00010 )
                {
                    if ( DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i) )
                    {
                        if ( ( DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),i) >= interf_key_response_count      ) &&
                             ( DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),i) >= interf_key_response_count_rfsh )    )
                        {
                            /*
                               Key has been read, so we can safely short-
                               circuit the countdown process.
                            */

                            DEBDEREF(interf_key_worktable,i+2) = 0;
                        }

                        if ( DEBDEREF(interf_key_worktable,i+2) == 0 )
                        {
                            /*
                               Key has been down long enough to be read (and
                               may have been read - see previous if
                               statement), so no need to hold the key down
                               any longer.  Instead, just make the key state
                               correspond to reality.
                            */

                            DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i) = DEBDEREF(interf_key_worktable,i+1);
                        }

                        else
                        {
                            /*
                               Continue the countdown, keep key state const.
                            */

                            DEBDEREF(interf_key_worktable,i+2)--;
                        }

                        interf_key_keydown |= DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i);
                    }
                }
            }

            if ( !interf_key_keydown )
            {
                INTERF_KEY_RSET_COUNTER(what)  = 0;
                INTERF_KEY_UPDAT_COUNTER(what) = 0;
            }
        }
    }

    else
    {
        interf_key_lowcall_flag = 0;

        if ( !interf_key_keyfileupdn )
        {
            if ( ( ( INTERF_KEY_UPDAT_COUNTER(what) >= interf_key_release_updates_max ) || ( INTERF_KEY_RSET_COUNTER(what) >= interf_key_lpenreset_max ) ) && ( INTERF_KEY_RSET_COUNTER(what) >= interf_key_lpenreset_min ) )
            {
                interf_key_keydown     = 0;
                interf_key_keyfileupdn = 1;

                switch ( pc_fgetc(interf_key_sourcefp) )
                {
                    case '@':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x000) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x000) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x000) = 0; break; }
                    case 'a':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x010) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x010) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x010) = 0; break; }
                    case 'b':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x020) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x020) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x020) = 0; break; }
                    case 'c':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x030) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x030) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x030) = 0; break; }
                    case 'd':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x040) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x040) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x040) = 0; break; }
                    case 'e':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x050) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x050) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x050) = 0; break; }
                    case 'f':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x060) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x060) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x060) = 0; break; }
                    case 'g':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x070) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x070) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x070) = 0; break; }
                    case 'h':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x080) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x080) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x080) = 0; break; }
                    case 'i':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x090) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x090) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x090) = 0; break; }
                    case 'j':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0a0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0a0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0a0) = 0; break; }
                    case 'k':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0b0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0b0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0b0) = 0; break; }
                    case 'l':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0c0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0c0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0c0) = 0; break; }
                    case 'm':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0d0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0d0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0d0) = 0; break; }
                    case 'n':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0e0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0e0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0e0) = 0; break; }
                    case 'o':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0f0) = 0; break; }
                    case 'p':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x100) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x100) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x100) = 0; break; }
                    case 'q':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x110) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x110) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x110) = 0; break; }
                    case 'r':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x120) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x120) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x120) = 0; break; }
                    case 's':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x130) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x130) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x130) = 0; break; }
                    case 't':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x140) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x140) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x140) = 0; break; }
                    case 'u':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x150) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x150) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x150) = 0; break; }
                    case 'v':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x160) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x160) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x160) = 0; break; }
                    case 'w':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x170) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x170) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x170) = 0; break; }
                    case 'x':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x180) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x180) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x180) = 0; break; }
                    case 'y':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x190) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x190) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x190) = 0; break; }
                    case 'z':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1a0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1a0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1a0) = 0; break; }
                    case '[':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1b0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1b0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1b0) = 0; break; }
                    case '\\': { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1c0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1c0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1c0) = 0; break; }
                    case ']':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1d0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1d0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1d0) = 0; break; }
                    case '^':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1e0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1e0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1e0) = 0; break; }

                    case '0':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x200) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x200) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x200) = 0; break; }
                    case '1':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x210) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x210) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x210) = 0; break; }
                    case '2':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x220) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x220) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x220) = 0; break; }
                    case '3':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x230) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x230) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x230) = 0; break; }
                    case '4':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x240) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x240) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x240) = 0; break; }
                    case '5':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x250) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x250) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x250) = 0; break; }
                    case '6':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x260) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x260) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x260) = 0; break; }
                    case '7':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x270) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x270) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x270) = 0; break; }
                    case '8':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x280) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x280) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x280) = 0; break; }
                    case '9':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x290) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x290) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x290) = 0; break; }
                    case ':':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2a0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2a0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2a0) = 0; break; }
                    case ';':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2b0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2b0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2b0) = 0; break; }
                    case ',':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2c0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2c0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2c0) = 0; break; }
                    case '-':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2d0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2d0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2d0) = 0; break; }
                    case '.':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2e0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2e0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2e0) = 0; break; }
                    case '/':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2f0) = 0; break; }



                    case '`':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x000) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x000) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x000) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'A':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x010) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x010) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x010) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'B':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x020) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x020) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x020) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'C':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x030) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x030) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x030) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'D':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x040) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x040) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x040) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'E':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x050) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x050) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x050) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'F':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x060) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x060) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x060) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'G':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x070) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x070) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x070) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'H':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x080) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x080) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x080) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'I':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x090) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x090) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x090) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'J':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0a0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0a0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0a0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'K':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0b0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0b0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0b0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'L':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0c0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0c0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0c0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'M':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0d0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0d0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0d0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'N':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0e0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0e0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0e0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'O':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x0f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x0f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x0f0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'P':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x100) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x100) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x100) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'Q':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x110) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x110) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x110) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'R':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x120) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x120) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x120) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'S':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x130) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x130) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x130) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'T':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x140) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x140) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x140) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'U':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x150) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x150) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x150) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'V':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x160) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x160) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x160) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'W':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x170) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x170) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x170) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'X':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x180) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x180) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x180) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'Y':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x190) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x190) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x190) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case 'Z':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1a0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1a0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1a0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '{':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1b0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1b0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1b0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '|':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1c0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1c0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1c0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '}':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1d0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1d0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1d0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '~':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x1e0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x1e0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x1e0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }


                    case '!':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x210) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x210) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x210) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '\"': { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x220) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x220) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x220) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '#':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x230) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x230) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x230) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '$':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x240) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x240) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x240) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '%':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x250) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x250) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x250) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '&':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x260) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x260) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x260) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '\'': { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x270) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x270) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x270) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '(':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x280) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x280) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x280) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case ')':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x290) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x290) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x290) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '*':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2a0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2a0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2a0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '+':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2b0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2b0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2b0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '<':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2c0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2c0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2c0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '=':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2d0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2d0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2d0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '>':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2e0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2e0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2e0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }
                    case '\?': { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x2f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x2f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x2f0) = 0; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x3f0) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x3f0) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x3f0) = 0; break; }




                    case '\t': { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x320) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x320) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x320) = 0; break; }
                    case '\n': { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x340) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x340) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x340) = 0; break; }
                    case ' ':  { interf_key_keydown = 1; DEBDEREF(INTERF_KEY_LPEN_TABLE(what),0x370) = 1; DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),0x370) = 0; DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),0x370) = 0; break; }


                    default: { break; }
                }
            }
        }

        else
        {
            /*
               Wait for keypresses to register.
            */

            for ( i = 0x00000 ; i <= 0x003F0 ; i += 0x00010 )
            {
                if ( DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i) )
                {
                    if ( ( DEBDEREF(INTERF_KEY_FEEDBACK_TABLE(what),i) < interf_key_response_count      ) ||
                         ( DEBDEREF(INTERF_KEY_FEEDRFSH_TABLE(what),i) < interf_key_response_count_rfsh )    )
                    {
                        return;
                    }

                    DEBDEREF(INTERF_KEY_LPEN_TABLE(what),i) = 0;
                }
            }

            INTERF_KEY_RSET_COUNTER(what)  = 0;
            INTERF_KEY_UPDAT_COUNTER(what) = 0;

            interf_key_keydown     = 0;
            interf_key_keyfileupdn = 0;

            if ( pc_feof(interf_key_sourcefp) )
            {
                interf_key_closefile();
            }
        }
    }


    /*
       Parallel port cycling
    */

    switch ( interf_para_mode )
    {
        case 0:
        {
            /* output direct to pc parallel port */

            #ifdef PARA_ACCESS_HARD
            {
                if ( lsync_point )
                {
                    if ( interf_para_trigpulsecnt )
                    {
                        interf_para_trigpulsecnt--;

                        if ( !interf_para_trigpulsecnt )
                        {
                            outportb(interf_para_lptport+2,0x00C);
                        }
                    }
                }

                interf_para_upstat_cnt++;

                if ( interf_para_upstat_cnt >= interf_para_readgranularity )
                {
                    interf_para_upstat_cnt = 0;

                    INTERF_PARA_DATA_BUS_OUT(what) = inportb(interf_para_lptport);
                    INTERF_PARA_STRB_BUS_OUT(what) = inportb(interf_para_lptport+1);

                    if ( interf_para_sim_pulse )
                    {
                        /*
                           The ack strobe has most likely been missed, so
                           also check the busy line, and if it's high the
                           printer isn't busy so whatever happened must have
                           finished.
                        */

                        if ( !( INTERF_PARA_STRB_BUS_OUT(what) & 0x040 ) || ( INTERF_PARA_STRB_BUS_OUT(what) & 0x080 ) )
                        {
                            /*
                               The printer is ready - either it's just
                               printed the last char or nothing at all.  If
                               interf_para_havepulsed is set then it's the
                               former.  If this is the case, then generate
                               a *pulse* - that's what the
                               interf_para_havestrobed does - it lets strobe
                               be set low for precisely one thingy, then it
                               goes high again until the next char is sent.
                            */

                            if ( interf_para_havepulsed && !interf_para_havestrobed )
                            {
                                INTERF_PARA_STRB_BUS_OUT(what) = 0;

                                interf_para_havestrobed = 1;
                            }

                            else
                            {
                                INTERF_PARA_STRB_BUS_OUT(what) = 1;
                            }
                        }

                        else
                        {
                            INTERF_PARA_STRB_BUS_OUT(what) = 1;

                            interf_para_havestrobed = 0;
                        }
                    }

                    else
                    {
                        if ( !( INTERF_PARA_STRB_BUS_OUT(what) & 0x040 ) )
                        {
                            INTERF_PARA_STRB_BUS_OUT(what) = 0;
                        }

                        else
                        {
                            INTERF_PARA_STRB_BUS_OUT(what) = 1;
                        }
                    }
                }

                INTERF_PARA_STROBE(what);
            }
            #endif

            break;
        }

        case 1:
        case 2:
        case 5:
        {
            /* output to file, NULL(handshaking), bios */

            switch ( interf_para_state )
            {
                case 1:
                {
                    interf_para_cycle_cnt += num_cycles;

                    if ( interf_para_cycle_cnt >= interf_para_responsetime_out )
                    {
                        INTERF_PARA_STRB_BUS_OUT(what) = 0;

                        INTERF_PARA_STROBE(what);

                        interf_para_cycle_cnt = 0;
                        interf_para_state     = 2;
                    }

                    break;
                }

                case 2:
                {
                    interf_para_cycle_cnt += num_cycles;

                    if ( interf_para_cycle_cnt >= interf_para_strobe_time )
                    {
                        INTERF_PARA_STRB_BUS_OUT(what) = 1;

                        INTERF_PARA_STROBE(what);

                        interf_para_cycle_cnt = 0;
                        interf_para_state     = 0;
                    }

                    break;
                }

                default:
                {
                    break;
                }
            }

            break;
        }

        case 3:
        {
            /* input from file */

            switch ( interf_para_state )
            {
                case 1:
                {
                    interf_para_cycle_cnt += num_cycles;

                    if ( interf_para_cycle_cnt >= interf_para_responsetime_in )
                    {
                        INTERF_PARA_STRB_BUS_OUT(what) = 0;

                        if ( pc_feof(interf_para_src_fp) )
                        {
                            interf_para_set_mode4();

                            INTERF_PARA_DATA_BUS_OUT(what) = 0;
                        }

                        else
                        {
                            INTERF_PARA_DATA_BUS_OUT(what) = pc_fgetc(interf_para_src_fp);
                        }

                        INTERF_PARA_STROBE(what);

                        interf_para_cycle_cnt = 0;
                        interf_para_state     = 2;
                    }

                    break;
                }

                case 2:
                {
                    interf_para_cycle_cnt += num_cycles;

                    if ( interf_para_cycle_cnt >= interf_para_strobe_time )
                    {
                        INTERF_PARA_STRB_BUS_OUT(what) = 1;

                        INTERF_PARA_STROBE(what);

                        interf_para_cycle_cnt = 0;
                        interf_para_state     = 0;
                    }

                    break;
                }

                default:
                {
                    break;
                }
            }

            break;
        }

        default:
        {
            break;
        }
    }

    /*
       Tape cycling
    */

    {
        interf_tape_out_elapsed_zclk = ( interf_tape_out_elapsed_zclk + num_cycles ) & INTERF_TAPE_CLK_MASK;
        interf_tape_in_elapsed_zclk  = ( interf_tape_in_elapsed_zclk  + num_cycles ) & INTERF_TAPE_CLK_MASK;

        /*
           Output cycle - If timeout then reset state
        */

        if ( interf_tape_out_elapsed_zclk > INTERF_TAPE_TIMEOUT_POINT )
        {
            interf_tape_out_elapsed_zclk = INTERF_TAPE_TIMEOUT_PUSH;

            interf_tape_out_reset_state_after_block();

            if ( interf_tape_autosave_mode )
            {
                interf_tape_out_reset_state_after_file();
            }
        }

        /*
           Input cycle
        */

        if ( interf_tape_in_type && ( interf_tape_in_elapsed_zclk >= interf_tape_lower[1] ) )
        {
            if ( interf_tape_in_kansascycle == 0 )
            {
                interf_tape_in_elapsed_zclk = 0;

                /*
                   0 is used to signify that the process has just started (this
                   is needed to make cycles HL, not LH).  Thus we need to set
                   the bit to 1 to get things started.
                */

                INTERF_TAPE_INSTATE(what) = 1;
                INTERF_TAPE_STROBE(what);
                interf_tape_in_state_fine = 1;

                interf_tape_in_kansascycle = 1;

                /*
                   Get bit from the bit buffer.
                */

                INTERF_TAPE_IN_GETBITFROMBUFF(interf_tape_in_bit_fine);
            }

            else
            {
                if ( interf_tape_in_elapsed_zclk >= interf_tape_cycles[interf_tape_in_bit_fine][interf_tape_in_state_fine] )
                {
                    /*
                       OK, current cycle done.
                    */

                    interf_tape_in_elapsed_zclk -= interf_tape_cycles[interf_tape_in_bit_fine][interf_tape_in_state_fine];

                    /*
                       Invert the state, and update the cycle counter if this
                       was a low->high transition.
                    */

                    if ( interf_tape_in_state_fine )
                    {
                        INTERF_TAPE_INSTATE(what) = 0;
                        INTERF_TAPE_STROBE(what);
                        interf_tape_in_state_fine = 0;
                    }

                    else
                    {
                        INTERF_TAPE_INSTATE(what) = 1;
                        INTERF_TAPE_STROBE(what);
                        interf_tape_in_state_fine = 1;

                        interf_tape_in_kansascycle++;
                    }

                    if ( interf_tape_in_kansascycle > interf_tape_count[interf_tape_in_tapesped][interf_tape_in_bit_fine] )
                    {
                        /*
                           Current bit is done, get the next one.
                        */

                        INTERF_TAPE_IN_GETBITFROMBUFF(interf_tape_in_bit_fine);

                        /*
                           Reset the counter (1 start deliberate)
                        */

                        interf_tape_in_kansascycle = 1;

                        /*
                           Change to 1200baud if trigger hit
                        */

                        if ( interf_tape_in_pos_bitcnt == interf_tape_in_spedchgbitcht )
                        {
                            interf_tape_in_tapesped = 1;
                        }
                    }
                }
            }

            if ( interf_tape_in_pos_bitcnt >= interf_tape_in_bufsize )
            {
                /*
                   This will actually cut the final stop bit off the final
                   transmitted byte.  That's why the buffer is always padded
                   with a NULL at the end, so only the (unimportant) NULL
                   character is affected.
                */

                INTERF_TAPE_INSTATE(what) = 0;
                INTERF_TAPE_STROBE(what);
                interf_tape_in_state_fine = 0;

                interf_tape_in_reset_state();
            }
        }
    }

    /*
       Screen cycling
    */

    {
        if ( interf_interact_flag || interf_scrn_stepmode )
        {
            /*
               Enter interactive mode.
            */

            interf_is_in_menu_mode = 1;
            INTERF_CTRL_PAUSE_EMU(what);
            #ifdef IS_ALLEGRO
            {
                interf_menu_enter();
            }
            #endif
            INTERF_CTRL_RESTART_EMU(what);
            interf_is_in_menu_mode = 0;
        }

        interf_interact_flag = 0;
    }

    #ifdef IS_WEB
    {
        /*
           Under allegro, the menu is only available when the emulation is
           paused.  See interf_menu_enter() for more info.  Not sure if this
           is optimal for the web based version.  See call for
           interf_menu_enter above.
        */
    }
    #endif

    if ( interf_prtscr_flag )
    {
        /*
           Save screen snapshot.
        */

        interf_scrn_screenshot(DEFAULT_SCREENDUMP_FILE);

        interf_prtscr_flag = 0;
    }

    return;

    clock_div = 0;
}

char *interf_getinf(module_data *what)
{
    char *dest;

    dest = DEBMALLOC(10*sizeof(UINT_8));

    sprintf(dest,"\n");

    return dest;

    what = NULL;
}

int interf_para_set_mode0(void)
{
    interf_para_set_mode4();

    #ifdef PARA_ACCESS_HARD
    {
        interf_para_mode  = 0;
        interf_para_state = 0;

        outportb(interf_para_lptport+2,0x008);
        outportb(interf_para_lptport+2,0x00C);

        outportb(interf_para_lptport,0);

        interf_para_havepulsed  = 0;
        interf_para_havestrobed = 0;
    }
    #endif

    return 0;
}

int interf_para_set_mode1(const char *filename)
{
    interf_para_set_mode4();

    interf_para_mode  = 1;
    interf_para_state = INTERF_PARA_RDY_BUS_OUT(interf_indir_nonvol);

    if ( interf_para_dest_fp != NULL )
    {
        pc_fclose(interf_para_dest_fp);
    }

    if ( ( interf_para_dest_fp = pc_fopen(filename,"wb") ) == NULL )
    {
        interf_para_set_mode4();

        return 1;
    }

    return 0;
}

int interf_para_set_mode2(void)
{
    interf_para_set_mode4();

    interf_para_mode  = 2;
    interf_para_state = INTERF_PARA_RDY_BUS_OUT(interf_indir_nonvol);

    return 0;
}

int interf_para_set_mode3(const char *filename)
{
    interf_para_set_mode4();

    if ( INTERF_PARA_RDY_BUS_IN(interf_indir_nonvol) )
    {
        interf_para_mode  = 3;
        interf_para_state = 0;
    }

    else
    {
        interf_para_mode  = 3;
        interf_para_state = 1;
    }

    if ( interf_para_src_fp != NULL )
    {
        pc_fclose(interf_para_src_fp);
    }

    if ( ( interf_para_src_fp = pc_fopen(filename,"rb") ) == NULL )
    {
        interf_para_set_mode4();

        return 1;
    }

    return 0;
}

int interf_para_set_mode4(void)
{
    switch ( interf_para_mode )
    {
        case 1:
        case 2:
        case 3:
        case 5:
        {
            switch ( interf_para_state )
            {
                case 1:
                {
                    interf_para_state     = 0;
                    interf_para_cycle_cnt = 0;

                    break;
                }

                case 2:
                {
                    interf_para_state     = 0;
                    interf_para_cycle_cnt = 0;

                    INTERF_PARA_STRB_BUS_OUT(interf_indir_nonvol) = 1;
                    INTERF_PARA_STROBE(interf_indir_nonvol);

                    break;
                }

                default:
                {
                    break;
                }
            }

            break;
        }

        default:
        {
            break;
        }
    }

    interf_para_mode  = 4;
    interf_para_state = 0;

    interf_para_cycle_cnt  = 0;
    interf_para_upstat_cnt = 0;

    if ( interf_para_dest_fp != NULL )
    {
        pc_fclose(interf_para_dest_fp);
    }

    if ( interf_para_src_fp != NULL )
    {
        pc_fclose(interf_para_src_fp);
    }

    interf_para_dest_fp = NULL;
    interf_para_src_fp  = NULL;

    return 0;
}

int interf_para_set_mode5(void)
{
    interf_para_set_mode4();

    #ifdef PARA_ACCESS_BIOS
    {
        interf_para_mode  = 5;
        interf_para_state = INTERF_PARA_RDY_BUS_OUT(interf_indir_nonvol);

        _bios_printer(_PRINTER_INIT,interf_para_lptnum-1,0);
    }
    #endif

    return 0;
}

void interf_para_data_written(void *what)
{
    switch ( interf_para_mode )
    {
        case 0:
        {
            /* Output to PC parallel port */

            #ifdef PARA_ACCESS_HARD
            {
                outportb(interf_para_lptport,INTERF_PARA_DATA_BUS_IN(what));

                if ( interf_para_sim_pulse )
                {
                    if ( INTERF_PARA_RDY_BUS_IN(what) )
                    {
                        if ( !interf_para_havepulsed )
                        {
                            outportb(interf_para_lptport+2,0x00D);

                            interf_para_havepulsed = 1;

                            interf_para_trigpulsecnt = interf_para_pulse_len;
                        }

                        else
                        {
                            outportb(interf_para_lptport+2,0x00C);
                        }
                    }

                    else
                    {
                        outportb(interf_para_lptport+2,0x00C);

                        interf_para_havepulsed = 0;
                    }
                }

                else
                {
                    if ( INTERF_PARA_RDY_BUS_IN(what) )
                    {
                        outportb(interf_para_lptport+2,0x00D);
                    }

                    else
                    {
                        outportb(interf_para_lptport+2,0x00C);
                    }
                }
            }
            #endif

            break;
        }

        case 1:
        {
            /* output to file */

            if ( ( interf_para_state == 0 ) && ( INTERF_PARA_RDY_BUS_IN(what) == 1 ) )
            {
                if ( pc_fputc(INTERF_PARA_DATA_BUS_IN(what),interf_para_dest_fp) == INTERF_PARA_DATA_BUS_IN(what) )
                {
                    interf_para_cycle_cnt = 0;
                    interf_para_state     = 1;
                }

                else
                {
                    interf_para_set_mode4();

                    return;
                }
            }

            break;
        }

        case 2:
        {
            /* NULL output with handshaking */

            if ( ( interf_para_state == 0 ) && ( INTERF_PARA_RDY_BUS_IN(what) == 1 ) )
            {
                interf_para_cycle_cnt = 0;
                interf_para_state     = 1;
            }

            break;
        }

        case 3:
        {
            /* input from file */

            if ( ( interf_para_state == 0 ) && ( INTERF_PARA_RDY_BUS_IN(what) == 0 ) )
            {
                if ( !pc_feof(interf_para_src_fp) )
                {
                    interf_para_cycle_cnt = 0;
                    interf_para_state     = 1;
                }

                else
                {
                    interf_para_set_mode4();
                }
            }

            break;
        }

        case 5:
        {
            /* output to bios */

            #ifdef PARA_ACCESS_BIOS
            {
                if ( ( interf_para_state == 0 ) && ( INTERF_PARA_RDY_BUS_IN(what) == 1 ) )
                {
                    _bios_printer(_PRINTER_WRITE,interf_para_lptnum-1,INTERF_PARA_DATA_BUS_IN(what));

                    interf_para_cycle_cnt = 0;
                    interf_para_state     = 1;
                }
            }
            #endif

            break;
        }

        default:
        {
            /* unconnected */

            break;
        }
    }

    return;
}

void interf_key_setfile(const char *interf_key_sourcefilename)
{
    UINT_16 i;

    interf_key_closefile();

    interf_key_keyfileupdn = 0;

    for ( i = 0x00000 ; i <= 0x003F0 ; i += 0x00010 )
    {
        DEBDEREF(INTERF_KEY_LPEN_TABLE(interf_indir_nonvol),i) = 0;
    }

    interf_key_sourcefp = pc_fopen(interf_key_sourcefilename,"rb");

    return;
}

void interf_key_closefile(void)
{
    UINT_16 i;

    if ( interf_key_sourcefp != NULL )
    {
        pc_fclose(interf_key_sourcefp);

        interf_key_sourcefp = NULL;
    }

    interf_key_keydown     = 0;
    interf_key_keyfileupdn = 0;

    for ( i = 0x00000 ; i <= 0x003F0 ; i += 0x00010 )
    {
        DEBDEREF(INTERF_KEY_LPEN_TABLE(interf_indir_nonvol),i) = 0;
        DEBDEREF(interf_key_worktable,i)                       = 0;
        DEBDEREF(interf_key_worktable,i+1)                     = 0;
        DEBDEREF(interf_key_worktable,i+2)                     = 0;
    }

    return;
}


void interf_speaker_state_change(void *what)
{
    UINT_32 freq_temp;

    if ( interf_snd_sndon )
    {
        /*
           We look at the sound from the +ve going edge, not the -ve going
           edge.  Also, need to make sure interf_snd_clkcycle_cnt_a
           isn't zero, as this will cause divide by zero problems.
        */

        interf_snd_speaker_state_old = interf_snd_speaker_state;
        interf_snd_speaker_state     = INTERF_SND_BITSTATE(what);

        if ( interf_snd_clkcycle_cnt_a && interf_snd_speaker_state && !interf_snd_speaker_state_old )
        {
            /*
               Calculate the cycle frequency and reset the counter.
            */

            freq_temp = 1000000000/(interf_snd_clkcycle_cnt_a*interf_snd_sndclk_period);

            if ( freq_temp < interf_snd_freq )
            {
                /*
                   Tone has decreased, or a click has occured.  In any case,
                   centralise the frequency and set the click indicator.
                */

                interf_snd_click_next = 1;

                interf_snd_clkcycle_cnt_a = interf_snd_clkcycle_cnt_b;
            }

            else if ( ( interf_snd_clkcycle_cnt_a > ((interf_snd_clkcycle_cnt_b*(interf_snd_clkcnterr_denom+interf_snd_clkcnterr_numer))/interf_snd_clkcnterr_denom) ) ||
                      ( interf_snd_clkcycle_cnt_a < ((interf_snd_clkcycle_cnt_b*(interf_snd_clkcnterr_denom-interf_snd_clkcnterr_numer))/interf_snd_clkcnterr_denom) )    )
            {
                /*
                   Tone has gone out of range, so insert a click.
                */

                interf_snd_click_here = 1;
            }

            interf_snd_freq = freq_temp;

            /*
               Reset counters.
            */

            interf_snd_clkcycle_cnt_b = interf_snd_clkcycle_cnt_a;
            interf_snd_clkcycle_cnt_a = 0;

            /*
               Set the flag to indicate that there's a tone in this interval.
            */

            interf_snd_tone_here = 1;
        }
    }

    return;
}

void interf_snd_nosound(void)
{
    #ifdef SOUND_PCSPEAKER
    nosound();
    #endif

    #ifdef SOUND_SOUNDCARD
    if ( !interf_snd_cardbad )
    {
        if ( interf_snd_sound_on_a )
        {
            stop_sample(interf_snd_sample_a);

            interf_snd_sound_on_a = 0;
        }

        if ( interf_snd_sound_on_b )
        {
            stop_sample(interf_snd_sample_b);

            interf_snd_sound_on_b = 0;
        }

        if ( interf_snd_sound_on_c )
        {
            stop_sample(interf_snd_sample_c);

            interf_snd_sound_on_c = 0;
        }
    }
    #endif

    return;
}

void interf_snd_sound(int freq)
{
    #ifdef SOUND_PCSPEAKER
    sound(freq);
    #endif

    #ifdef SOUND_SOUNDCARD
    if ( !interf_snd_cardbad )
    {
        interf_snd_frq_a = (1000*STRETCH*freq)/BASE_FREQ_A;
        interf_snd_frq_b = (1000*STRETCH*freq)/BASE_FREQ_B;
        interf_snd_frq_c = (1000*STRETCH*freq)/BASE_FREQ_C;

        if ( !interf_snd_sound_on_a && !interf_snd_sound_on_b && !interf_snd_sound_on_c )
        {
            freq_start:

            if ( interf_snd_frq_a <= (1000*STRETCH)/SHRINK )
            {
                play_sample(interf_snd_sample_a,255,128,interf_snd_frq_a,1);

                interf_snd_sound_on_a = 1;
            }

            else if ( interf_snd_frq_b <= (1000*STRETCH)/SHRINK )
            {
                play_sample(interf_snd_sample_b,255,128,interf_snd_frq_b,1);

                interf_snd_sound_on_b = 1;
            }

            else
            {
                play_sample(interf_snd_sample_c,255,128,interf_snd_frq_c,1);
                      
                interf_snd_sound_on_c = 1;
            }
        }

        else
        {
            if ( interf_snd_sound_on_a )
            {
                if (  ( interf_snd_frq_a > MAXSTRETCH*1000 ) || ( interf_snd_frq_a < MINFREQ ) )
                {
                    stop_sample(interf_snd_sample_a);

                    goto freq_start;
                }

                adjust_sample(interf_snd_sample_a,255,128,interf_snd_frq_a,1);
            }

            else if ( interf_snd_sound_on_b )
            {
                if ( ( interf_snd_frq_b > MAXSTRETCH*1000 ) || ( interf_snd_frq_b < MINFREQ ) )
                {
                    stop_sample(interf_snd_sample_b);

                    goto freq_start;
                }

                adjust_sample(interf_snd_sample_b,255,128,interf_snd_frq_b,1);
            }

            else
            {
                if ( ( interf_snd_frq_c > MAXSTRETCH*1000 ) || ( interf_snd_frq_c < MINFREQ ) )
                {
                    stop_sample(interf_snd_sample_c);

                    goto freq_start;
                }

                adjust_sample(interf_snd_sample_c,255,128,interf_snd_frq_c,1);
            }           
        }
    }
    #endif

    return;

    freq = 0;
}

void interf_snd_soundclick(void)
{
    #ifdef SOUND_PCSPEAKER
    interf_snd_sound(1);
    #endif

    #ifdef SOUND_SOUNDCARD
    play_sample(interf_click_sample,255,128,1000,0);
    #endif

    return;
}

void interf_snd_toggle_snd(void)
{
    if ( interf_snd_sndon )
    {
        interf_snd_turn_snd_off();
    }

    else
    {
        interf_snd_turn_snd_on();
    }

    return;
}

void interf_snd_turn_snd_on(void)
{
    interf_snd_sndon = 1;

    return;
}

void interf_snd_turn_snd_off(void)
{
    interf_snd_sndon = 0;

    interf_snd_nosound();

    return;
}

void interf_tape_out_reset_state_after_block(void)
{
    interf_tape_out_byte         = 0;
    interf_tape_out_baud         = 0;
    interf_tape_out_type         = 0;
    interf_tape_out_state_fine   = INTERF_TAPE_OUTSTATE(interf_indir_nonvol);
    interf_tape_out_state_course = 0;
    interf_tape_out_tapefreq     = 0;
    interf_tape_out_crc          = 0;
    interf_tape_out_kansascycle  = 0;
    interf_tape_out_elapsed_zclk = 0;
    interf_tape_out_hl_state     = 0;
    interf_tape_out_hl_length    = 0;

    return;
}

void interf_tape_out_reset_state_after_file(void)
{
    interf_tape_out_reset_state_after_block();

    interf_tape_autosave_mode = 1;

    interf_tape_out_hl_overall   = 0;
    interf_tape_out_hl_endit     = 0;

    interf_tape_out_filename[0] = '\0';
    interf_tape_out_filename[1] = '\0';
    interf_tape_out_filename[2] = '\0';
    interf_tape_out_filename[3] = '\0';
    interf_tape_out_filename[4] = '\0';
    interf_tape_out_filename[5] = '\0';
    interf_tape_out_filename[6] = '\0';

    interf_tape_out_filetype = 0;
    interf_tape_out_filesize = 0;
    interf_tape_out_fileload = 0;
    interf_tape_out_fileexec = 0;
    interf_tape_out_filesped = 0;
    interf_tape_out_fileauto = 0;

    interf_tape_out_close_autosave_file();

    return;
}

void interf_tape_out_reset_state_after_stream(void)
{
    interf_tape_out_reset_state_after_block();

    interf_tape_out_close_output_stream();

    interf_tape_out_set_mode_3();

    return;
}

void interf_tape_out_reset_state(void)
{
    interf_tape_out_reset_state_after_file();
    interf_tape_out_reset_state_after_stream();

    return;
}

void interf_tape_out_send_byte(void)
{
    FILE *interf_tape_out_mtd_fp;

    if ( !interf_tape_out_type )
    {
        /*
           This is a byte in a data stream
        */

        switch ( interf_tape_out_hl_state )
        {
            case 0:
            {
                /*
                   Wait for ff to indicate the start of the header.
                */

                if ( interf_tape_out_byte == 0x0ff )
                {
                    interf_tape_out_hl_state = 1;
                }

                break;
            }

            case 1:
            {
                /*
                   This should be followed by a 2a to indicate the start of
                   proceedings.
                */

                if ( interf_tape_out_byte == 0x02a )
                {
                    interf_tape_out_hl_state = 2;
                }

                else if ( interf_tape_out_byte != 0x0ff )
                {
                    interf_tape_out_reset_state_after_block();
                }

                break;
            }

            case 2:
            {
                /*
                   Next comes the length byte.  If this is 0 then the block
                   will be 256 bytes long.  Otherwise, the block will be 1
                   longer than indicated here.
                */

                interf_tape_out_hl_state = 3;

                interf_tape_out_hl_length = interf_tape_out_byte;

                if ( interf_tape_out_hl_length == 0 )
                {
                    interf_tape_out_hl_length = 0x0ff;
                }

                else
                {
                    interf_tape_out_hl_length--;
                }

                interf_tape_out_crc = 0x000;

                break;
            }

            case 3:
            {
                /*
                   This is a byte in the data block.
                */

                interf_tape_out_crc ^= interf_tape_out_byte;

                if ( interf_tape_out_crc & 0x001 )
                {
                    interf_tape_out_crc = ( ( interf_tape_out_crc >> 0x001 ) & 0x07f ) | 0x080;
                }

                else
                {
                    interf_tape_out_crc =   ( interf_tape_out_crc >> 0x001 ) & 0x07f;
                }

                if ( interf_tape_out_hl_length == 0 )
                {
                    interf_tape_out_hl_state = 4;
                }

                else
                {
                    interf_tape_out_hl_length--;
                }

                switch ( interf_tape_out_mode )
                {
                    case 1:
                    {
                        if ( pc_fputc(interf_tape_out_byte,interf_tape_out_dest_fp) != interf_tape_out_byte )
                        {
                            interf_tape_out_reset_state_after_stream();
                        }

                        break;
                    }

                    case 2:
                    case 3:
                    {
                        break;
                    }

                    default:
                    {
                        /*
                           State unknown, something badly wrong.
                        */

                        interf_tape_out_reset_state();

                        break;
                    }
                }

                break;
            }

            case 4:
            {
                interf_tape_out_hl_state = 0;

                break;
            }

            default:
            {
                /*
                   State unknown, something badly wrong.
                */

                interf_tape_out_reset_state();

                break;
            }
        }
    }

    else
    {
        /*
           This is a byte in a file being saved
        */

        switch ( interf_tape_out_hl_state )
        {
            case 0:
            {
                /*
                   Waiting for a 00 to indicate beginning of file.
                */

                if ( interf_tape_out_byte == 0x000 )
                {
                    interf_tape_out_hl_state = 1;
                }

                break;
            }

            case 1:
            {
                /*
                   Waiting for SOH (start of header) byte.
                */

                if ( interf_tape_out_byte == 0x001 )
                {
                    interf_tape_out_hl_state = 2;

                    interf_tape_out_crc = 0x000;
                }

                else if ( interf_tape_out_byte != 0x000 )
                {
                    /*
                       There's some non-zero values in the pre-header, which
                       is not allowed.  Let's assume this was just a false
                       start, so don't do anything too drastic.  Note that
                       the file will not have started at this point in any
                       case, so no need to worry about autosave stuff just
                       yet.
                    */

                    interf_tape_out_reset_state_after_block();
                }

                break;
            }

            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Record the filename (6 bytes).  While we're at it, it
                   is important to make sure the filename is "OK" for
                   DOS/Windows.  Default overwrite character for unusable
                   chars is "_".

                   FIXME: - IF this is ever ported to linux, this may need
                            to be changed to allow for differences in the
                            linux file name conventions.
                          - Technically, spaces seem to be allowed in the
                            filenames for the microbee tape format (but not
                            the disk format).  However, these are more
                            trouble than they are worth, esp. as dos is a
                            target system for this emulator.  Hence they
                            are dis-allowed here.  This shouldn't matter, as
                            afaik no-one used them on the bee back in the
                            day.
                */

                if ( ( interf_tape_out_byte >= ' '  ) &&
                     ( interf_tape_out_byte <= '~'  ) &&
                     ( interf_tape_out_byte != ' '  ) &&
                     ( interf_tape_out_byte != '\\' ) &&
                     ( interf_tape_out_byte != '/'  ) &&
                     ( interf_tape_out_byte != ':'  ) &&
                     ( interf_tape_out_byte != '*'  ) &&
                     ( interf_tape_out_byte != '\?' ) &&
                     ( interf_tape_out_byte != '\"' ) &&
                     ( interf_tape_out_byte != '<'  ) &&
                     ( interf_tape_out_byte != '>'  ) &&
                     ( interf_tape_out_byte != '|'  )    )
                {
                    interf_tape_out_filename[interf_tape_out_hl_state-2] = interf_tape_out_byte;
                }

                else if ( interf_tape_out_byte != 0x000 )
                {
                    interf_tape_out_filename[interf_tape_out_hl_state-2] = '_';
                }

                else
                {
                    interf_tape_out_filename[interf_tape_out_hl_state-2] = '\0';
                }

                interf_tape_out_filename[6] = '\0';

                interf_tape_out_hl_state++;

                break;
            }

            case 8:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get filetype character
                */

                interf_tape_out_filetype = interf_tape_out_byte;

                interf_tape_out_hl_state++;

                break;
            }

            case 9:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get LSB of filesize
                */

                interf_tape_out_filesize = SETLOWER8OF16(interf_tape_out_filesize,interf_tape_out_byte);

                interf_tape_out_hl_state++;

                break;
            }

            case 10:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get MSB of filesize
                */

                interf_tape_out_filesize = SETUPPER8OF16(interf_tape_out_filesize,interf_tape_out_byte);

                interf_tape_out_hl_state++;

                break;
            }

            case 11:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get LSB of load address
                */

                interf_tape_out_fileload = SETLOWER8OF16(interf_tape_out_fileload,interf_tape_out_byte);

                interf_tape_out_hl_state++;

                break;
            }

            case 12:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get MSB of load address
                */

                interf_tape_out_fileload = SETUPPER8OF16(interf_tape_out_fileload,interf_tape_out_byte);

                interf_tape_out_hl_state++;

                break;
            }

            case 13:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get LSB of autoexec address
                */

                interf_tape_out_fileexec = SETLOWER8OF16(interf_tape_out_fileexec,interf_tape_out_byte);

                interf_tape_out_hl_state++;

                break;
            }

            case 14:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get MSB of autoexec address
                */

                interf_tape_out_fileexec = SETUPPER8OF16(interf_tape_out_fileexec,interf_tape_out_byte);

                interf_tape_out_hl_state++;

                break;
            }

            case 15:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get filespeed byte.  00 indicates that the data is stored
                   at 300baud, while ff means 1200baud.  For now, I don't
                   bother to check this, which may be a mistake...
                */

                interf_tape_out_filesped = interf_tape_out_byte;

                interf_tape_out_hl_state++;

                break;
            }

            case 16:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   Get autoexec byte.  00 is normal (no autoexec), whilst
                   ff means "jump to autoexec address after loading".
                   Again, we don't check this.
                */

                interf_tape_out_fileauto = interf_tape_out_byte;

                interf_tape_out_hl_state++;

                break;
            }

            case 17:
            {
                interf_tape_out_crc = MASKEDADD8(interf_tape_out_crc,interf_tape_out_byte);

                /*
                   This is a dummy byte, which is ignored.
                */

                interf_tape_out_hl_state++;

                break;
            }

            case 18:
            {
                /*
                   The checksum for the header is here.  See previously
                   for details.  At present, we don't check to see if this
                   corresponds to the crc generated by the bee... this may
                   be a mistake, but the rationale is that it is better to
                   be generous in the input you accept, and strict in what
                   you produce.  On the other hand, a bad crc here may
                   indicate a problem (well, ok, it does, but anyhow), so
                   stopping might be a better thing to do.
                */

                interf_tape_out_crc = TAKETWOS(interf_tape_out_crc);
                interf_tape_out_crc = ( ( ( interf_tape_out_crc & 0x0f0 ) - 0x010 ) & 0x0f0 ) | ( interf_tape_out_crc & 0x00f );

                /*
                   We must set the speed to either 300 or 1200 baud
                   depending on the speed byte, as this is the speed at
                   which any data following this has been recorded.
                */

                if ( interf_tape_out_filesped ) 
                {
                    interf_tape_out_baud = 1;
                }

                else
                {
                    interf_tape_out_baud = 0;
                }

                interf_tape_out_hl_state++;

                if ( interf_tape_autosave_mode )
                {
                    /*
                       Construct the filename.  This consists of the
                       interf_tape_out_filename string followed by .M, where
                       M is the interf_tape_out_filetype character.

                       FIXME: for some reason, the .B file produced is
                              not a true .mwb file - need to write a
                              converter program.
                    */

                    strcpy(interf_tape_out_autosave_name,interf_tape_out_filename);

                    if ( (                                  ( interf_tape_out_filetype < 0x030 ) ) ||
                         ( ( interf_tape_out_filetype > 0x039 ) && ( interf_tape_out_filetype < 0x041 ) ) ||
                         ( ( interf_tape_out_filetype > 0x05a ) && ( interf_tape_out_filetype < 0x061 ) ) ||
                         ( ( interf_tape_out_filetype > 0x07a )                                  )    )
                    {
                        interf_tape_out_filetype = '_';
                    }

                    interf_tape_out_autosave_name[strlen(interf_tape_out_autosave_name)+2] = '\0';
                    interf_tape_out_autosave_name[strlen(interf_tape_out_autosave_name)+1] = interf_tape_out_filetype;
                    interf_tape_out_autosave_name[strlen(interf_tape_out_autosave_name)]   = '.';

                    /*
                       Save header data in filename interf_tape_out_autosave_name
                       cat with .mtd (microbee tape data).
                    */

                    strcpy(interf_tape_out_file_tapedest,interf_tape_out_autosave_dir);
                    strcat(interf_tape_out_file_tapedest,interf_tape_out_filename);
                    strcat(interf_tape_out_file_tapedest,".mtd");

                    if ( NULL != ( interf_tape_out_mtd_fp = fopen(interf_tape_out_file_tapedest,"wt") ) )
                    {
                        fprintf(interf_tape_out_mtd_fp,"Savename    = %s\n",interf_tape_out_autosave_name);
                        fprintf(interf_tape_out_mtd_fp,"Filename    = %s\n",interf_tape_out_filename);
                        fprintf(interf_tape_out_mtd_fp,"Filetype    = %c\n",(char) interf_tape_out_filetype);
                        fprintf(interf_tape_out_mtd_fp,"Filesize    = %04x\n",(int) interf_tape_out_filesize);
                        fprintf(interf_tape_out_mtd_fp,"Load_addr   = %04x\n",(int) interf_tape_out_fileload);
                        fprintf(interf_tape_out_mtd_fp,"Exec_addr   = %04x\n",(int) interf_tape_out_fileexec);
                        fprintf(interf_tape_out_mtd_fp,"File_speed  = %02x\n",(int) interf_tape_out_fileauto);
                        fprintf(interf_tape_out_mtd_fp,"Exec_mode   = %02x\n",(int) interf_tape_out_fileauto);
                        fprintf(interf_tape_out_mtd_fp,"Checksum    = %02x\n",(int) interf_tape_out_byte);
                        fprintf(interf_tape_out_mtd_fp,"LocChecksum = %02x\n",(int) interf_tape_out_crc);

                        fclose(interf_tape_out_mtd_fp);
                    }

                    /*
                       Open file as stream.
                    */

                    strcpy(interf_tape_out_file_tapedest,interf_tape_out_autosave_dir);
                    strcat(interf_tape_out_file_tapedest,interf_tape_out_autosave_name);

                    if ( NULL == ( interf_tape_out_autosave_fp = pc_fopen(interf_tape_out_file_tapedest,"wb") ) )
                    {
                        interf_tape_out_reset_state_after_file();
                    }
                }

                if ( interf_tape_out_filesize == 0 )
                {
                    if ( interf_tape_autosave_mode )
                    {
                        interf_tape_out_reset_state_after_file();
                    }
                }

                else
                {
                    interf_tape_out_crc = 0;

                    while ( interf_tape_out_filesize > 0x0ff )
                    {
                        interf_tape_out_filesize -= 0x0100;
                        interf_tape_out_hl_overall++;
                    }

                    interf_tape_out_hl_endit = interf_tape_out_filesize;

                    if ( interf_tape_out_hl_overall )
                    {
                        interf_tape_out_hl_length = 0x0ff;
                    }

                    else
                    {
                        interf_tape_out_hl_length = interf_tape_out_hl_endit-1;
                    }
                }

                break;
            }

            case 19:
            {
                /*
                   In the midst of a data block.  In interf_tape_out_hl_length is
                   zero then this is the last byte in the block, so need to
                   set the state appropriately.
                */

                interf_tape_out_crc = MASKEDADD8(interf_tape_out_byte,TAKETWOS(interf_tape_out_crc)) ^ 0x0ff;

                if ( interf_tape_out_hl_length == 0 )
                {
                    interf_tape_out_hl_state = 20;
                }

                else
                {
                    interf_tape_out_hl_length--;
                }

                if ( interf_tape_autosave_mode )
                {
                    if ( pc_fputc(interf_tape_out_byte,interf_tape_out_autosave_fp) != interf_tape_out_byte )
                    {
                        interf_tape_out_reset_state_after_file();
                    }
                }

                else
                {
                    switch ( interf_tape_out_mode )
                    {
                        case 1:
                        {
                            if ( pc_fputc(interf_tape_out_byte,interf_tape_out_dest_fp) != interf_tape_out_byte )
                            {
                                interf_tape_out_reset_state_after_stream();
                            }

                            break;
                        }

                        case 2:
                        case 3:
                        {
                            break;
                        }

                        default:
                        {
                            /*
                               State unknown, something badly wrong.
                            */

                            interf_tape_out_reset_state();

                            break;
                        }
                    }
                }

                break;
            }

            case 20:
            {
                /*
                   This is the checksum/crc byte for the most recent block.
                */

                if ( interf_tape_out_hl_overall )
                {
                    interf_tape_out_hl_overall--;

                    interf_tape_out_crc = 0;

                    if ( interf_tape_out_hl_overall )
                    {
                        interf_tape_out_hl_length = 0x0ff;
                    }

                    else
                    {
                        interf_tape_out_hl_length = interf_tape_out_hl_endit-1;
                    }

                    interf_tape_out_hl_state = 19;
                }

                else
                {
                    interf_tape_out_reset_state_after_block();

                    if ( interf_tape_autosave_mode )
                    {
                        interf_tape_out_reset_state_after_file();
                    }
                }

                break;
            }

            default:
            {
                /*
                   State unknown, something badly wrong.
                */

                interf_tape_out_reset_state();

                break;
            }
        }
    }

    return;
}

void interf_tape_state_change(void *what)
{
    if ( interf_tape_out_state_fine != INTERF_TAPE_OUTSTATE(what) )
    {
        /*
           The state of the output bit has changed, so action is required.
        */

        interf_tape_out_state_fine = INTERF_TAPE_OUTSTATE(what);

        switch ( interf_tape_out_state_course )
        {
            case 0:
            {
                /*
                   Need to detect type and baud rate.  All signals should
                   start with a sequence of either 00's or FF's.  For a
                   data-stream, frequencies will be:

                   1200Hz boundary bit
                   2400Hz data bit    +
                   2400Hz data bit    |
                   2400Hz data bit    |
                   2400Hz data bit    | Data FF
                   2400Hz data bit    |
                   2400Hz data bit    |
                   2400Hz data bit    |
                   2400Hz data bit    +
                   2400Hz stop bit
                   2400Hz stop bit

                   Similarly, for a save operation:

                   1200Hz boundary bit
                   1200Hz data bit    +
                   1200Hz data bit    |
                   1200Hz data bit    |
                   1200Hz data bit    | Data 00
                   1200Hz data bit    |
                   1200Hz data bit    |
                   1200Hz data bit    |
                   1200Hz data bit    +
                   2400Hz stop bit
                   2400Hz stop bit

                   The point is, the number of cycles at 1200Hz at the
                   start of the stream will tell us everything we need to
                   know about the speed and type of the data.  The counts
                   are as follows:

                   4  cycles at 1200Hz => 300  baud data stream
                   1  cycle  at 1200Hz => 1200 baud data stream
                   36 cycles at 1200Hz => save operation (speed unknown)

                   Detect stuff by checking the count on the first 
                   2400Hz cycle.
                */

                if ( interf_tape_out_state_fine )
                {
                    /*
                       Just had a low->high transition
                    */

                    if ( interf_tape_out_kansascycle )
                    {
                        if ( ( interf_tape_out_elapsed_zclk >= interf_tape_lower[0] ) &&
                             ( interf_tape_out_elapsed_zclk <= interf_tape_upper[0] )    )
                        {
                            /*
                               1200Hz cycle detected.

                               NB: The use of "200" in the following is
                                   essentially arbitrary.  Now, the signal
                                   will always start at 1200Hz, then go to
                                   2400Hz (either during data transmission
                                   or when the stop bit is reached), and
                                   then return to 1200Hz at some point.
                                   For the first byte, this will happen
                                   precisely once...
                                   So, interf_tape_out_kansascycle counts 1200Hz
                                   cycles starting from 0.  If the count
                                   exceeds 36 then the 1200Hz preamble has
                                   bee impossibly wrong.  Normally, though,
                                   a 2400Hz signal will intervene.  When it
                                   does, interf_tape_out_kansascycle is incremented
                                   by 200.  When the 2400Hz component finally
                                   ends, we can detect that this is a return
                                   to 1200Hz (and not the initial burst) by
                                   looking to see if interf_tape_out_kansascycle
                                   is greater than 200.
                            */

                            interf_tape_out_kansascycle++;

                            if ( interf_tape_out_kansascycle > 200 )
                            {
                                /*
                                   This is a return to 0, so is (presumably)
                                   the first stop bit of the next byte.  Set
                                   various stuff, and jump straight to the
                                   next state (sorry goto haters, but I can't
                                   be buggered making this "pretty").
                                */

                                if ( interf_tape_out_type )
                                {
                                    /*
                                       save type
                                    */

                                    interf_tape_out_state_course = 1;

                                    interf_tape_out_tapefreq    = 0;
                                    interf_tape_out_kansascycle = 0;

                                    goto state_1_jumpstart;
                                }

                                else
                                {
                                    /*
                                       stream type
                                    */

                                    interf_tape_out_state_course = 1;

                                    interf_tape_out_tapefreq    = 0;
                                    interf_tape_out_kansascycle = 0;

                                    goto state_1_jumpstart;
                                }
                            }

                            /*
                               Strictly, 36 should be used here.  However,
                               this screws up secondary save operations, so
                               some leeway is required.
                            */

                            else if ( interf_tape_out_kansascycle > 36 )
                            {
                                /*
                                   1200Hz preamble too long, something is
                                   severely borked.
                                */

                                interf_tape_out_reset_state();
                            }
                        }

                        else if ( ( interf_tape_out_elapsed_zclk >= interf_tape_lower[1] ) &&
                                  ( interf_tape_out_elapsed_zclk <= interf_tape_upper[1] )    )
                        {
                            /*
                               2400Hz cycle detected
                            */

                            interf_tape_out_kansascycle++;

                            if ( interf_tape_out_kansascycle < 200 )
                            {
                                /*
                                   This is the first 2400Hz cycle, and so the
                                   cycle count should tell us the type and
                                   also baud rate for a stream operation.
                                */

                                interf_tape_out_kansascycle += 200;

                                if ( interf_tape_out_kansascycle == (201+interf_tape_count[1][0]) )
                                {
                                    /*
                                       1200baud stream of data
                                    */

                                    interf_tape_out_baud = 1;
                                    interf_tape_out_type = 0;
                                }

                                else if ( interf_tape_out_kansascycle == (201+interf_tape_count[0][0]) )
                                {
                                    /*
                                       300baud stream of data
                                    */

                                    interf_tape_out_baud = 0;
                                    interf_tape_out_type = 0;
                                }

                                else if ( interf_tape_out_kansascycle == (201+(9*interf_tape_count[0][0])) )
                                {
                                    /*
                                       save (speed unknown), but always 
                                       begins with a 300baud header.
                                    */

                                    interf_tape_out_baud = 0;
                                    interf_tape_out_type = 1;
                                }

                                else
                                {
                                    /*
                                       Cycle count not recognised, things
                                       rather fucked up.
                                    */

                                    interf_tape_out_reset_state();
                                }
                            }
                        }

                        else
                        {
                            /*
                               The 1/2 cycle length is outside the acceptable
                               range - things are fucked up.
                            */

                            interf_tape_out_reset_state();
                        }
                    }

                    else
                    {
                        interf_tape_out_kansascycle = 1;
                    }
                }

                break;
            }

            case 1:
            {
                /*
                   This is the first boundary bit
                */

                if ( interf_tape_out_state_fine )
                {
                    state_1_jumpstart:

                    /*
                       Just had a low->high transition
                    */

                    /*
                       Check the frequency - it should be 1200Hz
                    */

                    if ( interf_tape_out_kansascycle && ( ( interf_tape_out_elapsed_zclk < interf_tape_lower[0] ) ||
                                                   ( interf_tape_out_elapsed_zclk > interf_tape_upper[0] )    ) )
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }

                    /*
                       The cycle check is here for the 1200baud case, where
                       we need to skip the first one (which will fail this
                       test) as it may lead to confusion.
                    */

                    else if ( ( interf_tape_out_elapsed_zclk >= interf_tape_lower[0] ) ||
                              ( interf_tape_out_elapsed_zclk <= interf_tape_upper[0] )    )
                    {
                        interf_tape_out_kansascycle++;

                        if ( interf_tape_out_kansascycle >= interf_tape_count[interf_tape_out_baud][0] )
                        {
                            /*
                               The boundary bit has now finished, so move
                               on to the first data bit.
                            */

                            interf_tape_out_state_course++;

                            interf_tape_out_byte         = 0;
                            interf_tape_out_tapefreq     = 0;
                            interf_tape_out_kansascycle  = 0;
                        }
                    }
                }

                else
                {
                    /*
                       Just had a high->low transition
                    */

                    /*
                       Check the frequency - it should be 1200Hz
                    */

                    if ( ( interf_tape_out_elapsed_zclk < interf_tape_lower[0] ) ||
                         ( interf_tape_out_elapsed_zclk > interf_tape_upper[0] )    )
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }
                }

                break;
            }

            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            {
                /*
                   This is a data bit
                */

                if ( interf_tape_out_state_fine )
                {
                    /*
                       Just had a low->high transition
                    */

                    /*
                       Check the frequency
                    */

                    if ( ( interf_tape_out_elapsed_zclk < interf_tape_lower[interf_tape_out_tapefreq] ) ||
                         ( interf_tape_out_elapsed_zclk > interf_tape_upper[interf_tape_out_tapefreq] )    )
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }

                    else
                    {
                        interf_tape_out_kansascycle++;

                        if ( interf_tape_out_kansascycle >= interf_tape_count[interf_tape_out_baud][interf_tape_out_tapefreq] )
                        {
                            /*
                               The bit has finished.

                               1. Record the bit value.
                               2. Increment the state.
                            */

                            if ( interf_tape_out_tapefreq )
                            {
                                /*
                                   2400Hz is 1, so set relevant bit
                                */

                                interf_tape_out_byte |= ( 1 << (interf_tape_out_state_course-2) );
                            }

                            interf_tape_out_state_course++;

                            interf_tape_out_tapefreq    = 0;
                            interf_tape_out_kansascycle = 0;
                        }
                    }
                }

                else if ( interf_tape_out_kansascycle )
                {
                    /*
                       Just had a high->low transition, but not the first.
                    */

                    if ( ( interf_tape_out_elapsed_zclk < interf_tape_lower[interf_tape_out_tapefreq] ) ||
                         ( interf_tape_out_elapsed_zclk > interf_tape_upper[interf_tape_out_tapefreq] )    )
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }
                }

                else
                {
                    /*
                       The first low cycle just finished.  From this we
                       should be able to calculate the frequency of the
                       signal.
                    */

                    if ( ( interf_tape_out_elapsed_zclk >= interf_tape_lower[0] ) &&
                         ( interf_tape_out_elapsed_zclk <= interf_tape_upper[0] )    )
                    {
                        interf_tape_out_tapefreq = 0;
                    }

                    else if ( ( interf_tape_out_elapsed_zclk >= interf_tape_lower[1] ) &&
                              ( interf_tape_out_elapsed_zclk <= interf_tape_upper[1] )    )
                    {
                        interf_tape_out_tapefreq = 1;
                    }

                    else
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }
                }

                break;
            }

            case 10:
            case 11:
            {
                /*
                   This is a stop bit
                */

                if ( interf_tape_out_state_fine )
                {
                    /*
                       Just had a low->high transition
                    */

                    /*
                       Check the frequency - it should be 2400Hz
                    */

                    if ( ( interf_tape_out_elapsed_zclk < interf_tape_lower[1] ) ||
                         ( interf_tape_out_elapsed_zclk > interf_tape_upper[1] )    )
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }

                    else
                    {
                        interf_tape_out_kansascycle++;

                        if ( interf_tape_out_kansascycle >= interf_tape_count[interf_tape_out_baud][1] )
                        {
                            /*
                               The stop bit has now finished, so increment
                               the state.
                            */

                            interf_tape_out_state_course++;

                            interf_tape_out_tapefreq     = 0;
                            interf_tape_out_kansascycle  = 0;

                            if ( interf_tape_out_state_course == 12 )
                            {
                                interf_tape_out_send_byte();

                                interf_tape_out_state_course = 1;
                            }
                        }
                    }
                }

                else
                {
                    /*
                       Just had a high->low transition
                    */

                    /*
                       Check the frequency - it should be 2400Hz
                    */

                    if ( ( interf_tape_out_elapsed_zclk < interf_tape_lower[1] ) ||
                         ( interf_tape_out_elapsed_zclk > interf_tape_upper[1] )    )
                    {
                        /*
                           The 1/2 cycle length is outside the acceptable
                           range - things are fucked up.
                        */

                        interf_tape_out_reset_state();
                    }
                }

                break;
            }

            default:
            {
                /*
                   State unknown, something badly wrong.
                */

                interf_tape_out_reset_state();

                break;
            }
        }

        interf_tape_out_elapsed_zclk = 0;
    }

    return;
}

int interf_tape_out_set_mode_1(char *filename)
{
    interf_tape_out_close_output_stream();

    interf_tape_out_dest_fp = pc_fopen(filename,"wb");

    if ( interf_tape_out_dest_fp == NULL )
    {
        interf_tape_out_set_mode_3();

        return 1;
    }

    interf_tape_out_mode = 1;

    return 0;
}

int interf_tape_out_set_mode_3(void)
{
    interf_tape_out_close_output_stream();

    interf_tape_out_mode = 3;

    return 0;
}

void interf_tape_out_close_output_stream(void)
{
    if ( interf_tape_out_dest_fp != NULL )
    {
        pc_fclose(interf_tape_out_dest_fp);
    }

    interf_tape_out_dest_fp = NULL;

    return;
}

void interf_tape_out_close_autosave_file(void)
{
    if ( interf_tape_out_autosave_fp != NULL )
    {
        pc_fclose(interf_tape_out_autosave_fp);
    }

    interf_tape_out_autosave_fp = NULL;

    return;
}




void interf_tape_in_reset_state(void)
{
    if ( interf_tape_in_buffer != NULL )
    {
        DEBFREE(interf_tape_in_buffer);
    }

    interf_tape_in_type          = 0;
    interf_tape_in_buffer        = NULL;
    interf_tape_in_pos_byte      = 0;
    interf_tape_in_pos_bit       = 0x000000001;
    interf_tape_in_pos_bitcnt    = 0;
    interf_tape_in_spedchgbitcht = 0x0ffffffff;
    interf_tape_in_bufsize       = 0;
    interf_tape_in_tapesped      = 0;
    interf_tape_in_state_fine    = 0;
    interf_tape_in_bit_fine      = 0;
    interf_tape_in_crc           = 0;
    interf_tape_in_kansascycle   = 0;
    interf_tape_in_elapsed_zclk  = 0;

    return;
}

void interf_tape_in_putbyteinbuff(UINT_8 what)
{
    INTERF_TAPE_IN_PUTBITINBUFF(0);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x001);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x002);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x004);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x008);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x010);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x020);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x040);
    INTERF_TAPE_IN_PUTBITINBUFF(what & 0x080);
    INTERF_TAPE_IN_PUTBITINBUFF(1);
    INTERF_TAPE_IN_PUTBITINBUFF(1);

    return;
}

char *interf_tape_in_pipe_data(char *filename, int tapesped)
{
    UINT_32 filesize;
    UINT_32 filesizewithblocks;
    UINT_32 i;
    UINT_32 currblocksize;
    UINT_8 fixblocksize;
    UINT_8 whatbyte;

    PC_FILE *interf_tape_in_src__fp;

    interf_tape_in_reset_state();

    {
        if ( ( interf_tape_in_src__fp = pc_fopen(filename,"rb") ) == NULL )
        {
            return "Couldn't open file.";
        }

        /*
           Count the bytes in the file.
        */

        filesize = 0;

        while ( !pc_feof(interf_tape_in_src__fp) )
        {
            pc_fgetc(interf_tape_in_src__fp);

            filesize++;
        }

        pc_fclose(interf_tape_in_src__fp);

        if ( filesize <= 0 )
        {
            return "No data in file.";
        }

        if ( ( interf_tape_in_src__fp = pc_fopen(filename,"rb") ) == NULL )
        {
            return "Couldn't re-open file.";
        }

        /*
           Add header size to filesize, so that filesize becomes the number
           of characters that will be passed.  42 is the size of the header
           block.
        */

        filesizewithblocks = filesize + (((filesize+0x000ff)/0x0100)*42);

        /*
           Calculate the number of bits required to pipe the file in.  This
           includes 1 boundary bit, 8 data bits and 2 stop bits, giving a
           total of 11 bits per byte to be passed in.

           NB: Include a final NULL, as the final byte will have it's last
               stop bit trimmed, hence want this to affect an irrelevant
               bit, not a relevant bit.
        */

        interf_tape_in_bufsize = (filesizewithblocks+1)*11;

        /*
           Allocate the buffer.  The buffer is kept in blocks of 32 bits, so
           some adjustment is required.
        */

        if ( ( interf_tape_in_buffer = (UINT_32 *) DEBMALLOC(((interf_tape_in_bufsize+0x01f)/0x020)*sizeof(UINT_32)) ) == NULL )
        {
            pc_fclose(interf_tape_in_src__fp);

            return "Out of memory.";
        }

        /*
           Get pipe speed.
        */

        interf_tape_in_tapesped = tapesped;

        /*
           Now convert the file into the contents of the file buffer

           Basic block structure: ff   +
                                  ff   | 20 of these
                                  ...  |
                                  ff   +
                                  2a
                                  ll   - length (0 for 256 bytes)
                                  data
                                  crc  - crc byte.  Initialize crc = 0 and
                                         for each byte d in data do:
                                         crc := (crc XOR d) >> 1
                                  ff   +
                                  ff   | 17 of these
                                  ...  |
                                  ff   +
        */

        interf_tape_in_pos_byte   = 0;
        interf_tape_in_pos_bit    = 0x000000001;
        interf_tape_in_pos_bitcnt = 0;

        while ( filesize > 0 )
        {
            interf_tape_in_crc = 0;

            if ( filesize >= 0x00100 )
            {
                currblocksize = 0x0100;
            }

            else
            {
                currblocksize = filesize;
            }

            fixblocksize = ( currblocksize & 0x0ff );
            filesize -= currblocksize;

            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x02a);
            interf_tape_in_putbyteinbuff(fixblocksize);

            for ( i = 1 ; i <= currblocksize ; i++ )
            {
                interf_tape_in_putbyteinbuff(whatbyte = pc_fgetc(interf_tape_in_src__fp));

                interf_tape_in_crc ^= whatbyte;

                if ( interf_tape_in_crc & 0x001 )
                {
                    interf_tape_in_crc = ( ( interf_tape_in_crc >> 0x001 ) & 0x07f ) | 0x080;
                }

                else
                {
                    interf_tape_in_crc =   ( interf_tape_in_crc >> 0x001 ) & 0x07f;
                }
            }

            interf_tape_in_putbyteinbuff(interf_tape_in_crc);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
            interf_tape_in_putbyteinbuff(0x0ff);
        }

        /*
           Put final NULL into the buffer.
        */

        interf_tape_in_putbyteinbuff(0x000);

        /*
           Set control data.
        */

        interf_tape_in_type         = 1;
        interf_tape_in_pos_byte     = 0;
        interf_tape_in_pos_bit      = 0x000000001;
        interf_tape_in_pos_bitcnt   = 0;
        interf_tape_in_state_fine   = 0;
        interf_tape_in_bit_fine     = 0;
        interf_tape_in_crc          = 0;
        interf_tape_in_kansascycle  = 0;
        interf_tape_in_elapsed_zclk = 0;

        pc_fclose(interf_tape_in_src__fp);

        /*
           Put speedchange bit out of bounds
        */

        interf_tape_in_spedchgbitcht = 0x0ffffffff;
    }

    return NULL;
}

int interf_tape_in_convert_to_memload(UINT_8 head_ft, UINT_8 head_fsl, UINT_8 head_fsh, UINT_8 head_lal, UINT_8 head_lah, UINT_8 head_ael, UINT_8 head_aeh, UINT_8 head_sp, UINT_8 head_ac, UINT_8 head_nu, char *yfilename, UINT_32 filesize, PC_FILE *interf_tape_in_src__fp)
{
    UINT_8  whatbyte;
    UINT_32 currblocksize;
    UINT_32 i;
    UINT_32 filesizewithblocks;

    interf_tape_in_reset_state();

    /*
       Add header size to filesize, so that filesize becomes the number
       of characters that will be passed.  For loading, there are 82
       bytes in the "header" + 1 crc byte for each data block.
    */

    filesizewithblocks = filesize + 82 + ((filesize+0x000ff)/0x0100);

    /*
       Calculate the number of bits required to pipe the file in.  This
       includes 1 boundary bit, 8 data bits and 2 stop bits, giving a
       total of 11 bits per byte to be passed in.

       NB: Include a final NULL, as the final byte will have it's last
           stop bit trimmed, hence want this to affect an irrelevant
           bit, not a relevant bit.
    */

    interf_tape_in_bufsize = (filesizewithblocks+1)*11;

    /*
       Allocate the buffer.  The buffer is kept in blocks of 32 bits, so
       some adjustment is required.
    */

    if ( ( interf_tape_in_buffer = (UINT_32 *) DEBMALLOC(((interf_tape_in_bufsize+0x01f)/0x020)*sizeof(UINT_32)) ) == NULL )
    {
        pc_fclose(interf_tape_in_src__fp);

        return 1;
    }

    /*
       Convert the file into the contents of the file buffer.
    */

    interf_tape_in_tapesped   = 0;
    interf_tape_in_pos_byte   = 0;
    interf_tape_in_pos_bit    = 0x000000001;
    interf_tape_in_pos_bitcnt = 0;

    /*
       First dump the header.
    */

    /*
       64 preceeding NULLs
    */

    for ( i = 1 ; i <= 64 ; i++ )
    {
        interf_tape_in_putbyteinbuff(0x000);
    }

    /*
       SOH
    */

    interf_tape_in_putbyteinbuff(0x001);

    interf_tape_in_crc = 0;

    /*
       Dump the filename
    */

    for ( i = 1 ; i <= 6 ; i++ )
    {
        if ( strlen(yfilename) >= i )
        {
            whatbyte = yfilename[i-1];
        }

        else
        {
            whatbyte = 0x000;
        }

        interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
        interf_tape_in_putbyteinbuff(whatbyte);
    }

    /*
       Dump the rest of the header.
    */

    whatbyte = head_ft;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_fsl;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_fsh;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_lal;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_lah;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_ael;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_aeh;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_sp;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_ac;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    whatbyte = head_nu;
    interf_tape_in_crc = MASKEDADD8(interf_tape_in_crc,whatbyte);
    interf_tape_in_putbyteinbuff(whatbyte);

    /*
       checksum: 1. sum the lot from after SOH to nu
                 2. take the 2's complement.
                 3. Then the weird bit - if the solution so far is xy
                    in hex (eg. 6e means x=6, y=e) then subtract 1
                    from x, giving the checksum (x-1)y so, in the
                    above example, the checksum would be 5e.
    */

    interf_tape_in_crc = TAKETWOS(interf_tape_in_crc);
    interf_tape_in_crc = ( ( ( interf_tape_in_crc & 0x0f0 ) - 0x010 ) & 0x0f0 ) | ( interf_tape_in_crc & 0x00f );
    interf_tape_in_putbyteinbuff(interf_tape_in_crc);

    /*
       If 1200baud, set speed change point
    */

    if ( head_sp )
    {
        interf_tape_in_spedchgbitcht = interf_tape_in_pos_bitcnt;
    }

    else
    {
        interf_tape_in_spedchgbitcht = 0x0ffffffff;
    }

    /*
       Translate the body.
    */

    while ( filesize > 0 )
    {
        interf_tape_in_crc = 0;

        if ( filesize >= 0x00100 )
        {
            currblocksize = 0x0100;
        }

        else
        {
            currblocksize = filesize;
        }

        filesize -= currblocksize;

        for ( i = 1 ; i <= currblocksize ; i++ )
        {
            interf_tape_in_putbyteinbuff(whatbyte = pc_fgetc(interf_tape_in_src__fp));

            /*
               For each byte: crc := CPL(d-c)
            */

            interf_tape_in_crc = MASKEDADD8(TAKETWOS(interf_tape_in_crc),whatbyte) ^ 0x0ff;
        }

        interf_tape_in_putbyteinbuff(interf_tape_in_crc);
    }

    /*
       Put final NULL into the buffer.
    */

    interf_tape_in_putbyteinbuff(0x000);

    /*
       Set control data.
    */

    interf_tape_in_type         = 1;
    interf_tape_in_pos_byte     = 0;
    interf_tape_in_pos_bit      = 0x000000001;
    interf_tape_in_pos_bitcnt   = 0;
    interf_tape_in_state_fine   = 0;
    interf_tape_in_bit_fine     = 0;
    interf_tape_in_crc          = 0;
    interf_tape_in_kansascycle  = 0;
    interf_tape_in_elapsed_zclk = 0;

    return 0;
}



#define LEFT_CORRECT (   (interf_scrn_physical_width/interf_scrn_horiz_line_mult)  \
                       + INTERF_SCRN_MAX_SCRNWIDTH_BEE                   \
                       - (interf_scrn_l_left_offset+interf_scrn_l_s_width+interf_scrn_l_right_offset)  ) / 2

#define TOP_CORRECT  (   (interf_scrn_physical_height/interf_scrn_vert_line_mult)  \
                       + INTERF_SCRN_MAX_SCRNHIGHT_BEE                  \
                       - (interf_scrn_l_top_offset+interf_scrn_l_s_height+interf_scrn_l_bottom_offset) ) / 2

void interf_scrn_set_left_margin(void *what)
{
    interf_scrn_blank_screen();

    interf_scrn_l_left_offset = INTERF_GFX_LEFT_MARGIN(what);
    interf_scrn_left_correct  = LEFT_CORRECT;

    interf_scrn_refresh_screen();

    return;
}

void interf_scrn_set_screen_width(void *what)
{
    interf_scrn_blank_screen();

    interf_scrn_l_s_width    = INTERF_GFX_SCRN_WIDTH(what);
    interf_scrn_left_correct = LEFT_CORRECT;

    interf_scrn_refresh_screen();

    return;
}

void interf_scrn_set_right_margin(void *what)
{
    interf_scrn_blank_screen();

    interf_scrn_l_right_offset = INTERF_GFX_RIGHT_MARGIN(what);
    interf_scrn_left_correct   = LEFT_CORRECT;

    interf_scrn_refresh_screen();

    return;
}

void interf_scrn_set_top_margin(void *what)
{
    interf_scrn_blank_screen();

    interf_scrn_l_top_offset = INTERF_GFX_TOP_MARGIN(what);
    interf_scrn_top_correct  = TOP_CORRECT;

    interf_scrn_refresh_screen();

    return;
}

void interf_scrn_set_screen_height(void *what)
{
    interf_scrn_blank_screen();

    interf_scrn_l_s_height  = INTERF_GFX_SCRN_HEIGHT(what);
    interf_scrn_top_correct = TOP_CORRECT;

    interf_scrn_refresh_screen();

    return;
}

void interf_scrn_set_bottom_margin(void *what)
{
    interf_scrn_blank_screen();

    interf_scrn_l_bottom_offset = INTERF_GFX_BOTTOM_MARGIN(what);
    interf_scrn_top_correct     = TOP_CORRECT;

    interf_scrn_refresh_screen();

    return;
}

void interf_scrn_8pixel_draw(void *what)
{
    #ifdef IS_ALLEGRO
    {
        long beescrn_aa_xpos;
        long beescrn_aa_ypos;
        long beescrn_za_xpos;
        long beescrn_za_ypos;
        long beescrn_qa_xpos;
        long beescrn_qa_ypos;
        long beescrn_ra_xpos;
        long beescrn_ra_ypos;
    
        beescrn_ra_xpos = INTERF_GFX_X_POS(what);
        beescrn_ra_ypos = INTERF_GFX_Y_POS(what);
    
        if ( interf_scrn_monitor_type )
        {
            if ( INTERF_GFX_INVERSION_BUS(what) )
            {
                INTERF_GFX_COL_BUS_FORE(what) = interf_scrn_mono_backcolour;
                INTERF_GFX_COL_BUS_BACK(what) = interf_scrn_mono_forecolour;
            }
    
            else
            {
                INTERF_GFX_COL_BUS_FORE(what) = interf_scrn_mono_forecolour;
                INTERF_GFX_COL_BUS_BACK(what) = interf_scrn_mono_backcolour;
            }
        }
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x080 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x040 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x020 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x010 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x008 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x004 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x002 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        beescrn_ra_xpos++;
    
        if ( ( beescrn_ra_xpos < INTERF_SCRN_MAX_SCRNWIDTH_BEE ) && ( beescrn_ra_ypos < INTERF_SCRN_MAX_SCRNHIGHT_BEE ) )
        {
            if ( INTERF_GFX_PIXEL_BUS(what) & 0x001 )
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
    
            else
            {
                _putpixel(interf_scrn_bee_screen,beescrn_ra_xpos,beescrn_ra_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        if ( interf_scrn_multip_fill && !interf_scrn_stepmode )
        {
            beescrn_qa_xpos = ((interf_scrn_left_correct+INTERF_GFX_X_POS(what)+interf_scrn_l_left_offset-(INTERF_SCRN_MAX_SCRNWIDTH_BEE /2)))*interf_scrn_horiz_line_mult;
            beescrn_qa_ypos = ((interf_scrn_top_correct +INTERF_GFX_Y_POS(what)+interf_scrn_l_top_offset -(INTERF_SCRN_MAX_SCRNHIGHT_BEE/2)))*interf_scrn_vert_line_mult;
    
            acquire_bitmap(screen);
    
            for ( beescrn_za_xpos = 0 ; beescrn_za_xpos < interf_scrn_horiz_line_mult ; beescrn_za_xpos++ )
            {
                for ( beescrn_za_ypos = 0 ; beescrn_za_ypos < interf_scrn_vert_line_mult ; beescrn_za_ypos++ )
                {
                    beescrn_aa_xpos = beescrn_qa_xpos+beescrn_za_xpos;
                    beescrn_aa_ypos = beescrn_qa_ypos+beescrn_za_ypos;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x080 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x040 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x020 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x010 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x008 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x004 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x002 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
    
                    beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
                    if ( ( beescrn_aa_xpos >= 0                           ) &&
                         ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                         ( beescrn_aa_ypos >= 0                           ) &&
                         ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
                    {
                        if ( INTERF_GFX_PIXEL_BUS(what) & 0x001 )
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
    
                        else
                        {
                            _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                        }
                    }
                }
            }
    
            release_bitmap(screen);
        }
    
        else if ( !interf_scrn_stepmode )
        {
            beescrn_aa_xpos = ((interf_scrn_left_correct+INTERF_GFX_X_POS(what)+interf_scrn_l_left_offset-(INTERF_SCRN_MAX_SCRNWIDTH_BEE /2)))*interf_scrn_horiz_line_mult;
            beescrn_aa_ypos = ((interf_scrn_top_correct +INTERF_GFX_Y_POS(what)+interf_scrn_l_top_offset -(INTERF_SCRN_MAX_SCRNHIGHT_BEE/2)))*interf_scrn_vert_line_mult;
    
            acquire_bitmap(screen);
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x080 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x040 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x020 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x010 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x008 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x004 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x002 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            beescrn_aa_xpos += interf_scrn_horiz_line_mult;
    
            if ( ( beescrn_aa_xpos >= 0                           ) &&
                 ( beescrn_aa_xpos <  interf_scrn_physical_width  ) &&
                 ( beescrn_aa_ypos >= 0                           ) &&
                 ( beescrn_aa_ypos <  interf_scrn_physical_height )    )
            {
                if ( INTERF_GFX_PIXEL_BUS(what) & 0x001 )
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_FORE(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
    
                else
                {
                    _putpixel(screen,beescrn_aa_xpos,beescrn_aa_ypos,INTERF_GFX_COL_BUS_BACK(what)+INTERF_SCRN_COLOUR_OFFSET);
                }
            }
    
            release_bitmap(screen);
        }
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Draw a sequence of 8 pixels.  Pixels are (left to right):

           INTERF_GFX_PIXEL_BUS(what) & 0x080
           INTERF_GFX_PIXEL_BUS(what) & 0x040
           INTERF_GFX_PIXEL_BUS(what) & 0x020
           INTERF_GFX_PIXEL_BUS(what) & 0x010
           INTERF_GFX_PIXEL_BUS(what) & 0x008
           INTERF_GFX_PIXEL_BUS(what) & 0x004
           INTERF_GFX_PIXEL_BUS(what) & 0x002
           INTERF_GFX_PIXEL_BUS(what) & 0x001

           If the relevant bit is set then the colour is set to foreground -
           otherwise it is background.  In colour modes (when
           interf_scrn_monitor_type == 0) the foreground colour is given by
           INTERF_GFX_COL_BUS_FORE(what) and the background by
           INTERF_GFX_COL_BUS_BACK(what).  Otherwise, the colours depend on
           INTERF_GFX_INVERSION_BUS(what).  If this bus is not set then the
           foreground colour is given by interf_scrn_mono_forecolour and
           the background by interf_scrn_mono_backcolour.  If this bus IS set
           then this is reversed.  Easiest to just cut and paste the code
           from above for this.

           The coordinates of the leftmost pixel are INTERF_GFX_X_POS(what),
           INTERF_GFX_Y_POS(what).  There are also offsets, but these
           probably aren't necessary for the web version, so best to just
           ignore them here.
        */
    }
    #endif
    
    return;

    what = NULL;
}

void interf_scrn_blank_screen(void)
{
    #ifdef IS_ALLEGRO
    {
        if ( !interf_scrn_stepmode )
        {
            rectfill(screen,0,0,interf_scrn_physical_width-1,interf_scrn_physical_height-1,interf_scrn_mono_backcolour+INTERF_SCRN_COLOUR_OFFSET);
        }
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Nothing for now.  This is used when adjusting screen margins
           to change the physical screen.
        */
    }
    #endif

    return;
}

void interf_scrn_refresh_screen(void)
{
    #ifdef IS_ALLEGRO
    {
        UINT_16 i;
        UINT_16 left_dest;
        UINT_16 right_dest;
        UINT_16 top_dest;
        UINT_16 bottom_dest;
    
        interf_scrn_lsource = 0;
        interf_scrn_rsource = interf_scrn_l_s_width;
    
        left_dest  = interf_scrn_left_correct+interf_scrn_l_left_offset;
        right_dest = interf_scrn_left_correct+interf_scrn_l_s_width+interf_scrn_l_left_offset-(INTERF_SCRN_MAX_SCRNWIDTH_BEE/2);
    
        if ( left_dest < (INTERF_SCRN_MAX_SCRNWIDTH_BEE/2) )
        {
            interf_scrn_lsource += ((INTERF_SCRN_MAX_SCRNWIDTH_BEE/2)-left_dest);
            left_dest            = 0;
        }
    
        else
        {
            left_dest -= (INTERF_SCRN_MAX_SCRNWIDTH_BEE/2);
        }
    
        if ( right_dest > interf_scrn_physical_width )
        {
            interf_scrn_rsource -= (right_dest-interf_scrn_physical_width);
            right_dest           = interf_scrn_physical_width;
        }
    
        interf_scrn_tsource = 0;
        interf_scrn_bsource = interf_scrn_l_s_height;
    
        top_dest    = interf_scrn_top_correct+interf_scrn_l_top_offset;
        bottom_dest = interf_scrn_top_correct+interf_scrn_l_s_height+interf_scrn_l_top_offset-(INTERF_SCRN_MAX_SCRNHIGHT_BEE/2);
    
        if ( top_dest < (INTERF_SCRN_MAX_SCRNHIGHT_BEE/2) )
        {
            interf_scrn_tsource += ((INTERF_SCRN_MAX_SCRNHIGHT_BEE/2)-top_dest);
            top_dest             = 0;
        }
    
        else
        {
            top_dest -= (INTERF_SCRN_MAX_SCRNHIGHT_BEE/2);
        }
    
        if ( bottom_dest > interf_scrn_physical_height )
        {
            interf_scrn_bsource -= (bottom_dest-interf_scrn_physical_height);
            bottom_dest          = interf_scrn_physical_height;
        }
    
        if ( !interf_scrn_stepmode )
        {
            stretch_blit(interf_scrn_bee_screen,screen,interf_scrn_lsource,interf_scrn_tsource,(interf_scrn_rsource-interf_scrn_lsource),(interf_scrn_bsource-interf_scrn_tsource),left_dest*interf_scrn_horiz_line_mult,top_dest*interf_scrn_vert_line_mult,(interf_scrn_rsource-interf_scrn_lsource)*interf_scrn_horiz_line_mult,(interf_scrn_bsource-interf_scrn_tsource)*interf_scrn_vert_line_mult);
        }
    
        /* ASSUMPTION HERE: stretch_blit will do line multiplication by
           duplication of lines.  Hence, if multip_fill_option == 0 we need
           to overwrite odd numbered lines with black. */
    
        if ( !interf_scrn_stepmode && !interf_scrn_multip_fill & ( interf_scrn_horiz_line_mult == 2 ) )
        {
            for ( i = 0x001 ; i < interf_scrn_physical_width ; i += interf_scrn_horiz_line_mult )
            {
                vline(screen,i,0,interf_scrn_physical_height-1,interf_scrn_mono_backcolour+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    
        if ( !interf_scrn_stepmode && !interf_scrn_multip_fill & ( interf_scrn_vert_line_mult == 2 ) )
        {
            for ( i = 0x001 ; i < interf_scrn_physical_height ; i += interf_scrn_vert_line_mult )
            {
                hline(screen,0,i,interf_scrn_physical_width-1,interf_scrn_mono_backcolour+INTERF_SCRN_COLOUR_OFFSET);
            }
        }
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Refresh the screen.  Not necessary unless you decide to do fancy
           stuff like allow for screen margins (maybe not in ver 1?)
        */
    }
    #endif
    
    return;
}


int interf_scrn_screenshot(const char shotname[])
{
    #ifdef IS_ALLEGRO
    {
        BITMAP *screenshot_bmp;
    
        if ( ( screenshot_bmp = create_sub_bitmap(interf_scrn_bee_screen,interf_scrn_lsource,interf_scrn_tsource,interf_scrn_rsource-interf_scrn_lsource,interf_scrn_bsource-interf_scrn_tsource) ) == NULL )
        {
            return 1;
        }
    
        if ( save_bmp(shotname,screenshot_bmp,interf_scrn_palette) )
        {
            return 1;
        }
    
        destroy_bitmap(screenshot_bmp);
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Grab screenshot (filename given).
        */
    }
    #endif

    return 0;

    char temp;
    temp = shotname[0];
}


void interf_scrn_fix_palette(void)
{
    #ifdef IS_ALLEGRO
    {
        if ( interf_scrn_colour_full <= 0  ) { interf_scrn_colour_full = 0;  }
        if ( interf_scrn_colour_full >= 63 ) { interf_scrn_colour_full = 63; }
    
        if ( interf_scrn_colour_back <= 0  ) { interf_scrn_colour_back = 0;  }
        if ( interf_scrn_colour_back >= 63 ) { interf_scrn_colour_back = 63; }
    
        if ( interf_scrn_colour_half < interf_scrn_colour_back ) { interf_scrn_colour_half = interf_scrn_colour_back; }
        if ( interf_scrn_colour_half > interf_scrn_colour_full ) { interf_scrn_colour_half = interf_scrn_colour_full; }
    
        interf_scrn_colour_ooo.r = interf_scrn_colour_back;
        interf_scrn_colour_ool.r = interf_scrn_colour_back;
        interf_scrn_colour_ooh.r = interf_scrn_colour_back;
        interf_scrn_colour_olo.r = interf_scrn_colour_back;
        interf_scrn_colour_oll.r = interf_scrn_colour_back;
        interf_scrn_colour_olh.r = interf_scrn_colour_back;
        interf_scrn_colour_oho.r = interf_scrn_colour_back;
        interf_scrn_colour_ohl.r = interf_scrn_colour_back;
        interf_scrn_colour_ohh.r = interf_scrn_colour_back;
        interf_scrn_colour_loo.r = interf_scrn_colour_half;
        interf_scrn_colour_lol.r = interf_scrn_colour_half;
        interf_scrn_colour_loh.r = interf_scrn_colour_half;
        interf_scrn_colour_llo.r = interf_scrn_colour_half;
        interf_scrn_colour_lll.r = interf_scrn_colour_half;
        interf_scrn_colour_llh.r = interf_scrn_colour_half;
        interf_scrn_colour_lho.r = interf_scrn_colour_half;
        interf_scrn_colour_lhl.r = interf_scrn_colour_half;
        interf_scrn_colour_lhh.r = interf_scrn_colour_half;
        interf_scrn_colour_hoo.r = interf_scrn_colour_full;
        interf_scrn_colour_hol.r = interf_scrn_colour_full;
        interf_scrn_colour_hoh.r = interf_scrn_colour_full;
        interf_scrn_colour_hlo.r = interf_scrn_colour_full;
        interf_scrn_colour_hll.r = interf_scrn_colour_full;
        interf_scrn_colour_hlh.r = interf_scrn_colour_full;
        interf_scrn_colour_hho.r = interf_scrn_colour_full;
        interf_scrn_colour_hhl.r = interf_scrn_colour_full;
        interf_scrn_colour_hhh.r = interf_scrn_colour_full;
    
        interf_scrn_colour_ooo.g = interf_scrn_colour_back;
        interf_scrn_colour_ool.g = interf_scrn_colour_back;
        interf_scrn_colour_ooh.g = interf_scrn_colour_back;
        interf_scrn_colour_olo.g = interf_scrn_colour_half;
        interf_scrn_colour_oll.g = interf_scrn_colour_half;
        interf_scrn_colour_olh.g = interf_scrn_colour_half;
        interf_scrn_colour_oho.g = interf_scrn_colour_full;
        interf_scrn_colour_ohl.g = interf_scrn_colour_full;
        interf_scrn_colour_ohh.g = interf_scrn_colour_full;
        interf_scrn_colour_loo.g = interf_scrn_colour_back;
        interf_scrn_colour_lol.g = interf_scrn_colour_back;
        interf_scrn_colour_loh.g = interf_scrn_colour_back;
        interf_scrn_colour_llo.g = interf_scrn_colour_half;
        interf_scrn_colour_lll.g = interf_scrn_colour_half;
        interf_scrn_colour_llh.g = interf_scrn_colour_half;
        interf_scrn_colour_lho.g = interf_scrn_colour_full;
        interf_scrn_colour_lhl.g = interf_scrn_colour_full;
        interf_scrn_colour_lhh.g = interf_scrn_colour_full;
        interf_scrn_colour_hoo.g = interf_scrn_colour_back;
        interf_scrn_colour_hol.g = interf_scrn_colour_back;
        interf_scrn_colour_hoh.g = interf_scrn_colour_back;
        interf_scrn_colour_hlo.g = interf_scrn_colour_half;
        interf_scrn_colour_hll.g = interf_scrn_colour_half;
        interf_scrn_colour_hlh.g = interf_scrn_colour_half;
        interf_scrn_colour_hho.g = interf_scrn_colour_full;
        interf_scrn_colour_hhl.g = interf_scrn_colour_full;
        interf_scrn_colour_hhh.g = interf_scrn_colour_full;
    
        interf_scrn_colour_ooo.b = interf_scrn_colour_back;
        interf_scrn_colour_ool.b = interf_scrn_colour_half;
        interf_scrn_colour_ooh.b = interf_scrn_colour_full;
        interf_scrn_colour_olo.b = interf_scrn_colour_back;
        interf_scrn_colour_oll.b = interf_scrn_colour_half;
        interf_scrn_colour_olh.b = interf_scrn_colour_full;
        interf_scrn_colour_oho.b = interf_scrn_colour_back;
        interf_scrn_colour_ohl.b = interf_scrn_colour_half;
        interf_scrn_colour_ohh.b = interf_scrn_colour_full;
        interf_scrn_colour_loo.b = interf_scrn_colour_back;
        interf_scrn_colour_lol.b = interf_scrn_colour_half;
        interf_scrn_colour_loh.b = interf_scrn_colour_full;
        interf_scrn_colour_llo.b = interf_scrn_colour_back;
        interf_scrn_colour_lll.b = interf_scrn_colour_half;
        interf_scrn_colour_llh.b = interf_scrn_colour_full;
        interf_scrn_colour_lho.b = interf_scrn_colour_back;
        interf_scrn_colour_lhl.b = interf_scrn_colour_half;
        interf_scrn_colour_lhh.b = interf_scrn_colour_full;
        interf_scrn_colour_hoo.b = interf_scrn_colour_back;
        interf_scrn_colour_hol.b = interf_scrn_colour_half;
        interf_scrn_colour_hoh.b = interf_scrn_colour_full;
        interf_scrn_colour_hlo.b = interf_scrn_colour_back;
        interf_scrn_colour_hll.b = interf_scrn_colour_half;
        interf_scrn_colour_hlh.b = interf_scrn_colour_full;
        interf_scrn_colour_hho.b = interf_scrn_colour_back;
        interf_scrn_colour_hhl.b = interf_scrn_colour_half;
        interf_scrn_colour_hhh.b = interf_scrn_colour_full;
    
    
        interf_scrn_palette[0x080] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x081] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x082] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x083] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x084] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x085] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x086] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x087] = interf_scrn_colour_ooo;
        interf_scrn_palette[0x088] = interf_scrn_colour_ool;
        interf_scrn_palette[0x089] = interf_scrn_colour_ooh;
        interf_scrn_palette[0x08A] = interf_scrn_colour_ool;
        interf_scrn_palette[0x08B] = interf_scrn_colour_ooh;
        interf_scrn_palette[0x08C] = interf_scrn_colour_ool;
        interf_scrn_palette[0x08D] = interf_scrn_colour_ooh;
        interf_scrn_palette[0x08E] = interf_scrn_colour_ool;
        interf_scrn_palette[0x08F] = interf_scrn_colour_ooh;
        interf_scrn_palette[0x090] = interf_scrn_colour_olo;
        interf_scrn_palette[0x091] = interf_scrn_colour_olo;
        interf_scrn_palette[0x092] = interf_scrn_colour_oho;
        interf_scrn_palette[0x093] = interf_scrn_colour_oho;
        interf_scrn_palette[0x094] = interf_scrn_colour_olo;
        interf_scrn_palette[0x095] = interf_scrn_colour_olo;
        interf_scrn_palette[0x096] = interf_scrn_colour_oho;
        interf_scrn_palette[0x097] = interf_scrn_colour_oho;
        interf_scrn_palette[0x098] = interf_scrn_colour_oll;
        interf_scrn_palette[0x099] = interf_scrn_colour_olh;
        interf_scrn_palette[0x09A] = interf_scrn_colour_ohl;
        interf_scrn_palette[0x09B] = interf_scrn_colour_ohh;
        interf_scrn_palette[0x09C] = interf_scrn_colour_oll;
        interf_scrn_palette[0x09D] = interf_scrn_colour_olh;
        interf_scrn_palette[0x09E] = interf_scrn_colour_ohl;
        interf_scrn_palette[0x09F] = interf_scrn_colour_ohh;
        interf_scrn_palette[0x0A0] = interf_scrn_colour_loo;
        interf_scrn_palette[0x0A1] = interf_scrn_colour_loo;
        interf_scrn_palette[0x0A2] = interf_scrn_colour_loo;
        interf_scrn_palette[0x0A3] = interf_scrn_colour_loo;
        interf_scrn_palette[0x0A4] = interf_scrn_colour_hoo;
        interf_scrn_palette[0x0A5] = interf_scrn_colour_hoo;
        interf_scrn_palette[0x0A6] = interf_scrn_colour_hoo;
        interf_scrn_palette[0x0A7] = interf_scrn_colour_hoo;
        interf_scrn_palette[0x0A8] = interf_scrn_colour_lol;
        interf_scrn_palette[0x0A9] = interf_scrn_colour_loh;
        interf_scrn_palette[0x0AA] = interf_scrn_colour_lol;
        interf_scrn_palette[0x0AB] = interf_scrn_colour_loh;
        interf_scrn_palette[0x0AC] = interf_scrn_colour_hol;
        interf_scrn_palette[0x0AD] = interf_scrn_colour_hoh;
        interf_scrn_palette[0x0AE] = interf_scrn_colour_hol;
        interf_scrn_palette[0x0AF] = interf_scrn_colour_hoh;
        interf_scrn_palette[0x0B0] = interf_scrn_colour_llo;
        interf_scrn_palette[0x0B1] = interf_scrn_colour_llo;
        interf_scrn_palette[0x0B2] = interf_scrn_colour_lho;
        interf_scrn_palette[0x0B3] = interf_scrn_colour_lho;
        interf_scrn_palette[0x0B4] = interf_scrn_colour_hlo;
        interf_scrn_palette[0x0B5] = interf_scrn_colour_hlo;
        interf_scrn_palette[0x0B6] = interf_scrn_colour_hho;
        interf_scrn_palette[0x0B7] = interf_scrn_colour_hho;
        interf_scrn_palette[0x0B8] = interf_scrn_colour_lll;
        interf_scrn_palette[0x0B9] = interf_scrn_colour_llh;
        interf_scrn_palette[0x0BA] = interf_scrn_colour_lhl;
        interf_scrn_palette[0x0BB] = interf_scrn_colour_lhh;
        interf_scrn_palette[0x0BC] = interf_scrn_colour_hll;
        interf_scrn_palette[0x0BD] = interf_scrn_colour_hlh;
        interf_scrn_palette[0x0BE] = interf_scrn_colour_hhl;
        interf_scrn_palette[0x0BF] = interf_scrn_colour_hhh;
    
        interf_scrn_palette[0x0C1] = interf_scrn_full_colour_hhh;
        interf_scrn_palette[0x0C2] = interf_scrn_full_colour_lll;
        interf_scrn_palette[0x0C3] = interf_scrn_full_colour_ooo;
    
        /*
           Bee pointer palette components - these have been reverse engineered
           from mbee32k.bmp
        */
    
        interf_scrn_palette[0x0D1] = interf_scrn_full_colour_xxx; /* dark grey */
        interf_scrn_palette[0x0D2] = interf_scrn_full_colour_hlo; /* orange    */
        interf_scrn_palette[0x0D3] = interf_scrn_full_colour_hhh; /* white     */
    }
    #endif

    #ifdef IS_WEB
    {
        /*
           Fixup the palette.
        */
    }
    #endif

    return;
}

void interf_scrn_correct_scrnsize(void)
{
    interf_scrn_left_correct = LEFT_CORRECT;
    interf_scrn_top_correct  = TOP_CORRECT;

    return;
}

int interf_scrn_set_gfxmode(void)
{
    #ifdef IS_ALLEGRO
    {
        /*
           The video mode is selected in order of preference depending on what
           is available on the system in question.
        */
    
        if ( interf_scrn_pref_fullscrn )
        {
            if ( set_gfx_mode(GFX_AUTODETECT_FULLSCREEN,interf_scrn_physical_width,interf_scrn_physical_height,0,0) )
            {
                if ( set_gfx_mode(GFX_AUTODETECT_WINDOWED,interf_scrn_physical_width,interf_scrn_physical_height,0,0) )
                {
                    if ( set_gfx_mode(GFX_AUTODETECT,interf_scrn_physical_width,interf_scrn_physical_height,0,0) )
                    {
                        if ( ( interf_scrn_physical_width == 640 ) && ( interf_scrn_physical_height == 480 ) )
                        {
                            return 1;
                        }
    
                        interf_menu_setvidmode0();
    
                        return interf_scrn_set_gfxmode();
                    }
                }
            }
        }
    
        else
        {
            if ( set_gfx_mode(GFX_AUTODETECT_WINDOWED,interf_scrn_physical_width,interf_scrn_physical_height,0,0) )
            {
                if ( set_gfx_mode(GFX_AUTODETECT_FULLSCREEN,interf_scrn_physical_width,interf_scrn_physical_height,0,0) )
                {
                    if ( set_gfx_mode(GFX_AUTODETECT,interf_scrn_physical_width,interf_scrn_physical_height,0,0) )
                    {
                        if ( ( interf_scrn_physical_width == 640 ) && ( interf_scrn_physical_height == 480 ) )
                        {
                            return 1;
                        }
    
                        interf_menu_setvidmode0();
    
                        return interf_scrn_set_gfxmode();
                    }
                }
            }
        }
    
        /*
           Set switch mode (in order of preference)
        */
    
        if ( set_display_switch_mode(SWITCH_BACKGROUND) == -1 )
        {
            if ( set_display_switch_mode(SWITCH_BACKAMNESIA) == -1 )
            {
                if ( set_display_switch_mode(SWITCH_NONE) == -1 )
                {
                    return 1;
                }
            }
        }
    
        set_display_switch_callback(SWITCH_IN,interf_scrn_refresh_screen);
    }
    #endif

    return 0;
}

void interf_scrn_set_bright(UINT_8 what)
{
    interf_scrn_bright = what;

    interf_scrn_colour_full = (((long) interf_scrn_bright)*63)/255;
    interf_scrn_colour_half = ((255-((long) interf_scrn_contrast))*((long) interf_scrn_colour_full))/255;
    interf_scrn_colour_back = 0;

    interf_scrn_fix_palette();

    return;
}

void interf_scrn_set_contrast(UINT_8 what)
{
    interf_scrn_contrast = what;

    interf_scrn_colour_full = (((long) interf_scrn_bright)*63)/255;
    interf_scrn_colour_half = ((255-((long) interf_scrn_contrast))*((long) interf_scrn_colour_full))/255;
    interf_scrn_colour_back = 0;

    interf_scrn_fix_palette();

    return;
}

int interf_scrn_set_vidmode(int what)
{
    #ifdef IS_ALLEGRO
    {
        interf_scrn_video_mode = what;
    
        switch ( (interf_scrn_video_mode) )
        {
            case 0:
            {
                interf_scrn_physical_width  = 640;
                interf_scrn_physical_height = 480;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 1;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            case 1:
            {
                interf_scrn_physical_width  = 800;
                interf_scrn_physical_height = 600;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 2;
    
                interf_scrn_multip_fill = 1;
    
                break;
            }
    
            case 2:
            {
                interf_scrn_physical_width  = 1024;
                interf_scrn_physical_height = 768;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 2;
    
                interf_scrn_multip_fill = 1;
    
                break;
            }
    
            case 3:
            {
                interf_scrn_physical_width  = 1280;
                interf_scrn_physical_height = 1024;
    
                interf_scrn_horiz_line_mult = 2;
                interf_scrn_vert_line_mult  = 2;
    
                interf_scrn_multip_fill = 1;
    
                break;
            }
    
            case 4:
            {
                interf_scrn_physical_width  = 800;
                interf_scrn_physical_height = 600;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 2;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            case 5:
            {
                interf_scrn_physical_width  = 1024;
                interf_scrn_physical_height = 768;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 2;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            case 6:
            {
                interf_scrn_physical_width  = 1280;
                interf_scrn_physical_height = 1024;
    
                interf_scrn_horiz_line_mult = 2;
                interf_scrn_vert_line_mult  = 2;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            case 7:
            {
                interf_scrn_physical_width  = 800;
                interf_scrn_physical_height = 600;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 1;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            case 8:
            {
                interf_scrn_physical_width  = 1024;
                interf_scrn_physical_height = 768;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 1;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            case 9:
            {
                interf_scrn_physical_width  = 1280;
                interf_scrn_physical_height = 1024;
    
                interf_scrn_horiz_line_mult = 1;
                interf_scrn_vert_line_mult  = 1;
    
                interf_scrn_multip_fill = 0;
    
                break;
            }
    
            default:
            {
                return 1;
    
                break;
            }
        }
    }    
    #endif

    return 0;

    what = NULL;
}






















































/*
   Allegro menu functionality
   ==========================
*/

#ifdef IS_ALLEGRO

#define MENU_WHITE      0x0C1
#define MENU_GREY       0x0C2
#define MENU_BLACK      0x0C3
#define ctrl(x)         (x - 'a' + 1)

void interf_menu_update_menu_marks(void);

int interf_menu_xstepperenter(int msg, DIALOG *d, int c);
int interf_menu_setbrightx(void *dp3, int d2);
int interf_menu_setcontrastx(void *dp3, int d2);

int interf_menu_cpuclkon(void);
int interf_menu_cpuclkoff(void);
int interf_menu_stepreturn(void);
int interf_menu_prtscrn(void);
int interf_menu_return(void);
int interf_menu_exit(void);
int interf_menu_scrnshot(void);

int interf_menu_soundoff(void);
int interf_menu_soundon(void);

int interf_menu_setvidmodewindow(void);
int interf_menu_setvidmodefullscrn(void);
int interf_menu_setvidmode1(void);
int interf_menu_setvidmode2(void);
int interf_menu_setvidmode3(void);
int interf_menu_setvidmode4(void);
int interf_menu_setvidmode5(void);
int interf_menu_setvidmode6(void);
int interf_menu_setvidmode7(void);
int interf_menu_setvidmode8(void);
int interf_menu_setvidmode9(void);
int interf_menu_setdispcolour(void);
int interf_menu_setdispgreen(void);
int interf_menu_setdispamber(void);
int interf_menu_setdispbw(void);
int interf_menu_setdispinvgreen(void);
int interf_menu_setdispinvamber(void);
int interf_menu_setdispinvbw(void);

int interf_menu_keyboardnormal(void);
int interf_menu_keyboardfile(void);

int interf_menu_setparamode0(void);
int interf_menu_setparamode1(void);
int interf_menu_setparamode2(void);
int interf_menu_setparamode3(void);
int interf_menu_setparamode4(void);
int interf_menu_setparamode5(void);

int interf_menu_settapemode1(void);
int interf_menu_settapemode3(void);
int interf_menu_tapeautosaveon(void);
int interf_menu_tapeautosaveoff(void);
int interf_menu_tapesavedir(void);
int interf_menu_tapeinpipe(void);
int interf_menu_tapeinloaddata(void);
int interf_menu_tapeinloadmtd(void);
int interf_menu_tapestopload(void);


char interf_menu_main_clock_stra[] = "- CPU clock speed &normal (3.141 MHz).";
char interf_menu_main_clock_strb[] = "- CPU clock speed &unlimited.";

MENU interf_menu_main_clock[] =
{
    { interf_menu_main_clock_stra, interf_menu_cpuclkon,  NULL, 0, NULL },
    { interf_menu_main_clock_strb, interf_menu_cpuclkoff, NULL, 0, NULL },
    { NULL,                        NULL,                  NULL, 0, NULL }
};

MENU interf_menu_main[] =
{
    { "CPU clock &rate",               NULL,                   interf_menu_main_clock, 0, NULL },
    { "",                              NULL,                   NULL,                   0, NULL },
    { "S&ingle step emulator \tENTER", interf_menu_stepreturn, NULL,                   0, NULL },
    { "Res&tart emulator \tESC",       interf_menu_return,     NULL,                   0, NULL },
    { "E&xit emulator \tctrl-x",       interf_menu_exit,       NULL,                   0, NULL },
    { NULL,                            NULL,                   NULL,                   0, NULL }
};

char interf_menu_sound_stra[] = "- Sound o&ff";
char interf_menu_sound_strb[] = "- Sound o&n";

MENU interf_menu_sound[] =
{
    { interf_menu_sound_stra, interf_menu_soundoff, NULL, 0, NULL },
    { interf_menu_sound_strb, interf_menu_soundon,  NULL, 0, NULL },
    { NULL,                   NULL,                 NULL, 0, NULL }
};

char interf_menu_display_stra[] = "- &Colour mode";
char interf_menu_display_strb[] = "- &Greenscreen mode";
char interf_menu_display_strc[] = "- &Amberscreen mode";
char interf_menu_display_strd[] = "- &Black and White mode";
char interf_menu_display_stre[] = "- Inverted G&reenscreen mode";
char interf_menu_display_strf[] = "- Inverted A&mberscreen mode";
char interf_menu_display_strg[] = "- Inverted B&lack and White mode";

char interf_menu_display_resol_stra[] = "- &0. 640x480";
char interf_menu_display_resol_strb[] = "- &1. 800x600 (vertically doubled)";
char interf_menu_display_resol_strc[] = "- &2. 1024x768 (vertically doubled)";
char interf_menu_display_resol_strd[] = "- &3. 1280x1024 (bi-directionally doubled)";
char interf_menu_display_resol_stre[] = "- &4. 800x600 (vertically stretched)";
char interf_menu_display_resol_strf[] = "- &5. 1024x768 (vertically stretched)";
char interf_menu_display_resol_strg[] = "- &6. 1280x1024 (bi-directionally stretched)";
char interf_menu_display_resol_strh[] = "- &7. 800x600";
char interf_menu_display_resol_stri[] = "- &8. 1024x768";
char interf_menu_display_resol_strj[] = "- &9. 1280x1024";
char interf_menu_display_resol_strx[] = "- Default to &window";
char interf_menu_display_resol_stry[] = "  Default to &fullscreen";

MENU interf_menu_display_resol[] =
{
    { interf_menu_display_resol_stra, interf_menu_setvidmode0,        NULL, 0, NULL },
    { interf_menu_display_resol_strb, interf_menu_setvidmode1,        NULL, 0, NULL },
    { interf_menu_display_resol_strc, interf_menu_setvidmode2,        NULL, 0, NULL },
    { interf_menu_display_resol_strd, interf_menu_setvidmode3,        NULL, 0, NULL },
    { interf_menu_display_resol_stre, interf_menu_setvidmode4,        NULL, 0, NULL },
    { interf_menu_display_resol_strf, interf_menu_setvidmode5,        NULL, 0, NULL },
    { interf_menu_display_resol_strg, interf_menu_setvidmode6,        NULL, 0, NULL },
    { interf_menu_display_resol_strh, interf_menu_setvidmode7,        NULL, 0, NULL },
    { interf_menu_display_resol_stri, interf_menu_setvidmode8,        NULL, 0, NULL },
    { interf_menu_display_resol_strj, interf_menu_setvidmode9,        NULL, 0, NULL },
    { "",                             NULL,                           NULL, 0, NULL },
    { interf_menu_display_resol_strx, interf_menu_setvidmodewindow,   NULL, 0, NULL },
    { interf_menu_display_resol_stry, interf_menu_setvidmodefullscrn, NULL, 0, NULL },
    { NULL,                           NULL,                           NULL, 0, NULL }
};

MENU interf_menu_display[] =
{
    { "Take &Screenshot",       interf_menu_scrnshot,        NULL,                      0, NULL },
    { "",                       NULL,                        NULL,                      0, NULL },
    { interf_menu_display_stra, interf_menu_setdispcolour,   NULL,                      0, NULL },
    { interf_menu_display_strb, interf_menu_setdispgreen,    NULL,                      0, NULL },
    { interf_menu_display_strc, interf_menu_setdispamber,    NULL,                      0, NULL },
    { interf_menu_display_strd, interf_menu_setdispbw,       NULL,                      0, NULL },
    { interf_menu_display_stre, interf_menu_setdispinvgreen, NULL,                      0, NULL },
    { interf_menu_display_strf, interf_menu_setdispinvamber, NULL,                      0, NULL },
    { interf_menu_display_strg, interf_menu_setdispinvbw,    NULL,                      0, NULL },
    { "",                       NULL,                        NULL,                      0, NULL },
    { "Se&t Resolution",        NULL,                        interf_menu_display_resol, 0, NULL },
    { NULL,                     NULL,                        NULL,                      0, NULL }
};

char interf_menu_keyboard_stra[] = "- &Normal";
char interf_menu_keyboard_strb[] = "- &Redirect from file";

MENU interf_menu_keyboard[] =
{
    { interf_menu_keyboard_stra, interf_menu_keyboardnormal, NULL, 0, NULL },
    { interf_menu_keyboard_strb, interf_menu_keyboardfile,   NULL, 0, NULL },
    { NULL,                      NULL,                       NULL, 0, NULL }
};

char interf_menu_para_stra[] = "- Output to pc &parallel port (direct)";
char interf_menu_para_strb[] = "- Output to pc parallel port (&via os)";
char interf_menu_para_strc[] = "- Output to &file";
char interf_menu_para_strd[] = "- Output to &NULL";
char interf_menu_para_stre[] = "- &Input from &file";
char interf_menu_para_strf[] = "- &Unconnected";

#ifdef PARA_ACCESS_BIOS
#ifdef PARA_ACCESS_HARD
MENU interf_menu_para[] =
{
    { interf_menu_para_stra, interf_menu_setparamode0, NULL, 0, NULL },
    { interf_menu_para_strb, interf_menu_setparamode5, NULL, 0, NULL },
    { interf_menu_para_strc, interf_menu_setparamode1, NULL, 0, NULL },
    { interf_menu_para_strd, interf_menu_setparamode2, NULL, 0, NULL },
    { interf_menu_para_stre, interf_menu_setparamode3, NULL, 0, NULL },
    { interf_menu_para_strf, interf_menu_setparamode4, NULL, 0, NULL },
    { NULL,                  NULL,                     NULL, 0, NULL }
};
#endif

#ifndef PARA_ACCESS_HARD
MENU interf_menu_para[] =
{
    { interf_menu_para_stra, interf_menu_setparamode0, NULL, 0,          NULL },
    { interf_menu_para_strb, interf_menu_setparamode5, NULL, D_DISABLED, NULL },
    { interf_menu_para_strc, interf_menu_setparamode1, NULL, 0,          NULL },
    { interf_menu_para_strd, interf_menu_setparamode2, NULL, 0,          NULL },
    { interf_menu_para_stre, interf_menu_setparamode3, NULL, 0,          NULL },
    { interf_menu_para_strf, interf_menu_setparamode4, NULL, 0,          NULL },
    { NULL,                  NULL,                     NULL, 0,          NULL }
};
#endif
#endif

#ifndef PARA_ACCESS_BIOS
#ifdef PARA_ACCESS_HARD
MENU interf_menu_para[] =
{
    { interf_menu_para_stra, interf_menu_setparamode0, NULL, D_DISABLED, NULL },
    { interf_menu_para_strb, interf_menu_setparamode5, NULL, 0,          NULL },
    { interf_menu_para_strc, interf_menu_setparamode1, NULL, 0,          NULL },
    { interf_menu_para_strd, interf_menu_setparamode2, NULL, 0,          NULL },
    { interf_menu_para_stre, interf_menu_setparamode3, NULL, 0,          NULL },
    { interf_menu_para_strf, interf_menu_setparamode4, NULL, 0,          NULL },
    { NULL,                  NULL,                     NULL, 0,          NULL }
};
#endif

#ifndef PARA_ACCESS_HARD
#endif
MENU interf_menu_para[] =
{
    { interf_menu_para_stra, interf_menu_setparamode0, NULL, D_DISABLED, NULL },
    { interf_menu_para_strb, interf_menu_setparamode5, NULL, D_DISABLED, NULL },
    { interf_menu_para_strc, interf_menu_setparamode1, NULL, 0,          NULL },
    { interf_menu_para_strd, interf_menu_setparamode2, NULL, 0,          NULL },
    { interf_menu_para_stre, interf_menu_setparamode3, NULL, 0,          NULL },
    { interf_menu_para_strf, interf_menu_setparamode4, NULL, 0,          NULL },
    { NULL,                  NULL,                     NULL, 0,          NULL }
};
#endif

char interf_menu_rs232_stra[] = "  Output to pc &serial port";
char interf_menu_rs232_strb[] = "  Output to &file";
char interf_menu_rs232_strc[] = "  Output to virtual &modem";
char interf_menu_rs232_strd[] = "  Output to &NULL";
char interf_menu_rs232_stre[] = "- Output &Unconnected";

char interf_menu_rs232_strf[] = "  Input from pc s&erial port";
char interf_menu_rs232_strg[] = "  Input from fil&e";
char interf_menu_rs232_strh[] = "  Input from virtual mo&dem";
char interf_menu_rs232_stri[] = "- Input U&nconnected";

MENU interf_menu_rs232[] =
{
    { interf_menu_rs232_stra,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_strb,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_strc,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_strd,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_stre,         NULL, NULL, D_DISABLED, NULL },
    { "",                             NULL, NULL, 0,          NULL },
    { "Serial port &output settings", NULL, NULL, D_DISABLED, NULL },
    { "",                             NULL, NULL, 0,          NULL },
    { interf_menu_rs232_strf,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_strg,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_strh,         NULL, NULL, D_DISABLED, NULL },
    { interf_menu_rs232_stri,         NULL, NULL, D_DISABLED, NULL },
    { "",                             NULL, NULL, 0,          NULL },
    { "Serial port &input settings",  NULL, NULL, D_DISABLED, NULL },
    { NULL,                           NULL, NULL, 0,          NULL }
};

char interf_menu_tape_stra[] = "  Output to &Data file";
char interf_menu_tape_strb[] = "  Output to &Wav file";
char interf_menu_tape_strc[] = "- Output &Unconnected";

char interf_menu_tape_strd[] = "  Autosave off";
char interf_menu_tape_stre[] = "- Autosave on";

MENU interf_menu_tape[] =
{
    { interf_menu_tape_stra,        interf_menu_settapemode1,    NULL, 0,          NULL },
    { interf_menu_tape_strb,        NULL,                        NULL, D_DISABLED, NULL },
    { interf_menu_tape_strc,        interf_menu_settapemode3,    NULL, 0,          NULL },
    { "",                           NULL,                        NULL, 0,          NULL },
    { interf_menu_tape_strd,        interf_menu_tapeautosaveoff, NULL, 0,          NULL },
    { interf_menu_tape_stre,        interf_menu_tapeautosaveon,  NULL, 0,          NULL },
    { "",                           NULL,                        NULL, 0,          NULL },
    { "Select AutoSave Directory",  interf_menu_tapesavedir,     NULL, 0,          NULL },
    { "",                           NULL,                        NULL, 0,          NULL },
    { "Pipe input from D&ata file", interf_menu_tapeinpipe,      NULL, 0,          NULL },
    { "Pipe input from Wa&v file",  NULL,                        NULL, D_DISABLED, NULL },
    { "Load from Da&ta file",       interf_menu_tapeinloaddata,  NULL, 0,          NULL },
    { "Load from &mtd file",        interf_menu_tapeinloadmtd,   NULL, 0,          NULL },
    { "",                           NULL,                        NULL, 0,          NULL },
    { "Stop load operation",        interf_menu_tapestopload,    NULL, 0,          NULL },
    { NULL,                         NULL,                        NULL, 0,          NULL }
};

MENU interf_menu_base[] =
{
    { "&Main",     NULL, interf_menu_main,     0, NULL },
    { "&Sound",    NULL, interf_menu_sound,    0, NULL },
    { "&Display",  NULL, interf_menu_display,  0, NULL },
    { "&Keyboard", NULL, interf_menu_keyboard, 0, NULL },
    { "&Parallel", NULL, interf_menu_para,     0, NULL },
    { "S&erial",   NULL, interf_menu_rs232,    0, NULL },
    { "&Tape",     NULL, interf_menu_tape,     0, NULL },
    { NULL,        NULL, NULL,                 0, NULL }
};

DIALOG interf_menu_dialog[] =
{
    /* (dialog proc)             (x)  (y)  (w)  (h) (fg)         (bg)        (key)      (f) (d1)                 (d2) (dp) */

    { d_clear_proc,              0,   0,   640, 480, MENU_BLACK, MENU_BLACK, 0,         0,  0,                   0,   NULL,                         NULL,                     NULL },
    { d_bitmap_proc,             0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, 0,         0,  0,                   0,   NULL,                         NULL,                     NULL },
    { d_menu_proc,               0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, 0,         0,  0,                   0,   interf_menu_base,             NULL,                     NULL },
    { d_slider_proc,             50,  400, 200, 20,  MENU_WHITE, MENU_BLACK, 0,         0,  255,                 0,   NULL,                         interf_menu_setbrightx,   NULL },
    { d_text_proc,               50,  425, 200, 30,  MENU_WHITE, MENU_BLACK, 0,         0,  0,                   0,   "Bright",                     NULL,                     NULL },
    { d_slider_proc,             340, 400, 200, 20,  MENU_WHITE, MENU_BLACK, 0,         0,  255,                 0,   NULL,                         interf_menu_setcontrastx, NULL },
    { d_text_proc,               340, 425, 200, 30,  MENU_WHITE, MENU_BLACK, 0,         0,  0,                   0,   "Contrast",                   NULL,                     NULL },
    { d_yield_proc,              0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, 0,         0,  0,                   0,   NULL,                         NULL,                     NULL },
    { d_keyboard_proc,           0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, ctrl('x'), 0,  0,                   0,   interf_menu_exit,             NULL,                     NULL },
    { d_keyboard_proc,           0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, ctrl('m'), 0,  0,                   0,   interf_menu_stepreturn,       NULL,                     NULL },
    { d_keyboard_proc,           0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, 0,         0,  INTERF_SCRNDUMP_KEY, 0,   interf_menu_prtscrn,          NULL,                     NULL },
    { interf_menu_xstepperenter, 0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, 0,         0,  0,                   0,   NULL,                         NULL,                     NULL },
    { NULL,                      0,   0,   0,   0,   0,          0,          0,         0,  0,                   0,   NULL,                         NULL,                     NULL }
};








char interf_menu_tapeinfiledest_str[DEFAULT_STRLEN] = "";
char interf_menu_scrnshot_str[DEFAULT_STRLEN]       = "";

long interf_menu_brightrange;
long interf_menu_contrastrange;

BITMAP *interf_menu_mouse_pointer = NULL;



void interf_menu_enter(void)
{
    UINT_16 pic_width;
    UINT_16 pic_height;
    BITMAP *screenshot_bmp;

    /*
       Make mouse pointer (unless it has already been made).
    */

    if ( interf_menu_mouse_pointer == NULL )
    {
        if ( ( interf_menu_mouse_pointer = create_bitmap(20,20) ) != NULL )
        {

            /*
               The following was machine generated based on mbee32k.bmp,
               which is a scaled, re-colored version of mbee32k.ico.
               Re-coloring was done to 3 colours: black, white and orange.
               Subsequent Colour relabelling code generation was thus:

               0x0d7 -> 0x0D1
               0x0dc -> 0x0D2
               0x0ef -> 0x0D3

               Relevant code:

                    interf_menu_mouse_pointer = load_bmp("mbee32k.bmp",interf_scrn_palette);
                { UINT_16 j;
                for ( i = 0 ; i <= 19 ; i++ ) {
                for ( j = 0 ; j <= 19 ; j++ ) {
                fprintf(stderr,"    putpixel(interf_menu_mouse_pointer,%d,%d,0x0%2x)\n",i,j,getpixel(interf_menu_mouse_pointer,i,j));
                } } }

            */

            {
                putpixel(interf_menu_mouse_pointer, 0, 0,0x000);
                putpixel(interf_menu_mouse_pointer, 0, 1,0x000);
                putpixel(interf_menu_mouse_pointer, 0, 2,0x000);
                putpixel(interf_menu_mouse_pointer, 0, 3,0x0D1);
                putpixel(interf_menu_mouse_pointer, 0, 4,0x0D1);
                putpixel(interf_menu_mouse_pointer, 0, 5,0x0D1);
                putpixel(interf_menu_mouse_pointer, 0, 6,0x0D1);
                putpixel(interf_menu_mouse_pointer, 0, 7,0x000);
                putpixel(interf_menu_mouse_pointer, 0, 8,0x000);
                putpixel(interf_menu_mouse_pointer, 0, 9,0x000);
                putpixel(interf_menu_mouse_pointer, 0,10,0x000);
                putpixel(interf_menu_mouse_pointer, 0,11,0x000);
                putpixel(interf_menu_mouse_pointer, 0,12,0x000);
                putpixel(interf_menu_mouse_pointer, 0,13,0x000);
                putpixel(interf_menu_mouse_pointer, 0,14,0x000);
                putpixel(interf_menu_mouse_pointer, 0,15,0x000);
                putpixel(interf_menu_mouse_pointer, 0,16,0x000);
                putpixel(interf_menu_mouse_pointer, 0,17,0x000);
                putpixel(interf_menu_mouse_pointer, 0,18,0x000);
                putpixel(interf_menu_mouse_pointer, 0,19,0x000);
                putpixel(interf_menu_mouse_pointer, 1, 0,0x000);
                putpixel(interf_menu_mouse_pointer, 1, 1,0x000);
                putpixel(interf_menu_mouse_pointer, 1, 2,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1, 3,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 1, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 1, 6,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1, 7,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1, 8,0x000);
                putpixel(interf_menu_mouse_pointer, 1, 9,0x000);
                putpixel(interf_menu_mouse_pointer, 1,10,0x000);
                putpixel(interf_menu_mouse_pointer, 1,11,0x000);
                putpixel(interf_menu_mouse_pointer, 1,12,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1,13,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1,14,0x0D1);
                putpixel(interf_menu_mouse_pointer, 1,15,0x000);
                putpixel(interf_menu_mouse_pointer, 1,16,0x000);
                putpixel(interf_menu_mouse_pointer, 1,17,0x000);
                putpixel(interf_menu_mouse_pointer, 1,18,0x000);
                putpixel(interf_menu_mouse_pointer, 1,19,0x000);
                putpixel(interf_menu_mouse_pointer, 2, 0,0x000);
                putpixel(interf_menu_mouse_pointer, 2, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer, 2, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2, 8,0x0D1);
                putpixel(interf_menu_mouse_pointer, 2, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer, 2,10,0x000);
                putpixel(interf_menu_mouse_pointer, 2,11,0x0D1);
                putpixel(interf_menu_mouse_pointer, 2,12,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2,13,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2,14,0x0D3);
                putpixel(interf_menu_mouse_pointer, 2,15,0x0D1);
                putpixel(interf_menu_mouse_pointer, 2,16,0x000);
                putpixel(interf_menu_mouse_pointer, 2,17,0x000);
                putpixel(interf_menu_mouse_pointer, 2,18,0x000);
                putpixel(interf_menu_mouse_pointer, 2,19,0x000);
                putpixel(interf_menu_mouse_pointer, 3, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer, 3, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer, 3, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer, 3,10,0x0D1);
                putpixel(interf_menu_mouse_pointer, 3,11,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3,12,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3,13,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3,14,0x0D3);
                putpixel(interf_menu_mouse_pointer, 3,15,0x0D1);
                putpixel(interf_menu_mouse_pointer, 3,16,0x000);
                putpixel(interf_menu_mouse_pointer, 3,17,0x000);
                putpixel(interf_menu_mouse_pointer, 3,18,0x000);
                putpixel(interf_menu_mouse_pointer, 3,19,0x000);
                putpixel(interf_menu_mouse_pointer, 4, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer, 4, 1,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer, 4,10,0x0D1);
                putpixel(interf_menu_mouse_pointer, 4,11,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4,12,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4,13,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4,14,0x0D3);
                putpixel(interf_menu_mouse_pointer, 4,15,0x0D1);
                putpixel(interf_menu_mouse_pointer, 4,16,0x000);
                putpixel(interf_menu_mouse_pointer, 4,17,0x000);
                putpixel(interf_menu_mouse_pointer, 4,18,0x000);
                putpixel(interf_menu_mouse_pointer, 4,19,0x000);
                putpixel(interf_menu_mouse_pointer, 5, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer, 5, 1,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5, 7,0x0D1);
                putpixel(interf_menu_mouse_pointer, 5, 8,0x0D1);
                putpixel(interf_menu_mouse_pointer, 5, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5,10,0x0D1);
                putpixel(interf_menu_mouse_pointer, 5,11,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5,12,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5,13,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5,14,0x0D3);
                putpixel(interf_menu_mouse_pointer, 5,15,0x0D1);
                putpixel(interf_menu_mouse_pointer, 5,16,0x0D1);
                putpixel(interf_menu_mouse_pointer, 5,17,0x000);
                putpixel(interf_menu_mouse_pointer, 5,18,0x000);
                putpixel(interf_menu_mouse_pointer, 5,19,0x000);
                putpixel(interf_menu_mouse_pointer, 6, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6, 1,0x0D3);
                putpixel(interf_menu_mouse_pointer, 6, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 6, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 6, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 6, 5,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6, 6,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 6, 8,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,10,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,11,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,12,0x0D3);
                putpixel(interf_menu_mouse_pointer, 6,13,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,14,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,15,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,16,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,17,0x0D1);
                putpixel(interf_menu_mouse_pointer, 6,18,0x000);
                putpixel(interf_menu_mouse_pointer, 6,19,0x000);
                putpixel(interf_menu_mouse_pointer, 7, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 7, 3,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7, 4,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 7, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 7, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 7, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer, 7, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer, 7,10,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,11,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,12,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,13,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,14,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,15,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,16,0x0D2);
                putpixel(interf_menu_mouse_pointer, 7,17,0x0D1);
                putpixel(interf_menu_mouse_pointer, 7,18,0x000);
                putpixel(interf_menu_mouse_pointer, 7,19,0x000);
                putpixel(interf_menu_mouse_pointer, 8, 0,0x000);
                putpixel(interf_menu_mouse_pointer, 8, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8, 2,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8,10,0x0D3);
                putpixel(interf_menu_mouse_pointer, 8,11,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8,12,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8,13,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8,14,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8,15,0x0D2);
                putpixel(interf_menu_mouse_pointer, 8,16,0x0D2);
                putpixel(interf_menu_mouse_pointer, 8,17,0x0D2);
                putpixel(interf_menu_mouse_pointer, 8,18,0x0D1);
                putpixel(interf_menu_mouse_pointer, 8,19,0x000);
                putpixel(interf_menu_mouse_pointer, 9, 0,0x000);
                putpixel(interf_menu_mouse_pointer, 9, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer, 9, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9,10,0x0D3);
                putpixel(interf_menu_mouse_pointer, 9,11,0x0D1);
                putpixel(interf_menu_mouse_pointer, 9,12,0x0D2);
                putpixel(interf_menu_mouse_pointer, 9,13,0x0D2);
                putpixel(interf_menu_mouse_pointer, 9,14,0x0D2);
                putpixel(interf_menu_mouse_pointer, 9,15,0x0D2);
                putpixel(interf_menu_mouse_pointer, 9,16,0x0D2);
                putpixel(interf_menu_mouse_pointer, 9,17,0x0D1);
                putpixel(interf_menu_mouse_pointer, 9,18,0x0D1);
                putpixel(interf_menu_mouse_pointer, 9,19,0x000);
                putpixel(interf_menu_mouse_pointer,10, 0,0x000);
                putpixel(interf_menu_mouse_pointer,10, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer,10, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer,10, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer,10,10,0x0D3);
                putpixel(interf_menu_mouse_pointer,10,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,10,12,0x0D2);
                putpixel(interf_menu_mouse_pointer,10,13,0x0D2);
                putpixel(interf_menu_mouse_pointer,10,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,10,15,0x0D1);
                putpixel(interf_menu_mouse_pointer,10,16,0x0D1);
                putpixel(interf_menu_mouse_pointer,10,17,0x0D1);
                putpixel(interf_menu_mouse_pointer,10,18,0x0D1);
                putpixel(interf_menu_mouse_pointer,10,19,0x000);
                putpixel(interf_menu_mouse_pointer,11, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer,11, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer,11, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer,11, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer,11,10,0x0D3);
                putpixel(interf_menu_mouse_pointer,11,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,13,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,15,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,16,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,17,0x0D2);
                putpixel(interf_menu_mouse_pointer,11,18,0x0D1);
                putpixel(interf_menu_mouse_pointer,11,19,0x000);
                putpixel(interf_menu_mouse_pointer,12, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer,12, 1,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer,12, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer,12,10,0x0D1);
                putpixel(interf_menu_mouse_pointer,12,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,12,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,12,13,0x0D1);
                putpixel(interf_menu_mouse_pointer,12,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,12,15,0x0D2);
                putpixel(interf_menu_mouse_pointer,12,16,0x0D2);
                putpixel(interf_menu_mouse_pointer,12,17,0x0D2);
                putpixel(interf_menu_mouse_pointer,12,18,0x0D1);
                putpixel(interf_menu_mouse_pointer,12,19,0x000);
                putpixel(interf_menu_mouse_pointer,13, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer,13, 1,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer,13, 9,0x0D3);
                putpixel(interf_menu_mouse_pointer,13,10,0x0D1);
                putpixel(interf_menu_mouse_pointer,13,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,13,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,13,13,0x0D2);
                putpixel(interf_menu_mouse_pointer,13,14,0x0D2);
                putpixel(interf_menu_mouse_pointer,13,15,0x0D2);
                putpixel(interf_menu_mouse_pointer,13,16,0x0D2);
                putpixel(interf_menu_mouse_pointer,13,17,0x0D1);
                putpixel(interf_menu_mouse_pointer,13,18,0x0D1);
                putpixel(interf_menu_mouse_pointer,13,19,0x000);
                putpixel(interf_menu_mouse_pointer,14, 0,0x0D1);
                putpixel(interf_menu_mouse_pointer,14, 1,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 8,0x0D3);
                putpixel(interf_menu_mouse_pointer,14, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer,14,10,0x0D1);
                putpixel(interf_menu_mouse_pointer,14,11,0x0D2);
                putpixel(interf_menu_mouse_pointer,14,12,0x0D2);
                putpixel(interf_menu_mouse_pointer,14,13,0x0D2);
                putpixel(interf_menu_mouse_pointer,14,14,0x0D2);
                putpixel(interf_menu_mouse_pointer,14,15,0x0D1);
                putpixel(interf_menu_mouse_pointer,14,16,0x0D1);
                putpixel(interf_menu_mouse_pointer,14,17,0x0D1);
                putpixel(interf_menu_mouse_pointer,14,18,0x0D1);
                putpixel(interf_menu_mouse_pointer,14,19,0x000);
                putpixel(interf_menu_mouse_pointer,15, 0,0x000);
                putpixel(interf_menu_mouse_pointer,15, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer,15, 2,0x0D3);
                putpixel(interf_menu_mouse_pointer,15, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,15, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,15, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,15, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,15, 7,0x0D3);
                putpixel(interf_menu_mouse_pointer,15, 8,0x0D1);
                putpixel(interf_menu_mouse_pointer,15, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,10,0x0D2);
                putpixel(interf_menu_mouse_pointer,15,11,0x0D2);
                putpixel(interf_menu_mouse_pointer,15,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,13,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,15,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,16,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,17,0x0D1);
                putpixel(interf_menu_mouse_pointer,15,18,0x000);
                putpixel(interf_menu_mouse_pointer,15,19,0x000);
                putpixel(interf_menu_mouse_pointer,16, 0,0x000);
                putpixel(interf_menu_mouse_pointer,16, 1,0x0D1);
                putpixel(interf_menu_mouse_pointer,16, 2,0x0D1);
                putpixel(interf_menu_mouse_pointer,16, 3,0x0D3);
                putpixel(interf_menu_mouse_pointer,16, 4,0x0D3);
                putpixel(interf_menu_mouse_pointer,16, 5,0x0D3);
                putpixel(interf_menu_mouse_pointer,16, 6,0x0D3);
                putpixel(interf_menu_mouse_pointer,16, 7,0x0D1);
                putpixel(interf_menu_mouse_pointer,16, 8,0x0D1);
                putpixel(interf_menu_mouse_pointer,16, 9,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,10,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,13,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,15,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,16,0x0D2);
                putpixel(interf_menu_mouse_pointer,16,17,0x0D1);
                putpixel(interf_menu_mouse_pointer,16,18,0x000);
                putpixel(interf_menu_mouse_pointer,16,19,0x000);
                putpixel(interf_menu_mouse_pointer,17, 0,0x000);
                putpixel(interf_menu_mouse_pointer,17, 1,0x000);
                putpixel(interf_menu_mouse_pointer,17, 2,0x000);
                putpixel(interf_menu_mouse_pointer,17, 3,0x0D1);
                putpixel(interf_menu_mouse_pointer,17, 4,0x0D1);
                putpixel(interf_menu_mouse_pointer,17, 5,0x0D1);
                putpixel(interf_menu_mouse_pointer,17, 6,0x0D1);
                putpixel(interf_menu_mouse_pointer,17, 7,0x000);
                putpixel(interf_menu_mouse_pointer,17, 8,0x000);
                putpixel(interf_menu_mouse_pointer,17, 9,0x000);
                putpixel(interf_menu_mouse_pointer,17,10,0x0D1);
                putpixel(interf_menu_mouse_pointer,17,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,17,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,17,13,0x0D2);
                putpixel(interf_menu_mouse_pointer,17,14,0x0D2);
                putpixel(interf_menu_mouse_pointer,17,15,0x0D2);
                putpixel(interf_menu_mouse_pointer,17,16,0x0D1);
                putpixel(interf_menu_mouse_pointer,17,17,0x000);
                putpixel(interf_menu_mouse_pointer,17,18,0x000);
                putpixel(interf_menu_mouse_pointer,17,19,0x000);
                putpixel(interf_menu_mouse_pointer,18, 0,0x000);
                putpixel(interf_menu_mouse_pointer,18, 1,0x000);
                putpixel(interf_menu_mouse_pointer,18, 2,0x000);
                putpixel(interf_menu_mouse_pointer,18, 3,0x000);
                putpixel(interf_menu_mouse_pointer,18, 4,0x000);
                putpixel(interf_menu_mouse_pointer,18, 5,0x000);
                putpixel(interf_menu_mouse_pointer,18, 6,0x000);
                putpixel(interf_menu_mouse_pointer,18, 7,0x000);
                putpixel(interf_menu_mouse_pointer,18, 8,0x000);
                putpixel(interf_menu_mouse_pointer,18, 9,0x000);
                putpixel(interf_menu_mouse_pointer,18,10,0x000);
                putpixel(interf_menu_mouse_pointer,18,11,0x0D1);
                putpixel(interf_menu_mouse_pointer,18,12,0x0D2);
                putpixel(interf_menu_mouse_pointer,18,13,0x0D2);
                putpixel(interf_menu_mouse_pointer,18,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,18,15,0x0D1);
                putpixel(interf_menu_mouse_pointer,18,16,0x000);
                putpixel(interf_menu_mouse_pointer,18,17,0x000);
                putpixel(interf_menu_mouse_pointer,18,18,0x000);
                putpixel(interf_menu_mouse_pointer,18,19,0x000);
                putpixel(interf_menu_mouse_pointer,19, 0,0x000);
                putpixel(interf_menu_mouse_pointer,19, 1,0x000);
                putpixel(interf_menu_mouse_pointer,19, 2,0x000);
                putpixel(interf_menu_mouse_pointer,19, 3,0x000);
                putpixel(interf_menu_mouse_pointer,19, 4,0x000);
                putpixel(interf_menu_mouse_pointer,19, 5,0x000);
                putpixel(interf_menu_mouse_pointer,19, 6,0x000);
                putpixel(interf_menu_mouse_pointer,19, 7,0x000);
                putpixel(interf_menu_mouse_pointer,19, 8,0x000);
                putpixel(interf_menu_mouse_pointer,19, 9,0x000);
                putpixel(interf_menu_mouse_pointer,19,10,0x000);
                putpixel(interf_menu_mouse_pointer,19,11,0x000);
                putpixel(interf_menu_mouse_pointer,19,12,0x0D1);
                putpixel(interf_menu_mouse_pointer,19,13,0x0D1);
                putpixel(interf_menu_mouse_pointer,19,14,0x0D1);
                putpixel(interf_menu_mouse_pointer,19,15,0x000);
                putpixel(interf_menu_mouse_pointer,19,16,0x000);
                putpixel(interf_menu_mouse_pointer,19,17,0x000);
                putpixel(interf_menu_mouse_pointer,19,18,0x000);
                putpixel(interf_menu_mouse_pointer,19,19,0x000);
            }
        }
    }

    /*
       Stop any current notes
    */

    interf_snd_nosound();

    /*
       Set keyboard rates and clear the buffer.  For some reason this doesn't
       work in windows.
    */

    set_keyboard_rate(700,100);
    clear_keybuf();

    /*
       Grab the current microbee screen for menu background.  Trim to the
       default MENU size
    */

    if ( ( pic_width  = (interf_scrn_rsource-interf_scrn_lsource) ) > 640 ) { pic_width  = 640; }
    if ( ( pic_height = (interf_scrn_bsource-interf_scrn_tsource) ) > 480 ) { pic_height = 480; }

    screenshot_bmp = create_bitmap(interf_scrn_rsource-interf_scrn_lsource,interf_scrn_bsource-interf_scrn_tsource);
    blit(interf_scrn_bee_screen,screenshot_bmp,interf_scrn_lsource,interf_scrn_tsource,0,0,interf_scrn_rsource-interf_scrn_lsource,interf_scrn_bsource-interf_scrn_tsource);

    (interf_menu_dialog[1]).dp = screenshot_bmp;
    (interf_menu_dialog[1]).w  = pic_width;
    (interf_menu_dialog[1]).h  = pic_height;
    (interf_menu_dialog[1]).x  = (640-pic_width)/2;
    (interf_menu_dialog[1]).y  = (480-pic_height)/2;

    /*
       Set screen mode to windowed, 640x480 (unless already in this mode).
       Fallback to fullscreen or autodetect if windowed not possible.
    */

    if ( ( ( ( interf_scrn_video_mode != 0 ) || interf_scrn_pref_fullscrn )) && !interf_scrn_stepmode )
    {
        if ( set_gfx_mode(GFX_AUTODETECT_WINDOWED,640,480,0,0) )
        {
            if ( set_gfx_mode(GFX_AUTODETECT_FULLSCREEN,640,480,0,0) )
            {
                set_gfx_mode(GFX_AUTODETECT,640,480,0,0);
            }
        }
    }

    /*
       Set the palette.
    */

    set_palette(interf_scrn_palette);

    /*
       Clear the screen.
    */

    rectfill(screen,0,0,640-1,480-1,MENU_BLACK);

    /*
       Set GUI colours.
    */

    gui_fg_color = MENU_WHITE;
    gui_bg_color = MENU_BLACK;
    gui_mg_color = MENU_GREY;

    /*
       Set bright/contrast local variable pointers.
    */

    interf_menu_brightrange   = interf_scrn_bright;
    interf_menu_contrastrange = interf_scrn_contrast;

    (interf_menu_dialog[3]).d2 = interf_menu_brightrange;
    (interf_menu_dialog[5]).d2 = interf_menu_contrastrange;

    /*
       Set the mouse pointer
    */

    set_mouse_sprite(NULL);

    if ( interf_menu_mouse_pointer != NULL )
    {
        set_mouse_sprite(interf_menu_mouse_pointer);
    }

    /*
       Fix menu marks for current settings (to allow for key shortcuts)
    */

    interf_menu_update_menu_marks();

    /*
       Step mode stuff.
    */

    if ( interf_scrn_stepmode == 1 )
    {
        interf_scrn_stepmode = 0;
    }

    /*
       Allegro does the menu stuff now.
    */

    do_dialog(interf_menu_dialog,-1);

    /*
       Kill the local copy of the screen.
    */

    destroy_bitmap((interf_menu_dialog[1]).dp);

    /*
       Put keyboard repeat rates back as before, clear keyboard buffer.
    */

    set_keyboard_rate(0,0);
    clear_keybuf();

    /*
       Attempt to enter the graphics mode for emulation.
    */

    if ( ( ( ( interf_scrn_video_mode != 0 ) || interf_scrn_pref_fullscrn ) ) && !interf_scrn_stepmode )
    {
        if ( interf_scrn_set_gfxmode() )
        {
            /*
               Unable to set *any* graphics mode, so put in step mode (so
               that it won't just go into nothing) and tell the emulator
               to exit asap.
            */

            interf_scrn_stepmode = 1;

            INTERF_CTRL_EXIT(interf_indir_nonvol);
        }

        else
        {
            set_palette(interf_scrn_palette);
        }
    }

    /*
       If in step mode, do some stuff.
    */

    if ( !interf_scrn_stepmode )
    {
        interf_scrn_blank_screen();
        interf_scrn_refresh_screen();
    }

    return;
}

void interf_menu_update_menu_marks(void)
{
    {
        interf_menu_para_stra[0] = ' ';
        interf_menu_para_strb[0] = ' ';
        interf_menu_para_strc[0] = ' ';
        interf_menu_para_strd[0] = ' ';
        interf_menu_para_stre[0] = ' ';
        interf_menu_para_strf[0] = ' ';

        switch ( interf_para_mode )
        {
            case 0:  { interf_menu_para_stra[0] = '-'; break; }
            case 5:  { interf_menu_para_strb[0] = '-'; break; }
            case 1:  { interf_menu_para_strc[0] = '-'; break; }
            case 2:  { interf_menu_para_strd[0] = '-'; break; }
            case 3:  { interf_menu_para_stre[0] = '-'; break; }
            default: { interf_menu_para_strf[0] = '-'; break; }
        }
    }

    {
        interf_menu_keyboard_stra[0] = ' ';
        interf_menu_keyboard_strb[0] = ' ';

        if ( interf_key_sourcefp == NULL ) { interf_menu_keyboard_stra[0] = '-'; }
        else                               { interf_menu_keyboard_strb[0] = '-'; }
    }

    {
        interf_menu_sound_stra[0] = ' ';
        interf_menu_sound_strb[0] = ' ';

        switch ( interf_snd_sndon )
        {
            case 0:  { interf_menu_sound_stra[0] = '-'; break; }
            default: { interf_menu_sound_strb[0] = '-'; break; }
        }
    }

    {
        interf_menu_tape_stra[0] = ' ';
        interf_menu_tape_strb[0] = ' ';
        interf_menu_tape_strc[0] = ' ';

        interf_menu_tape_strd[0] = ' ';
        interf_menu_tape_stre[0] = ' ';

        switch ( interf_tape_out_mode )
        {
            case 1:  { interf_menu_tape_stra[0] = '-'; break; }
            case 2:  { interf_menu_tape_strb[0] = '-'; break; }
            default: { interf_menu_tape_strc[0] = '-'; break; }
        }

        switch ( interf_tape_autosave_mode )
        {
            case 0:  { interf_menu_tape_strd[0] = '-'; break; }
            default: { interf_menu_tape_stre[0] = '-'; break; }
        }

        if ( interf_tape_in_type )
        {
            (interf_menu_tape[9 ]).flags = D_DISABLED;
            (interf_menu_tape[10]).flags = D_DISABLED;
            (interf_menu_tape[11]).flags = D_DISABLED;
            (interf_menu_tape[12]).flags = D_DISABLED;

            (interf_menu_tape[14]).flags = 0;
        }

        else
        {
            (interf_menu_tape[9 ]).flags = 0;
            (interf_menu_tape[10]).flags = D_DISABLED;
            (interf_menu_tape[11]).flags = 0;
            (interf_menu_tape[12]).flags = 0;

            (interf_menu_tape[14]).flags = D_DISABLED;
        }
    }

    {
        interf_menu_main_clock_stra[0] = ' ';
        interf_menu_main_clock_strb[0] = ' ';

        if ( interf_speed_emu_on ) { interf_menu_main_clock_stra[0] = '-'; }
        else                       { interf_menu_main_clock_strb[0] = '-'; }
    }

    {
        if ( !interf_scrn_monitor_type )
        {
            interf_menu_display_stra[0] = '-';

            interf_menu_display_strb[0] = ' ';
            interf_menu_display_strc[0] = ' ';
            interf_menu_display_strd[0] = ' ';
            interf_menu_display_stre[0] = ' ';
            interf_menu_display_strf[0] = ' ';
            interf_menu_display_strg[0] = ' ';
        }

        else
        {
            interf_menu_display_stra[0] = ' ';

            interf_menu_display_strb[0] = ' ';
            interf_menu_display_strc[0] = ' ';
            interf_menu_display_strd[0] = ' ';
            interf_menu_display_stre[0] = ' ';
            interf_menu_display_strf[0] = ' ';
            interf_menu_display_strg[0] = ' ';

            switch ( interf_scrn_monitor_type )
            {
                case 0:  { interf_menu_display_stra[0] = '-'; break; }
                case 1:  { interf_menu_display_strb[0] = '-'; break; }
                case 2:  { interf_menu_display_strc[0] = '-'; break; }
                case 3:  { interf_menu_display_strd[0] = '-'; break; }
                case 4:  { interf_menu_display_stre[0] = '-'; break; }
                case 5:  { interf_menu_display_strf[0] = '-'; break; }
                default: { interf_menu_display_strg[0] = '-'; break; }
            }
        }

        interf_menu_display_resol_stra[0] = ' ';
        interf_menu_display_resol_strb[0] = ' ';
        interf_menu_display_resol_strc[0] = ' ';
        interf_menu_display_resol_strd[0] = ' ';
        interf_menu_display_resol_stre[0] = ' ';
        interf_menu_display_resol_strf[0] = ' ';
        interf_menu_display_resol_strg[0] = ' ';
        interf_menu_display_resol_strh[0] = ' ';
        interf_menu_display_resol_stri[0] = ' ';
        interf_menu_display_resol_strj[0] = ' ';

        switch ( interf_scrn_video_mode )
        {
            case 0:  { interf_menu_display_resol_stra[0] = '-'; break; }
            case 1:  { interf_menu_display_resol_strb[0] = '-'; break; }
            case 2:  { interf_menu_display_resol_strc[0] = '-'; break; }
            case 3:  { interf_menu_display_resol_strd[0] = '-'; break; }
            case 4:  { interf_menu_display_resol_stre[0] = '-'; break; }
            case 5:  { interf_menu_display_resol_strf[0] = '-'; break; }
            case 6:  { interf_menu_display_resol_strg[0] = '-'; break; }
            case 7:  { interf_menu_display_resol_strh[0] = '-'; break; }
            case 8:  { interf_menu_display_resol_stri[0] = '-'; break; }
            default: { interf_menu_display_resol_strj[0] = '-'; break; }
        }

        interf_menu_display_resol_strx[0] = ' ';
        interf_menu_display_resol_stry[0] = ' ';

        switch ( interf_scrn_pref_fullscrn )
        {
            case 0:  { interf_menu_display_resol_strx[0] = '-'; break; }
            default: { interf_menu_display_resol_stry[0] = '-'; break; }
        }
    }

    return;
}


int interf_menu_setbrightx(void *dp3, int d2)
{
    interf_menu_brightrange = d2;

    interf_scrn_set_bright(interf_menu_brightrange);

    set_palette(interf_scrn_palette);

    return D_O_K;

    dp3 = NULL;
}

int interf_menu_setcontrastx(void *dp3, int d2)
{
    interf_menu_contrastrange = d2;

    interf_scrn_set_contrast(interf_menu_contrastrange);

    set_palette(interf_scrn_palette);

    return D_O_K;

    dp3 = NULL;
}

int interf_menu_xstepperenter(int msg, DIALOG *d, int c)
{
    if ( interf_scrn_stepmode == 2 )
    {
        return D_EXIT;
    }

    return D_O_K;

    msg = c;
    d = NULL;
}





int interf_menu_cpuclkon(void)
{
    INTERF_CTRL_SPEEDCTRL_ON(interf_indir_nonvol);

    interf_speed_emu_on = 1;

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_cpuclkoff(void)
{
    INTERF_CTRL_SPEEDCTRL_OFF(interf_indir_nonvol);

    interf_speed_emu_on = 0;

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_stepreturn(void)
{
    interf_scrn_stepmode = 1;

    return D_CLOSE;
}

int interf_menu_prtscrn(void)
{
    BITMAP  *zzzbmp;
    PALETTE  zzzpal;

    get_palette(zzzpal);
    zzzbmp = create_sub_bitmap(screen,0,0,640,480);
    save_bitmap(DEFAULT_SCREENDUMP_FILE,zzzbmp,zzzpal);
    destroy_bitmap(zzzbmp);

    return D_O_K;
}

int interf_menu_return(void)
{
    return D_CLOSE;
}

int interf_menu_exit(void)
{
    interf_scrn_stepmode = 1;

    INTERF_CTRL_EXIT(interf_indir_nonvol);

    return D_CLOSE;
}

int interf_menu_scrnshot(void)
{
    if ( file_select_ex("Select Screenshot File (bmp)",interf_menu_scrnshot_str,NULL,300,0,0) )
    {
        if ( interf_scrn_screenshot(interf_menu_scrnshot_str) )
        {
            alert("","Unable to Open File.","","&OK",NULL,'o',0);
        }
    }

    return D_O_K;
}

int interf_menu_soundoff(void)
{
    interf_snd_turn_snd_off();

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_soundon(void)
{
    interf_snd_turn_snd_on();

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmodewindow(void)
{
    interf_scrn_pref_fullscrn = 0;

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmodefullscrn(void)
{
    interf_scrn_pref_fullscrn = 1;

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode0(void)
{
    if ( interf_scrn_video_mode != 0 )
    {
        if ( interf_scrn_set_vidmode(0) )
        {
            alert("","Video mode 0 not available.","","&OK",NULL,'o',0);
        }

        interf_scrn_correct_scrnsize();
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode1(void)
{
    if ( interf_scrn_video_mode != 1 )
    {
        if ( alert("Warning:","800x600 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(1) )
            {
                alert("","Video mode 1 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode2(void)
{
    if ( interf_scrn_video_mode != 2 )
    {
        if ( alert("Warning:","1024x768 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(2) )
            {
                alert("","Video mode 2 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode3(void)
{
    if ( interf_scrn_video_mode != 3 )
    {
        if ( alert("Warning:","1280x1024 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(3) )
            {
                alert("","Video mode 3 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode4(void)
{
    if ( interf_scrn_video_mode != 4 )
    {
        if ( alert("Warning:","800x600 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(4) )
            {
                alert("","Video mode 4 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode5(void)
{
    if ( interf_scrn_video_mode != 5 )
    {
        if ( alert("Warning:","1024x768 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(5) )
            {
                alert("","Video mode 5 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode6(void)
{
    if ( interf_scrn_video_mode != 6 )
    {
        if ( alert("Warning:","1280x1024 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(6) )
            {
                alert("","Video mode 6 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode7(void)
{
    if ( interf_scrn_video_mode != 7 )
    {
        if ( alert("Warning:","800x600 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(7) )
            {
                alert("","Video mode 7 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode8(void)
{
    if ( interf_scrn_video_mode != 8 )
    {
        if ( alert("Warning:","1024x768 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(8) )
            {
                alert("","Video mode 8 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setvidmode9(void)
{
    if ( interf_scrn_video_mode != 9 )
    {
        if ( alert("Warning:","1280x1024 may cause some systems to hang.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 1 )
        {
            if ( interf_scrn_set_vidmode(9) )
            {
                alert("","Video mode 9 not available.","","&OK",NULL,'o',0);
            }

            interf_scrn_correct_scrnsize();
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setdispcolour(void)
{
    if ( interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }
/*phantomx*/
    interf_scrn_monitor_type = 0;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_WHITE;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_setdispgreen(void)
{
    if ( !interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }

    interf_scrn_monitor_type = 1;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_GREEN;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_setdispamber(void)
{
    if ( !interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }

    interf_scrn_monitor_type = 2;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_AMBER;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_setdispbw(void)
{
    if ( !interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }

    interf_scrn_monitor_type = 3;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_WHITE;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_BLACK;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_setdispinvgreen(void)
{
    if ( !interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }

    interf_scrn_monitor_type = 4;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_BLACK;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_GREEN;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_setdispinvamber(void)
{
    if ( !interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }

    interf_scrn_monitor_type = 5;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_BLACK;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_AMBER;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_setdispinvbw(void)
{
    if ( !interf_scrn_monitor_type )
    {
        if ( alert("Note:","Until the next RESET-ESC, Basic may behave oddly.","Continue anyhow?", "&Yes", "&No", 'y', 'n') == 2 )
        {
            return D_O_K;
        }
    }

    interf_scrn_monitor_type = 6;

    interf_scrn_mono_forecolour = INTERF_SCRN_COLOUR_BLACK;
    interf_scrn_mono_backcolour = INTERF_SCRN_COLOUR_WHITE;

    interf_menu_update_menu_marks();

    INTERF_SCRN_RFSH(interf_indir_nonvol);

    return D_O_K;
}

int interf_menu_keyboardnormal(void)
{
    interf_key_closefile();

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_keyboardfile(void)
{
    if ( file_select_ex("Keystroke Source file",interf_key_sourcefilename,NULL,300,0,0) )
    {
        interf_key_setfile(interf_key_sourcefilename);

        if ( interf_key_sourcefp == NULL )
        {
            alert("Error:","Unable to open file.",".","&OK",NULL,'o',0);
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setparamode0(void)
{
    if ( interf_para_set_mode0() )
    {
        alert("Error:","Parallel port access blocked.","Reverting to unconnected.","&OK",NULL,'o',0);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setparamode1(void)
{
    PC_FILE *tempfp;

    if ( file_select_ex("Parallel Port Destination file",interf_para_filename_dest,NULL,300,0,0) )
    {
        if ( ( tempfp = pc_fopen(interf_para_filename_dest,"rt") ) != NULL )
        {
            pc_fclose(tempfp);

            if ( alert("File already exists.","Overwrite anyhow?","","&OK","&Cancel",'o','c') == 2 )
            {
                interf_para_set_mode4();

                goto exit_point;
            }
        }

        if ( interf_para_set_mode1(interf_para_filename_dest) )
        {
            alert("Error:","Couldn't open file.","Reverting to unconnected.","&OK",NULL,'o',0);
        }
    }

    exit_point:

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setparamode2(void)
{
    if ( interf_para_set_mode2() )
    {
        alert("Error:","Unknown parallel port error.","Reverting to unconnected.","&OK",NULL,'o',0);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setparamode3(void)
{
    if ( file_select_ex("Parallel Port Source File",interf_para_filename_src,NULL,300,0,0) )
    {
/*FIXME - temporarily removed       z80pio_data_rd_A(beepara_pio_state); this seems to be necessary*/

        if ( interf_para_set_mode3(interf_para_filename_src) )
        {
            alert("Error:","Couldn't open file.","Reverting to unconnected.","&OK",NULL,'o',0);
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setparamode4(void)
{
    if ( interf_para_set_mode4() )
    {
        alert("Wow!","An impossible error has occured.","Hmmmmmmmmm.","&OK",NULL,'o',0);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_setparamode5(void)
{
    if ( interf_para_set_mode5() )
    {
        alert("Error:","Unable to access BIOS.","Reverting to unconnected.","&OK",NULL,'o',0);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_settapemode1(void)
{
    PC_FILE *tempfp;

    if ( file_select_ex("Tape Port Destination File",interf_tape_out_file_tapedest,NULL,300,0,0) )
    {
        if ( ( tempfp = pc_fopen(interf_tape_out_file_tapedest,"rt") ) != NULL )
        {
            pc_fclose(tempfp);

            if ( alert("File already exists.","Overwrite anyhow?","","&OK","&Cancel",'o','c') == 2 )
            {
                interf_tape_out_set_mode_3();

                goto exit_point;
            }
        }

        if ( interf_tape_out_set_mode_1(interf_tape_out_file_tapedest) )
        {
            alert("Error:","Couldn't open file.","Reverting to unconnected.","&OK",NULL,'o',0);
        }
    }

    exit_point:

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_settapemode3(void)
{
    if ( interf_tape_out_set_mode_3() )
    {
        alert("Error:","WOW - an impossible error!","Let's do that again!","&OK",NULL,'o',0);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_tapeautosaveon(void)
{
    interf_tape_autosave_mode = 1;

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_tapeautosaveoff(void)
{
    interf_tape_autosave_mode = 0;

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_tapesavedir(void)
{
    if ( file_select_ex("Tape Port Destination Directory",interf_tape_out_autosave_dir,"/d",300,0,0) )
    {
        strcpy(interf_tape_out_autosave_dir,"./");
    }

    if ( interf_tape_out_autosave_dir[strlen(interf_tape_out_autosave_dir)-1] != '/' )
    {
        interf_tape_out_autosave_dir[strlen(interf_tape_out_autosave_dir)+1] = '\0';
        interf_tape_out_autosave_dir[strlen(interf_tape_out_autosave_dir)]   = '/';
    }

    return D_O_K;
}

int interf_menu_tapeinpipe(void)
{
    char *errdesc;

    if ( file_select_ex("Source file",interf_menu_tapeinfiledest_str,NULL,300,0,0) )
    {
        /*
           Get pipe speed.
        */

        if ( alert("","Select speed.","","&300 baud","&1200 baud",'3','1') == 1 )
        {
            interf_tape_in_tapesped = 0;
        }

        else
        {
            interf_tape_in_tapesped = 1;
        }

        if ( ( errdesc = interf_tape_in_pipe_data(interf_menu_tapeinfiledest_str,interf_tape_in_tapesped) ) != NULL )
        {
            alert("Error:",errdesc,"","&OK",NULL,'o',0);

            return D_O_K;
        }
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_tapeinloaddata(void)
{
    UINT_32 filesize;
    char *xfilename;
    char *yfilename;
    int hasguessext;

    int head_la;
    int head_ae;
    char str_head_la[16] = "0x00100";
    char str_head_ae[16] = "0x00100";
    char str_head_ft[16] = "M";

    UINT_8 head_ft;
    UINT_8 head_fsl;
    UINT_8 head_fsh;
    UINT_8 head_lal;
    UINT_8 head_lah;
    UINT_8 head_ael;
    UINT_8 head_aeh;
    UINT_8 head_sp;
    UINT_8 head_ac;
    UINT_8 head_nu;

    PC_FILE *interf_tape_in_src__fp;

    DIALOG beetape_detail_dialog[] =
    {
        /* (dialog proc)     (x)  (y)  (w)  (h)  (fg)        (bg)     (key) (f)  (d1)(d2)(dp) */

        { d_shadow_box_proc, 50,  75,  450, 300, MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, NULL,                      NULL, NULL },
        { d_button_proc,     100, 310, 150, 40,  MENU_WHITE, MENU_BLACK, 0, D_EXIT,0, 0, "OK",                      NULL, NULL },
        { d_button_proc,     300, 310, 150, 40,  MENU_WHITE, MENU_BLACK, 0, D_EXIT,0, 0, "Cancel",                  NULL, NULL },
        { d_ctext_proc,      250, 100, 400, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, "Tape Header Details.",    NULL, NULL },
        { d_text_proc,       100, 130, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, "Filetag:",                NULL, NULL },
        { d_text_proc,       100, 160, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, "Filetype:",               NULL, NULL },
        { d_text_proc,       100, 190, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, "Load address:",           NULL, NULL },
        { d_text_proc,       100, 220, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, "Execute address:",        NULL, NULL },
        { d_edit_proc,       300, 130, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     6, 1, NULL,                      NULL, NULL },
        { d_edit_proc,       300, 160, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     1, 1, str_head_ft,               NULL, NULL },
        { d_edit_proc,       300, 190, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     7, 1, str_head_la,               NULL, NULL },
        { d_edit_proc,       300, 220, 150, 40,  MENU_WHITE, MENU_BLACK, 0, 0,     7, 1, str_head_ae,               NULL, NULL },
        { d_check_proc,      100, 250, 300, 15,  MENU_WHITE, MENU_BLACK, 0, 0,     1, 0, " Fast load (1200 baud).", NULL, NULL },
        { d_check_proc,      100, 280, 300, 15,  MENU_WHITE, MENU_BLACK, 0, 0,     1, 0, " Autexecute mode.",       NULL, NULL },
        { d_yield_proc,      0,   0,   0,   0,   MENU_WHITE, MENU_BLACK, 0, 0,     0, 0, NULL,                      NULL, NULL },
        { NULL,              0,   0,   0,   0,   0,          0,          0, 0,     0, 0, NULL,                      NULL, NULL }
    };

    if ( file_select_ex("Source file",interf_menu_tapeinfiledest_str,NULL,300,0,0) )
    {
        if ( ( interf_tape_in_src__fp = pc_fopen(interf_menu_tapeinfiledest_str,"rb") ) == NULL )
        {
            alert("Error:","Couldn't open file.","","&OK",NULL,'o',0);

            return D_O_K;
        }

        /*
           Count the bytes in the file.
        */

        filesize = 0;

        while ( !pc_feof(interf_tape_in_src__fp) )
        {
            pc_fgetc(interf_tape_in_src__fp);

            filesize++;
        }

        pc_fclose(interf_tape_in_src__fp);

        if ( filesize <= 0 )
        {
            alert("Error:","No data in file.","","&OK",NULL,'o',0);

            return D_O_K;
        }

        if ( ( interf_tape_in_src__fp = pc_fopen(interf_menu_tapeinfiledest_str,"rb") ) == NULL )
        {
            alert("Error:","Couldn't re-open file.","","&OK",NULL,'o',0);

            return D_O_K;
        }


        /*
           Filename (size characters)
        */

        yfilename = get_filename(interf_menu_tapeinfiledest_str);

        /*
           Try to guess the type, and relevant header info
        */

        hasguessext = 0;

        xfilename = get_extension(interf_menu_tapeinfiledest_str);

        if ( strlen(xfilename) == 3 )
        {
            if ( ( ( xfilename[0] == 'm' ) || ( xfilename[0] == 'M' ) ) &&
                 ( ( xfilename[1] == 'w' ) || ( xfilename[1] == 'W' ) ) &&
                 ( ( xfilename[2] == 'b' ) || ( xfilename[2] == 'B' ) )    )
            {
                hasguessext = 1;
            }

            if ( ( ( xfilename[0] == 'c' ) || ( xfilename[0] == 'C' ) ) &&
                 ( ( xfilename[1] == 'o' ) || ( xfilename[1] == 'O' ) ) &&
                 ( ( xfilename[2] == 'm' ) || ( xfilename[2] == 'M' ) )    )
            {
                hasguessext = 2;
            }

            if ( ( ( xfilename[0] == 'c' ) || ( xfilename[0] == 'C' ) ) &&
                 ( ( xfilename[1] == 'p' ) || ( xfilename[1] == 'P' ) ) &&
                 ( ( xfilename[2] == 'm' ) || ( xfilename[2] == 'M' ) )    )
            {
                hasguessext = 2;
            }
        }

        else if ( strlen(xfilename) == 1 )
        {
            if ( ( xfilename[0] == 'B' ) || ( xfilename[0] == 'b' ) )
            {
                hasguessext = 1;
            }

            else if ( ( xfilename[0] == 'M' ) || ( xfilename[0] == 'm' ) )
            {
                hasguessext = 2;
            }
        }

        /*
           Trim the filename.
        */

        if ( strlen(yfilename) > 6 )
        {
            yfilename[6] = '\0';
        }

        switch ( hasguessext )
        {
            case 1:
            {
                /*
                   Use default MWB header
                */

                head_ft  = 'B';
                head_fsl = filesize & 0x0ff;
                head_fsh = ( filesize >> 8 ) & 0x0ff;
                head_lal = 0x0c0;
                head_lah = 0x008;
                head_ael = 0x000;
                head_aeh = 0x000;
                head_sp  = 0x001;
                head_ac  = 0x000;
                head_nu  = 0x000;

                head_la = 0x008c0;
                head_ae = 0x00000;

                if ( alert("File appears to be MWB format.","Use automatic header?","","&Yes","&No",'y','n') == 2 )
                {
                    goto unknown_type;
                }

                /*
                   Get load speed.
                */

                if ( alert("","Select speed.","","&300 baud","&1200 baud",'3','1') == 1 )
                {
                    head_sp = 0;
                }

                else
                {
                    head_sp = 1;
                }

                break;
            }

            case 2:
            {
                /*
                   Use default COM header
                */

                head_ft  = 'M';
                head_fsl = filesize & 0x0ff;
                head_fsh = ( filesize >> 8 ) & 0x0ff;
                head_lal = 0x000;
                head_lah = 0x001;
                head_ael = 0x000;
                head_aeh = 0x001;
                head_sp  = 0x001;
                head_ac  = 0x0ff;
                head_nu  = 0x000;

                head_la = 0x00100;
                head_ae = 0x00100;

                if ( alert("File appears to be COM format.","Use automatic header?","","&Yes","&No",'y','n') == 2 )
                {
                    goto unknown_type;
                }

                /*
                   Get load speed.
                */

                if ( alert("","Select speed.","","&300 baud","&1200 baud",'3','1') == 1 )
                {
                    head_sp = 0;
                }

                else
                {
                    head_sp = 1;
                }

                break;
            }

            default:
            {
                /*
                   Type unknown: By default, treat as a COM file
                */

                head_ft  = 'M';
                head_fsl = filesize & 0x0ff;
                head_fsh = ( filesize >> 8 ) & 0x0ff;
                head_lal = 0x000;
                head_lah = 0x001;
                head_ael = 0x000;
                head_aeh = 0x001;
                head_sp  = 0x001;
                head_ac  = 0x0ff;
                head_nu  = 0x000;

                head_la = 0x00100;
                head_ae = 0x00100;

                unknown_type:

                if ( head_sp )
                {
                    (beetape_detail_dialog[12]).flags = D_SELECTED;
                }

                if ( head_ac )
                {
                    (beetape_detail_dialog[13]).flags = D_SELECTED;
                }

                str_head_la[0] = '0';
                str_head_la[1] = 'x';
                str_head_la[2] = '0';
                str_head_la[3] = ( ( head_la >> 12 ) & 0x00f ) + '0'; if ( str_head_la[3] > '9' ) { str_head_la[3] += ('a'-10-'0'); }
                str_head_la[4] = ( ( head_la >>  8 ) & 0x00f ) + '0'; if ( str_head_la[4] > '9' ) { str_head_la[4] += ('a'-10-'0'); }
                str_head_la[5] = ( ( head_la >>  4 ) & 0x00f ) + '0'; if ( str_head_la[5] > '9' ) { str_head_la[5] += ('a'-10-'0'); }
                str_head_la[6] = (   head_la         & 0x00f ) + '0'; if ( str_head_la[6] > '9' ) { str_head_la[6] += ('a'-10-'0'); }
                str_head_la[7] = '\0';

                str_head_ae[0] = '0';
                str_head_ae[1] = 'x';
                str_head_ae[2] = '0';
                str_head_ae[3] = ( ( head_ae >> 12 ) & 0x00f ) + '0'; if ( str_head_ae[3] > '9' ) { str_head_ae[3] += ('a'-10-'0'); }
                str_head_ae[4] = ( ( head_ae >>  8 ) & 0x00f ) + '0'; if ( str_head_ae[4] > '9' ) { str_head_ae[4] += ('a'-10-'0'); }
                str_head_ae[5] = ( ( head_ae >>  4 ) & 0x00f ) + '0'; if ( str_head_ae[5] > '9' ) { str_head_ae[5] += ('a'-10-'0'); }
                str_head_ae[6] = (   head_ae         & 0x00f ) + '0'; if ( str_head_ae[6] > '9' ) { str_head_ae[6] += ('a'-10-'0'); }
                str_head_ae[7] = '\0';

                str_head_ft[0] = head_ft;
                str_head_ft[1] = '\0';

                (beetape_detail_dialog[8]).dp  = yfilename;

                if ( popup_dialog(beetape_detail_dialog,1) == 1 )
                {
                    sscanf(str_head_la,"%i",&head_la);
                    sscanf(str_head_ae,"%i",&head_ae);

                    head_ft = str_head_ft[0];

                    head_lal = (     head_la                    & 0x000ff );
                    head_lah = ( ( ( head_la & 0x0ff00 ) >> 8 ) & 0x000ff );

                    head_ael = (     head_ae                    & 0x000ff );
                    head_aeh = ( ( ( head_ae & 0x0ff00 ) >> 8 ) & 0x000ff );

                    head_fsl = (     filesize                    & 0x000ff );
                    head_fsh = ( ( ( filesize & 0x0ff00 ) >> 8 ) & 0x000ff );

                    if ( (beetape_detail_dialog[12]).flags & D_SELECTED )
                    {
                        head_sp = 0x001;
                    }

                    else
                    {
                        head_sp = 0x000;
                    }

                    if ( (beetape_detail_dialog[13]).flags & D_SELECTED )
                    {
                        head_ac = 0x0ff;
                    }

                    else
                    {
                        head_ac = 0x000;
                    }

                    head_nu = 0x000;
                }

                else
                {
                    pc_fclose(interf_tape_in_src__fp);

                    return D_O_K;
                }

                break;
            }
        }

        if ( interf_tape_in_convert_to_memload(head_ft,head_fsl,head_fsh,head_lal,head_lah,head_ael,head_aeh,head_sp,head_ac,head_nu,yfilename,filesize,interf_tape_in_src__fp) )
        {
            alert("Error:","Out of memory.","","&OK",NULL,'o',0);
        }

        pc_fclose(interf_tape_in_src__fp);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_tapeinloadmtd(void)
{
    char savename[CONFIG_BUFFER_LEN+1];
    char filetag[CONFIG_BUFFER_LEN+1];
    char buffera[CONFIG_BUFFER_LEN+1];
    char bufferb[CONFIG_BUFFER_LEN+1];
    int k,l;
    char c,d;
    int head_fs = 0;
    int head_la = 0;
    int head_ae = 0;
    UINT_8 head_ft = 0;
    UINT_8 head_fsl = 0;
    UINT_8 head_fsh = 0;
    UINT_8 head_lal = 0;
    UINT_8 head_lah = 0;
    UINT_8 head_ael = 0;
    UINT_8 head_aeh = 0;
    UINT_8 head_sp = 0;
    UINT_8 head_ac = 0;
    UINT_8 head_nu = 0;

    PC_FILE *interf_tape_in_src__fp;

    if ( file_select_ex("Source file",interf_menu_tapeinfiledest_str,"MTD",300,0,0) )
    {
        if ( ( interf_tape_in_src__fp = pc_fopen(interf_menu_tapeinfiledest_str,"rt") ) == NULL )
        {
            alert("Error:","Couldn't open MTD file.","","&OK",NULL,'o',0);

            return D_O_K;
        }

        while ( !pc_feof(interf_tape_in_src__fp) )
        {
            if ( pc_fgets(buffera,CONFIG_BUFFER_LEN,interf_tape_in_src__fp) == NULL )
            {
                goto next_stage;
            }

            if ( buffera[strlen("Savename")] == ' ' )
            {
                if ( strncmp(buffera,"Savename",strlen("Savename")) == 0 )
                {
                    l = -1;

                    savename[0] = '\0';

                    if ( strlen("Savename")+1 < strlen(buffera) )
                    {
                        for ( k = strlen("Savename")+1 ; k < (int) strlen(buffera) ; k++ )
                        {
                            switch ( l )
                            {
                                case -1:
                                {
                                    if ( buffera[k-1] == '=' )
                                    {
                                        l = 0;
                                    }

                                    break;
                                }

                                case 0:
                                {
                                    if ( ( buffera[k-1] != ' '  ) &&
                                         ( buffera[k-1] != '\t' )    )
                                    {
                                        l = 1;

                                        goto default_proceed;
                                    }

                                    break;
                                }

                                default:
                                {
                                    default_proceed:

                                    if ( l < CONFIG_BUFFER_LEN )
                                    {
                                        savename[l-1] = buffera[k-1];
                                        savename[l]   = '\0';
                                    }

                                    l++;

                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if ( buffera[strlen("Filename")] == ' ' )
            {
                if ( strncmp(buffera,"Filename",strlen("Filename")) == 0 )
                {
                    l = -1;

                    filetag[0] = '\0';

                    if ( strlen("Filename")+1 < strlen(buffera) )
                    {
                        for ( k = strlen("Filename")+1 ; k < (int) strlen(buffera) ; k++ )
                        {
                            switch ( l )
                            {
                                case -1:
                                {
                                    if ( buffera[k-1] == '=' )
                                    {
                                        l = 0;
                                    }

                                    break;
                                }

                                case 0:
                                {
                                    if ( ( buffera[k-1] != ' '  ) &&
                                         ( buffera[k-1] != '\t' )    )
                                    {
                                        l = 1;

                                        goto default_proceed_b;
                                    }

                                    break;
                                }

                                default:
                                {
                                    default_proceed_b:

                                    if ( l < CONFIG_BUFFER_LEN )
                                    {
                                        filetag[l-1] = buffera[k-1];
                                        filetag[l]   = '\0';
                                    }

                                    l++;

                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if ( buffera[strlen("Filetype")] == ' ' )
            {
                if ( strncmp(buffera,"Filetype",strlen("Filetype")) == 0 )
                {
                    sscanf(buffera,"%s %c %c",bufferb,&c,&d);

                    head_ft = d;
                }
            }

            if ( buffera[strlen("Filesize")] == ' ' )
            {
                if ( strncmp(buffera,"Filesize",strlen("Filesize")) == 0 )
                {
                    sscanf(buffera,"%s %c %x",bufferb,&c,&head_fs);
                }
            }

            if ( buffera[strlen("Load_addr")] == ' ' )
            {
                if ( strncmp(buffera,"Load_addr",strlen("Load_addr")) == 0 )
                {
                    sscanf(buffera,"%s %c %x",bufferb,&c,&head_la);
                }
            }

            if ( buffera[strlen("Exec_addr")] == ' ' )
            {
                if ( strncmp(buffera,"Exec_addr",strlen("Exec_addr")) == 0 )
                {
                    sscanf(buffera,"%s %c %x",bufferb,&c,&head_ae);
                }
            }

            if ( buffera[strlen("File_speed")] == ' ' )
            {
                if ( strncmp(buffera,"File_speed",strlen("File_speed")) == 0 )
                {
                    sscanf(buffera,"%s %c %x",bufferb,&c,&k);

                    head_sp = k;
                }
            }

            if ( buffera[strlen("Exec_mode")] == ' ' )
            {
                if ( strncmp(buffera,"Exec_mode",strlen("Exec_mode")) == 0 )
                {
                    sscanf(buffera,"%s %c %x",bufferb,&c,&k);

                    head_ac = k;
                }
            }
        }

        next_stage:

        pc_fclose(interf_tape_in_src__fp);

        replace_filename(bufferb,interf_menu_tapeinfiledest_str,savename,CONFIG_BUFFER_LEN);

        if ( ( interf_tape_in_src__fp = pc_fopen(bufferb,"rb") ) == NULL )
        {
            alert("Error:","Couldn't open data file.","","&OK",NULL,'o',0);

            return D_O_K;
        }

        head_lal = (     head_la                    & 0x000ff );
        head_lah = ( ( ( head_la & 0x0ff00 ) >> 8 ) & 0x000ff );

        head_ael = (     head_ae                    & 0x000ff );
        head_aeh = ( ( ( head_ae & 0x0ff00 ) >> 8 ) & 0x000ff );

        head_fsl = (     head_fs                    & 0x000ff );
        head_fsh = ( ( ( head_fs & 0x0ff00 ) >> 8 ) & 0x000ff );

        if ( interf_tape_in_convert_to_memload(head_ft,head_fsl,head_fsh,head_lal,head_lah,head_ael,head_aeh,head_sp,head_ac,head_nu,filetag,head_fs,interf_tape_in_src__fp) )
        {
            alert("Error:","Out of memory.","","&OK",NULL,'o',0);
        }

        pc_fclose(interf_tape_in_src__fp);
    }

    interf_menu_update_menu_marks();

    return D_O_K;
}

int interf_menu_tapestopload(void)
{
    interf_tape_in_reset_state();

    interf_menu_update_menu_marks();

    return D_O_K;
}

#endif









#ifdef IS_WEB

/*
   These macros are needed by the emulator for timing.  The emulation needs
   a timer with ~1 millisecond granularity.  Timers are installed using
   either:

   int install_int_ex(void (*proc)(), SECS_TO_TIMER(x)) - call function proc
        every x seconds.
   int install_int_ex(void (*proc)(), MSEC_TO_TIMER(x)) - call function proc
        every x milliseconds.

   void remove_int(void (*proc)()) - stop calling the interupt function.
*/

int install_int_ex(void (*proc)(), int speed)
{
    return 0;
}

void remove_int(void (*proc)())
{
    return;
}

#endif
