#include "../Parameter_files/INIT_PARAMS.H"
#include "../Parameter_files/ANAL_PARAMS.H"
#include "../Parameter_files/HEAT_PARAMS.H"
#include "bubble_helper_progs.c"
#include "elec_interp.c"
#include "../Parameter_files/Variables.h"

#define NSPEC_MAX (int) 23
#define RECFAST_NPTS (int) 501
#define KAPPA_10_NPTS (int) 27
#define KAPPA_10_elec_NPTS (int) 20
#define KAPPA_10_pH_NPTS (int) 17

#define KAPPA_10_NPTS_Spline (int) 30
#define KAPPA_10_elec_NPTS_Spline (int) 30
#define KAPPA_10_pH_NPTS_Spline (int) 30

#define X_RAY_Tvir_POINTS (int) 100

#define zpp_interp_points (int) (400)                  /* Number of interpolation points for the interpolation table for z'' */
#define dens_Ninterp (int) (400)                       /* Number of interpolation points for the interpolation table for the value of the density field */


/* Define some global variables; yeah i know it isn't "good practice" but doesn't matter */
//double zpp_edge[NUM_FILTER_STEPS_FOR_Ts], sigma_atR[NUM_FILTER_STEPS_FOR_Ts], sigma_Tmin[NUM_FILTER_STEPS_FOR_Ts], ST_over_PS[NUM_FILTER_STEPS_FOR_Ts], sum_lyn[NUM_FILTER_STEPS_FOR_Ts];
unsigned long long box_ct;
double const_zp_prefactor, dt_dzp, x_e_ave;
double growth_factor_zp, dgrowth_factor_dzp, PS_ION_EFF;
int NO_LIGHT;
float M_MIN_at_z, M_MIN_at_zp;
FILE *LOG;

int n_redshifts_1DTable;
double zbin_width_1DTable,zmin_1DTable,zmax_1DTable,zbin_width_1DTable;

double *FgtrM_1DTable_linear;

/* initialization routine */
int init_heat();

/* A simple linear 2D table for quickly estimating Fcoll given a z and Tvir (for Ts.c) */
int init_FcollTable(float zmin, float zmax);

/* destruction/deallocation routine */
void destruct_heat();

 /* returns the spectral emissity */
double spectral_emissivity(double nu_norm, int flag);

/* Ionization fraction from RECFAST. */
double xion_RECFAST(float z, int flag);

/* IGM temperature from RECFAST; includes Compton heating and adiabatic expansion only. */
double T_RECFAST(float z, int flag);

/* Main driver for evolution */
void evolveInt(float zp, float curr_delNL0[], double freq_int_heat[], double freq_int_ion[], double freq_int_lya[], int COMPUTE_Ts, double y[], double deriv[]);

float dfcoll_dz(float z, float Tmin, float del_bias, float sig_bias);

/* Compton heating rate */
double dT_comp(double z, double TK, double xe);

/* Calculates the optical depth for a photon arriving at z = zp with frequency nu,
 emitted at z = zpp */
double tauX(double nu, double x_e, double zp, double zpp, double HI_filling_factor_zp);
double tauX_approx(double nu, double x_e, double zp, double zpp, double HI_filling_factor_zp);

/* The total weighted HI + HeI + HeII  cross-section in pcm^-2 */
double species_weighted_x_ray_cross_section(double nu, double x_e);

/* Returns the frequency threshold where \tau = 1 between zp and zpp,
 in the IGM with mean electron fraction x_e */
double nu_tau_one(double zp, double zpp, double x_e, double HI_filling_factor_zp);
double nu_tau_one_approx(double zp, double zpp, double x_e, double HI_filling_factor_zp);

 /* Main integral driver for the frequency integral in the evolution equations */
double integrate_over_nu(double zp, double local_x_e, double lower_int_limit, int FLAG);

/* Returns the maximum redshift at which a Lyn transition contributes to Lya
   flux at z */
float zmax(float z, int n);

/* Returns frequency of Lyman-n, in units of Lyman-alpha */
double nu_n(int n);

/* Returns recycling fraction (=fraction of photons converted into
 * Lyalpha for Ly-n resonance */
double frecycle(int n);

 /* returns the spin temperature */
float get_Ts(float z, float delta, float TK, float xe, float Jalpha, float * curr_xalpha);

//float get_Ts_float(float z, float delta, float TK, float xe, float Jalpha, float * curr_xalpha);

float get_Ts_float(float z, float delta, float TK, float xe, float Jalpha);

/* Spin Temperature helper functions */
double xcoll(double z, double TK, double delta, double xe);
float xcoll_float(float z, float TK, float delta, float xe);
double xcoll_HI(double z, double TK, double delta, double xe);
float xcoll_HI_float(float z, float TK, float delta, float xe);
double xcoll_elec(double z, double TK, double delta, double xe);
float xcoll_elec_float(float z, float TK, float delta, float xe);
double xcoll_prot(double z, double TK, double delta, double xe);
float xcoll_prot_float(float z, float TK, float delta, float xe);
double kappa_10_pH(double T, int flag);
double kappa_10_pH_float(double T, int flag);
double kappa_10_elec(double T, int flag);
double kappa_10_elec_float(double T, int flag);
double kappa_10(double TK, int flag);
double kappa_10_float(double TK, int flag);
double xalpha_tilde(double z, double Jalpha, double TK, double TS,
		    double delta, double xe);
float xalpha_tilde_float(float z, float Jalpha, float TK, float TS,
                    float delta, float xe);
double taugp(double z, double delta, double xe);
double Salpha_tilde(double TK, double TS, double tauGP);
float Salpha_tilde_float(float TK, float TS, float tauGP);
double Tc_eff(double TK, double TS);
float Tc_eff_float(float TK, float TS);
/**********  END PROTOTYPE DEFINITIONS  *******************************/


int init_heat()
{
    kappa_10(1.0,1);
    kappa_10_float(1.0,1);
    kappa_10_elec_float(1.0,1);
    kappa_10_pH_float(1.0,1);
    if( kappa_10_elec(1.0,1) < 0)
        return -2;
    if( kappa_10_pH(1.0,1) < 0)
        return -3;
    if (T_RECFAST(100, 1) < 0)
        return -4;
    if (xion_RECFAST(100, 1) < 0)
        return -5;
    if (spectral_emissivity(0,1) < 0)
        return -6;
    if( kappa_10_elec_float(1.0,1) < 0)
        return -2;
    if( kappa_10_pH_float(1.0,1) < 0)
        return -3;

    initialize_interp_arrays();

    return 0;
}

int init_FcollTable(float zmin, float zmax)
{

    int i;
    double z_table;

    zmin_1DTable = zmin;
    zmax_1DTable = 1.2*zmax;

    zbin_width_1DTable = 0.1;

    n_redshifts_1DTable = (int)ceil((zmax_1DTable - zmin_1DTable)/zbin_width_1DTable);

    FgtrM_1DTable_linear = (double *)calloc(n_redshifts_1DTable,sizeof(double));

    for(i=0;i<n_redshifts_1DTable;i++) {
        z_table = zmin_1DTable + zbin_width_1DTable*(double)i;

        FgtrM_1DTable_linear[i] = log10(FgtrM(z_table, get_M_min_ion(z_table)));
    }

    return 0;
}


void free_FcollTable()
{
    free(FgtrM_1DTable_linear);
}



void destruct_heat()
{
  kappa_10(1.0,2);
  kappa_10_float(1.0,2);
  kappa_10_elec(1.0,2);
  kappa_10_elec_float(1.0,2);
  kappa_10_pH(1.0,2);
  kappa_10_pH_float(1.0,2);
  T_RECFAST(100.0,2);
  xion_RECFAST(100.0,2);
  spectral_emissivity(0.0, 2);
}


/* Returns the maximum redshift at which a Lyn transition contributes to Lya
   flux at z */
float zmax(float z, int n){
  double num, denom;
  num = 1 - pow(n+1, -2);
  denom = 1 - pow(n, -2);
  return (1+z)*num/denom - 1;
}


/* Returns frequency of Lyman-n, in units of Lyman-alpha */
double nu_n(int n)
{
  double ans;

  ans = 1.0 - pow(n, -2.0);
  ans /= 0.75;
  return ans;
}


/* Returns recycling fraction (=fraction of photons converted into
 * Lyalpha for Ly-n resonance */
double frecycle(int n)
{
  switch (n){
  case 0:
    return 1;
  case 1:
    return 1;
  case 2:
    return 1;
  case 3:
    return 0;
  case 4:
    return 0.2609;
  case 5:
    return 0.3078;
  case 6:
    return 0.3259;
  case 7:
    return 0.3353;
  case 8:
    return 0.3410;
  case 9:
    return 0.3448;
  case 10:
    return 0.3476;
  case 11:
    return 0.3496;
  case 12:
    return 0.3512;
  case 13:
    return 0.3524;
  case 14:
    return 0.3535;
  case 15:
    return 0.3543;
  case 16:
    return 0.3550;
  case 17:
    return 0.3556;
  case 18:
    return 0.3561;
  case 19:
    return 0.3565;
  case 20:
    return 0.3569;
  case 21:
    return 0.3572;
  case 22:
    return 0.3575;
  case 23:
    return 0.3578;
  case 24:
    return 0.3580;
  case 25:
    return 0.3582;
  case 26:
    return 0.3584;
  case 27:
    return 0.3586;
  case 28:
    return 0.3587;
  case 29:
    return 0.3589;
  case 30:
    return 0.3590;
  default:
    return 0;
  }
}


/* Reads in and constructs table of the piecewise power-law fits to Pop 2 and
 * Pop 3 stellar spectra, from Barkana */
