/* This module contains the functions for the kernel bandwidth function. */

/* Copyright (C) J. Racine, 1995-2001 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <errno.h>

// timing tests
#include <time.h>

#include "headers.h"

#include <R.h>

#ifdef MPI2

#include "mpi.h"

extern  int my_rank;
extern  int source;
extern  int dest;
extern  int tag;
extern  int iNum_Processors;
extern  int iSeed_my_rank;
extern  MPI_Status status;
#endif

/*
int int_LARGE_SF;
int int_DEBUG;
int int_VERBOSE;
int int_NOKEYPRESS;
int int_DISPLAY_CV;
int int_RANDOM_SEED;
int int_MINIMIZE_IO;
int int_ORDERED_CATEGORICAL_GRADIENT;
int int_PREDICT;
int int_ROBUST;
int int_SIMULATION;
int int_TAYLOR;
int int_WEIGHTS;
*/

/* Some externals for numerical routines */

extern int num_obs_train_extern;
extern int num_obs_eval_extern;
extern int num_var_continuous_extern;
extern int num_var_unordered_extern;
extern int num_var_ordered_extern;
extern int num_reg_continuous_extern;
extern int num_reg_unordered_extern;
extern int num_reg_ordered_extern;
extern int *num_categories_extern;
extern double **matrix_categorical_vals_extern;

extern double **matrix_X_continuous_train_extern;
extern double **matrix_X_unordered_train_extern;
extern double **matrix_X_ordered_train_extern;
extern double **matrix_X_continuous_eval_extern;
extern double **matrix_X_unordered_eval_extern;
extern double **matrix_X_ordered_eval_extern;

extern double **matrix_Y_continuous_train_extern;
extern double **matrix_Y_unordered_train_extern;
extern double **matrix_Y_ordered_train_extern;
extern double **matrix_Y_continuous_eval_extern;
extern double **matrix_Y_unordered_eval_extern;
extern double **matrix_Y_ordered_eval_extern;

extern double *vector_Y_extern;
extern double *vector_T_extern;
extern double *vector_Y_eval_extern;

/* Quantile - no Y ordered or unordered used, but defined anyways */

extern double **matrix_Y_continuous_quantile_extern;
extern double **matrix_Y_unordered_quantile_extern;
extern double **matrix_Y_ordered_quantile_extern;
extern double **matrix_X_continuous_quantile_extern;
extern double **matrix_X_unordered_quantile_extern;
extern double **matrix_X_ordered_quantile_extern;

extern int int_ll_extern;

extern int KERNEL_reg_extern;
extern int KERNEL_reg_unordered_extern;
extern int KERNEL_reg_ordered_extern;
extern int KERNEL_den_extern;
extern int KERNEL_den_unordered_extern;
extern int KERNEL_den_ordered_extern;
extern int BANDWIDTH_reg_extern;
extern int BANDWIDTH_den_extern;

extern int itmax_extern;
extern double small_extern;
extern double gamma_extern;
extern double *vector_scale_factor_extern;

extern double y_min_extern;
extern double y_max_extern;

// cdens + trees extern
extern double **matrix_XY_continuous_train_extern;
extern double **matrix_XY_unordered_train_extern;
extern double **matrix_XY_ordered_train_extern;
extern double **matrix_XY_continuous_eval_extern;
extern double **matrix_XY_unordered_eval_extern;
extern double **matrix_XY_ordered_eval_extern;

// cdf extern
extern double dbl_memfac_ccdf_extern;
extern double dbl_memfac_dls_extern;

// timing
extern double timing_extern;

#ifdef RCSID
static char rcsid[] = "$Id: kernelcv.c,v 1.9 2006/11/02 16:56:49 tristen Exp $";
#endif

#define RBWM_CVAIC 0
#define RBWM_CVLS 1

#define LL_LC  0
#define LL_LL  1

#define BW_FIXED   0
#define BW_GEN_NN  1
#define BW_ADAP_NN 2


