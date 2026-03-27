#ifndef ISR_H
#define ISR_H

#include <cstdlib>
#include <math.h>


/* This class defines and implements stuff, which is bad practice, but I don't
    know how to do a multi-source project with dmc compiler                   */

/**
 * @brief This file defines and implements an interrupt class (isr). It should
 * be triggered periodically by a specified frequency. Initial time delay can
 * also be specified in order to control phase between multiple isrs. External
 * code to be executed is called by the control class (no function pointers), so
 * the user should modify this part of the code directly in case the external
 * class is modified.
 *
 * This class also contains two logic outputs that share a dead time between
 * them. The dead time is defined in the constructor and can be received as an
 * external argument.
 *
 */


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */

class ISR_FUNC;
class control;
struct cntrl_out_str;
struct inputData;


/* -------------------------------------------------------------------------- */
/*                                Custom types                                */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                                   Classes                                  */
/* -------------------------------------------------------------------------- */

class isr
{   

    public:
  
    double isr_prd;                 // Period of isr periodic interrupt
    double dead_time;               // Dead time in seconds

    double ttol;                    // Time tolerance when there is a switching 
                                    // event
    double max_step;                // Max time step adjusted dynamically
    double t_n1;                    // Time of previous simulation step
    double t_next_isr;              // Time of next isr
    double t_next_out_transition;   // Time for next output transition after 
                                    // dead time

    double t_tstep_lim;             // Auxiliary to control time step lim

    bool isr_trigger;               // Auxiliary bool - true every time periodic
                                    // function is called, and only at one point

    bool sw_event;                  // Auxiliary bool - true when there is a 
                                    // switching event. It's both related to the
                                    // true outputs and switching within the 
                                    // software. This variable is used in the
                                    // Trunc function that the spice solver 
                                    // utilizes

    bool out_high;                  // Final output for high side
    bool out_high_call;             // Called output after control function, 
                                    // only updated after dead time if a 
                                    // transition occurs
    bool out_high_n1;               // Previous real high output
    
    bool out_low;                   
    bool out_low_call;  
    bool out_low_n1;
    
    bool enable;                    // General enable for outputs
    bool enable_start;              // Initial enable during sim. start
    bool tstep_lim_flag;            // Flag is on when limitting timestep
    bool dead_band;                 // True during dead band


    /* ---------------------------------------------------------------------- */
    /*                      Main isr handler function                         */
    /* ---------------------------------------------------------------------- */
    void isr_handler(double *t, class control::ISR_FUNC *isr_ptr, \
        class control *cntrl_ptr, inputData *indata)
    {

        // Writes false to trigger, code below writes true if needed
        sw_event = false;
        isr_trigger = false;

        // Cheks if time has passed 
        if(t_n1 < t_next_isr && *t >= t_next_isr)
        {
            sw_event = true;
            isr_trigger = true;
            // Computs next isr time
            t_next_isr += isr_prd;

            /* -------------------------------------------------------------- */
            /*               PUT PERIODIC CODE BELOW                          */
            /* -------------------------------------------------------------- */
            isr_ptr->isr(cntrl_ptr);
        }

        /* ------------------------------------------------------------------ */
        /*                        Output triggers                             */
        /* ------------------------------------------------------------------ */
    
        // Finally updates final outputs as needed
        if(*t < dead_time)  enable_start = false;
        else    enable_start = true;

        if(t_n1 < t_next_out_transition && *t >= t_next_out_transition){
            sw_event = true;
            dead_band = false;
        }

        out_high = out_high_call & enable & enable_start & !dead_band;
        out_low = out_low_call & enable & enable_start & !dead_band;

        // Any weird exceptions should be caught below
        if(out_high_n1 != out_high || out_low_n1 != out_low) sw_event = true;

        // Saves stuff for next iterations
        t_n1 = *t;
        out_high_n1 = out_high;
        out_low_n1 = out_low;
    };

/* -------------------------------------------------------------------------- */
/*                            Qspice trunc handler                            */
/* -------------------------------------------------------------------------- */
    void isr_trunc_handler(double *timestep, double t)
    {

        // Periodic triggers
        if(*timestep > isr_prd)
            *timestep = isr_prd;
    }

    double isr_max_step_handler(double t)
    {
        if(sw_event){
            t_tstep_lim = t;
            tstep_lim_flag = true;
        }
        if(t - t_tstep_lim >= dead_time)    tstep_lim_flag = false;
        if(tstep_lim_flag){
            return ttol;
        }

        // If here, then return max_step
        return max_step;
    }

    /* ---------------------------------------------------------------------- */
    /*                        Constructor (inline)                            */
    /* ---------------------------------------------------------------------- */
    inline isr(double fs = 40e3, double dt = 50e-9){
        
        // Initializes some elements
        dead_time = dt;
        enable = true;
        enable_start = false;
        sw_event = true;
        out_high = false;
        out_low = false;
        tstep_lim_flag = false;

        // Initial time stamps
        t_next_isr = dead_time/10;
        t_next_out_transition = t_next_isr + dead_time;

        // Default ttol = dead_time/8
        ttol = dead_time/8;

        // Previous time at 0 = 0
        t_n1 = 0;
        
        // Limits time on sim. start
        t_tstep_lim = 0;

        // Max timestep equals period
        max_step = 1/fs;

        // Derives period from frequency
        isr_prd = 1/fs;
    }

};

#endif