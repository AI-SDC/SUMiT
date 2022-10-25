#include "stdafx.h"
#include <stdio.h>
#include "ProgressLog.h"

ProgressLog::ProgressLog(const char *filename) {
    if ((ofp = fopen(filename, "w")) == NULL) {
        logger->error(1, "Unable to open progress log file %s", filename);
    }

    fprintf(ofp, "UWE Cell Suppression Program v%s\n", VERSION);
    fprintf(ofp, "Start: %s\n\n", sys.time_string(NULL));
    fprintf(ofp, "Filename                    Cells        P        S    Exact   Broken    Evals         Cost\n\n");
}

ProgressLog::~ProgressLog() {
    if (ofp != NULL) {
        fprintf(ofp, "\nFinish: %s\n", sys.time_string(NULL));
        fclose(ofp);
    }
}

void ProgressLog::write_log_file(const char* filename, int cells, int P, int S, int exact, int broken, int calls, double cost) {
    if (ofp != NULL) {
        fprintf(ofp, "%-24s %8d %8d %8d %8d %8d %8d %12.3lf\n", filename, cells, P, S, exact, broken, calls, cost);
        fflush(ofp);
    }
}

void ProgressLog::write_log_message(const char* message) {
    if (ofp != NULL) {
        fprintf(ofp, "%s\n", message);
        fflush(ofp);
    }
}

void ProgressLog::write_newline() {
    if (ofp != NULL) {
        fprintf(ofp, "\n");
        fflush(ofp);
    }
}
