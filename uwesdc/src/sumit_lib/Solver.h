#pragma once

#include "stdafx.h"
#include "ServerConnection.h"
#include "JJData.h"

#define INDIVIDUAL_PROTECTION 0
#define GROUP_PROTECTION 1

// This is used as a zero-based index
#define NUMBER_OF_MODELS 3
//#define FULL_MODEL	0
#define YPLUS_MODEL	1
#define YMINUS_MODEL	2

#define SESSION_EXCEPTION 111

class Solver {
    
public:
    const char *host;
    const char *port;
    
    Solver(const char *host, const char *port);
    ~Solver();
    char *getSession();
    void runProtection(const char *injjfilename, const char *perm_filename, int protection_type, int model_type, double max_cost);
    int getCores();
    int getLimit();
    int getProtocol();
    int getStatus();
    double getResult();
    int getElapsedTime();
    void getCostFile(const char* filename);
    void getJJFile(const char* filename);

private:
    char session[64];
    
    void run_model(const char *injjfilename, const char *perm_filename, const char *protection_type, const char *model_type, double max_cost);
    
};
