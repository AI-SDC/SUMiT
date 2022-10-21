#include "stdafx.h"
#include <set>
#include <math.h>
#include <string.h>
#include "JJData.h"

JJData::JJData(const char* filename) {
    FILE *ifp;
    int file_id;
    char divider;
    
    name = new char[strlen(filename) + 1];
    strcpy(name, filename);

    int line_number = 0;
    nprotected = 0;
    max_eqn_size = 0;

    if ((ifp = fopen(filename, "r")) == NULL) {
        logger->error(201, "File not found: %s", filename);
    }

    // Line 1 - file type
    line_number++;    
    if (fscanf(ifp, "%d\n", &file_id) != 1) {
        logger->error(202, "Incorrect format at line %d: %s", line_number, filename);
    }

    if (file_id != 0) {
        logger->error(203, "Unknown file type: %s", filename);
    }

    // Line 2 - number of cells
    line_number++;    
    if (fscanf(ifp, "%d\n", &ncells) != 1) {
        logger->error(204, "Incorrect format at line %d: %s", line_number, filename);
    }

    // Cells
    cells = new Cell[ncells];

    for (CellIndex i = 0; i < ncells; i++) {
        line_number++;
        if (fscanf(ifp, " %d %lf %lf %c %lf %lf %lf %lf %lf\n",
                &cells[i].id,
                &cells[i].nominal_value,
                &cells[i].loss_of_information_weight,
                &cells[i].status,
                &cells[i].lower_bound,
                &cells[i].upper_bound,
                &cells[i].lower_protection_level,
                &cells[i].upper_protection_level,
                &cells[i].sliding_protection_level) != 9) {
            logger->error(205, "Incorrect format at line %d: %s", line_number, filename);
        }
        
        cells[i].level = 0;
        map[cells[i].id] = i;
        
        if (cells[i].nominal_value < FLOAT_PRECISION) {
            cells[i].status = 'z';
        }
        
        if (cells[i].status == 'z') {
            nprotected++;
        }
    }

    logger->log(2, "%d cells read: %s", ncells, filename);
    logger->log(2, "%d protected cells", nprotected);

    // Number of equations
    line_number++;
    if (fscanf(ifp, "%d\n", &nsums) != 1) {
        logger->error(208, "Incorrect format at line %d: %s", line_number, filename);
    }

    // Consistency equations
    consistency_eqtns = new ConsistencyEquation[nsums];
    
    int start_of_constraints = line_number;

    for (SumIndex i = 0; i < nsums; i++) {
        line_number++;
        
        if (fscanf(ifp, "%lf ", &consistency_eqtns[i].RHS) != 1) {
            logger->error(209, "Incorrect total format at line %d: %s", line_number, filename);
        }
        
        if (fscanf(ifp, " %d ", &consistency_eqtns[i].size_of_eqtn) != 1) {
            logger->error(210, "Incorrect equation size format at line %d: %s", line_number, filename);
        }
        
        if (consistency_eqtns[i].size_of_eqtn < 2) {
            logger->error(211, "Equation size too small at line %d: %s", line_number, filename);
        }
        
        if ((fscanf(ifp, " %c ", &divider) != 1) || (divider != ':')) {
            logger->error(212, "Incorrect divider format at line %d: %s", line_number, filename);
        }

        consistency_eqtns[i].cell_index = new CellIndex[consistency_eqtns[i].size_of_eqtn];
        consistency_eqtns[i].plus_or_minus = new int[consistency_eqtns[i].size_of_eqtn];
        consistency_eqtns[i].marginal_index = -1;

        for (CellIndex j = 0; j < consistency_eqtns[i].size_of_eqtn; j++) {
            CellID id;
            if (fscanf(ifp, " %d (%d)", &id, &consistency_eqtns[i].plus_or_minus[j]) != 2) {
                logger->error(213, "Incorrect consistency equation format at line %d: %s", line_number, filename);
            }
            consistency_eqtns[i].cell_index[j] = cell_id_to_index(id);
            
            if (consistency_eqtns[i].plus_or_minus[j] < 0) {
                if (consistency_eqtns[i].marginal_index == -1) {
                    consistency_eqtns[i].marginal_index = consistency_eqtns[i].cell_index[j];
                } else {
                    logger->error(222, "Duplicate total in consistency equation at line %d: %s", line_number, filename);
                }
            }
        }
        
        if (consistency_eqtns[i].marginal_index == -1) {
            logger->error(223, "Missing total in consistency equation at line %d: %s", line_number, filename);
        }
    }

    logger->log(2, "%d consistency equations read: %s", nsums, filename);

    line_number = start_of_constraints;
    bool error = false;
    
    for (SumIndex i = 0; i < nsums; i++) {
        line_number++;

        CellIndex size = consistency_eqtns[i].size_of_eqtn;
        
        if (size > max_eqn_size) {
            max_eqn_size = size;
        }

        double sum = 0.0;

        for (CellIndex j = 0; j < size; j++) {
            sum = sum + (double)consistency_eqtns[i].plus_or_minus[j] * cells[consistency_eqtns[i].cell_index[j]].nominal_value;
        }

        if (fabs(sum - consistency_eqtns[i].RHS) > MARGINAL_EQUALITY_TOLERANCE) {
            logger->log(1, "Inconsistent constraint at line %d: constraint index = %d, sum = %lf, RHS = %lf", line_number, i, sum, consistency_eqtns[i].RHS);
            for (CellIndex j = 0; j < size; j++) {
                logger->log(1, "Cell index %d, ID %d, value %lf", consistency_eqtns[i].cell_index[j], cell_index_to_id(consistency_eqtns[i].cell_index[j]), (double)consistency_eqtns[i].plus_or_minus[j] * cells[consistency_eqtns[i].cell_index[j]].nominal_value);
            }
            error = true;
        }
    }
    
    if (error) {
        logger->error(214, "JJ file contains one or more inconsistent constraints: %s", filename);
    }

    // Infer levels
    bool changed;
    
    do {
        changed = false;
        
        for (SumIndex i = 0; i < nsums; i++) {
            ConsistencyEquation* eqtn = &consistency_eqtns[i];
            CellIndex marginal_index = cell_id_to_index(find_marginal_id(eqtn));

            // Level of marginal is one greater than maximum level of underlying cells
            int level = 0;

            for (CellIndex j = 0; j < eqtn->size_of_eqtn; j++) {
                if ((eqtn->cell_index[j] != marginal_index) && (cells[eqtn->cell_index[j]].level > level)) {
                    level = cells[eqtn->cell_index[j]].level;
                }
            }

            level++;

            // Set level of marginal if it is greater than that arising from other consistency equations
            if (level > cells[marginal_index].level) {
                cells[marginal_index].level = level;
                changed = true;
            }
        }
    } while (changed);
    
    nlevels = 0;
    for (CellIndex i = 0; i < ncells; i++) {
        if (cells[i].level > nlevels) {
            nlevels = cells[i].level;
        }
    }
    
    if (ncells > 0) {
        nlevels++;
    }

    logger->log(2, "Maximum consistency equation size %d", max_eqn_size);
    logger->log(2, "%d levels", nlevels);

    fclose(ifp);
}