double spectral_emissivity(double nu_norm, int flag)
{
  static int n[NSPEC_MAX];
  static float nu_n[NSPEC_MAX], alpha_S_2[NSPEC_MAX];
  static float alpha_S_3[NSPEC_MAX], N0_2[NSPEC_MAX], N0_3[NSPEC_MAX];
  double n0_fac;
//  double ans, tot, lya;
  double ans;
  int i;
  FILE *F;

  if (flag == 1) {
    /* Read in the data */
    if (!(F = fopen(STELLAR_SPECTRA_FILENAME, "r"))){
      fprintf(stderr, "spectral_emissivity: Unable to open file: stellar_spectra.dat for reading\nAborting\n");
      fprintf(LOG, "spectral_emissivity: Unable to open file: stellar_spectra.dat for reading\nAborting\n");
      return -1;
    }

    for (i=1;i<NSPEC_MAX;i++) {
      fscanf(F, "%i %e %e %e %e", &n[i], &N0_2[i], &alpha_S_2[i], &N0_3[i], &alpha_S_3[i]);
      //      printf("%i\t%e\t%e\t%e\t%e\n", n[i], N0_2[i], alpha_S_2[i], N0_3[i], alpha_S_3[i]);
    }
    fclose(F);

    for (i=1;i<NSPEC_MAX;i++) {
      nu_n[i] = 4.0/3.0*(1.0-1.0/pow(n[i],2.0));
    }

    for (i=1;i<(NSPEC_MAX-1);i++) {
      n0_fac = (pow(nu_n[i+1],alpha_S_2[i]+1) - pow(nu_n[i],alpha_S_2[i]+1));
      N0_2[i] *= (alpha_S_2[i]+1)/n0_fac*Pop2_ion;
      n0_fac = (pow(nu_n[i+1],alpha_S_3[i]+1) - pow(nu_n[i],alpha_S_3[i]+1));
      N0_3[i] *= (alpha_S_3[i]+1)/n0_fac*Pop3_ion;
    }

    return 0.0;
  }

  ans = 0.0;
  for (i=1;i<(NSPEC_MAX-1);i++) {
    //    printf("checking between %e and %e\n", nu_n[i], nu_n[i+1]);
    if ((nu_norm >= nu_n[i]) && (nu_norm < nu_n[i+1])) {
      // We are in the correct spectral region
      if (Pop == 2)
	ans = N0_2[i]*pow(nu_norm,alpha_S_2[i]);
      else
	ans = N0_3[i]*pow(nu_norm,alpha_S_3[i]);
      //           printf("nu_norm=%e i=%i ans=%e\n", nu_norm, i, ans);
      return ans/Ly_alpha_HZ;
    }
  }

  i= NSPEC_MAX-1;
  if (Pop == 2)
    return  N0_2[i]*pow(nu_norm,alpha_S_2[i])/Ly_alpha_HZ;
  else
    return N0_3[i]*pow(nu_norm,alpha_S_3[i])/Ly_alpha_HZ;
  //  return 0;
}

/********************************************************************
 ************************** IGM Evolution ***************************
  This function creates the d/dz' integrands
*********************************************************************/
void evolveInt(float zp, float curr_delNL0[], double freq_int_heat[], double freq_int_ion[], double freq_int_lya[], int COMPUTE_Ts, double y[], double deriv[]){

    double  dfdzp, dadia_dzp, dcomp_dzp, dxheat_dt, ddz, dxion_source_dt, dxion_sink_dt;
    double zpp, dzpp, nu_temp;
    int zpp_ct;
    double T, x_e, dTdzp, dx_edzp, dfcoll, zpp_integrand;
    double dxe_dzp, n_b, dspec_dzp, dxheat_dzp, dxlya_dt, dstarlya_dt;

    x_e = y[0];
    T = y[1];
    n_b = N_b0 * pow(1+zp, 3) * (1+curr_delNL0[0]*growth_factor_zp);

    // First, let's do the trapazoidal integration over zpp
    dxheat_dt = 0;
    dxion_source_dt = 0;
    dxlya_dt = 0;
    dstarlya_dt = 0;
    if (!NO_LIGHT){
        for (zpp_ct = 0; zpp_ct < NUM_FILTER_STEPS_FOR_Ts; zpp_ct++){
            // set redshift of half annulus; dz'' is negative since we flipped limits of integral
            if (zpp_ct==0){
                zpp = (zpp_edge[0]+zp)*0.5;
                dzpp = zp - zpp_edge[0];
            }
            else{
                zpp = (zpp_edge[zpp_ct]+zpp_edge[zpp_ct-1])*0.5;
                dzpp = zpp_edge[zpp_ct-1] - zpp_edge[zpp_ct];
            }

            dfcoll = dfcoll_dz(zpp, sigma_Tmin[zpp_ct], curr_delNL0[zpp_ct], sigma_atR[zpp_ct]);
            dfcoll *= ST_over_PS[zpp_ct] * dzpp; // this is now a positive quantity
            zpp_integrand = dfcoll * (1+curr_delNL0[zpp_ct]*dicke(zpp)) * pow(1+zpp, -X_RAY_SPEC_INDEX);
            dxheat_dt += zpp_integrand * freq_int_heat[zpp_ct];
            dxion_source_dt += zpp_integrand * freq_int_ion[zpp_ct];
            if (COMPUTE_Ts){
                dxlya_dt += zpp_integrand * freq_int_lya[zpp_ct];
                dstarlya_dt += dfcoll * (1+curr_delNL0[zpp_ct]*dicke(zpp)) * pow(1+zp,2)*(1+zpp) * sum_lyn[zpp_ct];
            }
        }

        // add prefactors
        dxheat_dt *= const_zp_prefactor;
        dxion_source_dt *= const_zp_prefactor;
        if (COMPUTE_Ts){
            dxlya_dt *= const_zp_prefactor*n_b;
            dstarlya_dt *= F_STAR * C * N_b0 / FOURPI;

            /*
             if ((dxlya_dt < 0) || (dstarlya_dt<0)){
             printf("***Jalpha_x=%e, Jalpha_star=%e\n", dxlya_dt, dstarlya_dt);
             for (zpp_ct = 0; zpp_ct < NUM_FILTER_STEPS_FOR_Ts; zpp_ct++)
             printf("freq_int=%e, sum_lyn=%e\n", freq_int_lya[zpp_ct], sum_lyn[zpp_ct]);

             }
             */
        }
    } // end NO_LIGHT if statement

    /**** Now we can solve the evolution equations  *****/

    /*** First let's do dxe_dzp ***/

    dxion_sink_dt = alpha_A(T) * CLUMPING_FACTOR * x_e*x_e * f_H * n_b;
    dxe_dzp = dt_dzp*(dxion_source_dt - dxion_sink_dt);
    deriv[0] = dxe_dzp;

    /*
     if (box_ct==1000000){
     printf("%g\t%g\t%g\n", x_e, zpp_integrand);
     printf("(z', freq_int_ion[zpp_ct])\n");
     for (zpp_ct = 0; zpp_ct < NUM_FILTER_STEPS_FOR_Ts; zpp_ct++){
        printf("(%g, %g)\n", zpp_ct, freq_int_ion[zpp_ct]);
     }
     printf("source term is %e, sink term is %e\n", dt_dzp*dxion_source_dt, dt_dzp*dxion_sink_dt);
     printf("dxe_dzp=%e\n", dxe_dzp);
     }
     */

    /*** Next, let's get the temperature components ***/

    // first, adiabatic term
    dadia_dzp = 3/(1.0+zp);
    if (fabs(curr_delNL0[0]) > FRACT_FLOAT_ERR) // add adiabatic heating/cooling from structure formation
        dadia_dzp += dgrowth_factor_dzp/(1.0/curr_delNL0[0]+growth_factor_zp);
    dadia_dzp *= (2.0/3.0)*T;
    //  printf("dTstructure/dz=%e, total Adiabatic heat/dzp (structure + expansion) at zp=%.3f = %e\n", dgrowth_factor_dzp/(1.0/curr_delNL0[0]+growth_factor_zp), zp, dadia_dzp);

    // next heating due to the changing species
    dspec_dzp = - dxe_dzp * T / (1+x_e);
    //  printf("changing species, dT/dz'=%e\n", dspec_dzp);

    // next, Compton heating
    dcomp_dzp = dT_comp(zp, T, x_e);
    //  printf("Compton heat/dzp at zp=%.3f = %e\n", zp, dcomp_dzp);

    // lastly, X-ray heating
    dxheat_dzp = dxheat_dt * dt_dzp * 2.0 / 3.0 / k_B / (1.0+x_e);
    //  printf("x-ray heating, dT/dz=%e\n", dxheat_dzp);

    // summing them up...
    deriv[1] = dxheat_dzp + dcomp_dzp + dspec_dzp + dadia_dzp;
    //  printf("total dT/dz=%e\n", deriv[1]);

    /*** Finally, if we are at the last redshift step, Lya ***/
    deriv[2] = dxlya_dt + dstarlya_dt;

    // stuff for marcos
    deriv[3] = dxheat_dzp;
    deriv[4] = dt_dzp*dxion_source_dt;
}