double cv_func_regression_categorical_ls(double *vector_scale_factor){
  double cv = 0.0;
  clock_t start, diff;

  if(check_valid_scale_factor_cv(
                                 KERNEL_reg_extern,
                                 KERNEL_reg_unordered_extern,
                                 BANDWIDTH_reg_extern,
                                 BANDWIDTH_reg_extern,
                                 0,
                                 num_obs_train_extern,
                                 0,
                                 0,
                                 0,
                                 num_reg_continuous_extern,
                                 num_reg_unordered_extern,
                                 num_reg_ordered_extern,
                                 num_categories_extern,
                                 vector_scale_factor) == 1)
    {
      //Rprintf("toasty!\n");
      //for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern; ii++)
      //Rprintf("%3.15g ", vector_scale_factor[ii]);
      //Rprintf("\n");

      return(DBL_MAX);
    }
    start = clock();

    cv = (np_kernel_estimate_regression_categorical_ls_aic(
                                                            int_ll_extern,
                                                            RBWM_CVLS,
                                                            KERNEL_reg_extern,
                                                            KERNEL_reg_unordered_extern,
                                                            KERNEL_reg_ordered_extern,
                                                            BANDWIDTH_reg_extern,
                                                            num_obs_train_extern,
                                                            num_reg_unordered_extern,
                                                            num_reg_ordered_extern,
                                                            num_reg_continuous_extern,
                                                            matrix_X_unordered_train_extern,
                                                            matrix_X_ordered_train_extern,
                                                            matrix_X_continuous_train_extern,
                                                            vector_Y_extern,
                                                            &vector_scale_factor[1],
                                                            num_categories_extern));
    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);

    return(cv);

}

double cv_func_regression_categorical_ls_nn(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for least squares regression
                    cross-validation */

/* Declarations */

    double cv = 0.0;

    int i;

    double *mean;

    double *py;
    double *pm;

#ifdef MPI2
    int stride;
#endif

/* Allocate memory for objects */

#ifndef MPI2
    mean = alloc_vecd(num_obs_train_extern);
#endif

#ifdef MPI2

    stride = (int)ceil((double) num_obs_train_extern / (double) iNum_Processors);
    if(stride < 1) stride = 1;
    mean = alloc_vecd(stride*iNum_Processors);
#endif

/* Compute the cross-validation function */

    if(kernel_estimate_regression_categorical_leave_one_out(
        int_ll_extern,
        KERNEL_reg_extern,
        KERNEL_reg_unordered_extern,
        KERNEL_reg_ordered_extern,
        BANDWIDTH_reg_extern,
        num_obs_train_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        vector_Y_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        mean)==1)
    {
        free(mean);
        return(DBL_MAX);
    }

    py = &vector_Y_extern[0];
    pm = &mean[0];

    for(i=0;i<num_obs_train_extern;i++)
    {
        cv += ipow((*py++ - *pm++),2);
    }

    cv /= (double) num_obs_train_extern;


    free(mean);

    return(cv);

}



double cv_func_density_categorical_ml(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        0,
        0,
        0,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1)
    {
        return(DBL_MAX);
    }

/* Compute the cross-validation function */

    if(kernel_estimate_density_categorical_leave_one_out_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        &cv)==1)
    {
        return(DBL_MAX);
    }
    

    return(cv);

}

double np_cv_func_density_categorical_ml(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;
    clock_t start, diff;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        0,
        0,
        0,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1)
    {
        return(DBL_MAX);
    }

/* Compute the cross-validation function */
    start = clock();
    
    if(np_kernel_estimate_density_categorical_leave_one_out_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        &cv)==1)
    {
        return(DBL_MAX);
    }

    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);

    return(cv);

}

double cv_func_con_distribution_categorical_ls(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;
    clock_t start, diff;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_reg_unordered_extern, /* Only for conditioning vars in conditional den */
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        num_var_continuous_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) {

      //                        Rprintf("toasty!\n");
      //                  for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
      //                    Rprintf("%3.15g ", vector_scale_factor[ii]);
      //                  Rprintf("\n");

      return(DBL_MAX);
    }

