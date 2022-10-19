#include "Unpicker.h"
#include "UWECellSuppression.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

Unpicker::Unpicker(const char *injjfilename)
{
    jjData = new JJData(injjfilename);

    cell_bounds = new CellBounds[jjData->ncells];

    for (CellIndex i = 0; i < jjData->ncells; i++) {
        if ((jjData->cells[i].status == 'u') ||
            (jjData->cells[i].status == 'm')) {
            cell_bounds[i].lower = 0.0; // jjData->cells[i].lower_bound;
            cell_bounds[i].upper = DBL_MAX; // jjData->cells[i].upper_bound;
        } else {
            cell_bounds[i].lower = jjData->cells[i].nominal_value;
            cell_bounds[i].upper = jjData->cells[i].nominal_value;
        }
    }

    no_consolidated_eqtns = 0;
    nos_processing_eqtns_lower = 0;
    nos_processing_eqtns_upper = 0;

    NumberOfPrimaryCells = 0;
    NumberOfSecondaryCells = 0;
    NumberOfPrimaryCells_ValueKnownExactly = 0;
    NumberOfPrimaryCells_ValueKnownWithinProtection = 0;

    processing_eqtns_lower = NULL;
    processing_eqtns_upper = NULL;
}

Unpicker::~Unpicker(void)
{
    delete[] cell_bounds;

    if (consolidated_eqtns != NULL) {
        for (SumIndex i = 0; i < no_consolidated_eqtns; i++) {
            if (consolidated_eqtns[i].cell_index != NULL) {
                delete[] consolidated_eqtns[i].cell_index;
            }

            if (consolidated_eqtns[i].plus_or_minus != NULL) {
                delete[] consolidated_eqtns[i].plus_or_minus;
            }
        }

        delete[] consolidated_eqtns;

        consolidated_eqtns = NULL;
    }

    if (processing_eqtns_lower != NULL) {
        for (SumIndex i = 0; i < nos_processing_eqtns_lower; i++) {
            if (processing_eqtns_lower[i].cells_lower_to_add != NULL) {
                delete[] processing_eqtns_lower[i].cells_lower_to_add;
            }

            if (processing_eqtns_lower[i].cells_lower_to_sub != NULL) {
                delete[] processing_eqtns_lower[i].cells_lower_to_sub;
            }

            if (processing_eqtns_lower[i].cells_upper_to_add != NULL) {
                delete[] processing_eqtns_lower[i].cells_upper_to_add;
            }

            if (processing_eqtns_lower[i].cells_upper_to_sub != NULL) {
                delete[] processing_eqtns_lower[i].cells_upper_to_sub;
            }
        }

        delete[] processing_eqtns_lower;

        processing_eqtns_lower = NULL;
    }

    if (processing_eqtns_upper != NULL) {
        for (SumIndex i = 0; i < nos_processing_eqtns_upper; i++) {
            if (processing_eqtns_upper[i].cells_lower_to_add != NULL) {
                delete[] processing_eqtns_upper[i].cells_lower_to_add;
            }

            if (processing_eqtns_upper[i].cells_lower_to_sub != NULL) {
                delete[] processing_eqtns_upper[i].cells_lower_to_sub;
            }

            if (processing_eqtns_upper[i].cells_upper_to_add != NULL) {
                delete[] processing_eqtns_upper[i].cells_upper_to_add;
            }

            if (processing_eqtns_upper[i].cells_upper_to_sub != NULL) {
                delete[] processing_eqtns_upper[i].cells_upper_to_sub;
            }
        }

        delete[] processing_eqtns_upper;

        processing_eqtns_upper = NULL;
    }

    delete jjData;
}

void
Unpicker::tidy_up_the_consolidated_equations()
{
    int i;
    int j;

    /* Tidy up the consolidated equations */

    for (i = 0; i < no_consolidated_eqtns; i++) {
        if (consolidated_eqtns[i].RHS < 0) {
            consolidated_eqtns[i].RHS = -1.0 * consolidated_eqtns[i].RHS;

            for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                consolidated_eqtns[i].plus_or_minus[j] =
                    -1 * consolidated_eqtns[i].plus_or_minus[j];
            }
        }
    }
}

