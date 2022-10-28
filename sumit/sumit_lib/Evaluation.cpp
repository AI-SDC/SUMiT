#include "stdafx.h"
#include "Evaluation.h"
#include "GAProtection.h"

Evaluation::Evaluation(const CellIndex *genes, int genome_size, int model_type) {
    // Chromosome must be self-contained, so copy
    this->genes = new CellIndex[genome_size];

    for (int i = 0; i < genome_size; i++) {
        this->genes[i] = genes[i];
    }

    this->genome_size = genome_size;
    this->model_type = model_type;
}

Evaluation::Evaluation(const Evaluation& orig) {
    // Chromosome must be self-contained, so copy
    genes = new CellIndex[orig.genome_size];

    for (int i = 0; i < orig.genome_size; i++) {
        genes[i] = orig.genes[i];
    }

    genome_size = orig.genome_size;
    model_type = orig.model_type;
}

Evaluation::~Evaluation() {
    delete[] genes;
}

bool Evaluation::equals(const Evaluation* eval) {
    if (eval->model_type != model_type) {
        return false;
    }

    // Code assumes that genomes are the same size
    for (int i = 0; i < genome_size; i++) {
        if (eval->genes[i] != genes[i]) {
            return false;
        }
    }

    return true;
}

void Evaluation::log_genome() {
    char s[32];
    int length = 0;
    for (int i = 0; i < genome_size; i++) {
        length += sprintf(s, " %d", genes[i]);
    }

    char *str = new char[length + 1];
    char *p = str;
    for (int i = 0; i < genome_size; i++) {
        p += sprintf(p, " %d", genes[i]);
    }

    logger->log(4, "Genome: %s", str);

    delete[] str;
}