/* Compute the cross-validation function */
    start = clock();

    if(np_kernel_estimate_con_distribution_categorical_leave_one_out_ls_cv(KERNEL_den_extern,
                                                                           KERNEL_den_unordered_extern,
                                                                           KERNEL_den_ordered_extern,
                                                                           KERNEL_reg_extern,
                                                                           KERNEL_reg_unordered_extern,
                                                                           KERNEL_reg_ordered_extern,
                                                                           BANDWIDTH_den_extern,
                                                                           num_obs_train_extern,
                                                                           num_obs_eval_extern,
                                                                           num_var_unordered_extern,
                                                                           num_var_ordered_extern,
                                                                           num_var_continuous_extern,
                                                                           num_reg_unordered_extern,
                                                                           num_reg_ordered_extern,
                                                                           num_reg_continuous_extern,
                                                                           dbl_memfac_ccdf_extern,
                                                                           matrix_Y_unordered_train_extern,
                                                                           matrix_Y_ordered_train_extern,
                                                                           matrix_Y_continuous_train_extern,
                                                                           matrix_X_unordered_train_extern,
                                                                           matrix_X_ordered_train_extern,
                                                                           matrix_X_continuous_train_extern,
                                                                           matrix_XY_unordered_train_extern, 
                                                                           matrix_XY_ordered_train_extern, 
                                                                           matrix_XY_continuous_train_extern,
                                                                           matrix_Y_unordered_eval_extern,
                                                                           matrix_Y_ordered_eval_extern,
                                                                           matrix_Y_continuous_eval_extern,
                                                                           &vector_scale_factor[1],
                                                                           num_categories_extern,
                                                                           matrix_categorical_vals_extern,
                                                                           &cv)==1)
      {
        //                        Rprintf("toaster!\n");
        //                        for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
        //                          Rprintf("%3.15g ", vector_scale_factor[ii]);
        //                        Rprintf("\n");

        return(DBL_MAX);
      }
    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);


    //        for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //          Rprintf("%3.15g ", vector_scale_factor[ii]);
    //                Rprintf("%3.15g ", cv);
    //                Rprintf("\n");

    return(cv);

}

double cv_func_con_density_categorical_ml(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_reg_unordered_extern, /* Only for conditioning vars in conditional den */
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        num_var_continuous_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) return(DBL_MAX);

/* Compute the cross-validation function */

    if(kernel_estimate_con_density_categorical_leave_one_out_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
				KERNEL_reg_extern,
        KERNEL_reg_unordered_extern,
        KERNEL_reg_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_var_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_Y_unordered_train_extern,
        matrix_Y_ordered_train_extern,
        matrix_Y_continuous_train_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        &cv)==1)
    {
        return(DBL_MAX);
    }


    return(cv);

}

double np_cv_func_con_density_categorical_ml(double *vector_scale_factor){

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;
    clock_t start, diff;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_reg_unordered_extern, /* Only for conditioning vars in conditional den */
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        num_var_continuous_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) {

      //                  Rprintf("toasty!\n");
      //            for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
      //              Rprintf("%3.15g ", vector_scale_factor[ii]);
      //            Rprintf("\n");

      return(DBL_MAX);
    }
/* Compute the cross-validation function */
    start = clock();

    if(np_kernel_estimate_con_density_categorical_leave_one_out_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
				KERNEL_reg_extern,
        KERNEL_reg_unordered_extern,
        KERNEL_reg_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_var_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_Y_unordered_train_extern,
        matrix_Y_ordered_train_extern,
        matrix_Y_continuous_train_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        matrix_XY_unordered_train_extern,
        matrix_XY_ordered_train_extern,
        matrix_XY_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        &cv)==1)
    {
      //                  Rprintf("toaster!\n");
      //                  for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
      //                    Rprintf("%3.15g ", vector_scale_factor[ii]);
      //                  Rprintf("\n");

        return(DBL_MAX);
    }
    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);


    //    for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //      Rprintf("%3.15g ", vector_scale_factor[ii]);
    //            Rprintf("%3.15g ", cv);
    //            Rprintf("\n");


    return(cv);

}

