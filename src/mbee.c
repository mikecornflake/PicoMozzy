/*

          PicoMozzy - a Microbee 32k Emulator v1.19.06.2005 (beta)

                        By Alistair Shilton
                           apsh@ee.unimelb.edu.au

          ========================================================

TO DO: make "microbee" filetype which can do ascii/bin processing
       automatically for microbee conventions.

*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "u_dtype.h"
#include "z80cpu.h"
#include "6545.h"
#include "z80pio.h"
#include "interf.h"
#include "configer.h"
#include "debmaloc.h"
#include "beefile.h"
#include "genmod.h"




#define KEYBOARD_USES_LPEN              1
/*#define FAST_IS_SLOW                    1*/
#define CONFIG_FILE                     "mbee32k.ini"

#ifdef KEYBOARD_USES_LPEN
#define ROMREAD0_LPEN_MASK              0x0ffffff00
#define ROMREAD1_LPEN_MASK              0x0ffff0000
#endif

#ifndef KEYBOARD_USES_LPEN
#define ROMREAD0_LPEN_MASK              0x000000000
#define ROMREAD1_LPEN_MASK              0x000000000
#endif

#define CRTC6545_RELATIVE_CLOCK_RATE    2
#define DEFAULT_CLOCK_PERIOD            296
#define DEFAULT_TIMER_PERIOD            1000000
#define DEFAULT_CATCHUP_COUNT           500
#define DEFAULT_MAX_CLOCKOVR            60000
#define DEFAULT_MAX_CLOCKOVR_PB         50000
#define DEFAULT_LAG_POINT               2
#define REAL_CRTC_GRANULARITY           1
#define REAL_CRTC_CLOCK_DIV             1
#define DEFAULT_MAX_CRTC_GRANULARITY    128
#define DEFAULT_MAX_CRTC_CLOCK_DIV      16
#define OVERLOOK_TIMER_PERIOD           2



module_data *bus_z80_wait;
module_data *bus_z80_rfsh;
module_data *bus_z80_data;
module_data *bus_z80_addr;
module_data *bus_z80_reti_count;
module_data *bus_z80_tab_num_start;
module_data *bus_z80_tab_num_finish;
module_data *bus_z80_tab_wr_wait;
module_data *bus_z80_tab_rd_wait;
module_data *bus_sy6545_data;
module_data *bus_sy6545_addr;
module_data *mask_romread;
module_data *mask_colback;
module_data *mask_colctrl;
module_data *mask_soundbit;
module_data *mask_tapeout;
module_data *mask_video_mem_addr;
module_data *mask_video_mem_slfaddr;
module_data *mask_video_charline;
module_data *mask_video_data;
module_data *assign_romread;
module_data *assign_colback;
module_data *assign_colctrl;
module_data *assign_lpenmask0;
module_data *assign_lpenmask1;
module_data *assign_video_data;
module_data *assign_video_charline;
module_data *branch_romread_change;
module_data *do_romread_goes_high;
module_data *do_romread_goes_low;
module_data *do_startup_ramtest;
module_data *do_fixup_romread;
module_data *branch_if_romread_diff;
module_data *branch_mem_startup;
module_data *do_romread_wr;
module_data *do_reset_ramset;
module_data *do_init_ramset;
module_data *lut8_colour_table;
module_data *or_backcol_backint;
module_data *dowhile_redo_bcolint;
module_data *do_vdu_ram_wr;
module_data *do_col_ram_wr;
module_data *do_pcg_ram_wr;
module_data *branch_vdu_ram_wr;
module_data *branch_sw_col_pcg;
module_data *branch_memwr;
module_data *do_switch_in_col_ram;
module_data *do_switch_in_pcg_ram;
module_data *branch_sub_colpcg;
module_data *do_colback_change;
module_data *do_colctrl_change;
module_data *branch_diff_colback;
module_data *branch_diff_colctrl;
module_data *do_col_port_wr;
module_data *do_pio_b_rdy_data_out;
module_data *do_tape_strober;
module_data *mask_pio_b;
module_data *or_pio_b_tape;
module_data *setbus_cpu_tab;
module_data *bus_cputabsel;
module_data *bus_lpen_callmask;
module_data *mem_lpen_table;
module_data *mem_lpen_feedback;
module_data *mem_lpen_feedrfsh;
module_data *bee_interf;
module_data *z80cpu_base;
module_data *z80pio_base;
module_data *sy6545_base;
module_data *jtable_io_wr__base;
module_data *jtable_io_rd__base;
module_data *do_z80_ack_INT;
module_data *do_z80_ack_reset;
module_data *bus_cnt_lpen;
module_data *bus_cnt_update;
module_data *bus_video_mem_addr;
module_data *bus_video_data;
module_data *bus_video_char_line;
module_data *bus_geom;
module_data *bus_geom_pos_x;
module_data *bus_geom_pos_y;
module_data *bus_col_isfore;
module_data *bus_col_fore;
module_data *bus_col_back;
module_data *bus_col_inv;
module_data *bus_pio_ieo;
module_data *bus_pio_iei;
module_data *bus_pio_a_rdy;
module_data *bus_pio_a_strb;
module_data *bus_pio_a_data;
module_data *bus_pio_b_rdy;
module_data *bus_pio_b_strb;
module_data *bus_pio_b_data;
module_data *bus_sound_bit;
module_data *bus_tape_out;
module_data *bus_tape_in;
module_data *bus_romread;
module_data *bus_colctrl;
module_data *bus_colback;
module_data *bus_new_romread;
module_data *bus_new_colctrl;
module_data *bus_new_colback;
module_data *mem_user_ram_a;
module_data *mem_user_ram_b;
module_data *mem_vdu_ram;
module_data *mem_pcg_ram;
module_data *mem_colour_ram;
module_data *mem_rom1;
module_data *mem_rom2;
module_data *mem_rom3;
module_data *mem_rom4;
module_data *mem_rom5;

void set_reset_flag(void *what);
void clear_reset_flag(void *what);
void stop_emulator(void *what);
void timer_speed_emul_off(void *what);
void timer_speed_emul_on(void *what);
void pause_emulation(void *what);
void restart_emulation(void *what);
void speed_throttle(void);
void overlook_timer(void);

void sync_clock(void);

/*
   Variables
   =========

   mbee_reset_flag: Set if the reset key is held down, reset otherwise.
   mbee_power_flag: Set while the emulator is running.  Reset to exit.
   do_throttle: Set if speed emulation is on.
   is_paused: Set when in interupt mode (emulation paused).
   sync_point: This is set whenever the timer interupt function is called
        so that the sync_clock can then do stuff that should be in the timer
        interupt but can't be because you can't call functions in this
        context.
   is_wait: Set when the emulation is in a wait loop.
   overlook_timer_firstcall: Set on startup, reset by first call to the
        overlook timer.
*/

         int mbee_reset_flag          = 0;
         int mbee_power_flag          = 1;
volatile int do_throttle              = 0;
volatile int is_paused                = 0;
volatile int sync_point               = 0;
volatile int overlook_timer_firstcall = 1;

volatile UINT_64 throttle_call_count      = 0;
volatile SINT_64 actual_clocks            = 0;
volatile UINT_32 clock_period             = DEFAULT_CLOCK_PERIOD;
volatile UINT_32 timer_period_x           = DEFAULT_TIMER_PERIOD;
         UINT_32 catchup_point            = DEFAULT_CATCHUP_COUNT;
volatile UINT_32 max_clockovr             = DEFAULT_MAX_CLOCKOVR;
volatile UINT_32 max_clockovr_pb          = DEFAULT_MAX_CLOCKOVR_PB;
volatile UINT_32 is_wait                  = 0;
volatile UINT_32 lag_point                = DEFAULT_LAG_POINT;
volatile UINT_32 crtc_granularity         = REAL_CRTC_GRANULARITY;
volatile UINT_32 max_crtc_granularity     = DEFAULT_MAX_CRTC_GRANULARITY;
         UINT_32 temp_crtc_granularity    = REAL_CRTC_GRANULARITY;
volatile UINT_8  crtc_clock_division      = REAL_CRTC_CLOCK_DIV;
volatile UINT_8  max_crtc_clock_division  = DEFAULT_MAX_CRTC_CLOCK_DIV;
         UINT_8  temp_crtc_clock_division = REAL_CRTC_CLOCK_DIV;









/************************************************************************/
/************************************************************************/


void speed_throttle(void)
{
    sync_point = 1;

    if ( !is_paused )
    {
        throttle_call_count++;

        if ( do_throttle )
        {
            actual_clocks += timer_period_x/clock_period;

            #ifndef DISABLE_CRTC_THROTTLING
            if ( actual_clocks > max_clockovr )
            {
                actual_clocks = max_clockovr_pb;

                /*
                   The emulator is not keeping up, so try to reduce the
                   load due to CRTC emulation.
                */

                if ( crtc_granularity < max_crtc_granularity )
                {
                    crtc_granularity++;
                }

                else if ( crtc_clock_division < max_crtc_clock_division )
                {
                    crtc_granularity     = REAL_CRTC_GRANULARITY;
                    crtc_clock_division *= 2;
                }
            }
            #endif

            /*
               If we're going really fast, we can afford to slow down
               CRTC emulation to improve accuracy.
            */

            #ifndef DISABLE_CRTC_THROTTLING
            if ( is_wait )
            {
                if ( is_wait >= lag_point )
                {
                    if ( crtc_granularity > REAL_CRTC_GRANULARITY )
                    {
                        crtc_granularity--;
                    }

                    else if ( crtc_clock_division > REAL_CRTC_CLOCK_DIV )
                    {
                        crtc_granularity     = max_crtc_granularity;
                        crtc_clock_division /= 2;
                    }

                    is_wait = 1;
                }

                else
                {
                    is_wait++;
                }
            }
            #endif
        }
    }

    return;
}
END_OF_FUNCTION(speed_throttle)

void overlook_timer(void)
{
    if ( !is_paused )
    {
        if ( !overlook_timer_firstcall )
        {
            timer_period_x = (OVERLOOK_TIMER_PERIOD*10000000)/(throttle_call_count/100);
            throttle_call_count = 0;
        }

        else
        {
            overlook_timer_firstcall = 0;
            throttle_call_count      = 0;
        }
    }

    else
    {
        overlook_timer_firstcall = 1;
        throttle_call_count      = 0;
    }

    return;
}
END_OF_FUNCTION(overlook_timer)


