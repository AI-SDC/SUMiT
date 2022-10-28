#include "stdafx.h"
#include <stdlib.h>
#include "Eliminate.h"

Eliminate::Eliminate(const char* injjfilename) {
    jjData = new JJData(injjfilename);

    logger->log(3, "Elimination precision is %" PRIdFAST64, INT64_PRECISION);

    grand_total = 0;
    nPrimaryCells = 0;

    cells = new Cell[jjData->ncells];

    // Copy cell data from JJ reader and convert to int_fast64_t for speed
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        // Some fields are not used in elimination and have been commented out to reduce memory usage
//        cells[i].id = jjData->cells[i].id;
        cells[i].nominal_value = (int_fast64_t)(jjData->cells[i].nominal_value * INT64_PRECISION);
        cells[i].loss_of_information_weight = (int_fast64_t)(jjData->cells[i].loss_of_information_weight * INT64_PRECISION);
        cells[i].status = jjData->cells[i].status;
//        cells[i].lower_bound = (int_fast64_t)(jjData->cells[i].lower_bound * LONGLONG_PRECISION);
//        cells[i].upper_bound = (int_fast64_t)(jjData->cells[i].upper_bound * LONGLONG_PRECISION);
        cells[i].lower_protection_level = (int_fast64_t)(jjData->cells[i].lower_protection_level * INT64_PRECISION);
        cells[i].upper_protection_level = (int_fast64_t)(jjData->cells[i].upper_protection_level * INT64_PRECISION);
//        cells[i].sliding_protection_level = (int_fast64_t)(jjData->cells[i].sliding_protection_level * LONGLONG_PRECISION);

        if (cells[i].nominal_value > grand_total) {
            grand_total = cells[i].nominal_value;
        }

        if (cells[i].status == 'u') {
            nPrimaryCells++;
        }
    }

    primaryCells = new CellIndex[nPrimaryCells];

    int j = 0;
    for(CellIndex i=0; i < jjData->ncells; i++) {
        if (cells[i].status == 'u') {
           primaryCells[j++] = i;
        }
    }

    consistency_eqtns = new ConsistencyEquation[jjData->nsums];

    for (SumIndex i = 0; i < jjData->nsums; i++) {
        consistency_eqtns[i].RHS = (int_fast64_t)(jjData->consistency_eqtns[i].RHS * INT64_PRECISION);
        consistency_eqtns[i].cell_index = jjData->consistency_eqtns[i].cell_index;
        consistency_eqtns[i].plus_or_minus = jjData->consistency_eqtns[i].plus_or_minus;
        consistency_eqtns[i].size_of_eqtn = jjData->consistency_eqtns[i].size_of_eqtn;
    }

    OrderedCells = NULL;

    cell_bounds = NULL;
    savedCellBounds = NULL;

    consolidated_eqtns = NULL;
    no_consolidated_eqtns = 0;
    saved_consolidated_equations = NULL;
    num_BackupConsolidatedEquations = 0;

    processing_eqtns_lower = NULL;
    processing_eqtns_upper = NULL;

    biggestConsEqn = 0;
}

Eliminate::~Eliminate(void) {
    delete[] cells;
    cells = NULL;

    delete[] primaryCells;
    primaryCells = NULL;

    // Do not delete memory for consistency_eqtns[i].cell_number consistency_eqtns[i].plus_or_minus
    // as these are pointers into jjData and will be handled when jjData is destroyed
    delete[] consistency_eqtns;
    consistency_eqtns = NULL;

    delete jjData;
}

void Eliminate::WriteJJFile(const char* outfilename) {
    // Update suppression pattern in JJ reader
    for (int i = 0; i < jjData->ncells; i++) {
        jjData->cells[i].status = cells[i].status;
    }

    jjData->write_jj_file(outfilename);
}

void Eliminate::allocate_cell_bounds_memory() {
    cell_bounds = new CellBounds[jjData->ncells];
    savedCellBounds = new CellBounds[jjData->ncells];
}

void Eliminate::init_cell_bounds() {
    int i;

    for (i = 0; i < jjData->ncells; i++) {
        if ((cells[i].status == 'u') || (cells[i].status == 'm'))
          {
            cell_bounds[i].lower = 0; // cells[i].lower_bound;
            cell_bounds[i].upper = grand_total; // cells[i].upper_bound;
          }
        else
          {
            cell_bounds[i].lower = cells[i].nominal_value;
            cell_bounds[i].upper = cells[i].nominal_value;
          }
    }
}

void Eliminate::release_cell_bounds_memory() {
    if (cell_bounds != NULL) {
        delete[] cell_bounds;
        cell_bounds = NULL;
    }
    if (savedCellBounds != NULL) {
        delete[] savedCellBounds;
        savedCellBounds = NULL;
    }
}

void Eliminate::release_Consolidated_Eqtns_memory(void) {
    int i;

    if (consolidated_eqtns != NULL) {
        for (i = 0; i < no_consolidated_eqtns; i++) {
            if (consolidated_eqtns[i].cell_number != NULL) {
                delete[] consolidated_eqtns[i].cell_number;
                consolidated_eqtns[i].cell_number = NULL;
            }

            if (consolidated_eqtns[i].plus_or_minus != NULL) {
                delete[] consolidated_eqtns[i].plus_or_minus;
                consolidated_eqtns[i].plus_or_minus = NULL;
            }
        }

        delete[] consolidated_eqtns;
        no_consolidated_eqtns= 0;
        consolidated_eqtns = (ConsolidatedEquation *) NULL;
    }
}