double np_cv_func_con_density_categorical_ls(double *vector_scale_factor){

/* Numerical recipes wrapper function for least squares conditional density
                    cross-validation */

/* Declarations */

  double cv = 0.0;

  if(check_valid_scale_factor_cv(KERNEL_den_extern,
                                 KERNEL_reg_unordered_extern,  /* Only for conditioning vars in conditional den */
                                 BANDWIDTH_den_extern,
                                 BANDWIDTH_den_extern,
                                 0,
                                 num_obs_train_extern,
                                 num_var_continuous_extern,
                                 num_var_unordered_extern,
                                 num_var_ordered_extern,
                                 num_reg_continuous_extern,
                                 num_reg_unordered_extern,
                                 num_reg_ordered_extern,
                                 num_categories_extern,
                                 vector_scale_factor) == 1) {
    //        Rprintf("toasty\n");
    //        for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //          Rprintf("%3.15g ", vector_scale_factor[ii]);
    //        Rprintf("\n");

    return(DBL_MAX);
  }

  /* Compute the cross-validation function */

  if(np_kernel_estimate_con_density_categorical_convolution_cv(KERNEL_den_extern,
                                                               KERNEL_den_unordered_extern,
                                                               KERNEL_den_ordered_extern,
                                                               KERNEL_reg_extern,
                                                               KERNEL_reg_unordered_extern,
                                                               KERNEL_reg_ordered_extern,
                                                               BANDWIDTH_den_extern,
                                                               num_obs_train_extern,
                                                               num_var_unordered_extern,
                                                               num_var_ordered_extern,
                                                               num_var_continuous_extern,
                                                               num_reg_unordered_extern,
                                                               num_reg_ordered_extern,
                                                               num_reg_continuous_extern,
                                                               matrix_Y_unordered_train_extern,
                                                               matrix_Y_ordered_train_extern,
                                                               matrix_Y_continuous_train_extern,
                                                               matrix_X_unordered_train_extern,
                                                               matrix_X_ordered_train_extern,
                                                               matrix_X_continuous_train_extern,
                                                               &vector_scale_factor[1],
                                                               num_categories_extern,
                                                               matrix_categorical_vals_extern,
                                                               &cv)==1) {
    //                Rprintf("toaster!!\n");
    //                for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //                  Rprintf("%3.15g ", vector_scale_factor[ii]);
    //                Rprintf("\n");

    return(DBL_MAX);
  }

  //        for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
  //          Rprintf("%3.15g ", vector_scale_factor[ii]);
  //          Rprintf("%3.15g ", cv);
  //        Rprintf("\n");

  return(cv);

}

double np_cv_func_con_density_categorical_ls_npksum(double *vector_scale_factor){

/* Numerical recipes wrapper function for least squares conditional density
                    cross-validation */

/* Declarations */

  double cv = 0.0;
  clock_t start, diff;

  if(check_valid_scale_factor_cv(KERNEL_den_extern,
                                 KERNEL_reg_unordered_extern,  /* Only for conditioning vars in conditional den */
                                 BANDWIDTH_den_extern,
                                 BANDWIDTH_den_extern,
                                 0,
                                 num_obs_train_extern,
                                 num_var_continuous_extern,
                                 num_var_unordered_extern,
                                 num_var_ordered_extern,
                                 num_reg_continuous_extern,
                                 num_reg_unordered_extern,
                                 num_reg_ordered_extern,
                                 num_categories_extern,
                                 vector_scale_factor) == 1) {
    //        Rprintf("toasty\n");
    //        for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //          Rprintf("%3.15g ", vector_scale_factor[ii]);
    //        Rprintf("\n");

    return(DBL_MAX);
  }
  /* Compute the cross-validation function */
    start = clock();
    if(np_kernel_estimate_con_density_categorical_leave_one_out_ls_cv(KERNEL_den_extern,
                                                                      KERNEL_den_unordered_extern,
                                                                      KERNEL_den_ordered_extern,
                                                                      KERNEL_reg_extern,
                                                                      KERNEL_reg_unordered_extern,
                                                                      KERNEL_reg_ordered_extern,
                                                                      BANDWIDTH_den_extern,
                                                                      num_obs_train_extern,
                                                                      num_var_unordered_extern,
                                                                      num_var_ordered_extern,
                                                                      num_var_continuous_extern,
                                                                      num_reg_unordered_extern,
                                                                      num_reg_ordered_extern,
                                                                      num_reg_continuous_extern,
                                                                      dbl_memfac_ccdf_extern,
                                                                      matrix_Y_unordered_train_extern,
                                                                      matrix_Y_ordered_train_extern,
                                                                      matrix_Y_continuous_train_extern,
                                                                      matrix_X_unordered_train_extern,
                                                                      matrix_X_ordered_train_extern,
                                                                      matrix_X_continuous_train_extern,
                                                                      matrix_XY_unordered_train_extern, 
                                                                      matrix_XY_ordered_train_extern, 
                                                                      matrix_XY_continuous_train_extern,
                                                                      &vector_scale_factor[1],
                                                                      num_categories_extern,
                                                                      matrix_categorical_vals_extern,
                                                                      &cv)==1)
      {
        //                Rprintf("toaster!!\n");
        //                for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
        //                  Rprintf("%3.15g ", vector_scale_factor[ii]);
        //                Rprintf("\n");

        return(DBL_MAX);
      }
    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);

    //        for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //          Rprintf("%3.15g ", vector_scale_factor[ii]);
    //          Rprintf("%3.15g ", cv);
    //        Rprintf("\n");

  return(cv);

}