int main(int argc, char *argv[])
{
    int configerror;
    SetupData main_setdat[] = { { "timer_period",         &timer_period_x,          2, 1,   50     },
                                { "max_crtc_granularity", &max_crtc_granularity,    2, 1,   512    },
                                { "max_crtc_clock_div",   &max_crtc_clock_division, 0, 1,   512    },
                                { "clock_period",         &clock_period,            2, 10,  10000  },
                                { "max_clockovr",         &max_clockovr,            2, 100, 200000 },
                                { "max_clockovr_pb",      &max_clockovr_pb,         2, 100, 200000 },
                                { "catchup_point",        &catchup_point,           2, 100, 200000 },
                                { "lag_point",            &lag_point,               2, 2,   100    },
                                { "", NULL, 0, 0, 0 } };
    SetupData *all_setdat[2] = { main_setdat , NULL };
    char *configfilename;
    char standard_configfile[] = CONFIG_FILE;

    LOCK_FUNCTION(speed_throttle);
    LOCK_FUNCTION(overlook_timer);

    LOCK_VARIABLE(do_throttle);
    LOCK_VARIABLE(is_paused);
    LOCK_VARIABLE(actual_clocks);
    LOCK_VARIABLE(clock_period);
    LOCK_VARIABLE(timer_period_x);
    LOCK_VARIABLE(is_wait);
    LOCK_VARIABLE(max_clockovr);
    LOCK_VARIABLE(max_clockovr_pb);
    LOCK_VARIABLE(lag_point);
    LOCK_VARIABLE(crtc_granularity);
    LOCK_VARIABLE(crtc_clock_division);
    LOCK_VARIABLE(max_crtc_granularity);
    LOCK_VARIABLE(max_crtc_clock_division);
    LOCK_VARIABLE(sync_point);
    LOCK_VARIABLE(overlook_timer_firstcall);
    LOCK_VARIABLE(throttle_call_count);

    #ifdef DEBUGMODE
    fprintf(stderr,"starting emulator\n");
    #endif

    /*
       Automated configuration routine
    */

    configfilename = standard_configfile;

    if ( argc == 2 )
    {
        configfilename = argv[1];
    }

    if ( argc >= 3 )
    {
        printf("Usage: mbee32k {control_file}\n");

        return 2;
    }

    if ( ( configerror = load_config_file(configfilename,all_setdat) ) )
    {
        printf("Control file %s reports error %d.\n",configfilename,configerror);

        return 1;
    }

    timer_period_x *= 1000000;

    /*
       Allocate modules
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"allocating modules\n");
    #endif

    if ( ( mask_colback           = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_colctrl           = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_pio_b             = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_romread           = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_soundbit          = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_tapeout           = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_video_mem_addr    = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_video_mem_slfaddr = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_video_charline    = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( mask_video_data        = andconstmod_alloc("")                  ) == NULL ) { return 10; }
    if ( ( assign_colback         = assignmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( assign_colctrl         = assignmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( assign_romread         = assignmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( assign_video_data      = assignmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( assign_video_charline  = assignmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( assign_lpenmask0       = assignconstmod_alloc("")               ) == NULL ) { return 10; }
    if ( ( assign_lpenmask1       = assignconstmod_alloc("")               ) == NULL ) { return 10; }
    if ( ( bus_cnt_lpen           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_cnt_update         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_col_back           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_col_fore           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_col_inv            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_col_isfore         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_colback            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_colctrl            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_cputabsel          = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_geom               = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_geom_pos_x         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_geom_pos_y         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_lpen_callmask      = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_new_colback        = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_new_colctrl        = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_new_romread        = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_iei            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_ieo            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_a_data         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_a_rdy          = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_a_strb         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_b_data         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_b_rdy          = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_pio_b_strb         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_romread            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_sound_bit          = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_sy6545_addr        = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_sy6545_data        = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_tape_in            = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_tape_out           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_video_char_line    = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_video_data         = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_video_mem_addr     = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_addr           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_data           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_reti_count     = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_rfsh           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_tab_num_start  = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_tab_num_finish = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_tab_rd_wait    = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_tab_wr_wait    = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( bus_z80_wait           = busmod_alloc("")                       ) == NULL ) { return 10; }
    if ( ( do_fixup_romread       = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_init_ramset         = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_reset_ramset        = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_romread_goes_high   = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_romread_goes_low    = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_romread_wr          = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_startup_ramtest     = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_vdu_ram_wr          = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_z80_ack_INT         = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_z80_ack_reset       = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_col_ram_wr          = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_pcg_ram_wr          = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_switch_in_col_ram   = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_switch_in_pcg_ram   = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_colback_change      = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_colctrl_change      = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_col_port_wr         = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_pio_b_rdy_data_out  = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( do_tape_strober        = domod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( dowhile_redo_bcolint   = dowhilemod_alloc("")                   ) == NULL ) { return 10; }
    if ( ( branch_if_romread_diff = equalsmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( branch_mem_startup     = equalsconstmod_alloc("")               ) == NULL ) { return 10; }
    if ( ( branch_romread_change  = istruemod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( branch_vdu_ram_wr      = istruemod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( branch_sw_col_pcg      = istruemod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( branch_memwr           = lessconstmod_alloc("")                 ) == NULL ) { return 10; }
    if ( ( branch_sub_colpcg      = istruemod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( branch_diff_colback    = equalsmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( branch_diff_colctrl    = equalsmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( lut8_colour_table      = lut8mod_alloc("")                      ) == NULL ) { return 10; }
    if ( ( mem_colour_ram         = memmod_alloc("ram_col")                ) == NULL ) { return 10; }
    if ( ( mem_lpen_feedback      = memmod_alloc("ram_lpen_readcnt")       ) == NULL ) { return 10; }
    if ( ( mem_lpen_feedrfsh      = memmod_alloc("ram_lpen_rfshcnt")       ) == NULL ) { return 10; }
    if ( ( mem_lpen_table         = memmod_alloc("ram_lpen_keymap")        ) == NULL ) { return 10; }
    if ( ( mem_pcg_ram            = memmod_alloc("ram_pcg")                ) == NULL ) { return 10; }
    if ( ( mem_rom1               = memmod_alloc("rom_basic1")             ) == NULL ) { return 10; }
    if ( ( mem_rom2               = memmod_alloc("rom_basic2")             ) == NULL ) { return 10; }
    if ( ( mem_rom3               = memmod_alloc("rom_edasm")              ) == NULL ) { return 10; }
    if ( ( mem_rom4               = memmod_alloc("rom_empty")              ) == NULL ) { return 10; }
    if ( ( mem_rom5               = memmod_alloc("rom_char")               ) == NULL ) { return 10; }
    if ( ( mem_user_ram_a         = memmod_alloc("ram_base1")              ) == NULL ) { return 10; }
    if ( ( mem_user_ram_b         = memmod_alloc("ram_base2")              ) == NULL ) { return 10; }
    if ( ( mem_vdu_ram            = memmod_alloc("ram_vdu")                ) == NULL ) { return 10; }
    if ( ( or_backcol_backint     = ormod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( or_pio_b_tape          = ormod_alloc("")                        ) == NULL ) { return 10; }
    if ( ( setbus_cpu_tab         = setbusmod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( jtable_io_rd__base     = table8mod_alloc("")                    ) == NULL ) { return 10; }
    if ( ( jtable_io_wr__base     = table8mod_alloc("")                    ) == NULL ) { return 10; }

    if ( ( bee_interf             = interf_alloc("")                       ) == NULL ) { return 10; }
    if ( ( sy6545_base            = sy6545_alloc("crtc")                   ) == NULL ) { return 10; }
    if ( ( z80cpu_base            = z80cpu_alloc("")                       ) == NULL ) { return 10; }
    if ( ( z80pio_base            = z80pio_alloc("")                       ) == NULL ) { return 10; }

    /*
       Set variables controlling module performance
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"set module variables\n");
    #endif

    DEBDEREF((mask_colback->var_8bit),0)            = 0x00e;
    DEBDEREF((mask_colctrl->var_8bit),0)            = 0x040;
    DEBDEREF((mask_pio_b->var_8bit),0)              = 0x0fe;
    DEBDEREF((mask_romread->var_8bit),0)            = 0x001;
    DEBDEREF((mask_soundbit->var_8bit),0)           = 0x040;
    DEBDEREF((mask_tapeout->var_8bit),0)            = 0x002;
    DEBDEREF((mask_video_mem_addr->var_16bit),0)    = 0x007FF;
    DEBDEREF((mask_video_mem_slfaddr->var_16bit),0) = 0x007FF;
    DEBDEREF((mask_video_charline->var_8bit),0)     = 0x00f;
    DEBDEREF((mask_video_data->var_16bit),0)        = 0x000E0;

    DEBDEREF((assign_lpenmask0->var_32bit),0) = ROMREAD0_LPEN_MASK;
    DEBDEREF((assign_lpenmask1->var_32bit),0) = ROMREAD1_LPEN_MASK;

    DEBDEREF((branch_mem_startup->var_16bit),0) = 0x07fff;
    DEBDEREF((branch_memwr->var_16bit),0)       = 0x0f800;

    DEBDEREF((bus_lpen_callmask->var_32bit),0) = ROMREAD0_LPEN_MASK;

    DEBDEREF((do_fixup_romread->var_32bit),0)      = 2;
    DEBDEREF((do_init_ramset->var_32bit),0)        = 54;
    DEBDEREF((do_reset_ramset->var_32bit),0)       = 10;
    DEBDEREF((do_romread_goes_high->var_32bit),0)  = 7;
    DEBDEREF((do_romread_goes_low->var_32bit),0)   = 7;
    DEBDEREF((do_startup_ramtest->var_32bit),0)    = 2;
    DEBDEREF((do_romread_wr->var_32bit),0)         = 2;
    DEBDEREF((do_z80_ack_INT->var_32bit),0)        = 3;
    DEBDEREF((do_z80_ack_reset->var_32bit),0)      = 7;
    DEBDEREF((do_vdu_ram_wr->var_32bit),0)         = 4;
    DEBDEREF((do_col_ram_wr->var_32bit),0)         = 10;
    DEBDEREF((do_pcg_ram_wr->var_32bit),0)         = 11;
    DEBDEREF((do_switch_in_col_ram->var_32bit),0)  = 6;
    DEBDEREF((do_switch_in_pcg_ram->var_32bit),0)  = 6;
    DEBDEREF((do_colback_change->var_32bit),0)     = 3;
    DEBDEREF((do_colctrl_change->var_32bit),0)     = 2;
    DEBDEREF((do_col_port_wr->var_32bit),0)        = 5;
    DEBDEREF((do_pio_b_rdy_data_out->var_32bit),0) = 4;
    DEBDEREF((do_tape_strober->var_32bit),0)       = 3;

    DEBDEREF((dowhile_redo_bcolint->var_32bit),0) = 9;

    DEBDEREF((lut8_colour_table->var_8bit),0x000) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x001) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x002) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x003) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x004) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x005) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x006) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x007) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x008) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x009) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x00a) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x00b) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x00c) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x00d) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x00e) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x00f) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x010) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x011) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x012) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x013) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x014) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x015) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x016) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x017) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x018) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x019) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x01a) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x01b) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x01c) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x01d) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x01e) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x01f) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x020) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x021) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x022) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x023) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x024) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x025) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x026) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x027) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x028) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x029) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x02a) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x02b) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x02c) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x02d) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x02e) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x02f) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x030) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x031) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x032) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x033) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x034) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x035) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x036) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x037) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x038) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x039) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x03a) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x03b) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x03c) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x03d) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x03e) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x03f) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x040) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x041) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x042) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x043) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x044) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x045) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x046) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x047) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x048) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x049) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x04a) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x04b) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x04c) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x04d) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x04e) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x04f) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x050) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x051) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x052) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x053) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x054) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x055) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x056) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x057) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x058) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x059) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x05a) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x05b) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x05c) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x05d) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x05e) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x05f) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x060) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x061) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x062) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x063) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x064) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x065) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x066) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x067) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x068) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x069) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x06a) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x06b) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x06c) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x06d) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x06e) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x06f) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x070) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x071) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x072) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x073) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x074) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x075) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x076) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x077) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x078) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x079) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x07a) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x07b) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x07c) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x07d) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x07e) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x07f) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x080) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x081) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x082) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x083) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x084) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x085) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x086) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x087) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x088) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x089) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x08a) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x08b) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x08c) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x08d) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x08e) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x08f) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x090) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x091) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x092) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x093) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x094) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x095) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x096) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x097) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x098) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x099) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x09a) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x09b) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x09c) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x09d) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x09e) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x09f) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a0) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a1) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a2) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a3) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a4) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a5) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a6) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a7) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a8) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x0a9) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0aa) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ab) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ac) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ad) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ae) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x0af) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b0) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b1) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b2) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b3) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b4) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b5) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b6) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b7) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b8) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x0b9) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ba) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x0bb) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0bc) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x0bd) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0be) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x0bf) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c0) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c1) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c2) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c3) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c4) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c5) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c6) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c7) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c8) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x0c9) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ca) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x0cb) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0cc) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x0cd) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ce) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x0cf) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d0) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d1) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d2) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d3) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d4) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d5) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d6) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d7) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d8) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x0d9) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0da) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x0db) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0dc) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x0dd) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0de) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x0df) = 0x03d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e0) = 0x000;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e1) = 0x008;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e2) = 0x010;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e3) = 0x018;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e4) = 0x020;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e5) = 0x028;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e6) = 0x030;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e7) = 0x038;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e8) = 0x006;
    DEBDEREF((lut8_colour_table->var_8bit),0x0e9) = 0x00e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ea) = 0x016;
    DEBDEREF((lut8_colour_table->var_8bit),0x0eb) = 0x01e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ec) = 0x026;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ed) = 0x02e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ee) = 0x036;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ef) = 0x03e;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f0) = 0x007;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f1) = 0x00f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f2) = 0x017;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f3) = 0x01f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f4) = 0x027;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f5) = 0x02f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f6) = 0x037;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f7) = 0x03f;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f8) = 0x005;
    DEBDEREF((lut8_colour_table->var_8bit),0x0f9) = 0x00d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0fa) = 0x015;
    DEBDEREF((lut8_colour_table->var_8bit),0x0fb) = 0x01d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0fc) = 0x025;
    DEBDEREF((lut8_colour_table->var_8bit),0x0fd) = 0x02d;
    DEBDEREF((lut8_colour_table->var_8bit),0x0fe) = 0x035;
    DEBDEREF((lut8_colour_table->var_8bit),0x0ff) = 0x03d;

    DEBDEREF((mem_colour_ram->var_32bit),0)    = 0x007ff;
    DEBDEREF((mem_colour_ram->var_32bit),1)    = 0x007ff;
    DEBDEREF((mem_lpen_feedback->var_32bit),0) = 0x03fff;
    DEBDEREF((mem_lpen_feedback->var_32bit),1) = 0x003f0;
    DEBDEREF((mem_lpen_feedrfsh->var_32bit),0) = 0x03fff;
    DEBDEREF((mem_lpen_feedrfsh->var_32bit),1) = 0x003f0;
    DEBDEREF((mem_lpen_table->var_32bit),0)    = 0x03fff;
    DEBDEREF((mem_lpen_table->var_32bit),1)    = 0x003f0;
    DEBDEREF((mem_pcg_ram->var_32bit),0)       = 0x007ff;
    DEBDEREF((mem_pcg_ram->var_32bit),1)       = 0x007ff;
    DEBDEREF((mem_rom1->var_32bit),0)          = 0x01fff;
    DEBDEREF((mem_rom1->var_32bit),1)          = 0x01fff;
    DEBDEREF((mem_rom2->var_32bit),0)          = 0x01fff;
    DEBDEREF((mem_rom2->var_32bit),1)          = 0x01fff;
    DEBDEREF((mem_rom3->var_32bit),0)          = 0x01fff;
    DEBDEREF((mem_rom3->var_32bit),1)          = 0x01fff;
    DEBDEREF((mem_rom4->var_32bit),0)          = 0x00fff;
    DEBDEREF((mem_rom4->var_32bit),1)          = 0x00fff;
    DEBDEREF((mem_rom5->var_32bit),0)          = 0x007ff;
    DEBDEREF((mem_rom5->var_32bit),1)          = 0x007ff;
    DEBDEREF((mem_user_ram_a->var_32bit),0)    = 0x03fff;
    DEBDEREF((mem_user_ram_a->var_32bit),1)    = 0x03fff;
    DEBDEREF((mem_user_ram_b->var_32bit),0)    = 0x03fff;
    DEBDEREF((mem_user_ram_b->var_32bit),1)    = 0x03fff;
    DEBDEREF((mem_vdu_ram->var_32bit),0)       = 0x007ff;
    DEBDEREF((mem_vdu_ram->var_32bit),1)       = 0x007ff;

    DEREF_STRGVAR(mem_rom1,0) = "bas522a.rom";
    DEREF_STRGVAR(mem_rom2,0) = "bas522b.rom";
    DEREF_STRGVAR(mem_rom3,0) = "edasm.rom";
    DEREF_STRGVAR(mem_rom4,0) = "";
    DEREF_STRGVAR(mem_rom5,0) = "charrom.rom";

    DEREF_STRGVAR(sy6545_base,0) = "charrom.rom";

    DEREF_STRGVAR(bee_interf,0) = configfilename;

    DEBDEREF((setbus_cpu_tab->var_32bit),0) = 10;
    DEBDEREF((setbus_cpu_tab->var_32bit),3) = 5;

    /*
       Initialise modules
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"init modules\n");
    #endif

    if ( andconstmod_init(mask_colback)           ) { return 11; }
    if ( andconstmod_init(mask_colctrl)           ) { return 11; }
    if ( andconstmod_init(mask_pio_b)             ) { return 11; }
    if ( andconstmod_init(mask_romread)           ) { return 11; }
    if ( andconstmod_init(mask_soundbit)          ) { return 11; }
    if ( andconstmod_init(mask_tapeout)           ) { return 11; }
    if ( andconstmod_init(mask_video_mem_addr)    ) { return 11; }
    if ( andconstmod_init(mask_video_mem_slfaddr) ) { return 11; }
    if ( andconstmod_init(mask_video_charline)    ) { return 11; }
    if ( andconstmod_init(mask_video_data)        ) { return 11; }
    if ( assignmod_init(assign_colback)           ) { return 11; }
    if ( assignmod_init(assign_colctrl)           ) { return 11; }
    if ( assignmod_init(assign_romread)           ) { return 11; }
    if ( assignmod_init(assign_video_data)        ) { return 11; }
    if ( assignmod_init(assign_video_charline)    ) { return 11; }
    if ( assignconstmod_init(assign_lpenmask0)    ) { return 11; }
    if ( assignconstmod_init(assign_lpenmask1)    ) { return 11; }
    if ( busmod_init(bus_cnt_lpen)                ) { return 11; }
    if ( busmod_init(bus_cnt_update)              ) { return 11; }
    if ( busmod_init(bus_col_back)                ) { return 11; }
    if ( busmod_init(bus_col_fore)                ) { return 11; }
    if ( busmod_init(bus_col_inv)                 ) { return 11; }
    if ( busmod_init(bus_col_isfore)              ) { return 11; }
    if ( busmod_init(bus_colback)                 ) { return 11; }
    if ( busmod_init(bus_colctrl)                 ) { return 11; }
    if ( busmod_init(bus_cputabsel)               ) { return 11; }
    if ( busmod_init(bus_geom)                    ) { return 11; }
    if ( busmod_init(bus_geom_pos_x)              ) { return 11; }
    if ( busmod_init(bus_geom_pos_y)              ) { return 11; }
    if ( busmod_init(bus_lpen_callmask)           ) { return 11; }
    if ( busmod_init(bus_new_colback)             ) { return 11; }
    if ( busmod_init(bus_new_colctrl)             ) { return 11; }
    if ( busmod_init(bus_new_romread)             ) { return 11; }
    if ( busmod_init(bus_pio_iei)                 ) { return 11; }
    if ( busmod_init(bus_pio_ieo)                 ) { return 11; }
    if ( busmod_init(bus_pio_a_data)              ) { return 11; }
    if ( busmod_init(bus_pio_a_rdy)               ) { return 11; }
    if ( busmod_init(bus_pio_a_strb)              ) { return 11; }
    if ( busmod_init(bus_pio_b_data)              ) { return 11; }
    if ( busmod_init(bus_pio_b_rdy)               ) { return 11; }
    if ( busmod_init(bus_pio_b_strb)              ) { return 11; }
    if ( busmod_init(bus_romread)                 ) { return 11; }
    if ( busmod_init(bus_sound_bit)               ) { return 11; }
    if ( busmod_init(bus_sy6545_addr)             ) { return 11; }
    if ( busmod_init(bus_sy6545_data)             ) { return 11; }
    if ( busmod_init(bus_tape_in)                 ) { return 11; }
    if ( busmod_init(bus_tape_out)                ) { return 11; }
    if ( busmod_init(bus_video_char_line)         ) { return 11; }
    if ( busmod_init(bus_video_data)              ) { return 11; }
    if ( busmod_init(bus_video_mem_addr)          ) { return 11; }
    if ( busmod_init(bus_z80_addr)                ) { return 11; }
    if ( busmod_init(bus_z80_data)                ) { return 11; }
    if ( busmod_init(bus_z80_reti_count)          ) { return 11; }
    if ( busmod_init(bus_z80_rfsh)                ) { return 11; }
    if ( busmod_init(bus_z80_tab_num_start)       ) { return 11; }
    if ( busmod_init(bus_z80_tab_num_finish)      ) { return 11; }
    if ( busmod_init(bus_z80_tab_rd_wait)         ) { return 11; }
    if ( busmod_init(bus_z80_tab_wr_wait)         ) { return 11; }
    if ( busmod_init(bus_z80_wait)                ) { return 11; }
    if ( domod_init(do_fixup_romread)             ) { return 11; }
    if ( domod_init(do_init_ramset)               ) { return 11; }
    if ( domod_init(do_reset_ramset)              ) { return 11; }
    if ( domod_init(do_romread_goes_high)         ) { return 11; }
    if ( domod_init(do_romread_goes_low)          ) { return 11; }
    if ( domod_init(do_startup_ramtest)           ) { return 11; }
    if ( domod_init(do_romread_wr)                ) { return 11; }
    if ( domod_init(do_z80_ack_INT)               ) { return 11; }
    if ( domod_init(do_z80_ack_reset)             ) { return 11; }
    if ( domod_init(do_vdu_ram_wr)                ) { return 11; }
    if ( domod_init(do_col_ram_wr)                ) { return 11; }
    if ( domod_init(do_pcg_ram_wr)                ) { return 11; }
    if ( domod_init(do_switch_in_col_ram)         ) { return 11; }
    if ( domod_init(do_switch_in_pcg_ram)         ) { return 11; }
    if ( domod_init(do_colback_change)            ) { return 11; }
    if ( domod_init(do_colctrl_change)            ) { return 11; }
    if ( domod_init(do_col_port_wr)               ) { return 11; }
    if ( domod_init(do_pio_b_rdy_data_out)        ) { return 11; }
    if ( domod_init(do_tape_strober)              ) { return 11; }
    if ( dowhilemod_init(dowhile_redo_bcolint)    ) { return 11; }
    if ( equalsmod_init(branch_if_romread_diff)   ) { return 11; }
    if ( equalsconstmod_init(branch_mem_startup)  ) { return 11; }
    if ( istruemod_init(branch_romread_change)    ) { return 11; }
    if ( istruemod_init(branch_vdu_ram_wr)        ) { return 11; }
    if ( istruemod_init(branch_sw_col_pcg)        ) { return 11; }
    if ( lessconstmod_init(branch_memwr)          ) { return 11; }
    if ( istruemod_init(branch_sub_colpcg)        ) { return 11; }
    if ( equalsmod_init(branch_diff_colback)      ) { return 11; }
    if ( equalsmod_init(branch_diff_colctrl)      ) { return 11; }
    if ( lut8mod_init(lut8_colour_table)          ) { return 11; }
    if ( memmod_init(mem_colour_ram)              ) { return 11; }
    if ( memmod_init(mem_lpen_feedback)           ) { return 11; }
    if ( memmod_init(mem_lpen_feedrfsh)           ) { return 11; }
    if ( memmod_init(mem_lpen_table)              ) { return 11; }
    if ( memmod_init(mem_pcg_ram)                 ) { return 11; }
    if ( memmod_init(mem_rom1)                    ) { return 11; }
    if ( memmod_init(mem_rom2)                    ) { return 11; }
    if ( memmod_init(mem_rom3)                    ) { return 11; }
    if ( memmod_init(mem_rom4)                    ) { return 11; }
    if ( memmod_init(mem_rom5)                    ) { return 11; }
    if ( memmod_init(mem_user_ram_a)              ) { return 11; }
    if ( memmod_init(mem_user_ram_b)              ) { return 11; }
    if ( memmod_init(mem_vdu_ram)                 ) { return 11; }
    if ( ormod_init(or_backcol_backint)           ) { return 11; }
    if ( ormod_init(or_pio_b_tape)                ) { return 11; }
    if ( setbusmod_init(setbus_cpu_tab)           ) { return 11; }
    if ( table8mod_init(jtable_io_rd__base)       ) { return 11; }
    if ( table8mod_init(jtable_io_wr__base)       ) { return 11; }

    if ( interf_init(bee_interf)                  ) { return 11; }
    if ( sy6545_init(sy6545_base)                 ) { return 11; }
    if ( z80cpu_init(z80cpu_base)                 ) { return 11; }
    if ( z80pio_init(z80pio_base)                 ) { return 11; }

    /*
       Set module pointers.
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"set module pointers\n");
    #endif

    DEBDEREF((bee_interf->bus_8bit),0)                = DEBDEREF((bus_col_isfore->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),1)                = DEBDEREF((bus_col_fore->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),2)                = DEBDEREF((bus_col_back->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),3)                = DEBDEREF((bus_col_inv->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),4)                = DEBDEREF((mem_lpen_table->bus_8bit),2);
    DEBDEREF((bee_interf->bus_8bit),5)                = DEBDEREF((mem_lpen_feedback->bus_8bit),2);
    DEBDEREF((bee_interf->bus_8bit),6)                = DEBDEREF((mem_lpen_feedrfsh->bus_8bit),2);
    DEBDEREF((bee_interf->bus_8bit),7)                = DEBDEREF((bus_pio_a_data->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),8)                = DEBDEREF((bus_pio_a_strb->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),9)                = DEBDEREF((bus_pio_a_rdy->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),10)               = DEBDEREF((bus_pio_a_data->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),11)               = DEBDEREF((bus_pio_a_strb->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),12)               = DEBDEREF((bus_pio_a_rdy->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),13)               = DEBDEREF((bus_sound_bit->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),14)               = DEBDEREF((bus_tape_in->bus_8bit),0);
    DEBDEREF((bee_interf->bus_8bit),15)               = DEBDEREF((bus_tape_out->bus_8bit),0);
    DEBDEREF((bee_interf->bus_16bit),0)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((bee_interf->bus_16bit),1)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((bee_interf->bus_16bit),2)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((bee_interf->bus_16bit),3)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((bee_interf->bus_16bit),4)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((bee_interf->bus_16bit),6)               = DEBDEREF((bus_geom_pos_x->bus_16bit),0);
    DEBDEREF((bee_interf->bus_16bit),7)               = DEBDEREF((bus_geom_pos_y->bus_16bit),0);
    DEBDEREF((bee_interf->bus_32bit),0)               = DEBDEREF((bus_cnt_lpen->bus_32bit),0);
    DEBDEREF((bee_interf->bus_32bit),1)               = DEBDEREF((bus_cnt_update->bus_32bit),0);
    DEBDEREF((bee_interf->sig_calls_outof_module),0)  = DEBDEREF((sy6545_base->sig_calls_into_module),1);
    DEBDEREF((bee_interf->sig_calls_outof_module),1)  = stop_emulator;
    DEBDEREF((bee_interf->sig_calls_outof_module),2)  = timer_speed_emul_on;
    DEBDEREF((bee_interf->sig_calls_outof_module),3)  = timer_speed_emul_off;
    DEBDEREF((bee_interf->sig_calls_outof_module),4)  = set_reset_flag;
    DEBDEREF((bee_interf->sig_calls_outof_module),5)  = clear_reset_flag;
    DEBDEREF((bee_interf->sig_calls_outof_module),6)  = pause_emulation;
    DEBDEREF((bee_interf->sig_calls_outof_module),7)  = restart_emulation;
    DEBDEREF((bee_interf->sig_calls_outof_module),8)  = DEBDEREF((z80pio_base->sig_calls_into_module),9);
    DEBDEREF((bee_interf->sig_calls_outof_module),9)  = DEBDEREF((do_tape_strober->sig_calls_into_module),0);
    DEBDEREF((bee_interf->sig_calls_outof_args),0)    = sy6545_base;
    DEBDEREF((bee_interf->sig_calls_outof_args),1)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),2)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),3)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),4)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),5)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),6)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),7)    = NULL;
    DEBDEREF((bee_interf->sig_calls_outof_args),8)    = z80pio_base;
    DEBDEREF((bee_interf->sig_calls_outof_args),9)    = do_tape_strober;

    DEBDEREF((mask_colback->bus_8bit),0) = DEBDEREF((bus_new_colback->bus_8bit),0);
    DEBDEREF((mask_colback->bus_8bit),1) = DEBDEREF((bus_z80_data->bus_8bit),0);

    DEBDEREF((mask_colctrl->bus_8bit),0) = DEBDEREF((bus_new_colctrl->bus_8bit),0);
    DEBDEREF((mask_colctrl->bus_8bit),1) = DEBDEREF((bus_z80_data->bus_8bit),0);

    DEBDEREF((mask_pio_b->bus_8bit),0) = DEBDEREF((bus_pio_b_data->bus_8bit),0);
    DEBDEREF((mask_pio_b->bus_8bit),1) = DEBDEREF((bus_pio_b_data->bus_8bit),0);

    DEBDEREF((mask_romread->bus_8bit),0) = DEBDEREF((bus_new_romread->bus_8bit),0);
    DEBDEREF((mask_romread->bus_8bit),1) = DEBDEREF((bus_z80_data->bus_8bit),0);

    DEBDEREF((mask_soundbit->bus_8bit),0) = DEBDEREF((bus_sound_bit->bus_8bit),0);
    DEBDEREF((mask_soundbit->bus_8bit),1) = DEBDEREF((bus_pio_b_data->bus_8bit),0);

    DEBDEREF((mask_tapeout->bus_8bit),0) = DEBDEREF((bus_tape_out->bus_8bit),0);
    DEBDEREF((mask_tapeout->bus_8bit),1) = DEBDEREF((bus_pio_b_data->bus_8bit),0);

    DEBDEREF((mask_video_mem_addr->bus_16bit),0) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((mask_video_mem_addr->bus_16bit),1) = DEBDEREF((bus_z80_addr->bus_16bit),0);

    DEBDEREF((mask_video_mem_slfaddr->bus_16bit),0) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((mask_video_mem_slfaddr->bus_16bit),1) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);

    DEBDEREF((mask_video_data->bus_16bit),0) = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((mask_video_data->bus_16bit),1) = DEBDEREF((bus_video_data->bus_16bit),0);

    DEBDEREF((mask_video_charline->bus_8bit),0) = DEBDEREF((bus_video_char_line->bus_8bit),0);
    DEBDEREF((mask_video_charline->bus_8bit),1) = DEBDEREF((bus_video_char_line->bus_8bit),0);

    DEBDEREF((assign_colback->bus_8bit),0) = DEBDEREF((bus_colback->bus_8bit),0);
    DEBDEREF((assign_colback->bus_8bit),1) = DEBDEREF((bus_new_colback->bus_8bit),0);

    DEBDEREF((assign_colctrl->bus_8bit),0) = DEBDEREF((bus_colctrl->bus_8bit),0);
    DEBDEREF((assign_colctrl->bus_8bit),1) = DEBDEREF((bus_new_colctrl->bus_8bit),0);

    DEBDEREF((assign_romread->bus_8bit),0) = DEBDEREF((bus_romread->bus_8bit),0);
    DEBDEREF((assign_romread->bus_8bit),1) = DEBDEREF((bus_new_romread->bus_8bit),0);

    DEBDEREF((assign_video_data->bus_16bit),0) = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((assign_video_data->bus_8bit),1)  = DEBDEREF((bus_z80_data->bus_8bit),0);

    DEBDEREF((assign_video_charline->bus_8bit),0)  = DEBDEREF((bus_video_char_line->bus_8bit),0);
    DEBDEREF((assign_video_charline->bus_16bit),1) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);

    DEBDEREF((assign_lpenmask0->bus_32bit),0) = DEBDEREF((bus_lpen_callmask->bus_32bit),0);

    DEBDEREF((assign_lpenmask1->bus_32bit),0) = DEBDEREF((bus_lpen_callmask->bus_32bit),0);

    DEBDEREF((branch_if_romread_diff->bus_8bit),0)               = DEBDEREF((bus_romread->bus_8bit),0);
    DEBDEREF((branch_if_romread_diff->bus_8bit),1)               = DEBDEREF((bus_new_romread->bus_8bit),0);
    DEBDEREF((branch_if_romread_diff->sig_calls_outof_module),1) = DEBDEREF((do_fixup_romread->sig_calls_into_module),0);
    DEBDEREF((branch_if_romread_diff->sig_calls_outof_args),1)   = (void *) do_fixup_romread;

    DEBDEREF((branch_mem_startup->bus_16bit),0)              = DEBDEREF((bus_z80_addr->bus_16bit),0);
    DEBDEREF((branch_mem_startup->sig_calls_outof_module),2) = DEBDEREF((do_init_ramset->sig_calls_into_module),0);
    DEBDEREF((branch_mem_startup->sig_calls_outof_args),2)   = (void *) do_init_ramset;

    DEBDEREF((branch_romread_change->bus_8bit),0)               = DEBDEREF((bus_romread->bus_8bit),0);
    DEBDEREF((branch_romread_change->sig_calls_outof_module),0) = DEBDEREF((do_romread_goes_high->sig_calls_into_module),0);
    DEBDEREF((branch_romread_change->sig_calls_outof_module),1) = DEBDEREF((do_romread_goes_low->sig_calls_into_module),0);
    DEBDEREF((branch_romread_change->sig_calls_outof_args),0)   = (void *) do_romread_goes_high;
    DEBDEREF((branch_romread_change->sig_calls_outof_args),1)   = (void *) do_romread_goes_low;

    DEBDEREF((branch_vdu_ram_wr->bus_8bit),0)               = DEBDEREF((bus_romread->bus_8bit),0);
    DEBDEREF((branch_vdu_ram_wr->sig_calls_outof_module),1) = DEBDEREF((do_vdu_ram_wr->sig_calls_into_module),0);
    DEBDEREF((branch_vdu_ram_wr->sig_calls_outof_args),1)   = (void *) do_vdu_ram_wr;

    DEBDEREF((branch_sw_col_pcg->bus_8bit),0)               = DEBDEREF((bus_colctrl->bus_8bit),0);
    DEBDEREF((branch_sw_col_pcg->sig_calls_outof_module),0) = DEBDEREF((do_col_ram_wr->sig_calls_into_module),0);
    DEBDEREF((branch_sw_col_pcg->sig_calls_outof_module),1) = DEBDEREF((do_pcg_ram_wr->sig_calls_into_module),0);
    DEBDEREF((branch_sw_col_pcg->sig_calls_outof_args),0)   = (void *) do_col_ram_wr;
    DEBDEREF((branch_sw_col_pcg->sig_calls_outof_args),1)   = (void *) do_pcg_ram_wr;

    DEBDEREF((branch_memwr->bus_16bit),0)              = DEBDEREF((bus_z80_addr->bus_16bit),0);
    DEBDEREF((branch_memwr->sig_calls_outof_module),2) = DEBDEREF((branch_vdu_ram_wr->sig_calls_into_module),0);
    DEBDEREF((branch_memwr->sig_calls_outof_module),3) = DEBDEREF((branch_sw_col_pcg->sig_calls_into_module),0);
    DEBDEREF((branch_memwr->sig_calls_outof_args),2)   = (void *) branch_vdu_ram_wr;
    DEBDEREF((branch_memwr->sig_calls_outof_args),3)   = (void *) branch_sw_col_pcg;

    DEBDEREF((branch_sub_colpcg->bus_8bit),0)               = DEBDEREF((bus_colctrl->bus_8bit),0);
    DEBDEREF((branch_sub_colpcg->sig_calls_outof_module),0) = DEBDEREF((do_switch_in_col_ram->sig_calls_into_module),0);
    DEBDEREF((branch_sub_colpcg->sig_calls_outof_module),1) = DEBDEREF((do_switch_in_pcg_ram->sig_calls_into_module),0);
    DEBDEREF((branch_sub_colpcg->sig_calls_outof_args),0)   = (void *) do_switch_in_col_ram;
    DEBDEREF((branch_sub_colpcg->sig_calls_outof_args),1)   = (void *) do_switch_in_pcg_ram;

    DEBDEREF((branch_diff_colback->bus_8bit),0)               = DEBDEREF((bus_colback->bus_8bit),0);
    DEBDEREF((branch_diff_colback->bus_8bit),1)               = DEBDEREF((bus_new_colback->bus_8bit),0);
    DEBDEREF((branch_diff_colback->sig_calls_outof_module),1) = DEBDEREF((do_colback_change->sig_calls_into_module),0);
    DEBDEREF((branch_diff_colback->sig_calls_outof_args),1)   = (void *) do_colback_change;

    DEBDEREF((branch_diff_colctrl->bus_8bit),0)               = DEBDEREF((bus_colctrl->bus_8bit),0);
    DEBDEREF((branch_diff_colctrl->bus_8bit),1)               = DEBDEREF((bus_new_colctrl->bus_8bit),0);
    DEBDEREF((branch_diff_colctrl->sig_calls_outof_module),1) = DEBDEREF((do_colctrl_change->sig_calls_into_module),0);
    DEBDEREF((branch_diff_colctrl->sig_calls_outof_args),1)   = (void *) do_colctrl_change;

    DEBDEREF((do_fixup_romread->sig_calls_outof_module),0) = DEBDEREF((assign_romread->sig_calls_into_module),0);
    DEBDEREF((do_fixup_romread->sig_calls_outof_module),1) = DEBDEREF((branch_romread_change->sig_calls_into_module),0);
    DEBDEREF((do_fixup_romread->sig_calls_outof_args),0)   = (void *) assign_romread;
    DEBDEREF((do_fixup_romread->sig_calls_outof_args),1)   = (void *) branch_romread_change;

    DEBDEREF((do_reset_ramset->sig_calls_outof_module),0) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x000);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),1) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x03F);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),2) = DEBDEREF((bus_cputabsel->sig_calls_into_module),0);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),3) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),4) = DEBDEREF((z80cpu_base->sig_calls_into_module),23);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),5) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x040);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),6) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x07F);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),7) = DEBDEREF((bus_cputabsel->sig_calls_into_module),1);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),8) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_reset_ramset->sig_calls_outof_module),9) = DEBDEREF((z80cpu_base->sig_calls_into_module),23);
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),0)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),1)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),2)   = (void *) bus_cputabsel;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),3)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),4)   = (void *) z80cpu_base;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),5)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),6)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),7)   = (void *) bus_cputabsel;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),8)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_reset_ramset->sig_calls_outof_args),9)   = (void *) z80cpu_base;

    DEBDEREF((do_init_ramset->sig_calls_outof_module), 0) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x000);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 1) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x03F);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 2) = DEBDEREF((bus_cputabsel->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 3) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 4) = DEBDEREF((z80cpu_base->sig_calls_into_module),16);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 5) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 6) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 7) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x040);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 8) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x07F);
    DEBDEREF((do_init_ramset->sig_calls_outof_module), 9) = DEBDEREF((bus_cputabsel->sig_calls_into_module),1);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),10) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),11) = DEBDEREF((z80cpu_base->sig_calls_into_module),16);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),12) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),13) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),14) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x080);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),15) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x09F);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),16) = DEBDEREF((bus_cputabsel->sig_calls_into_module),5);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),17) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),18) = DEBDEREF((z80cpu_base->sig_calls_into_module),15);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),19) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),20) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),21) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0A0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),22) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0BF);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),23) = DEBDEREF((bus_cputabsel->sig_calls_into_module),6);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),24) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),25) = DEBDEREF((z80cpu_base->sig_calls_into_module),15);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),26) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),27) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),28) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0C0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),29) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0DF);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),30) = DEBDEREF((bus_cputabsel->sig_calls_into_module),7);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),31) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),32) = DEBDEREF((z80cpu_base->sig_calls_into_module),15);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),33) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),34) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),35) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0E0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),36) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0EF);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),37) = DEBDEREF((bus_cputabsel->sig_calls_into_module),8);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),38) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),39) = DEBDEREF((z80cpu_base->sig_calls_into_module),15);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),40) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),41) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),42) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0F0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),43) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0F7);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),44) = DEBDEREF((bus_cputabsel->sig_calls_into_module),2);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),45) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),46) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),47) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),48) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0F8);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),49) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0FF);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),50) = DEBDEREF((bus_cputabsel->sig_calls_into_module),3);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),51) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),52) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_init_ramset->sig_calls_outof_module),53) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 0)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 1)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 2)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 3)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 4)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 5)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 6)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 7)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 8)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args), 9)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),10)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),11)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),12)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),13)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),14)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),15)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),16)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),17)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),18)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),19)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),20)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),21)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),22)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),23)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),24)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),25)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),26)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),27)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),28)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),29)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),30)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),31)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),32)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),33)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),34)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),35)   = (void *) bus_z80_tab_num_start ;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),36)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),37)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),38)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),39)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),40)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),41)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),42)   = (void *) bus_z80_tab_num_start ;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),43)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),44)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),45)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),46)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),47)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),48)   = (void *) bus_z80_tab_num_start ;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),49)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),50)   = (void *) bus_cputabsel;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),51)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),52)   = (void *) z80cpu_base;
    DEBDEREF((do_init_ramset->sig_calls_outof_args),53)   = (void *) z80cpu_base;

    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),0) = DEBDEREF((assign_lpenmask1->sig_calls_into_module),18);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),1) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0F0);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),2) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0F7);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),3) = DEBDEREF((bus_cputabsel->sig_calls_into_module),9);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),4) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),5) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_module),6) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),0)   = (void *) assign_lpenmask1;
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),1)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),2)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),3)   = (void *) bus_cputabsel;
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),4)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),5)   = (void *) z80cpu_base;
    DEBDEREF((do_romread_goes_high->sig_calls_outof_args),6)   = (void *) z80cpu_base;

    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),0) = DEBDEREF((assign_lpenmask0->sig_calls_into_module),18);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),1) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0F0);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),2) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0F7);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),3) = DEBDEREF((bus_cputabsel->sig_calls_into_module),2);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),4) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),5) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_module),6) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),0)   = (void *) assign_lpenmask0;
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),1)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),2)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),3)   = (void *) bus_cputabsel;
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),4)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),5)   = (void *) z80cpu_base;
    DEBDEREF((do_romread_goes_low->sig_calls_outof_args),6)   = (void *) z80cpu_base;

    DEBDEREF((do_startup_ramtest->sig_calls_outof_module),0) = DEBDEREF((bus_z80_data->sig_calls_into_module),256);
    DEBDEREF((do_startup_ramtest->sig_calls_outof_module),1) = DEBDEREF((branch_mem_startup->sig_calls_into_module),1);
    DEBDEREF((do_startup_ramtest->sig_calls_outof_args),0)   = (void *) bus_z80_data;
    DEBDEREF((do_startup_ramtest->sig_calls_outof_args),1)   = (void *) branch_mem_startup;

    DEBDEREF((do_romread_wr->sig_calls_outof_module),0) = DEBDEREF((mask_romread->sig_calls_into_module),0);
    DEBDEREF((do_romread_wr->sig_calls_outof_module),1) = DEBDEREF((branch_if_romread_diff->sig_calls_into_module),0);
    DEBDEREF((do_romread_wr->sig_calls_outof_args),0)   = (void *) mask_romread;
    DEBDEREF((do_romread_wr->sig_calls_outof_args),1)   = (void *) branch_if_romread_diff;

    DEBDEREF((do_z80_ack_INT->sig_calls_outof_module),0) = DEBDEREF((bus_z80_data->sig_calls_into_module),256);
    DEBDEREF((do_z80_ack_INT->sig_calls_outof_module),1) = DEBDEREF((z80pio_base->sig_calls_into_module),12);
    DEBDEREF((do_z80_ack_INT->sig_calls_outof_module),2) = DEBDEREF((z80cpu_base->sig_calls_into_module),4);
    DEBDEREF((do_z80_ack_INT->sig_calls_outof_args),0)   = (void *) bus_z80_data;
    DEBDEREF((do_z80_ack_INT->sig_calls_outof_args),1)   = (void *) z80pio_base;
    DEBDEREF((do_z80_ack_INT->sig_calls_outof_args),2)   = (void *) z80cpu_base;

    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),0) = DEBDEREF((bus_romread->sig_calls_into_module),256);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),1) = DEBDEREF((bus_colctrl->sig_calls_into_module),256);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),2) = DEBDEREF((bus_colback->sig_calls_into_module),256);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),3) = DEBDEREF((bus_lpen_callmask->sig_calls_into_module),256);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),4) = DEBDEREF((sy6545_base->sig_calls_into_module),0);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),5) = DEBDEREF((z80pio_base->sig_calls_into_module),0);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_module),6) = DEBDEREF((do_reset_ramset->sig_calls_into_module),0);
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),0)   = (void *) bus_romread;
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),1)   = (void *) bus_colctrl;
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),2)   = (void *) bus_colback;
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),3)   = (void *) bus_lpen_callmask;
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),4)   = (void *) sy6545_base;
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),5)   = (void *) z80pio_base;
    DEBDEREF((do_z80_ack_reset->sig_calls_outof_args),6)   = (void *) do_reset_ramset;

    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_module),0) = DEBDEREF((mask_video_mem_addr->sig_calls_into_module),1);
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_module),1) = DEBDEREF((assign_video_data->sig_calls_into_module),26);
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_module),2) = DEBDEREF((mem_vdu_ram->sig_calls_into_module),2);
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_module),3) = DEBDEREF((sy6545_base->sig_calls_into_module),7);
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_args),0)   = (void *) mask_video_mem_addr;
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_args),1)   = (void *) assign_video_data;
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_args),2)   = (void *) mem_vdu_ram;
    DEBDEREF((do_vdu_ram_wr->sig_calls_outof_args),3)   = (void *) sy6545_base;

    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),0) = DEBDEREF((mask_video_mem_addr->sig_calls_into_module),1);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),1) = DEBDEREF((mem_colour_ram->sig_calls_into_module),2);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),2) = DEBDEREF((lut8_colour_table->sig_calls_into_module),1);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),3) = DEBDEREF((sy6545_base->sig_calls_into_module),8);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),4) = DEBDEREF((assign_video_data->sig_calls_into_module),26);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),5) = DEBDEREF((mask_video_data->sig_calls_into_module),1);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),6) = DEBDEREF((bus_video_data->sig_calls_into_module),265);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),7) = DEBDEREF((bus_video_data->sig_calls_into_module),265);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),8) = DEBDEREF((or_backcol_backint->sig_calls_into_module),3);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_module),9) = DEBDEREF((sy6545_base->sig_calls_into_module),9);
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),0)   = (void *) mask_video_mem_addr;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),1)   = (void *) mem_colour_ram;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),2)   = (void *) lut8_colour_table;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),3)   = (void *) sy6545_base;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),4)   = (void *) assign_video_data;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),5)   = (void *) mask_video_data;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),6)   = (void *) bus_video_data;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),7)   = (void *) bus_video_data;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),8)   = (void *) or_backcol_backint;
    DEBDEREF((do_col_ram_wr->sig_calls_outof_args),9)   = (void *) sy6545_base;

    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),0)  = DEBDEREF((mask_video_mem_addr->sig_calls_into_module),1);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),1)  = DEBDEREF((assign_video_data->sig_calls_into_module),26);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),2)  = DEBDEREF((mem_pcg_ram->sig_calls_into_module),2);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),3)  = DEBDEREF((assign_video_charline->sig_calls_into_module),1);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),4)  = DEBDEREF((mask_video_charline->sig_calls_into_module),0);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),5)  = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),265);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),6)  = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),265);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),7)  = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),265);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),8)  = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),265);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),9)  = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),290);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_module),10) = DEBDEREF((sy6545_base->sig_calls_into_module),6);
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),0)    = (void *) mask_video_mem_addr;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),1)    = (void *) assign_video_data;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),2)    = (void *) mem_pcg_ram;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),3)    = (void *) assign_video_charline;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),4)    = (void *) mask_video_charline;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),5)    = (void *) bus_video_mem_addr;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),6)    = (void *) bus_video_mem_addr;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),7)    = (void *) bus_video_mem_addr;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),8)    = (void *) bus_video_mem_addr;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),9)    = (void *) bus_video_mem_addr;
    DEBDEREF((do_pcg_ram_wr->sig_calls_outof_args),10)   = (void *) sy6545_base;

    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_module),0) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0F8);
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_module),1) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0FF);
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_module),2) = DEBDEREF((bus_cputabsel->sig_calls_into_module),4);
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_module),3) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_module),4) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_module),5) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_args),0)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_args),1)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_args),2)   = (void *) bus_cputabsel;
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_args),3)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_args),4)   = (void *) z80cpu_base;
    DEBDEREF((do_switch_in_col_ram->sig_calls_outof_args),5)   = (void *) z80cpu_base;

    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_module),0) = DEBDEREF((bus_z80_tab_num_start ->sig_calls_into_module),0x0F8);
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_module),1) = DEBDEREF((bus_z80_tab_num_finish->sig_calls_into_module),0x0FF);
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_module),2) = DEBDEREF((bus_cputabsel->sig_calls_into_module),3);
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_module),3) = DEBDEREF((setbus_cpu_tab->sig_calls_into_module),0);
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_module),4) = DEBDEREF((z80cpu_base->sig_calls_into_module),19);
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_module),5) = DEBDEREF((z80cpu_base->sig_calls_into_module),22);
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_args),0)   = (void *) bus_z80_tab_num_start;
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_args),1)   = (void *) bus_z80_tab_num_finish;
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_args),2)   = (void *) bus_cputabsel;
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_args),3)   = (void *) setbus_cpu_tab;
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_args),4)   = (void *) z80cpu_base;
    DEBDEREF((do_switch_in_pcg_ram->sig_calls_outof_args),5)   = (void *) z80cpu_base;

    DEBDEREF((do_colback_change->sig_calls_outof_module),0) = DEBDEREF((assign_colback->sig_calls_into_module),0);
    DEBDEREF((do_colback_change->sig_calls_outof_module),1) = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),256);
    DEBDEREF((do_colback_change->sig_calls_outof_module),2) = DEBDEREF((dowhile_redo_bcolint->sig_calls_into_module),1);
    DEBDEREF((do_colback_change->sig_calls_outof_args),0)   = (void *) assign_colback;
    DEBDEREF((do_colback_change->sig_calls_outof_args),1)   = (void *) bus_video_mem_addr;
    DEBDEREF((do_colback_change->sig_calls_outof_args),2)   = (void *) dowhile_redo_bcolint;

    DEBDEREF((do_colctrl_change->sig_calls_outof_module),0) = DEBDEREF((assign_colctrl->sig_calls_into_module),0);
    DEBDEREF((do_colctrl_change->sig_calls_outof_module),1) = DEBDEREF((branch_sub_colpcg->sig_calls_into_module),0);
    DEBDEREF((do_colctrl_change->sig_calls_outof_args),0)   = (void *) assign_colctrl;
    DEBDEREF((do_colctrl_change->sig_calls_outof_args),1)   = (void *) branch_sub_colpcg;

    DEBDEREF((do_col_port_wr->sig_calls_outof_module),0) = DEBDEREF((mask_colctrl->sig_calls_into_module),0);
    DEBDEREF((do_col_port_wr->sig_calls_outof_module),1) = DEBDEREF((mask_colback->sig_calls_into_module),0);
    DEBDEREF((do_col_port_wr->sig_calls_outof_module),2) = DEBDEREF((bus_new_colback->sig_calls_into_module),259);
    DEBDEREF((do_col_port_wr->sig_calls_outof_module),3) = DEBDEREF((branch_diff_colback->sig_calls_into_module),0);
    DEBDEREF((do_col_port_wr->sig_calls_outof_module),4) = DEBDEREF((branch_diff_colctrl->sig_calls_into_module),0);
    DEBDEREF((do_col_port_wr->sig_calls_outof_args),0)   = (void *) mask_colctrl;
    DEBDEREF((do_col_port_wr->sig_calls_outof_args),1)   = (void *) mask_colback;
    DEBDEREF((do_col_port_wr->sig_calls_outof_args),2)   = (void *) bus_new_colback;
    DEBDEREF((do_col_port_wr->sig_calls_outof_args),3)   = (void *) branch_diff_colback;
    DEBDEREF((do_col_port_wr->sig_calls_outof_args),4)   = (void *) branch_diff_colctrl;

    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_module),0) = DEBDEREF((mask_tapeout->sig_calls_into_module),0);
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_module),1) = DEBDEREF((bee_interf->sig_calls_into_module),9);
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_module),2) = DEBDEREF((mask_soundbit->sig_calls_into_module),0);
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_module),3) = DEBDEREF((bee_interf->sig_calls_into_module),8);
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_args),0)   = (void *) mask_tapeout;
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_args),1)   = (void *) bee_interf;
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_args),2)   = (void *) mask_soundbit;
    DEBDEREF((do_pio_b_rdy_data_out->sig_calls_outof_args),3)   = (void *) bee_interf;

    DEBDEREF((do_tape_strober->sig_calls_outof_module),0) = DEBDEREF((mask_pio_b->sig_calls_into_module),0);
    DEBDEREF((do_tape_strober->sig_calls_outof_module),1) = DEBDEREF((or_pio_b_tape->sig_calls_into_module),0);
    DEBDEREF((do_tape_strober->sig_calls_outof_module),2) = DEBDEREF((z80pio_base->sig_calls_into_module),10);
    DEBDEREF((do_tape_strober->sig_calls_outof_args),0)   = (void *) mask_pio_b;
    DEBDEREF((do_tape_strober->sig_calls_outof_args),1)   = (void *) or_pio_b_tape;
    DEBDEREF((do_tape_strober->sig_calls_outof_args),2)   = (void *) z80pio_base;

    DEBDEREF((dowhile_redo_bcolint->bus_16bit),0)              = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),0) = DEBDEREF((mem_colour_ram->sig_calls_into_module),5);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),1) = DEBDEREF((assign_video_data->sig_calls_into_module),26);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),2) = DEBDEREF((mask_video_data->sig_calls_into_module),1);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),3) = DEBDEREF((bus_video_data->sig_calls_into_module),265);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),4) = DEBDEREF((bus_video_data->sig_calls_into_module),265);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),5) = DEBDEREF((or_backcol_backint->sig_calls_into_module),3);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),6) = DEBDEREF((sy6545_base->sig_calls_into_module),9);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),7) = DEBDEREF((bus_video_mem_addr->sig_calls_into_module),263);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_module),8) = DEBDEREF((mask_video_mem_slfaddr->sig_calls_into_module),1);
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),0)   = (void *) mem_colour_ram;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),1)   = (void *) assign_video_data;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),2)   = (void *) mask_video_data;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),3)   = (void *) bus_video_data;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),4)   = (void *) bus_video_data;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),5)   = (void *) or_backcol_backint;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),6)   = (void *) sy6545_base;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),7)   = (void *) bus_video_mem_addr;
    DEBDEREF((dowhile_redo_bcolint->sig_calls_outof_args),8)   = (void *) mask_video_mem_slfaddr;

    DEBDEREF((lut8_colour_table->bus_16bit),0) = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((lut8_colour_table->bus_8bit),1)  = DEBDEREF((bus_z80_data->bus_8bit),0);

    DEBDEREF((mem_colour_ram->bus_8bit),0)  = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((mem_colour_ram->bus_16bit),0) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);

    DEBDEREF((mem_lpen_feedback->bus_8bit),0)  = DEBDEREF((bus_sy6545_data->bus_8bit),0);
    DEBDEREF((mem_lpen_feedback->bus_16bit),0) = DEBDEREF((bus_sy6545_addr->bus_16bit),0);

    DEBDEREF((mem_lpen_feedrfsh->bus_8bit),0)  = DEBDEREF((bus_sy6545_data->bus_8bit),0);
    DEBDEREF((mem_lpen_feedrfsh->bus_16bit),0) = DEBDEREF((bus_sy6545_addr->bus_16bit),0);

    DEBDEREF((mem_lpen_table->bus_8bit),0)  = DEBDEREF((bus_sy6545_data->bus_8bit),0);
    DEBDEREF((mem_lpen_table->bus_16bit),0) = DEBDEREF((bus_sy6545_addr->bus_16bit),0);

    DEBDEREF((mem_pcg_ram->bus_8bit),0)  = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((mem_pcg_ram->bus_16bit),0) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);

    DEBDEREF((mem_vdu_ram->bus_8bit),0)  = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((mem_vdu_ram->bus_16bit),0) = DEBDEREF((bus_video_mem_addr->bus_16bit),0);

    DEBDEREF((or_backcol_backint->bus_8bit),2)  = DEBDEREF((bus_colback->bus_8bit),0);
    DEBDEREF((or_backcol_backint->bus_16bit),0) = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((or_backcol_backint->bus_16bit),1) = DEBDEREF((bus_video_data->bus_16bit),0);

    DEBDEREF((or_pio_b_tape->bus_8bit),0) = DEBDEREF((bus_pio_b_data->bus_8bit),0);
    DEBDEREF((or_pio_b_tape->bus_8bit),1) = DEBDEREF((bus_pio_b_data->bus_8bit),0);
    DEBDEREF((or_pio_b_tape->bus_8bit),2) = DEBDEREF((bus_tape_in->bus_8bit),0);

    DEBDEREF((setbus_cpu_tab->modptrs),0)   = z80cpu_base;
    DEBDEREF((setbus_cpu_tab->bus_8bit),0)  = DEBDEREF((bus_cputabsel->bus_8bit),0);
    DEBDEREF((setbus_cpu_tab->bus_8bit),3)  = DEBDEREF((mem_user_ram_a->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),4)  = DEBDEREF((mem_user_ram_b->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),5)  = DEBDEREF((mem_vdu_ram->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),6)  = DEBDEREF((mem_pcg_ram->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),7)  = DEBDEREF((mem_colour_ram->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),8)  = DEBDEREF((mem_rom1->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),9)  = DEBDEREF((mem_rom2->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),10) = DEBDEREF((mem_rom3->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),11) = DEBDEREF((mem_rom4->bus_8bit),2);
    DEBDEREF((setbus_cpu_tab->bus_8bit),12) = DEBDEREF((mem_rom5->bus_8bit),2);

    DEBDEREF((jtable_io_rd__base->bus_16bit),0)                 = DEBDEREF((bus_z80_addr->bus_16bit),0);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x00) = DEBDEREF((z80pio_base->sig_calls_into_module),5);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x01) = DEBDEREF((z80pio_base->sig_calls_into_module),6);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x02) = DEBDEREF((z80pio_base->sig_calls_into_module),7);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x03) = DEBDEREF((z80pio_base->sig_calls_into_module),8);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x0c) = DEBDEREF((sy6545_base->sig_calls_into_module),4);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x0d) = DEBDEREF((sy6545_base->sig_calls_into_module),5);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x0e) = DEBDEREF((sy6545_base->sig_calls_into_module),4);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x0f) = DEBDEREF((sy6545_base->sig_calls_into_module),5);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x10) = DEBDEREF((z80pio_base->sig_calls_into_module),5);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x11) = DEBDEREF((z80pio_base->sig_calls_into_module),6);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x12) = DEBDEREF((z80pio_base->sig_calls_into_module),7);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x13) = DEBDEREF((z80pio_base->sig_calls_into_module),8);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x1c) = DEBDEREF((sy6545_base->sig_calls_into_module),4);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x1d) = DEBDEREF((sy6545_base->sig_calls_into_module),5);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x1e) = DEBDEREF((sy6545_base->sig_calls_into_module),4);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_module),0x1f) = DEBDEREF((sy6545_base->sig_calls_into_module),5);
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x00)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x01)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x02)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x03)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x0c)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x0d)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x0e)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x0f)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x10)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x11)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x12)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x13)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x1c)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x1d)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x1e)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_rd__base->sig_calls_outof_args),0x1f)   = (void *) sy6545_base;

    DEBDEREF((jtable_io_wr__base->bus_16bit),0)                 = DEBDEREF((bus_z80_addr->bus_16bit),0);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x00) = DEBDEREF((z80pio_base->sig_calls_into_module),1);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x01) = DEBDEREF((z80pio_base->sig_calls_into_module),2);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x02) = DEBDEREF((z80pio_base->sig_calls_into_module),3);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x03) = DEBDEREF((z80pio_base->sig_calls_into_module),4);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x08) = DEBDEREF((do_col_port_wr->sig_calls_into_module),0);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x0b) = DEBDEREF((do_romread_wr->sig_calls_into_module),0);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x0c) = DEBDEREF((sy6545_base->sig_calls_into_module),2);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x0d) = DEBDEREF((sy6545_base->sig_calls_into_module),3);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x0e) = DEBDEREF((sy6545_base->sig_calls_into_module),2);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x0f) = DEBDEREF((sy6545_base->sig_calls_into_module),3);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x10) = DEBDEREF((z80pio_base->sig_calls_into_module),1);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x11) = DEBDEREF((z80pio_base->sig_calls_into_module),2);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x12) = DEBDEREF((z80pio_base->sig_calls_into_module),3);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x13) = DEBDEREF((z80pio_base->sig_calls_into_module),4);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x18) = DEBDEREF((do_col_port_wr->sig_calls_into_module),0);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x1b) = DEBDEREF((do_romread_wr->sig_calls_into_module),0);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x1c) = DEBDEREF((sy6545_base->sig_calls_into_module),2);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x1d) = DEBDEREF((sy6545_base->sig_calls_into_module),3);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x1e) = DEBDEREF((sy6545_base->sig_calls_into_module),2);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_module),0x1f) = DEBDEREF((sy6545_base->sig_calls_into_module),3);
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x00)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x01)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x02)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x03)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x08)   = (void *) do_col_port_wr;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x0b)   = (void *) do_romread_wr;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x0c)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x0d)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x0e)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x0f)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x10)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x11)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x12)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x13)   = (void *) z80pio_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x18)   = (void *) do_col_port_wr;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x1b)   = (void *) do_romread_wr;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x1c)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x1d)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x1e)   = (void *) sy6545_base;
    DEBDEREF((jtable_io_wr__base->sig_calls_outof_args),0x1f)   = (void *) sy6545_base;

    DEBDEREF((sy6545_base->bus_8bit),0)                = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),1)                = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),2)                = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),3)                = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),4)                = DEBDEREF((bus_video_char_line->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),5)                = DEBDEREF((bus_col_isfore->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),6)                = DEBDEREF((bus_col_fore->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),7)                = DEBDEREF((bus_col_back->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),8)                = DEBDEREF((bus_col_inv->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),9)                = DEBDEREF((bus_sy6545_data->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_8bit),10)               = DEBDEREF((bus_sy6545_data->bus_8bit),0);
    DEBDEREF((sy6545_base->bus_16bit),0)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),1)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),2)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),3)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),4)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),5)               = DEBDEREF((bus_geom->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),6)               = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),7)               = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),8)               = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),9)               = DEBDEREF((bus_video_mem_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),10)              = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),11)              = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),12)              = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),13)              = DEBDEREF((bus_video_data->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),14)              = DEBDEREF((bus_geom_pos_x->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),15)              = DEBDEREF((bus_geom_pos_y->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),16)              = DEBDEREF((bus_sy6545_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),17)              = DEBDEREF((bus_sy6545_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),18)              = DEBDEREF((bus_sy6545_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),19)              = DEBDEREF((bus_sy6545_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),20)              = DEBDEREF((bus_sy6545_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_16bit),21)              = DEBDEREF((bus_sy6545_addr->bus_16bit),0);
    DEBDEREF((sy6545_base->bus_32bit),0)               = DEBDEREF((bus_cnt_lpen->bus_32bit),0);
    DEBDEREF((sy6545_base->bus_32bit),1)               = DEBDEREF((bus_cnt_update->bus_32bit),0);
    DEBDEREF((sy6545_base->bus_32bit),2)               = DEBDEREF((bus_lpen_callmask->bus_32bit),0);
    DEBDEREF((sy6545_base->sig_calls_outof_module),0)  = DEBDEREF((bee_interf->sig_calls_into_module),0);
    DEBDEREF((sy6545_base->sig_calls_outof_module),1)  = DEBDEREF((bee_interf->sig_calls_into_module),1);
    DEBDEREF((sy6545_base->sig_calls_outof_module),2)  = DEBDEREF((bee_interf->sig_calls_into_module),2);
    DEBDEREF((sy6545_base->sig_calls_outof_module),3)  = DEBDEREF((bee_interf->sig_calls_into_module),3);
    DEBDEREF((sy6545_base->sig_calls_outof_module),4)  = DEBDEREF((bee_interf->sig_calls_into_module),4);
    DEBDEREF((sy6545_base->sig_calls_outof_module),5)  = DEBDEREF((bee_interf->sig_calls_into_module),5);
    DEBDEREF((sy6545_base->sig_calls_outof_module),6)  = DEBDEREF((bee_interf->sig_calls_into_module),6);
    DEBDEREF((sy6545_base->sig_calls_outof_module),7)  = DEBDEREF((mem_lpen_table->sig_calls_into_module),5);
    DEBDEREF((sy6545_base->sig_calls_outof_module),8)  = DEBDEREF((mem_lpen_table->sig_calls_into_module),5);
    DEBDEREF((sy6545_base->sig_calls_outof_module),9)  = DEBDEREF((mem_lpen_feedback->sig_calls_into_module),8);
    DEBDEREF((sy6545_base->sig_calls_outof_module),10) = DEBDEREF((mem_lpen_feedback->sig_calls_into_module),8);
    DEBDEREF((sy6545_base->sig_calls_outof_module),11) = DEBDEREF((mem_lpen_feedrfsh->sig_calls_into_module),8);
    DEBDEREF((sy6545_base->sig_calls_outof_module),12) = DEBDEREF((mem_lpen_feedrfsh->sig_calls_into_module),8);
    DEBDEREF((sy6545_base->sig_calls_outof_args),0)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),1)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),2)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),3)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),4)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),5)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),6)    = (void *) bee_interf;
    DEBDEREF((sy6545_base->sig_calls_outof_args),7)    = (void *) mem_lpen_table;
    DEBDEREF((sy6545_base->sig_calls_outof_args),8)    = (void *) mem_lpen_table;
    DEBDEREF((sy6545_base->sig_calls_outof_args),9)    = (void *) mem_lpen_feedback;
    DEBDEREF((sy6545_base->sig_calls_outof_args),10)   = (void *) mem_lpen_feedback;
    DEBDEREF((sy6545_base->sig_calls_outof_args),11)   = (void *) mem_lpen_feedrfsh;
    DEBDEREF((sy6545_base->sig_calls_outof_args),12)   = (void *) mem_lpen_feedrfsh;

    DEBDEREF((z80pio_base->bus_8bit),0)               = DEBDEREF((bus_pio_ieo->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),1)               = DEBDEREF((bus_pio_iei->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),2)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),3)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),4)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),5)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),6)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),7)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),8)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),9)               = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),10)              = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),11)              = DEBDEREF((bus_pio_a_rdy->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),12)              = DEBDEREF((bus_pio_a_strb->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),13)              = DEBDEREF((bus_pio_a_data->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),14)              = DEBDEREF((bus_pio_b_rdy->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),15)              = DEBDEREF((bus_pio_b_strb->bus_8bit),0);
    DEBDEREF((z80pio_base->bus_8bit),16)              = DEBDEREF((bus_pio_b_data->bus_8bit),0);
    DEBDEREF((z80pio_base->sig_calls_outof_module),0) = DEBDEREF((bee_interf->sig_calls_into_module),7);
    DEBDEREF((z80pio_base->sig_calls_outof_module),1) = DEBDEREF((do_pio_b_rdy_data_out->sig_calls_into_module),0);
    DEBDEREF((z80pio_base->sig_calls_outof_module),3) = DEBDEREF((z80cpu_base->sig_calls_into_module),3);
    DEBDEREF((z80pio_base->sig_calls_outof_args),0)   = (void *) bee_interf;
    DEBDEREF((z80pio_base->sig_calls_outof_args),1)   = (void *) do_pio_b_rdy_data_out;
    DEBDEREF((z80pio_base->sig_calls_outof_args),3)   = (void *) z80cpu_base;

    DEBDEREF((z80cpu_base->bus_8bit),0)                = DEBDEREF((bus_z80_wait->bus_8bit),0);
    DEBDEREF((z80cpu_base->bus_8bit),1)                = DEBDEREF((bus_z80_rfsh->bus_8bit),0);
    DEBDEREF((z80cpu_base->bus_8bit),2)                = DEBDEREF((bus_z80_data->bus_8bit),0);
    DEBDEREF((z80cpu_base->bus_8bit),3)                = DEBDEREF((bus_z80_tab_num_start->bus_8bit),0);
    DEBDEREF((z80cpu_base->bus_8bit),4)                = DEBDEREF((bus_z80_tab_num_finish->bus_8bit),0);
    DEBDEREF((z80cpu_base->bus_16bit),0)               = DEBDEREF((bus_z80_addr->bus_16bit),0);
    DEBDEREF((z80cpu_base->bus_16bit),1)               = DEBDEREF((bus_z80_tab_wr_wait->bus_16bit),0);
    DEBDEREF((z80cpu_base->bus_16bit),2)               = DEBDEREF((bus_z80_tab_rd_wait->bus_16bit),0);
    DEBDEREF((z80cpu_base->bus_32bit),0)               = DEBDEREF((bus_z80_reti_count->bus_32bit),0);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),0)  = &stop_emulator;
    DEBDEREF((z80cpu_base->sig_calls_outof_module),1)  = DEBDEREF((do_z80_ack_reset->sig_calls_into_module),0);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),5)  = DEBDEREF((do_z80_ack_INT->sig_calls_into_module),0);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),6)  = DEBDEREF((do_startup_ramtest->sig_calls_into_module),0);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),7)  = DEBDEREF((branch_memwr->sig_calls_into_module),1);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),8)  = DEBDEREF((do_startup_ramtest->sig_calls_into_module),0);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),9)  = DEBDEREF((jtable_io_wr__base->sig_calls_into_module),1);
    DEBDEREF((z80cpu_base->sig_calls_outof_module),10) = DEBDEREF((jtable_io_rd__base->sig_calls_into_module),1);
    DEBDEREF((z80cpu_base->sig_calls_outof_args),0)    = NULL;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),1)    = (void *) do_z80_ack_reset;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),5)    = (void *) do_z80_ack_INT;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),6)    = (void *) do_startup_ramtest;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),7)    = (void *) branch_memwr;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),8)    = (void *) do_startup_ramtest;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),9)    = (void *) jtable_io_wr__base;
    DEBDEREF((z80cpu_base->sig_calls_outof_args),10)   = (void *) jtable_io_rd__base;

    /*
       Finalise modules
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"finalise modules\n");
    #endif

    andconstmod_go(mask_colback);
    andconstmod_go(mask_colctrl);
    andconstmod_go(mask_pio_b);
    andconstmod_go(mask_romread);
    andconstmod_go(mask_soundbit);
    andconstmod_go(mask_tapeout);
    andconstmod_go(mask_video_mem_addr);
    andconstmod_go(mask_video_mem_slfaddr);
    andconstmod_go(mask_video_data);
    andconstmod_go(mask_video_charline);
    assignmod_go(assign_colback);
    assignmod_go(assign_colctrl);
    assignmod_go(assign_romread);
    assignmod_go(assign_video_data);
    assignmod_go(assign_video_charline);
    assignconstmod_go(assign_lpenmask0);
    assignconstmod_go(assign_lpenmask1);
    busmod_go(bus_cnt_lpen);
    busmod_go(bus_cnt_update);
    busmod_go(bus_col_back);
    busmod_go(bus_col_fore);
    busmod_go(bus_col_inv);
    busmod_go(bus_col_isfore);
    busmod_go(bus_colback);
    busmod_go(bus_colctrl);
    busmod_go(bus_cputabsel);
    busmod_go(bus_geom);
    busmod_go(bus_geom_pos_x);
    busmod_go(bus_geom_pos_y);
    busmod_go(bus_lpen_callmask);
    busmod_go(bus_new_colback);
    busmod_go(bus_new_colctrl);
    busmod_go(bus_new_romread);
    busmod_go(bus_pio_iei);
    busmod_go(bus_pio_ieo);
    busmod_go(bus_pio_a_data);
    busmod_go(bus_pio_a_rdy);
    busmod_go(bus_pio_a_strb);
    busmod_go(bus_pio_b_data);
    busmod_go(bus_pio_b_rdy);
    busmod_go(bus_pio_b_strb);
    busmod_go(bus_romread);
    busmod_go(bus_sound_bit);
    busmod_go(bus_sy6545_addr);
    busmod_go(bus_sy6545_data);
    busmod_go(bus_tape_in);
    busmod_go(bus_tape_out);
    busmod_go(bus_video_char_line);
    busmod_go(bus_video_data);
    busmod_go(bus_video_mem_addr);
    busmod_go(bus_z80_addr);
    busmod_go(bus_z80_data);
    busmod_go(bus_z80_reti_count);
    busmod_go(bus_z80_rfsh);
    busmod_go(bus_z80_tab_num_start);
    busmod_go(bus_z80_tab_num_finish);
    busmod_go(bus_z80_tab_rd_wait);
    busmod_go(bus_z80_tab_wr_wait);
    busmod_go(bus_z80_wait);
    domod_go(do_fixup_romread);
    domod_go(do_init_ramset);
    domod_go(do_reset_ramset);
    domod_go(do_romread_goes_high);
    domod_go(do_romread_goes_low);
    domod_go(do_startup_ramtest);
    domod_go(do_romread_wr);
    domod_go(do_z80_ack_INT);
    domod_go(do_z80_ack_reset);
    domod_go(do_vdu_ram_wr);
    domod_go(do_col_ram_wr);
    domod_go(do_pcg_ram_wr);
    domod_go(do_switch_in_col_ram);
    domod_go(do_switch_in_pcg_ram);
    domod_go(do_colback_change);
    domod_go(do_colctrl_change);
    domod_go(do_col_port_wr);
    domod_go(do_pio_b_rdy_data_out);
    domod_go(do_tape_strober);
    dowhilemod_go(dowhile_redo_bcolint);
    equalsmod_go(branch_if_romread_diff);
    equalsconstmod_go(branch_mem_startup);
    istruemod_go(branch_romread_change);
    istruemod_go(branch_vdu_ram_wr);
    istruemod_go(branch_sw_col_pcg);
    lessconstmod_go(branch_memwr);
    istruemod_go(branch_sub_colpcg);
    equalsmod_go(branch_diff_colback);
    equalsmod_go(branch_diff_colctrl);
    lut8mod_go(lut8_colour_table);
    memmod_go(mem_colour_ram);
    memmod_go(mem_lpen_feedback);
    memmod_go(mem_lpen_feedrfsh);
    memmod_go(mem_lpen_table);
    memmod_go(mem_pcg_ram);
    memmod_go(mem_rom1);
    memmod_go(mem_rom2);
    memmod_go(mem_rom3);
    memmod_go(mem_rom4);
    memmod_go(mem_rom5);
    memmod_go(mem_user_ram_a);
    memmod_go(mem_user_ram_b);
    memmod_go(mem_vdu_ram);
    ormod_go(or_backcol_backint);
    ormod_go(or_pio_b_tape);
    setbusmod_go(setbus_cpu_tab);
    table8mod_go(jtable_io_rd__base);
    table8mod_go(jtable_io_wr__base);

    interf_go(bee_interf);
    sy6545_go(sy6545_base);
    z80cpu_go(z80cpu_base);
    z80pio_go(z80pio_base);

    /*
       Install the timer function.
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"timer install\n");
    #endif

    if ( install_int_ex(speed_throttle,MSEC_TO_TIMER(timer_period_x/1000000)) )
    {
        return 14;
    }

    if ( install_int_ex(overlook_timer,SECS_TO_TIMER(OVERLOOK_TIMER_PERIOD)) )
    {
        return 15;
    }

    /*
       Start the emulator.  This will only exit when the emulation is
       finished, and the "off" signal is passed to the emulator.
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"enter emulator\n");
    #endif

    sync_clock();

    #ifdef DEBUGMODE
    fprintf(stderr,"exited emulator\n");
    #endif

    /*
       Remove timer interupt.
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"remove timer interupt\n");
    #endif

    remove_int(overlook_timer);
    remove_int(speed_throttle);

    /*
       Remove emulation modules in reverse order.
    */

    #ifdef DEBUGMODE
    fprintf(stderr,"remove modules\n");
    #endif

    andconstmod_stop(mask_colback);
    andconstmod_stop(mask_colctrl);
    andconstmod_stop(mask_pio_b);
    andconstmod_stop(mask_romread);
    andconstmod_stop(mask_soundbit);
    andconstmod_stop(mask_tapeout);
    andconstmod_stop(mask_video_mem_addr);
    andconstmod_stop(mask_video_mem_slfaddr);
    andconstmod_stop(mask_video_data);
    andconstmod_stop(mask_video_charline);
    assignmod_stop(assign_colback);
    assignmod_stop(assign_colctrl);
    assignmod_stop(assign_romread);
    assignmod_stop(assign_video_data);
    assignmod_stop(assign_video_charline);
    assignconstmod_stop(assign_lpenmask0);
    assignconstmod_stop(assign_lpenmask1);
    busmod_stop(bus_cnt_lpen);
    busmod_stop(bus_cnt_update);
    busmod_stop(bus_col_back);
    busmod_stop(bus_col_fore);
    busmod_stop(bus_col_inv);
    busmod_stop(bus_col_isfore);
    busmod_stop(bus_colback);
    busmod_stop(bus_colctrl);
    busmod_stop(bus_cputabsel);
    busmod_stop(bus_geom);
    busmod_stop(bus_geom_pos_x);
    busmod_stop(bus_geom_pos_y);
    busmod_stop(bus_lpen_callmask);
    busmod_stop(bus_new_colback);
    busmod_stop(bus_new_colctrl);
    busmod_stop(bus_new_romread);
    busmod_stop(bus_pio_iei);
    busmod_stop(bus_pio_ieo);
    busmod_stop(bus_pio_a_data);
    busmod_stop(bus_pio_a_rdy);
    busmod_stop(bus_pio_a_strb);
    busmod_stop(bus_pio_b_data);
    busmod_stop(bus_pio_b_rdy);
    busmod_stop(bus_pio_b_strb);
    busmod_stop(bus_romread);
    busmod_stop(bus_sound_bit);
    busmod_stop(bus_sy6545_addr);
    busmod_stop(bus_sy6545_data);
    busmod_stop(bus_tape_in);
    busmod_stop(bus_tape_out);
    busmod_stop(bus_video_char_line);
    busmod_stop(bus_video_data);
    busmod_stop(bus_video_mem_addr);
    busmod_stop(bus_z80_addr);
    busmod_stop(bus_z80_data);
    busmod_stop(bus_z80_reti_count);
    busmod_stop(bus_z80_rfsh);
    busmod_stop(bus_z80_tab_num_start);
    busmod_stop(bus_z80_tab_num_finish);
    busmod_stop(bus_z80_tab_rd_wait);
    busmod_stop(bus_z80_tab_wr_wait);
    busmod_stop(bus_z80_wait);
    domod_stop(do_fixup_romread);
    domod_stop(do_init_ramset);
    domod_stop(do_reset_ramset);
    domod_stop(do_romread_goes_high);
    domod_stop(do_romread_goes_low);
    domod_stop(do_startup_ramtest);
    domod_stop(do_romread_wr);
    domod_stop(do_z80_ack_INT);
    domod_stop(do_z80_ack_reset);
    domod_stop(do_vdu_ram_wr);
    domod_stop(do_col_ram_wr);
    domod_stop(do_pcg_ram_wr);
    domod_stop(do_switch_in_col_ram);
    domod_stop(do_switch_in_pcg_ram);
    domod_stop(do_colback_change);
    domod_stop(do_colctrl_change);
    domod_stop(do_col_port_wr);
    domod_stop(do_pio_b_rdy_data_out);
    domod_stop(do_tape_strober);
    dowhilemod_stop(dowhile_redo_bcolint);
    equalsmod_stop(branch_if_romread_diff);
    equalsconstmod_stop(branch_mem_startup);
    istruemod_stop(branch_romread_change);
    istruemod_stop(branch_vdu_ram_wr);
    istruemod_stop(branch_sw_col_pcg);
    lessconstmod_stop(branch_memwr);
    istruemod_stop(branch_sub_colpcg);
    equalsmod_stop(branch_diff_colback);
    equalsmod_stop(branch_diff_colctrl);
    lut8mod_stop(lut8_colour_table);
    memmod_stop(mem_colour_ram);
    memmod_stop(mem_lpen_feedback);
    memmod_stop(mem_lpen_feedrfsh);
    memmod_stop(mem_lpen_table);
    memmod_stop(mem_pcg_ram);
    memmod_stop(mem_rom1);
    memmod_stop(mem_rom2);
    memmod_stop(mem_rom3);
    memmod_stop(mem_rom4);
    memmod_stop(mem_rom5);
    memmod_stop(mem_user_ram_a);
    memmod_stop(mem_user_ram_b);
    memmod_stop(mem_vdu_ram);
    ormod_stop(or_backcol_backint);
    ormod_stop(or_pio_b_tape);
    setbusmod_stop(setbus_cpu_tab);
    table8mod_stop(jtable_io_rd__base);
    table8mod_stop(jtable_io_wr__base);

    interf_stop(bee_interf);
    sy6545_stop(sy6545_base);
    z80cpu_stop(z80cpu_base);
    z80pio_stop(z80pio_base);

    andconstmod_remove(mask_colback);
    andconstmod_remove(mask_colctrl);
    andconstmod_remove(mask_pio_b);
    andconstmod_remove(mask_romread);
    andconstmod_remove(mask_soundbit);
    andconstmod_remove(mask_tapeout);
    andconstmod_remove(mask_video_mem_addr);
    andconstmod_remove(mask_video_mem_slfaddr);
    andconstmod_remove(mask_video_data);
    andconstmod_remove(mask_video_charline);
    assignmod_remove(assign_colback);
    assignmod_remove(assign_colctrl);
    assignmod_remove(assign_romread);
    assignmod_remove(assign_video_data);
    assignmod_remove(assign_video_charline);
    assignconstmod_remove(assign_lpenmask0);
    assignconstmod_remove(assign_lpenmask1);
    busmod_remove(bus_cnt_lpen);
    busmod_remove(bus_cnt_update);
    busmod_remove(bus_col_back);
    busmod_remove(bus_col_fore);
    busmod_remove(bus_col_inv);
    busmod_remove(bus_col_isfore);
    busmod_remove(bus_colback);
    busmod_remove(bus_colctrl);
    busmod_remove(bus_cputabsel);
    busmod_remove(bus_geom);
    busmod_remove(bus_geom_pos_x);
    busmod_remove(bus_geom_pos_y);
    busmod_remove(bus_lpen_callmask);
    busmod_remove(bus_new_colback);
    busmod_remove(bus_new_colctrl);
    busmod_remove(bus_new_romread);
    busmod_remove(bus_pio_iei);
    busmod_remove(bus_pio_ieo);
    busmod_remove(bus_pio_a_data);
    busmod_remove(bus_pio_a_rdy);
    busmod_remove(bus_pio_a_strb);
    busmod_remove(bus_pio_b_data);
    busmod_remove(bus_pio_b_rdy);
    busmod_remove(bus_pio_b_strb);
    busmod_remove(bus_romread);
    busmod_remove(bus_sound_bit);
    busmod_remove(bus_sy6545_addr);
    busmod_remove(bus_sy6545_data);
    busmod_remove(bus_tape_in);
    busmod_remove(bus_tape_out);
    busmod_remove(bus_video_char_line);
    busmod_remove(bus_video_data);
    busmod_remove(bus_video_mem_addr);
    busmod_remove(bus_z80_addr);
    busmod_remove(bus_z80_data);
    busmod_remove(bus_z80_reti_count);
    busmod_remove(bus_z80_rfsh);
    busmod_remove(bus_z80_tab_num_start);
    busmod_remove(bus_z80_tab_num_finish);
    busmod_remove(bus_z80_tab_rd_wait);
    busmod_remove(bus_z80_tab_wr_wait);
    busmod_remove(bus_z80_wait);
    domod_remove(do_fixup_romread);
    domod_remove(do_init_ramset);
    domod_remove(do_reset_ramset);
    domod_remove(do_romread_goes_high);
    domod_remove(do_romread_goes_low);
    domod_remove(do_startup_ramtest);
    domod_remove(do_romread_wr);
    domod_remove(do_z80_ack_INT);
    domod_remove(do_z80_ack_reset);
    domod_remove(do_vdu_ram_wr);
    domod_remove(do_col_ram_wr);
    domod_remove(do_pcg_ram_wr);
    domod_remove(do_switch_in_col_ram);
    domod_remove(do_switch_in_pcg_ram);
    domod_remove(do_colback_change);
    domod_remove(do_colctrl_change);
    domod_remove(do_col_port_wr);
    domod_remove(do_pio_b_rdy_data_out);
    domod_remove(do_tape_strober);
    dowhilemod_remove(dowhile_redo_bcolint);
    equalsmod_remove(branch_if_romread_diff);
    equalsconstmod_remove(branch_mem_startup);
    istruemod_remove(branch_romread_change);
    istruemod_remove(branch_vdu_ram_wr);
    istruemod_remove(branch_sw_col_pcg);
    lessconstmod_remove(branch_memwr);
    istruemod_remove(branch_sub_colpcg);
    equalsmod_remove(branch_diff_colback);
    equalsmod_remove(branch_diff_colctrl);
    lut8mod_remove(lut8_colour_table);
    memmod_remove(mem_colour_ram);
    memmod_remove(mem_lpen_feedback);
    memmod_remove(mem_lpen_feedrfsh);
    memmod_remove(mem_lpen_table);
    memmod_remove(mem_pcg_ram);
    memmod_remove(mem_rom1);
    memmod_remove(mem_rom2);
    memmod_remove(mem_rom3);
    memmod_remove(mem_rom4);
    memmod_remove(mem_rom5);
    memmod_remove(mem_user_ram_a);
    memmod_remove(mem_user_ram_b);
    memmod_remove(mem_vdu_ram);
    ormod_remove(or_backcol_backint);
    ormod_remove(or_pio_b_tape);
    setbusmod_remove(setbus_cpu_tab);
    table8mod_remove(jtable_io_rd__base);
    table8mod_remove(jtable_io_wr__base);

    interf_remove(bee_interf);
    sy6545_remove(sy6545_base);
    z80cpu_remove(z80cpu_base);
    z80pio_remove(z80pio_base);

    return 0;
}
END_OF_MAIN()