void Eliminate::release_SavedConsolidated_Eqtns_memory(void) {
    int i;

    if (saved_consolidated_equations != NULL) {
        for (i = 0; i < num_BackupConsolidatedEquations; i++) {
            if (saved_consolidated_equations[i].cell_number != NULL) {
                delete[] saved_consolidated_equations[i].cell_number;
                saved_consolidated_equations[i].cell_number = NULL;
            }

            if (saved_consolidated_equations[i].plus_or_minus != NULL) {
                delete[] saved_consolidated_equations[i].plus_or_minus;
                saved_consolidated_equations[i].plus_or_minus = NULL;
            }
        }

        delete[] saved_consolidated_equations;
        num_BackupConsolidatedEquations= 0;
        saved_consolidated_equations = (ConsolidatedEquation *) NULL;
    }
}

void Eliminate::restoreConsolidated_EquationsFromBackup(void)
{
    int i,j;
    release_Consolidated_Eqtns_memory( );
    consolidated_eqtns = new ConsolidatedEquation[num_BackupConsolidatedEquations];
    no_consolidated_eqtns = num_BackupConsolidatedEquations;


    for (i = 0; i < num_BackupConsolidatedEquations; i++)
      {
        consolidated_eqtns[i].size_of_eqtn = saved_consolidated_equations[i].size_of_eqtn;
        consolidated_eqtns[i].RHS = saved_consolidated_equations[i].RHS;
        consolidated_eqtns[i].cell_number = new int [saved_consolidated_equations[i].size_of_eqtn];
        consolidated_eqtns[i].plus_or_minus = new int [saved_consolidated_equations[i].size_of_eqtn];


        for (j=0;j< saved_consolidated_equations[i].size_of_eqtn;j++)
          {
            consolidated_eqtns[i].cell_number[j] = saved_consolidated_equations[i].cell_number[j];
            consolidated_eqtns[i].plus_or_minus[j] = saved_consolidated_equations[i].plus_or_minus[j];
          }
      }
}


void Eliminate::backup_Consolidated_Equations(void)
{
    int i,j, nonzeroConsolidatedEquations=0;
    int numEqnsCopied;

    release_SavedConsolidated_Eqtns_memory( );

    for(i=0;i<no_consolidated_eqtns;i++)
      {
        if (consolidated_eqtns[i].size_of_eqtn>0) {
            nonzeroConsolidatedEquations++;
        }
      }

    saved_consolidated_equations = new ConsolidatedEquation[nonzeroConsolidatedEquations];
    num_BackupConsolidatedEquations = nonzeroConsolidatedEquations;


    for (i = 0, numEqnsCopied=0; i < no_consolidated_eqtns; i++)
      {
        if(consolidated_eqtns[i].size_of_eqtn>0)
          {
            saved_consolidated_equations[numEqnsCopied].size_of_eqtn = consolidated_eqtns[i].size_of_eqtn;
            saved_consolidated_equations[numEqnsCopied].RHS = consolidated_eqtns[i].RHS;
            saved_consolidated_equations[numEqnsCopied].cell_number = new int [consolidated_eqtns[i].size_of_eqtn];
            saved_consolidated_equations[numEqnsCopied].plus_or_minus = new int [consolidated_eqtns[i].size_of_eqtn];

            for (j=0;j< saved_consolidated_equations[numEqnsCopied].size_of_eqtn;j++)
              {
                saved_consolidated_equations[numEqnsCopied].cell_number[j] = consolidated_eqtns[i].cell_number[j];
                saved_consolidated_equations[numEqnsCopied].plus_or_minus[j] = consolidated_eqtns[i].plus_or_minus[j];
              }
            numEqnsCopied++;

          }
      }
}


void Eliminate::backupCellBounds(void)
{
    int i;
    if (jjData->ncells == 0)
      {
        logger->error(1, "called backupCellBoinds when ncells is zero");
      }
    else  if  ( (cell_bounds == NULL) || (savedCellBounds==NULL) )
      {
        logger->error(1, "called backupCellBoinds with NULL arrays");
      }

    else
      {for(i=0;i<jjData->ncells;i++)
        {
          savedCellBounds[i] = cell_bounds[i];
        }
      }
}

void Eliminate::restoreSavedCellBounds(void)
{
    int i;
    if (jjData->ncells == 0)
      {
        logger->error(1, "called backupCellBoinds when ncells is zero");
      }
    else    if  ( (cell_bounds == NULL) || (savedCellBounds==NULL) )
      {
        logger->error(1, "called backupCellBounds with NULL arrays");
      }
    else
      {
        for(i=0;i<jjData->ncells;i++)
          {
            cell_bounds[i] = savedCellBounds[i] ;
          }
      }
}



