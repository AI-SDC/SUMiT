#pragma once

#define VERSION "1.7.0"

// The minimum amount for two floating point numbers to be deemed different
#define FLOAT_PRECISION 0.0001

#define MAX_HOST_NAME_SIZE 64
#define MAX_PORT_NUMBER_SIZE 32

#define MIN(X, Y) (((X) < (Y))? (X): (Y))
#define MAX(X, Y) (((X) > (Y))? (X): (Y))

extern bool debugging;
extern Logger *logger;
extern System sys;

int main(int argc, char* argv[]);
