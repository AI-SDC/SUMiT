#pragma once

#define CLP 1
#define GLPK 2
#define CPLEX 3

#define SOLVER CLP

#include <stdafx.h>
#include <stdlib.h>
#if SOLVER == CPLEX
#include <OsiCpxSolverInterface.hpp>
#elif SOLVER == GLPK
#include <OsiGlpkSolverInterface.hpp>
#else
#include <OsiClpSolverInterface.hpp>
#endif
#include <JJData.h>
#include <Groups.h>

#define INDIVIDUAL_PROTECTION 0
#define GROUP_PROTECTION 1

#define FULL_MODEL	0
#define YPLUS_MODEL	1
#define YMINUS_MODEL	2

class Solver {

public:
    Solver(const char* injjfilename, bool groupProtection);
    ~Solver();

    double* run_individual_protection(const char* perm_filename, int model_type, double max_cost, int* costs_size);
    double* run_group_protection(const char* perm_filename, int model_type, double max_cost, int* costs_size);
    int get_number_of_groups();
    void write_cost_file(const char* filename, double* costs, int size);
    void write_jj_file(const char* filename);
    //bool Solver::getCompletedSuccessfully();
    bool getCompletedSuccessfully();

private:
    JJData *jjData;
    Groups *groups;
    int number_of_groups;



    ///////////////////////////////////////////////////////////////////////////////////////
    ///////                             Time Data                                    //////
    ///////////////////////////////////////////////////////////////////////////////////////


    time_t start_seconds;
    time_t stop_seconds;
    time_t current_seconds;



    ///////////////////////////////////////////////////////////////////////////////////////
    ///////                         COIN-OR Data                                     //////
    ///////////////////////////////////////////////////////////////////////////////////////


#define COLUMN_ORDERED 1
#define ROW_ORDERED    0

    OsiSolverInterface *si;

    int *row_indices;
    int *col_indices;
    double *elements;

    CoinPackedMatrix *matrixA;

    double *varLB;
    double *varUB;

    double *objCoeffs;

    double *rowLB;
    double *rowUB;

    int YminusOffset;
    int YplusOffset;

    double get_cost();
    int* read_permutation_file(const char* filename);
    void allocate_coin_memory();
    void release_coin_memory();
    void logModel();

};
