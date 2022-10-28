#include "stdafx.h"
#include <string.h>
#include "GroupedGAProtection.h"
#include "Solver.h"
#include "Groups.h"

GroupedGAProtection::GroupedGAProtection(const char *host, const char *port, const char *injjfilename, const char *outjjfilename, const char *samples_filename, unsigned int seed, int cores, int execution_time, bool run_elimination) : GAProtection(host, port, injjfilename, outjjfilename, samples_filename, seed, cores, execution_time, run_elimination) {
    // Create groups
    Groups *groups = new Groups(jjData);
    number_of_genes = groups->number_of_groups;

    logger->log(3, "%d groups", number_of_genes);

    allocate_pools();

    samples_log = NULL;

    if (number_of_genes > 0) {
        if (debugging) {
            samples_log = new SamplesLog(injjfilename, samples_filename, number_of_genes);
        }

        // Ascending order of group index
        for (int i = 0; i < pool_parent_size; i++) {
            for (GeneIndex j = 0; j < number_of_genes; j++) {
                pool_parent[i].genes[j] = j;
            }
        }

        // Descending order of group index
        for (GeneIndex i = 0; i < number_of_genes; i++) {
            pool_parent[1].genes[i] = number_of_genes - i - 1;
        }

        // Random order of groups
        fill_parent_pool(YPLUS_MODEL);

        evaluate_fitness(pool_parent_size, pool_parent, GROUP_PROTECTION, YPLUS_MODEL, false, true, 0.0);
        evaluate_best_parent(GROUP_PROTECTION);
    }

    delete groups;
}

void GroupedGAProtection::protect(bool limit_cost) {
    logger->log(3, "Grouped protection: %s", injjfilename);

    if (number_of_genes > 0) {
        double max_cost = get_worst_fitness();

        select_for_pool_mating();
        apply_crossover();

        for (int offspring = 0; offspring < POOL_OFFSPRING_SIZE; offspring++) {
            duplicate_clones(offspring);
            apply_mutation();

            int actual_pool_clones_size = grow_clones_pool(YPLUS_MODEL);

            evaluate_fitness(actual_pool_clones_size, pool_clones, GROUP_PROTECTION, YPLUS_MODEL, false, true, limit_cost? max_cost: 0.0);
            replacement(actual_pool_clones_size);
        }

        evaluate_best_parent(GROUP_PROTECTION);
    }
}

double GroupedGAProtection::fitness() {
    if (number_of_genes > 0) {
        return evaluate_best_parent(GROUP_PROTECTION);
    } else {
        return 0.0;
    }
}
