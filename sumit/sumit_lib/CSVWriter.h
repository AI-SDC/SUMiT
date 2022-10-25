#pragma once

#include "stdafx.h"
#include "JJData.h"

#define MAX_BUFFER_SIZE 500
#define MAX_STRING_SIZE 200

#define MAX_NO_METADATA_FIELDS  30

class CSVWriter {
    
public:
    CSVWriter(const char* injjfilename);
    ~CSVWriter(void);
    
    // This method is for legacy tabular partitioning only
    void write_csv_file(const char* mapfilename, const char* csvfilename);

private:

    char InputBuffer[MAX_BUFFER_SIZE];
    char Tokens[MAX_NO_METADATA_FIELDS][MAX_STRING_SIZE];

    JJData *jjData;

    bool getline(FILE* fp, char* buffer);
    int getTokens(char* instring, char seperator);

};
