#include "stdafx.h"
#include <string.h>
#include <float.h>
#include "GAProtection.h"
#include "Solver.h"
#include "Eliminate.h"
#include "EvaluationCache.h"
#include "Groups.h"

GAProtection::GAProtection(const char *host, const char *port, const char *injjfilename, const char *outjjfilename, const char *samples_filename, unsigned int seed, int cores, int execution_time, bool run_elimination) {
    time(&start_seconds);

    logger->log(2, "Protecting: %s", injjfilename);

    strcpy(this->injjfilename, injjfilename);
    strcpy(this->outjjfilename, outjjfilename);
    this->run_elimination = run_elimination;
    max_seconds = execution_time;

    logger->log(3, "%d second time limit", max_seconds);

    jjData = new JJData(injjfilename);

    if (jjData->get_number_of_primary_cells() != 0) {
        algorithm_for_selection = SELECTION_TOURNAMENT;
        algorithm_for_crossover = CROSSOVER_ORDER;
        algorithm_for_mutation = MUTATION_ASSORTED;
        algorithm_for_replacement = REPLACE_TOURNAMENT;
        mutation_type = MUTATION_SWAP;

        strcpy(this->host, host);
        strcpy(this->port, port);

        if (seed != 0) {
            random.seed(seed);
        }

        // Check for available server cores
        int available_cores = 0;

        try {
            Solver *solver = new Solver(host, port);
            if (solver->getProtocol() != 4) {
                // Terminate the remote solver before exiting
                delete solver;

                logger->error(1, "Unsupported version of client server protocol");
            }

            available_cores = solver->getLimit();

            delete solver;

            switch (available_cores) {
                case 0:
                    logger->error(1, "No available server cores (minimum allowable number of cores is two)");
                    break;

                case 1:
                    logger->error(1, "Only one available server core (minimum allowable number of cores is two)");
                    break;
            }
        } catch (int e) {
            // Keep the compiler from complaining
            e = 0;

            // Session not available
            // For now, just give up.  This situation should not occur with only one single-threaded client trying to access the server
            // If more than one client thread is required to access the server, then the situation will need handling properly (waiting)
            logger->error(1, "No available server sessions");
        }

        // Select the actual number of cores to use
        switch (cores) {
            case 0:
                // Automatic server selection of number of cores
                this->cores = available_cores;
                break;

            case 1:
                // Manual selection of number of cores
                logger->error(1, "Minimum allowable number of cores is two");
                break;

            default:
                // Manual selection of number of cores
                this->cores = MIN(cores, available_cores);
                break;
        }

        // Limit the number of cores to the pool size
        if (this->cores > POOL_PARENT_SIZE) {
            this->cores = POOL_PARENT_SIZE;
        }

        logger->log(3, "%d cores available", available_cores);
        logger->log(3, "Using %d cores", this->cores);
    } else {
        pool_parent_size = 0;
        default_number_of_clones = 0;
        pool_clones_size = 0;
    }

    replace_next = 0;

    number_of_evals = 0;
    number_of_counted_evals = 0;
    stable_fitness = DBL_MAX;
    stable_generations = 0;
    terminated = false;
}

GAProtection::~GAProtection(void) {
//    logger->log(4, "Evaluation cache full model requests: %d", evaluationCache->requests[FULL_MODEL]);
//    if (evaluationCache->requests[FULL_MODEL] > 0) {
//        logger->log(4, "Evaluation cache full model hits: %d (%.1f%%)", evaluationCache->hits[FULL_MODEL], (float)(evaluationCache->hits[FULL_MODEL] * 100) / (float)evaluationCache->requests[FULL_MODEL]);
//        logger->log(4, "Evaluation cache full model misses: %d (%.1f%%)", evaluationCache->misses[FULL_MODEL],  (float)(evaluationCache->misses[FULL_MODEL] * 100) / (float)evaluationCache->requests[FULL_MODEL]);
//    }

    logger->log(4, "Evaluation cache yplus model requests: %d", evaluationCache->requests[YPLUS_MODEL]);
    if (evaluationCache->requests[YPLUS_MODEL] > 0) {
        logger->log(4, "Evaluation cache yplus model hits: %d (%.1f%%)", evaluationCache->hits[YPLUS_MODEL], (float)(evaluationCache->hits[YPLUS_MODEL] * 100) / (float)evaluationCache->requests[YPLUS_MODEL]);
        logger->log(4, "Evaluation cache yplus model misses: %d (%.1f%%)", evaluationCache->misses[YPLUS_MODEL], (float)(evaluationCache->misses[YPLUS_MODEL] * 100) / (float)evaluationCache->requests[YPLUS_MODEL]);
    }

    logger->log(4, "Evaluation cache yminus model requests: %d", evaluationCache->requests[YMINUS_MODEL]);
    if (evaluationCache->requests[YMINUS_MODEL] > 0) {
        logger->log(4, "Evaluation cache yminus model hits: %d (%.1f%%)", evaluationCache->hits[YMINUS_MODEL], (float)(evaluationCache->hits[YMINUS_MODEL] * 100) / (float)evaluationCache->requests[YMINUS_MODEL]);
        logger->log(4, "Evaluation cache yminus model misses: %d (%.1f%%)", evaluationCache->misses[YMINUS_MODEL], (float)(evaluationCache->misses[YMINUS_MODEL] * 100) / (float)evaluationCache->requests[YMINUS_MODEL]);
    }

    delete evaluationCache;

    if (pool_parent != NULL) {
        for (int i = 0; i < POOL_PARENT_SIZE; i++) {
            delete[] pool_parent[i].costs;
            delete[] pool_parent[i].genes;
        }

        delete[] pool_parent;
        pool_parent = NULL;
    }

    if (pool_mating != NULL) {
        for (int i = 0; i < POOL_MATING_SIZE; i++) {
            delete[] pool_mating[i].costs;
            delete[] pool_mating[i].genes;
        }

        delete[] pool_mating;
        pool_mating = NULL;
    }

    if (pool_offspring != NULL) {
        for (int i = 0; i < POOL_OFFSPRING_SIZE; i++) {
            delete[] pool_offspring[i].costs;
            delete[] pool_offspring[i].genes;
        }

        delete[] pool_offspring;
        pool_offspring = NULL;
    }

    if (pool_clones != NULL) {
        for (int i = 0; i < pool_clones_size; i++) {
            delete[] pool_clones[i].costs;
            delete[] pool_clones[i].genes;
        }

        delete[] pool_clones;
        pool_clones = NULL;
    }

    if (samples_log != NULL) {
        delete samples_log;
    }

    delete jjData;

    logger->log(3, "GA elapsed time %d seconds: %s", (int)(time(NULL) - start_seconds), injjfilename);
}

