/* -------------------------------------------------------------------------- */
/*                        Digital controller for Qspice                       */
/* -------------------------------------------------------------------------- */

/*
    Author: Rafael J. Scholtz
    Description: this is intended to be a template the for user to implement a
    digital control algorithm in a simulation. Because pwm is so relevant in
    this scenario, a pwm class was also implemented, but other forms of modula-
    tion (or lack of) can also be implemented.

    This code was adapted from physicsboy [1] and robdunn4 [2]:
    [1]. https://github.com/physicboy/QSPICE
    [2]. https://github.com/robdunn4/QSpice
*/

#include <cstdlib>
#include <math.h>
#include <cstring>

/* -------------------------------------------------------------------------- */
/*                             Version information                            */
/* -------------------------------------------------------------------------- */
#define PROGRAM_NAME    "digital_controller"
#define PROGRAM_VERSION "v1.0"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

// must follow above versioning information
#include "Cblock.h"



/*------------------------------------------------------------------------------
 * DbgLog - useful for debugging
 *----------------------------------------------------------------------------*/
// declare DbgLog instance for logging; instance name must be dbgLog; change
// file name and max line limit if desired (-1 = max).
//#include "DbgLog.h"
//DbgLog dbgLog("@qdebug.log", 500);


/* -------------------------------------------------------------------------- */
/*                         Custom types and defines                           */
/* -------------------------------------------------------------------------- */

// Pi
#define M_PI            3.14159265358979323846  /* pi */

// vline normalization factor (max value of vline_sense)
#define VLINE_SENSE_MAX 1.4142135623731742
#define VLINE_MAX       373.3523804664971


// Struct with control output variables
struct cntrl_out_str{
  double duty;
  bool invert_pol;
};


// Struct with control input variables
struct inputData{
  double  vline_sense;
  double  vo_sense;
  double  IL_sense;
  double  t;
};


/* -------------------------------------------------------------------------- */
/*                                Control class                               */
/* -------------------------------------------------------------------------- */
class control
{
  public:

  // General
  double fs;                      // Switching freq. [for integrator]
  double duty_max_ss;             // Max duty in soft start


  // Input signals
  inputData indata;               // Input data for controller
  double Hvin_dc_gain;
  double Hvo_dc_gain;
  double Hiin_dc_gain;
  double Vref_adc;
  double vin_meas;
  double iin_meas;
  double vo_meas;

  // Output signals
  struct cntrl_out_str cntrl_out; // Control output for pwm module

  // Controller output
  double duty;

  // PLL
  double wn;        // Nominal grid freq.
  double wg;        // Tracked freq.
  double wg_n1;     // Past value of tracked freq.
  double theta;     // PLL output angle
  double theta_n1;  // Past output angle
  double vq;
  double vq_n1;
  double vq_n2;
  double vq_n3;
  double ma_out;
  double pi_out;
  double pi_in;
  double pi_in_n1;
  unsigned long long pll_count;
  unsigned long long pll_count_max;
  double vin_norm;
  double vin_norm_n1;
  double vin_norm_n2;
  double vin_peak;                // Estimated peak of input line voltage


/* -------------------------------------------------------------------------- */
/*                             Moving average PLL                             */
/* -------------------------------------------------------------------------- */
  void PLL_MA(){

    // MA only runs at 60*8 Hz - so requires decimation
    // normalizes vin
    double vin = (indata.vline_sense-Vref_adc/2.0) / VLINE_MAX;

    pll_count++;
    if(pll_count >= pll_count_max){

      // Updates past self values
      vq_n3 = vq_n2;
      vq_n2 = vq_n1;
      vq_n1 = vq;

      // Multiplies internal angle with input
      vq = -2*sin(theta) * vin;
      ma_out = (vq + vq_n1 + vq_n2 + vq_n3)/4;

      // Resets decimation counter
      pll_count = 0;

    }

    // PI filter
    pi_in = ma_out;
    pi_out = pi_out + (100*pi_in - 99.97*pi_in_n1); // fs = 64800

    // Adds nominal freq
    wg = wn + pi_out;

    // Integrates freq.
    theta = fmod(theta_n1 + wg_n1*(1/fs), 2*M_PI);

    // Saves past values
    theta_n1 = theta;
    pi_in_n1 = pi_in;

    wg_n1 = wg;
  }


/* -------------------------------------------------------------------------- */
/*                          ISR FUNC class definition                         */
/* -------------------------------------------------------------------------- */
class ISR_FUNC{