// Return whether to trace a particular cell
// Edit cell ids to trace
#define TRACE(id) false
//#define TRACE(id) (id == 49) || (id == 24982) || (id == 543) || (id == 7068)

// Create a partitioned JJ file from a parent table and a list of cells
// This constructor takes care of generating marginals and consistency equations for the partition and removes zero cells
// partition_cells is the set of parent cells to include in the partition.  Cells may appear in any order.  Any marginals present are ignored.
// partition_size is the size of the set of partition_cells
// partition_id is a one-based identifier used to identify the partition in error and log messages
JJData::JJData(JJData* parent, CellID* partition_cells, int partition_size, int partition_id) {
    nlevels = parent->nlevels;
    nprotected = 0;
    max_eqn_size = 0;

    // The partition name is one plus the partition index to match partition naming elsewhere in the code
    name = new char[strlen("partition XXXXX") + 1];
    sprintf(name, "partition %d", partition_id);

    // Create an array to hold the nominal values for the cells in the partition
    double* values = new double[parent->ncells];
    for (CellIndex i = 0; i < parent->ncells; i++) {
        values[i] = 0.0;
    }
    
    // Initialise the set of cells in the partition
    for (int i = 0; i < partition_size; i++) {
        CellIndex parent_index = parent->cell_id_to_index(partition_cells[i]);
        
        // Ignore any marginals supplied - we will include any required marginals later on
        // Also ignore protected cells (including those with value zero) as these cannot contribute to cell suppression
        if ((parent->cells[parent_index].level == 0) && (parent->cells[parent_index].status != 'z')) {
            // Values of underlying cells are inherited from parent table and do not change with partitioning
            values[parent_index] = parent->cells[parent_index].nominal_value;
        }
    }
    
    // Iterate through the hierarchy recalculating marginals
    for (int level = 1; level < parent->nlevels; level++) {
        logger->log(5, "Level %d", level);
        
        // Add the direct marginals of the current level to the set of required cells
        for (SumIndex i = 0; i < parent->nsums; i++) {
            ConsistencyEquation* parent_eqtn = &parent->consistency_eqtns[i];
            
            // Only look at marginals for the current level
            if (parent->cells[parent_eqtn->marginal_index].level == level) {
//                CellID marginal_id = parent->cell_index_to_id(parent_eqtn->marginal_index);
                if (TRACE(marginal_id)) {
//                    logger->log(5, "Marginal %d: Level %d, value %lf, parent value %lf", marginal_id, parent->cells[parent_eqtn->marginal_index].level, values[parent_eqtn->marginal_index], parent->cells[parent_eqtn->marginal_index].nominal_value);
                }

                // Run through the consistency equation recalculating the marginal using only the cells that are included in the partition
                double value = 0.0;
                for (CellIndex j = 0; j < parent_eqtn->size_of_eqtn; j++) {
                    // Adjust the value of the marginal if the cell is included in the partition
                    CellIndex parent_index = parent_eqtn->cell_index[j];
                    if (parent_index != parent_eqtn->marginal_index) {
                        // This test is needed to avoid errors accumulating from adding zero cells that do not have an exact zero representation
                        if (values[parent_index] >= FLOAT_PRECISION) {
                            value += values[parent_index];
                            if (TRACE(marginal_id)) {
//                                logger->log(5, "Marginal %d: Adding cell %d, level %d, value %lf, parent value %lf", marginal_id, parent->cell_index_to_id(parent_index), parent->cells[parent_index].level, values[parent_index], parent->cells[parent_index].nominal_value);
                            }
                        } else if (TRACE(marginal_id)) {
//                            logger->log(5, "Marginal %d: Skipping cell %d, level %d, value %lf, parent value %lf", marginal_id, parent->cell_index_to_id(parent_index), parent->cells[parent_index].level, values[parent_index], parent->cells[parent_index].nominal_value);
                        }
                    } 
                }
                
                // Add the marginal to the set of required cells if it has not already been added and it is non-zero (i.e., required)
                if ((values[parent_eqtn->marginal_index] < FLOAT_PRECISION) && (value >= FLOAT_PRECISION)) {
                    values[parent_eqtn->marginal_index] = value;
                    if (TRACE(marginal_id)) {
//                        logger->log(5, "Marginal %d: Assigned value %lf", marginal_id, values[parent_eqtn->marginal_index]);
                    }
                } else {
                    if (fabs(value - values[parent_eqtn->marginal_index]) > MARGINAL_EQUALITY_TOLERANCE) {
                        // Marginals generated from different consistency equations do not match
                        logger->error(215, "Inconsistent marginal values generated for marginal %d  in equation index %d (equation value %lf, stored value %lf)", parent->cell_index_to_id(parent_eqtn->marginal_index), i, value, values[parent_eqtn->marginal_index]);
                    }
                }
            }
        }
    }

    // Determine the number of cells in the partition and the maximum value
    ncells = 0;
    double max_value = 0.0;
    for (CellIndex i = 0; i < parent->ncells; i++) {
        if ((values[i] >= FLOAT_PRECISION)) {
            ncells++;
            
            if (values[i] > max_value) {
                max_value = values[i];
            }
        }
    }
    
    cells = new Cell[ncells];
    
    // Generate the permanent data structure for the required cells
    CellIndex j = 0;
    for (CellIndex i = 0; i < parent->ncells; i++) {
        if ((values[i] >= FLOAT_PRECISION)) {
            cells[j] = parent->cells[i];
            cells[j].nominal_value = values[i];
            cells[j].upper_bound = max_value * 2;
            cells[j].lower_protection_level = values[i] / 10;
            cells[j].upper_protection_level = values[i] / 10;
            map[cells[j].id] = j;
            j++;
        }
    }
    
    delete[] values;

    // Determine the number of consistency equations in the partition
    nsums = 0;
    ConsistencyEquation partition_eqtn;
    for (SumIndex i = 0; i < parent->nsums; i++) {
        if (generate_partition_consistency_equation(parent, i, &partition_eqtn)) {
            delete[] partition_eqtn.cell_index;
            delete[] partition_eqtn.plus_or_minus;
            nsums++;
        }
    }

    // Generate revised consistency equations for the partition
    consistency_eqtns = new ConsistencyEquation[nsums];

    j = 0;
    for (SumIndex i = 0; i < parent->nsums; i++) {
        if (generate_partition_consistency_equation(parent, i, &partition_eqtn)) {
            consistency_eqtns[j++] = partition_eqtn;

            if (partition_eqtn.size_of_eqtn > max_eqn_size) {
                max_eqn_size = partition_eqtn.size_of_eqtn;
            }
        }
    }

    logger->log(3, "Maximum consistency equation size %d", max_eqn_size);
    logger->log(3, "%d levels", nlevels);
}