void GAProtection::allocate_pools() {
    if (number_of_genes == 0) {
            max_evaluations = 0;
            pool_parent_size = 0;
            default_number_of_clones = 0;
            pool_clones_size = 0;

            pool_parent = NULL;
            pool_mating = NULL;
            pool_offspring = NULL;
            pool_clones = NULL;
    } else {
            // Limit the number of evaluations to the lower of the number of permutations or MAX_EVALUATIONS
            int n = 1;

            for (int i = 2; i <= number_of_genes; i++) {
                n *= i;
                if (n > MAX_EVALUATIONS) {
                    break;
                }
            }

            max_evaluations = MIN(n, MAX_EVALUATIONS);
            logger->log(3, "Maximum number of evaluations %d", max_evaluations);

            pool_parent_size = MIN(cores, max_evaluations);

            logger->log(3, "Number of parents %d", pool_parent_size);

            default_number_of_clones = MIN(cores, (max_evaluations - pool_parent_size));

            // Maximum possible number of clones occurs when all offspring are cached and the same number of random individuals are created to use up all idle solvers
            pool_clones_size = default_number_of_clones * 2;

            logger->log(3, "Default number of clones %d", default_number_of_clones);

            pool_parent = new Individual[POOL_PARENT_SIZE];
            pool_mating = new Individual[POOL_MATING_SIZE];
            pool_offspring = new Individual[POOL_OFFSPRING_SIZE];
            pool_clones = new Individual[pool_clones_size];

            for (int i = 0; i < POOL_PARENT_SIZE; i++) {
                pool_parent[i].genes = new CellIndex[number_of_genes];
                pool_parent[i].costs = new double[number_of_genes];
                pool_parent[i].number_of_costs = 0;
                pool_parent[i].fitness = 0.0;
            }

            for (int i = 0; i < POOL_MATING_SIZE; i++) {
                pool_mating[i].genes = new CellIndex[number_of_genes];
                pool_mating[i].costs = new double[number_of_genes];
                pool_mating[i].number_of_costs = 0;
                pool_mating[i].fitness = 0.0;
            }

            for (int i = 0; i < POOL_OFFSPRING_SIZE; i++) {
                pool_offspring[i].genes = new CellIndex[number_of_genes];
                pool_offspring[i].costs = new double[number_of_genes];
                pool_offspring[i].number_of_costs = 0;
                pool_offspring[i].fitness = 0.0;
            }

            for (int i = 0; i < pool_clones_size; i++) {
                pool_clones[i].genes = new CellIndex[number_of_genes];
                pool_clones[i].costs = new double[number_of_genes];
                pool_clones[i].number_of_costs = 0;
                pool_clones[i].fitness = 0.0;
            }
    }

    evaluationCache = new EvaluationCache(number_of_genes);
}

bool GAProtection::invalid_offspring(int offspring) {
    bool rc = false;
    int gene_count[NUMBER_OF_GENES];

    for (int i = 0; i < NUMBER_OF_GENES; i++) {
        gene_count[i] = 0;
    }

    for (GeneIndex i = 0; i < number_of_genes; i++) {
        CellIndex j = pool_offspring[offspring].genes[i];
        gene_count[j]++;
    }

    for (GeneIndex i = 1; i <= number_of_genes; i++) {
        if (gene_count[i] == 0) {
            // Missing gene
            logger->log(2, "Missing gene: %d", i);
            rc = true;
        } else if (gene_count[i] > 1) {
            // Duplicate gene
            logger->log(2, "Duplicate gene: %d", i);
            rc = true;
        }
    }

    return rc;
}

void GAProtection::sort_pool_by_fitness(int number_to_sort, struct Individual pool[]) {
    for (int i = 0; i < number_to_sort; i++) {
        for (int j = i; j < number_to_sort; j++) {
            if (pool[i].fitness > pool[j].fitness) {
                // Swap individuals
                // Note that we do a shallow copy here - swapping pointers only
                struct Individual temp;
                temp.genes = pool[i].genes;
                temp.costs = pool[i].costs;
                temp.number_of_costs = pool[i].number_of_costs;
                temp.fitness = pool[i].fitness;

                pool[i].genes = pool[j].genes;
                pool[i].costs = pool[j].costs;
                pool[i].number_of_costs = pool[j].number_of_costs;
                pool[i].fitness = pool[j].fitness;

                pool[j].genes = temp.genes;
                pool[j].costs = temp.costs;
                pool[j].number_of_costs = temp.number_of_costs;
                pool[j].fitness = temp.fitness;
            }
        }
    }
}

// Perform a deep copy of an individual
void GAProtection::copy_individual(struct Individual* to, struct Individual* from) {
    // Sanity check the number of costs
    if (from->number_of_costs > number_of_genes) {
        logger->error(1, "Incorrect number of costs");
    }

    for (GeneIndex i = 0; i < number_of_genes; i++) {
        to->genes[i] = from->genes[i];
    }

    for (GeneIndex i = 0; i < from->number_of_costs; i++) {
        to->costs[i] = from->costs[i];
    }

    to->number_of_costs = from->number_of_costs;
    to->fitness = from->fitness;
}

double GAProtection::get_best_fitness() {
    return pool_parent[get_best_parent()].fitness;
}

int GAProtection::get_best_parent() {
    double best_fitness = pool_parent[0].fitness;
    int best_parent = 0;

    for (int i = 1; i < pool_parent_size; i++) {
        double fitness = pool_parent[i].fitness;

        if (fitness < best_fitness) {
            best_fitness = fitness;
            best_parent = i;
        }
    }

    return best_parent;
}

double GAProtection::get_worst_fitness() {
    double worst_fitness = pool_parent[0].fitness;

    for (int i = 1; i < pool_parent_size; i++) {
        double fitness = pool_parent[i].fitness;

        if (fitness > worst_fitness) {
            worst_fitness = fitness;
        }
    }

    return worst_fitness;
}

/******************************************************************************************/
/*                                                                                        */
/*                                        Selection                                       */
/*                                                                                        */
/******************************************************************************************/

void GAProtection::selection_truncation() {
    // Copy fittest x%
    sort_pool_by_fitness(pool_parent_size, pool_parent);

    for (int i = 0; i < POOL_MATING_SIZE; i++) {
        copy_individual(&pool_mating[i], &pool_parent[i]);
    }
}