void Eliminate::release_processing_eqtns_memory() {
    for(int i=0;i<no_processing_eqtns;i++)
      {
        if (processing_eqtns_lower[i].cells_lower_to_add != NULL) {
            delete[] processing_eqtns_lower[i].cells_lower_to_add;
            processing_eqtns_lower[i].cells_lower_to_add = NULL;
        }

        if (processing_eqtns_lower[i].cells_lower_to_sub != NULL) {
            delete[] processing_eqtns_lower[i].cells_lower_to_sub;
            processing_eqtns_lower[i].cells_lower_to_sub = NULL;
        }

        if (processing_eqtns_lower[i].cells_upper_to_add != NULL) {
            delete[] processing_eqtns_lower[i].cells_upper_to_add;
            processing_eqtns_lower[i].cells_upper_to_add = NULL;
        }

        if (processing_eqtns_lower[i].cells_upper_to_sub != NULL) {
            delete[] processing_eqtns_lower[i].cells_upper_to_sub;
            processing_eqtns_lower[i].cells_upper_to_sub = NULL;
        }

        if (processing_eqtns_upper[i].cells_lower_to_add != NULL) {
            delete[] processing_eqtns_upper[i].cells_lower_to_add;
            processing_eqtns_upper[i].cells_lower_to_add = NULL;
        }

        if (processing_eqtns_upper[i].cells_lower_to_sub != NULL) {
            delete[] processing_eqtns_upper[i].cells_lower_to_sub;
            processing_eqtns_upper[i].cells_lower_to_sub = NULL;
        }

        if (processing_eqtns_upper[i].cells_upper_to_add != NULL) {
            delete[] processing_eqtns_upper[i].cells_upper_to_add;
            processing_eqtns_upper[i].cells_upper_to_add = NULL;
        }

        if (processing_eqtns_upper[i].cells_upper_to_sub != NULL) {
            delete[] processing_eqtns_upper[i].cells_upper_to_sub;
            processing_eqtns_upper[i].cells_upper_to_sub = NULL;
        }
      }

    delete[] processing_eqtns_lower;
    processing_eqtns_lower = NULL;

    delete[] processing_eqtns_upper;
    processing_eqtns_upper = NULL;

    no_processing_eqtns = 0;
}

void Eliminate::tidy_up_the_consolidated_equations() {
    int i;
    int j;

    /* Tidy up the consolidated equations */

    for (i = 0; i < no_consolidated_eqtns; i++) {
        if (consolidated_eqtns[i].RHS < 0) {
            consolidated_eqtns[i].RHS = -1 * consolidated_eqtns[i].RHS;

            for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                consolidated_eqtns[i].plus_or_minus[j] = -1 * consolidated_eqtns[i].plus_or_minus[j];
            }
        }
    }
}

void Eliminate::consolidate_the_consistency_equations() {
    int i;
    int j;
    int nsuppressed;
    int cell_no;
    int consolidated_index;
    int *numsuppressed = new int[jjData->nsums];
    int numCreated;

    release_Consolidated_Eqtns_memory();

    // Find the required number of consolidated equations

    no_consolidated_eqtns = 0;

    for (i = 0; i < jjData->nsums; i++)
      {
        nsuppressed = 0;

        /* Count the number of suppressed cells in the equation */

        for (j = 0; j < consistency_eqtns[i].size_of_eqtn; j++)
          {
            cell_no = consistency_eqtns[i].cell_index[j];

            if ((cell_no >= 0) && (cell_no < jjData->ncells))
              {
                if ((cells[cell_no].status == 'u') || (cells[cell_no].status == 'm'))
                  {
                    nsuppressed++;
                  }
              }
          }

        /* If there is one or more suppressed cell then we increment the number of consolidated equations */
        numsuppressed[i] = nsuppressed;
        //
        if (nsuppressed > 0) {
            no_consolidated_eqtns++;
        }
      }

    // Allocate memory for the consolidated equations - after checking it  is clean first
    if (consolidated_eqtns != NULL) {
        release_Consolidated_Eqtns_memory();
    }

    consolidated_eqtns = new ConsolidatedEquation[no_consolidated_eqtns];

    for (i = 0; i < no_consolidated_eqtns; i++) {
        consolidated_eqtns[i].cell_number = NULL;
        consolidated_eqtns[i].plus_or_minus = NULL;
    }



    // Setup the required number of consolidated equations

    numCreated = 0;// it is used as a loop variable below

    for (i = 0; i < jjData->nsums; i++)
    {
        nsuppressed = numsuppressed[i];
        /* If there is one or more suppressed cell then we write it to the consolidated equations */
        if (nsuppressed > 0)
          {
            if(numCreated>no_consolidated_eqtns)
                logger->error(1, "something has gone wrong with counts of equations in consolidate_consistency_equations");
            consolidated_eqtns[numCreated].cell_number = new int [nsuppressed];
            consolidated_eqtns[numCreated].plus_or_minus = new int [nsuppressed];

            consolidated_eqtns[numCreated].RHS = consistency_eqtns[i].RHS;

            consolidated_index = 0;

            for (j = 0; j < consistency_eqtns[i].size_of_eqtn; j++)
              {
                cell_no = consistency_eqtns[i].cell_index[j];

                if ((cell_no >= 0) && (cell_no < jjData->ncells))
                  {
                    if ((cells[cell_no].status == 'u') || (cells[cell_no].status == 'm'))
                      {
                        consolidated_eqtns[numCreated].cell_number [consolidated_index] = cell_no;
                        consolidated_eqtns[numCreated].plus_or_minus[consolidated_index] = consistency_eqtns[i].plus_or_minus[j];
                        consolidated_index++;
                      }
                    else
                      {
                        consolidated_eqtns[numCreated].RHS = consolidated_eqtns[numCreated].RHS - (consistency_eqtns[i].plus_or_minus[j] * cells[cell_no].nominal_value);
                      }
                  }
              }
            if (consolidated_index != numsuppressed[i])
                logger->error(1, "mismatch in consolidated equations size: stored %d but should have been %d", consolidated_index,numsuppressed[i]);
            else
                consolidated_eqtns[numCreated].size_of_eqtn  = consolidated_index;
            numCreated++;

        } //end if( nsuppressed>0)
    }

    /* Display the number of consolidated equations */

    //logger->log(3, "%d consolidated equations", no_consolidated_eqtns);

    tidy_up_the_consolidated_equations();

    logger->log(3,"%d initial consolidated equations created", no_consolidated_eqtns);
    delete[] numsuppressed;
    numsuppressed = NULL;
}