JJData::~JJData() {
    if (cells != NULL) {
        delete[] cells;
    }

    if (consistency_eqtns != NULL) {
        for (SumIndex i = 0; i < nsums; i++) {
            if (consistency_eqtns[i].cell_index != NULL) {
                delete[] consistency_eqtns[i].cell_index;
            }

            if (consistency_eqtns[i].plus_or_minus != NULL) {
                delete[] consistency_eqtns[i].plus_or_minus;
            }
        }

        delete[] consistency_eqtns;
    }
    
    delete[] name;
}

void JJData::write_jj_file(const char* filename) {
    FILE *ofp;

    if ((ofp = fopen(filename, "w")) == NULL) {
        logger->error(216, "Unable to create file: %s", filename);
    }

    fprintf(ofp, "0\n");
    fprintf(ofp, "%d \n", ncells);

    for (CellIndex i = 0; i < ncells; i++) {
        fprintf(ofp, "%d %f %f %c %f %f %f %f %f\n", cells[i].id, cells[i].nominal_value, cells[i].loss_of_information_weight, cells[i].status, cells[i].lower_bound, cells[i].upper_bound, cells[i].lower_protection_level, cells[i].upper_protection_level, cells[i].sliding_protection_level);
    }

    fprintf(ofp, "%d\n", nsums);

    for (SumIndex i = 0; i < nsums; i++) {
        CellIndex size = consistency_eqtns[i].size_of_eqtn;

        fprintf(ofp, "0 %d :", size);

        for (CellIndex j = 0; j < size; j++) {
            fprintf(ofp, " %d (%d)", cell_index_to_id(consistency_eqtns[i].cell_index[j]), consistency_eqtns[i].plus_or_minus[j]);
        }
        fprintf(ofp, "\n");
    }

    fclose(ofp);
}

