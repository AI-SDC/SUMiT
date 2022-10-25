#pragma once

#include "stdafx.h"
#include "JJData.h"

class Evaluation {

public:
    Evaluation(const CellIndex *genes, int chromosome_size, int model_type);
    Evaluation(const Evaluation& orig);
    ~Evaluation();
    bool equals(const Evaluation* eval);
    void log_genome();
    CellIndex *genes;
    int genome_size;
    int model_type;

};