int Eliminate::simplify_the_consistency_equations() {
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
                cell_no = consolidated_eqtns[i].cell_number[0];

                if (cell_bounds[cell_no].lower == cell_bounds[cell_no].upper) {
                    consolidated_eqtns[i].size_of_eqtn = 0;
                    nos_simplfied++;
                }
                break;

            default:
                for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                    cell_no = consolidated_eqtns[i].cell_number[j];

                    if (cell_bounds[cell_no].lower == cell_bounds[cell_no].upper) {
                        /* Remove this cell from the equation as its value is known */

                        if (consolidated_eqtns[i].plus_or_minus[j] == 1)
                          {
                            consolidated_eqtns[i].RHS = consolidated_eqtns[i].RHS - cell_bounds[cell_no].upper;
                          }
                        else
                          {
                            consolidated_eqtns[i].RHS = consolidated_eqtns[i].RHS + cell_bounds[cell_no].upper;
                          }

                        for (k = j + 1; k < consolidated_eqtns[i].size_of_eqtn; k++)
                          {
                            consolidated_eqtns[i].cell_number[k - 1] = consolidated_eqtns[i].cell_number[k];
                            consolidated_eqtns[i].plus_or_minus[k - 1] = consolidated_eqtns[i].plus_or_minus[k];
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

bool Eliminate::set_initial_lower_and_upper_bounds() {
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

                cell_no = consolidated_eqtns[i].cell_number[0];
                cell_bounds[cell_no].lower = consolidated_eqtns[i].RHS;
                cell_bounds[cell_no].upper = consolidated_eqtns[i].RHS;
                break;

            default:
                /* See if they are all adds... if they are we can get upper bounds */

                //SPEEDUP:  they all should be by definition
                no_plus = 0;

                for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                    if (consolidated_eqtns[i].plus_or_minus[j] == 1) {
                        no_plus++;
                    }
                }

                if (no_plus == consolidated_eqtns[i].size_of_eqtn) {
                    /* We can give upper bounds */

                    for (j = 0; j < consolidated_eqtns[i].size_of_eqtn; j++) {
                        cell_no = consolidated_eqtns[i].cell_number[j];

                        if (cell_bounds[cell_no].lower < 0) {
                            cell_bounds[cell_no].lower = 0;
                        }

                        if (cell_bounds[cell_no].upper > consolidated_eqtns[i].RHS)
                          {
                            cell_bounds[cell_no].upper = consolidated_eqtns[i].RHS;
                          }
                    }
                }
                break;
        }
    }
    return (rc);
}

bool Eliminate::getSafety() {
    int i,j;
    int nsup_found_1;
    int nsup_bounds_broken_1;
    bool safe;
    int_fast64_t temp;
    int_fast64_t safety_margin;

    safe = true;
    safety_margin = 1;

    nsup_found_1 = 0;
    nsup_bounds_broken_1 = 0;


    for (j = 0; j < nPrimaryCells; j++)
      {
        i = primaryCells[j];


        temp = cells[i].nominal_value - cells[i].lower_protection_level;

        if (temp < 0)
          {
            temp = 1;
          }

        if (cell_bounds[i].lower == cell_bounds[i].upper)
          {
            nsup_found_1++;
          }
        else if ( (cell_bounds[i].lower + safety_margin) > temp)
          {
            nsup_bounds_broken_1++;
          }
        else if ((cell_bounds[i].upper - safety_margin) < (cells[i].nominal_value + cells[i].upper_protection_level))
          {
            nsup_bounds_broken_1++;
          }
        /*
         else if (cells[i].sliding_protection_level > 0)
         {
         if ((cell_bounds[i].upper - cell_bounds[i].lower) < cells[i].sliding_protection_level)
         {
         nsup_bounds_broken_1++;
         }
         }
         */



        logger->log(5, "%d cells completely exposed", nsup_found_1);
        logger->log(5, "%d cells partially exposed", nsup_bounds_broken_1);

        if (initial_nsup_bounds_broken_1 >0) {
            logger->log(2, "initial number of cells with bounds broken = %d", initial_nsup_bounds_broken_1);
        }

        if (initial_nsup_found_1 >0) {
            logger->log(2, "initial number of cells with bounds broken = %d", initial_nsup_bounds_broken_1);
        }



        if ((nsup_found_1 > initial_nsup_found_1) || (nsup_bounds_broken_1 > initial_nsup_bounds_broken_1)) {
            safe = false;
        }


      }



    return (safe);
}