void JJData::reset() {
    for (CellIndex i = 0; i < ncells; i++) {
        if (cells[i].status == 'm') {
            cells[i].status = 's';
        }
    }
}

CellIndex JJData::get_number_of_cells() {
    return ncells;
}

CellIndex JJData::get_number_of_primary_cells() {
    CellIndex count = 0;
            
    for (CellIndex i = 0; i < ncells; i++) {
        if (cells[i].status == 'u') {
            count++;
        }
    }
    
    return count;
}

void JJData::recombine(const char* partitioned_jj_filename) {
    JJData* partition = new JJData(partitioned_jj_filename);
    
    for (CellIndex i = 0; i < partition->ncells; i++) {
        CellID id = partition->cells[i].id;

        if (id >= ncells) {
            logger->error(217, "Cell ID %d out of range for recombination: %s", id, partitioned_jj_filename);
        }
        
        if (partition->cells[cell_id_to_index(i)].status == 'm') {
            cells[id].status = 'm';
        }
    }
    
    delete partition;
}

bool JJData::generate_partition_consistency_equation(JJData* parent, SumIndex index, ConsistencyEquation* partition_equation) {
    ConsistencyEquation* parent_eqtn = &parent->consistency_eqtns[index];
    CellIndex marginal_index_in_eqtn = parent->find_marginal_index_in_equation(parent_eqtn);

    // Initialise new partition consistency equation
    partition_equation->RHS = parent_eqtn->RHS;

    CellID* cell_id = new CellID[parent_eqtn->size_of_eqtn];
    int* plus_or_minus = new int[parent_eqtn->size_of_eqtn];
    
    partition_equation->size_of_eqtn = 0;
    for (CellIndex i = 0; i < parent_eqtn->size_of_eqtn; i++) {
        // Include the term if the cell is present in the partition
        CellID id = parent->cell_index_to_id(parent_eqtn->cell_index[i]);
        if ((i == marginal_index_in_eqtn) || (map.count(id) > 0)) {
            cell_id[partition_equation->size_of_eqtn] = id;
            plus_or_minus[partition_equation->size_of_eqtn] = parent_eqtn->plus_or_minus[i];
            partition_equation->size_of_eqtn++;
        }
    }
    
    // Resize arrays to number of cells in the new consistency equation
    if (partition_equation->size_of_eqtn > 1) {
        partition_equation->cell_index = new CellIndex[partition_equation->size_of_eqtn];
        partition_equation->plus_or_minus = new int[partition_equation->size_of_eqtn];

        for (CellIndex i = 0; i < partition_equation->size_of_eqtn; i++) {
            partition_equation->cell_index[i] = cell_id_to_index(cell_id[i]);
            partition_equation->plus_or_minus[i] = plus_or_minus[i];
        }
    }
    
    delete[] cell_id;
    delete[] plus_or_minus;
    
    if (partition_equation->size_of_eqtn > 1) {
        return true;
    } else {
        return false;
    }
}

