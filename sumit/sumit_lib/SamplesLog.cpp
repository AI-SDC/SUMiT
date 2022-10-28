#include "stdafx.h"
#include <float.h>
#include "SamplesLog.h"
#include "GAProtection.h"
#include "Eliminate.h"

SamplesLog::SamplesLog(const char *injjfilename, const char *samples_filename, int chromosome_size) {
    this->chromosome_size = chromosome_size;

    samplesfp = fopen(samples_filename, "w");
    if (samplesfp != NULL) {
        fprintf(samplesfp, "%d %s\n", chromosome_size, injjfilename);
        fflush(samplesfp);
    }
}

SamplesLog::~SamplesLog() {
    if (samplesfp != NULL) {
        fclose(samplesfp);
    }
}

void SamplesLog::log_sample(struct Individual *sample, double fitness, int protection_type, int model_type, char *elimination_filename, int elapsed_time) {
    if (samplesfp != NULL) {
        bool run_elimination = (elimination_filename != NULL);
        fprintf(samplesfp, "%d %d %d ", protection_type, model_type, run_elimination? 1: 0);
        for (int i = 0; i < chromosome_size; i++) {
            fprintf(samplesfp, "%d ", sample->genes[i]);
        }

        double fitnessAfterElimination = DBL_MIN;
        if (run_elimination) {
            Eliminate *eliminate = new Eliminate(elimination_filename);
            eliminate->RemoveExcessSuppression();
            fitnessAfterElimination = eliminate->getCost();
            delete eliminate;
        } else {
            fitnessAfterElimination = fitness;
        }

        fprintf(samplesfp, "%f %f %d\n", fitness, fitnessAfterElimination, elapsed_time);
        fflush(samplesfp);
    }
}