  public:
  // Only contains an empty function, defined by children
  virtual void isr(control *cntrl){
    return;
  }

};

/* -------------------------------------------------------------------------- */
/*                      ISR for random number generation                      */
/* -------------------------------------------------------------------------- */
class ISR_random : public ISR_FUNC{

  public:
  // Entries for random number testing
  double n1;
  double n2;
  double std_noise;
  int lowerBound;
  int upperBound;
  double mean;
  double var;

  void isr(control *cntrl){

    // Generates number between lower bound and upper bound
    int randAux = rand() % (upperBound - lowerBound + 1) + lowerBound;
    n1 = (double)randAux / (double)upperBound;

    randAux = rand() % (upperBound - lowerBound + 1) + lowerBound;
    n2 = (double)randAux / (double)upperBound;

    std_noise = sqrt(-2.0 * log(n1) * var) * cos(2*M_PI*n2) + mean;

  };

  ISR_random(double mean_arg = 0, double var_arg = 1){
    n1 = 1;
    n2 = 0;
    lowerBound = 1;
    upperBound = RAND_MAX - 1;
    mean = mean_arg;
    var = var_arg;
    std_noise = 0.0;
  };
};

ISR_random isr1;
ISR_random isr2;
ISR_random isr3;


/* -------------------------------------------------------------------------- */
/*            Main control routine - called at every carrier valley           */
/* -------------------------------------------------------------------------- */

  cntrl_out_str* exp_handler(){

    // ---- PLL with moving average ---- //
    // Decimation by 16.2e3 for sampling freq. of 480 Hz
    PLL_MA();

    // Updates sampled signals
    vin_meas = (indata.vline_sense - Vref_adc/2.0)/Hvin_dc_gain;
    iin_meas = (indata.IL_sense - Vref_adc/2.0)/Hiin_dc_gain;
    vo_meas = (indata.vo_sense)/Hvo_dc_gain;

    // Gain for sinusoidal reference
    vin_norm = cos(theta - 2*M_PI*0);

    // Peak detector
    if(vin_norm_n1 > vin_norm_n2 && vin_norm_n1 > vin_norm){
      double vin_peak_new = vin_meas;
      vin_peak = vin_peak_new;
    }

    const double max_duty = 1.0;
    // --- Soft-start limiter of max duty --- //
    if(duty_max_ss < max_duty){
     double ss_time = 0.4;
     duty_max_ss += max_duty/(ss_time*fs);  // Requires ss_time seconds to reach
                                            //max value
    }
    if(duty_max_ss > max_duty) duty_max_ss = max_duty;


/* -------------------------------------------------------------------------- */
/*                    CONTROL OR EXPERIMENT CODE GOES BELOW                   */
/* -------------------------------------------------------------------------- */

    // Just sets duty to 0.5
    cntrl_out.duty = 0.5;


    // Handles exceptions
    if(cntrl_out.duty > duty_max_ss)  cntrl_out.duty = duty_max_ss;
    if(cntrl_out.duty < 0)  cntrl_out.duty = 0;

    // Handles polarity inversion
    if(vin_norm >= 0){
      cntrl_out.invert_pol = true;
    }
    else{
      if(vin_norm < 0) cntrl_out.invert_pol = false;
    }


    // Stores remaining prev. cycle variables
    vin_norm_n2 = vin_norm_n1;
    vin_norm_n1 = vin_norm;

    return &cntrl_out;
};