void GAProtection::selection_tournament() {
    for (int i = 0; i < POOL_MATING_SIZE; i++) {
        int l = (int)((double)(pool_parent_size) * random());
        int k = (int)((double)(pool_parent_size) * random());

        if (l < 0) {
            l = 0;
        } else if (l >= pool_parent_size) {
            l = pool_parent_size - 1;
        }

        if (k < 0) {
            k = 0;
        } else if (k >= pool_parent_size) {
            k = pool_parent_size - 1;
        }

        if ((l >= 0) && (l < pool_parent_size) && (k >= 0) && (k < pool_parent_size)) {
            if (pool_parent[l].fitness < pool_parent[k].fitness) {
                copy_individual(&pool_mating[i], &pool_parent[l]);
            } else {
                copy_individual(&pool_mating[i], &pool_parent[k]);
            }
        }
    }
}

void GAProtection::selection_proportionate() {
    int l = 0;
    double *fitness_proportionate = new double[pool_parent_size];
    double total_fitness = 0.0;

    for (int i = 0; i < pool_parent_size; i++) {
        total_fitness = total_fitness + (double) pool_parent[i].fitness;
    }

    fitness_proportionate[0] = ((double) pool_parent[0].fitness) / total_fitness;

    for (int i = 1; i < pool_parent_size; i++) {
        fitness_proportionate[i] = fitness_proportionate[i - 1] + ((double) pool_parent[i].fitness) / total_fitness;
    }

    for (int i = 0; i < POOL_MATING_SIZE; i++) {
        double r = random();

        for (int k = 0; k < pool_parent_size; k++) {
            if (r > fitness_proportionate[k]) {
                l = k;
            }
        }

        if ((l >= 0) && (l < pool_parent_size)) {
            copy_individual(&pool_mating[i], &pool_parent[l]);
        }
    }

    delete[] fitness_proportionate;
}

void GAProtection::select_for_pool_mating() {
    switch (algorithm_for_selection) {
        case SELECTION_TRUNCATION:
            selection_truncation();
            break;

        case SELECTION_TOURNAMENT:
            selection_tournament();
            break;

        case SELECTION_FITNESS_PROPORTIONATE:
            selection_proportionate();
            break;

        default:
            break;
    }
}

/******************************************************************************************/
/*                                                                                        */
/*                           Crossover                                                    */
/*                                                                                        */
/******************************************************************************************/

void GAProtection::print_crossover_details(int parent1, int parent2, int offspring) {
    logger->log(5, "Parent 1:");
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        logger->log(5, " %2d", pool_mating[parent1].genes[i]);
    }

    logger->log(5, "Parent 2:");
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        logger->log(5, " %2d", pool_mating[parent2].genes[i]);
    }

    logger->log(5, "Offspring:");
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        logger->log(5, " %2d", pool_offspring[offspring].genes[i]);
    }
}

CellIndex GAProtection::find_gene_location(int parent, int gene) {
    int rc = -1;

    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if (gene == pool_mating[parent].genes[i]) {
            rc = i;
        }
    }

    if (rc == -1) {
        logger->error(1, "Gene %d not found in parent %d", gene, parent);
    }

    return rc;
}

void GAProtection::map_gene(int parent1, int g1, int parent2, int g2) {
    CellIndex j = find_gene_location(parent2, g1);

    if (pool_offspring[number_of_offspring].genes[j] == 0) {
        pool_offspring[number_of_offspring].genes[j] = g2;
    } else {
        g1 = pool_mating[parent1].genes[j];

        map_gene(parent1, g1, parent2, g2);
    }
}

void GAProtection::crossover_partially_mapped(int parent1, int parent2) {
    if (number_of_offspring < POOL_OFFSPRING_SIZE) {
        GeneIndex i = (GeneIndex)(random() * number_of_genes);

        GeneIndex j = i;

        while (j == i) {
            j = (GeneIndex)(random() * number_of_genes);
        }

        GeneIndex min;
        GeneIndex max;
        if (i > j) {
            max = i;
            min = j;
        } else {
            max = j;
            min = i;
        }

        for (GeneIndex k = 0; k < number_of_genes; k++) {
            pool_offspring[number_of_offspring].genes[k] = -1;
        }

        for (GeneIndex k = min; k <= max; k++) {
            pool_offspring[number_of_offspring].genes[k] = pool_mating[parent1].genes[k];
        }

        for (GeneIndex k = min; k <= max; k++) {
            CellIndex g2 = pool_mating[parent2].genes[k];

            j = find_gene_location(parent1, g2);

            if ((j < min) || (j > max)) {
                CellIndex g1 = pool_mating[parent1].genes[k];

                map_gene(parent1, g1, parent2, g2);
            }
        }

        for (GeneIndex k = 0; k < number_of_genes; k++) {
            if (pool_offspring[number_of_offspring].genes[k] == -1) {
                pool_offspring[number_of_offspring].genes[k] = pool_mating[parent2].genes[k];
            }
        }

        number_of_offspring++;
    }
}