double cv_func_con_density_categorical_ls(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for least squares conditional density
                    cross-validation */

/* Declarations */

    double cv = 0.0;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_reg_unordered_extern, /* Only for conditioning vars in conditional den */
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        num_var_continuous_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1){

      //      Rprintf("toasty!!\n");
      //      for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
      //        Rprintf("%3.15g ", vector_scale_factor[ii]);
      //      Rprintf("\n");

      return(DBL_MAX);
    }
/* Compute the cross-validation function */

    if(kernel_estimate_con_density_categorical_convolution_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
				KERNEL_reg_extern,
        KERNEL_reg_unordered_extern,
        KERNEL_reg_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_var_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_Y_unordered_train_extern,
        matrix_Y_ordered_train_extern,
        matrix_Y_continuous_train_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        matrix_categorical_vals_extern,
        &cv)==1)
    {
      //      Rprintf("toaster!!\n");
      //      for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
      //        Rprintf("%3.15g ", vector_scale_factor[ii]);
      //      Rprintf("\n");

      return(DBL_MAX);
    }

    //    for(int ii = 1; ii <= num_reg_continuous_extern + num_reg_unordered_extern + num_reg_ordered_extern + num_var_continuous_extern + num_var_unordered_extern + num_var_ordered_extern; ii++)
    //      Rprintf("%3.15g ", vector_scale_factor[ii]);
    //      Rprintf("%3.15g ", cv);
    //    Rprintf("\n");

    return(cv);

}

/* Feb 7 2010 */

double cv_func_con_distribution_categorical_ccdf(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for conditional distribution
function */

/* Declarations */

    double cv = 0.0;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_reg_unordered_extern, /* Only for conditioning vars in conditional den */
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        num_var_continuous_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) return(DBL_MAX);

    if(kernel_estimate_con_distribution_categorical_leave_one_out_ccdf(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
				KERNEL_reg_extern,
        KERNEL_reg_unordered_extern,
        KERNEL_reg_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_var_unordered_extern,
        num_var_ordered_extern,
        num_var_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_Y_unordered_train_extern,
        matrix_Y_ordered_train_extern,
        matrix_Y_continuous_train_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        matrix_categorical_vals_extern,
        &cv,
        small_extern,
        itmax_extern)==1)
    {
        return(DBL_MAX);
    }


    return(cv);

}

double cv_func_density_categorical_ls(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        0,
        0,
        0,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) return(DBL_MAX);

/* Compute the cross-validation function */

    if(kernel_estimate_density_categorical_convolution_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        matrix_categorical_vals_extern,
        &cv)==1)
    {
        return(DBL_MAX);
    }


    return(cv);

}

double np_cv_func_density_categorical_ls(double *vector_scale_factor){

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;
    clock_t start, diff;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        0,
        0,
        0,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) return(DBL_MAX);

/* Compute the cross-validation function */
    start = clock();

    if(np_kernel_estimate_density_categorical_convolution_cv(KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        &vector_scale_factor[1],
        num_categories_extern,
        matrix_categorical_vals_extern,
        &cv)==1)
    {
        return(DBL_MAX);
    }

    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);

    return(cv);

}

double cv_func_distribution_categorical_ls(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for likelihood density
                    cross-validation */

/* Declarations */

    double cv = 0.0;
    clock_t start, diff;

    if(check_valid_scale_factor_cv(
        KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        BANDWIDTH_den_extern,
        BANDWIDTH_den_extern,
        0,
        num_obs_train_extern,
        0,
        0,
        0,
        num_reg_continuous_extern,
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_categories_extern,
        vector_scale_factor) == 1) return(DBL_MAX);

/* Compute the cross-validation function */
    start = clock();
    if(np_kernel_estimate_distribution_ls_cv(KERNEL_den_extern,
                                             KERNEL_den_unordered_extern,
                                             KERNEL_den_ordered_extern,
                                             BANDWIDTH_den_extern,
                                             num_obs_train_extern,
                                             num_obs_eval_extern,
                                             num_reg_unordered_extern,
                                             num_reg_ordered_extern,
                                             num_reg_continuous_extern,
                                             dbl_memfac_dls_extern,
                                             matrix_X_unordered_train_extern,
                                             matrix_X_ordered_train_extern,
                                             matrix_X_continuous_train_extern,
                                             matrix_X_unordered_eval_extern,
                                             matrix_X_ordered_eval_extern,
                                             matrix_X_continuous_eval_extern,
                                             &vector_scale_factor[1],
                                             num_categories_extern,
                                             matrix_categorical_vals_extern,
                                             &cv)==1)
    {
        return(DBL_MAX);
    }

    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);

    return(cv);

}