/*
  Evaluates the frequency integral in the Tx evolution equation
  photons starting from zpp arive at zp, with mean IGM electron
  fraction of x_e (used to compute tau), and local electron
  fraction local_x_e
  FLAG = 0 for heat integral
  FLAG = 1 for ionization integral
  FLAG = 2 for Lya integral
*/
double integrand_in_nu_heat_integral(double nu, void * params){
  double species_sum;
  float x_e = *(double *) params;

  // HI
  species_sum = interp_fheat((nu - NUIONIZATION)/NU_over_EV, x_e)
               * hplank*(nu - NUIONIZATION) * f_H * (1-x_e) * HI_ion_crosssec(nu);

  // HeI
  species_sum += interp_fheat((nu - HeI_NUIONIZATION)/NU_over_EV, x_e)
               * hplank*(nu - HeI_NUIONIZATION) * f_He * (1-x_e) * HeI_ion_crosssec(nu);

  // HeII
  species_sum += interp_fheat((nu - HeII_NUIONIZATION)/NU_over_EV, x_e)
               * hplank*(nu - HeII_NUIONIZATION) * f_He * x_e * HeII_ion_crosssec(nu);

  return species_sum * pow(nu/NU_X_THRESH, -X_RAY_SPEC_INDEX-1);
}
double integrand_in_nu_ion_integral(double nu, void * params){
  double species_sum, F_i;
  float x_e = *(double *) params;

  // photoionization of HI, prodicing e- of energy h*(nu - nu_HI)
  F_i = interp_nion_HI((nu - NUIONIZATION)/NU_over_EV, x_e) +
    interp_nion_HeI((nu - NUIONIZATION)/NU_over_EV, x_e) +
    interp_nion_HeII((nu - NUIONIZATION)/NU_over_EV, x_e) + 1;
  species_sum = F_i * f_H * (1-x_e) * HI_ion_crosssec(nu);

  // photoionization of HeI, prodicing e- of energy h*(nu - nu_HeI)
  F_i = interp_nion_HI((nu - HeI_NUIONIZATION)/NU_over_EV, x_e) +
    interp_nion_HeI((nu - HeI_NUIONIZATION)/NU_over_EV, x_e) +
    interp_nion_HeII((nu - HeI_NUIONIZATION)/NU_over_EV, x_e) + 1;
  species_sum += F_i * f_He * (1-x_e) * HeI_ion_crosssec(nu);

  // photoionization of HeII, prodicing e- of energy h*(nu - nu_HeII)
  F_i = interp_nion_HI((nu - HeII_NUIONIZATION)/NU_over_EV, x_e) +
    interp_nion_HeI((nu - HeII_NUIONIZATION)/NU_over_EV, x_e) +
    interp_nion_HeII((nu - HeII_NUIONIZATION)/NU_over_EV, x_e) + 1;
  species_sum += F_i * f_He * x_e * HeII_ion_crosssec(nu);

  return species_sum * pow(nu/NU_X_THRESH, -X_RAY_SPEC_INDEX-1);
}
double integrand_in_nu_lya_integral(double nu, void * params){
  double species_sum;
  float x_e = *(double *) params;

  // HI
  species_sum = interp_n_Lya((nu - NUIONIZATION)/NU_over_EV, x_e)
    * f_H * (double)(1-x_e) * HI_ion_crosssec(nu);

  // HeI
  species_sum += interp_n_Lya((nu - HeI_NUIONIZATION)/NU_over_EV, x_e)
    * f_He * (double)(1-x_e) * HeI_ion_crosssec(nu);

  // HeII
  species_sum += interp_n_Lya((nu - HeII_NUIONIZATION)/NU_over_EV, x_e)
    * f_He * (double)x_e * HeII_ion_crosssec(nu);

  return species_sum * pow(nu/NU_X_THRESH, -X_RAY_SPEC_INDEX-1);
}
double integrate_over_nu(double zp, double local_x_e, double lower_int_limit, int FLAG){
       double result, error;
       double rel_tol  = 0.01; //<- relative tolerance
       gsl_function F;
       gsl_integration_workspace * w
	 = gsl_integration_workspace_alloc (1000);

       if (DEBUG_ON){
	 printf("integrate over nu, parameters: %f, %f, %e, %i, thread# %i\n", zp, local_x_e, lower_int_limit, FLAG, omp_get_thread_num());
       }
       /*
       if (DO_NOT_COMPARE_NUS)
	 lower_int_limit = NU_X_THRESH;
       else
	 lower_int_limit = FMAX(nu_tau_one(zp, zpp, x_e, HI_filling_factor_zp), NU_X_THRESH);
       */

       F.params = &local_x_e;

       //       printf("lower limit is %e\n", lower_int_limit / NU_over_EV);
       if (FLAG==0)
	 F.function = &integrand_in_nu_heat_integral;
       else if (FLAG==1)
	 F.function = &integrand_in_nu_ion_integral;
       else {
	 F.function = &integrand_in_nu_lya_integral;
       }

//    gsl_integration_qag (&F, lower_int_limit, NU_X_MAX, 0, rel_tol, 1000, GSL_INTEG_GAUSS61, w, &result, &error);
    gsl_integration_qag (&F, lower_int_limit, NU_X_MAX, 0, rel_tol, 1000, GSL_INTEG_GAUSS15, w, &result, &error);
       gsl_integration_workspace_free (w);


       // if it is the Lya integral, add prefactor
       if (FLAG == 2)
	 return result * C / FOURPI / Ly_alpha_HZ / hubble(zp);

       /*
       if (isnan(result))
	 fprintf(stderr, "We have a NaN in the intergrator with calling params: %g,%g,%g,%i\n", zp, local_x_e, lower_int_limit, FLAG);
       */
       return result;
}



/*
  The total weighted HI + HeI + HeII  cross-section in pcm^-2
  technically, the x_e should be local, line of sight (not global) here,
  but that would be very slow...
*/
double species_weighted_x_ray_cross_section(double nu, double x_e){
  double HI_factor, HeI_factor, HeII_factor;

  HI_factor = f_H * (1-x_e) * HI_ion_crosssec(nu);
  HeI_factor = f_He * (1-x_e) * HeI_ion_crosssec(nu);
  HeII_factor = f_He * x_e * HeII_ion_crosssec(nu);

  return HI_factor + HeI_factor + HeII_factor;
}



/*
  Calculates the optical depth for a photon arriving at z = zp with frequency nu,
  emitted at z = zpp.
  The filling factor of neutral IGM at zp is HI_filling_factor_zp.
*/
typedef struct{
  double nu_0, x_e, ion_eff;
} tauX_params;
double tauX_integrand(double zhat, void *params){

    double n, drpropdz, nuhat, HI_filling_factor_zhat, sigma_tilde, fcoll;
    tauX_params *p = (tauX_params *) params;

    drpropdz = C * dtdz(zhat);
    n = N_b0 * pow(1+zhat, 3);
    nuhat = p->nu_0 * (1+zhat);
    fcoll = FgtrM(zhat, get_M_min_ion(zhat));
    if (fcoll < 1e-20)
        HI_filling_factor_zhat = 1;
    else
        HI_filling_factor_zhat = 1 - p->ion_eff * fcoll/(1.0 - x_e_ave); //simplification to use the <x_e> value at zp and not zhat.  should'nt matter much since the evolution in x_e_ave is slower than fcoll.  in principle should make an array to store past values of x_e_ave..
    if (HI_filling_factor_zhat < 1e-4) HI_filling_factor_zhat = 1e-4; //set a floor for post-reionization stability

    sigma_tilde = species_weighted_x_ray_cross_section(nuhat, p->x_e);
    //  printf("in taux integrand, %e, %e, %e, %e, %e, %f, %e, %e\n", drpropdz * n * HI_filling_factor_zhat * sigma_tilde, drpropdz, n, HI_filling_factor_zhat, sigma_tilde, zhat, p->ion_eff, FgtrM(zhat, get_M_min_ion(zhat)));
    return drpropdz * n * HI_filling_factor_zhat * sigma_tilde;
    }
double tauX(double nu, double x_e, double zp, double zpp, double HI_filling_factor_zp){

    double result, error, fcoll;

    gsl_function F;

    double rel_tol  = 0.005; //<- relative tolerance
//    double rel_tol  = 0.01; //<- relative tolerance
    gsl_integration_workspace * w = gsl_integration_workspace_alloc (1000);
    tauX_params p;

    /*
       if (DEBUG_ON)
        printf("in taux, parameters are: %e, %e, %f, %f, %e\n", nu, x_e, zp, zpp, HI_filling_factor_zp);
    */
    F.function = &tauX_integrand;
    p.nu_0 = nu/(1+zp);
    p.x_e = x_e;
    // effective efficiency for the PS (not ST) mass function; quicker to compute...
    if (HI_filling_factor_zp > FRACT_FLOAT_ERR){
        fcoll = FgtrM(zp, M_MIN_at_zp);
        p.ion_eff = (1.0 - HI_filling_factor_zp) / fcoll * (1.0 - x_e_ave);
        PS_ION_EFF = p.ion_eff;
    }
    else
        p.ion_eff = PS_ION_EFF; // uses the previous one in post reionization regime

    F.params = &p;
    gsl_integration_qag (&F, zpp, zp, 0, rel_tol,1000, GSL_INTEG_GAUSS15, w, &result, &error);
//    gsl_integration_qag (&F, zpp, zp, 0, rel_tol,1000, GSL_INTEG_GAUSS61, w, &result, &error);
    gsl_integration_workspace_free (w);

    /*
    if (DEBUG_ON)
    printf("returning from tauX, return value=%e\n", result);
    */
    return result;
}




/*
 Calculates the optical depth for a photon arriving at z = zp with frequency nu,
 emitted at z = zpp.
 The filling factor of neutral IGM at zp is HI_filling_factor_zp.

 *** Brad Greig (22/11/2016) ***
 An approximation to evaluate this using the global averaged filling factor at that zp. Same
 approximation that uses the global averaged x_e

 Used to speed up Ts.c and remove parameter dependence reducing the dimensionality of the required interpolation
 table in the new version of 21CMMC (including spin-temperature fluctuations).

 */