void GAProtection::crossover_distance_preserving(int parent1, int parent2) {
    int number_of_fragments = 0;
    int length = 0;

    // Initialise the work area
    int parent1_location[NUMBER_OF_GENES];
    int parent2_location[NUMBER_OF_GENES];
    int fragment_length [NUMBER_OF_GENES];
    bool parent2_reversed[NUMBER_OF_GENES];
    int parent_totals [NUMBER_OF_GENES];
    int offspring_order [NUMBER_OF_GENES];

    for (GeneIndex i = 0; i < number_of_genes; i++) {
        parent1_location[i] = 0;
        parent2_location[i] = 0;
        fragment_length [i] = 0;
        parent2_reversed[i] = false;
        parent_totals[i] = 0;
        offspring_order[i] = i;
    }

    // Identify the fragments
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        CellIndex gene = pool_mating[parent1].genes[i];
        GeneIndex j = find_gene_location(parent2, gene);

        GeneIndex next_i = (i + 1) % number_of_genes;
        GeneIndex next_j = (j + 1) % number_of_genes;
        GeneIndex previous_j = j - 1;
        if (previous_j < 0) previous_j = number_of_genes - 1;

        if (length == 0) {
            length++;
            parent1_location[number_of_fragments] = i;
            parent2_location[number_of_fragments] = j;
            fragment_length [number_of_fragments] = length;

            if (pool_mating[parent1].genes[next_i] == pool_mating[parent2].genes[next_j]) {
            } else if (pool_mating[parent1].genes[next_i] == pool_mating[parent2].genes[previous_j]) {
                parent2_reversed[number_of_fragments] = true;
            } else {
                length = 0;
                number_of_fragments++;
            }
        } else {
            length++;
            fragment_length [number_of_fragments] = length;

            if (parent2_reversed[number_of_fragments]) {
                parent2_location[number_of_fragments] = j;

                if (pool_mating[parent1].genes[next_i] == pool_mating[parent2].genes[previous_j]) {
                } else {
                    length = 0;
                    number_of_fragments++;
                }
            } else {
                if (pool_mating[parent1].genes[next_i] == pool_mating[parent2].genes[next_j]) {
                } else {
                    length = 0;
                    number_of_fragments++;
                }
            }
        }
    }

    if (length > 0) {
        number_of_fragments++;
    }

    // Calculate average positions of the fragments
    for (int i = 0; i < number_of_fragments; i++) {
        parent_totals[i] = parent1_location[i] + parent2_location[i];
    }

    // Sort by average positions ready to set up the offspring
    for (int i = 0; i < number_of_fragments; i++) {
        for (int j = i; j < number_of_fragments; j++) {
            if (parent_totals[i] > parent_totals[j]) {
                int temp = parent_totals[i];
                parent_totals[i] = parent_totals[j];
                parent_totals[j] = temp;

                temp = offspring_order[i];
                offspring_order[i] = offspring_order[j];
                offspring_order[j] = temp;
            }
        }
    }

    // Setup the offspring
    int offspring_index = 0;

    for (int i = 0; i < number_of_fragments; i++) {
        int fragment = offspring_order[i];

        if ((i % 2) == 0) {
            for (int j = 0; j < fragment_length[fragment]; j++) {
                GeneIndex parent_index = (parent1_location[fragment] + j) % number_of_genes;
                pool_offspring[number_of_offspring].genes[offspring_index] = pool_mating[parent1].genes[parent_index];
                offspring_index++;
            }
        } else {
            for (int j = 0; j < fragment_length[fragment]; j++) {
                GeneIndex parent_index = (parent2_location[fragment] + j) % number_of_genes;
                pool_offspring[number_of_offspring].genes[offspring_index] = pool_mating[parent2].genes[parent_index];
                offspring_index++;
            }
        }
    }

    if (invalid_offspring(number_of_offspring)) {
        print_crossover_details(parent1, parent2, number_of_offspring);
        logger->error(1, "Invalid offspring");
    }

    number_of_offspring++;
}

void GAProtection::crossover_edge(int parent1, int parent2) {
    logger->error(1, "Edge crossover not implemented");
}

bool GAProtection::not_in_genes(CellIndex g, CellIndex *genes) {
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if (g == genes[i]) {
            return false;
        }
    }

    return true;
}

void GAProtection::crossover_order(int parent1, int parent2) {
    if (number_of_offspring < POOL_OFFSPRING_SIZE) {
        GeneIndex i = (GeneIndex)(random() * number_of_genes);

        GeneIndex j = i;

        while (j == i) {
            j = (GeneIndex)(random() * number_of_genes);
        }

        GeneIndex min;
        GeneIndex max;
        if (i > j) {
            max = i;
            min = j;
        } else {
            max = j;
            min = i;
        }

        for (GeneIndex k = 0; k < number_of_genes; k++) {
            pool_offspring[number_of_offspring].genes[k] = -1;
        }

        for (GeneIndex k = min; k <= max; k++) {
            pool_offspring[number_of_offspring].genes[k] = pool_mating[parent1].genes[k];
        }

        i = (max + 1) % number_of_genes;

        for (GeneIndex k = 0; k < number_of_genes; k++) {
            j = (max + k + 1) % number_of_genes;

            CellIndex g = pool_mating[parent2].genes[j];

            if (not_in_genes(g, pool_offspring[number_of_offspring].genes)) {
                pool_offspring[number_of_offspring].genes[i] = g;
                i = (i + 1) % number_of_genes;
            }
        }

        for (GeneIndex k = 0; k < number_of_genes; k++) {
            if (pool_offspring[number_of_offspring].genes[k] == -1) {
                logger->error(1, "Order crossover failed");
            }
        }

        number_of_offspring++;
    }
}

void GAProtection::crossover_cycle(int parent1, int parent2) {
    logger->error(1, "Cycle crossover not implemented");
}

bool GAProtection::valid_mating(int parent) {
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if ((pool_mating[parent].genes[i] < 0) || (pool_mating[parent].genes[i] >= jjData->ncells)) {
            logger->error(1, "Invalid parent %d", parent);
        }
    }

    return true;
}

void GAProtection::apply_crossover() {
    int parent1;
    int parent2;

    number_of_offspring = 0;

    while (number_of_offspring < POOL_OFFSPRING_SIZE) {
        if (POOL_MATING_SIZE == 2) {
            parent1 = 0;
            parent2 = 1;
        } else {
            parent1 = (int)((double)(POOL_MATING_SIZE) * random());
            parent2 = (int)((double)(POOL_MATING_SIZE) * random());

            int sanity = 0;

            while ((parent1 == parent2) && (sanity < 10)) {
                parent2 = (int)((double)(POOL_MATING_SIZE) * random());
                sanity++;
            }
        }

        if (valid_mating(parent1) && valid_mating(parent2)) {
            if (random() < RECOMBINATION_PROBABILITY) {
                switch (algorithm_for_crossover) {
                    case CROSSOVER_PARTIALLY_MAPPED:
                        crossover_partially_mapped(parent1, parent2);
                        break;

                    case CROSSOVER_EDGE:
                        crossover_edge(parent1, parent2);
                        break;

                    case CROSSOVER_ORDER:
                        crossover_order(parent1, parent2);
                        break;

                    case CROSSOVER_CYCLE:
                        crossover_cycle(parent1, parent2);
                        break;

                    case CROSSOVER_DISTANCE_PRESERVING:
                        crossover_distance_preserving(parent1, parent2);
                        break;

                    default:
                        break;
                }
            } else {
                offspring1 = number_of_offspring;
                offspring2 = number_of_offspring + 1;

                if (offspring1 < POOL_OFFSPRING_SIZE) {
                    for (GeneIndex i = 0; i < number_of_genes; i++) {
                        pool_offspring[offspring1].genes[i] = pool_mating[parent1].genes[i];
                    }

                    number_of_offspring++;
                }

                if (offspring2 < POOL_OFFSPRING_SIZE) {
                    for (GeneIndex i = 0; i < number_of_genes; i++) {
                        pool_offspring[offspring2].genes[i] = pool_mating[parent2].genes[i];
                    }
                    number_of_offspring++;
                }
            }
        }
    }
}

