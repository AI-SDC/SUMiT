#include "stdafx.h"
#include "Groups.h"

Groups::Groups(JJData *jjData) {
    this->jjData = jjData;
    
    // Select primary cells and store them
    CellStore *stored_cells = new CellStore(jjData);
    stored_cells->store_selected_cells();
    stored_cells->order_cells_by_largest_weighting();

    cell_constraints_mapping = new CellToConstraints[jjData->ncells];

    for (CellIndex i = 0; i < jjData->ncells; i++) {
        cell_constraints_mapping[i].number_of_constraints = 0;
        cell_constraints_mapping[i].constraint_number = NULL;
    }

    constraints_cell_mapping = new ConstraintsToCell[jjData->nsums];

    for (SumIndex i = 0; i < jjData->nsums; i++) {
        constraints_cell_mapping[i].number_of_cells = 0;
        constraints_cell_mapping[i].half_sum_lower_protection_levels = 0.0;
        constraints_cell_mapping[i].half_sum_upper_protection_levels = 0.0;
        constraints_cell_mapping[i].cell_index = NULL;
    }
    
    // Sort constraints by size
    for (SumIndex i = 0; i < jjData->nsums; i++) {
        for (SumIndex j = 0; j < jjData->nsums; j++) {
            if ((i < j) && (jjData->consistency_eqtns[i].size_of_eqtn > jjData->consistency_eqtns[j].size_of_eqtn)) {
                // Swap i and j
                JJData::ConsistencyEquation temp_constraint;
                temp_constraint.size_of_eqtn = jjData->consistency_eqtns[i].size_of_eqtn;
                temp_constraint.cell_index = jjData->consistency_eqtns[i].cell_index;
                temp_constraint.plus_or_minus = jjData->consistency_eqtns[i].plus_or_minus;
                temp_constraint.RHS = jjData->consistency_eqtns[i].RHS;

                jjData->consistency_eqtns[i].size_of_eqtn = jjData->consistency_eqtns[j].size_of_eqtn;
                jjData->consistency_eqtns[i].cell_index = jjData->consistency_eqtns[j].cell_index;
                jjData->consistency_eqtns[i].plus_or_minus = jjData->consistency_eqtns[j].plus_or_minus;
                jjData->consistency_eqtns[i].RHS = jjData->consistency_eqtns[j].RHS;

                jjData->consistency_eqtns[j].size_of_eqtn = temp_constraint.size_of_eqtn;
                jjData->consistency_eqtns[j].cell_index = temp_constraint.cell_index;
                jjData->consistency_eqtns[j].plus_or_minus = temp_constraint.plus_or_minus;
                jjData->consistency_eqtns[j].RHS = temp_constraint.RHS;
            }
        }
    }

    // Work out how much space is required to store the data
    for (SumIndex i = 0; i < jjData->nsums; i++) {
        CellIndex size = jjData->consistency_eqtns[i].size_of_eqtn;

        for (CellIndex j = 0; j < size; j++) {
            CellIndex c = jjData->consistency_eqtns[i].cell_index[j];

            if (jjData->cells[c].status == 'u') {
                cell_constraints_mapping[c].number_of_constraints++;
                constraints_cell_mapping[i].number_of_cells++;
            }
        }
    }

    // Allocate memory and initialise it
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        if (cell_constraints_mapping[i].number_of_constraints > 0) {
            cell_constraints_mapping[i].constraint_number = new CellIndex[cell_constraints_mapping[i].number_of_constraints];
            cell_constraints_mapping[i].number_of_constraints = 0;
        }
    }

    for (SumIndex i = 0; i < jjData->nsums; i++) {
        if (constraints_cell_mapping[i].number_of_cells > 0) {
            constraints_cell_mapping[i].cell_index = new CellIndex[constraints_cell_mapping[i].number_of_cells];
            constraints_cell_mapping[i].number_of_cells = 0;
        }
    }

    // Fill in the cell/constraint mappings
    for (SumIndex i = 0; i < jjData->nsums; i++) {
        CellIndex size = jjData->consistency_eqtns[i].size_of_eqtn;

        for (CellIndex j = 0; j < size; j++) {
            CellIndex c = jjData->consistency_eqtns[i].cell_index[j];

            if (jjData->cells[c].status == 'u') {
                CellIndex k = cell_constraints_mapping[c].number_of_constraints;
                cell_constraints_mapping[c].constraint_number[k] = i;
                k++;
                cell_constraints_mapping[c].number_of_constraints = k;

                k = constraints_cell_mapping[i].number_of_cells;
                constraints_cell_mapping[i].cell_index[k] = c;
                k++;
                constraints_cell_mapping[i].number_of_cells = k;
            }
        }
    }

    // Calculate half the sum of the protection levels
    for (SumIndex i = 0; i < jjData->nsums; i++) {
        double sum_lp = 0.0;
        double sum_up = 0.0;
        CellIndex size = constraints_cell_mapping[i].number_of_cells;

        if (size > 0) {
            for (CellIndex j = 0; j < size; j++) {
                CellIndex cell = constraints_cell_mapping[i].cell_index[j];
                sum_lp = sum_lp + jjData->cells[cell].lower_protection_level;
                sum_up = sum_up + jjData->cells[cell].upper_protection_level;
            }

            constraints_cell_mapping[i].half_sum_lower_protection_levels = sum_lp / 2.0;
            constraints_cell_mapping[i].half_sum_upper_protection_levels = sum_up / 2.0;
        }
    }

    if (stored_cells->size > 0) {
        if (stored_cells->size < 400) {
            max_groups = 100;
        } else if (stored_cells->size < 800) {
            max_groups = 80;
        } else if (stored_cells->size < 1200) {
            max_groups = 60;
        } else if (stored_cells->size < 1600) {
            max_groups = 40;
        } else if (stored_cells->size < 2000) {
            max_groups = 20;
        } else {
            max_groups = 10;
        }
        max_groups_limit = 200;
        max_group_size = MAX(1, stored_cells->size / max_groups);

        // Allocate group memory
        group = new Groupings[max_groups_limit];

        for (int i = 0; i < max_groups_limit; i++) {
            group[i].size = 0;
            group[i].cell_index = new CellIndex[max_group_size];
        }

        // Assign cells to groups
        CellIndex number_of_cells_assigned = 0;

        for (CellIndex i = 0; i < stored_cells->size; i++) {
            bool assigned = false;
            CellIndex cell = stored_cells->cells[i];

            for (int j = 0; ((j < max_groups_limit) && (! assigned)); j++) {
                if (group[j].size == 0) {
                    group[j].cell_index[0] = cell;
                    group[j].size = 1;
                    number_of_cells_assigned++;
                    assigned = true;
                } else if (group[j].size < max_group_size) {
                    bool common_constraint = false;
                    CellIndex nos_of_constraints = cell_constraints_mapping[cell].number_of_constraints;

                    if (cell_already_in_group(cell, j)) {
                        // Skip the checks as the cell is already in the group
                        common_constraint = true;
                        assigned = true;
                    }

                    for (CellIndex k = 0; ((k < nos_of_constraints) && (! common_constraint)); k++) {
                        CellIndex constraint = cell_constraints_mapping[cell].constraint_number[k];
                        double sum_lp = 0.0;
                        double sum_up = 0.0;
                        CellIndex shared_cell_count = 0;
                        for (CellIndex l = 0; l < group[j].size; l++) {
                            CellIndex cell_in_group = group[j].cell_index[l];

                            if (cell_in_constraint(cell_in_group, constraint)) {
                                sum_lp = sum_lp + jjData->cells[cell_in_group].lower_protection_level;
                                sum_up = sum_up + jjData->cells[cell_in_group].upper_protection_level;
                                shared_cell_count++;
                            }
                        }
                        if (shared_cell_count > 0) {
                            if ((sum_lp + jjData->cells[cell].lower_protection_level > constraints_cell_mapping[constraint].half_sum_lower_protection_levels)
                                    || (sum_up + jjData->cells[cell].upper_protection_level > constraints_cell_mapping[constraint].half_sum_upper_protection_levels)) {
                                common_constraint = true;
                            }
                        }
                    }

                    if (! common_constraint) {
                        group[j].cell_index[group[j].size++] = cell;
                        number_of_cells_assigned++;
                        assigned = true;
                    }
                }
            }
        }

        // Get number of groups        
        number_of_groups = 0;

        for (int i = 0; i < max_groups_limit; i++) {
            if (group[i].size > 0) {
                number_of_groups++;
            }
        }

        // Order cells in their groups
        for (CellIndex grp = 0; grp < number_of_groups; grp++) {
            CellIndex sz = group[grp].size;

            if (sz > 0) {
                for (CellIndex i = 0; i < sz; i++) {
                    for (CellIndex j = 0; j < sz; j++) {
                        if (i < j) {
                            CellIndex cell_i = group[grp].cell_index[i];
                            CellIndex cell_j = group[grp].cell_index[j];

                            if (jjData->cells[cell_i].nominal_value > jjData->cells[cell_j].nominal_value) {
                                group[grp].cell_index[i] = cell_j;
                                group[grp].cell_index[j] = cell_i;
                            }
                        }
                    }
                }
            }
        }

        // Show group sizes
        for (int i = 0; i < max_groups_limit; i++) {
            if (group[i].size > 0) {
                logger->log(4, "Group %d contains %d sensitive cells", i, group[i].size);
            }
        }
    }
    
    delete stored_cells;
}

Groups::~Groups() {
    for (int i = 0; i < max_groups_limit; i++) {
        if (group[i].cell_index != NULL) {
            delete[] group[i].cell_index;
            group[i].cell_index = NULL;
        }
    }

    delete[] group;
}

bool Groups::cell_in_constraint(CellIndex cell, CellIndex constraint) {
    bool rc = false;

    for (CellIndex i = 0; i < constraints_cell_mapping[constraint].number_of_cells; i++) {
        if (cell == constraints_cell_mapping[constraint].cell_index[i]) {
            return (true);
        }
    }

    return rc;
}

bool Groups::cell_already_in_group(CellIndex cell, CellIndex grp) {
    bool rc = false;

    for (CellIndex i = 0; i < group[grp].size; i++) {
        if (cell == group[grp].cell_index[i]) {
            rc = true;
        }
    }

    return rc;
}
