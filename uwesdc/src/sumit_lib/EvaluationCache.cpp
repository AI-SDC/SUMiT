#include "stdafx.h"
#include <cstring>
#include "EvaluationCache.h"
#include "GAProtection.h"

EvaluationCache::EvaluationCache(int genome_size) {
    this->genome_size = genome_size;
    
    for (int i = 0; i < NUMBER_OF_MODELS; i++) {
        requests[i] = 0;
        hits[i] = 0;
        misses[i] = 0;
    }
}

EvaluationCache::~EvaluationCache() {
#if _MSC_VER==1600
    // Visual Studio 2010 doesn't fully support C++11
    for(std::map<Evaluation, struct Result, cmp>::iterator it=map.begin(); it != map.end(); ++it) {
        remove(it->second);
    }
#else
    for (auto const &key : map) {
        remove(key.second);
    }
#endif
    
    map.clear();
}

void EvaluationCache::remove(struct Result result) {
    if (result.jj_file != NULL) {
        sys.remove_file(result.jj_file);
        delete[] result.jj_file;
        result.jj_file = NULL;
    }
    if (result.costs != NULL) {
        delete[] result.costs;
        result.costs = NULL;
    }
    
    result.number_of_costs = 0;
    result.fitness = 0.0;
}

bool EvaluationCache::cached(const CellIndex *genes, int model_type) {
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    bool cached = (map.count(eval) > 0);
    
    requests[model_type]++;

    if (cached) {
        hits[model_type]++;
    } else {
        misses[model_type]++;
    }
    
    return cached;
}

char *EvaluationCache::jjFile(const CellIndex *genes, int model_type) {
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    std::map<Evaluation, struct Result, cmp>::iterator it = map.find(eval);
    
    if (it != map.end()) {
        return it->second.jj_file;
    } else {
        return NULL;
    }
}

int EvaluationCache::costs(const CellIndex *genes, int model_type, double **costs) {
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    std::map<Evaluation, struct Result, cmp>::iterator it = map.find(eval);
    
    if (it != map.end()) {
        for (int i = 0; i < it->second.number_of_costs; i++) {
            (*costs)[i] = it->second.costs[i];
        }

        return it->second.number_of_costs;
    } else {
        return 0;
    }
}

double EvaluationCache::fitness(const CellIndex *genes, int model_type) {
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    std::map<Evaluation, struct Result, cmp>::iterator it = map.find(eval);
    
    if (it != map.end()) {
        return it->second.fitness;
    } else {
        return 0.0;
    }
}

void EvaluationCache::add(const CellIndex *genes, int model_type, char *jj_file, double *costs, double fitness, int number_of_costs) {
    struct Result result;
    
    // Remove any previous result
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    std::map<Evaluation, struct Result, cmp>::iterator it = map.find(eval);
    
    if (it != map.end()) {
        remove(it->second);
    }

    // Filename and costs must be self-contained, so copy
    if (jj_file != NULL) {
        result.jj_file = new char[strlen(jj_file) + 1];
        char *p = jj_file;
        char *q = result.jj_file;
        while (*p) {
            *q++ = *p++;
        }
        *q = *p;
    } else {
        result.jj_file = NULL;
    }
    
    if (costs != NULL) {
        result.number_of_costs = number_of_costs;
        result.costs = new double[number_of_costs];
        for (int i = 0; i < number_of_costs; i++) {
            result.costs[i] = costs[i];
        }
    } else {
        result.number_of_costs = 0;
        result.costs = NULL;
    }
    
    result.fitness = fitness;
    
    map[Evaluation(genes, genome_size, model_type)] = result;
}

void EvaluationCache::log_genome(const CellIndex *genes, int model_type) {
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    
    if (map.count(eval) > 0) {
        eval.log_genome();
    }
}

void EvaluationCache::log_costs(const CellIndex *genes, int model_type) {
    Evaluation eval = Evaluation(genes, genome_size, model_type);
    std::map<Evaluation, struct Result, cmp>::iterator it = map.find(eval);
    
    if (it != map.end()) {
        char s[32];
        int length = 0;
        for (int i = 0; i < it->second.number_of_costs; i++) {
            length += sprintf(s, " %f", it->second.costs[i]);
        }

        char *str = new char[length + 1];
        char *p = str;
        for (int i = 0; i < it->second.number_of_costs; i++) {
            p += sprintf(p, " %f", it->second.costs[i]);
        }

        logger->log(4, "Costs: %s", str);

        delete[] str;
    }
}
