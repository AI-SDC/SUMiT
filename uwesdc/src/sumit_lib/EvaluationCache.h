#pragma once

#include "stdafx.h"
#include <map>
#include "Evaluation.h"
#include "Solver.h"
#if defined __APPLE__
#include <errno.h>
#endif


class EvaluationCache {
public:
    EvaluationCache(int chromosome_size);
    ~EvaluationCache();
    
    bool cached(const CellIndex *genes, int model_type);
    char *jjFile(const CellIndex *genes, int model_type);
    int costs(const CellIndex *genes, int model_type, double **costs);
    double fitness(const CellIndex *genes, int model_type);
    void add(const CellIndex *genes, int model_type, char *jj_file, double *costs, double fitness, int number_of_costs);
    void log_genome(const CellIndex *genes, int model_type);
    void log_costs(const CellIndex *genes, int model_type);

    int requests[NUMBER_OF_MODELS];
    int hits[NUMBER_OF_MODELS];
    int misses[NUMBER_OF_MODELS];

private:
    
    struct Result {
        char *jj_file;
        int number_of_costs;
        double *costs;
        double fitness;
    };

    struct cmp {
        // Compare two evaluations based on model and genome
        bool operator()(const Evaluation& eval1, const Evaluation& eval2) const {
            if (eval1.model_type < eval2.model_type) {
                return true;
            } else if (eval1.model_type > eval2.model_type) {
                return false;
            }
            
            // Code assumes that genomes of eval1 and eval2 are the same size
            for (CellIndex i = 0; i < eval1.genome_size; i++) {
                if (eval1.genes[i] < eval2.genes[i]) {
                    return true;
                } else if (eval1.genes[i] > eval2.genes[i]) {
                    return false;
                }
            }

            return false;
        }
    };

    int genome_size;
    std::map<Evaluation, struct Result, cmp> map;
    
    void remove(struct Result result);
    
};