/**********************************************************************

                           Core of emulator
                           ================

The sync_clock function provides linking between the various emulator
modules.  The z80 cpu emulation is central - every so often, if will call
this to say "I've done clk_bus clock cycles, time to let the other
emulator modules catch up".  This function will make take other emulation
blocks through an appropriate number of cycles each, allowing them to
update there internal states and make relevant calls (ie. update the screen,
check the ports, etc.).

                          Clock synchronisation
                          =====================

For each clock cycle of the z80, the 6545 through 1/2 a clock cycle.
However, this results in very slow emulation.  To overcome this problems,
some cheating is necessary.

crtc_granularity: the 6545 clock synchronisation function is only called
                  when the number of accumulated 6545 clock cycles reaches
                  this number.

These can be set one of two values.  When no (relevant) keys are down,
they are set to the _no_key values, which can be quite large, meaning that
the screen updates are slowed.  As there are no keys down, this should not
affect the operation of the microbee (which uses the 6545 to test for keys)
overly much.

If a relevant key is pressed, however, this is throttled back to true (no
granularity, no division) operation, with the 6545 running at true (1/2
cpu clock speed) speed.  Thus the keyboard operation is not impeeded.

These values have been selected by trial and error.


**********************************************************************/

void sync_clock(void)
{
    UINT_16 clk_bus                    = 0;
    UINT_32 local_sy6545_cycle_counter = 0;
    UINT_32 sy6545_clk_odds            = 0;
    UINT_16 cycle_counter_sy6545_bus   = 0;
    int local_sync_point;

    while ( mbee_power_flag )
    {
        clk_bus += 4;

        local_sync_point = sync_point;
        sync_point       = 0;

        z80cpu_cycle(z80cpu_base,clk_bus,1,local_sync_point);

        /*
           Microbee clock sync section.
        */

        if ( do_throttle )
        {
            actual_clocks -= clk_bus;

            if ( actual_clocks < -catchup_point )
            {
                is_wait = 1;

                while ( actual_clocks < 0 )
                {
                    if ( !do_throttle )
                    {
                        break;
                    }
                }
            }
        }

        is_wait = 0;

        /*
           6545 clock

           - Add any "left over" clock cycles to the clk_bus counter.
           - Round the clock count to half the z80 cpu frequency and save
             any leftovers for next time.
           - Halve the count.
           - update the 6545 cycle counter.
        */

        local_sy6545_cycle_counter += ( (clk_bus+sy6545_clk_odds) / CRTC6545_RELATIVE_CLOCK_RATE );
        sy6545_clk_odds             = ( (clk_bus+sy6545_clk_odds) % CRTC6545_RELATIVE_CLOCK_RATE );

        if ( local_sy6545_cycle_counter > crtc_granularity )
        {
            cycle_counter_sy6545_bus    = local_sy6545_cycle_counter / crtc_clock_division;
            local_sy6545_cycle_counter -= cycle_counter_sy6545_bus * crtc_clock_division;

            sy6545_cycle(sy6545_base,cycle_counter_sy6545_bus,crtc_clock_division,local_sync_point);
        }

        /*
           Interface cycling
        */

        interf_cycle(bee_interf,clk_bus,1,local_sync_point);

        /*
           Clear the clock cycle bus
        */

        clk_bus = 0;

        if ( mbee_reset_flag )
        {
            /*
               The reset key has been pressed.  Pass this signal back to the z80
               emulation module.  This module will subsequently call the correct
               reset function.
            */

            (DEBDEREF((z80cpu_base->sig_calls_into_module),0))((void *) z80cpu_base);
        }

        /*
           Finally, some pio stuff to take care of - if a reti instruction has
           occured, we need to tell the z80 pio about it.
        */

        if ( (*(DEBDEREF((bus_z80_reti_count->bus_32bit),0))) )
        {
            (*(DEBDEREF((bus_z80_reti_count->bus_32bit),0))) = 0;

            (DEBDEREF((z80pio_base->sig_calls_into_module),13))((void *) z80pio_base);
        }
    }

    return;
}