void GAProtection::duplicate_clones(int offspring) {
    for (int i = 0; i < pool_clones_size; i++) {
        copy_individual(&pool_clones[i], &pool_offspring[offspring]);
    }
}

/******************************************************************************************/
/*                                                                                        */
/*                           Mutation                                                     */
/*                                                                                        */
/******************************************************************************************/

void GAProtection::randomise_clone(int offspring) {
    mutation_swap(offspring, 1.0);
}

void GAProtection::mutation_swap(int offspring, double mutation_rate) {
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if (random() <= mutation_rate) {
            GeneIndex j = i;

            while (j == i) {
                j = (GeneIndex)(random() * number_of_genes);
            }

            CellIndex gene = pool_clones[offspring].genes[i];
            pool_clones[offspring].genes[i] = pool_clones[offspring].genes[j];
            pool_clones[offspring].genes[j] = gene;
        }
    }
}

void GAProtection::mutation_insert(int offspring, double mutation_rate) {
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if (random() <= mutation_rate) {
            GeneIndex j = i;

            while (j == i) {
                j = (GeneIndex)(random() * number_of_genes);
            }

            if (i < j) {
                CellIndex gene = pool_clones[offspring].genes[j];

                for (GeneIndex k = j; k > i + 1; k--) {
                    pool_clones[offspring].genes[k] = pool_clones[offspring].genes[k - 1];
                }
                pool_clones[offspring].genes[i + 1] = gene;
            } else if (i > j) {
                CellIndex gene = pool_clones[offspring].genes[j];

                for (GeneIndex k = j; k < i - 1; k++) {
                    pool_clones[offspring].genes[k] = pool_clones[offspring].genes[k + 1];
                }
                pool_clones[offspring].genes[i - 1] = gene;
            }
        }
    }
}

void GAProtection::mutation_scramble(int offspring, double mutation_rate) {
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if (random() <= mutation_rate) {
            GeneIndex j = i;

            while (j == i) {
                j = (GeneIndex)(random() * number_of_genes);
            }

            GeneIndex min;
            GeneIndex max;
            if (i > j) {
                max = i;
                min = j;
            } else {
                max = j;
                min = i;
            }
            GeneIndex range = max - min;

            for (GeneIndex k = 0; k < number_of_genes; k++) {
                GeneIndex m = min + (GeneIndex)(random() * range);
                GeneIndex n = min + (GeneIndex)(random() * range);

                CellIndex gene = pool_clones[offspring].genes[m];
                pool_clones[offspring].genes[m] = pool_clones[offspring].genes[n];
                pool_clones[offspring].genes[n] = gene;
            }
        }
    }
}

void GAProtection::mutation_inversion(int offspring, double mutation_rate) {
    for (GeneIndex i = 0; i < number_of_genes; i++) {
        if (random() <= mutation_rate) {
            GeneIndex j = i;

            while (j == i) {
                j = (GeneIndex)(random() * number_of_genes);
            }

            GeneIndex min;
            GeneIndex max;
            if (i > j) {
                max = i;
                min = j;
            } else {
                max = j;
                min = i;
            }
            GeneIndex range = (max - min) / 2;

            for (GeneIndex k = 0; k < range; k++) {
                CellIndex gene = pool_clones[offspring].genes[min + k];
                pool_clones[offspring].genes[min + k] = pool_clones[offspring].genes[max - k];
                pool_clones[offspring].genes[max - k] = gene;
            }
        }
    }
}

void GAProtection::apply_mutation() {
    // Randomise one of the clones if population size is below the desired limit
    int random_offspring = (pool_parent_size < POOL_PARENT_SIZE)? 1: 0;

    for (int offspring = 0; offspring < random_offspring; offspring++) {
        mutation_swap(offspring, 1.0);
    }

    double mutation_rate = 1.0 / (double)number_of_genes;

    for (int offspring = random_offspring; offspring < default_number_of_clones; offspring++) {

        switch (algorithm_for_mutation) {
            case MUTATION_SWAP:
                mutation_swap(offspring, mutation_rate);
                break;

            case MUTATION_INSERT:
                mutation_insert(offspring, mutation_rate);
                break;

            case MUTATION_SCRAMBLE:
                mutation_scramble(offspring, mutation_rate);
                break;

            case MUTATION_INVERSION:
                mutation_inversion(offspring, mutation_rate);
                break;

            case MUTATION_ASSORTED:
                if (mutation_type == MUTATION_SWAP) {
                    mutation_swap(offspring, mutation_rate);
                } else if (mutation_type == MUTATION_INSERT) {
                    mutation_insert(offspring, mutation_rate);
                } else if (mutation_type == MUTATION_SCRAMBLE) {
                    mutation_scramble(offspring, mutation_rate);
                } else {
                    mutation_inversion(offspring, mutation_rate);
                }
                mutation_type = (mutation_type + 1) % 4;
                break;

            default:
                break;
        }
    }
}

/******************************************************************************************/
/*                                                                                        */
/*                                        Replacement                                     */
/*                                                                                        */
/******************************************************************************************/

int GAProtection::get_fittest_clone(int pool_size) {
    double best_fitness = pool_clones[0].fitness;
    int fittest = 0;
    for (int i = 1; i < pool_size; i++) {
        if (pool_clones[i].fitness < best_fitness) {
            best_fitness = pool_clones[i].fitness;
            fittest = i;
        }
    }

    int nmr_fittest_clones = 0;
    for (int i = 0; i < pool_size; i++) {
        if (pool_clones[i].fitness == best_fitness) {
            nmr_fittest_clones++;
        }
    }

    if (nmr_fittest_clones == 1) {
        // Exactly one fittest clone
        return fittest;
    } else {
        // Multiple fittest clones - choose one at random
        int selection = (int)(random() * nmr_fittest_clones);

        int n = 0;
        for (int i = 0; i < pool_size; i++) {
            if (pool_clones[i].fitness == best_fitness) {
                if (n == selection) {
                    return i;
                } else {
                    n++;
                }
            }
        }

        // Keep the compiler from complaining
        logger->error(1, "Unexpected error");
        return 0;
    }
}

