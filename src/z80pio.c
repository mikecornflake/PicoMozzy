
#include "z80pio.h"
#include "u_dtype.h"
#include "debmaloc.h"
#include "modules.h"


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

   NB: Data for the z80pio is somewhat sketchy, so the functionality of
       this emulator is not likely to be exactly accurate.  I have
       attempted to leave notes at the more important murky points and
       larger assumptions, but not being an expert, I've probably missed
       some.  Here are the main unknowns (to me):

       - what happens if port b is set to mode 2?
       - what happens if port a is mode 2 and port b is not set as required
         by the docs?
       - if an interupt is inhibitted, is this interupt kept in a queue
         to be sent once the inbibition is removed (which I have assumed)
         or is it just lost?
       - what if a byte written to the control byte doesn't match the
         templates given?  For example, what if I write xxxx1011, or
         xxxx1101?
       - what happens when an output, input or whatever cycle is not
         finished and the computer starts another, or changes the mode
         or similar?


   State data
   ==========

   *_reg_output:   output register
   *_reg_input:    input register
   *_reg_mode:     mode byte ab000000
                   ab = 00 mode 0 - output mode
                        01 mode 1 - input mode
                        10 mode 2 - bidirectional mode
                        11 mode 3 - bit mode
   *_reg_intvect:  interupt vector abcdefg0
   *_reg_ioctrl:   I/O register control byte.  For each bit:
                   0 = means output
                   1 = means input
   *_reg_intctrl:  interupt control byte - abc00000
                   a = 0 interupt disable
                       1 interupt enable
                   b = 0 interupt on OR function
                       1 interupt on AND function
                   c = 0 active level is LOW
                       1 active level is HIGH
   *_reg_maskctrl: mask control byte.  For each bit:
                   0 = monitor for interupt if input
                   1 = ignore

   *_regctrl_pending:  0 = normal
                       1 = next write to ctrl port is reg ctrl byte
   *_maskctrl_pending: 0 = normal
                       1 = next write to ctrl port is mask ctrl byte
   *_int_inhibit:      0 = normal
                       1 = ints inhibitted by higher priority device
   *_mode02_state:     0 = no data to send.
                       1 = data sent to destination, waiting for reply.
                       2 = got reply, want to send int, inhibitted.
                       3 = got reply, sent int, waiting for ack.
                       4 = got reply, sent int, waiting for ack, inhibitted.
                       5 = got reply, send int, received ack, wait for reti.
                       6 = got reply, send int, received ack, wait for reti,
                           inhibitted.
   *_mode123_state:    0 = no data received, none expected.
                       1 = asked for data, waiting for reply.
                       2 = data recd, want to send int, inhibitted.
                       3 = data recd, sent int, waiting for ack.
                       4 = data recd, sent int, waiting for ack, inhibitted.
                       5 = data recd, sent int, received ack, wait for reti.
                       6 = data recd, sent int, received ack, wait for reti,
                           inhibitted.
   *_strb_prime:       0 = normal.
                       1 = strb low, waiting for rising edge.





   Communication from the z80 pio to connected devices
   ===================================================

   z80pio_*_strb_data_in - function called to tell pio that relevant *
                           inputs (ie. the data bus and the strb signal)
                           may have been changed.
   z80pio_iei_in         - function called to tell the pio that the iei
                           signal has changed.
   z80pio_ack_int        - called to acknowledge an interupt signal has bee
                           received and action taken.  This will put the
                           relevant interupt vector on the data_bus.
   z80pio_reti_signal    - called when a reti opcode is processed by the
                           cpu.

   Data bus description
   ====================

   ie*      - interupt enable pins.  ieo is an output (to next pio in the
              daisy-chain), and iei is an input (from the previous pio in
              the daisy chain.  In general, 0 means don't inhibit interupts,
              1 means inhibit interupts.
   data_bus - data bus on cpu side.

   *_data - data bus on output side.
   *_rdy  - 0 pio not ready
            1 pio is ready
   *_strb - active low strobe signal


   Function pointer description
   ============================

   *_rdy_data_out  - function called if the relevant * outputs (ie. the data
                     bus or the ready signal) may have changed.
   ieo_out         - function called if the state of the ieo signal has
                     changed.
   signal_interupt - called to signal that an interupt has occured (see
                     below for details).


   Interupt stuff
   ==============

   Interupts function as follows:

   1. the pio sends the interupt by calling signal_interupt().
   2. this is passed to the z80 cpu module, which will (if interupts
      are enabled etc) call back to acknowledge the interupt.
   3. The function so called should then call z80pio_ack_int(), which will
      put the appropriate vector on the data_bus and return.
   4. Once the interupt routine is finished, it may end in reti.  Upon (or
      near) when the reti is got from memory the function z80pio_reti_signal
      should be called to finish the interupt cycle.

*/


typedef struct
{
    /* Internal state of A module (nominally read only) */

    UINT_8 a_reg_output;
    UINT_8 a_reg_input;
    UINT_8 a_reg_mode;
    UINT_8 a_reg_intvect;
    UINT_8 a_reg_ioctrl;
    UINT_8 a_reg_intctrl;
    UINT_8 a_reg_maskctrl;

    UINT_8 a_regctrl_pending;
    UINT_8 a_maskctrl_pending;
    UINT_8 a_int_inhibit;
    UINT_8 a_mode02_state;
    UINT_8 a_mode123_state;
    UINT_8 a_strb_prime;


    /* Internal state of B module (nominally read only) */

    UINT_8 b_reg_output;
    UINT_8 b_reg_input;
    UINT_8 b_reg_mode;
    UINT_8 b_reg_intvect;
    UINT_8 b_reg_ioctrl;
    UINT_8 b_reg_intctrl;
    UINT_8 b_reg_maskctrl;

    UINT_8 b_regctrl_pending;
    UINT_8 b_maskctrl_pending;
    UINT_8 b_int_inhibit;
    UINT_8 b_mode02_state;
    UINT_8 b_mode123_state;
    UINT_8 b_strb_prime;
}
z80pio_state;


void z80pio_reset(void *what);

void z80pio_data_wr_A(void *what);
void z80pio_ctrl_wr_A(void *what);

void z80pio_data_wr_B(void *what);
void z80pio_ctrl_wr_B(void *what);

void z80pio_data_rd_A(void *what);
void z80pio_ctrl_rd_A(void *what);

void z80pio_data_rd_B(void *what);
void z80pio_ctrl_rd_B(void *what);

void z80pio_a_strb_data_in(void *what);
void z80pio_b_strb_data_in(void *what);

void z80pio_iei_in(void *what);
void z80pio_ack_int(void *what);
void z80pio_reti_signal(void *what);



#define DEFAULT_Z80PIO_A_REG_OUTPUT     0x000
#define DEFAULT_Z80PIO_A_REG_INPUT      0x000
#define DEFAULT_Z80PIO_A_REG_MODE       0x001
#define DEFAULT_Z80PIO_A_REG_INTVECT    0x000
#define DEFAULT_Z80PIO_A_REG_IOCTRL     0x0FF
#define DEFAULT_Z80PIO_A_REG_INTCTRL    0x000
#define DEFAULT_Z80PIO_A_REG_MASKCTRL   0x000

#define DEFAULT_Z80PIO_A_REGCTRL_PENDING        0
#define DEFAULT_Z80PIO_A_MASKCTRL_PENDING       0
#define DEFAULT_Z80PIO_A_INT_INHIBIT            0

#define DEFAULT_Z80PIO_B_REG_OUTPUT     0x000
#define DEFAULT_Z80PIO_B_REG_INPUT      0x000
#define DEFAULT_Z80PIO_B_REG_MODE       0x001
#define DEFAULT_Z80PIO_B_REG_INTVECT    0x000
#define DEFAULT_Z80PIO_B_REG_IOCTRL     0x0FF
#define DEFAULT_Z80PIO_B_REG_INTCTRL    0x000
#define DEFAULT_Z80PIO_B_REG_MASKCTRL   0x000

#define DEFAULT_Z80PIO_B_REGCTRL_PENDING        0
#define DEFAULT_Z80PIO_B_MASKCTRL_PENDING       0
#define DEFAULT_Z80PIO_B_INT_INHIBIT            0

#define DEFAULT_Z80PIO_A_RDY    0
#define DEFAULT_Z80PIO_B_RDY    0
#define DEFAULT_Z80PIO_IEO      0


#define Z80PIO_IEO(what)        DEREF_8BUS(what,0)
#define Z80PIO_IEI(what)        DEREF_8BUS(what,1)

#define Z80PIO_DATA_BUS_A(what) DEREF_8BUS(what,2)
#define Z80PIO_DATA_BUS_B(what) DEREF_8BUS(what,3)
#define Z80PIO_DATA_BUS_C(what) DEREF_8BUS(what,4)
#define Z80PIO_DATA_BUS_D(what) DEREF_8BUS(what,5)
#define Z80PIO_DATA_BUS_E(what) DEREF_8BUS(what,6)
#define Z80PIO_DATA_BUS_F(what) DEREF_8BUS(what,7)
#define Z80PIO_DATA_BUS_G(what) DEREF_8BUS(what,8)
#define Z80PIO_DATA_BUS_H(what) DEREF_8BUS(what,9)
#define Z80PIO_DATA_BUS_I(what) DEREF_8BUS(what,10)

#define Z80PIO_A_RDY(what)      DEREF_8BUS(what,11)
#define Z80PIO_A_STRB(what)     DEREF_8BUS(what,12)
#define Z80PIO_A_DATA(what)     DEREF_8BUS(what,13)

#define Z80PIO_B_RDY(what)      DEREF_8BUS(what,14)
#define Z80PIO_B_STRB(what)     DEREF_8BUS(what,15)
#define Z80PIO_B_DATA(what)     DEREF_8BUS(what,16)

#define Z80PIO_A_RDY_DATA_OUT(what)     OUTFNCALL(what,0)
#define Z80PIO_B_RDY_DATA_OUT(what)     OUTFNCALL(what,1)
#define Z80PIO_IEO_OUT(what)            OUTFNCALL(what,2)
#define Z80PIO_SIGNAL_INTERUPT(what)    OUTFNCALL(what,3)

#define Z80PIO_A_REG_OUTPUT(what)   (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_output)
#define Z80PIO_A_REG_INPUT(what)    (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_input)
#define Z80PIO_A_REG_MODE(what)     (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_mode)
#define Z80PIO_A_REG_INTVECT(what)  (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_intvect)
#define Z80PIO_A_REG_IOCTRL(what)   (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_ioctrl)
#define Z80PIO_A_REG_INTCTRL(what)  (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_intctrl)
#define Z80PIO_A_REG_MASKCTRL(what) (((z80pio_state *) DEREF_INTERNAL(what))->a_reg_maskctrl)

#define Z80PIO_A_REGCTRL_PENDING(what)  (((z80pio_state *) DEREF_INTERNAL(what))->a_regctrl_pending)
#define Z80PIO_A_MASKCTRL_PENDING(what) (((z80pio_state *) DEREF_INTERNAL(what))->a_maskctrl_pending)
#define Z80PIO_A_INT_INHIBIT(what)      (((z80pio_state *) DEREF_INTERNAL(what))->a_int_inhibit)
#define Z80PIO_A_MODE02_STATE(what)     (((z80pio_state *) DEREF_INTERNAL(what))->a_mode02_state)
#define Z80PIO_A_MODE123_STATE(what)    (((z80pio_state *) DEREF_INTERNAL(what))->a_mode123_state)
#define Z80PIO_A_STRB_PRIME(what)       (((z80pio_state *) DEREF_INTERNAL(what))->a_strb_prime)

#define Z80PIO_B_REG_OUTPUT(what)   (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_output)
#define Z80PIO_B_REG_INPUT(what)    (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_input)
#define Z80PIO_B_REG_MODE(what)     (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_mode)
#define Z80PIO_B_REG_INTVECT(what)  (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_intvect)
#define Z80PIO_B_REG_IOCTRL(what)   (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_ioctrl)
#define Z80PIO_B_REG_INTCTRL(what)  (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_intctrl)
#define Z80PIO_B_REG_MASKCTRL(what) (((z80pio_state *) DEREF_INTERNAL(what))->b_reg_maskctrl)

#define Z80PIO_B_REGCTRL_PENDING(what)  (((z80pio_state *) DEREF_INTERNAL(what))->b_regctrl_pending)
#define Z80PIO_B_MASKCTRL_PENDING(what) (((z80pio_state *) DEREF_INTERNAL(what))->b_maskctrl_pending)
#define Z80PIO_B_INT_INHIBIT(what)      (((z80pio_state *) DEREF_INTERNAL(what))->b_int_inhibit)
#define Z80PIO_B_MODE02_STATE(what)     (((z80pio_state *) DEREF_INTERNAL(what))->b_mode02_state)
#define Z80PIO_B_MODE123_STATE(what)    (((z80pio_state *) DEREF_INTERNAL(what))->b_mode123_state)
#define Z80PIO_B_STRB_PRIME(what)       (((z80pio_state *) DEREF_INTERNAL(what))->b_strb_prime)


inline void z80pio_a_setup_0_mode(void *what);
inline void z80pio_a_setup_1_mode(void *what);
inline void z80pio_a_setup_2_mode(void *what);
inline void z80pio_a_setup_3_mode(void *what);

inline void z80pio_b_setup_0_mode(void *what);
inline void z80pio_b_setup_1_mode(void *what);
inline void z80pio_b_setup_3_mode(void *what);

inline void z80pio_a_mode_3_int_gen_test(void *what);
inline void z80pio_b_mode_3_int_gen_test(void *what);

void z80pio_inhibit_int_from_a(void *what);
void z80pio_inhibit_int_from_b(void *what);

void z80pio_uninhibit_int_from_a(void *what);
void z80pio_uninhibit_int_from_b(void *what);



module_data *z80pio_alloc(const char *module_name)
{
    module_data *result;

    result = gen_module_data(module_name,0,0,0,0,0,0,17,0,0,14,4);

    return result;
}

int z80pio_init(module_data *what)
{
    int result = 1;

    DEREF_INFN(what,0)  = z80pio_reset;
    DEREF_INFN(what,1)  = z80pio_data_wr_A;
    DEREF_INFN(what,2)  = z80pio_ctrl_wr_A;
    DEREF_INFN(what,3)  = z80pio_data_wr_B;
    DEREF_INFN(what,4)  = z80pio_ctrl_wr_B;
    DEREF_INFN(what,5)  = z80pio_data_rd_A;
    DEREF_INFN(what,6)  = z80pio_ctrl_rd_A;
    DEREF_INFN(what,7)  = z80pio_data_rd_B;
    DEREF_INFN(what,8)  = z80pio_ctrl_rd_B;
    DEREF_INFN(what,9)  = z80pio_a_strb_data_in;
    DEREF_INFN(what,10) = z80pio_b_strb_data_in;
    DEREF_INFN(what,11) = z80pio_iei_in;
    DEREF_INFN(what,12) = z80pio_ack_int;
    DEREF_INFN(what,13) = z80pio_reti_signal;

    if ( ( DEREF_INTERNAL(what) = (z80pio_state *) DEBMALLOC(sizeof(z80pio_state)) ) != NULL )
    {
        result = 0;
    }

    return result;
}

void z80pio_go(module_data *what)
{
    /*
       This will set most of the internal state.
    */

    z80pio_reset(what);

    /*
       These are unaffected by reset, so must be set separately on
       powerup.
    */

    Z80PIO_A_REG_INTVECT(what) = DEFAULT_Z80PIO_A_REG_INTVECT;
    Z80PIO_B_REG_INTVECT(what) = DEFAULT_Z80PIO_B_REG_INTVECT;

    return;
}

void z80pio_reset(void *what)
{
    /*
       Reset the pio state.  Note that the vector address registers are
       not reset by the reset signal on a real PIO, and hence are not
       changed here.  Also, mode 1 is a guess.
    */


    Z80PIO_A_REG_OUTPUT(what)   = DEFAULT_Z80PIO_A_REG_OUTPUT;
    Z80PIO_A_REG_INPUT(what)    = DEFAULT_Z80PIO_A_REG_INPUT;
    Z80PIO_A_REG_MODE(what)     = DEFAULT_Z80PIO_A_REG_MODE;
    Z80PIO_A_REG_IOCTRL(what)   = DEFAULT_Z80PIO_A_REG_IOCTRL;
    Z80PIO_A_REG_INTCTRL(what)  = DEFAULT_Z80PIO_A_REG_INTCTRL;
    Z80PIO_A_REG_MASKCTRL(what) = DEFAULT_Z80PIO_A_REG_MASKCTRL;

    Z80PIO_A_REGCTRL_PENDING(what)  = DEFAULT_Z80PIO_A_REGCTRL_PENDING;
    Z80PIO_A_MASKCTRL_PENDING(what) = DEFAULT_Z80PIO_A_MASKCTRL_PENDING;
    Z80PIO_A_INT_INHIBIT(what)      = DEFAULT_Z80PIO_A_INT_INHIBIT;

    Z80PIO_B_REG_OUTPUT(what)   = DEFAULT_Z80PIO_B_REG_OUTPUT;
    Z80PIO_B_REG_INPUT(what)    = DEFAULT_Z80PIO_B_REG_INPUT;
    Z80PIO_B_REG_MODE(what)     = DEFAULT_Z80PIO_B_REG_MODE;
    Z80PIO_B_REG_IOCTRL(what)   = DEFAULT_Z80PIO_B_REG_IOCTRL;
    Z80PIO_B_REG_INTCTRL(what)  = DEFAULT_Z80PIO_B_REG_INTCTRL;
    Z80PIO_B_REG_MASKCTRL(what) = DEFAULT_Z80PIO_B_REG_MASKCTRL;

    Z80PIO_B_REGCTRL_PENDING(what)  = DEFAULT_Z80PIO_B_REGCTRL_PENDING;
    Z80PIO_B_MASKCTRL_PENDING(what) = DEFAULT_Z80PIO_B_MASKCTRL_PENDING;
    Z80PIO_B_INT_INHIBIT(what)      = DEFAULT_Z80PIO_B_INT_INHIBIT;

    /* this is a guess, really */

    z80pio_a_setup_1_mode(what);
    z80pio_b_setup_1_mode(what);

    z80pio_iei_in(what);

    return;
}

void z80pio_stop(module_data *what)
{
    return;

    what = NULL;
}

void z80pio_remove(module_data *what)
{
    if ( what != NULL )
    {
        if ( DEREF_INTERNAL(what) != NULL )
        {
            DEBFREE(DEREF_INTERNAL(what));
        }

        free_module_data(what);
    }

    return;
}

void z80pio_cycle(module_data *what, UINT_16 num_cycles, UINT_8 clock_div, int lsync_point)
{
    return;

    what = NULL;
    num_cycles = 0;
    clock_div = 0;
    lsync_point = 0;
}

void z80pio_ctrl_wr_A(void *what)
{
    if ( Z80PIO_A_REGCTRL_PENDING(what) )
    {
        /*
           The last write set mode 3, so this must be the IO register
           control word.
        */

        Z80PIO_A_REGCTRL_PENDING(what) = 0;
        Z80PIO_A_REG_IOCTRL(what)      = Z80PIO_DATA_BUS_B(what);

        return;
    }

    if ( Z80PIO_A_MASKCTRL_PENDING(what) )
    {
        /*
           The last write was the interupt control word, and this indicated
           that the next write must be the mask control word.  So this must
           be it.
        */

        Z80PIO_A_MASKCTRL_PENDING(what) = 0;
        Z80PIO_A_REG_MASKCTRL(what)     = Z80PIO_DATA_BUS_B(what);

        /*
           If bit 4 was set then this also causes any pending interupts
           to be reset.  Hence the following code.
        */

        switch ( Z80PIO_A_REG_MODE(what) )
        {
            case 0x000: /* mode 0 set */
            {
                z80pio_a_setup_0_mode(what);

                break;
            }

            case 0x040: /* mode 1 set */
            {
                z80pio_a_setup_1_mode(what);

                Z80PIO_A_RDY(what)           = 0;
                Z80PIO_A_MODE123_STATE(what) = 1;

                Z80PIO_A_RDY_DATA_OUT(what);

                break;
            }

            case 0x080: /* mode 2 set */
            {
                z80pio_a_setup_2_mode(what);

                Z80PIO_B_RDY(what)           = 0;
                Z80PIO_B_MODE123_STATE(what) = 1;

                break;
            }

            default:    /* mode 3 set */
            {
                z80pio_a_setup_3_mode(what);

                break;
            }
        }

        return;
    }

    if ( Z80PIO_DATA_BUS_B(what) & 0x001 )
    {
        switch ( Z80PIO_DATA_BUS_B(what) & 0x00F )
        {
            case 0x00F: /* xxxx1111 == mode control word */
            {
                Z80PIO_A_REG_MODE(what) = Z80PIO_DATA_BUS_B(what) & 0x0C0;

                switch ( Z80PIO_A_REG_MODE(what) )
                {
                    case 0x000: /* mode 0 set */
                    {
                        z80pio_a_setup_0_mode(what);

                        break;
                    }

                    case 0x040: /* mode 1 set */
                    {
                        z80pio_a_setup_1_mode(what);

                        Z80PIO_A_RDY(what)           = 0;
                        Z80PIO_A_MODE123_STATE(what) = 1;

                        Z80PIO_A_RDY_DATA_OUT(what);

                        break;
                    }

                    case 0x080: /* mode 2 set */
                    {
                        z80pio_a_setup_2_mode(what);

                        Z80PIO_B_RDY(what)           = 0;
                        Z80PIO_B_MODE123_STATE(what) = 1;

                        break;
                    }

                    default:    /* mode 3 set */
                    {
                        z80pio_a_setup_3_mode(what);
                        z80pio_a_mode_3_int_gen_test(what);

                        Z80PIO_A_REGCTRL_PENDING(what) = 1;

                        break;
                    }
                }

                break;
            }

            case 0x007: /* xxxx0111 == interupt control word */
            {
                Z80PIO_A_MASKCTRL_PENDING(what) = Z80PIO_DATA_BUS_B(what) & 0x010;
                Z80PIO_A_REG_INTCTRL(what)      = Z80PIO_DATA_BUS_B(what) & 0x0E0;

                break;
            }

            case 0x003: /* xxxx0011 == interupt disable word */
            {
                Z80PIO_A_REG_INTCTRL(what) &= 0x07F;
                Z80PIO_A_REG_INTCTRL(what) |= ( Z80PIO_DATA_BUS_B(what) & 0x080 );

                break;
            }

            default:   /* unknown control word */
            {
                /*
                   Presumably, this just gets ignored (don't know for sure,
                   though).
                */

                break;
            }
        }
    }

    else
    {
        /*
           LSB == 0, indicating that this must be an interupt vector word.
        */

        Z80PIO_A_REG_INTVECT(what) = Z80PIO_DATA_BUS_B(what);
    }

    return;
}

void z80pio_ctrl_wr_B(void *what)
{
    if ( Z80PIO_B_REGCTRL_PENDING(what) )
    {
        /*
           The last write set mode 3, so this must be the IO register
           control word.
        */

        Z80PIO_B_REGCTRL_PENDING(what) = 0;
        Z80PIO_B_REG_IOCTRL(what)      = Z80PIO_DATA_BUS_D(what);

        return;
    }

    if ( Z80PIO_B_MASKCTRL_PENDING(what) )
    {
        /*
           The last write was the interupt control word, and this indicated
           that the next write must be the mask control word.  So this must
           be it.
        */

        Z80PIO_B_MASKCTRL_PENDING(what) = 0;
        Z80PIO_B_REG_MASKCTRL(what)     = Z80PIO_DATA_BUS_D(what);

        /*
           If bit 4 was set then this also causes any pending interupts
           to be reset.  Hence the following code.
        */

        switch ( Z80PIO_B_REG_MODE(what) )
        {
            case 0x000: /* mode 0 set */
            {
                z80pio_b_setup_0_mode(what);

                break;
            }

            case 0x040: /* mode 1 set */
            {
                z80pio_b_setup_1_mode(what);

                Z80PIO_B_RDY(what)           = 0;
                Z80PIO_B_MODE123_STATE(what) = 1;

                Z80PIO_B_RDY_DATA_OUT(what);

                break;
            }

            case 0x080: /* mode 2 set (not allowed?) */
            {
                break;
            }

            default:    /* mode 3 set */
            {
                z80pio_b_setup_3_mode(what);

                break;
            }
        }

        return;
    }

    if ( Z80PIO_DATA_BUS_D(what) & 0x001 )
    {
        switch ( Z80PIO_DATA_BUS_D(what) & 0x00F )
        {
            case 0x00F: /* xxxx1111 == mode control word */
            {
                Z80PIO_B_REG_MODE(what) = Z80PIO_DATA_BUS_D(what) & 0x0C0;

                switch ( Z80PIO_B_REG_MODE(what) )
                {
                    case 0x000: /* mode 0 set */
                    {
                        z80pio_b_setup_0_mode(what);

                        break;
                    }

                    case 0x040: /* mode 1 set */
                    {
                        z80pio_b_setup_1_mode(what);

                        Z80PIO_B_RDY(what)           = 0;
                        Z80PIO_B_MODE123_STATE(what) = 1;

                        Z80PIO_B_RDY_DATA_OUT(what);

                        break;
                    }

                    case 0x080: /* mode 2 set (not allowed?) */
                    {
                        break;
                    }

                    default:    /* mode 3 set */
                    {
                        z80pio_b_setup_3_mode(what);
                        z80pio_b_mode_3_int_gen_test(what);

                        Z80PIO_B_REGCTRL_PENDING(what) = 1;

                        break;
                    }
                }

                break;
            }

            case 0x007: /* xxxx0111 == interupt control word */
            {
                Z80PIO_B_MASKCTRL_PENDING(what) = Z80PIO_DATA_BUS_D(what) & 0x010;
                Z80PIO_B_REG_INTCTRL(what)      = Z80PIO_DATA_BUS_D(what) & 0x0E0;

                break;
            }

            case 0x003: /* xxxx0011 == interupt disable word */
            {
                Z80PIO_B_REG_INTCTRL(what) &= 0x07F;
                Z80PIO_B_REG_INTCTRL(what) |= ( Z80PIO_DATA_BUS_D(what) & 0x080 );

                break;
            }

            default:   /* unknown control word */
            {
                /*
                   Presumably, this just gets ignored (don't know for sure,
                   though).
                */

                break;
            }
        }
    }

    else
    {
        /*
           LSB == 0, indicating that this must be an interupt vector word.
        */

        Z80PIO_B_REG_INTVECT(what) = Z80PIO_DATA_BUS_D(what);
    }

    return;
}

void z80pio_ctrl_rd_A(void *what)
{
    /* this is a guess */

    Z80PIO_DATA_BUS_F(what) = 0;

    return;
}

void z80pio_ctrl_rd_B(void *what)
{
    /* this is a guess */

    Z80PIO_DATA_BUS_H(what) = 0;

    return;
}

void z80pio_data_wr_A(void *what)
{
    switch ( Z80PIO_A_REG_MODE(what) )
    {
        case 0x000:
        {
            Z80PIO_A_REG_OUTPUT(what) = Z80PIO_DATA_BUS_A(what);

            z80pio_a_setup_0_mode(what);

            Z80PIO_A_RDY(what)          = 1;
            Z80PIO_A_DATA(what)         = Z80PIO_A_REG_OUTPUT(what);
            Z80PIO_A_MODE02_STATE(what) = 1;
            Z80PIO_A_STRB_PRIME(what)   = 0;

            Z80PIO_A_RDY_DATA_OUT(what);

            break;
        }

        case 0x040:
        {
            Z80PIO_A_REG_OUTPUT(what) = Z80PIO_DATA_BUS_A(what);

            break;
        }

        case 0x080:
        {
            Z80PIO_A_REG_OUTPUT(what) = Z80PIO_DATA_BUS_A(what);

            z80pio_a_setup_2_mode(what);

            Z80PIO_A_RDY(what)          = 1;
            /* Data doesn't get put on the lines until strobe goes low */
            Z80PIO_A_MODE02_STATE(what) = 1;
            Z80PIO_A_STRB_PRIME(what)   = 0;

            Z80PIO_A_RDY_DATA_OUT(what);

            break;
        }

        case 0x0C0:
        {
            Z80PIO_A_REG_OUTPUT(what)  = Z80PIO_DATA_BUS_A(what);
            Z80PIO_A_REG_OUTPUT(what) &= ( Z80PIO_A_REG_IOCTRL(what) ^ 0x0ff );
            Z80PIO_A_REG_OUTPUT(what) |= ( Z80PIO_A_REG_INPUT(what) & ( Z80PIO_A_REG_IOCTRL(what) ) );
            Z80PIO_A_REG_INPUT(what)   = Z80PIO_A_REG_OUTPUT(what);

            z80pio_a_setup_3_mode(what);

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_data_wr_B(void *what)
{
    switch ( Z80PIO_B_REG_MODE(what) )
    {
        case 0x000:
        {
            Z80PIO_B_REG_OUTPUT(what) = Z80PIO_DATA_BUS_C(what);

            z80pio_b_setup_0_mode(what);

            Z80PIO_B_RDY(what)          = 1;
            Z80PIO_B_DATA(what)         = Z80PIO_B_REG_OUTPUT(what);
            Z80PIO_B_MODE02_STATE(what) = 1;
            Z80PIO_B_STRB_PRIME(what)   = 0;

            Z80PIO_B_RDY_DATA_OUT(what);

            break;
        }

        case 0x040:
        case 0x080:
        {
            Z80PIO_B_REG_OUTPUT(what) = Z80PIO_DATA_BUS_C(what);

            break;
        }

        case 0x0C0:
        {
            Z80PIO_B_REG_OUTPUT(what)  = Z80PIO_DATA_BUS_C(what);
            Z80PIO_B_REG_OUTPUT(what) &= ( Z80PIO_B_REG_IOCTRL(what) ^ 0x0ff );
            Z80PIO_B_REG_OUTPUT(what) |= ( Z80PIO_B_REG_INPUT(what) & ( Z80PIO_B_REG_IOCTRL(what) ) );
            Z80PIO_B_REG_INPUT(what)   = Z80PIO_B_REG_OUTPUT(what);

            z80pio_b_setup_3_mode(what);

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_data_rd_A(void *what)
{
    switch ( Z80PIO_A_REG_MODE(what) )
    {
        case 0x000:
        {
            Z80PIO_DATA_BUS_E(what) = Z80PIO_A_REG_OUTPUT(what);

            break;
        }

        case 0x040:
        {
            Z80PIO_DATA_BUS_E(what) = Z80PIO_A_REG_INPUT(what);

            z80pio_a_setup_1_mode(what);

            Z80PIO_A_RDY(what)           = 0;
            Z80PIO_A_MODE123_STATE(what) = 1;

            Z80PIO_A_RDY_DATA_OUT(what);

            break;
        }

        case 0x080:
        {
            Z80PIO_DATA_BUS_E(what) = Z80PIO_A_REG_INPUT(what);

            z80pio_a_setup_2_mode(what);

            Z80PIO_B_RDY(what)           = 0;
            Z80PIO_A_MODE123_STATE(what) = 1;

            Z80PIO_B_RDY_DATA_OUT(what);

            break;
        }

        case 0x0C0:
        {
            Z80PIO_DATA_BUS_E(what) = Z80PIO_A_REG_OUTPUT(what);

            z80pio_a_setup_3_mode(what);

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_data_rd_B(void *what)
{
    switch ( Z80PIO_B_REG_MODE(what) )
    {
        case 0x000:
        {
            Z80PIO_DATA_BUS_G(what) = Z80PIO_B_REG_OUTPUT(what);

            break;
        }

        case 0x040:
        {
            Z80PIO_DATA_BUS_G(what) = Z80PIO_B_REG_INPUT(what);

            z80pio_b_setup_1_mode(what);

            Z80PIO_B_RDY(what)           = 0;
            Z80PIO_B_MODE123_STATE(what) = 1;

            Z80PIO_B_RDY_DATA_OUT(what);

            break;
        }

        case 0x0C0:
        {
            Z80PIO_DATA_BUS_G(what) = Z80PIO_B_REG_OUTPUT(what);

            z80pio_b_setup_3_mode(what);

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_a_strb_data_in(void *what)
{
    switch ( Z80PIO_A_REG_MODE(what) )
    {
        case 0x000:
        {
            if ( Z80PIO_A_MODE02_STATE(what) == 1 )
            {
                /* just to make sure */

                Z80PIO_A_RDY(what)  = 1;
                Z80PIO_A_DATA(what) = Z80PIO_A_REG_OUTPUT(what); 

                if ( Z80PIO_A_STRB_PRIME(what) == 0 )
                {
                    /* first, need a high-low transition */

                    if ( Z80PIO_A_STRB(what) == 0 )
                    {
                        Z80PIO_A_STRB_PRIME(what) = 1;
                    }
                }

                else
                {
                    /* do stuff if this is a low-high transition */

                    if ( Z80PIO_A_STRB(what) == 1 )
                    {
                        Z80PIO_A_RDY(what)        = 0;
                        Z80PIO_A_STRB_PRIME(what) = 0;

                        Z80PIO_A_RDY_DATA_OUT(what);


                        if ( Z80PIO_A_REG_INTCTRL(what) & 0x080 )
                        {
                            if ( Z80PIO_A_INT_INHIBIT(what) )
                            {
                                Z80PIO_A_MODE02_STATE(what) = 2;
                            }

                            else
                            {
                                z80pio_inhibit_int_from_a(what);

                                Z80PIO_A_MODE02_STATE(what) = 3;

                                Z80PIO_SIGNAL_INTERUPT(what);
                            }
                        }

                        else
                        {
                            z80pio_a_setup_0_mode(what);
                        }
                    }
                }
            }

            break;
        }

        case 0x040:
        {
            if ( Z80PIO_A_MODE123_STATE(what) == 1 )
            {
                /* just to make sure */

                Z80PIO_A_RDY(what) = 0;

                if ( Z80PIO_A_STRB_PRIME(what) == 0 )
                {
                    if ( Z80PIO_A_STRB(what) == 0 )
                    {
                        Z80PIO_A_STRB_PRIME(what) = 1;
                    }
                }

                else
                {
                    if ( Z80PIO_A_STRB(what) == 1 )
                    {
                        Z80PIO_A_REG_INPUT(what) = Z80PIO_A_DATA(what);

                        Z80PIO_A_RDY(what)        = 1;
                        Z80PIO_A_STRB_PRIME(what) = 0;

                        Z80PIO_A_RDY_DATA_OUT(what);

                        if ( Z80PIO_A_REG_INTCTRL(what) & 0x080 )
                        {
                            if ( Z80PIO_A_INT_INHIBIT(what) )
                            {
                                Z80PIO_A_MODE123_STATE(what) = 2;
                            }

                            else
                            {
                                z80pio_inhibit_int_from_a(what);

                                Z80PIO_A_MODE123_STATE(what) = 3;

                                Z80PIO_SIGNAL_INTERUPT(what);
                            }
                        }

                        else
                        {
                            z80pio_a_setup_1_mode(what);
                        }
                    }
                }
            }

            break;
        }

        case 0x080:
        {
            if ( Z80PIO_A_MODE02_STATE(what) == 1 )
            {
                /* just to make sure */

                Z80PIO_A_RDY(what) = 1;

                if ( Z80PIO_A_STRB_PRIME(what) == 0 )
                {
                    if ( Z80PIO_A_STRB(what) == 0 )
                    {
                        /* in mode 2, data is put onto the bus now */

                        Z80PIO_A_DATA(what) = Z80PIO_A_REG_OUTPUT(what);

                        Z80PIO_A_STRB_PRIME(what) = 1;

                        Z80PIO_A_RDY_DATA_OUT(what);
                    }
                }

                else
                {
                    if ( Z80PIO_A_STRB(what) == 1 )
                    {
                        Z80PIO_A_RDY(what)        = 0;
                        Z80PIO_A_STRB_PRIME(what) = 0;

                        Z80PIO_A_RDY_DATA_OUT(what);

                        if ( Z80PIO_A_REG_INTCTRL(what) & 0x080 )
                        {
                            if ( Z80PIO_A_INT_INHIBIT(what) )
                            {
                                Z80PIO_A_MODE02_STATE(what) = 2;
                            }

                            else
                            {
                                z80pio_inhibit_int_from_a(what);

                                Z80PIO_A_MODE02_STATE(what) = 3;

                                Z80PIO_SIGNAL_INTERUPT(what);
                            }
                        }

                        else
                        {
                            z80pio_a_setup_2_mode(what);
                        }
                    }
                }
            }

            break;
        }

        case 0x0C0:
        {
            Z80PIO_A_REG_INPUT(what)   = Z80PIO_A_DATA(what);
            Z80PIO_A_REG_OUTPUT(what) &= ( Z80PIO_A_REG_IOCTRL(what) ^ 0x0ff );
            Z80PIO_A_REG_OUTPUT(what) |= ( Z80PIO_A_REG_INPUT(what) & ( Z80PIO_A_REG_IOCTRL(what) ) );
            Z80PIO_A_REG_INPUT(what)   = Z80PIO_A_REG_OUTPUT(what);

            z80pio_a_mode_3_int_gen_test(what);

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_b_strb_data_in(void *what)
{
    switch ( Z80PIO_B_REG_MODE(what) )
    {
        case 0x000:
        {
            if ( Z80PIO_B_MODE02_STATE(what) == 1 )
            {
                /* just to make sure */

                Z80PIO_B_RDY(what)  = 1;
                Z80PIO_B_DATA(what) = Z80PIO_B_REG_OUTPUT(what); 

                if ( Z80PIO_B_STRB_PRIME(what) == 0 )
                {
                    if ( Z80PIO_B_STRB(what) == 0 )
                    {
                        Z80PIO_B_STRB_PRIME(what) = 1;
                    }
                }

                else
                {
                    if ( Z80PIO_B_STRB(what) == 1 )
                    {
                        Z80PIO_B_RDY(what)        = 0;
                        Z80PIO_B_STRB_PRIME(what) = 0;

                        Z80PIO_B_RDY_DATA_OUT(what);

                        if ( Z80PIO_B_REG_INTCTRL(what) & 0x080 )
                        {
                            if ( Z80PIO_B_INT_INHIBIT(what) )
                            {
                                Z80PIO_B_MODE02_STATE(what) = 2;
                            }

                            else
                            {
                                z80pio_inhibit_int_from_b(what);

                                Z80PIO_B_MODE02_STATE(what) = 3;

                                Z80PIO_SIGNAL_INTERUPT(what);
                            }
                        }

                        else
                        {
                            z80pio_b_setup_0_mode(what);
                        }
                    }
                }
            }

            break;
        }

        case 0x040:
        {
            if ( Z80PIO_B_MODE123_STATE(what) == 1 )
            {
                /* just to make sure */

                Z80PIO_B_RDY(what) = 0;

                if ( Z80PIO_B_STRB_PRIME(what) == 0 )
                {
                    if ( Z80PIO_B_STRB(what) == 0 )
                    {
                        Z80PIO_B_STRB_PRIME(what) = 1;
                    }
                }

                else
                {
                    if ( Z80PIO_B_STRB(what) == 1 )
                    {
                        Z80PIO_B_REG_INPUT(what) = Z80PIO_B_DATA(what);

                        Z80PIO_B_RDY(what)        = 1;
                        Z80PIO_B_STRB_PRIME(what) = 0;

                        Z80PIO_B_RDY_DATA_OUT(what);

                        if ( Z80PIO_B_REG_INTCTRL(what) & 0x080 )
                        {
                            if ( Z80PIO_B_INT_INHIBIT(what) )
                            {
                                Z80PIO_B_MODE123_STATE(what) = 2;
                            }

                            else
                            {
                                z80pio_inhibit_int_from_b(what);

                                Z80PIO_B_MODE123_STATE(what) = 3;

                                Z80PIO_SIGNAL_INTERUPT(what);
                            }
                        }

                        else
                        {
                            z80pio_b_setup_1_mode(what);
                        }
                    }
                }
            }

            break;
        }

        case 0x0C0:
        {
            Z80PIO_B_REG_INPUT(what)   = Z80PIO_B_DATA(what);
            Z80PIO_B_REG_OUTPUT(what) &= ( Z80PIO_B_REG_IOCTRL(what) ^ 0x0ff );
            Z80PIO_B_REG_OUTPUT(what) |= ( Z80PIO_B_REG_INPUT(what) & ( Z80PIO_B_REG_IOCTRL(what) ) );
            Z80PIO_B_REG_INPUT(what)   = Z80PIO_B_REG_OUTPUT(what);

            z80pio_b_mode_3_int_gen_test(what);

            break;
        }

        default:
        {
            break;
        }
    }

    if ( ( Z80PIO_A_REG_MODE(what) == 0x080 ) && ( Z80PIO_A_MODE123_STATE(what) == 1 ) )
    {
        /* just to make sure */

        Z80PIO_B_RDY(what) = 0;

        if ( Z80PIO_B_STRB_PRIME(what) == 0 )
        {
            if ( Z80PIO_B_STRB(what) == 0 )
            {
                Z80PIO_B_STRB_PRIME(what) = 1;
            }
        }

        else
        {
            if ( Z80PIO_B_STRB(what) == 1 )
            {
                Z80PIO_A_REG_INPUT(what) = Z80PIO_A_DATA(what);

                Z80PIO_B_RDY(what)        = 1;
                Z80PIO_B_STRB_PRIME(what) = 0;

                Z80PIO_B_RDY_DATA_OUT(what);

                if ( Z80PIO_A_REG_INTCTRL(what) & 0x080 )
                {
                    if ( Z80PIO_A_INT_INHIBIT(what) )
                    {
                        Z80PIO_A_MODE123_STATE(what) = 2;
                    }

                    else
                    {
                        z80pio_inhibit_int_from_a(what);

                        Z80PIO_A_MODE123_STATE(what) = 3;

                        Z80PIO_SIGNAL_INTERUPT(what);
                    }
                }

                else
                {
                    z80pio_a_setup_2_mode(what);
                }
            }
        }
    }

    return;
}

void z80pio_iei_in(void *what)
{
    if ( Z80PIO_IEI(what) )
    {
        Z80PIO_A_INT_INHIBIT(what) = 1;

        switch ( Z80PIO_A_MODE02_STATE(what) )
        {
            case 0:
            case 1:
            {
                z80pio_inhibit_int_from_a(what);

                break;
            }

            case 3:
            {
                Z80PIO_A_MODE02_STATE(what) = 4;

                z80pio_inhibit_int_from_a(what);

                break;
            }

            case 5:
            {
                Z80PIO_A_MODE02_STATE(what) = 6;

                z80pio_inhibit_int_from_a(what);

                break;
            }

            default:
            {
                break;
            }
        }

        switch ( Z80PIO_A_MODE123_STATE(what) )
        {
            case 0:
            case 1:
            {
                z80pio_inhibit_int_from_a(what);

                break;
            }

            case 3:
            {
                Z80PIO_A_MODE123_STATE(what) = 4;

                z80pio_inhibit_int_from_a(what);

                break;
            }

            case 5:
            {
                Z80PIO_A_MODE123_STATE(what) = 6;

                z80pio_inhibit_int_from_a(what);

                break;
            }

            default:
            {
                break;
            }
        }
    }

    else
    {
        Z80PIO_A_INT_INHIBIT(what) = 0;

        switch ( Z80PIO_A_MODE02_STATE(what) )
        {
            case 0:
            case 1:
            {
                z80pio_uninhibit_int_from_a(what);

                break;
            }

            case 2:
            {
                z80pio_inhibit_int_from_a(what);

                Z80PIO_A_MODE02_STATE(what) = 3;

                Z80PIO_SIGNAL_INTERUPT(what);

                break;
            }

            case 4:
            {
                Z80PIO_A_MODE02_STATE(what) = 3;

                break;
            }

            case 6:
            {
                Z80PIO_A_MODE02_STATE(what) = 5;

                break;
            }

            default:
            {
                break;
            }
        }

        switch ( Z80PIO_A_MODE123_STATE(what) )
        {
            case 0:
            case 1:
            {
                z80pio_uninhibit_int_from_a(what);

                break;
            }

            case 2:
            {
                z80pio_inhibit_int_from_a(what);

                Z80PIO_A_MODE02_STATE(what) = 3;

                Z80PIO_SIGNAL_INTERUPT(what);

                break;
            }

            case 4:
            {
                Z80PIO_A_MODE123_STATE(what) = 3;

                break;
            }

            case 6:
            {
                Z80PIO_A_MODE123_STATE(what) = 5;

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

void z80pio_ack_int(void *what)
{
    Z80PIO_DATA_BUS_I(what) = 0;

    if ( Z80PIO_A_MODE02_STATE(what) == 3 )
    {
        Z80PIO_DATA_BUS_I(what) = Z80PIO_A_REG_INTVECT(what);

        Z80PIO_A_MODE02_STATE(what) = 5;
    }

    if ( Z80PIO_B_MODE02_STATE(what) == 3 )
    {
        Z80PIO_DATA_BUS_I(what) = Z80PIO_B_REG_INTVECT(what);

        Z80PIO_B_MODE02_STATE(what) = 5;
    }

    if ( Z80PIO_A_MODE123_STATE(what) == 3 )
    {
        if ( Z80PIO_A_REG_MODE(what) == 0x080 )
        {
            Z80PIO_DATA_BUS_I(what) = Z80PIO_B_REG_INTVECT(what);
        }

        else
        {
            Z80PIO_DATA_BUS_I(what) = Z80PIO_A_REG_INTVECT(what);
        }

        Z80PIO_A_MODE123_STATE(what) = 5;
    }

    if ( Z80PIO_B_MODE123_STATE(what) == 3 )
    {
        Z80PIO_DATA_BUS_I(what) = Z80PIO_B_REG_INTVECT(what);

        Z80PIO_B_MODE123_STATE(what) = 5;
    }

    return;
}

void z80pio_reti_signal(void *what)
{
    if ( Z80PIO_A_MODE02_STATE(what) == 5 )
    {
        Z80PIO_A_MODE02_STATE(what) = 0;

        z80pio_uninhibit_int_from_a(what);
    }

    if ( Z80PIO_B_MODE02_STATE(what) == 5 )
    {
        Z80PIO_B_MODE02_STATE(what) = 0;

        z80pio_uninhibit_int_from_b(what);
    }

    if ( Z80PIO_A_MODE123_STATE(what) == 5 )
    {
        Z80PIO_A_MODE123_STATE(what) = 0;

        z80pio_uninhibit_int_from_a(what);
    }

    if ( Z80PIO_B_MODE123_STATE(what) == 5 )
    {
        Z80PIO_B_MODE123_STATE(what) = 0;

        z80pio_uninhibit_int_from_b(what);
    }

    return;
}



void z80pio_a_setup_0_mode(void *what)
{
    Z80PIO_A_STRB_PRIME(what) = 0;

    Z80PIO_A_REG_MODE(what) = 0x000;

    Z80PIO_A_MODE02_STATE(what)  = 0;
    Z80PIO_A_MODE123_STATE(what) = 0;

    Z80PIO_A_RDY(what)  = 0;
    Z80PIO_A_DATA(what) = Z80PIO_A_REG_OUTPUT(what);

    Z80PIO_A_RDY_DATA_OUT(what);

    return;
}

void z80pio_a_setup_1_mode(void *what)
{
    Z80PIO_A_STRB_PRIME(what) = 0;

    Z80PIO_A_REG_MODE(what) = 0x040;

    Z80PIO_A_MODE02_STATE(what)  = 0;
    Z80PIO_A_MODE123_STATE(what) = 0;

    Z80PIO_A_RDY(what) = 1;

    Z80PIO_A_RDY_DATA_OUT(what);

    return;
}

void z80pio_a_setup_2_mode(void *what)
{
    Z80PIO_A_STRB_PRIME(what) = 0;
    Z80PIO_B_STRB_PRIME(what) = 0;

    Z80PIO_A_REG_MODE(what) = 0x080;

    Z80PIO_A_MODE02_STATE(what)  = 0;
    Z80PIO_A_MODE123_STATE(what) = 0;

    Z80PIO_A_RDY(what) = 0;

    Z80PIO_A_RDY_DATA_OUT(what);

    Z80PIO_B_RDY(what) = 1;

    Z80PIO_B_RDY_DATA_OUT(what);

    return;
}

void z80pio_a_setup_3_mode(void *what)
{
    Z80PIO_A_STRB_PRIME(what) = 0;

    Z80PIO_A_REG_MODE(what) = 0x0C0;

    Z80PIO_A_MODE02_STATE(what)  = 0;
    Z80PIO_A_MODE123_STATE(what) = 0;

    Z80PIO_A_RDY(what) = DEFAULT_Z80PIO_A_RDY;

    Z80PIO_A_DATA(what) &= Z80PIO_A_REG_IOCTRL(what);
    Z80PIO_A_DATA(what) |= ( Z80PIO_A_REG_OUTPUT(what) & ( Z80PIO_A_REG_IOCTRL(what) ^ 0x0FF ) );

    Z80PIO_A_RDY_DATA_OUT(what);

    return;
}

void z80pio_b_setup_0_mode(void *what)
{
    Z80PIO_B_STRB_PRIME(what) = 0;

    Z80PIO_B_REG_MODE(what) = 0x000;

    Z80PIO_B_MODE02_STATE(what)  = 0;
    Z80PIO_B_MODE123_STATE(what) = 0;

    Z80PIO_B_RDY(what)  = 0;
    Z80PIO_B_DATA(what) = Z80PIO_B_REG_OUTPUT(what);

    Z80PIO_B_RDY_DATA_OUT(what);

    return;
}

void z80pio_b_setup_1_mode(void *what)
{
    Z80PIO_B_STRB_PRIME(what) = 0;

    Z80PIO_B_REG_MODE(what) = 0x040;

    Z80PIO_B_MODE02_STATE(what)  = 0;
    Z80PIO_B_MODE123_STATE(what) = 0;

    Z80PIO_B_RDY(what) = 1;

    Z80PIO_B_RDY_DATA_OUT(what);

    return;
}

void z80pio_b_setup_3_mode(void *what)
{
    Z80PIO_B_STRB_PRIME(what) = 0;

    Z80PIO_B_REG_MODE(what) = 0x0C0;

    Z80PIO_B_MODE02_STATE(what)  = 0;
    Z80PIO_B_MODE123_STATE(what) = 0;

    Z80PIO_B_RDY(what) = DEFAULT_Z80PIO_B_RDY;

    Z80PIO_B_DATA(what) &= Z80PIO_B_REG_IOCTRL(what);
    Z80PIO_B_DATA(what) |= ( Z80PIO_B_REG_OUTPUT(what) & ( Z80PIO_B_REG_IOCTRL(what) ^ 0x0FF ) );

    Z80PIO_B_RDY_DATA_OUT(what);

    return;
}

void z80pio_a_mode_3_int_gen_test(void *what)
{
    UINT_16 i;
    UINT_8 is_int = 0;

    if ( Z80PIO_A_REG_INTCTRL(what) & 0x080 )
    {
        switch ( Z80PIO_A_REG_INTCTRL(what) & 0x060 )
        {
            case 0x000:
            {
                /* LOW OR */

                is_int = 0;

                for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                {
                    /* its an input   */
                    /* its not masked */
                    /* its low        */

                    if (  ( Z80PIO_A_REG_IOCTRL(what)   & i ) &&
                         !( Z80PIO_A_REG_MASKCTRL(what) & i ) &&
                         !( Z80PIO_A_DATA(what)         & i )    )
                    {
                        is_int = 1;
                    }
                }

                break;
            }

            case 0x020:
            {
                /* HIGH OR */

                is_int = 0;

                for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                {
                    /* its an input   */
                    /* its not masked */
                    /* its high       */

                    if (  ( Z80PIO_A_REG_IOCTRL(what)   & i ) &&
                         !( Z80PIO_A_REG_MASKCTRL(what) & i ) &&
                          ( Z80PIO_A_DATA(what)         & i )    )
                    {
                        is_int = 1;
                    }
                }

                break;
            }

            case 0x040:
            {
                /* LOW AND */

                is_int = 0;

                if ( Z80PIO_A_REG_MASKCTRL(what) != 0x0FF )
                {
                    is_int = 1;

                    for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                    {
                        /* its an input   */
                        /* its not masked */
                        /* its high       */

                        if (  ( Z80PIO_A_REG_IOCTRL(what)   & i ) &&
                             !( Z80PIO_A_REG_MASKCTRL(what) & i ) &&
                              ( Z80PIO_A_DATA(what)         & i )    )
                        {
                            is_int = 0;
                        }
                    }
                }

                break;
            }

            default: /* case 0x060: */
            {
                /* HIGH AND */

                is_int = 0;

                if ( Z80PIO_A_REG_MASKCTRL(what) != 0x0FF )
                {
                    is_int = 1;

                    for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                    {
                        /* its an input   */
                        /* its not masked */
                        /* its low        */

                        if (  ( Z80PIO_A_REG_IOCTRL(what)   & i ) &&
                             !( Z80PIO_A_REG_MASKCTRL(what) & i ) &&
                             !( Z80PIO_A_DATA(what)         & i )    )
                        {
                            is_int = 0;
                        }
                    }
                }

                break;
            }
        }

        if ( is_int )
        {
            if ( Z80PIO_A_INT_INHIBIT(what) )
            {
                Z80PIO_A_MODE123_STATE(what) = 2;
            }

            else
            {
                z80pio_inhibit_int_from_a(what);

                Z80PIO_A_MODE123_STATE(what) = 3;

                Z80PIO_SIGNAL_INTERUPT(what);
            }
        }
    }

    return;
}

void z80pio_b_mode_3_int_gen_test(void *what)
{
    UINT_16 i;
    UINT_8 is_int = 0;

    /* interupts are enabled   */
    /* port a is not in mode 2 */

    if ( ( Z80PIO_B_REG_INTCTRL(what)  & 0x080 ) &&
         ( Z80PIO_A_REG_MODE(what)    != 0x080 )    )
    {
        switch ( Z80PIO_B_REG_INTCTRL(what) & 0x060 )
        {
            case 0x000:
            {
                /* LOW OR */

                is_int = 0;

                for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                {
                    /* its an input   */
                    /* its not masked */
                    /* its low        */

                    if (  ( Z80PIO_B_REG_IOCTRL(what)   & i ) &&
                         !( Z80PIO_B_REG_MASKCTRL(what) & i ) &&
                         !( Z80PIO_B_DATA(what)         & i )    )
                    {
                        is_int = 1;
                    }
                }

                break;
            }

            case 0x020:
            {
                /* HIGH OR */

                is_int = 0;

                for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                {
                    /* its an input   */
                    /* its not masked */
                    /* its high       */

                    if (  ( Z80PIO_B_REG_IOCTRL(what)   & i ) &&
                         !( Z80PIO_B_REG_MASKCTRL(what) & i ) &&
                          ( Z80PIO_B_DATA(what)         & i )    )
                    {
                        is_int = 1;
                    }
                }

                break;
            }

            case 0x040:
            {
                /* LOW AND */

                is_int = 0;

                if ( Z80PIO_B_REG_MASKCTRL(what) != 0x0FF )
                {
                    is_int = 1;

                    for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                    {
                        /* its an input   */
                        /* its not masked */
                        /* its high       */

                        if (  ( Z80PIO_B_REG_IOCTRL(what)   & i ) &&
                             !( Z80PIO_B_REG_MASKCTRL(what) & i ) &&
                              ( Z80PIO_B_DATA(what)         & i )    )
                        {
                            is_int = 0;
                        }
                    }
                }

                break;
            }

            default: /* case 0x060: */
            {
                /* HIGH AND */

                is_int = 0;

                if ( Z80PIO_B_REG_MASKCTRL(what) != 0x0FF )
                {
                    is_int = 1;

                    for ( i = 0x00001 ; i < 0x00100 ; i *= 2 )
                    {
                        /* its an input   */
                        /* its not masked */
                        /* its low        */

                        if (  ( Z80PIO_B_REG_IOCTRL(what)   & i ) &&
                             !( Z80PIO_B_REG_MASKCTRL(what) & i ) &&
                             !( Z80PIO_B_DATA(what)         & i )    )
                        {
                            is_int = 0;
                        }
                    }
                }

                break;
            }
        }

        if ( is_int )
        {
            if ( Z80PIO_B_INT_INHIBIT(what) )
            {
                Z80PIO_B_MODE123_STATE(what) = 2;
            }

            else
            {
                z80pio_inhibit_int_from_b(what);

                Z80PIO_B_MODE123_STATE(what) = 3;

                Z80PIO_SIGNAL_INTERUPT(what);
            }
        }
    }

    return;
}

void z80pio_inhibit_int_from_a(void *what)
{
    Z80PIO_B_INT_INHIBIT(what) = 1;

    switch ( Z80PIO_B_MODE02_STATE(what) )
    {
        case 0:
        case 1:
        {
            z80pio_inhibit_int_from_b(what);

            break;
        }

        case 3:
        {
            Z80PIO_B_MODE02_STATE(what) = 4;

            z80pio_inhibit_int_from_b(what);

            break;
        }

        case 5:
        {
            Z80PIO_B_MODE02_STATE(what) = 6;

            z80pio_inhibit_int_from_b(what);

            break;
        }

        default:
        {
            break;
        }
    }

    switch ( Z80PIO_B_MODE123_STATE(what) )
    {
        case 0:
        case 1:
        {
            z80pio_inhibit_int_from_b(what);

            break;
        }

        case 3:
        {
            Z80PIO_B_MODE123_STATE(what) = 4;

            z80pio_inhibit_int_from_b(what);

            break;
        }

        case 5:
        {
            Z80PIO_B_MODE123_STATE(what) = 6;

            z80pio_inhibit_int_from_b(what);

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_inhibit_int_from_b(void *what)
{
    Z80PIO_IEO(what) = 1;

    Z80PIO_IEO_OUT(what);

    return;
}

void z80pio_uninhibit_int_from_a(void *what)
{
    Z80PIO_B_INT_INHIBIT(what) = 0;

    switch ( Z80PIO_B_MODE02_STATE(what) )
    {
        case 0:
        case 1:
        {
            z80pio_uninhibit_int_from_b(what);

            break;
        }

        case 2:
        {
            Z80PIO_B_MODE02_STATE(what) = 3;

            Z80PIO_SIGNAL_INTERUPT(what);

            break;
        }

        case 4:
        {
            Z80PIO_B_MODE02_STATE(what) = 3;

            break;
        }

        case 6:
        {
            Z80PIO_B_MODE02_STATE(what) = 5;

            break;
        }

        default:
        {
            break;
        }
    }

    switch ( Z80PIO_B_MODE123_STATE(what) )
    {
        case 0:
        case 1:
        {
            z80pio_uninhibit_int_from_b(what);

            break;
        }

        case 2:
        {
            Z80PIO_B_MODE123_STATE(what) = 3;

            Z80PIO_SIGNAL_INTERUPT(what);

            break;
        }

        case 4:
        {
            Z80PIO_B_MODE123_STATE(what) = 3;

            break;
        }

        case 6:
        {
            Z80PIO_B_MODE123_STATE(what) = 5;

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void z80pio_uninhibit_int_from_b(void *what)
{
    Z80PIO_IEO(what) = 0;

    Z80PIO_IEO_OUT(what);

    return;
}





char *z80pio_getinf(module_data *what)
{
    char *dest;

    char pio_ctrl_reg_state0[] = "Normal             ";
    char pio_ctrl_reg_state1[] = "Pend reg ctrl word ";
    char pio_ctrl_reg_state2[] = "Pend mask ctrl word";

    char   pio_a_mode;
    UINT_8 pio_a_intvect;
    char   pio_a_intonoff[] = "OFF";
    UINT_8 pio_a_ioctrl;                /* mode 3 only */
    UINT_8 pio_a_intmask;               /* mode 3 only */
    char   pio_a_intctrl[] = "OR ";     /* mode 3 only */
    char   pio_a_intlevel[] = "LOW ";   /* mode 3 only */

    char  *pio_a_ctrl_regstate = pio_ctrl_reg_state0;
    char   pio_a_int_inhib[] = "NO ";
    UINT_8 pio_a_stat_out;
    UINT_8 pio_a_stat_in;
    UINT_8 pio_a_strbstat;
    UINT_8 pio_a_outbyte;
    UINT_8 pio_a_inbyte;

    char   pio_b_mode;
    UINT_8 pio_b_intvect;
    char   pio_b_intonoff[] = "OFF";
    UINT_8 pio_b_ioctrl;                /* mode 3 only */
    UINT_8 pio_b_intmask;               /* mode 3 only */
    char   pio_b_intctrl[] = "OR ";     /* mode 3 only */
    char   pio_b_intlevel[] = "LOW ";   /* mode 3 only */

    char  *pio_b_ctrl_regstate = pio_ctrl_reg_state0;
    char   pio_b_int_inhib[] = "NO ";
    UINT_8 pio_b_stat_out;
    UINT_8 pio_b_stat_in;
    UINT_8 pio_b_strbstat;
    UINT_8 pio_b_outbyte;
    UINT_8 pio_b_inbyte;

    dest = DEBMALLOC(1200*sizeof(char));

    {
        pio_a_outbyte = Z80PIO_A_REG_OUTPUT(what);
        pio_a_inbyte  = Z80PIO_A_REG_INPUT(what);

        pio_a_mode     = ( ( Z80PIO_A_REG_MODE(what) >> 6 ) & 0x03 ) + '0';
        pio_a_intvect  = Z80PIO_A_REG_INTVECT(what);
        pio_a_ioctrl   = Z80PIO_A_REG_IOCTRL(what);
        pio_a_intmask  = Z80PIO_A_REG_MASKCTRL(what);
        pio_a_stat_out = Z80PIO_A_MODE02_STATE(what);
        pio_a_stat_in  = Z80PIO_A_MODE123_STATE(what);
        pio_a_strbstat = Z80PIO_A_STRB_PRIME(what);

        if ( Z80PIO_A_REGCTRL_PENDING(what) )
        {
            pio_a_ctrl_regstate = pio_ctrl_reg_state1;
        }

        if ( Z80PIO_A_MASKCTRL_PENDING(what) )
        {
            pio_a_ctrl_regstate = pio_ctrl_reg_state2;
        }

        if ( Z80PIO_A_REG_INTCTRL(what) & 0x080 )
        {
            pio_a_intonoff[1] = 'N';
            pio_a_intonoff[2] = ' ';
        }

        if ( Z80PIO_A_REG_INTCTRL(what) & 0x040 )
        {
            pio_a_intctrl[0] = 'A';
            pio_a_intctrl[1] = 'N';
            pio_a_intctrl[2] = 'D';
        }

        if ( Z80PIO_A_REG_INTCTRL(what) & 0x020 )
        {
            pio_a_intlevel[0] = 'H';
            pio_a_intlevel[1] = 'I';
            pio_a_intlevel[2] = 'G';
            pio_a_intlevel[3] = 'H';
        }

        if ( Z80PIO_A_INT_INHIBIT(what) )
        {
            pio_a_int_inhib[0] = 'Y';
            pio_a_int_inhib[1] = 'E';
            pio_a_int_inhib[2] = 'S';
        }
    }

    {
        pio_b_outbyte = Z80PIO_B_REG_OUTPUT(what);
        pio_b_inbyte  = Z80PIO_B_REG_INPUT(what);

        pio_b_mode     = ( ( Z80PIO_B_REG_MODE(what) >> 6 ) & 0x03 ) + '0';
        pio_b_intvect  = Z80PIO_B_REG_INTVECT(what);
        pio_b_ioctrl   = Z80PIO_B_REG_IOCTRL(what);
        pio_b_intmask  = Z80PIO_B_REG_MASKCTRL(what);
        pio_b_stat_out = Z80PIO_B_MODE02_STATE(what);
        pio_b_stat_in  = Z80PIO_B_MODE123_STATE(what);
        pio_b_strbstat = Z80PIO_B_STRB_PRIME(what);

        if ( Z80PIO_A_REGCTRL_PENDING(what) )
        {
            pio_b_ctrl_regstate = pio_ctrl_reg_state1;
        }

        if ( Z80PIO_A_MASKCTRL_PENDING(what) )
        {
            pio_b_ctrl_regstate = pio_ctrl_reg_state2;
        }

        if ( Z80PIO_B_REG_INTCTRL(what) & 0x080 )
        {
            pio_b_intonoff[1] = 'N';
            pio_b_intonoff[2] = ' ';
        }

        if ( Z80PIO_B_REG_INTCTRL(what) & 0x040 )
        {
            pio_b_intctrl[0] = 'A';
            pio_b_intctrl[1] = 'N';
            pio_b_intctrl[2] = 'D';
        }

        if ( Z80PIO_B_REG_INTCTRL(what) & 0x020 )
        {
            pio_b_intlevel[0] = 'H';
            pio_b_intlevel[1] = 'I';
            pio_b_intlevel[2] = 'G';
            pio_b_intlevel[3] = 'H';
        }

        if ( Z80PIO_B_INT_INHIBIT(what) )
        {
            pio_b_int_inhib[0] = 'Y';
            pio_b_int_inhib[1] = 'E';
            pio_b_int_inhib[2] = 'S';
        }
    }

    sprintf(dest,"         ======  Z80 PIO State Summary ======          "             "\n"
                 "                                                       "             "\n"
                 " Port A (mode %c) [%02x %d %d]  | Port B (mode %c) [%02x %d %d]  "   "\n"
                 "                           |                           "             "\n"
                 " Interupts:  %s           | Interupts:  %s           "               "\n"
                 " Inhibitted: %s           | Inhibitted: %s           "               "\n"
                 " Vector:     %02x            | Vector:     %02x            "         "\n"
                 " Int mask:   %02x   (mode 3) | Int mask:   %02x   (mode 3) "         "\n"
                 " Int logic:  %s  (mode 3) | Int logic:  %s  (mode 3) "               "\n"
                 " Int level:  %s (mode 3) | Int level:  %s (mode 3) "                 "\n"
                 " IO control: %02x   (mode 3) | IO control: %02x   (mode 3) "         "\n"
                 "                           |                           "             "\n"
                 " State in/out: %d/%d         | State in/out: %d/%d         "         "\n"
                 " Strobe state: %d           | Strobe state: %d           "           "\n"
                 " Ctrl: %s | Ctrl: %s "                                               "\n"
                 " Output byte: %02x           | Output byte: %02x           "         "\n"
                 " Input byte:  %02x           | Input byte:  %02x           "         "\n"


                 ,pio_a_mode,Z80PIO_A_DATA(what),Z80PIO_A_RDY(what),Z80PIO_A_STRB(what),pio_b_mode,Z80PIO_B_DATA(what),Z80PIO_B_RDY(what),Z80PIO_B_STRB(what)

                 ,pio_a_intonoff,pio_b_intonoff
                 ,pio_a_int_inhib,pio_b_int_inhib
                 ,pio_a_intvect,pio_b_intvect
                 ,pio_a_intmask,pio_b_intmask
                 ,pio_a_intctrl,pio_b_intctrl
                 ,pio_a_intlevel,pio_b_intlevel
                 ,pio_a_ioctrl,pio_b_ioctrl

                 ,(int) pio_a_stat_in,(int) pio_a_stat_out,(int) pio_b_stat_in,(int) pio_b_stat_out
                 ,(int) pio_a_strbstat,(int) pio_b_strbstat
                 ,pio_a_ctrl_regstate,pio_b_ctrl_regstate
                 ,pio_a_outbyte,pio_b_outbyte
                 ,pio_a_inbyte,pio_b_inbyte);

    return dest;
}