  // Constructor
  control(double fs_arg = 40e3, double fline = 60,                            \
  double mean1_arg = 0, double var1_arg = 1,                                  \
  double mean2_arg = 0, double var2_arg = 1,                                  \
  double mean3_arg = 0, double var3_arg = 1,                                  \
  double Hvin_dc_gain_arg = 1.0, double Hiin_dc_gain_arg = 1.0,               \
  double Hvo_dc_gain_arg = 1.0, double Vref_adc_arg = 0.0){


    // Input
    Hvin_dc_gain = Hvin_dc_gain_arg;
    Hiin_dc_gain = Hiin_dc_gain_arg;
    Hvo_dc_gain = Hvo_dc_gain_arg;
    Vref_adc = Vref_adc_arg;

    // Noise
    this->isr1 = * (new ISR_random(mean1_arg, var1_arg));
    this->isr2 = * (new ISR_random(mean2_arg, var2_arg));
    this->isr3 = * (new ISR_random(mean3_arg, var3_arg));

    // General
    fs = fs_arg;

    // Peak detector
    vin_peak = VLINE_MAX;

    // PLL
    wn = 2*M_PI*fline;        // Nominal grid freq.
    pll_count_max = (unsigned long long)round(fs_arg/(fline*2*4));

  }
};

/* -------------------------------------------------------------------------- */
/*                               Custom modules                               */
/* -------------------------------------------------------------------------- */
#include "pwm.h"
#include "isr.h"

/* -------------------------------------------------------------------------- */
/*                              Per instance data                             */
/* -------------------------------------------------------------------------- */
struct InstData {

  // pwm structure holding its information
  pwm_data pwm1;

  // control class
  control cntrl;

  // interrupts
  isr isr1; // Noise
  isr isr2; // Noise
  isr isr3; // Noise

  // time for sim settings
  double t;

  // Constructor to inialize non-zero members
  InstData(
  double fs, double fclk, double fline, double dt, CARRIER mode, SAMPLE_TIME instant, \
  double mean1, double var1, double freq1,                                     \
  double mean2, double var2, double freq2,                                     \
  double mean3, double var3, double freq3,                                     \
  double Hvin_dc_gain, double Hiin_dc_gain, double Hvo_dc_gain, double Vref_adc) {

    // pwm
    this->pwm1 = * (new pwm_data(fs, fclk, dt, mode, instant));

    // control
    this->cntrl = * (new control(fs, fline, mean1, var1, mean2, var2,         \
    mean3, var3, Hvin_dc_gain, Hiin_dc_gain, Hvo_dc_gain, Vref_adc));

    // programs isr according to settings
    this->isr1 = * (new isr(freq1, dt)); // dt not used
    this->isr2 = * (new isr(freq2, dt)); // dt not used
    this->isr3 = * (new isr(freq3, dt)); // dt not used

  };

};

/*------------------------------------------------------------------------------
 * UDATA() definition -- regenerate the template with QSpice and revise this
 * whenever ports/attributes change; make input/attribute parameters const&
 *----------------------------------------------------------------------------*/
#define UDATA(data)                                                           \
  double  vline_sense  = data[ 0].d; /* input */                              \
  double  Vo_sense     = data[ 1].d; /* input */                              \
  double  IL_sense     = data[ 2].d; /* input */                              \
  double  fs           = data[ 3].d; /* input parameter */                    \
  double  fclk         = data[ 4].d; /* input parameter */                    \
  double  fline        = data[ 5].d; /* input parameter */                    \
  double  tdead        = data[ 6].d; /* input parameter */                    \
  int     carrier_mode = data[ 7].i; /* input parameter */                    \
  int     sample_mode  = data[ 8].i; /* input parameter */                    \
  int     seed         = data[ 9].i; /* input parameter */                    \
  double  mean1        = data[10].d; /* input parameter */                    \
  double  var1         = data[11].d; /* input parameter */                    \
  double  freq1        = data[12].d; /* input parameter */                    \
  double  mean2        = data[13].d; /* input parameter */                    \
  double  var2         = data[14].d; /* input parameter */                    \
  double  freq2        = data[15].d; /* input parameter */                    \
  double  mean3        = data[16].d; /* input parameter */                    \
  double  var3         = data[17].d; /* input parameter */                    \
  double  freq3        = data[18].d; /* input parameter */                    \
  double  Hvin_dc_gain = data[19].d; /* input parameter */                    \
  double  Hiin_dc_gain = data[20].d; /* input parameter */                    \
  double  Hvo_dc_gain  = data[21].d; /* input parameter */                    \
  double  Vref_adc     = data[22].d; /* input parameter */                    \
  double &Shigh        = data[23].d; /* output */                             \
  double &Slow         = data[24].d; /* output */                             \
  double &duty         = data[25].d; /* output */                             \
  double &Vin_sample   = data[26].d; /* output */                             \
  double &Iin_sample   = data[27].d; /* output */                             \
  double &Vo_sample    = data[28].d; /* output */                             \
  double &sampleFlag   = data[29].d; /* output */                             \
  double &noise1       = data[30].d; /* output */                             \
  double &noise2       = data[31].d; /* output */                             \
  double &noise3       = data[32].d; /* output */                             \
  double &debug        = data[33].d; /* output */