bool Eliminate::improve_lower_and_upper_bounds() {
    bool rc;
    int i;
    int j;
    int k;
    int sz;
    int cellno;
    int cell;
    int_fast64_t result;
    int indexik;
    int lower_bound_improvements;
    int upper_bound_improvements;
    int sizeOfEqi;
    int_fast64_t improvement;

    //logger->log(3, "Enter improve_lower_and_upper_bounds()");

    rc = false;
    lower_bound_improvements = 0;
    upper_bound_improvements = 0;



    for (i = 0; i < no_consolidated_eqtns; i++)
      {
        sizeOfEqi = consolidated_eqtns[i].size_of_eqtn;


        // Improve lower bounds
        for (k = 0; k < sizeOfEqi; k++)
          {
            indexik = index(i,k);
            result = processing_eqtns_lower[indexik].RHS;
            cellno = processing_eqtns_lower[indexik].cellnumber;


            sz = processing_eqtns_lower[indexik].no_lower_adds;
            for (j = sz; j >0;) {
                cell = processing_eqtns_lower[indexik].cells_lower_to_add[--j];
                result = result + cell_bounds[cell].lower;
            }


            sz = processing_eqtns_lower[index(i,k)].no_upper_adds;
            for (j = sz; j >0; ) {
                cell = processing_eqtns_lower[indexik].cells_upper_to_add[--j];
                result = result + cell_bounds[cell].upper;
            }


            sz = processing_eqtns_lower[index(i,k)].no_lower_subs;
            for (j =  sz; j>0;) {
                cell = processing_eqtns_lower[indexik].cells_lower_to_sub[--j];
                result = result - cell_bounds[cell].lower;
            }


            sz = processing_eqtns_lower[index(i,k)].no_upper_subs;
            for (j = sz; j >0; ) {
                cell = processing_eqtns_lower[indexik].cells_upper_to_sub[--j];
                result = result - cell_bounds[cell].upper;
            }

            if (cell_bounds[cellno].lower < result)
              {
                improvement = result - cell_bounds[cellno].lower;
                logger->log(5,"lower bound for cell %d increased by %" PRIdFAST64, cellno, improvement);

                //set the new value
                cell_bounds[cellno].lower = result;
                //test if it is time to leave
                if((cell_bounds[cellno].lower < 0) || (cell_bounds[cellno].lower >cell_bounds[cellno].upper))
                  {
                    logger->log(3,"improve_lower_and_upper_bounds(): had to correct lower bound on cell %d from %" PRIdFAST64 " to upper bound value",cellno, cell_bounds[cellno].lower);
                    cell_bounds[cellno].lower = cell_bounds[cellno].upper;
                    return(false);                }

                else if (improvement > UNPICKING_PRECISION)
                  {
                    lower_bound_improvements++;
                    rc = true;
                  }
              }
          }

        // Improve upper bounds

        for (k = 0; k < sizeOfEqi; k++)
          {
            indexik = index(i,k);
            result = processing_eqtns_upper[indexik].RHS;
            cellno = processing_eqtns_upper[indexik].cellnumber;

            sz = processing_eqtns_upper[indexik].no_lower_adds;
            for (j = sz; j >0; ) {
                cell = processing_eqtns_upper[indexik].cells_lower_to_add[--j];
                result = result + cell_bounds[cell].lower;
            }


            sz = processing_eqtns_upper[indexik].no_upper_adds;
            for (j = sz; j >0;) {
                cell = processing_eqtns_upper[indexik].cells_upper_to_add[--j];
                result = result + cell_bounds[cell].upper;
            }


            sz = processing_eqtns_upper[index(i,k)].no_lower_subs;
            for (j = sz; j >0 ; ) {
                cell = processing_eqtns_upper[indexik].cells_lower_to_sub[--j];
                result = result - cell_bounds[cell].lower;
            }


            sz = processing_eqtns_upper[indexik].no_upper_subs;
            for (j = sz; j >0;) {
                cell = processing_eqtns_upper[indexik].cells_upper_to_sub[--j];
                result = result - cell_bounds[cell].upper;
            }


            if (cell_bounds[cellno].upper > result)
              {
                improvement = cell_bounds[cellno].upper - result;
                logger->log(5,"upper bound for cell %d reduced by %" PRIdFAST64, cellno, improvement);

                //set value
                cell_bounds[cellno].upper = result;
                //is it time to leave?
                if((cell_bounds[cellno].upper < cell_bounds[cellno].lower)|| (cell_bounds[cellno].upper >grand_total))
                  {
                    logger->log(3,"had to correct upper bound on cell %d from %" PRIdFAST64 " to lower bound value %" PRIdFAST64,cellno, cell_bounds[cellno].upper,cell_bounds[cellno].lower);
                    cell_bounds[cellno].upper=cell_bounds[cellno].lower;
                    return(false);

                  }
                else if (improvement >UNPICKING_PRECISION) {
                    upper_bound_improvements++;
                    rc = true;
                  }
              }
          }






      }//end of i loop over consolidated equations



    //logger->log(3, "%d lower bound improvements", lower_bound_improvements);
    //logger->log(3, "%d upper bound improvements", upper_bound_improvements);




    return (rc);
}