CellIndex JJData::cell_id_to_index(CellID id) {
    CellIndex index = map[id];

    if ((index < 0) || (index >= ncells)) {
        logger->error(218, "Invalid cell ID %d: %s", id, name);
    }

    return index;
}

CellID JJData::cell_index_to_id(CellIndex index) {
    if ((index < 0) || (index >= ncells)) {
        logger->error(219, "Invalid cell index %d: %s", index, name);
    }

    return cells[index].id;
}

CellID JJData::find_marginal_id(ConsistencyEquation* equation) {
    CellIndex marginal_index_in_eqtn = -1;
    
    for (CellIndex i = 0; i < equation->size_of_eqtn; i++) {
        if (equation->plus_or_minus[i] < 0) {
            marginal_index_in_eqtn = i;
            break;
        }
    }
    
    if (marginal_index_in_eqtn == -1) {
        logger->error(220, "Missing marginal in consistency equation: %s", name);
    }
    
    return cells[equation->cell_index[marginal_index_in_eqtn]].id;
}

CellIndex JJData::find_marginal_index_in_equation(ConsistencyEquation* equation) {
    CellIndex marginal_index_in_eqtn = -1;
    
    for (CellIndex i = 0; i < equation->size_of_eqtn; i++) {
        if (equation->plus_or_minus[i] < 0) {
            marginal_index_in_eqtn = i;
            break;
        }
    }
    
    if (marginal_index_in_eqtn == -1) {
        logger->error(221, "Missing marginal in consistency equation: %s", name);
    }
    
    return marginal_index_in_eqtn;
}