/**********************************************************************
 ***                                                                ***
 ***                    Callback functions                          ***
 ***                                                                ***
 **********************************************************************/


void timer_speed_emul_off(void *what)
{
    do_throttle   = 0;
    actual_clocks = 0;

    temp_crtc_granularity    = crtc_granularity;
    temp_crtc_clock_division = crtc_clock_division;

    #ifdef FAST_IS_SLOW
    crtc_granularity    = REAL_CRTC_GRANULARITY;
    crtc_clock_division = REAL_CRTC_CLOCK_DIV;
    #endif

    #ifndef FAST_IS_SLOW
    crtc_granularity    = max_crtc_granularity;
    crtc_clock_division = max_crtc_clock_division;
    #endif

    return;

    what = NULL;
}

void timer_speed_emul_on(void *what)
{
    do_throttle   = 1;
    actual_clocks = 0;

    crtc_granularity    = temp_crtc_granularity;
    crtc_clock_division = temp_crtc_clock_division;

    return;

    what = NULL;
}

void stop_emulator(void *what)
{
    /*
       Machine power off, due to either an emulation error or simple exit.
    */

    mbee_power_flag = 0;

    return;

    what = NULL;
}


void pause_emulation(void *what)
{
    is_paused = 1;

    return;

    what = NULL;
}