void Eliminate::RecordExistingExposure(int when) {
    int iterations;
    int i;
    int nsup_1;
    int nsup_found_1;
    int nsup_bounds_broken_1;
    int_fast64_t temp;
    int_fast64_t safety_margin;
    bool ok = true;

    safety_margin = 1;

    nsup_1 = 0;
    nsup_found_1 = 0;
    nsup_bounds_broken_1 = 0;



    if (set_initial_lower_and_upper_bounds()) {
        while (simplify_the_consistency_equations() > 0) {
            set_initial_lower_and_upper_bounds();
        }

        iterations = 0;


        SetUpProcessingEquationsForCurrentConsolidatedEquations();

        while ((improve_lower_and_upper_bounds()) && (iterations < 5)) {
            iterations++;
        }

        for (i = 0; i < jjData->ncells; i++)
          {
            ok = true;
            temp=0;
            if (cells[i].status == 'u')
              {
                nsup_1++;

                temp = cells[i].nominal_value - cells[i].lower_protection_level;

                if (temp < 0) {
                    temp = 1;
                }

                if (cell_bounds[i].lower == cell_bounds[i].upper)
                  {
                    ok = false;
                    nsup_found_1++;
                  }
                else if (cell_bounds[i].lower + safety_margin > temp)
                  {
                    nsup_bounds_broken_1++;
                    ok=false;
                  }
                else if (cell_bounds[i].upper - safety_margin < cells[i].nominal_value + cells[i].upper_protection_level)
                  {
                    nsup_bounds_broken_1++;
                    ok=false;
                  }
              }
            if(ok==false)
              {
                logger->log(2, "Warning, after protection = %d: cell %d nominal %" PRIdFAST64 ", desired bounds %" PRIdFAST64 " - %" PRIdFAST64 " , actual bounds %" PRIdFAST64 " - %" PRIdFAST64,
                            i,when, cells[i].nominal_value, temp, (cells[i].nominal_value+ cells[i].upper_protection_level),cell_bounds[i].lower,cell_bounds[i].upper );
              }
          }
    }


    initial_nsup_found_1 = nsup_found_1;
    initial_nsup_bounds_broken_1 = nsup_bounds_broken_1;
    if(when==0)
      {
        logger->log(2, "Prior to Elimination there are %d primary cells fully exposed", initial_nsup_found_1);
        logger->log(2, "Prior to Elimination there are %d primary cells partially exposed", initial_nsup_bounds_broken_1);
      }
    else
      {
        logger->log(2, "After Elimination there are %d primary cells fully exposed", initial_nsup_found_1);
        logger->log(2, "After Elimination there are %d primary cells partially exposed", initial_nsup_bounds_broken_1);
      }

}

bool Eliminate::SafeToRemoveSecondaryCell() {
    int iterations;
    bool safe;

    static int maxiterations;
    safe = false;

    simplify_the_consistency_equations();
    set_initial_lower_and_upper_bounds();
    while (simplify_the_consistency_equations() > 0)
      {
        set_initial_lower_and_upper_bounds();
      }

    iterations = 0;


    SetUpProcessingEquationsForCurrentConsolidatedEquations();
    while ((improve_lower_and_upper_bounds()))// &&(iterations < 6))
      {
        iterations++;
      }

    safe = getSafety();

    //just for testing purposes
    if (iterations > maxiterations)
      {
        maxiterations = iterations;
        logger->log(2, "new max iterations value %d", maxiterations);

      }


    return (safe);
}

void Eliminate::allocate_ordered_cells_memory() {
    OrderedCells = new int[jjData->ncells];
}

void Eliminate::release_ordered_cells_memory() {
    if (OrderedCells != NULL) {
        delete[] OrderedCells;
        OrderedCells = NULL;
    }
}

void Eliminate::init_ordered_cells() {
    int i;
    int j;
    int k;
    int l;

    for (i = 0; i < jjData->ncells; i++) {
        OrderedCells[i] = i;
    }

    for (i = 0; i < jjData->ncells; i++) {
        for (j = i+1; j < jjData->ncells; j++) {
            k = OrderedCells[i];
            l = OrderedCells[j];

            if (cells[k].loss_of_information_weight < cells[l].loss_of_information_weight) {
                OrderedCells[i] = l;
                OrderedCells[j] = k;
            }
        }
    }
    /*
     for (i=0; i<jjData->ncells; i++)
     {
     logger->log(3, "%3d %lf ", OrderedCells[i], cells[i].loss_of_information_weight);
     }
     */
}