typedef struct{
    double nu_0, x_e, ion_eff;
} tauX_params_approx;
double tauX_integrand_approx(double zhat, void *params){

    double n, drpropdz, nuhat, sigma_tilde, fcoll, HI_filling_factor_zhat;

    int z_fcoll_int1,z_fcoll_int2;
    float z_fcoll_val1,z_fcoll_val2;

    tauX_params_approx *p = (tauX_params_approx *) params;

    drpropdz = C * dtdz(zhat);
    n = N_b0 * pow(1+zhat, 3);
    nuhat = p->nu_0 * (1+zhat);

    z_fcoll_int1 = (int)floor(( zhat - zmin_1DTable )/zbin_width_1DTable);
    z_fcoll_int2 = z_fcoll_int1 + 1;

    z_fcoll_val1 = zmin_1DTable + zbin_width_1DTable*(float)z_fcoll_int1;
    z_fcoll_val2 = zmin_1DTable + zbin_width_1DTable*(float)z_fcoll_int2;

    fcoll = FgtrM_1DTable_linear[z_fcoll_int1] + ( zhat - z_fcoll_val1 )*( FgtrM_1DTable_linear[z_fcoll_int2] - FgtrM_1DTable_linear[z_fcoll_int1] )/( z_fcoll_val2 - z_fcoll_val1 );

    fcoll = pow(10.,fcoll);
//    fcoll = FgtrM(zhat, get_M_min_ion(zhat));
    if (fcoll < 1e-20)
        HI_filling_factor_zhat = 1;
    else
        HI_filling_factor_zhat = 1 - p->ion_eff * fcoll/(1.0 - x_e_ave); //simplification to use the <x_e> value at zp and not zhat.  should'nt matter much since the evolution in x_e_ave is slower than fcoll.  in principle should make an array to store past values of x_e_ave..
    if (HI_filling_factor_zhat < 1e-4) HI_filling_factor_zhat = 1e-4; //set a floor for post-reionization stability

    sigma_tilde = species_weighted_x_ray_cross_section(nuhat, p->x_e);
    //  printf("in taux integrand, %e, %e, %e, %e, %e, %f, %e, %e\n", drpropdz * n * HI_filling_factor_zhat * sigma_tilde, drpropdz, n, HI_filling_factor_zhat, sigma_tilde, zhat, p->ion_eff, FgtrM(zhat, get_M_min_ion(zhat)));
    return drpropdz * n * HI_filling_factor_zhat * sigma_tilde;
}
double tauX_approx(double nu, double x_e, double zp, double zpp, double HI_filling_factor_zp){

    double result, error, fcoll;

    gsl_function F;

    double rel_tol  = 0.005; //<- relative tolerance
    //    double rel_tol  = 0.01; //<- relative tolerance
    gsl_integration_workspace * w = gsl_integration_workspace_alloc (1000);
    tauX_params_approx p;

    int z_fcoll_int1,z_fcoll_int2;
    float z_fcoll_val1,z_fcoll_val2;

    /*
     if (DEBUG_ON)
     printf("in taux, parameters are: %e, %e, %f, %f, %e\n", nu, x_e, zp, zpp, HI_filling_factor_zp);
     */
    F.function = &tauX_integrand_approx;
    p.nu_0 = nu/(1+zp);
    p.x_e = x_e;

    // effective efficiency for the PS (not ST) mass function; quicker to compute...
    if (HI_filling_factor_zp > FRACT_FLOAT_ERR){
//        fcoll = FgtrM(zp, M_MIN_at_zp);

        z_fcoll_int1 = (int)floor(( zp - zmin_1DTable )/zbin_width_1DTable);
        z_fcoll_int2 = z_fcoll_int1 + 1;

        z_fcoll_val1 = zmin_1DTable + zbin_width_1DTable*(float)z_fcoll_int1;
        z_fcoll_val2 = zmin_1DTable + zbin_width_1DTable*(float)z_fcoll_int2;

        fcoll = FgtrM_1DTable_linear[z_fcoll_int1] + ( zp - z_fcoll_val1 )*( FgtrM_1DTable_linear[z_fcoll_int2] - FgtrM_1DTable_linear[z_fcoll_int1] )/( z_fcoll_val2 - z_fcoll_val1 );

        fcoll = pow(10.,fcoll);

        p.ion_eff = (1.0 - HI_filling_factor_zp) / fcoll * (1.0 - x_e_ave);
        PS_ION_EFF = p.ion_eff;
    }
    else
        p.ion_eff = PS_ION_EFF; // uses the previous one in post reionization regime

    F.params = &p;
    gsl_integration_qag (&F, zpp, zp, 0, rel_tol,1000, GSL_INTEG_GAUSS15, w, &result, &error);
//    gsl_integration_qag (&F, zpp, zp, 0, rel_tol,1000, GSL_INTEG_GAUSS61, w, &result, &error);
    gsl_integration_workspace_free (w);

    /*
     if (DEBUG_ON)
     printf("returning from tauX, return value=%e\n", result);
     */
    return result;
}


/*
 Returns the frequency threshold where \tau_X = 1, given parameter values of
 electron fraction in the IGM outside of HII regions, x_e,
 recieved redshift, zp, and emitted redshift, zpp.

 *** Brad Greig (22/11/2016) ***
 An approximation to evaluate this using the global averaged filling factor at that zp. Same
 approximation that uses the global averaged x_e

 Used to speed up Ts.c and remove parameter dependence reducing the dimensionality of the required interpolation
 table in the new version of 21CMMC (including spin-temperature fluctuations).

 */
typedef struct{
    double x_e, zp, zpp, HI_filling_factor_zp;
} nu_tau_one_params_approx;
double nu_tau_one_helper_approx(double nu, void * params){
    nu_tau_one_params_approx *p = (nu_tau_one_params_approx *) params;
    return tauX_approx(nu, p->x_e, p->zp, p->zpp, p->HI_filling_factor_zp) - 1;
}
double nu_tau_one_approx(double zp, double zpp, double x_e, double HI_filling_factor_zp){

    int status, iter, max_iter;
    const gsl_root_fsolver_type * T;
    gsl_root_fsolver * s;
    gsl_function F;
    double x_lo, x_hi, r=0;
    double relative_error = 0.02;
    nu_tau_one_params_approx p;

    if (DEBUG_ON){
        printf("in nu tau one, called with parameters: zp=%f, zpp=%f, x_e=%e, HI_filling_at_zp=%e\n", zp, zpp, x_e, HI_filling_factor_zp);
    }

    // check if too ionized
    if (x_e > 0.9999){
//        fprintf(stderr,"Ts.c: WARNING: x_e value is too close to 1 for convergence in nu_tau_one\n");
        return -1;
    }

    // select solver and allocate memory
    T = gsl_root_fsolver_brent;
    s = gsl_root_fsolver_alloc(T); // non-derivative based Brent method
    if (!s){
        fprintf(stderr, "Ts.c: Unable to allocate memory in function nu_tau_one!\n");
        return -1;
    }

    //check if lower bound has null
    if (tauX_approx(HeI_NUIONIZATION, x_e, zp, zpp, HI_filling_factor_zp) < 1)
        return HeI_NUIONIZATION;

    // set frequency boundary values
    x_lo= HeI_NUIONIZATION;
    x_hi = 1e6 * NU_over_EV;

    // select function we wish to solve
    p.x_e = x_e;
    p.zp = zp;
    p.zpp = zpp;
    p.HI_filling_factor_zp = HI_filling_factor_zp;
    F.function = &nu_tau_one_helper_approx;
    F.params = &p;
    gsl_root_fsolver_set (s, &F, x_lo, x_hi);

    // iterate until we guess close enough
    if (DEBUG_ON) printf ("%5s [%9s, %9s] %9s %9s\n", "iter", "lower", "upper", "root", "err(est)");
    iter = 0;
    max_iter = 100;
    do{
        iter++;
        status = gsl_root_fsolver_iterate (s);
        r = gsl_root_fsolver_root (s);
        //      printf("iter%i, r=%e\n", iter, r);
        x_lo = gsl_root_fsolver_x_lower (s);
        x_hi = gsl_root_fsolver_x_upper (s);
        status = gsl_root_test_interval (x_lo, x_hi, 0, relative_error);
        if (DEBUG_ON){
            printf ("%5d [%.7e, %.7e] %.7e %.7e\n", iter, x_lo, x_hi, r, (x_hi - x_lo)/r);
            fflush(NULL);
        }
    }
    while (status == GSL_CONTINUE && iter < max_iter);

    // deallocate and return
    gsl_root_fsolver_free (s);
    if (DEBUG_ON) printf("Root found at %e eV", r/NU_over_EV);
    return r;
}





/*
  Returns the frequency threshold where \tau_X = 1, given parameter values of
  electron fraction in the IGM outside of HII regions, x_e,
  recieved redshift, zp, and emitted redshift, zpp.
*/
typedef struct{
  double x_e, zp, zpp, HI_filling_factor_zp;
} nu_tau_one_params;
double nu_tau_one_helper(double nu, void * params){
  nu_tau_one_params *p = (nu_tau_one_params *) params;
  return tauX(nu, p->x_e, p->zp, p->zpp, p->HI_filling_factor_zp) - 1;
}
double nu_tau_one(double zp, double zpp, double x_e, double HI_filling_factor_zp){

    int status, iter, max_iter;
    const gsl_root_fsolver_type * T;
    gsl_root_fsolver * s;
    gsl_function F;
    double x_lo, x_hi, r=0;
    double relative_error = 0.02;
    nu_tau_one_params p;

    if (DEBUG_ON){
        printf("in nu tau one, called with parameters: zp=%f, zpp=%f, x_e=%e, HI_filling_at_zp=%e\n", zp, zpp, x_e, HI_filling_factor_zp);
    }

    // check if too ionized
    if (x_e > 0.9999){
        fprintf(stderr,"Ts.c: WARNING: x_e value is too close to 1 for convergence in nu_tau_one\n");
        return -1;
    }

    // select solver and allocate memory
    T = gsl_root_fsolver_brent;
    s = gsl_root_fsolver_alloc(T); // non-derivative based Brent method
    if (!s){
        fprintf(stderr, "Ts.c: Unable to allocate memory in function nu_tau_one!\n");
        return -1;
    }

    //check if lower bound has null
    if (tauX(HeI_NUIONIZATION, x_e, zp, zpp, HI_filling_factor_zp) < 1)
        return HeI_NUIONIZATION;

    // set frequency boundary values
    x_lo= HeI_NUIONIZATION;
    x_hi = 1e6 * NU_over_EV;

    // select function we wish to solve
    p.x_e = x_e;
    p.zp = zp;
    p.zpp = zpp;
    p.HI_filling_factor_zp = HI_filling_factor_zp;
    F.function = &nu_tau_one_helper;
    F.params = &p;
    gsl_root_fsolver_set (s, &F, x_lo, x_hi);

    // iterate until we guess close enough
    if (DEBUG_ON) printf ("%5s [%9s, %9s] %9s %9s\n", "iter", "lower", "upper", "root", "err(est)");
    iter = 0;
    max_iter = 100;
    do{
        iter++;
        status = gsl_root_fsolver_iterate (s);
        r = gsl_root_fsolver_root (s);
        //      printf("iter%i, r=%e\n", iter, r);
        x_lo = gsl_root_fsolver_x_lower (s);
        x_hi = gsl_root_fsolver_x_upper (s);
        status = gsl_root_test_interval (x_lo, x_hi, 0, relative_error);
        if (DEBUG_ON){
            printf ("%5d [%.7e, %.7e] %.7e %.7e\n", iter, x_lo, x_hi, r, (x_hi - x_lo)/r);
            fflush(NULL);
        }
    }
    while (status == GSL_CONTINUE && iter < max_iter);

    // deallocate and return
    gsl_root_fsolver_free (s);
    if (DEBUG_ON) printf("Root found at %e eV", r/NU_over_EV);
    return r;
}