void restart_emulation(void *what)
{
    is_paused = 0;

    return;

    what = NULL;
}

void set_reset_flag(void *what)
{
    mbee_reset_flag = 1;

    return;

    what = NULL;
}

void clear_reset_flag(void *what)
{
    mbee_reset_flag = 0;

    return;

    what = NULL;
}










/*
((bus_cputabsel->sig_calls_into_module)[0--4])((void *) bus_cputabsel);
((setbus_cpu_tab->sig_calls_into_module)[0])((void *) setbus_cpu_tab);

0 = user_ram_a
1 = user_ram_b
2 = vdu_ram
3 = pcg_ram
4 = colour_ram
5 = rom1
6 = rom2
7 = rom3
8 = rom4
9 = rom5


module_data *bus_cputabsel;

module_data *setbus_cpu_tab;

(setbus_cpu_tab->var_32bit)[0] = 10;
(setbus_cpu_tab->var_32bit)[3] = 5;

(setbus_cpu_tab->modptrs)[0] = z80cpu_base;

(setbus_cpu_tab->bus_8bit)[0]  = (bus_cputabsel->bus_8bit)[0];
(setbus_cpu_tab->bus_8bit)[3]  = (mem_user_ram_a->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[4]  = (mem_user_ram_b->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[5]  = (mem_vdu_ram->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[6]  = (mem_pcg_ram->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[7]  = (mem_colour_ram->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[8]  = (mem_rom1->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[9]  = (mem_rom2->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[10] = (mem_rom3->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[11] = (mem_rom4->bus_8bit)[2];
(setbus_cpu_tab->bus_8bit)[12] = (mem_rom5->bus_8bit)[2];





   VDU RAM arrangement
   ===================

   bit 0-6: character number
   bit 7:   source bit -  0 = character ROM
                          1 = PCG RAM


   Colour RAM:
   ===========

   Colour on the 32k is "odd" and not fully specified in any of the docs I
   have.  Each colour line (Red, Green or Blue) is specified by 2 bits,
   namely data (0 = off, 1 = on) and intensity (0 = half, 1 = full), giving
   a total of 6 TTL lines / pixel.  So for each of colour line we have 3
   states:

    DATA   INTENSITY
     0      x            no colour (gun off)
     1      0            half intensity colour (gun half on)
     1      1            full intensity colour (gun full on)

   (giving 27 distinguishable shadings, but probably easier to round off
   to 6 bits, or 64 (0x040) colours with redundant blacknesses).

   Each byte of colour RAM is thus:

       bit 0-4 foreground colour selection
       bit 5   "RED"   background data (1 = on)
       bit 6   "GREEN" background data (1 = on)
       bit 7   "BLUE"  background data (1 = on)

   Background intensity is the same for the entire screen, and is set by
   port 08, which is:

       bit 0:   not used
       bit 1:   "RED"   background intensity (1 = high)
       bit 2:   "GREEN" background intensity (1 = high)
       bit 3:   "BLUE"  background intensity (1 = high)
       bit 4-5: not used
       bit 6:   colour RAM enable (0 = PCG, 1 = colour, F800-FFFF).
       bit 7:   not used

   Going from the Microbee Basic reference manual for the premium series,
   section 27.1, the following table is given for "old" (pre premium)
   machines:

   Foreground Colour          Decimal code
   ---------------------------------------
   Black                          0
   Blue                           1
   Green                          2
   Cyan                           3
   Red                            4
   Magenta                        5
   Yellow                         6
   White                          7
   Black II                       16
   Blue II                        17
   Green II                       18
   Cyan II                        19
   Red II                         20
   Magenta II                     21
   Yellow II                      22
   White II                       23
   
   Background Colour          Decimal code          (added: RGB value)
   ---------------------------------------
   Black                         0                        000
   Red                           1                        100
   Green                         2                        010
   Yellow                        3                        110
   Blue                          4                        001
   Magenta                       5                        101
   Cyan                          6                        011
   White                         7                        111
   
   Which, after some snooping on what basic does with the colour command
   given these values, indicates that the background is as above, and for
   the foreground:

   bit 0:   "RED"   foreground
   bit 1:   "GREEN" foreground
   bit 2:   "BLUE"  foreground
   bit 3-4: ??

   Bits 3 and 4 remain a mystery - they must control intensity, and pattern
   association suggests that colours 8-15 and colours 24-31 are some
   duplicate of colours 0-7 and 16-23.  Not having a (working) colour
   monitor for my bee, I can't tell for sure, so my (baseless) *guess* is
   that colours 16-23 are high intensity (ie. all intensity bits high).
   The rest are just a guess - I've chosen 8-15 with RG intensity high,
   and 24-31 with RB intensity high.  This is done via a LUT.

   BASIC colour commands: colour  n - set foreground colour    0 <= n <= 31
                          colourb n - set background colour    0 <= n <= 7
                          colourm n - set background intensity 0 <= n <= 7




*/