void Eliminate::eliminate_secondary_suppression() {
    int i;
    int j;
    int k;
    int sz;
    int count_primaries;
    int count_secondaries;
    int count_removable;
    int count_marginals;
    int iterations;

    // Cut down the work needed to do

    for (i = 0; i < jjData->ncells; i++) {
        if (cells[i].status == 'm')
          {
            cells[i].removable = true;
          } else {
              cells[i].removable = false;
          }
    }

    for (iterations = 0; iterations < 6; iterations++)
      {

        //jim adding comments as I work out what the code does
        //loop through each consistency equation
        for (i = 0; i < jjData->nsums; i++)
          {
            sz = consistency_eqtns[i].size_of_eqtn;

            if (sz > 0)
              {
                // and every cell therein
                count_primaries = 0;
                count_secondaries = 0;
                count_removable = 0;
                count_marginals=0;

                for (j = 0; j < sz; j++)
                  {
                    k = consistency_eqtns[i].cell_index[j];
                    if(consistency_eqtns[i].plus_or_minus[j]== -1)
                        count_marginals++;

                    if (cells[k].status == 'u')
                      {
                        count_primaries++;
                      }
                    else if (cells[k].status == 'm')
                      {
                        count_secondaries++;

                        if (cells[k].removable)
                          {
                            count_removable++;
                          }
                        //i.e. not all secondary cells are removable - especially on the second and subsequent iterations
                      }
                  }
                //so now we know what is in the consistency equation, we can tell the function don't bother to look at the secondaries  if
                if (
                    //there is one primary and just one secondary protecting it
                    ((count_primaries == 1) && (count_secondaries == 1))
                    //or no primaries but two secondaries which must be protecting each other, and one has already been flagged as not removable
                    || ((count_primaries == 0) && (count_secondaries == 2) && (count_removable == 1))
                    //||((count_primaries == 0) && (count_secondaries == 2) && (count_marginals == 0))
                    //Jim: SPEEDUP I thin kthat a third condition is that a cell is not a candidate for removal if it is a marginal
                    //possible other condition 1: if the sum of the non-biggest primaries, plus the sum of the secondaries, minus any one of them, is less than the required protection level for the biggest primary
                    )
                  {
                    for (j = 0; j < sz; j++)
                      {
                        k = consistency_eqtns[i].cell_index[j];

                        if (cells[k].status == 'm') {
                            cells[k].removable = false;
                            //mark  secondary cells as needed - so not a candidate for removal
                        }
                      }
                  }
              }//end if sz >0 i.e. end of dealing with one consistency equation
          }//end loop over all consistency equations
      }//end of iterations through process of generating candidates for removal

    count_secondaries = 0;
    count_removable = 0;

    for (i = 0; i < jjData->ncells; i++)
      {
        if (cells[i].status == 'm')
          {
            count_secondaries++;

            if (cells[i].removable)
              {
                count_removable++;
              }
          }
      }

    logger->log(2, "Attempting to remove %d out of %d secondaries... %d cells, %d equations", count_removable, count_secondaries, jjData->ncells, jjData->nsums);





    // Do the elimination
    //make sure the consolidated equations exist - they should have been produced by RecordExistingExposure
    if(no_consolidated_eqtns==0)
      {
        logger->log(2,"woops -seem to need to create consolidated equations ");
        consolidate_the_consistency_equations();
      }
    backup_Consolidated_Equations();
    number_of_secondary_cells_removed=0;
    init_cell_bounds();
    set_initial_lower_and_upper_bounds() ;
    backupCellBounds();



    for (i = 0; i < jjData->ncells; i++)
      {
        //this line of code switches from the order presented to biggest-first order
        j = OrderedCells[i];
        if (cells[j].removable)
          {
            backup_Consolidated_Equations();
            backupCellBounds();
            cells[j].status = 's';
            cell_bounds[j].lower = cells[j].nominal_value;
            cell_bounds[j].upper =cells[j].nominal_value;

            if (SafeToRemoveSecondaryCell())
              {
                number_of_secondary_cells_removed++;
                logger->log(5, "ok to remove cell %d", j);
                backup_Consolidated_Equations();
                backupCellBounds();
              }
            else //reverse the process
              {
                cells[j].status = 'm';
                restoreConsolidated_EquationsFromBackup(); //because th call to simplify_the_consistency_equations(); might hae changed some
                restoreSavedCellBounds(); //because they will have beenchanged during the unicking attempt in SafeToRemoveSecondaryCell()
                logger->log(5, "not safe to publish cell %d", j);
              }

          }
      }

    logger->log(2, "%d secondary cells removed",number_of_secondary_cells_removed);
}

void Eliminate::display_secondary_suppression() {
    int i;
    int j;
    double cost;

    j = 1;
    cost = 0.0;

    for (i = 0; i < jjData->ncells; i++) {
        if (cells[i].status == 'm') {
            j++;
            cost = cost + (double)cells[i].loss_of_information_weight;

            logger->log(3, " %3ld", i);
        }
    }

    logger->log(3, "%d cells suppressed at a cost of %lf ", j - 1, cost);
}

void Eliminate::RemoveExcessSuppression() {
    time(&start_seconds);

    number_of_secondary_cells_removed = 0;

    allocate_cell_bounds_memory();
    init_cell_bounds();
    consolidate_the_consistency_equations();
    if (allocateMemoryForProcessingEquations() == false)
      {
        logger->error(1, "Unable to allocate memory for processing equations");
      }



    RecordExistingExposure(0);

    allocate_ordered_cells_memory();
    init_ordered_cells();

    logger->log(2, "Eliminating secondary cells...");
    eliminate_secondary_suppression();
    logger->log(2, "Number of secondary cells removed = %d", number_of_secondary_cells_removed);

    consolidate_the_consistency_equations();
#ifdef DEBUGGING
    RecordExistingExposure(1);
#endif
    release_ordered_cells_memory();
    release_cell_bounds_memory();
    release_processing_eqtns_memory();
    release_Consolidated_Eqtns_memory();
    release_SavedConsolidated_Eqtns_memory();

    time(&stop_seconds);

    logger->log(2, "Elimination time %d seconds", (int)(stop_seconds - start_seconds));
}

double Eliminate::getCost() {
    double cost = 0.0;
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        if ((cells[i].status == 'u') || (cells[i].status == 'm')) {
            // Use exact version of cost from JJReader to avoid rounding issues with local (int_fast64_t) version
            cost = cost + jjData->cells[i].loss_of_information_weight;
        }
    }

    return cost;
}

int Eliminate::index( int x, int y )  { return x * biggestConsEqn  + y; }




bool  Eliminate::allocateMemoryForProcessingEquations()
{
    int i;
    bool returnval = true;
    //clean up anythjing that was here before

    no_processing_eqtns = 0;

    if (no_consolidated_eqtns <=0)
      {
        returnval = false;
      }
    else
      {

        //phase 1: find out how much memory we need to allocate
        biggestConsEqn = 0;
        for (i = 0; i < no_consolidated_eqtns; i++)
          {
            if (consolidated_eqtns[i].size_of_eqtn > biggestConsEqn) {
                biggestConsEqn = consolidated_eqtns[i].size_of_eqtn;
            }
          }
        if (biggestConsEqn==0) {
            returnval=false;
            logger->error(1, "Consolidated equations all have size 0");
        }
        else
          {   // then allocate it
              no_processing_eqtns = biggestConsEqn * no_consolidated_eqtns;
              processing_eqtns_lower = new  ProcessingEquation [no_processing_eqtns];
              processing_eqtns_upper = new  ProcessingEquation [no_processing_eqtns];
              for(i=0;i<no_processing_eqtns;i++)
                {
                  processing_eqtns_lower[i].cells_lower_to_add = new int[biggestConsEqn];
                  processing_eqtns_lower[i].cells_lower_to_sub = new int[biggestConsEqn];
                  processing_eqtns_lower[i].cells_upper_to_add = new int[biggestConsEqn];
                  processing_eqtns_lower[i].cells_upper_to_sub = new int[biggestConsEqn];

                  processing_eqtns_upper[i].cells_lower_to_add = new int[biggestConsEqn];
                  processing_eqtns_upper[i].cells_lower_to_sub = new int[biggestConsEqn];
                  processing_eqtns_upper[i].cells_upper_to_add = new int[biggestConsEqn];
                  processing_eqtns_upper[i].cells_upper_to_sub = new int[biggestConsEqn];

                }

          }
      }
    return returnval;
}

