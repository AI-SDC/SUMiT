#pragma once

#include <stdio.h>

class Logger {

public:
    Logger(int level, const char *filename);
    ~Logger();
    void setLevel(int level);
    int getLevel();
    void consoleSummary(bool summary);
    void log(int level, const char *fmt, ...);
    void error(int error_code, const char *fmt, ...);

private:
    int level;
    char filename[64];
    
    FILE *ofp;
    int stdout_dup;
    bool summary;

    void terminate(bool error);
};