/*
   Comms buses
   ===========

   bus_geom:            comms bus b/w the beescrn block and the 6545 module.
   bus_video_mem_addr:  6545 comms bus - address in 6545 video mem map.
   bus_video_data:      6545 data bus.
   bus_video_char_line: 6545 line bus - says which line of a char is being
                        refered to when writing to character RAM.

   bus_geom_pos_x: comms bus b/w beescrn block and 6545 module.
   bus_geom_pos_y: comms bus b/w beescrn block and 6545 module.
   bus_col_isfore: comms bus b/w beescrn block and 6545 module.
   bus_col_fore:   comms bus b/w beescrn block and 6545 module.
   bus_col_back:   comms bus b/w beescrn block and 6545 module.
   bus_col_inv:    comms bus b/w beescrn block and 6545 module.

   bus_pio_ieo:    (1 bit) ieo bus for z80 pio module.
   bus_pio_iei:    (1 bit) iei bus for z80 pio module.
   bus_pio_a_rdy:  (1 bit) port a ready bus for z80 pio module.
   bus_pio_a_strb: (1 bit) port a strobe bus for z80 pio module.
   bus_pio_a_data: (8 bit) port a data bus for z80 pio module.
   bus_pio_b_rdy:  (1 bit) port b ready bus for z80 pio module.
   bus_pio_b_strb: (1 bit) port b strobe bus for z80 pio module.
   bus_pio_b_data: (8 bit) port b data bus for z80 pio module.

   bus_sound_bit: (1 bit) used to communicate the state of the sound bit (ie.
                  pio port b bit 6) to the sound block

   bus_tape_out: (1 bit) comms the state of the tape out bit to tape block.
   bus_tape_in:  (1 bit) comms the state of the tape in bit from tape block.
*/

