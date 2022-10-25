#include <stdafx.h>
#include <time.h>
#include <JJData.h>
#include <CellStore.h>
#include <Groups.h>
#include <UWECellSuppression.h>
#include "Solver.h"

Solver::Solver(const char *injjfilename, bool group_protection) {
    jjData = new JJData(injjfilename);

    if (group_protection) {
        // Create groups
        groups = new Groups(jjData);
        number_of_groups = groups->number_of_groups;
    } else {
        groups = NULL;
        number_of_groups = jjData->get_number_of_primary_cells();
    }
    
    YminusOffset = 0;
    YplusOffset = jjData->ncells;

#if SOLVER == CPLEX
    si = new OsiCpxSolverInterface;
    logger->log(1, "Using CPLEX");
#elif SOLVER == GLPK
    si = new OsiGlpkSolverInterface;
    logger->log(1, "Using GLPK");
#else
    si = new OsiClpSolverInterface;
    logger->log(1, "Using CLP");
#endif
}

Solver::~Solver() {
    delete si;
    
    if (groups) {
        delete groups;
    }
    
    delete jjData;
}

double* Solver::run_individual_protection(const char* perm_filename, int model_type, double max_cost, int* costs_size) {
    CellIndex* ordered_cells = read_permutation_file(perm_filename);
    
    jjData->reset();

    allocate_coin_memory();

    si->messageHandler()->setLogLevel((logger->getLevel() > 6)? 4: 0);
    si->loadProblem(*matrixA, varLB, varUB, objCoeffs, rowLB, rowUB);
    si->setIntParam(OsiMaxNumIteration, 1000000);
    si->initialSolve();
    
    double* costs = new double[number_of_groups];
    
    *costs_size = 0;
    
    for (CellIndex i = 0; i < number_of_groups; i++) {
        CellIndex cell = ordered_cells[i];

        time(&current_seconds);

        // Set objective function sense (1 for min (default), -1 for max,)
        si->setObjSense(1);

        // Y minus
        if ((model_type == FULL_MODEL) || (model_type == YMINUS_MODEL)) {
            logger->log(5, "Solving Y minus for cell %d", cell);
            si->setColBounds(YminusOffset + cell, jjData->cells[cell].lower_protection_level + 0.1, MAX(jjData->cells[cell].nominal_value, jjData->cells[cell].lower_protection_level + 0.1));
            si->setColBounds(YplusOffset + cell, 0.0, 0.0);
            si->resolve();
            logModel();
            
            for (CellIndex j = 0; j < jjData->ncells; j++) {
                logger->log(5, "%d %lf %lf", j, si->getColSolution()[YminusOffset + j], si->getColSolution()[YplusOffset + j]);
                if ((si->getColSolution()[YminusOffset + j] + si->getColSolution()[YplusOffset + j]) > 0.01) {
                    if (jjData->cells[j].status == 's') {
                        logger->log(5, "Suppress secondary cell %d", j);
                        jjData->cells[j].status = 'm';
                        
                         // Set primary cell coefficient to 0
                        si->setObjCoeff(YminusOffset + j, 0.0);
                        si->setObjCoeff(YplusOffset + j, 0.0);
                    }
                }
            }
        }
        
        // Y plus
        if ((model_type == FULL_MODEL) || (model_type == YPLUS_MODEL)) {
            logger->log(5, "Solving Y plus for cell %d", cell);
            si->setColBounds(YminusOffset + cell, 0.0, 0.0);
            si->setColBounds(YplusOffset + cell, jjData->cells[cell].upper_protection_level + 0.1, MAX(jjData->cells[cell].nominal_value, jjData->cells[cell].upper_protection_level + 0.1));
            si->resolve();
            logModel();

            for (CellIndex j = 0; j < jjData->ncells; j++) {
                logger->log(5, "%d %lf %lf", j, si->getColSolution()[YminusOffset + j], si->getColSolution()[YplusOffset + j]);
                if ((si->getColSolution()[YminusOffset + j] + si->getColSolution()[YplusOffset + j]) > 0.01) {
                    if (jjData->cells[j].status == 's') {
                        logger->log(5, "Suppress secondary cell %d", j);
                        jjData->cells[j].status = 'm';

                         // Set primary cell coefficient to 0
                        si->setObjCoeff(YminusOffset + j, 0.0);
                        si->setObjCoeff(YplusOffset + j, 0.0);
                    }
                }
            }
        }
        
        // Reset lower and upper variable bounds
        si->setColBounds(YminusOffset + cell, 0.0, jjData->cells[cell].nominal_value);
        si->setColBounds(YplusOffset + cell, 0.0, jjData->cells[cell].nominal_value);
        
        // Note cost
        costs[i] = get_cost();
        (*costs_size)++;
        
        // Terminate early if cost limit specified and reached
        if ((fabs(max_cost) >= FLOAT_PRECISION) && (costs[i] >= max_cost)) {
            break;
        }
    }
    
    delete[] ordered_cells;

    release_coin_memory();
    
    return costs;
}