void Eliminate::SetUpProcessingEquationsForCurrentConsolidatedEquations(void)
{
    int i,j,k,sz;
    int_fast64_t consolidated_RHS;
    int indexij,idx;
    int dest_cell,dest_plus_minus;

    /////////////////////
    // Initialise memory //
    /////////////////////

    //phase 1: zero it all

    for (j = no_consolidated_eqtns*biggestConsEqn; j >0  ; )
      {
        j--;
        processing_eqtns_lower[j].no_lower_adds = 0;
        processing_eqtns_lower[j].no_lower_subs = 0;
        processing_eqtns_lower[j].no_upper_adds = 0;
        processing_eqtns_lower[j].no_upper_subs = 0;
        processing_eqtns_upper[j].no_lower_adds = 0;
        processing_eqtns_upper[j].no_lower_subs = 0;
        processing_eqtns_upper[j].no_upper_adds = 0;
        processing_eqtns_upper[j].no_upper_subs = 0;
      }


    //phase 2: put the real values in

    for (i = 0; i < no_consolidated_eqtns; i++)
      {

        sz = consolidated_eqtns[i].size_of_eqtn;

        if (sz > 0)
          {
            ////////////////////////////////////
            // Setup the processing equations //
            ////////////////////////////////////

            consolidated_RHS = consolidated_eqtns[i].RHS;

            for (j = 0; j < sz; j++)
              {
                indexij = index(i,j);

                // Setup processing equations

                dest_cell = consolidated_eqtns[i].cell_number[j];
                dest_plus_minus = consolidated_eqtns[i].plus_or_minus[j];

                processing_eqtns_lower[indexij].cellnumber = dest_cell;
                processing_eqtns_upper[indexij].cellnumber = dest_cell;

                if (dest_plus_minus == 1)
                  {
                    // The cell we are looking at is +ve

                    processing_eqtns_lower[indexij].RHS = consolidated_RHS;
                    processing_eqtns_upper[indexij].RHS = consolidated_RHS;

                    for (k = 0; k < sz; k++) {//remember k and j are both counters in the ith consolidated equation
                        if (k != j)
                          {
                            // Get all the other cells in the consolidated equation and put them in the right place

                            if (consolidated_eqtns[i].plus_or_minus[k] == 1)
                              {
                                // Set up lower calculation

                                idx = processing_eqtns_lower[indexij].no_upper_subs;
                                processing_eqtns_lower[indexij].cells_upper_to_sub[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_lower[indexij].no_upper_subs = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper[indexij].no_lower_subs;
                                processing_eqtns_upper[indexij].cells_lower_to_sub[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_upper[indexij].no_lower_subs = idx;
                              }
                            else
                              {
                                // Set up lower calculation

                                idx = processing_eqtns_lower[indexij].no_lower_adds;
                                processing_eqtns_lower[indexij].cells_lower_to_add[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_lower[indexij].no_lower_adds = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper[indexij].no_upper_adds;
                                processing_eqtns_upper[indexij].cells_upper_to_add[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_upper[indexij].no_upper_adds = idx;
                              }
                          }
                    }//end loop over k
                  }//end dest_plus_minus==1
                else
                  {
                    // The cell we are looking at is -ve

                    processing_eqtns_lower[indexij].RHS = -consolidated_RHS;
                    processing_eqtns_upper[indexij].RHS = -consolidated_RHS;

                    for (k = 0; k < sz; k++)
                      {
                        if (k != j) {
                            // Get all the other cells in the consolidated equation and put them in the right place

                            if (consolidated_eqtns[i].plus_or_minus[k] == 1) {
                                // Set up lower calculation

                                idx = processing_eqtns_lower[indexij].no_lower_adds;
                                processing_eqtns_lower[indexij].cells_lower_to_add[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_lower[indexij].no_lower_adds = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper[indexij].no_upper_adds;
                                processing_eqtns_upper[indexij].cells_upper_to_add[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_upper[indexij].no_upper_adds = idx;
                            } else {
                                // Set up lower calculation

                                idx = processing_eqtns_lower[indexij].no_upper_subs;
                                processing_eqtns_lower[indexij].cells_upper_to_sub[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_lower[indexij].no_upper_subs = idx;

                                // Set up upper calculation

                                idx = processing_eqtns_upper[indexij].no_lower_subs;
                                processing_eqtns_upper[indexij].cells_lower_to_sub[idx] = consolidated_eqtns[i].cell_number[k];
                                idx++;
                                processing_eqtns_upper[indexij].no_lower_subs = idx;
                            }
                        }
                      }
                  }

              }
          }

      }

}