/*------------------------------------------------------------------------------
 * Evaluation Function -- name must match DLL name, all lower case
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void digital_controller(
    InstData **opaque, double t, uData data[]) {

  UDATA(data);

  InstData *inst = *opaque;

  if (!inst) {
    // allocate instance data
    inst = *opaque = new InstData(
      fs, fclk, fline, tdead, (CARRIER)carrier_mode, (SAMPLE_TIME)sample_mode, \
      mean1, var1, freq1, mean2, var2, freq2, mean3, var3, freq3,              \
      Hvin_dc_gain, Hiin_dc_gain, Hvo_dc_gain,Vref_adc);

    if (!inst) {
      // terminate with prejudice
      msg("Memory allocation failure.  Terminating simulation.\n");
      exit(1);
    }

    // Initialize random number generator
    srand(seed);

    // if important, output component parameters
    msg("DLL compiled with toolset: %s\n", TOOLSET);
  }

  // Stores time
  inst->t = t;

  // Packs data for control function
  inputData indata = {vline_sense, Vo_sense, IL_sense, t};

/* -------------------------------------------------------------------------- */
/*                              Runs isr handler                              */
/* -------------------------------------------------------------------------- */
  inst->isr1.isr_handler(&t, &inst->cntrl.isr1, &inst->cntrl, &indata);
  inst->isr2.isr_handler(&t, &inst->cntrl.isr2, &inst->cntrl, &indata);
  inst->isr3.isr_handler(&t, &inst->cntrl.isr3, &inst->cntrl, &indata);


/* -------------------------------------------------------------------------- */
/*                              Runs pwm handler                              */
/* -------------------------------------------------------------------------- */
  inst->pwm1.pwm_gen(&t, &inst->cntrl, &indata);

/* -------------------------------------------------------------------------- */
/*                               Updates outputs                              */
/* -------------------------------------------------------------------------- */
  Shigh = (double)inst->pwm1.pwm_high;
  Slow = (double)inst->pwm1.pwm_low;
  duty = (double)inst->cntrl.cntrl_out.duty;
  sampleFlag = (double)inst->pwm1.sample_flag;
  noise1 = (double)inst->cntrl.isr1.std_noise;
  noise2 = (double)inst->cntrl.isr2.std_noise;
  noise3 = (double)inst->cntrl.isr3.std_noise;
  Vin_sample = (double)inst->cntrl.vin_meas;
  Iin_sample = (double)inst->cntrl.iin_meas;
  Vo_sample = (double)inst->cntrl.vo_meas;
  debug = (double)inst->cntrl.duty;
}



/*------------------------------------------------------------------------------
 * MaxExtStepSize()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) double MaxExtStepSize(InstData *inst) {

  double step_pwm = inst->pwm1.pwm_max_step_handler(inst->t);
  double step_isr1 = inst->isr1.isr_max_step_handler(inst->t);
  double step_isr2 = inst->isr2.isr_max_step_handler(inst->t);
  double step_isr3 = inst->isr3.isr_max_step_handler(inst->t);

  double min = step_pwm;

  if(min > step_isr1)  min = step_isr1;
  if(min > step_isr2)  min = step_isr2;
  if(min > step_isr3)  min = step_isr3;

  return min;

}


/*------------------------------------------------------------------------------
 * Trunc()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, union uData *data, double *timestep) {


  inst->isr1.isr_trunc_handler(timestep, t);
  inst->isr2.isr_trunc_handler(timestep, t);
  inst->isr3.isr_trunc_handler(timestep, t);
  inst->pwm1.pwm_trunc_handler(timestep, t);

}


/*------------------------------------------------------------------------------
 * Destroy()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // if important, output a final component message; for example:

  msg("Done, records processed, file closed, whatever.\n");

  // free allocated memory
  delete inst;
  inst = nullptr;
}
/*==============================================================================
 * End of Cblock.cpp
 *============================================================================*/