/*
                     Clocking and Speed Control
                     ==========================


   How it Works - Basics
   =====================

   The z80 emulator will call sync_clock after each op to report the
   number of cycles carried out by the virtual z80 during that op.  The
   sync_clock function will subtract this from the signed counter
   actual_clocks.  If this counter is below the threshold value
   -catchup_point (which is negative) then it will wait until it goes above
   0 before continuing.

   There is also an interupt function, speed_throttle, which is called every
   timer_period_x nanoseconds (where timer_period_x must be a mutliple of
   1000000, as interupts can only have millisecond granularity).  When this
   is called, the value timer_period_x/clock_period (which is the number
   of clock cycles which should have passed on a real microbee running with
   clock period clock_period nanoseconds over a period of timer_period_x
   nanoseconds) is added to actual_clocks.

   Thus, on average, if actual_clocks is negative then the emulator is
   getting ahead of itself - that is, virtual cycles are being done on the
   emulator faster than actual cycles would have been on a real microbee.
   Hence we should stop for a bit to slow things up.  If actual_clocks is
   positive things are moving too fast - see below for details on how this
   is fixed.

   So we want to keep actual_clocks as close to zero as possible, and can
   insert wait states to make this happen.  To summarise:

   actual_clocks > 0: emulation is running too slowly
   actual_clocks = 0: emulation is running at precisely the right speed
   actual_clocks < 0: emulation is running faster than required

   Of course, there is a certain granularity here - actual_clocks is not
   increased continually, but in bursts.  Hence we need some leeway in how
   stricly we control the rate of the emulator.  This is why a wait state
   is only inserted when actual_clocks goes below -catchup_point.


   When the Computer Can't Keep Up
   ===============================

   While there is some leeway in the actual_clocks counter, if it becomes
   excessive then clearly the computer just can't go fast enough.  This is
   detected when actual_clocks > max_clockovr cycles (at which point
   actual_clocks is reset to max_clockovr_pb to prevent a counter overflow).
   While there are no guarantees, there are some things that can be done to
   fix this to a certain degree.  In particular, much of the computational
   load in the emulator is taken by the 6545 module.  However, much of this
   may be sacrificed if necessary - after all, much of this time is spent
   scanning back and foward across the screen doing nothing productive!

   6545 operation can be "slowed" in thusly:

   - the rate at which the 6545 is ran may be reduced using
     max_crtc_clock_division.  The number of clock cycles the 6545 does is
     the number of z80 cycle divided by (2*max_crtc_clock_division) (of
     course, the 6545 needs to be told about this so the cursor rate can be
     increased to compensate).  This will result in some weirdness if the
     display is changing quickly, but it's a reasonable compromise.
   - the granularity of the 6545 can be changed.  Rather than calling the
     6545 "go through n cycles" function whenever there is a cycle to be
     done, it will accumulate then untill the number of cycles exceeds
     crtc_granularity, and then call to do them all at once.  This saves
     on function calls and setup times.

   So, when we detect that actual_clocks > max_clockovr the following is
   done:

   1. set actual_clocks = max_clockovr_pb
   2. if ctrc_granularity < max_ctrc_granularity then increase
      ctrc_granularity by one and finish.
   3. if crtc_clock_division < max_crtc_clock division then set
      crtc_granularity = REAL_CRTC_GRANULARITY, double crtc_clock_division
      and tell the 6545 module about the change.

   Temporary process loads may make the emulator think that the computer
   is slower than it truly is, causing permanent degraded performance for
   where only temporary was necessary.  To overcome this, when the emulator
   enters a "wait" loop the is_wait counter will be set to 1 (it is zero by
   default).  When speed_throttle is called, it tests to see if is_wait is
   greater than lag_point.  If not, is_wait is incremented and nothing is
   done - nothing will happen until the lag_point^th call to speed_throttle
   during a single wait loop.  Upon the lag_point^th call to speed_throttle,
   however, the emulator accuracy can most likely be increased safely to
   give more accurate emulation without causing trouble.  The following
   steps are followed:

   1. if ctrc_granularity > REAL_CRTC_GRANULARITY then decrease
      ctrc_granularity by one and finish.
   2. if crtc_clock_division_is_key > REAL_CRTC_CLOCK_DIV then set
      crtc_granularity = max_crtc_granularity, halve crtc_clock_division
      and tell the 6545 module about the change.

   In this way, a compromise should be reached so that the emulator will
   run fast enough to pass for a "real" microbee while also being as accurate
   as possible on the given computer/load.

   An added complication here is that the microbee keyboard hangs off the
   6545 module, using the lightpen function.  Unfortunately, while the
   effect of slowing the 6545 on display quality is not too bad, the
   effect on keyboard responce can be more severe.  For this reason, the
   clock division and granularity variables (and their upper bounds)
   come in two varieties, one for when no keys are pressed and one for
   when keys are pressed (which can be detected using the key_down variable).
   Whenever key_down changes, the granularity and clock division parts are
   changed to the correct set.

   Finally, there are some temp variables used when speed control is turned
   off altogether.


   Erata - Automatic accuracy tracking
   ===================================

   I've found that the interupt function in allegro is not altogether
   accurate, especially when requested interupts are of the order of a few
   milliseconds.  Hence a second level timer interupt has been included that
   works as follows:

   1. When speed_throttle is called, the throttle_call_count counter is
      incremented.  So throttle_call_count*timer_period_x should be the
      number of nanoseconds since throttle_call_count was last reset to
      zero.
   2. Every OVERLOOK_TIMER_PERIOD seconds (where OVERLOOK_TIMER_PERIOD
      should be of the order of 2 seconds), timer_period_x will be adjusted
      to make it represent the "real" timer period, averaged over
      OVERLOOK_TIMER_PERIOD seconds.  The counter throttle_call_count will
      also be reset at this point.  Hence:

      timer_period_x = t_overlook / throttle_call_count

   where t_overlook is the time (in nanoseconds) since the last call to the
   overlook timer, as measured using a non-allegro time system (see time.h).



   Functions
   =========

   timer_speed_emul_off: called to turn off speed emulation.
   timer_speed_emul_on:  called to turn on speed emulation.
   speed_throttle:       interupt function which controls speed.
*/
/*
                Memory Map and Basic IO Functionality
                =====================================


   Microbee Memory Map - Basics
   ============================

   Usually, the microbee memory map is:

   0000 - 3FFF: 16k user RAM (mem_user_ram_a).
   4000 - 7FFF: 16k user RAM (mem_user_ram_b).
   8000 - 9FFF: Microworld Level II Basic ROM part a.
   A000 - BFFF: Microworld Level II Basic ROM part b.
   C000 - DFFF: Either EDASM or WordBee.
   E000 - EFFF: Empty.
   F000 - F7FF: VDU RAM (romread = 0) and character ROM (romread = 1).
   F800 - FFFF: PCG RAM (colctrl = 0) and colour RAM (colctrl != 0), unless
                this isn't a colour bee, in which case always PCG.

   This is implemented as follows using pointers:

   0000 - 3FFF: mem_user_ram_a (points to allocated memory).
   4000 - 7FFF: mem_user_ram_b (points to allocated memory).
   8000 - 9FFF: mem_rom1 (basic_ROM_a by default).
   A000 - BFFF: mem_rom2 (basic_ROM_a by default).
   C000 - DFFF: mem_rom3 (either wordbee_ROM, edasm_ROM or forth_ROM).
   E000 - EFFF: mem_rom4 (network_ROM by default).
   F000 - F7FF: mem_vdu_ram (points to allocated memory) if romread = 0.
                mem_rom5 (CHAR_ROM by default) if romread = 1.
   F800 - FFFF: mem_pcg_ram (pt to alloc mem) if colctrl = 0 or no colour emu.
                mem_colour_ram if colctrl != 0 and colour emulation is on.


   NB: - RAM is persistent on reset.
       - For optimality reasons, most reads in the z80 emulator core
         read dwords rather than bytes (ie 32 bits, rather than 8).
         Hence, an additional 3 bytes are allocated to each memory block
         to ensure that reads from memory always read from allocated
         memory.
       - Where possible, the z80 core is instructed to directly access
         memory, bypassing the relevant calls.


   Microbee Memory Map - Control Data
   ==================================

   startup_flag: When the Microbee 32k is turned on (or reset), the z80
                 will start reading from PC = 0.  However, the microbee
                 will put NOPs (0) on the bus until the trigger address
                 (8000) is reached.
                 To simulate this, the startup_flag is initially 0, and
                 also 0 upon reset of emulator.  While this is 0, any
                 memory reads below (which will be indirect upon
                 startup/reset) will return 0 (NOP).  This will continue
                 until the trigger address 8000 is reached,
                 at which time startup_flag will be set to 1 and memory
                 access from 0-7FFF set to direct.
   bus_romread:  This controls what appears in the mem range F000-F7FF.
                 If romread = 0 then VDU RAM appears here.  Otherwise
                 character ROM is found in this range.  romread is also
                 used by the keyboard circuit in the microbee, as
                 described later.
   bus_colctrl:  This controls what appears in the mem range F800-FFFF.
                 If colctrl = 0 then PCG RAM appears here.  Otherwise
                 colour RAM is to be found.
   bus_colback:  background colour intensity information.
                 bit 0: RED   intensity (1 = full)
                 bit 1: GREEN intensity (1 = full)
                 bit 2: BLUE  intensity (1 = full)


   Microbee Port Assignments - Basics
   ==================================

   00/10:       PIO port A data port.
   01/11:       PIO port A control port.
   02/12:       PIO port B data port.
   03/13:       PIO port B control port.
   04-07/14-17: not used.
   08/18:       COLOUR control port (see below).
                bit 0: not used.
                bit 1: RED background intensity (1 = full).
                bit 2: GREEN background intensity (1 = full).
                bit 3: BLUE background intensity (1 = full).
                bit 4: not used.
                bit 5: not used.
                bit 6: Colour RAM enable (0 = PCG, 1 = colour).
                bit 7: not used.
   09/19:       Colour "Wait off" (not used^).
   0A/1A:       Extended addressing port (not used*).
   0B/1B:       bit 0: ROMREAD
                bit 1-7: not used
   0C/1C/0E/1E: 6545 CRTC address/status port.
   0D/1D/0F/1F: 6545 CRTC data port.
   44:          FDC command/status (not yet implemented).
   45:          FDC track register (not yet implemented).
   46:          FDC sector register (not yet implemented).
   47:          FDC data register (not yet implemented).
   48:          Controller select/side/DD latch (not yet implemented).

   Notes: ^ This *might* have had something to do with VDU RAM timing at
            some stage.  Pages C.32-33 of the Microbee Technical Manual
            (32k version) say that screen deglitching is done by inserting
            wait states into the CPU when access to video RAM is attempted
            during non-blanked display.  These wait states halt the CPU
            until the blanked region is reached, when operation may
            continue.  This port is meant to disable this wait mechanism,
            making timing predictable for disk io...
            HOWEVER: on pages C.23 of the same manual describes a video
            blanking method that simply blanks characters around the time
            of any write to video RAM, thus avoiding snow without inserting
            wait states.  The blanking signal is called F000*.  Inspection
            of the circuit diagrams in the manual shows no evidence of any
            wait state insertion or colour "wait off" signals (although
            port 09 is decoded, it appears to not be used).  There is,
            however, a signal F000*, which does act in the manner described
            on C.23, so it appears that the blanking method is used, and
            thus this port seems to be irrelevant.  In any case, the F000*
            method is simpler to implement and appears the better option
            from a design standpoint, so I assume it is used in this
            emulator.
          * This has something to do with S100, apparently - ie. really old
            bees.  Unfortunately this is all I know.


   Microbee Port Assignments - Data
   ================================

   io_wr_table: When a port is written to the lower byte of the address is
                used to select a function from this table, and the function
                found is called to complete the write.
   io_rd_table: The same thing, but for port read operations.


   Specific Microbee Port Callback Functions
   =========================================

   pio_signal_interupt: called by the pio to indicate an interupt
*/