double* Solver::run_group_protection(const char* perm_filename, int model_type, double max_cost, int* costs_size) {
    int* ordered_groups = read_permutation_file(perm_filename);
    
    jjData->reset();

    allocate_coin_memory();

    si->messageHandler()->setLogLevel((logger->getLevel() > 6)? 4: 0);
    si->loadProblem(*matrixA, varLB, varUB, objCoeffs, rowLB, rowUB);
    si->setIntParam(OsiMaxNumIteration, 1000000);
    si->initialSolve();

    double* costs = new double[number_of_groups];

    *costs_size = 0;
    
    for (int i = 0; i < number_of_groups; i++) {
        int grp = ordered_groups[i];

        CellIndex size = groups->group[grp].size;

        if (size > 0) {
            // Set objective function sense (1 for min (default), -1 for max,)
            si->setObjSense(1);

            // Y minus
            if ((model_type == FULL_MODEL) || (model_type == YMINUS_MODEL)) {
                for (CellIndex j = 0; j < size; j++) {
                    CellIndex cell = groups->group[grp].cell_index[j];
                    si->setColBounds(YminusOffset + cell, jjData->cells[cell].lower_protection_level + 0.1, MAX(jjData->cells[cell].nominal_value, jjData->cells[cell].lower_protection_level + 0.1));
                    si->setColBounds(YplusOffset + cell, 0.0, 0.0);
                }

                logger->log(5, "Solving Y minus for group %d", grp);
                si->resolve();
                logModel();

                for (CellIndex j = 0; j < jjData->ncells; j++) {
                    if ((si->getColSolution()[YminusOffset + j] + si->getColSolution()[YplusOffset + j]) > 0.01) {
                        if (jjData->cells[j].status == 's') {
                            logger->log(5, "Suppress secondary cell %d", j);
                            jjData->cells[j].status = 'm';
                            // Set primary cell coefficient to 0
                            si->setObjCoeff(YminusOffset + j, 0.0);
                            si->setObjCoeff(YplusOffset + j, 0.0);
                        }
                    }
                }
            }

            // Y plus
            if ((model_type == FULL_MODEL) || (model_type == YPLUS_MODEL)) {
                for (CellIndex j = 0; j < size; j++) {
                    CellIndex cell = groups->group[grp].cell_index[j];
                    si->setColBounds(YminusOffset + cell, 0.0, 0.0);
                    si->setColBounds(YplusOffset + cell, jjData->cells[cell].upper_protection_level + 0.1, MAX(jjData->cells[cell].nominal_value, jjData->cells[cell].upper_protection_level + 0.1));
                }

                logger->log(5, "Solving Y plus for group %d", grp);
                si->resolve();
                logModel();

                for (CellIndex j = 0; j < jjData->ncells; j++) {
                    if ((si->getColSolution()[YminusOffset + j] + si->getColSolution()[YplusOffset + j]) > 0.01) {
                        if (jjData->cells[j].status == 's') {
                            logger->log(5, "Suppress secondary cell %d", j);
                            jjData->cells[j].status = 'm';
                            // Set primary cell coefficient to 0
                            si->setObjCoeff(YminusOffset + j, 0.0);
                            si->setObjCoeff(YplusOffset + j, 0.0);
                        }
                    }
                }
            }
            
            // Reset lower and upper variable bounds
            for (CellIndex j = 0; j < size; j++) {
                CellIndex cell = groups->group[grp].cell_index[j];
                si->setColBounds(YminusOffset + cell, 0.0, jjData->cells[cell].nominal_value);
                si->setColBounds(YplusOffset + cell, 0.0, jjData->cells[cell].nominal_value);
            }
        }
        
        // Note cost
        costs[i] = get_cost();
        (*costs_size)++;
        
        // Terminate early if cost limit specified and reached
        if ((fabs(max_cost) >= FLOAT_PRECISION) && (costs[i] >= max_cost)) {
            break;
        }
    }
    
    delete[] ordered_groups;

    release_coin_memory();
    
    return costs;
}

int Solver::get_number_of_groups() {
    return number_of_groups;
}

double Solver::get_cost() {
    double cost = 0.0;
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        if ((jjData->cells[i].status == 'u') || (jjData->cells[i].status == 'm')) {
            cost = cost + jjData->cells[i].loss_of_information_weight;
        }
    }
    
    return cost;
}

int* Solver::read_permutation_file(const char* filename) {
    FILE *ifp;
    
    if ((ifp = fopen(filename, "r")) == NULL) {
        logger->error(303, "Permutation file not found: %s", filename);
    }

    // Check the permutation size
    int size = 0;
    bool ok = true;
    do {
        int index;
        if (fscanf(ifp, "%d\n",  &index) == 1) {
            size++;
        } else {
            ok = false;
        }
    } while (ok);

    if (size != number_of_groups) {
        logger->error(304, "Permutation size (%d) does not match number of groups (%d)", size, number_of_groups);
    }

    // Read the permutation
    rewind(ifp);
    
    int* permutation = new int[size];

    int line = 0;
    for (int i = 0; i < size; i++) {
        if (fscanf(ifp, "%d\n",  &permutation[i]) != 1) {
            logger->error(305, "Error reading permutation file at line %d: %s", line, filename);
        }
        
        line++;
    }

    fclose(ifp);
    
    return permutation;
}

