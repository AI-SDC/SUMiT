#include "stdafx.h"
#include <stdlib.h>
#include "CSVWriter.h"

CSVWriter::CSVWriter(const char* injjfilename) {
    jjData = new JJData(injjfilename);
}

CSVWriter::~CSVWriter(void) {
    delete jjData;
}

int CSVWriter::getTokens(char* instring, char separator) {
    int number_of_tokens = -1;
    char chr = instring[0];
    int state = 0;
    int i = 0;
    int j = 0;

    while (chr != '\0') {
        switch (state) {
            case 0: // Initial state
                if (chr == separator) {
                    number_of_tokens++;
                    state = 1;
                } else if ((chr == ' ') || (chr == '\t') || (chr == '\n')) {
                    state = 1;
                } else {
                    number_of_tokens++;
                    j = 0;
                    Tokens[number_of_tokens][j] = chr;
                    Tokens[number_of_tokens][j + 1] = '\0';
                    state = 2;
                }
                break;

            case 1: // Leading white space
                if (chr == separator) {
                    number_of_tokens++;
                    state = 1;
                } else if ((chr == ' ') || (chr == '\t') || (chr == '\n')) {
                    state = 1;
                } else {
                    number_of_tokens++;
                    j = 0;
                    Tokens[number_of_tokens][j] = chr;
                    Tokens[number_of_tokens][j + 1] = '\0';
                    state = 2;
                }
                break;

            case 2: // Collecting chars
                if (chr == separator) {
                    //number_of_tokens++;
                    state = 1;
                } else {
                    j++;
                    Tokens[number_of_tokens][j] = chr;
                    Tokens[number_of_tokens][j + 1] = '\0';
                    state = 2;
                }
                break;

            default:
                break;
        }

        i++;
        chr = instring[i];
    }

    return number_of_tokens + 1;
}

bool CSVWriter::getline(FILE* fp, char* buffer) {
    bool rc = false;
    bool collect = true;

    int i = 0;
    while (collect) {
        int c = getc(fp);

        switch (c) {
            case EOF:
                if (i > 0) {
                    rc = true;
                }
                collect = false;
                break;

            case '\n':
                if (i > 0) {
                    rc = true;
                    collect = false;
                    buffer[i] = '\0';
                }
                break;

            default:
                buffer[i] = (char)c;
                i++;
                break;
        }
    }

    return rc;
}

// This method is for legacy tabular partitioning support only
void CSVWriter::write_csv_file(const char* mapfilename, const char* csvfilename) {
    FILE* ifp;
    FILE* ofp;

    if ((ifp = fopen(mapfilename, "r")) == NULL) {
        logger->error(1, "Unable to open input mapping file: %s", mapfilename);
    }

    if ((ofp = fopen(csvfilename, "w")) == NULL) {
        logger->error(1, "Unable to open output CSV file: %s", csvfilename);
    }

    while (getline(ifp, InputBuffer)) {
        int number_of_tokens = getTokens(InputBuffer, ',');

        if (number_of_tokens > 1) {
            int cell_number;
            if (sscanf(Tokens[0], "%d", &cell_number) == 1) {
                for (int i = 1; i < number_of_tokens; i++) {
                    fprintf(ofp, "%s,", Tokens[i]);
                }
                fprintf(ofp, "%lf,%c\n", jjData->cells[cell_number].nominal_value, jjData->cells[cell_number].status);
            }
        }
    }

    fclose(ifp);
    fclose(ofp);
}
