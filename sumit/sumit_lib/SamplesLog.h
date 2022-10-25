#pragma once

#include "stdafx.h"
#include "Individual.h"

class SamplesLog {
    
public:
    SamplesLog(const char *injjfilename, const char *samples_filename, int chromosome_size);
    ~SamplesLog();
    void log_sample(struct Individual *sample, double fitness, int protection_type, int model_type, char *elimination_filename, int elapsed_time);
    
private:
    FILE *samplesfp;
    int chromosome_size;

};