void
Unpicker::consolidate_the_consistency_equations()
{
    int i;
    int j;
    int nsuppressed;
    CellIndex cell_no;
    int consolidated_index;

    consolidated_eqtns = NULL;

    // Find the required number of consolidated equations

    no_consolidated_eqtns = 0;

    for (i = 0; i < jjData->nsums; i++) {
        nsuppressed = 0;

        /* Count the number of suppressed cells in the equation */

        for (j = 0; j < jjData->consistency_eqtns[i].size_of_eqtn; j++) {
            cell_no = jjData->consistency_eqtns[i].cell_index[j];

            if ((cell_no >= 0) && (cell_no < jjData->ncells)) {
                if ((jjData->cells[cell_no].status == 'u') ||
                    (jjData->cells[cell_no].status == 'm')) {
                    nsuppressed++;
                }
            }
        }

        /* If there is one or more suppressed cell then we increment the number
         * of consolidated equations */

        if (nsuppressed > 0) {
            no_consolidated_eqtns++;
        }
    }

    // Allocate memory for the consolidated equations

    consolidated_eqtns = new ConsolidatedEquation[no_consolidated_eqtns];

    for (i = 0; i < no_consolidated_eqtns; i++) {
        consolidated_eqtns[i].cell_index = NULL;
        consolidated_eqtns[i].plus_or_minus = NULL;
    }

    // Setup the required number of consolidated equations

    no_consolidated_eqtns = 0;

    for (i = 0; i < jjData->nsums; i++) {
        nsuppressed = 0;

        /* Count the number of suppressed cells in the equation */

        for (j = 0; j < jjData->consistency_eqtns[i].size_of_eqtn; j++) {
            cell_no = jjData->consistency_eqtns[i].cell_index[j];

            if ((cell_no >= 0) && (cell_no < jjData->ncells)) {
                if ((jjData->cells[cell_no].status == 'u') ||
                    (jjData->cells[cell_no].status == 'm')) {
                    nsuppressed++;
                }
            }
        }

        /* If there is one or more suppressed cell then we write it to the
         * consolidated equations */

        if (nsuppressed > 0) {
            consolidated_eqtns[no_consolidated_eqtns].cell_index =
                new CellIndex[nsuppressed];
            consolidated_eqtns[no_consolidated_eqtns].plus_or_minus =
                new int[nsuppressed];

            consolidated_eqtns[no_consolidated_eqtns].RHS =
                jjData->consistency_eqtns[i].RHS;

            consolidated_index = 0;

            for (j = 0; j < jjData->consistency_eqtns[i].size_of_eqtn; j++) {
                cell_no = jjData->consistency_eqtns[i].cell_index[j];

                if ((cell_no >= 0) && (cell_no < jjData->ncells)) {
                    if ((jjData->cells[cell_no].status == 'u') ||
                        (jjData->cells[cell_no].status == 'm')) {
                        consolidated_eqtns[no_consolidated_eqtns]
                            .cell_index[consolidated_index] = cell_no;
                        consolidated_eqtns[no_consolidated_eqtns]
                            .plus_or_minus[consolidated_index] =
                            jjData->consistency_eqtns[i].plus_or_minus[j];
                        consolidated_index++;
                        consolidated_eqtns[no_consolidated_eqtns].size_of_eqtn =
                            consolidated_index;
                    } else {
                        consolidated_eqtns[no_consolidated_eqtns].RHS =
                            consolidated_eqtns[no_consolidated_eqtns].RHS -
                            (jjData->consistency_eqtns[i].plus_or_minus[j] *
                             jjData->cells[cell_no].nominal_value);
                    }
                }
            }

            no_consolidated_eqtns++;
        }
    }

    tidy_up_the_consolidated_equations();
}

