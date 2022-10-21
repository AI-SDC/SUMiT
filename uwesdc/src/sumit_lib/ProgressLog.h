#pragma once

#include "stdafx.h"
#include <time.h>

class ProgressLog {

public:
    ProgressLog(const char *filename);
    ~ProgressLog();
    void write_log_file(const char* filename, int cells, int P, int S, int exact, int broken, int calls, double cost);
    void write_log_message(const char* message);
    void write_newline();

private:
    FILE *ofp;

    char *timeString(time_t *tm);
};