void Solver::write_cost_file(const char* filename, double* costs, int size) {
    FILE *ofp;

    if (*filename != '\0') {
        if ((ofp = fopen(filename, "w")) == NULL) {
            logger->error(302, "Unable to create file: %s", filename);
        }

        for (int i = 0; i < size; i++) {
            fprintf(ofp, "%lf\n", costs[i]);
        }

        fclose(ofp);
    }
}

void Solver::write_jj_file(const char* filename) {
    jjData->write_jj_file(filename);
}

void Solver::allocate_coin_memory() {
    int number_of_elements = 0;

    for (CellIndex i = 0; i < jjData->nsums; i++) {
        number_of_elements = number_of_elements + 2 * jjData->consistency_eqtns[i].size_of_eqtn;
    }

    varLB = new double[2 * jjData->ncells];
    varUB = new double[2 * jjData->ncells];
    objCoeffs = new double[2 * jjData->ncells];
    rowLB = new double[jjData->nsums];
    rowUB = new double[jjData->nsums];
    row_indices = new int[number_of_elements];
    col_indices = new int[number_of_elements];
    elements = new double[number_of_elements];

    int element = 0;

    for (SumIndex i = 0; i < jjData->nsums; i++) {
        for (CellIndex j = 0; j < jjData->consistency_eqtns[i].size_of_eqtn; j++) {
            CellIndex cell = jjData->consistency_eqtns[i].cell_index[j];

            row_indices[element] = i;
            col_indices[element] = YminusOffset + cell;
            elements[element] = -1.0 * (double)jjData->consistency_eqtns[i].plus_or_minus[j];
            element++;

            row_indices[element] = i;
            col_indices[element] = YplusOffset + cell;
            elements[element] = (double)jjData->consistency_eqtns[i].plus_or_minus[j];
            element++;
        }
    }

    for (CellIndex i = 0; i < jjData->ncells; i++) {
        varLB[YminusOffset + i] = 0.0;
        varLB[YplusOffset + i] = 0.0;

        if (jjData->cells[i].status == 'z') {
            varUB[YminusOffset + i] = 0.0;
            varUB[YplusOffset + i] = 0.0;
        } else {
            varUB[YminusOffset + i] = jjData->cells[i].nominal_value;
            varUB[YplusOffset + i] = jjData->cells[i].nominal_value;
        }

        if ((jjData->cells[i].status == 'u') || (jjData->cells[i].status == 'm')) {
            objCoeffs[YminusOffset + i] = 0.0;
            objCoeffs[YplusOffset + i] = 0.0;
        } else {
            objCoeffs[YminusOffset + i] = jjData->cells[i].loss_of_information_weight;
            objCoeffs[YplusOffset + i] = jjData->cells[i].loss_of_information_weight;
        }
    }

    for (CellIndex i = 0; i < jjData->nsums; i++) {
        rowLB[i] = jjData->consistency_eqtns[i].RHS;
        rowUB[i] = jjData->consistency_eqtns[i].RHS;
    }
    
    if (element != number_of_elements) {
        logger->error(301, "Incorrect number of elements in Coin Packed Matrix");
    }

    logger->log(5, "Coin Packed Matrix contains %d elements", number_of_elements);
    
    matrixA = new CoinPackedMatrix(ROW_ORDERED, row_indices, col_indices, elements, element);
}

void Solver::release_coin_memory() {
    if (row_indices != NULL) {
        delete[] row_indices;
    }

    if (col_indices != NULL) {
        delete[] col_indices;
    }

    if (elements != NULL) {
        delete[] elements;
    }

    if (varLB != NULL) {
        delete[] varLB;
    }

    if (varUB != NULL) {
        delete[] varUB;
    }

    if (objCoeffs != NULL) {
        delete[] objCoeffs;
    }

    if (rowLB != NULL) {
        delete[] rowLB;
    }

    if (rowUB != NULL) {
        delete[] rowUB;
    }

    if (matrixA != NULL) {
        delete matrixA;
    }
}

void Solver::logModel() {
    logger->log(6, "Model iteration count: %d", si->getIterationCount());
    logger->log(6, "Model abandoned: %s", si->isAbandoned()? "true" : "false");
    logger->log(6, "Model dual objective limit reached: %s", si->isDualObjectiveLimitReached()? "true" : "false");
    logger->log(6, "Model iteration limit reached: %s", si->isIterationLimitReached()? "true" : "false");
    logger->log(6, "Model proven dual infeasible: %s", si->isProvenDualInfeasible()? "true" : "false");
    logger->log(6, "Model proven optimal: %s", si->isProvenOptimal()? "true" : "false");
}