int
Unpicker::simplify_the_consistency_equations()
{
    int i;
    int j;
    int k;
    int nos_simplfied;
    int cell_no;

    nos_simplfied = 0;

    for (i = 0; i < no_consolidated_eqtns; i++) {
        switch (consolidated_eqtns[i].size_of_eqtn) {
            case 0:
                break;

            case 1:
                cell_no = consolidated_eqtns[i].cell_index[0];

                if (fabs(cell_bounds[cell_no].upper -
                         cell_bounds[cell_no].lower) < FLOAT_PRECISION) {
                    consolidated_eqtns[i].size_of_eqtn = 0;
                    nos_simplfied++;
                }
                break;

            default:
                for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                    cell_no = consolidated_eqtns[i].cell_index[j];

                    if (fabs(cell_bounds[cell_no].upper -
                             cell_bounds[cell_no].lower) < FLOAT_PRECISION) {
                        /* Remove this cell from the equation as it's value is
                         * known */

                        if (consolidated_eqtns[i].plus_or_minus[j] == 1) {
                            consolidated_eqtns[i].RHS =
                                consolidated_eqtns[i].RHS -
                                cell_bounds[cell_no].upper;
                        } else {
                            consolidated_eqtns[i].RHS =
                                consolidated_eqtns[i].RHS +
                                cell_bounds[cell_no].upper;
                        }

                        for (k = j + 1; k < consolidated_eqtns[i].size_of_eqtn;
                             k++) {
                            consolidated_eqtns[i].cell_index[k - 1] =
                                consolidated_eqtns[i].cell_index[k];
                            consolidated_eqtns[i].plus_or_minus[k - 1] =
                                consolidated_eqtns[i].plus_or_minus[k];
                        }
                        consolidated_eqtns[i].size_of_eqtn--;
                        nos_simplfied++;
                    }
                }
                break;
        }
    }
    tidy_up_the_consolidated_equations();
    return (nos_simplfied);
}

bool
Unpicker::set_initial_lower_and_upper_bounds()
{
    bool rc;
    int i;
    int j;
    int no_plus;
    int cell_no;

    rc = true;

    for (i = 0; i < no_consolidated_eqtns; i++) {
        no_plus = 0;

        switch (consolidated_eqtns[i].size_of_eqtn) {
            case 0:
                /* Something has gone wrong */
                rc = false;
                break;

            case 1:
                /* Simplest case, we have a solution */

                cell_no = consolidated_eqtns[i].cell_index[0];
                cell_bounds[cell_no].lower = consolidated_eqtns[i].RHS;
                cell_bounds[cell_no].upper = consolidated_eqtns[i].RHS;
                break;

            default:
                /* See if they are all adds... if they are we can get upper
                 * bounds */

                no_plus = 0;

                for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                    if (consolidated_eqtns[i].plus_or_minus[j] == 1) {
                        no_plus++;
                    }
                }

                if (no_plus == consolidated_eqtns[i].size_of_eqtn) {
                    /* We can give upper bounds */

                    for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                        cell_no = consolidated_eqtns[i].cell_index[j];

                        if (cell_bounds[cell_no].lower < 0.0) {
                            cell_bounds[cell_no].lower = 0.0;
                        }

                        if (cell_bounds[cell_no].upper >
                            consolidated_eqtns[i].RHS) {
                            cell_bounds[cell_no].upper =
                                consolidated_eqtns[i].RHS;
                        }
                    }
                }
                break;
        }
    }
    return rc;
}

