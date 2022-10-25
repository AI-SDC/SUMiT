#include "stdafx.h"
#include <string.h>
#include "IncrementalGAProtection.h"
#include "Solver.h"
#include "CellStore.h"
#include "Groups.h"

IncrementalGAProtection::IncrementalGAProtection(const char *host, const char *port, const char *injjfilename, const char *outjjfilename, const char *samples_filename, unsigned int seed, int cores, int execution_time, bool run_elimination) : GAProtection(host, port, injjfilename, outjjfilename, samples_filename, seed, cores, execution_time, run_elimination) {
    // Select primary cells and store them
    CellStore *stored_cells = new CellStore(jjData);
    stored_cells->store_selected_cells();
    stored_cells->order_cells_by_largest_weighting();
    number_of_genes = stored_cells->size;

    logger->log(3, "%d primary cells", number_of_genes);

    allocate_pools();

    samples_log = NULL;

    if (number_of_genes > 0) {
        if (debugging) {
            samples_log = new SamplesLog(injjfilename, samples_filename, number_of_genes);
        }

        // Descending order of cells by loss of information weight
        for (int i = 0; i < pool_parent_size; i++) {
            for (GeneIndex j = 0; j < number_of_genes; j++) {
                pool_parent[i].genes[j] = stored_cells->cells[j];
            }
        }

        // Ascending order of cells by loss of information weight
        for (GeneIndex i = 0; i < number_of_genes; i++) {
            pool_parent[1].genes[i] = stored_cells->cells[number_of_genes - i - 1];
        }

        // Random order of cells
        fill_parent_pool(YPLUS_MODEL);

        evaluate_fitness(pool_parent_size, pool_parent, INDIVIDUAL_PROTECTION, YPLUS_MODEL, false, true, 0.0);
        evaluate_best_parent(INDIVIDUAL_PROTECTION);
    }

    delete stored_cells;
}

void IncrementalGAProtection::protect(bool limit_cost) {
    logger->log(3, "Incremental protection: %s", injjfilename);

    if (number_of_genes > 0) {
        double max_cost = get_worst_fitness();

        select_for_pool_mating();
        apply_crossover();

        for (int offspring = 0; offspring < POOL_OFFSPRING_SIZE; offspring++) {
            duplicate_clones(offspring);
            apply_mutation();

            int actual_pool_clones_size = grow_clones_pool(YPLUS_MODEL);

            evaluate_fitness(actual_pool_clones_size, pool_clones, INDIVIDUAL_PROTECTION, YPLUS_MODEL, false, true, limit_cost? max_cost: 0.0);
            replacement(actual_pool_clones_size);
        }

        evaluate_best_parent(INDIVIDUAL_PROTECTION);
    }
}

double IncrementalGAProtection::fitness() {
    if (number_of_genes > 0) {
        return evaluate_best_parent(INDIVIDUAL_PROTECTION);
    } else {
        return 0.0;
    }
}