void GAProtection::replace_nothing(int pool_size) {
    // Get fittest clone
    int fittest = get_fittest_clone(pool_size);

    // Add to population and grow population
    copy_individual(&pool_parent[pool_parent_size], &pool_clones[fittest]);

    pool_parent_size++;
}

void GAProtection::replace_oldest(int pool_size) {
    // Get fittest clone
    int fittest = get_fittest_clone(pool_size);

    // Do replacement
    copy_individual(&pool_parent[replace_next], &pool_clones[fittest]);

    replace_next++;

    if (replace_next >= pool_parent_size) {
        replace_next = 0;
    }
}

void GAProtection::replace_worst(int pool_size) {
    // Get fittest clone
    int fittest = get_fittest_clone(pool_size);

    // Do replacement
    sort_pool_by_fitness(pool_parent_size, pool_parent);

    replace_next = pool_parent_size - 1;

    copy_individual(&pool_parent[replace_next], &pool_clones[fittest]);

    replace_next--;

    if (replace_next < 0) {
        replace_next = pool_parent_size - 1;
    }
}

void GAProtection::replace_tournament(int pool_size) {
    // Get fittest clone
    int fittest = get_fittest_clone(pool_size);

    // Do replacement
    if (pool_parent[replace_next].fitness > pool_clones[fittest].fitness) {
        copy_individual(&pool_parent[replace_next], &pool_clones[fittest]);
    }

    replace_next++;

    if (replace_next >= pool_parent_size) {
        replace_next = 0;
    }
}

void GAProtection::replace_worst_by_tournament(int pool_size) {
    // Get fittest clone
    int fittest = get_fittest_clone(pool_size);

    // Find worst
    replace_next = 0;
    for (int i = 1; i < pool_parent_size; i++) {
        if (pool_parent[i].fitness > pool_parent[replace_next].fitness) {
            replace_next = i;
        }
    }

    // Do replacement
    if (pool_parent[replace_next].fitness > pool_clones[fittest].fitness) {
        copy_individual(&pool_parent[replace_next], &pool_clones[fittest]);
    }
}

void GAProtection::replacement(int pool_size) {
    if (pool_parent_size < POOL_PARENT_SIZE) {
        replace_nothing(pool_size);
    } else {
        switch (algorithm_for_replacement) {
            case REPLACE_OLDEST:
                replace_oldest(pool_size);
                break;

            case REPLACE_WORSE:
                replace_worst(pool_size);
                break;

            case REPLACE_TOURNAMENT:
                replace_tournament(pool_size);
                break;

            case REPLACE_WORSE_BY_TOURNAMENT:
                replace_worst_by_tournament(pool_size);
                break;

            default:
                break;
        }
    }
}

/******************************************************************************************/
/*                                                                                        */
/*                                   Evaluation                                           */
/*                                                                                        */
/******************************************************************************************/

bool GAProtection::time_to_terminate() {
    if ((number_of_genes == 0) || terminated) {
        return true;
    }

    if (number_of_counted_evals >= max_evaluations) {
        logger->log(3, "GA terminated due to evaluation limit of %d evaluations being reached: %s", max_evaluations, injjfilename);
        terminated = true;
        return true;
    }

    double current_fittest = get_best_fitness();

    if (fabs(current_fittest - stable_fitness) < FLOAT_PRECISION) {
        stable_generations++;
    } else {
        stable_generations = 0;
    }

    stable_fitness = current_fittest;

    if (stable_generations >= STABLE_FOR_X_GENERATIONS) {
        logger->log(3, "GA terminated due to stability for %d generations: %s", STABLE_FOR_X_GENERATIONS, injjfilename);
        terminated = true;
        return true;
    }

    if ((time(NULL) - start_seconds) > max_seconds) {
        logger->log(3, "GA terminated due to time limit of %d seconds being reached: %s", max_seconds, injjfilename);
        terminated = true;
        return true;
    }

    return false;
}

// Return the total number of usages of the solver
int GAProtection::number_of_evaluations() {
    return number_of_evals;
}

void GAProtection::increase_polling_delay(int *delay) {
    switch (*delay) {
        case 0:
            *delay = 1;
            break;

        case 128:
            // Do nothing - maximum value reached
            break;

        default:
            *delay *= 2;
    }
}

int GAProtection::solvers_required(int number_to_evaluate, struct Individual pool[], int model_type) {
    bool *requires_evaluation = new bool[number_to_evaluate];

    // Check for cached individuals
    for (int i = 0; i < number_to_evaluate; i++) {
        if (evaluationCache->cached(pool[i].genes, model_type)) {
            requires_evaluation[i] = false;
        } else {
            requires_evaluation[i] = true;
        }
    }

    // Check for duplicate individuals
    Evaluation **eval = new Evaluation *[number_to_evaluate];

    for (int i = 0; i < number_to_evaluate; i++) {
        if (requires_evaluation[i]) {
            eval[i] = new Evaluation(pool[i].genes, number_of_genes, model_type);
        } else {
            eval[i] = NULL;
        }
    }

    for (int i = 0; i < number_to_evaluate; i++) {
        if (eval[i]) {
            for (int j = i + 1; j < number_to_evaluate; j++) {
                if ((eval[j]) && (requires_evaluation[j])) {
                    if (eval[j]->equals(eval[i])) {
                        requires_evaluation[j] = false;
                    }
                }
            }
        }
    }

    // Enumerate how many solvers are required for the specified pool
    int count = 0;
    for (int i = 0; i < number_to_evaluate; i++) {
        if (requires_evaluation[i]) {
            count++;
        }

        if (eval[i]) {
            delete eval[i];
        }
    }

    delete[] eval;
    delete[] requires_evaluation;

    logger->log(5, "%d solvers required", count);

    return count;
}

void GAProtection::fill_parent_pool(int model_type) {
    // Code assumes that two ordered individuals are already in the pool
    int used_pool_size = 2;
    int solvers_currently_required = solvers_required(used_pool_size, pool_parent, model_type);

    // If not all solvers are being utilised fill the pool with new unevaluated random individuals to make use of solvers
    while (solvers_currently_required < pool_parent_size) {
        // Add a random individual to the pool
        for (GeneIndex j = 0; j < number_of_genes; j++) {
            GeneIndex k = (GeneIndex)((double)(number_of_genes) * random());

            // Swap order
            CellIndex gene = pool_parent[used_pool_size].genes[k];
            pool_parent[used_pool_size].genes[k] = pool_parent[used_pool_size].genes[j];
            pool_parent[used_pool_size].genes[j] = gene;
        }

        // Check whether the addition made a difference (individual may be a duplicate of another pool member)
        int solvers_now_required = solvers_required(used_pool_size + 1, pool_parent, model_type);
        if (solvers_now_required > solvers_currently_required) {
            solvers_currently_required = solvers_now_required;
            used_pool_size++;
        }
    }
}