void
Unpicker::evaluate_exposure()
{
    NumberOfPrimaryCells = 0;
    NumberOfSecondaryCells = 0;
    NumberOfPrimaryCells_ValueKnownExactly = 0;
    NumberOfPrimaryCells_ValueKnownWithinProtection = 0;

    for (int i = 0; i < jjData->ncells; i++) {
        if (jjData->cells[i].status == 'u') {
            NumberOfPrimaryCells++;

            double temp = jjData->cells[i].nominal_value -
                jjData->cells[i].lower_protection_level;

            if (temp < 0.0) {
                temp = FLOAT_PRECISION;
            }

            if (fabs(cell_bounds[i].upper - cell_bounds[i].lower) <
                FLOAT_PRECISION) {
                if (fabs(cell_bounds[i].lower -
                         jjData->cells[i].nominal_value) < FLOAT_PRECISION) {
                    NumberOfPrimaryCells_ValueKnownExactly++;
                } else {
                    // logger->error(1, "Unpicker inconsistency: cell %d
                    // calculated to be %lf when true value is %lf", i,
                    // cell_bounds[i].lower, jjData->cells[i].nominal_value);
                }
            } else if (cell_bounds[i].lower > temp) {
                NumberOfPrimaryCells_ValueKnownWithinProtection++;
            } else if (cell_bounds[i].upper < jjData->cells[i].nominal_value +
                           jjData->cells[i].upper_protection_level) {
                NumberOfPrimaryCells_ValueKnownWithinProtection++;
            } else if (jjData->cells[i].sliding_protection_level > 0) {
                if ((cell_bounds[i].upper - cell_bounds[i].lower) <
                    jjData->cells[i].sliding_protection_level) {
                    NumberOfPrimaryCells_ValueKnownWithinProtection++;
                }
            }
        } else if (jjData->cells[i].status == 'm') {
            NumberOfSecondaryCells++;
        }
    }
}

void
Unpicker::print_exact_exposure(const char *filename)
{
    FILE *ofp;

    if ((ofp = fopen(filename, "w")) != NULL) {
        for (CellIndex i = 0; i < jjData->ncells; i++) {
            if (jjData->cells[i].status == 'u') {
                if (fabs(cell_bounds[i].upper - cell_bounds[i].lower) <
                    FLOAT_PRECISION) {
                    if (fabs(cell_bounds[i].lower -
                             jjData->cells[i].nominal_value) <
                        FLOAT_PRECISION) {
                        fprintf(ofp, "%d\n", i);
                    } else {
                        // logger->error(1, "Unpicker inconsistency: cell %d
                        // calculated to be %lf when true value is %lf", i,
                        // cell_bounds[i].lower,
                        // jjData->cells[i].nominal_value);
                    }
                }
            }
        }

        fclose(ofp);
    }
}

void
Unpicker::print_partial_exposure(const char *filename)
{
    FILE *ofp;

    if ((ofp = fopen(filename, "w")) != NULL) {
        for (CellIndex i = 0; i < jjData->ncells; i++) {
            if (jjData->cells[i].status == 'u') {
                bool ValueKnownWithinProtection = false;

                double temp = jjData->cells[i].nominal_value -
                    jjData->cells[i].lower_protection_level;

                if (temp < 0.0) {
                    temp = FLOAT_PRECISION;
                }

                if (cell_bounds[i].lower > temp) {
                    ValueKnownWithinProtection = true;
                } else if (cell_bounds[i].upper <
                           jjData->cells[i].nominal_value +
                               jjData->cells[i].upper_protection_level) {
                    ValueKnownWithinProtection = true;
                } else if (jjData->cells[i].sliding_protection_level > 0) {
                    if ((cell_bounds[i].upper - cell_bounds[i].lower) <
                        jjData->cells[i].sliding_protection_level) {
                        ValueKnownWithinProtection = true;
                    }
                }

                if (ValueKnownWithinProtection) {
                    fprintf(ofp, "%d\n", i);
                }
            }
        }

        fclose(ofp);
    }
}