/*
  Redshift derivative of the conditional collapsed fraction
 */
float dfcoll_dz(float z, float sigma_min, float del_bias, float sig_bias)
{
  double dz,z1,z2;
//  double mu, m1, m2;
  double fc1,fc2,ans;

  dz = 0.001;
  z1 = z + dz;
  z2 = z - dz;
  fc1 = sigmaparam_FgtrM_bias(z1, sigma_min, del_bias, sig_bias);
  fc2 = sigmaparam_FgtrM_bias(z2, sigma_min, del_bias, sig_bias);
  ans = (fc1 - fc2)/(2.0*dz);
  return ans;
}


/* Compton heating term */
double dT_comp(double z, double TK, double xe)
{
  double Trad,ans;

  Trad = T_cmb*(1.0+z);
  ans = (-1.51e-4) * (xe/(1.0+xe+f_He)) /(hubble(z)/Ho)/hlittle*pow(Trad,4.0)/(1.0+z);
  //fprintf(stderr, "%e\t%e\t%e\t%e\n", xe, pow(Trad,4.0), hubble(z)/Ho, ans);
  ans *= Trad - TK;
  return ans;
}



/********************************************************************
 ************************ RECFAST quantities ************************
 ********************************************************************/

/* IGM temperature from RECFAST; includes Compton heating and adiabatic
 * expansion only. */
double T_RECFAST(float z, int flag)
{
  double ans;
  static double zt[RECFAST_NPTS], TK[RECFAST_NPTS];
  static gsl_interp_accel *acc;
  static gsl_spline *spline;
  float currz, currTK, trash;
  int i;
  FILE *F;

  if (flag == 1) {
    // Read in the recfast data
    if ( !(F=fopen(RECFAST_FILENAME, "r")) ){
      fprintf(stderr, "T_RECFAST: Unable to open file: %s for reading\nAborting\n", RECFAST_FILENAME);
      fprintf(LOG, "T_RECFAST: Unable to open file: %s for reading\nAborting\n", RECFAST_FILENAME);
      return -1;
    }

    for (i=(RECFAST_NPTS-1);i>=0;i--) {
      fscanf(F, "%f %E %E %E", &currz, &trash, &trash, &currTK);
      zt[i] = currz;
      TK[i] = currTK;
    }
    fclose(F);

    // Set up spline table
    acc   = gsl_interp_accel_alloc ();
    spline  = gsl_spline_alloc (gsl_interp_cspline, RECFAST_NPTS);
    gsl_spline_init(spline, zt, TK, RECFAST_NPTS);

    return 0;
  }

  if (flag == 2) {
    // Free memory
    gsl_spline_free (spline);
    gsl_interp_accel_free(acc);
    return 0;
  }

  if (z > zt[RECFAST_NPTS-1]) { // Called at z>500! Bail out
    fprintf(stderr, "Called xion_RECFAST with z=%f, bailing out!\n", z);
    fprintf(LOG, "Called xion_RECFAST with z=%f, bailing out!\n", z);
    return -1;
  }
  else { // Do spline
    ans = gsl_spline_eval (spline, z, acc);
  }
  return ans;
}


/* Ionization fraction from RECFAST. */
double xion_RECFAST(float z, int flag)
{
  static double zt[RECFAST_NPTS], xion[RECFAST_NPTS];
  static gsl_interp_accel *acc;
  static gsl_spline *spline;
  float trash, currz, currxion;
  double ans;
  int i;
  FILE *F;


  if (flag == 1) {
    // Initialize vectors
    if ( !(F=fopen(RECFAST_FILENAME, "r")) ){
      fprintf(stderr, "xion_RECFAST: Unable to open file: %s for reading\nAborting\n", RECFAST_FILENAME);
      fprintf(LOG, "xion_RECFAST: Unable to open file: %s for reading\nAborting\n", RECFAST_FILENAME);
      return -1;
    }

    for (i=(RECFAST_NPTS-1);i>=0;i--) {
      fscanf(F, "%f %E %E %E", &currz, &currxion, &trash, &trash);
      zt[i] = currz;
      xion[i] = currxion;
    }
    fclose(F);

    // Set up spline table
    acc   = gsl_interp_accel_alloc ();
    spline  = gsl_spline_alloc (gsl_interp_cspline, RECFAST_NPTS);
    gsl_spline_init(spline, zt, xion, RECFAST_NPTS);

    return 0;
  }

  if (flag == 2) {
    gsl_spline_free (spline);
    gsl_interp_accel_free(acc);
    return 0;
  }

  if (z > zt[RECFAST_NPTS-1]) { // Called at z>500! Bail out
    fprintf(stderr, "Called xion_RECFAST with z=%f, bailing out!\n", z);
    fprintf(LOG, "Called xion_RECFAST with z=%f, bailing out!\n", z);
    return -1;
  }
  else { // Do spline
    ans = gsl_spline_eval (spline, z, acc);
  }
  return ans;
}


float get_Ts(float z, float delta, float TK, float xe, float Jalpha, float * curr_xalpha){
  double Trad,xc,xa_tilde;
  double TS,TSold,TSinv;
  double Tceff;

  Trad = T_cmb*(1.0+z);
  xc = xcoll(z,TK,delta,xe);
  if (Jalpha > 1.0e-20) { /* Must use WF effect */
    TS = Trad;
    TSold = 0.0;
    while (fabs(TS-TSold)/TS > 1.0e-3) {
      TSold = TS;
      xa_tilde = xalpha_tilde(z,Jalpha,TK,TS,delta,xe);
//      Tceff = Tc_eff(TK,TS);
      Tceff = Tc_eff(1./TK,1./TS);
//      TSinv = (1.0/Trad+xa_tilde/Tceff + xc/TK)/(1.0+xa_tilde+xc);
//      TS = 1.0/TSinv;
        TS = (1.0+xa_tilde+xc)/(1.0/Trad+xa_tilde/Tceff + xc/TK);
    }
    *curr_xalpha = xa_tilde;
  } else { /* Collisions only */
//    TSinv = (1.0/Trad + xc/TK)/(1.0 + xc);
//    TS = 1.0/TSinv;
      TS = (1.0 + xc)/(1.0/Trad + xc/TK);
    *curr_xalpha = 0;
  }

  return TS;
}


float get_Ts_float(float z, float delta, float TK, float xe, float Jalpha){
    float Trad,xc,xa_tilde;
    float TS,TSold,TSinv;
    float Tceff;

    Trad = T_cmb*(1.0+z);
    xc = xcoll_float(z,TK,delta,xe);
    if (Jalpha > 1.0e-20) { // Must use WF effect
        TS = Trad;
        TSold = 0.0;
        while (fabs(TS-TSold)/TS > 1.0e-3) {
            TSold = TS;
            xa_tilde = xalpha_tilde_float(z,Jalpha,TK,TS,delta,xe);
            Tceff = Tc_eff_float(1./TK,1./TS);
            TS = (1.0+xa_tilde+xc)/(1.0/Trad+xa_tilde/Tceff + xc/TK);
        }
    } else { // Collisions only
        TS = (1.0 + xc)/(1.0/Trad + xc/TK);
    }

    return TS;
}


double xcoll(double z, double TK, double delta, double xe){
  return xcoll_HI(z,TK,delta,xe) + xcoll_elec(z,TK,delta,xe) + xcoll_prot(z,TK,delta,xe);
}

float xcoll_float(float z, float TK, float delta, float xe){
    return xcoll_HI_float(z,TK,delta,xe) + xcoll_elec_float(z,TK,delta,xe) + xcoll_prot_float(z,TK,delta,xe);
}

float xcoll_HI_float(float z, float TK, float delta, float xe)
{
    float krate,nH,Trad;
    float xcoll;

    Trad = T_cmb*(1.0+z);
    nH = (1.0-xe)*No*pow(1.0+z,3.0)*(1.0+delta);
    krate = kappa_10_float(TK,0);
    xcoll = T21/Trad*nH*krate/A10_HYPERFINE;
    return xcoll;
}

double xcoll_HI(double z, double TK, double delta, double xe)
{
  double krate,nH,Trad;
  double xcoll;

  Trad = T_cmb*(1.0+z);
  nH = (1.0-xe)*No*pow(1.0+z,3.0)*(1.0+delta);
  krate = kappa_10(TK,0);
  xcoll = T21/Trad*nH*krate/A10_HYPERFINE;
  return xcoll;
}