int GAProtection::grow_clones_pool(int model_type) {
    int used_pool_size = default_number_of_clones;

    int max_solvers = MIN(default_number_of_clones, (max_evaluations - number_of_counted_evals));
    if (max_solvers != default_number_of_clones) {
        logger->log(3, "Maximum number of solvers is limited to %d", max_solvers);
    }

    // Check how many solvers will be needed for the default clone pool
    int solvers_currently_required = solvers_required(used_pool_size, pool_clones, model_type);

    int randomisation_attempts = 0;

    // If not all solvers are being utilised grow the pool with new unevaluated random individuals to make use of solvers
    while ((solvers_currently_required < max_solvers) && (randomisation_attempts < MAX_RANDOMISATION_ATTEMPTS)) {
        // Add a random individual to the pool
        randomise_clone(used_pool_size);

        // Check whether the addition made a difference (individual may have already been evaluated or be a duplicate of another pool member)
        int solvers_now_required = solvers_required(used_pool_size + 1, pool_clones, model_type);
        if (solvers_now_required == solvers_currently_required) {
            randomisation_attempts++;
        } else {
            solvers_currently_required = solvers_now_required;
            used_pool_size++;
            randomisation_attempts = 0;
        }
    }

    return used_pool_size;
}

void GAProtection::evaluate_fitness(int number_to_evaluate, struct Individual pool[], int protection_type, int model_type, bool get_outputjjfile, bool count_evals, double max_cost) {
    int elapsed_time;
    bool solver_terminated;

    // Sleep quantum is 10 milliseconds
    sys.initialise_sleep(10);

    int *status = new int[number_to_evaluate];
    Solver **solver = new Solver *[number_to_evaluate];
    int *delay = new int[number_to_evaluate];
    int *counter = new int[number_to_evaluate];

    // Check for cached individuals
    for (int i = 0; i < number_to_evaluate; i++) {
        if (evaluationCache->cached(pool[i].genes, model_type)) {
            pool[i].number_of_costs = evaluationCache->costs(pool[i].genes, model_type, &pool[i].costs);
            pool[i].fitness = evaluationCache->fitness(pool[i].genes, model_type);

            logger->log(3, "Cached fitness evaluation %d %d %d %lf", protection_type, model_type, i, pool[i].fitness);
            evaluationCache->log_genome(pool[i].genes, model_type);
            evaluationCache->log_costs(pool[i].genes, model_type);

            status[i] = 0;
        } else {
            status[i] = -3;
        }
        solver[i] = NULL;
        delay[i] = 0;
        counter[i] = 0;
    }
    logger->log(5, "Unallocated solver delay 0");

    // Mark duplicate individuals so that they are not evaluated
    Evaluation **eval = new Evaluation *[number_to_evaluate];

    for (int i = 0; i < number_to_evaluate; i++) {
        if (status[i] == -3) {
            eval[i] = new Evaluation(pool[i].genes, number_of_genes, model_type);
        } else {
            eval[i] = NULL;
        }
    }

    for (int i = 0; i < number_to_evaluate; i++) {
        if (eval[i]) {
            for (int j = i + 1; j < number_to_evaluate; j++) {
                if ((eval[j]) && (status[j] == -3)) {
                    if (eval[j]->equals(eval[i])) {
                        status[j] = -4;
                    }
                }
            }
        }
    }

    bool complete;

    do {
        complete = true;

        for (int i = 0; i < number_to_evaluate; i++) {
            if (counter[i] == 0) {
                switch (status[i]) {
                    // Solver not yet allocated - try to obtain one
                    case -3:
                        try {
                            solver[i] = new Solver(host, port);

                            // Check that the version of the server protocol is understood by the client
                            if (solver[i]->getProtocol() != 4) {
                                // Terminate all of this client's remote solvers before exiting
                                for (int j = 0; j < number_to_evaluate; j++) {
                                    if (solver[j] != NULL) {
                                        delete solver[j];
                                    }
                                }

                                logger->error(1, "Unsupported version of client server protocol");
                            }
                            logger->log(5, "Solver %s created", solver[i]->getSession());

                            status[i] = -2;
                            delay[i] = 0;
                            counter[i] = 0;

                            char *in_jj_file;
                            if (model_type == YPLUS_MODEL) {
                                in_jj_file = injjfilename;
                            } else {
                                in_jj_file = evaluationCache->jjFile(pool[i].genes, YPLUS_MODEL);
                                if (in_jj_file == NULL) {
                                    logger->error(1, "Missing cached yplus jj file");
                                }
                            }

                            // Create a permutation file for the solver
                            char temp_file[MAX_FILENAME_SIZE];
                            sys.make_tempfile(temp_file, MAX_FILENAME_SIZE);
                            write_perm_file(temp_file, pool[i].genes, number_of_genes);

                            // The remote solver interprets a max_cost of zero to mean unlimited cost (and hence no early termination)
                            solver[i]->runProtection(in_jj_file, temp_file, protection_type, model_type, max_cost);

                            sys.remove_file(temp_file);
                        } catch (int e) {
                            // Keep the compiler from complaining
                            e = 0;

                            // Session not available - increase delay for all unallocated solvers
                            increase_polling_delay(&delay[i]);

                            for (int j = 0; j < number_to_evaluate; j++) {
                                if (status[j] == -3) {
                                    delay[j] = delay[i];
                                    counter[j] = delay[i];
                                }
                            }

                            logger->log(5, "Unallocated solver delay %d", delay[i]);
                        }

                        complete = false;
                        break;

                    // Solver running - check completion status
                    case -2:
                    case -1:
                        status[i] = solver[i]->getStatus();
                        logger->log(5, "Solver %s status %d", solver[i]->getSession(), status[i]);
                        switch (status[i]) {
                            case -2:
                            case -1:
                                // Solver still running
                                complete = false;

                                increase_polling_delay(&delay[i]);
                                counter[i] = delay[i];
                                logger->log(5, "Solver %s delay %d", solver[i]->getSession(), delay[i]);
                                break;

                            case 0:
                                // Solver completed
                                logger->log(5, "Solver %s completed", solver[i]->getSession());
                                pool[i].fitness = solver[i]->getResult();
                                elapsed_time = solver[i]->getElapsedTime();

                                // Get the costs
                                char temp_file[MAX_FILENAME_SIZE];
                                sys.make_tempfile(temp_file, MAX_FILENAME_SIZE);
                                solver[i]->getCostFile(temp_file);
                                pool[i].number_of_costs = read_cost_file(temp_file, pool[i].costs);
                                sys.remove_file(temp_file);

                                // Cross-check fitness and costs
                                if (fabs(pool[i].fitness - pool[i].costs[pool[i].number_of_costs - 1]) >= FLOAT_PRECISION) {
                                    logger->error(1, "Fitness (%lf) does not match costs (%lf)", pool[i].fitness, pool[i].costs[number_of_genes - 1]);
                                }

                                // Keep a copy of the intermediate result for YPLUS as this may be used as the basis for a subsequent YMINUS model when evaluating the best individual
                                // Also, the result may be used during GA elimination
                                char* out_jj_file;
                                if ((model_type == YPLUS_MODEL) || run_elimination) {
                                    sys.make_tempfile(temp_file, MAX_FILENAME_SIZE);
                                    solver[i]->getJJFile(temp_file);
                                    out_jj_file = temp_file;
                                } else {
                                    out_jj_file = NULL;
                                }

                                // Cache JJ file, costs and fitness
                                evaluationCache->add(pool[i].genes, model_type, out_jj_file, pool[i].costs, pool[i].fitness, pool[i].number_of_costs);

                                if (samples_log != NULL) {
                                    samples_log->log_sample(&pool[i], pool[i].fitness, protection_type, model_type, run_elimination? out_jj_file: NULL, elapsed_time);
                                }

                                // Get a permanent copy of the output file if required
                                if (get_outputjjfile) {
                                    if (out_jj_file == NULL) {
                                        // File not yet downloaded
                                        solver[i]->getJJFile(outjjfilename);
                                    } else {
                                        // File has already been downloaded so just copy locally
                                        // This situation only occurs with GA elimination
                                        sys.copy_file(outjjfilename, out_jj_file);
                                    }
                                }

                                delete solver[i];
                                solver[i] = NULL;
                                delay[i] = 0;
                                counter[i] = 0;

                                solver_terminated = (pool[i].number_of_costs == number_of_genes)? false: true;

                                if (count_evals) {
                                    number_of_counted_evals++;
                                    logger->log(3, "%s fitness evaluation %d %d %d %lf (evaluation %d)", solver_terminated? "Terminated": "Completed", protection_type, model_type, i, pool[i].fitness, number_of_counted_evals);
                                } else {
                                    logger->log(3, "%s fitness evaluation %d %d %d %lf (uncounted)", solver_terminated? "Terminated": "Completed", protection_type, model_type, i, pool[i].fitness);
                                }
                                evaluationCache->log_genome(pool[i].genes, model_type);
                                evaluationCache->log_costs(pool[i].genes, model_type);

                                number_of_evals++;

                                // Session freed - zero delay for all unallocated solvers
                                for (int j = 0; j < number_to_evaluate; j++) {
                                    if (status[j] == -3) {
                                        delay[j] = 0;
                                        counter[j] = 0;
                                    }
                                }

                                logger->log(5, "Unallocated solver delay 0");
                                break;

                            default:
                                // Solver error - terminate all of this client's remote solvers before exiting
                                for (int j = 0; j < number_to_evaluate; j++) {
                                    if (solver[j] != NULL) {
                                        delete solver[j];
                                    }
                                }

                                logger->error(1, "Solver error %d", status[i]);
                        }
                        break;

                    // default:
                        // Solver complete - do nothing
                }
            } else {
                counter[i]--;
                complete = false;
            }
        }

        if (! complete) {
            sys.sleep();
        }

    } while (! complete);

    // Fill in the fitness for duplicate individuals
    for (int i = 0; i < number_to_evaluate; i++) {
        if (eval[i]) {
            for (int j = i + 1; j < number_to_evaluate; j++) {
                if ((eval[j]) && (status[j] == -4)) {
                    if (eval[j]->equals(eval[i])) {
                        for (GeneIndex k = 0; k < pool[i].number_of_costs; k++) {
                            pool[j].costs[k] = pool[i].costs[k];
                        }
                        pool[j].number_of_costs = pool[i].number_of_costs;
                        pool[j].fitness = pool[i].fitness;
                        logger->log(3, "Duplicate fitness evaluation %d %d %d %lf", protection_type, model_type, j, pool[j].fitness);
                        eval[j]->log_genome();
                        status[j] = 0;
                    }
                }
            }
        }
    }

    for (int i = 0; i < number_to_evaluate; i++) {
        if (eval[i]) {
            delete eval[i];
        }
    }

    delete[] eval;
    delete[] counter;
    delete[] delay;
    delete[] solver;
    delete[] status;
}