void
Unpicker::release_processing_eqtns_memory()
{
    int i;

    if (processing_eqtns_lower != NULL) {
        for (i = 0; i < nos_processing_eqtns_lower; i++) {
            if (processing_eqtns_lower[i].cells_lower_to_add != NULL) {
                delete[] processing_eqtns_lower[i].cells_lower_to_add;
            }

            if (processing_eqtns_lower[i].cells_lower_to_sub != NULL) {
                delete[] processing_eqtns_lower[i].cells_lower_to_sub;
            }

            if (processing_eqtns_lower[i].cells_upper_to_add != NULL) {
                delete[] processing_eqtns_lower[i].cells_upper_to_add;
            }

            if (processing_eqtns_lower[i].cells_upper_to_sub != NULL) {
                delete[] processing_eqtns_lower[i].cells_upper_to_sub;
            }
        }

        delete[] processing_eqtns_lower;

        processing_eqtns_lower = NULL;
    }

    if (processing_eqtns_upper != NULL) {
        for (i = 0; i < nos_processing_eqtns_upper; i++) {
            if (processing_eqtns_upper[i].cells_lower_to_add != NULL) {
                delete[] processing_eqtns_upper[i].cells_lower_to_add;
            }

            if (processing_eqtns_upper[i].cells_lower_to_sub != NULL) {
                delete[] processing_eqtns_upper[i].cells_lower_to_sub;
            }

            if (processing_eqtns_upper[i].cells_upper_to_add != NULL) {
                delete[] processing_eqtns_upper[i].cells_upper_to_add;
            }

            if (processing_eqtns_upper[i].cells_upper_to_sub != NULL) {
                delete[] processing_eqtns_upper[i].cells_upper_to_sub;
            }
        }

        delete[] processing_eqtns_upper;

        processing_eqtns_upper = NULL;
    }
}