/* Note that this assumes Helium ionized same as Hydrogen */
double xcoll_elec(double z, double TK, double delta, double xe)
{
  double krate,ne,Trad;
  double xcoll;

  Trad = T_cmb*(1.0+z);
  ne = xe*N_b0*pow(1.0+z,3.0)*(1.0+delta);
  krate = kappa_10_elec(TK,0);
  xcoll = T21/Trad*ne*krate/A10_HYPERFINE;
  return xcoll;
}

float xcoll_elec_float(float z, float TK, float delta, float xe)
{
    float krate,ne,Trad;
    float xcoll;

    Trad = T_cmb*(1.0+z);
    ne = xe*N_b0*pow(1.0+z,3.0)*(1.0+delta);
    krate = kappa_10_elec_float(TK,0);
    xcoll = T21/Trad*ne*krate/A10_HYPERFINE;
    return xcoll;
}

double xcoll_prot(double z, double TK, double delta, double xe)
{
  double krate,np,Trad;
  double xcoll;

  Trad = T_cmb*(1.0+z);
  np = xe*No*pow(1.0+z,3.0)*(1.0+delta);
  krate = kappa_10_pH(TK,0);
  xcoll = T21/Trad*np*krate/A10_HYPERFINE;
  return xcoll;
}

float xcoll_prot_float(float z, float TK, float delta, float xe)
{
    float krate,np,Trad;
    float xcoll;

    Trad = T_cmb*(1.0+z);
    np = xe*No*pow(1.0+z,3.0)*(1.0+delta);
    krate = kappa_10_pH_float(TK,0);
    xcoll = T21/Trad*np*krate/A10_HYPERFINE;
    return xcoll;
}

double kappa_10(double TK, int flag)
{
  int i;
  static double tkin[KAPPA_10_NPTS], kap[KAPPA_10_NPTS];
  static gsl_interp_accel *acc;
  static gsl_spline *spline;
  double ans;

  if (flag == 1) { /* Set up spline table */
    /* Initialize kappa from Zygelman (2005), Table 2, column 4 */
    tkin[0] = 1.0; kap[0] = 1.38e-13;
    tkin[1] = 2.0; kap[1] = 1.43e-13;
    tkin[2] = 4.0; kap[2] = 2.71e-13;
    tkin[3] = 6.0; kap[3] = 6.60e-13;
    tkin[4] = 8.0; kap[4] = 1.47e-12;
    tkin[5] = 10.0; kap[5] = 2.88e-12;
    tkin[6] = 15.0; kap[6] = 9.10e-12;
    tkin[7] = 20.0; kap[7] = 1.78e-11;
    tkin[8] = 25.0; kap[8] = 2.73e-11;
    tkin[9] = 30.0; kap[9] = 3.67e-11;
    tkin[10] = 40.0; kap[10] = 5.38e-11;
    tkin[11] = 50.0; kap[11] = 6.86e-11;
    tkin[12] = 60.0; kap[12] = 8.14e-11;
    tkin[13] = 70.0; kap[13] = 9.25e-11;
    tkin[14] = 80.0; kap[14] = 1.02e-10;
    tkin[15] = 90.0; kap[15] = 1.11e-10;
    tkin[16] = 100.0; kap[16] = 1.19e-10;
    tkin[17] = 200.0; kap[17] = 1.75e-10;
    tkin[18] = 300.0; kap[18] = 2.09e-10;
    tkin[19] = 501.0; kap[19] = 2.565e-10;
    tkin[20] = 701.0; kap[20] = 2.91e-10;
    tkin[21] = 1000.0; kap[21] = 3.31e-10;
    tkin[22] = 2000.0; kap[22] = 4.27e-10;
    tkin[23] = 3000.0; kap[23] = 4.97e-10;
    tkin[24] = 5000.0; kap[24] = 6.03e-10;
    tkin[25] = 7000.0; kap[25] = 6.87e-10;
    tkin[26] = 10000.0; kap[26] = 7.87e-10;

    /* Convert to logs for interpolation */
    for (i=0;i<KAPPA_10_NPTS;i++) {
      tkin[i] = log(tkin[i]);
      kap[i] = log(kap[i]);
    }

    /* Set up spline table */
    acc   = gsl_interp_accel_alloc ();
    spline  = gsl_spline_alloc (gsl_interp_cspline, KAPPA_10_NPTS);
    gsl_spline_init(spline, tkin, kap, KAPPA_10_NPTS);
    return 0;
  }

  if (flag == 2) { /* Clear memory */
    gsl_spline_free (spline);
    gsl_interp_accel_free(acc);
    return 0;
  }

  if (log(TK) < tkin[0]) { /* Below 1 K, just use that value */
    ans = kap[0];
  } else if (log(TK) > tkin[KAPPA_10_NPTS-1]) {
    /* Power law extrapolation */
    ans = log(exp(kap[KAPPA_10_NPTS-1])*pow(TK/exp(tkin[KAPPA_10_NPTS-1]),0.381));
  } else { /* Do spline */
    TK = log(TK);
    ans = gsl_spline_eval (spline, TK, acc);
  }
  return exp(ans);
}


double kappa_10_float(double TK, int flag)
{

    static double tkin_spline[KAPPA_10_NPTS_Spline], kap_spline[KAPPA_10_NPTS_Spline];
    double ans;
    int tkin_spline_int;

    if (flag == 1) {

        BinWidth_10 = 0.317597943861;
        inv_BinWidth_10 = 1./0.317597943861;

        tkin_spline[0] = 0.0; kap_spline[0] = -29.6115227098;
        tkin_spline[1] = 0.317597943861; kap_spline[1] = -29.6228184691;
        tkin_spline[2] = 0.635195887722; kap_spline[2] = -29.5917673123;
        tkin_spline[3] = 0.952793831584; kap_spline[3] = -29.4469989515;
        tkin_spline[4] = 1.27039177544; kap_spline[4] = -29.1171430989;
        tkin_spline[5] = 1.58798971931; kap_spline[5] = -28.5382192456;
        tkin_spline[6] = 1.90558766317; kap_spline[6] = -27.7424388865;
        tkin_spline[7] = 2.22318560703; kap_spline[7] = -26.8137036254;
        tkin_spline[8] = 2.54078355089; kap_spline[8] = -25.8749225449;
        tkin_spline[9] = 2.85838149475; kap_spline[9] = -25.0548322235;
        tkin_spline[10] = 3.17597943861; kap_spline[10] = -24.4005076336;
        tkin_spline[11] = 3.49357738247; kap_spline[11] = -23.8952335377;
        tkin_spline[12] = 3.81117532633; kap_spline[12] = -23.5075651004;
        tkin_spline[13] = 4.1287732702; kap_spline[13] = -23.201637629;
        tkin_spline[14] = 4.44637121406; kap_spline[14] = -22.9593758343;
        tkin_spline[15] = 4.76396915792; kap_spline[15] = -22.7534867461;
        tkin_spline[16] = 5.08156710178; kap_spline[16] = -22.5745752086;
        tkin_spline[17] = 5.39916504564; kap_spline[17] = -22.4195690855;
        tkin_spline[18] = 5.7167629895; kap_spline[18] = -22.2833176123;
        tkin_spline[19] = 6.03436093336; kap_spline[19] = -22.1549519419;
        tkin_spline[20] = 6.35195887722; kap_spline[20] = -22.0323282988;
        tkin_spline[21] = 6.66955682109; kap_spline[21] = -21.9149994721;
        tkin_spline[22] = 6.98715476495; kap_spline[22] = -21.800121439;
        tkin_spline[23] = 7.30475270881; kap_spline[23] = -21.6839502137;
        tkin_spline[24] = 7.62235065267; kap_spline[24] = -21.5662434981;
        tkin_spline[25] = 7.93994859653; kap_spline[25] = -21.4473595491;
        tkin_spline[26] = 8.25754654039; kap_spline[26] = -21.3279560712;
        tkin_spline[27] = 8.57514448425; kap_spline[27] = -21.2067614838;
        tkin_spline[28] = 8.89274242811; kap_spline[28] = -21.0835560288;
        tkin_spline[29] = 9.21034037198; kap_spline[29] = -20.9627928675;

    }

    TK = log(TK);

    if (TK < tkin_spline[0]) { // Below 1 K, just use that value
        ans = kap_spline[0];
    } else if (TK > tkin_spline[KAPPA_10_NPTS_Spline-1]) {
        // Power law extrapolation
        ans = log(exp(kap_spline[KAPPA_10_NPTS_Spline-1])*pow(exp(TK)/exp(tkin_spline[KAPPA_10_NPTS_Spline-1]),0.381));
    } else { // Do spline

//        tkin_spline_int = (int)floor((TK - tkin_spline[0])/BinWidth);
        tkin_spline_int = (int)floor((TK - tkin_spline[0])*inv_BinWidth_10);

//        tkin_spline_gradient1 = tkin_spline[0] + BinWidth*(float)tkin_spline_int;
//        tkin_spline_gradient2 = tkin_spline[0] + BinWidth*(float)(tkin_spline_int + 1);

//        ans = kap_spline[tkin_spline_int] + ( TK - tkin_spline_gradient1 )*( kap_spline[tkin_spline_int+1] - kap_spline[tkin_spline_int] )/( tkin_spline_gradient2 - tkin_spline_gradient1 );
        ans = kap_spline[tkin_spline_int] + ( TK - (tkin_spline[0] + BinWidth_10*(float)tkin_spline_int) )*( kap_spline[tkin_spline_int+1] - kap_spline[tkin_spline_int] )*inv_BinWidth_10;
    }

    return exp(ans);
}


/* Interpolate exact results for kappa_10^eH.  The table goes up to
 * 10^5 K, but values are only accurate for T<2x10^4 K.  From
 * Furlanetto & Furlanetto 2006 */
