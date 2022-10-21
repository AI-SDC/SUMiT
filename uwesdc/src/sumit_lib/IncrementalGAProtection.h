#pragma once

#include "stdafx.h"
#include "GAProtection.h"

class IncrementalGAProtection: public GAProtection {

public:
    IncrementalGAProtection(const char *host, const char *port, const char *injjfilename, const char *outjjfilename, const char *samplesFilename, unsigned int seed, int cores, int execution_time, bool runElimination);
    void protect(bool limit_cost);
    double fitness();
    
};
