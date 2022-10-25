#pragma once

#include "stdafx.h"
#include <time.h>
#include "JJData.h"
#include "MersenneTwister.h"
#include "Individual.h"
#include "SamplesLog.h"
#include "EvaluationCache.h"

#define SELECTION_TRUNCATION 0
#define SELECTION_TOURNAMENT 1
#define SELECTION_FITNESS_PROPORTIONATE 2

#define CROSSOVER_PARTIALLY_MAPPED 0
#define CROSSOVER_EDGE 1
#define CROSSOVER_ORDER 2
#define CROSSOVER_CYCLE 3
#define CROSSOVER_DISTANCE_PRESERVING 4

#define MUTATION_SWAP 0
#define MUTATION_INSERT 1
#define MUTATION_INVERSION 2
#define MUTATION_SCRAMBLE 3
#define MUTATION_ASSORTED 4

#define REPLACE_OLDEST 0
#define REPLACE_WORSE 1
#define REPLACE_TOURNAMENT 2
#define REPLACE_WORSE_BY_TOURNAMENT 3

#define POOL_PARENT_SIZE 20
#define POOL_MATING_SIZE 2
#define POOL_OFFSPRING_SIZE 1
#define MAX_RANDOMISATION_ATTEMPTS 10000
#define NUMBER_OF_GENES 10000
#define RECOMBINATION_PROBABILITY 0.7

#define MAX_EVALUATIONS 1000
#define STABLE_FOR_X_GENERATIONS 1000

typedef int GeneIndex;

class GAProtection {

public:
    GAProtection(const char *host, const char *port, const char *injjfilename, const char *outjjfilename, const char *samples_filename, unsigned int seed, int cores, int execution_time, bool run_elimination);
    virtual ~GAProtection(void);
    bool time_to_terminate();
    int number_of_evaluations();

    virtual void protect(bool limit_cost) = 0;
    virtual double fitness() = 0;

protected:
    MTRand random;

    char injjfilename[MAX_FILENAME_SIZE];
    JJData *jjData;
    CellIndex number_of_genes;
    CellIndex max_allele;
    SamplesLog *samples_log;
    struct Individual *pool_parent;
    struct Individual *pool_clones;

    int pool_parent_size;
    int pool_clones_size;
    int default_number_of_clones;

    void allocate_pools();
    void select_for_pool_mating();
    void apply_crossover();
    void duplicate_clones(int offspring);
    void randomise_clone(int offspring);
    void apply_mutation();
    void replacement(int pool_size);
    int solvers_required(int number_to_evaluate, struct Individual pool[], int model_type);
    void fill_parent_pool(int model_type);
    int grow_clones_pool(int model_type);
    void evaluate_fitness(int number_to_evaluate, struct Individual pool[], int protection_type, int model_type, bool get_outputjjfile, bool count_evals, double max_cost);
    double evaluate_best_parent(int protection_type);
    double get_worst_fitness();

private:
    char outjjfilename[MAX_FILENAME_SIZE];

    char host[MAX_HOST_NAME_SIZE];
    char port[MAX_PORT_NUMBER_SIZE];
    int cores;

    bool run_elimination;

    time_t start_seconds;

    struct Individual *pool_mating;
    struct Individual *pool_offspring;

    int algorithm_for_selection;
    int algorithm_for_crossover;
    int algorithm_for_mutation;
    int algorithm_for_replacement;

    int number_of_offspring;
    int offspring1;
    int offspring2;

    int mutation_type;
    int replace_next;

    int max_evaluations;
    int number_of_evals; // Total number of usages of the solver
    int number_of_counted_evals; // Number of evaluations not including evaluation of best parent
    double stable_fitness;
    int stable_generations;
    int max_seconds;
    bool terminated;

    double best_fitness;

    EvaluationCache *evaluationCache;

    bool invalid_offspring(int offspring);
    void sort_pool_by_fitness(int number_to_sort, struct Individual Pool[]);
    void copy_individual(struct Individual* to, struct Individual* from);
    double get_best_fitness();
    int get_best_parent();
    void selection_truncation();
    void selection_tournament();
    void selection_proportionate();
    void print_crossover_details(int parent1, int parent2, int offspring);
    CellIndex find_gene_location(int parent, int gene);
    void map_gene(int parent1, int g1, int parent2, int g2);
    void crossover_partially_mapped(int parent1, int parent2);
    void crossover_distance_preserving(int parent1, int parent2);
    void crossover_edge(int parent1, int parent2);
    bool not_in_genes(CellIndex g, CellIndex *genes);
    void crossover_order(int parent1, int parent2);
    void crossover_cycle(int parent1, int parent2);
    bool valid_mating(int parent);
    void mutation_swap(int offspring, double mutation_rate);
    void mutation_insert(int offspring, double mutation_rate);
    void mutation_scramble(int offspring, double mutation_rate);
    void mutation_inversion(int offspring, double mutation_rate);
    int get_fittest_clone(int pool_size);
    void replace_nothing(int pool_size);
    void replace_oldest(int pool_size);
    void replace_worst(int pool_size);
    void replace_tournament(int pool_size);
    void replace_worst_by_tournament(int pool_size);
    void increase_polling_delay(int *delay);
    void write_perm_file(const char* filename, int* perm, int size);
    int read_cost_file(const char* filename, double* costs);

};