double kappa_10_elec(double T, int flag)
{
  static double TK[KAPPA_10_elec_NPTS], kappa[KAPPA_10_elec_NPTS];
  static gsl_interp_accel *acc;
  static gsl_spline *spline;
  double ans;
  int i;
  float curr_TK, curr_kappa;
  FILE *F;

  if (flag == 1) {
    if (!(F=fopen(KAPPA_EH_FILENAME, "r"))){
      fprintf(stderr, "Unable to open the kappa_10^eH file at %s\nAborting\n", KAPPA_EH_FILENAME);
      fprintf(LOG, "Unable to open the kappa_10^eH file at %s\nAborting\n", KAPPA_EH_FILENAME);
      return 0;
    }

    for (i=0;i<KAPPA_10_elec_NPTS;i++) {
      fscanf(F, "%f %e", &curr_TK, &curr_kappa);
      TK[i] = curr_TK;
      kappa[i] = curr_kappa;
    }
    fclose(F);

    for (i=0;i<KAPPA_10_elec_NPTS;i++) {
      TK[i] = log(TK[i]);
      kappa[i] = log(kappa[i]);
    }

    /* Set up spline table */
    acc   = gsl_interp_accel_alloc ();
    spline  = gsl_spline_alloc (gsl_interp_cspline, KAPPA_10_elec_NPTS);
    gsl_spline_init(spline, TK, kappa, KAPPA_10_elec_NPTS);
    return 0;
  }

  if (flag == 2) {
    /* Free memory */
    gsl_spline_free (spline);
    gsl_interp_accel_free(acc);
    return 0;
  }

  T = log(T);
  if (T < TK[0]) { /* Use TK=1 K value if called at lower temperature */
    ans = kappa[0];
  }
  else if (T > TK[KAPPA_10_elec_NPTS-1]) {
    /* Power law extrapolation */
    ans  = kappa[KAPPA_10_elec_NPTS-1] +
      ((kappa[KAPPA_10_elec_NPTS-1] - kappa[KAPPA_10_elec_NPTS-2]) /
      (TK[KAPPA_10_elec_NPTS-1] - TK[KAPPA_10_elec_NPTS-2]) *
       (T-TK[KAPPA_10_elec_NPTS-1]));
  }
  else { /* Do spline */
    ans = gsl_spline_eval (spline, T, acc);
  }
  return exp(ans);
}

double kappa_10_elec_float(double T, int flag)
{

    static double TK_spline[KAPPA_10_elec_NPTS_Spline], kappa_spline[KAPPA_10_elec_NPTS_Spline];
    double ans;
    int TK_spline_int;

    if (flag == 1) {

        BinWidth_elec = 0.396997429827;
        inv_BinWidth_elec = 1./0.396997429827;

        TK_spline[0] = 0.0; kappa_spline[0] = -22.1549007191;
        TK_spline[1] = 0.396997429827; kappa_spline[1] = -21.9576919899;
        TK_spline[2] = 0.793994859653; kappa_spline[2] = -21.760758435;
        TK_spline[3] = 1.19099228948; kappa_spline[3] = -21.5641795674;
        TK_spline[4] = 1.58798971931; kappa_spline[4] = -21.3680349001;
        TK_spline[5] = 1.98498714913; kappa_spline[5] = -21.1724124486;
        TK_spline[6] = 2.38198457896; kappa_spline[6] = -20.9774403051;
        TK_spline[7] = 2.77898200879; kappa_spline[7] = -20.78327367;
        TK_spline[8] = 3.17597943861; kappa_spline[8] = -20.5901042551;
        TK_spline[9] = 3.57297686844; kappa_spline[9] = -20.3981934669;
        TK_spline[10] = 3.96997429827; kappa_spline[10] = -20.2078762485;
        TK_spline[11] = 4.36697172809; kappa_spline[11] = -20.0195787458;
        TK_spline[12] = 4.76396915792; kappa_spline[12] = -19.8339587914;
        TK_spline[13] = 5.16096658775; kappa_spline[13] = -19.6518934427;
        TK_spline[14] = 5.55796401757; kappa_spline[14] = -19.4745894649;
        TK_spline[15] = 5.9549614474; kappa_spline[15] = -19.3043925781;
        TK_spline[16] = 6.35195887722; kappa_spline[16] = -19.1444129787;
        TK_spline[17] = 6.74895630705; kappa_spline[17] = -18.9986014565;
        TK_spline[18] = 7.14595373688; kappa_spline[18] = -18.8720602784;
        TK_spline[19] = 7.5429511667; kappa_spline[19] = -18.768679825;
        TK_spline[20] = 7.93994859653; kappa_spline[20] = -18.6909581885;
        TK_spline[21] = 8.33694602636; kappa_spline[21] = -18.6387511068;
        TK_spline[22] = 8.73394345618; kappa_spline[22] = -18.6093755705;
        TK_spline[23] = 9.13094088601; kappa_spline[23] = -18.5992098958;
        TK_spline[24] = 9.52793831584; kappa_spline[24] = -18.6050625357;
        TK_spline[25] = 9.92493574566; kappa_spline[25] = -18.6319366207;
        TK_spline[26] = 10.3219331755; kappa_spline[26] = -18.7017996535;
        TK_spline[27] = 10.7189306053; kappa_spline[27] = -18.8477153986;
        TK_spline[28] = 11.1159280351; kappa_spline[28] = -19.0813436512;
        TK_spline[29] = 11.512925465; kappa_spline[29] = -19.408859606;
    }

    T = log(T);

    if (T < TK_spline[0]) { // Below 1 K, just use that value
        ans = kappa_spline[0];
    } else if (T > TK_spline[KAPPA_10_elec_NPTS_Spline-1]) {
        // Power law extrapolation
        ans  = kappa_spline[KAPPA_10_elec_NPTS_Spline-1] + ((kappa_spline[KAPPA_10_elec_NPTS_Spline-1] - kappa_spline[KAPPA_10_elec_NPTS_Spline-2]) / (TK_spline[KAPPA_10_elec_NPTS_Spline-1] - TK_spline[KAPPA_10_elec_NPTS_Spline-2]) * (T-TK_spline[KAPPA_10_elec_NPTS_Spline-1]));

    } else { // Do spline

//        TK_spline_int = (int)floor((T - TK_spline[0])/BinWidth);
        TK_spline_int = (int)floor((T - TK_spline[0])*inv_BinWidth_elec);

//        TK_spline_gradient1 = TK_spline[0] + BinWidth*(float)TK_spline_int;
//        TK_spline_gradient2 = TK_spline[0] + BinWidth*(float)(TK_spline_int + 1);

//        ans = kappa_spline[TK_spline_int] + ( T - TK_spline_gradient1 )*( kappa_spline[TK_spline_int+1] - kappa_spline[TK_spline_int] )/( TK_spline_gradient2 - TK_spline_gradient1 );
        ans = kappa_spline[TK_spline_int] + ( T - ( TK_spline[0] + BinWidth_elec*(float)TK_spline_int ) )*( kappa_spline[TK_spline_int+1] - kappa_spline[TK_spline_int] )*inv_BinWidth_elec;
    }
    return exp(ans);
}



/* Interpolate exact results for kappa_10^pH.  The table goes up to
 * 10^5 K, but values are only accurate for T<2x10^4 K.  From
 * Furlanetto & Furlanetto 2006 */
double kappa_10_pH(double T, int flag)
{
  static double TK[KAPPA_10_pH_NPTS], kappa[KAPPA_10_pH_NPTS];
  static gsl_interp_accel *acc;
  static gsl_spline *spline;
  double ans;
  int i;
  float curr_TK, curr_kappa;
  FILE *F;

  if (flag == 1) {
    if (!(F=fopen(KAPPA_PH_FILENAME, "r"))){
      fprintf(stderr, "Unable to open the kappa_10^pH file at %s\nAborting\n", KAPPA_PH_FILENAME);
      fprintf(LOG, "Unable to open the kappa_10^pH file at %s\nAborting\n", KAPPA_PH_FILENAME);
      return 0;
    }


    for (i=0;i<KAPPA_10_pH_NPTS;i++) {
      fscanf(F, "%f %e", &curr_TK, &curr_kappa);
      TK[i] = curr_TK;
      kappa[i] = curr_kappa;
      //      fprintf(stderr, "scanning %f\t%e\n", TK[i], kappa[i]);
    }
    fclose(F);

    for (i=0;i<KAPPA_10_pH_NPTS;i++) {
      TK[i] = log(TK[i]);
      kappa[i] = log(kappa[i]);
    }

    /* Set up spline table */
    acc   = gsl_interp_accel_alloc ();
    spline  = gsl_spline_alloc (gsl_interp_cspline, KAPPA_10_pH_NPTS);
    gsl_spline_init(spline, TK, kappa, KAPPA_10_pH_NPTS);
    return 0;
  }

  if (flag == 2) {
    /* Free memory */
    gsl_spline_free (spline);
    gsl_interp_accel_free(acc);
    return 0;
  }

  T = log(T);
  if (T < TK[0]){ /* Use TK=1 K value if called at lower temperature */
    ans = kappa[0];
  }
  else if (T > TK[KAPPA_10_pH_NPTS-1]) {
    /* Power law extrapolation */
//      printf("extrapolate\n");
      ans  = kappa[KAPPA_10_pH_NPTS-1] +
      ((kappa[KAPPA_10_pH_NPTS-1] - kappa[KAPPA_10_pH_NPTS-2]) /
      (TK[KAPPA_10_pH_NPTS-1] - TK[KAPPA_10_pH_NPTS-2]) *
       (T-TK[KAPPA_10_pH_NPTS-1]));
  } else { /* Do spline */
//      printf("spline\n");
    ans = gsl_spline_eval (spline, T, acc);
  }
  ans = exp(ans);
  return ans;
}

