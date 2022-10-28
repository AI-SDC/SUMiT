#include "stdafx.h"
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "Logger.h"

Logger::Logger(int level, const char *filename) {
    this->level = level;
    strcpy(this->filename, filename);
    summary = true;

    // Duplicate and redirect stdout to log file
    // This is to allow third-party log output to be included in the program's log file
    stdout_dup = sys.duplicate(sys.file_number(stdout));
    ofp = freopen(filename, "w", stdout);
}

Logger::~Logger() {
    terminate(false);
}

void Logger::terminate(bool error) {
    // Recover original stdout
    fflush(stdout);
    sys.duplicate2(stdout_dup, sys.file_number(stdout));
    sys.close(stdout_dup);

    // Do not close log file as it is stdout and needs to remain open

    if ((! error) && (level == 0)) {
        remove(filename);
    }
}

void Logger::setLevel(int level) {
    this->level = level;
}

int Logger::getLevel() {
    return level;
}

void Logger::consoleSummary(bool summary) {
    this->summary = summary;
}

void Logger::log(int level, const char *fmt, ...) {
    if ((level <= this->level) || (summary && (level == 1))) {
        // Get everything ready
        time_t now = time(NULL);

        char msg[8192];

        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        // Log to file according to log level
        if ((level <= this->level) && (ofp != NULL)) {
            fprintf(ofp, "%ld %d %s\n", (long)now, level, msg);
            fflush(ofp);
        }

        if (summary) {
            if (debugging) {
                // Output a copy of the full log message to the console according to log level
                fprintf(stderr, "%ld %d %s\n", (long)now, level, msg);
            } else if (level == 1) {
                // Output a summary of all level 1 messages to the console
                fprintf(stderr, "%s\n", msg);
            }
            fflush(stderr);
        }
    }
}

void Logger::error(int error_code, const char *fmt, ...) {
    time_t now = time(NULL);

    char msg[8192];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    // Log error to file
    if (ofp != NULL) {
        fprintf(ofp, "%ld Error: %s\n", (long)now, msg);
        fflush(ofp);
    }

    if (summary) {
        // Output a summary of all error messages to the console
        fprintf(stderr, "Error: %s\n", msg);
        fflush(stderr);
    } else if (debugging) {
        // Output a copy of the full error message to the console
        fprintf(stderr, "%ld Error: %s\n", (long)now, msg);
        fflush(stderr);
    }

    terminate(true);

    exit(error_code);
}