double GAProtection::evaluate_best_parent(int protection_type) {
    int best_parent = get_best_parent();
    logger->log(3, "Evaluating best parent %d", best_parent);

    // Take a copy of the pool best parent because the fitness evaluation function updates the fitness, which in this case is that of the full model and we don't want that mixed with surrogate model fitness values
    struct Individual copy;
    copy.genes = new CellIndex[number_of_genes];
    copy.costs = new double[number_of_genes];
    copy_individual(&copy, &pool_parent[best_parent]);
    // max_cost parameter for evaluate_fitness is set to zero to ensure that the true cost of the solution is determined with no early termination of the remote solver
    evaluate_fitness(1, &copy, protection_type, YMINUS_MODEL, true, false, 0.0);
    delete[] copy.costs;
    delete[] copy.genes;

    return copy.fitness;
}

void GAProtection::write_perm_file(const char* filename, int* perm, int size) {
    FILE *ofp;

    if ((ofp = fopen(filename, "w")) == NULL) {
        logger->error(1, "Unable to create permutation file: %s", filename);
    }

    for (int i = 0; i < size; i++) {
        fprintf(ofp, "%d\n", perm[i]);
    }

    fclose(ofp);
}

int GAProtection::read_cost_file(const char* filename, double* costs) {
    FILE *ifp;

    int line_number = 0;

    if ((ifp = fopen(filename, "r")) == NULL) {
        logger->error(1, "Cost file not found: %s", filename);
    }

    for (CellIndex i = 0; i < number_of_genes; i++) {
        if (fscanf(ifp, "%lf\n",  &costs[i]) != 1) {
            break;
        }
        line_number++;
    }

    fclose(ifp);

    return line_number;
}