bool
Unpicker::improve_lower_and_upper_bounds()
{
    bool rc;
    int i;
    int j;
    int k;
    int sz;
    int cellno;
    int cell;
    int idx;
    int dest_cell;
    int dest_plus_minus;
    double consolidated_RHS;
    double result;
    int required_eqtns;
    int lower_bound_improvements;
    int upper_bound_improvements;

    rc = false;
    lower_bound_improvements = 0;
    upper_bound_improvements = 0;

    for (i = 0; i < no_consolidated_eqtns; i++) {
        required_eqtns = consolidated_eqtns[i].size_of_eqtn;

        /////////////////////
        // Allocate memory //
        /////////////////////

        processing_eqtns_lower = new ProcessingEquation[required_eqtns];
        processing_eqtns_upper = new ProcessingEquation[required_eqtns];

        /////////////////////////////////////////
        // Initialise the processing equations //
        /////////////////////////////////////////

        nos_processing_eqtns_lower = 0;
        nos_processing_eqtns_upper = 0;

        for (j = 0; j < required_eqtns; j++) {
            processing_eqtns_lower[j].no_lower_adds = 0;
            processing_eqtns_lower[j].no_lower_subs = 0;
            processing_eqtns_lower[j].no_upper_adds = 0;
            processing_eqtns_lower[j].no_upper_subs = 0;
            processing_eqtns_lower[j].cells_lower_to_add = NULL;
            processing_eqtns_lower[j].cells_lower_to_sub = NULL;
            processing_eqtns_lower[j].cells_upper_to_add = NULL;
            processing_eqtns_lower[j].cells_upper_to_sub = NULL;

            processing_eqtns_upper[j].no_lower_adds = 0;
            processing_eqtns_upper[j].no_lower_subs = 0;
            processing_eqtns_upper[j].no_upper_adds = 0;
            processing_eqtns_upper[j].no_upper_subs = 0;
            processing_eqtns_upper[j].cells_lower_to_add = NULL;
            processing_eqtns_upper[j].cells_lower_to_sub = NULL;
            processing_eqtns_upper[j].cells_upper_to_add = NULL;
            processing_eqtns_upper[j].cells_upper_to_sub = NULL;
        }

        sz = consolidated_eqtns[i].size_of_eqtn;

        if (sz > 0) {
            ////////////////////////////////////
            // Setup the processing equations //
            ////////////////////////////////////

            consolidated_RHS = consolidated_eqtns[i].RHS;

            for (j = 0; j < sz; j++) {
                // Allocate memory for processing equations

                processing_eqtns_lower[nos_processing_eqtns_lower]
                    .cells_lower_to_add = new int[sz];
                processing_eqtns_lower[nos_processing_eqtns_lower]
                    .cells_lower_to_sub = new int[sz];
                processing_eqtns_lower[nos_processing_eqtns_lower]
                    .cells_upper_to_add = new int[sz];
                processing_eqtns_lower[nos_processing_eqtns_lower]
                    .cells_upper_to_sub = new int[sz];

                processing_eqtns_upper[nos_processing_eqtns_upper]
                    .cells_lower_to_add = new int[sz];
                processing_eqtns_upper[nos_processing_eqtns_upper]
                    .cells_lower_to_sub = new int[sz];
                processing_eqtns_upper[nos_processing_eqtns_upper]
                    .cells_upper_to_add = new int[sz];
                processing_eqtns_upper[nos_processing_eqtns_upper]
                    .cells_upper_to_sub = new int[sz];

                // Setup processing equations

                dest_cell = consolidated_eqtns[i].cell_index[j];
                dest_plus_minus = consolidated_eqtns[i].plus_or_minus[j];

                processing_eqtns_lower[nos_processing_eqtns_lower].cellnumber =
                    dest_cell;
                processing_eqtns_upper[nos_processing_eqtns_upper].cellnumber =
                    dest_cell;

                if (dest_plus_minus == 1) {
                    // The cell we are looking at is +ve

                    processing_eqtns_lower[nos_processing_eqtns_lower].RHS =
                        consolidated_RHS;
                    processing_eqtns_upper[nos_processing_eqtns_upper].RHS =
                        consolidated_RHS;

                    for (k = 0; k < sz; k++) {
                        if (k != j) {
                            // Get all the other cells in the consolidated
                            // equation and put them in the right place

                            if (consolidated_eqtns[i].plus_or_minus[k] == 1) {
                                // Set up lower calculation

                                idx = processing_eqtns_lower
                                          [nos_processing_eqtns_lower]
                                              .no_upper_subs;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .cells_upper_to_sub[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .no_upper_subs = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper
                                          [nos_processing_eqtns_upper]
                                              .no_lower_subs;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .cells_lower_to_sub[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .no_lower_subs = idx;
                            } else {
                                // Set up lower calculation

                                idx = processing_eqtns_lower
                                          [nos_processing_eqtns_lower]
                                              .no_lower_adds;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .cells_lower_to_add[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .no_lower_adds = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper
                                          [nos_processing_eqtns_upper]
                                              .no_upper_adds;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .cells_upper_to_add[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .no_upper_adds = idx;
                            }
                        }
                    }
                } else {
                    // The cell we are looking at is -ve

                    processing_eqtns_lower[nos_processing_eqtns_lower].RHS =
                        -consolidated_RHS;
                    processing_eqtns_upper[nos_processing_eqtns_upper].RHS =
                        -consolidated_RHS;

                    for (k = 0; k < sz; k++) {
                        if (k != j) {
                            // Get all the other cells in the consolidated
                            // equation and put them in the right place

                            if (consolidated_eqtns[i].plus_or_minus[k] == 1) {
                                // Set up lower calculation

                                idx = processing_eqtns_lower
                                          [nos_processing_eqtns_lower]
                                              .no_lower_adds;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .cells_lower_to_add[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .no_lower_adds = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper
                                          [nos_processing_eqtns_upper]
                                              .no_upper_adds;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .cells_upper_to_add[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .no_upper_adds = idx;
                            } else {
                                // Set up lower calculation

                                idx = processing_eqtns_lower
                                          [nos_processing_eqtns_lower]
                                              .no_upper_subs;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .cells_upper_to_sub[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_lower
                                    [nos_processing_eqtns_lower]
                                        .no_upper_subs = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper
                                          [nos_processing_eqtns_upper]
                                              .no_lower_subs;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .cells_lower_to_sub[idx] =
                                    consolidated_eqtns[i].cell_index[k];
                                idx++;
                                processing_eqtns_upper
                                    [nos_processing_eqtns_upper]
                                        .no_lower_subs = idx;
                            }
                        }
                    }
                }

                nos_processing_eqtns_lower++;
                nos_processing_eqtns_upper++;
            }

            ////////////////////////////////////
            // Solve the processing equations //
            ////////////////////////////////////

            // Improve lower bounds

            for (k = 0; k < nos_processing_eqtns_lower; k++) {
                result = processing_eqtns_lower[k].RHS;
                cellno = processing_eqtns_lower[k].cellnumber;

                sz = processing_eqtns_lower[k].no_lower_adds;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_lower[k].cells_lower_to_add[j];
                        result = result + cell_bounds[cell].lower;
                    }
                }

                sz = processing_eqtns_lower[k].no_upper_adds;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_lower[k].cells_upper_to_add[j];
                        result = result + cell_bounds[cell].upper;
                    }
                }

                sz = processing_eqtns_lower[k].no_lower_subs;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_lower[k].cells_lower_to_sub[j];
                        result = result - cell_bounds[cell].lower;
                    }
                }

                sz = processing_eqtns_lower[k].no_upper_subs;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_lower[k].cells_upper_to_sub[j];
                        result = result - cell_bounds[cell].upper;
                    }
                }

                if (cell_bounds[cellno].lower < result) {
                    cell_bounds[cellno].lower = result;
                    lower_bound_improvements++;
                    rc = true;
                }
            }

            // Improve upper bounds

            for (k = 0; k < nos_processing_eqtns_upper; k++) {
                result = processing_eqtns_upper[k].RHS;
                cellno = processing_eqtns_upper[k].cellnumber;

                sz = processing_eqtns_upper[k].no_lower_adds;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_upper[k].cells_lower_to_add[j];
                        result = result + cell_bounds[cell].lower;
                    }
                }

                sz = processing_eqtns_upper[k].no_upper_adds;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_upper[k].cells_upper_to_add[j];
                        result = result + cell_bounds[cell].upper;
                    }
                }

                sz = processing_eqtns_upper[k].no_lower_subs;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_upper[k].cells_lower_to_sub[j];
                        result = result - cell_bounds[cell].lower;
                    }
                }

                sz = processing_eqtns_upper[k].no_upper_subs;

                if (sz > 0) {
                    for (j = 0; j < sz; j++) {
                        cell = processing_eqtns_upper[k].cells_upper_to_sub[j];
                        result = result - cell_bounds[cell].upper;
                    }
                }

                if (cell_bounds[cellno].upper > result) {
                    cell_bounds[cellno].upper = result;
                    upper_bound_improvements++;
                    rc = true;
                }
            }
        }

        ////////////////////
        // Release memory //
        ////////////////////

        release_processing_eqtns_memory();
    }

    return rc;
}

void
Unpicker::Attack()
{
    int iterations;

    consolidate_the_consistency_equations();

    if (set_initial_lower_and_upper_bounds()) {
        while (simplify_the_consistency_equations() > 0) {
            set_initial_lower_and_upper_bounds();
        }

        iterations = 0;

        while ((improve_lower_and_upper_bounds()) && (iterations < 10)) {
            iterations++;
        }
    }

    evaluate_exposure();
}

int
Unpicker::GetNumberOfCells()
{
    return jjData->ncells;
}

int
Unpicker::GetNumberOfPrimaryCells()
{
    return NumberOfPrimaryCells;
}

int
Unpicker::GetNumberOfSecondaryCells()
{
    return NumberOfSecondaryCells;
}

int
Unpicker::GetNumberOfPrimaryCells_ValueKnownExactly()
{
    return NumberOfPrimaryCells_ValueKnownExactly;
}

int
Unpicker::GetNumberOfPrimaryCells_ValueKnownWithinProtection()
{
    return NumberOfPrimaryCells_ValueKnownWithinProtection;
}
