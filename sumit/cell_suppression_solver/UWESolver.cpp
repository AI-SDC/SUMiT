#include <stdafx.h>
#include <stdlib.h>
#include <time.h>
#include <UWECellSuppression.h>
#include "Solver.h"
#ifndef UWEVERSION
#define UWEVERSION "1.7.0"
#endif
#define MAX_ARG_LENGTH (1024 + MAX_FILENAME_SIZE * 4) // Basic arguments plus four filenames

bool debugging = false;
Logger *logger = NULL;
System sys;

char session[64];
char injjfilename[MAX_FILENAME_SIZE];
char outjjfilename[MAX_FILENAME_SIZE];
char permfilename[MAX_FILENAME_SIZE];
char costfilename[MAX_FILENAME_SIZE];
int protection = INDIVIDUAL_PROTECTION;
int model = FULL_MODEL;
double max_cost = 0.0;

void setKeyValue(const char *key, const char *value) {
    if (sys.string_case_compare(key, "session") == 0) {
        // Ignore
    } else if (sys.string_case_compare(key, "infile") == 0) {
        if (strlen(value) < MAX_FILENAME_SIZE) {
            if (*value == '\0') {
                logger->error(101, "No input file name specified");
            }
            strcpy(injjfilename, value);
        } else {
            logger->error(102, "Input file name too long: %s", value);
        }
    } else if (sys.string_case_compare(key, "outfile") == 0) {
        if (strlen(value) < MAX_FILENAME_SIZE) {
            if (*value == '\0') {
                logger->error(103, "No output file name specified");
            }
            strcpy(outjjfilename, value);
        } else {
            logger->error(104, "Output file name too long: %s", value);
        }
    } else if (sys.string_case_compare(key, "permfile") == 0) {
        if (strlen(value) < MAX_FILENAME_SIZE) {
            if (*value == '\0') {
                logger->error(117, "No permutation file name specified");
            }
            strcpy(permfilename, value);
        } else {
            logger->error(118, "Permutation file name too long: %s", value);
        }
    } else if (sys.string_case_compare(key, "costfile") == 0) {
        if (strlen(value) < MAX_FILENAME_SIZE) {
            if (*value == '\0') {
                logger->error(115, "No cost file name specified");
            }
            strcpy(costfilename, value);
        } else {
            logger->error(116, "Cost file name too long: %s", value);
        }
    } else if (sys.string_case_compare(key, "protection") == 0) {
        if (sys.string_case_compare(value, "individual") == 0) {
            protection = INDIVIDUAL_PROTECTION;
        } else if (sys.string_case_compare(value, "group") == 0) {
            protection = GROUP_PROTECTION;
        } else {
            logger->error(105, "Invalid protection type: %s", value);
        }
    } else if (sys.string_case_compare(key, "model") == 0) {
        if (sys.string_case_compare(value, "full") == 0) {
            model = FULL_MODEL;
        } else if (sys.string_case_compare(value, "yplus") == 0) {
            model = YPLUS_MODEL;
        } else if (sys.string_case_compare(value, "yminus") == 0) {
            model = YMINUS_MODEL;
        } else {
            logger->error(106, "Invalid model type: %s", value);
        }
    } else if (sys.string_case_compare(key, "maxcost") == 0) {
        if (sscanf(value, "%lf", &max_cost) != 1) {
            logger->error(119, "Invalid maximum cost: %s", value);
        }
    } else {
        logger->error(109, "Invalid key: %s", key);
    }
}

int main(int argc, char *argv[]) {
    injjfilename[0] = '\0';
    outjjfilename[0] = '\0';
    permfilename[0] = '\0';
    costfilename[0] = '\0';

    time_t startTime = time(NULL);

    char log_file_name[MAX_FILENAME_SIZE];
    sprintf(log_file_name, "Solver%dLog.txt", sys.get_process_id());

    logger = new Logger(0, log_file_name);
    logger->log(1, "UWESolver v%s", UWEVERSION);

    char cwd[MAX_FILENAME_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        logger->log(3, "Current working directory: %s", cwd);
    } else {
        logger->error(111, "Cannot get current working directory");
    }

    if (argc != 2) {
        logger->error(112, "Incorrect number of arguments");
    }
    if (strlen(argv[1]) >= MAX_ARG_LENGTH) {
        logger->error(113, "Arguments too long");
    }

    logger->log(3, "Arguments: %s", argv[1]);

    char key[MAX_ARG_LENGTH];
    char value[MAX_ARG_LENGTH];
    char *p = argv[1];
    char *k = key;
    char *v = value;

    int state = 0;
    while (*p != '\0') {
        switch (*p) {
            case '=':
                *k = '\0';
                k = key;
                state = 1;
                break;

            case '&':
                *v = '\0';
                v = value;
                state = 0;
                setKeyValue(key, value);
                break;

            default:
                if (state == 1) {
                    *v++ = *p;
                } else {
                    *k++ = *p;
                }
        }

        p++;
    }
    if (state == 1) {
        *v = '\0';
        setKeyValue(key, value);
    }

    Solver *solver = new Solver(injjfilename, (protection == GROUP_PROTECTION));

    logger->log(3, "%d groups", solver->get_number_of_groups());

    double* costs;
    int costs_size;
    if (protection == INDIVIDUAL_PROTECTION) {
        costs = solver->run_individual_protection(permfilename, model, max_cost, &costs_size);
    } else {
        costs = solver->run_group_protection(permfilename, model, max_cost, &costs_size);
    }

    solver->write_cost_file(costfilename, costs, costs_size);

    time_t now = time(NULL);
    int elapsedTime = (int)(now - startTime);

    solver->write_jj_file(outjjfilename);

    logger->log(3, "Result: %lf", costs[costs_size  - 1]);
    logger->log(3, "Elapsed time: %d", elapsedTime);
    logger->log(3, "Done");

    // No use of logger allowed after this statement
    // This must be done before outputting the program's results to stdout because the logger redirects stdout
    delete logger;

    // This is the only output allowed to the original stdout
    printf("%lf %d", costs[costs_size - 1], elapsedTime);

    delete[] costs;
    delete solver;

    exit(0);
}