double kappa_10_pH_float(double T, int flag)
{
    static double TK_spline[KAPPA_10_pH_NPTS_Spline], kappa_spline[KAPPA_10_pH_NPTS_Spline];
    double ans;
    int TK_spline_int;

//    BinWidth = 0.341499570777;

    if (flag == 1) {

        BinWidth_pH = 0.341499570777;
        inv_BinWidth_pH = 1./0.341499570777;

        TK_spline[0] = 0.0; kappa_spline[0] = -21.6395565688;
        TK_spline[1] = 0.341499570777; kappa_spline[1] = -21.5641675629;
        TK_spline[2] = 0.682999141554; kappa_spline[2] = -21.5225112028;
        TK_spline[3] = 1.02449871233; kappa_spline[3] = -21.5130514508;
        TK_spline[4] = 1.36599828311; kappa_spline[4] = -21.5342522691;
        TK_spline[5] = 1.70749785389; kappa_spline[5] = -21.5845293039;
        TK_spline[6] = 2.04899742466; kappa_spline[6] = -21.6581396414;
        TK_spline[7] = 2.39049699544; kappa_spline[7] = -21.7420392948;
        TK_spline[8] = 2.73199656622; kappa_spline[8] = -21.8221380683;
        TK_spline[9] = 3.07349613699; kappa_spline[9] = -21.8837908896;
        TK_spline[10] = 3.41499570777; kappa_spline[10] = -21.9167553997;
        TK_spline[11] = 3.75649527855; kappa_spline[11] = -21.9200173678;
        TK_spline[12] = 4.09799484933; kappa_spline[12] = -21.8938574675;
        TK_spline[13] = 4.4394944201; kappa_spline[13] = -21.8414464728;
        TK_spline[14] = 4.78099399088; kappa_spline[14] = -21.7684762963;
        TK_spline[15] = 5.12249356166; kappa_spline[15] = -21.6796222358;
        TK_spline[16] = 5.46399313243; kappa_spline[16] = -21.5784701374;
        TK_spline[17] = 5.80549270321; kappa_spline[17] = -21.4679438133;
        TK_spline[18] = 6.14699227399; kappa_spline[18] = -21.3503236936;
        TK_spline[19] = 6.48849184477; kappa_spline[19] = -21.2277666787;
        TK_spline[20] = 6.82999141554; kappa_spline[20] = -21.1017425964;
        TK_spline[21] = 7.17149098632; kappa_spline[21] = -20.9733966978;
        TK_spline[22] = 7.5129905571; kappa_spline[22] = -20.8437244283;
        TK_spline[23] = 7.85449012787; kappa_spline[23] = -20.7135746917;
        TK_spline[24] = 8.19598969865; kappa_spline[24] = -20.583135408;
        TK_spline[25] = 8.53748926943; kappa_spline[25] = -20.4523507819;
        TK_spline[26] = 8.8789888402; kappa_spline[26] = -20.3215504736;
        TK_spline[27] = 9.22048841098; kappa_spline[27] = -20.1917429161;
        TK_spline[28] = 9.56198798176; kappa_spline[28] = -20.0629513946;
        TK_spline[29] = 9.90348755254; kappa_spline[29] = -19.9343540344;
    }

    T = log(T);

    if (T < TK_spline[0]) { // Below 1 K, just use that value
        ans = kappa_spline[0];
    } else if (T > TK_spline[KAPPA_10_pH_NPTS_Spline-1]) {
        // Power law extrapolation
        ans  = kappa_spline[KAPPA_10_pH_NPTS_Spline-1] + ((kappa_spline[KAPPA_10_pH_NPTS_Spline-1] - kappa_spline[KAPPA_10_pH_NPTS_Spline-2]) / (TK_spline[KAPPA_10_pH_NPTS_Spline-1] - TK_spline[KAPPA_10_pH_NPTS_Spline-2]) * (T-TK_spline[KAPPA_10_pH_NPTS_Spline-1]));
        //        ans = log(exp(kappa_spline[KAPPA_10_elec_NPTS_Spline-1])*pow(T/exp(TK_spline[KAPPA_10_elec_NPTS_Spline-1]),0.381));
    } else { // Do spline

        TK_spline_int = (int)floor((T - TK_spline[0])*inv_BinWidth_pH);

//        TK_spline_gradient1 = TK_spline[0] + BinWidth*(double)TK_spline_int;
//        TK_spline_gradient2 = TK_spline[0] + BinWidth*(double)(TK_spline_int + 1);

//        ans = kappa_spline[TK_spline_int] + ( T - TK_spline_gradient1 )*( kappa_spline[TK_spline_int+1] - kappa_spline[TK_spline_int] )/( TK_spline_gradient2 - TK_spline_gradient1 );
          ans = kappa_spline[TK_spline_int] + ( T - (TK_spline[0] + BinWidth_pH*(double)TK_spline_int))*( kappa_spline[TK_spline_int+1] - kappa_spline[TK_spline_int] )*inv_BinWidth_pH;
    }
    ans = exp(ans);
    return ans;
}


/********************************************************************
 ********************* Wouthuysen-Field Coupling ********************
 ********************************************************************/

/* NOTE Jalpha is by number */
double xalpha_tilde(double z, double Jalpha, double TK, double TS,
		    double delta, double xe){
  double tgp,Stilde,x;

  tgp = taugp(z,delta,xe);
  Stilde = Salpha_tilde(1./TK,1./TS,tgp);
  x = 1.66e11/(1.0+z)*Stilde*Jalpha;
  return x;
}

float xalpha_tilde_float(float z, float Jalpha, float TK, float TS,
                    float delta, float xe){
    float tgp,Stilde,x;

    tgp = taugp(z,delta,xe);
    Stilde = Salpha_tilde_float(1./TK,1./TS,tgp);
    x = 1.66e11/(1.0+z)*Stilde*Jalpha;
    return x;
}

// Compute the Gunn-Peterson optical depth.
double taugp(double z, double delta, double xe){
  return 1.342881e-7 / hubble(z)*No*pow(1+z,3) * (1.0+delta)*(1.0-xe);
}

double Salpha_tilde(double TK, double TS, double tauGP)
{
  double xi;
  double ans;

  xi = pow(1.0e-7*tauGP*TK*TK, 1.0/3.0);
  ans = (1.0 - 0.0631789*TK + 0.115995*TK*TK - 0.401403*TS*TK + 0.336463*TS*TK*TK)/(1.0 + 2.98394*xi + 1.53583*xi*xi + 3.85289*xi*xi*xi);
  return ans;
}

float Salpha_tilde_float(float TK, float TS, float tauGP)
{
    float xi;
    float ans;

    xi = pow(1.0e-7*tauGP*TK*TK, 1.0/3.0);
    ans = 1.0 - 0.0631789*TK + 0.115995*TK*TK - 0.401403*TS*TK;
    ans += 0.336463*TS*TK*TK;
    ans /= 1.0 + 2.98394*xi + 1.53583*xi*xi + 3.85289*xi*xi*xi;
    return ans;
}

double Tc_eff(double TK, double TS)
{
  double ans;

  ans = 1.0/(TK + 0.405535*TK*(TS - TK));
  return ans;
}

float Tc_eff_float(float TK, float TS)
{
    float ans;

    ans = 1.0/(TK + 0.405535*TK*(TS - TK));
    return ans;
}









///////////////////////////////////////////////////////////////
//JBM:we built a simple linear interpolator that is fast./////
//////////////////////////////////////////////////////////////

#define ZINT_MIN (double) 10.0
#define ZINT_MAX (double) 50.0
#define NZINT (int) 500
#define ZINT_STEP (double) (ZINT_MAX-ZINT_MIN)/(NZINT-1.0)

#define VINT_MIN (double) 0.0
#define VINT_MAX (double) 90.0
#define NVINT (int) 100
#define VINT_STEP (double) (VINT_MAX-VINT_MIN)/(NVINT-1.0)

#define FCOLLMIN (double) 1e-13 //if Fcoll is smaller than this we set it to it to avoid numerical issues

#define FILENAME_FCOLL_VCB  "../External_tables/Fcollapse_table"
#define FILENAME_SIGMACOOL_VCB "../External_tables/sigmacool_table"



double interpol_linear_2D(const double x0, const double dx, const int Nx,
		const double y0, const double dy, const int Ny, double **ftab, const double x, const double y);


double interpol_linear_2D(const double x0,const  double dx, const int Nx, const double y0, const double dy, const int Ny,
    double **ftab, const double x, const double y) {
  //2D linear interpolation routine, assumes both x and y are linearly spaced for speed.
  long ix, iy;



  // Check if in range
  if (  (dx > 0 && (x<x0 || x>x0+dx*(Nx-1)))
      ||(dx < 0 && (x>x0 || x<x0+dx*(Nx-1))) ) {
	fprintf(stderr, "Error: interpol_linear_2D: value out of range in x. Range is (%le,%le) and x=%le. \n",x0,x0+dx*(Nx-1),x);
 //   exit(1);
  }

  if (  (dy > 0 && (y<y0 || y>y0+dy*(Ny-1)))
      ||(dy < 0 && (y>y0 || y<y0+dy*(Ny-1))) ) {
  fprintf(stderr, "Error: interpol_linear_2D: value out of range in y. Range is (%le,%le) and y=%le. \n",y0,y0+dy*(Ny-1),y);
 //   exit(1);
  }

  // Identify locations to interpolate
  ix = (long) round((x-x0)/dx);
  if (ix<1) ix=1;
  if (ix>Nx-2) ix=Nx-2;
  double fracx = x - (x0+dx*ix);

  iy = (long) round((y-y0)/dy);
  if (iy<1) iy=1;
  if (iy>Ny-2) iy=Ny-2;
  double fracy = y - (y0+dy*iy);

  double fcurr = ftab[ix][iy];
  double dfcurr_dx = (ftab[ix+1][iy]-ftab[ix-1][iy])/(2*dx);
  double dfcurr_dy = (ftab[ix][iy+1]-ftab[ix][iy-1])/(2*dy);


  double res = fcurr + dfcurr_dx * fracx + dfcurr_dy * fracy;


  return res;
}