double func_con_density_quantile(double *quantile)
{

/* Declarations */

    double func = 0.0;
    double cdf[1];
    double cdf_stderr[1];

    if((quantile[1] < y_min_extern)||(quantile[1] > y_max_extern))
    {
        return(DBL_MAX);
    }

    matrix_Y_continuous_quantile_extern[0][0]=quantile[1];

/* Compute the conditional density at y = quantile */

/* Can we disable MPI temporarily if it is on? */

    kernel_estimate_con_distribution_categorical_no_mpi(
        KERNEL_den_extern,
        KERNEL_den_unordered_extern,
        KERNEL_den_ordered_extern,
				KERNEL_reg_extern,
        KERNEL_reg_unordered_extern,
        KERNEL_reg_ordered_extern,
        BANDWIDTH_den_extern,
        num_obs_train_extern,
        1,                                        /* One evaluation observation */
        0,                                        /* Zero discrete Y */
        0,                                        /* Zero discrete Y */
        1,                                        /* One continuous Y */
        num_reg_unordered_extern,
        num_reg_ordered_extern,
        num_reg_continuous_extern,
        matrix_Y_unordered_train_extern,
        matrix_Y_ordered_train_extern,
        matrix_Y_continuous_train_extern,
        matrix_Y_unordered_quantile_extern, /* Not used */
        matrix_Y_ordered_quantile_extern,   /* Not used */
        matrix_Y_continuous_quantile_extern,
        matrix_X_unordered_train_extern,
        matrix_X_ordered_train_extern,
        matrix_X_continuous_train_extern,
        matrix_X_unordered_quantile_extern,
        matrix_X_ordered_quantile_extern,
        matrix_X_continuous_quantile_extern,
        &vector_scale_factor_extern[1],
        num_categories_extern,
        matrix_categorical_vals_extern,
        cdf,
        cdf_stderr,
        small_extern,
        itmax_extern);

    func = ipow(gamma_extern - cdf[0], 2);


    return(func);

}


double cv_func_regression_categorical_aic_c(double *vector_scale_factor)
{

/* Numerical recipes wrapper function for Hurvich/Simonoff/Tsai JRSS B 1998 */

/* Declarations */
  double cv = 0.0;
  clock_t start, diff;

  if(check_valid_scale_factor_cv(
                                 KERNEL_reg_extern,
                                 KERNEL_reg_unordered_extern,
                                 BANDWIDTH_reg_extern,
                                 BANDWIDTH_reg_extern,
                                 0,
                                 num_obs_train_extern,
                                 0,
                                 0,
                                 0,
                                 num_reg_continuous_extern,
                                 num_reg_unordered_extern,
                                 num_reg_ordered_extern,
                                 num_categories_extern,
                                 vector_scale_factor) == 1)
    {
      return(DBL_MAX);
    }

    start = clock();

    cv = (np_kernel_estimate_regression_categorical_ls_aic(int_ll_extern,
                                                            RBWM_CVAIC,
                                                            KERNEL_reg_extern,
                                                            KERNEL_reg_unordered_extern,
                                                            KERNEL_reg_ordered_extern,
                                                            BANDWIDTH_reg_extern,
                                                            num_obs_train_extern,
                                                            num_reg_unordered_extern,
                                                            num_reg_ordered_extern,
                                                            num_reg_continuous_extern,
                                                            matrix_X_unordered_train_extern,
                                                            matrix_X_ordered_train_extern,
                                                            matrix_X_continuous_train_extern,
                                                            vector_Y_extern,
                                                            &vector_scale_factor[1],
                                                            num_categories_extern));
    diff = clock() - start;
    timing_extern = ((double)diff)/((double)CLOCKS_PER_SEC);

    return(cv);
}
