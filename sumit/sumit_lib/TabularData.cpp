#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "TabularData.h"

TabularData::TabularData(char* p_metadatafilename, char* p_tabdatafilename) {
    strcpy(tabdatafilename, p_tabdatafilename);
    strcpy(metadatafilename, p_metadatafilename);

    logger->log(2, "Tabular data file: %s", tabdatafilename);

    if (! file_exists(metadatafilename)) {
        logger->error(1, "File %s does not exist", metadatafilename);
    }

    if (! file_exists(tabdatafilename)) {
        logger->error(1, "File %s does not exist", tabdatafilename);
    }

    init_metadata_variables();
    init_metadata_work_area();

    if (! ReadMetaData()) {
        logger->error(1, "Unable to read metadata");
    }

    logger->log(3, "Metadata Separator = %c", metadata_separator_char);
    logger->log(3, "Metadata Safe char = %c", metadata_safe_char);
    logger->log(3, "Metadata Unsafe char = %c", metadata_unsafe_char);
    logger->log(3, "Metadata Protect char = %c", metadata_protect_char);

    if (! ValidMetadata()) {
        logger->error(1, "Invalid metadata");
    }

    AddTotalCodesToIndex();

    if (! ParseKeyFields()) {
        logger->error(1, "Failed to parse key fields in %s", tabdatafilename);
    }

    if (! AddKeyFieldHierarchies()) {
        logger->error(1, "Unable to add key field hierarchies");
    }

    SortKeyFields();
    AddKeyFieldIndexes();

    if (debugging) {
        PrintKeyFileds("KeyValues.txt");
    }

    DisplayFieldInformation();

    logger->log(5, "Convert microdata to JJ format");

    NumberOfDimensions = 0;

    for (int i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            logger->log(5, "%s", MetaDataFields[i].Name);
            MetaDataFields[i].UseThisIndex = true;
            KeyIndexInUse[MetaDataFields[i].KeyIndex] = true;
            NumberOfDimensions++;
        }
    }

    if (NumberOfDimensions == 0) {
        logger->error(1, "Failed to select any index fields");
    }

    logger->log(5, "Table will be %d-dimensional", NumberOfDimensions);
    logger->log(5, "Indices:");

    ncells = 1;
    for (int i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            if (MetaDataFields[i].UseThisIndex) {
                logger->log(5, "%s", MetaDataFields[i].Name);
                ncells = ncells * NumberOfKeyEntries[MetaDataFields[i].KeyIndex];
            }
        }
    }

    logger->log(5, "%d cells", ncells);

    for (int i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            if (MetaDataFields[i].UseThisIndex) {
                logger->log(5, "Dimension %d", NumberOfKeyEntries[MetaDataFields[i].KeyIndex] + 1);
            }
        }
    }

    SetupIndexingOffsets();

    logger->log(5, "Variables:");

    for (int i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_NUMERIC) {
            logger->log(5, "%d %s", MetaDataFields[i].DataIndex, MetaDataFields[i].Name);
        }
    }

    for (int i = 0; i < NumberOfFields; i++) {
        switch (MetaDataFields[i].Type) {
            case FIELD_TYPE_NUMERIC:
                if (MetaDataFields[i].DataIndex == 0) {
                    MetaDataFields[i].UseThisIndex = true;
                    logger->log(5, "%s", MetaDataFields[i].Name);
                }
                break;

            case FIELD_TYPE_STATUS:
                MetaDataFields[i].UseThisIndex = true;
                break;

            default:
                // This is here to stop the compiler warning about enumeration values not being handled in the switch
                break;
        }
    }

    cells = new Cell[ncells];
    cell_bounds = new CellBounds[ncells];
    cell_type = new int[ncells];

    initialise_cells();
    initialise_consistency_equations();

    if (! ReadCellData()) {
        logger->error(1, "Unable to read micro data");
    }

    logger->log(3, "%d records read: %s", NumberOfRecordsRead, tabdatafilename);

    switch (NumberOfDimensions) {
        case 1:
            logger->log(3, "Calculating totals for a 1-d table");
            SetTotalsFor1dTable();
            logger->log(3, "Selecting primary suppressed cells");
            set_weights_and_lower_and_upper_bounds();
            label_zeroed_cells();
            label_primary_cells();
            //print_jj_file_cells();
            logger->log(3, "Setting up consistency equations for a 1-d table");
            CountConsistencyEquationsFor1dTable();
            allocate_consistency_equation_memory();
            PrintConsistencyEquationsFor1dTable();
            break;

        case 2:
            logger->log(3, "Calculating totals for a 2-d table");
            SetTotalsFor2dTable();
            logger->log(3, "Selecting primary suppressed cells");
            set_weights_and_lower_and_upper_bounds();
            label_zeroed_cells();
            label_primary_cells();
            //print_jj_file_cells();
            //Print2dTable();
            logger->log(3, "Setting up consistency equations for a 2-d table");
            CountConsistencyEquationsFor2dTable();
            allocate_consistency_equation_memory();
            PrintConsistencyEquationsFor2dTable();
            break;

        case 3:
            logger->log(3, "Calculating totals for a 3-d table");
            SetTotalsFor3dTable();
            logger->log(3, "Selecting primary suppressed cells");
            set_weights_and_lower_and_upper_bounds();
            label_zeroed_cells();
            label_primary_cells();
            //print_jj_file_cells();
            logger->log(3, "Setting up consistency equations for a 3-d table");
            CountConsistencyEquationsFor3dTable();
            allocate_consistency_equation_memory();
            PrintConsistencyEquationsFor3dTable();
            break;

        case 4:
            logger->log(3, "Calculating totals for a 4-d table");
            SetTotalsFor4dTable();
            logger->log(3, "Selecting primary suppressed cells");
            set_weights_and_lower_and_upper_bounds();
            update4dWeightings();
            label_zeroed_cells();
            label_primary_cells();
            //print_jj_file_cells();
            logger->log(3, "Setting up consistency equations for a 4-d table");
            CountConsistencyEquationsFor4dTable();
            allocate_consistency_equation_memory();
            PrintConsistencyEquationsFor4dTable();
            break;

        case 5:
            logger->log(3, "Calculating totals for a 5-d table");
            SetTotalsFor5dTable();
            logger->log(3, "Selecting primary suppressed cells");
            set_weights_and_lower_and_upper_bounds();
            label_zeroed_cells();
            label_primary_cells();
            //print_jj_file_cells();
            logger->log(3, "Setting up consistency equations for a 5-d table");
            CountConsistencyEquationsFor5dTable();
            allocate_consistency_equation_memory();
            PrintConsistencyEquationsFor5dTable();
            break;

        case 6:
            logger->log(3, "Calculating totals for a 6-d table");
            SetTotalsFor6dTable();
            logger->log(3, "Selecting primary suppressed cells");
            set_weights_and_lower_and_upper_bounds();
            label_zeroed_cells();
            label_primary_cells();
            //print_jj_file_cells();
            logger->log(3, "Setting up consistency equations for a 6-d table");
            CountConsistencyEquationsFor6dTable();
            allocate_consistency_equation_memory();
            PrintConsistencyEquationsFor6dTable();
            break;

        default:
            logger->error(1, "Table of unsupported dimension");
            break;
    }

    logger->log(2, "End tabular data file: %s", tabdatafilename);
}

TabularData::~TabularData(void) {
    if (cells != NULL) {
        delete[] cells;
    }

    if (cell_bounds != NULL) {
        delete[] cell_bounds;
    }

    if (cell_type != NULL) {
        delete[] cell_type;
    }

    if (consistency_eqtns != NULL) {
        for (int i = 0; i < nsums; i++) {
            if (consistency_eqtns[i].cell_number != NULL) {
                delete[] consistency_eqtns[i].cell_number;
            }

            if (consistency_eqtns[i].plus_or_minus != NULL) {
                delete[] consistency_eqtns[i].plus_or_minus;
            }
        }

        delete[] consistency_eqtns;
    }
}

int TabularData::getTokens(char* instring, char separator) {
    int number_of_tokens;
    char chr;
    int state;
    int i;
    int j;

    number_of_tokens = -1;
    chr = instring[0];
    state = 0;
    i = 0;
    j = 0;

    while (chr != '\0') {
        switch (state) {
            case 0: // Initial state
                if (chr == separator) {
                    number_of_tokens++;
                    state = 1;
                } else if ((chr == ' ') || (chr == '\t') || (chr == '\n')) {
                    state = 1;
                } else if (chr == '"') {
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
                } else if (chr == '"') {
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
                } else if (chr == '"') {
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

    return (number_of_tokens + 1);
}

void TabularData::init_metadata_variables() {
    metadata_separator_char = ',';
    metadata_safe_char = 's';
    metadata_unsafe_char = 'u';
    metadata_protect_char = 'p';
}

void TabularData::init_metadata_work_area() {
    int i;
    int j;

    NumberOfKeyFields = 0;
    NumberOfDataFields = 0;
    NumberOfFields = 0;

    for (i = 0; i < MAX_NO_METADATA_FIELDS; i++) {
        for (j = 0; j < MAX_STRING_SIZE; j++) {
            MetaDataFields[i].Name[j] = '\0';
            MetaDataFields[i].TotalCode[j] = '\0';
            MetaDataFields[i].CodeList[j] = '\0';
            MetaDataFields[i].HierCodeList[j] = '\0';
            MetaDataFields[i].HierLeadString[j] = '\0';
        }
        MetaDataFields[i].HierLeadString[0] = '@'; // default

        for (j = 0; j < 5; j++) {
            MetaDataFields[i].Distance[j] = 0;
            MetaDataFields[i].HierarchicalLevels[j] = 0;
        }
        MetaDataFields[i].Type = FIELD_TYPE_UNKNOWN;
        MetaDataFields[i].Hierarchical = false;
        MetaDataFields[i].CodeListPresent = false;
        MetaDataFields[i].HierCodeListPresent = false;
        MetaDataFields[i].HierLeadStringPresent = false;
        MetaDataFields[i].NumberOfDecimals = 0;
        MetaDataFields[i].NumberOfHierarchies = 0;

        MetaDataFields[i].KeyIndex = 0;
        MetaDataFields[i].DataIndex = 0;

        MetaDataFields[i].UseThisIndex = false;
    }

    for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
        NumberOfKeyEntries[i] = 0;
        KeyEntryOffsets [i] = 0;
        CurrentKeyIndex [i] = 0;
        KeyIndexInUse [i] = false;
        KeyIsHierarchical [i] = false;
        KeyHierarchyLevels[i] = 0;
    }

}

void TabularData::initialise_cells() {
    int i;

    for (i = 0; i < ncells; i++) {
        cells[i].frequency = 0;
        cells[i].loss_of_information_weight = 0.0;
        cells[i].lower_bound = 0.0;
        cells[i].lower_protection_level = 0.0;
        cells[i].nominal_value = 0.0;
        cells[i].status = 's';
        cells[i].rule = '+';
        cells[i].upper_bound = 0.0;
        cells[i].upper_protection_level = 0.0;
        cells[i].largest_value_1 = 0.0;
        cells[i].largest_value_2 = 0.0;
        cells[i].largest_value_3 = 0.0;
        cells[i].number_of_contributing_cells = 1;

        cell_type[i] = ORDINARY_CELL;
    }
}

void TabularData::initialise_consistency_equations() {
    int j;

    nsums = 0;

    consistency_eqtn.RHS = 0.0;
    consistency_eqtn.size_of_eqtn = 0;

    for (j = 0; j < MAX_SIZE_OF_EQTNS; j++) {
        consistency_eqtn.cell_number [j] = 0;
        consistency_eqtn.plus_or_minus[j] = 1;
    }
}

/* SetupIndexingOffsets()
 *
 * This routine sets up a set of offsets that are then used to multiply by the various table
 * indexes to calculate the cell position, i.e.
 *
 * cell number = (index 1 * offset 1) + (index 2 * offset 2) + etc..
 */
void TabularData::SetupIndexingOffsets() {
    int i;
    int offset;
    int kindex;

    offset = 1;

    for (i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            if (MetaDataFields[i].UseThisIndex) {
                kindex = MetaDataFields[i].KeyIndex;

                if (kindex < 0) {
                    logger->error(1, "kindex < 0");
                }

                if (kindex >= MAX_NO_KEY_FIELDS) {
                    logger->error(1, "kindex >= MAX_NO_KEY_FIELDS");
                }
                KeyEntryOffsets[kindex] = offset;

                logger->log(3, "Number of entries for index %d = %d, offset = %d", kindex, NumberOfKeyEntries[kindex], offset);

                offset = offset * NumberOfKeyEntries[kindex];
            }
        }
    }
}

int TabularData::GetCellNumber() {
    int i;
    int index;

    index = 0;

    for (i = 0; i < NumberOfKeyFields; i++) {
        if (KeyIndexInUse[i]) {
            index = index + (CurrentKeyIndex[i] * KeyEntryOffsets[i]);
        }
    }

    if (index > ncells) {
        logger->log(3, "%d = ", index);

        for (i = 0; i < NumberOfKeyFields; i++) {
            if (KeyIndexInUse[i]) {
                logger->log(3, "(%d * %d) +", CurrentKeyIndex[i], KeyEntryOffsets[i]);
            }
        }
    }

    return index;
}

bool TabularData::getline(FILE *fp, char *buffer) {
    bool rc;
    bool collect;
    char c;
    int i;

    rc = false;
    collect = true;

    i = 0;
    while (collect) {
        c = (char)getc(fp);

        switch (c) {
            case EOF:
                if (i > 0) {
                    rc = true;
                }
                collect = false;
                break;

            case '\r':
                // Ignore Windows CR character
                break;

            case '\n':
                if (i > 0) {
                    rc = true;
                    collect = false;
                    buffer[i] = '\0';
                }
                break;

            default:
                buffer[i] = c;
                i++;
                break;
        }
    }

    return rc;
}

bool TabularData::AttributeMatches(const char *buffer, const char *attribute) {
    int i = 0;
    int j = 0;

    /* Get rid of any leading white space */
    while ((buffer[i] == ' ') || (buffer[i] == '\t')) {
        i++;
    }

    /* Check that they match */
    char c;
    while ((c = attribute[j]) != '\0') {
        if (buffer[i] != c) {
            return false;
        }
        i++;
        j++;
    }

    return true;
}

void TabularData::strcpy_without_quotes(char *s, const char *t) {
    int i;
    int j;
    char c;

    i = 0;
    j = 0;

    while ((c = t[i]) != '\0') {
        if (c != '"') {
            s[j] = c;
            j++;
        }
        i++;
    }
    s[j] = '\0';
}

int TabularData::sscanf2(char* instring, const char* formatstring, char* token1, char* token2) {
    // This is a horribly written bit of code... but I was in a hurry!
    // formatstring is ignored. It is here for compatibility with sscanf()

    int number_of_tokens;
    char chr;
    int i;
    int j;
    int state; // state = {0-init, 1-space, 2-chars, 3-inside double quotes}

    number_of_tokens = 0;
    i = 0;
    j = 0;
    chr = instring[i];
    state = 0;

    while (chr != '\0') {
        switch (state) {
            case 0:
                if (chr == '\n') {
                } else if ((chr == ' ') || (chr == '\t')) {
                    state = 1; // space
                } else if (chr == '"') {
                    number_of_tokens++;
                    j = -1;
                    state = 3; // inside double quotes
                } else {
                    number_of_tokens++;
                    if (number_of_tokens == 1) {
                        j = 0;
                        token1[j] = chr;
                        token1[j + 1] = '\0';
                    } else if (number_of_tokens == 2) {
                        j = 0;
                        token2[j] = chr;
                        token2[j + 1] = '\0';
                    }
                    state = 2; // chars
                }
                break;

            case 1:
                if (chr == '\n') {
                } else if ((chr == ' ') || (chr == '\t')) {
                    state = 1; // space
                } else if (chr == '"') {
                    number_of_tokens++;
                    j = -1;
                    state = 3; // inside double quotes
                } else {
                    number_of_tokens++;
                    if (number_of_tokens == 1) {
                        j = 0;
                        token1[j] = chr;
                        token1[j + 1] = '\0';
                    } else if (number_of_tokens == 2) {
                        j = 0;
                        token2[j] = chr;
                        token2[j + 1] = '\0';
                    }
                    state = 2; // chars
                }
                break;

            case 2:
                if (chr == '\n') {
                } else if ((chr == ' ') || (chr == '\t')) {
                    state = 1; // space
                } else if (chr == '"') {
                    state = 0; // init
                } else {
                    if (number_of_tokens == 1) {
                        j++;
                        token1[j] = chr;
                        token1[j + 1] = '\0';
                    } else if (number_of_tokens == 2) {
                        j++;
                        token2[j] = chr;
                        token2[j + 1] = '\0';
                    }
                    state = 2; // chars
                }
                break;

            case 3:
                if (chr == '\n') {
                } else if (chr == '"') {
                    state = 0; // init
                } else {
                    if (number_of_tokens == 1) {
                        j++;
                        token1[j] = chr;
                        token1[j + 1] = '\0';
                    } else if (number_of_tokens == 2) {
                        j++;
                        token2[j] = chr;
                        token2[j + 1] = '\0';
                    }
                    state = 3; // chars
                }
                break;

            default:
                break;
        }
        i++;
        chr = instring[i];
    }



    return number_of_tokens;
}

void TabularData::UpdateStatusFromCSVFile(char* csvfilename) {
    FILE *ifp;
    char fieldvalue[MAX_STRING_SIZE];

    int status_index = NumberOfDimensions + 1;

    if ((ifp = fopen(csvfilename, "r")) == NULL) {
        logger->error(1, "Unable to open CSV file: %s", csvfilename);
    }

    while (getline(ifp, InputBuffer)) {
        int number_of_tokens = getTokens(InputBuffer, ',');

        if (number_of_tokens != (NumberOfDimensions + 2)) {
            logger->error(1, "Incorrect number of tokens (%s) in CSV file: %s", InputBuffer, csvfilename);
        }

        for (int i = 0; i < NumberOfDimensions; i++) {
            strcpy(fieldvalue, Tokens[i]);
            int number_of_entries = NumberOfKeyEntries[i];

            int key_entry = -1;

            for (int l = 0; l < number_of_entries; l++) {
                if (strcmp(fieldvalue, KeyFields[i][l].key_value) == 0) {
                    key_entry = KeyFields[i][l].key_index;
                }
            }

            if ((key_entry >= 0) && (key_entry < number_of_entries)) {
                CurrentKeyIndex[i] = key_entry;
            } else {
                logger->error(1, "Invalid key (%s) in CSV file: %s", fieldvalue, csvfilename);
            }
        }

        int cell_index = GetCellNumber();

        if ((cells[cell_index].status == 's') || (cells[cell_index].status == 'S')) {
            if ((Tokens[status_index][0] == 'm') || (Tokens[status_index][0] == 'M')) {
                cells[cell_index].status = 'm';
            }
        }
    }

    fclose(ifp);
}

bool TabularData::ReadMetaData() {
    FILE *ifp;
    bool rc;
    char fieldname1[MAX_STRING_SIZE];
    char fieldname2[MAX_STRING_SIZE];
    int param1;
    int param2;
    int param3;
    int param4;
    int param5;
    int field_number;
    int number_of_hierarchies;
    int i;

    logger->log(3, "Reading metadata");

    rc = false;
    field_number = -1;

    if ((ifp = fopen(metadatafilename, "r")) != NULL) {
        while (getline(ifp, InputBuffer)) {
            if (sscanf(InputBuffer, "%s %d %d %d %d %d", fieldname1, &param1, &param2, &param3, &param4, &param5) == 6) {
                logger->log(3, "  Field attribute %s", fieldname1);

                if (AttributeMatches(fieldname1, "<HIERLEVELS>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].HierarchicalLevels[0] = param1;
                        MetaDataFields[field_number].HierarchicalLevels[1] = param2;
                        MetaDataFields[field_number].HierarchicalLevels[2] = param3;
                        MetaDataFields[field_number].HierarchicalLevels[3] = param4;
                        MetaDataFields[field_number].HierarchicalLevels[4] = param5;

                        number_of_hierarchies = 0;

                        for (i = 0; i < 5; i++) {
                            if (MetaDataFields[field_number].HierarchicalLevels[i] != 0) {
                                number_of_hierarchies++;
                            }
                        }
                        MetaDataFields[field_number].NumberOfHierarchies = number_of_hierarchies;
                    }
                } else if (AttributeMatches(fieldname1, "<DISTANCE>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Distance[0] = param1;
                        MetaDataFields[field_number].Distance[1] = param2;
                        MetaDataFields[field_number].Distance[2] = param3;
                        MetaDataFields[field_number].Distance[3] = param4;
                        MetaDataFields[field_number].Distance[4] = param5;
                    }
                }
            } else if (sscanf2(InputBuffer, "%s %s", fieldname1, fieldname2) == 2) //write own copy that reads strings
            {
                //logger->log(5, "Field attribute %s", fieldname1);

                if (AttributeMatches(fieldname1, "<SEPARATOR>")) {
                    if (fieldname2[0] != '"') {
                        metadata_separator_char = fieldname2[0];
                    } else {
                        metadata_separator_char = fieldname2[1];
                    }
                } else if (AttributeMatches(fieldname1, "<SAFE>")) {
                    if (fieldname2[0] != '"') {
                        metadata_safe_char = fieldname2[0];
                    } else {
                        metadata_safe_char = fieldname2[1];
                    }
                } else if (AttributeMatches(fieldname1, "<UNSAFE>")) {
                    if (fieldname2[0] != '"') {
                        metadata_unsafe_char = fieldname2[0];
                    } else {
                        metadata_unsafe_char = fieldname2[1];
                    }
                } else if (AttributeMatches(fieldname1, "<PROTECT>")) {
                    if (fieldname2[0] != '"') {
                        metadata_protect_char = fieldname2[0];
                    } else {
                        metadata_protect_char = fieldname2[1];
                    }
                } else if (AttributeMatches(fieldname1, "<TOTCODE>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        strcpy_without_quotes(MetaDataFields[field_number].TotalCode, "TOTAL");
                        logger->log(3, "Total code %s", MetaDataFields[field_number].TotalCode);
                    }
                } else if (AttributeMatches(fieldname1, "<HIERCODELIST>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        char hierCodeFile[MAX_FILENAME_SIZE];
                        get_directory(hierCodeFile, metadatafilename);
                        strcat(hierCodeFile, fieldname2);
                        strcpy_without_quotes(MetaDataFields[field_number].HierCodeList, hierCodeFile);
                        MetaDataFields[field_number].HierCodeListPresent = true;
                        MetaDataFields[field_number].Hierarchical = true;
                        logger->log(3, "Hierarchical code list file %s", MetaDataFields[field_number].HierCodeList);
                    }
                } else if (AttributeMatches(fieldname1, "<HIERLEADSTRING>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        strcpy_without_quotes(MetaDataFields[field_number].HierLeadString, fieldname2);
                        MetaDataFields[field_number].HierLeadStringPresent = true;
                        logger->log(3, "Hierarchical lead string %s", MetaDataFields[field_number].HierLeadString);
                    }
                } else if (AttributeMatches(fieldname1, "<NUMERIC>")) {
                    if (AttributeMatches(fieldname2, "<SHADOW>")) {
                        if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                            MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_SHADOW;
                            MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                            NumberOfDataFields++;
                        }
                    } else if (AttributeMatches(fieldname2, "<COST>")) {
                        if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                            MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_COST;
                            MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                            NumberOfDataFields++;
                        }
                    } else if (AttributeMatches(fieldname2, "<LOWERPL>")) {
                        if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                            MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_LOWERPL;
                            MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                            NumberOfDataFields++;
                        }
                    } else if (AttributeMatches(fieldname2, "<UPPERPL>")) {
                        if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                            MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_UPPERPL;
                            MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                            NumberOfDataFields++;
                        }
                    }
                } else {
                }
            } else if (sscanf(InputBuffer, "%s", fieldname1) == 1) {
                //logger->log(5, "Field attribute %s", fieldname1);

                if (AttributeMatches(fieldname1, "<RECODEABLE>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_RECODEABLE;
                        MetaDataFields[field_number].KeyIndex = NumberOfKeyFields;
                        NumberOfKeyFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<HIERARCHICAL>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Hierarchical = true;

                        if (NumberOfKeyFields > 0) {
                            KeyIsHierarchical[NumberOfKeyFields - 1] = true;
                        }
                    }
                } else if (AttributeMatches(fieldname1, "<NUMERIC>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                        logger->log(3, "Response field number %d", field_number);
                    }
                } else if (AttributeMatches(fieldname1, "<NUMERIC><SHADOW>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_SHADOW;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<NUMERIC><COST>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_COST;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<NUMERIC><LOWERPL>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_LOWERPL;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<NUMERIC><UPPERPL>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_NUMERIC_UPPERPL;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<FREQUENCY>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_FREQUENCY;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<MAXSCORE>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_MAXSCORE;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                    }
                } else if (AttributeMatches(fieldname1, "<STATUS>")) {
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        MetaDataFields[field_number].Type = FIELD_TYPE_STATUS;
                        MetaDataFields[field_number].DataIndex = NumberOfDataFields;
                        NumberOfDataFields++;
                        logger->log(3, "Status field number %d", field_number);
                    }
                } else {
                    logger->log(3, "Variable name %s", fieldname1);
                    field_number++;
                    if ((field_number >= 0) && (field_number < MAX_NO_METADATA_FIELDS)) {
                        strcpy(MetaDataFields[field_number].Name, fieldname1);
                    }
                }
            }
        }

        rc = true;

        fclose(ifp);
    }

    NumberOfFields = field_number + 1;

    return rc;
}

bool TabularData::ValidMetadata() {
    bool rc;
    int i;

    rc = true;

    if (NumberOfKeyFields == 0) {
        rc = false;
    }

    if (NumberOfDataFields == 0) {
        rc = false;
    }

    for (i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_UNKNOWN) {
            rc = false;
        }
    }

    return rc;
}

bool TabularData::AddKeyToIndex(int keyindex, char *key, int hierarchy) {
    bool rc;
    int Number_of_entries;
    int i;

    rc = true;
    Number_of_entries = NumberOfKeyEntries[keyindex];

    if (Number_of_entries == 0) {
        strcpy(KeyFields[keyindex][Number_of_entries].key_value, key);
        KeyFields[keyindex][Number_of_entries].hierachy_level = hierarchy;
        Number_of_entries++;
    } else {
        i = 0;
        while ((i < Number_of_entries) && (strcmp(KeyFields[keyindex][i].key_value, key) != 0)) {
            i++;
        }

        if (i == Number_of_entries) // This is a new key field so add it.
        {
            if (Number_of_entries < MAX_NO_KEY_ENTRIES) {
                strcpy(KeyFields[keyindex][Number_of_entries].key_value, key);
                KeyFields[keyindex][Number_of_entries].hierachy_level = hierarchy;
                Number_of_entries++;
            } else {
                rc = false;
            }
        }
    }

    NumberOfKeyEntries[keyindex] = Number_of_entries;

    return rc;
}

void TabularData::SortKeyFields() {
    int i;
    int j;
    int k;
    int l;
    char keyvalue[MAX_KEY_SIZE];
    int hierarchy;

    for (i = 0; i < NumberOfFields; i++) {
        if ((MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) && (MetaDataFields[i].Hierarchical) && (MetaDataFields[i].NumberOfHierarchies > 0) && (!MetaDataFields[i].HierCodeListPresent)) {
            j = MetaDataFields[i].KeyIndex;

            for (k = 0; k < NumberOfKeyEntries[j]; k++) {
                for (l = k; l < NumberOfKeyEntries[i]; l++) {
                    if (strcmp(KeyFields[j][k].key_value, KeyFields[j][l].key_value) > 0) {
                        strcpy(keyvalue, KeyFields[j][k].key_value);
                        strcpy(KeyFields[j][k].key_value, KeyFields[j][l].key_value);
                        strcpy(KeyFields[j][l].key_value, keyvalue);

                        hierarchy = KeyFields[j][k].hierachy_level;
                        KeyFields[j][k].hierachy_level = KeyFields[j][l].hierachy_level;
                        KeyFields[j][l].hierachy_level = hierarchy;
                    }
                }
            }
        }
    }
}

int TabularData::string_length(char *s) {
    int rc;

    rc = 0;

    while (s[rc] != '\0') {
        rc++;
    }

    return rc;
}

bool TabularData::lead_match(char *lead, char *s) {
    bool rc;
    int i;
    char c;

    rc = true;

    i = 0;
    while ((c = lead[i]) != '\0') {
        if (c != s[i]) {
            rc = false;
        }
        i++;
    }

    return rc;
}

bool TabularData::AddKeyFieldHierarchies() {
    int i;
    int j;
    int k;
    int l;
    int m;
    int level;
    int nos_entries_before_hierarchy_added;
    int size_of_key;
    char keyvalue[MAX_KEY_SIZE];
    char c;
    FILE *ifp;
    bool rc;
    int number_of_leadstrings_found;
    int lead_length;

    rc = true;

    /* For HIERLEVELS fields add Hierarchy */

    for (i = 0; i < NumberOfFields; i++) {
        if ((MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) && (MetaDataFields[i].Hierarchical) && (MetaDataFields[i].NumberOfHierarchies > 0)) {
            j = MetaDataFields[i].KeyIndex;
            nos_entries_before_hierarchy_added = NumberOfKeyEntries[j];

            level = MetaDataFields[i].NumberOfHierarchies;

            size_of_key = 0;

            for (l = 0; l < MetaDataFields[i].NumberOfHierarchies - 1; l++) {
                size_of_key = size_of_key + MetaDataFields[i].HierarchicalLevels[l];

                for (k = 0; k < nos_entries_before_hierarchy_added; k++) {
                    m = 0;

                    while ((c = KeyFields[j][k].key_value[m]) != '\0') {
                        if (m < size_of_key) {
                            keyvalue[m] = c;
                        } else {
                            keyvalue[m] = ' ';
                        }
                        m++;
                    }
                    keyvalue[m] = '\0';

                    if (AddKeyToIndex(j, keyvalue, level)) {
                    }
                }

                level--;
            }

            KeyHierarchyLevels[MetaDataFields[i].KeyIndex] = MetaDataFields[i].NumberOfHierarchies;
        }
    }

    /* For HIERCODELEVELS fields add Hierarchy */

    for (i = 0; i < NumberOfFields; i++) {
        if ((MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) && (MetaDataFields[i].Hierarchical) && (MetaDataFields[i].HierCodeListPresent)) {
            j = MetaDataFields[i].KeyIndex;
            nos_entries_before_hierarchy_added = NumberOfKeyEntries[j];

            k = 0;

            // Add Total code

            if (MetaDataFields[i].TotalCode[0] == '\0') {
                strcpy(MetaDataFields[i].TotalCode, "TOTAL");
            }

            strcpy(KeyFields[j][k].key_value, MetaDataFields[i].TotalCode);
            KeyFields[j][k].hierachy_level = 2;
            k++;

            // Read in hierarchy names

            if ((ifp = fopen(MetaDataFields[i].HierCodeList, "r")) != NULL) {
                while (getline(ifp, InputBuffer)) {
                    if (strcmp(InputBuffer, MetaDataFields[i].TotalCode) != 0) {
                        strcpy(KeyFields[j][k].key_value, InputBuffer);
                        KeyFields[j][k].hierachy_level = 1;
                        k++;
                    }
                }

                NumberOfKeyEntries[j] = k;
                fclose(ifp);

                number_of_leadstrings_found = 0;

                for (k = 0; k < NumberOfKeyEntries[j]; k++) {
                    if (lead_match(MetaDataFields[i].HierLeadString, KeyFields[j][k].key_value)) {
                        number_of_leadstrings_found++;
                    }
                }

                while (number_of_leadstrings_found > 0) {
                    lead_length = string_length(MetaDataFields[i].HierLeadString);

                    for (k = 0; k < NumberOfKeyEntries[j]; k++) {
                        if (lead_match(MetaDataFields[i].HierLeadString, KeyFields[j][k].key_value)) {
                            l = lead_length;

                            while ((c = KeyFields[j][k].key_value[l]) != '\0') {
                                KeyFields[j][k].key_value[l - lead_length] = KeyFields[j][k].key_value[l];
                                l++;
                            }

                            KeyFields[j][k].key_value[l - lead_length] = '\0';
                        } else {
                            KeyFields[j][k].hierachy_level++;

                            if (MetaDataFields[i].NumberOfHierarchies < KeyFields[j][k].hierachy_level) {
                                MetaDataFields[i].NumberOfHierarchies = KeyFields[j][k].hierachy_level;
                            }
                        }
                    }

                    number_of_leadstrings_found = 0;

                    for (k = 0; k < NumberOfKeyEntries[j]; k++) {
                        if (lead_match(MetaDataFields[i].HierLeadString, KeyFields[j][k].key_value)) {
                            number_of_leadstrings_found++;
                        }
                    }
                }
            } else {
                logger->error(1, "HIERCODELEVELS file not found: %s", MetaDataFields[i].HierCodeList);
            }

            KeyHierarchyLevels[MetaDataFields[i].KeyIndex] = MetaDataFields[i].NumberOfHierarchies;
        }
    }

    return rc;
}

void TabularData::AddKeyFieldIndexes() {
    int i;
    int j;
    int k;

    for (i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            j = MetaDataFields[i].KeyIndex;

            for (k = 0; k < NumberOfKeyEntries[j]; k++) {
                KeyFields[j][k].key_index = k;
            }
        }
    }

}

void TabularData::AddTotalCodesToIndex() {
    int i;
    char keyvalue[MAX_KEY_SIZE];

    for (i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            if (MetaDataFields[i].TotalCode[0] == '\0') {
                strcpy(MetaDataFields[i].TotalCode, "TOTAL");
            }

            strcpy(keyvalue, MetaDataFields[i].TotalCode);

            if (AddKeyToIndex(MetaDataFields[i].KeyIndex, keyvalue, 2)) {
                logger->log(3, "Added %s to index %d", keyvalue, MetaDataFields[i].KeyIndex);
            }
        }
    }
}

bool TabularData::ParseKeyFields() {
    bool rc;
    FILE *ifp;
    int i;
    char keyvalue[MAX_KEY_SIZE];
    int number_of_tokens;

    rc = false;

    /* Add the Missing Indicators to the Indexes */

    /*
    for (i=0; i<NumberOfFields; i++)
    {
            if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE)
            {
                    if (AddKeyToIndex(MetaDataFields[i].KeyIndex, MetaDataFields[i].MissingIndicator, 1))
                    {
                    }
            }
    }
     */

    /* Add all other possible values to the Indexes */

    if ((ifp = fopen(tabdatafilename, "r")) != NULL) {
        rc = true;

        while (getline(ifp, InputBuffer)) {
            for (i = 0; i < NumberOfFields; i++) {
                if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
                    /* Get the Key value */

                    number_of_tokens = getTokens(InputBuffer, metadata_separator_char);

                    if (number_of_tokens != NumberOfFields) {
                        logger->error(1, "Number_of_tokens (%d) != NumberOfFields (%d)", number_of_tokens, NumberOfFields);
                    } else if (strcmp(MetaDataFields[i].Name, Tokens[i]) == 0) {
                    } else {
                        strcpy(keyvalue, Tokens[i]);

                        /* Save the Key value */

                        if (AddKeyToIndex(MetaDataFields[i].KeyIndex, keyvalue, 1)) {
                        } else {
                            rc = false;
                        }
                    }
                }
            }
        }
        fclose(ifp);
    } else {
        logger->error(1, "Tab data file not found: %s", tabdatafilename);
    }

    return rc;
}

void TabularData::PrintKeyFileds(const char* outfilename) {
    FILE *ofp;
    int i;
    int j;
    int k;

    if ((ofp = fopen(outfilename, "w")) != NULL) {
        fprintf(ofp, "\n");
        fprintf(ofp, "\nKey field values");
        fprintf(ofp, "\n----------------");
        fprintf(ofp, "\n");
        fprintf(ofp, "\n");

        for (i = 0; i < NumberOfFields; i++) {
            if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
                k = MetaDataFields[i].KeyIndex;

                fprintf(ofp, "\nKEY FIELD: %s", MetaDataFields[i].Name);
                fprintf(ofp, "\n");
                fprintf(ofp, "\n");
                fprintf(ofp, "Number of unique key values = %d", NumberOfKeyEntries[k]);
                fprintf(ofp, "\n");

                fprintf(ofp, "\nValue\tHierarchy\tIndex");

                for (j = 0; j < NumberOfKeyEntries[k]; j++) {
                    fprintf(ofp, "\n%s\t%d\t\t%d ", KeyFields[k][j].key_value, KeyFields[k][j].hierachy_level, KeyFields[k][j].key_index);
                }

                fprintf(ofp, "\n");
                fprintf(ofp, "\n");
            }
        }
        fclose(ofp);
    } else {
        logger->error(1, "Unable to open output file: %s", outfilename);
    }
}

void TabularData::DisplayFieldInformation() {
    logger->log(3, "%d key fields:", NumberOfKeyFields);

    for (int i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
            if (MetaDataFields[i].Hierarchical) {
                logger->log(3, "%s (size = %d, hierarchical)", MetaDataFields[i].Name, NumberOfKeyEntries[MetaDataFields[i].KeyIndex]);
            } else {
                logger->log(3, "%s (size = %d)", MetaDataFields[i].Name, NumberOfKeyEntries[MetaDataFields[i].KeyIndex]);
            }
        }
    }

    logger->log(3, "%d data fields", NumberOfDataFields);

    logger->log(3, "Numeric fields:");
    for (int i = 0; i < NumberOfFields; i++) {
        if (MetaDataFields[i].Type == FIELD_TYPE_NUMERIC) {
            logger->log(3, "%s", MetaDataFields[i].Name);
        }
    }
}

bool TabularData::ReadCellData() {
    bool rc;
    FILE *ifp;
    int i;
//    int j;
//    int k;
    int l;
    double magnitude_data = 0.0;
    char status_data[MAX_STRING_SIZE];
    int cell_index;
    char fieldvalue[MAX_STRING_SIZE];
//    int fieldstart;
//    int fieldend;
    int key_index;
    int key_entry;
    int number_of_entries;
    int number_of_tokens;

    NumberOfRecordsRead = 0;
    rc = false;

    logger->log(3, "Number of fields = %d", NumberOfFields);
    logger->log(3, "Field Types:");

    for (i = 0; i < NumberOfFields; i++) {
        logger->log(3, "%d", MetaDataFields[i].Type);
    }

    if ((ifp = fopen(tabdatafilename, "r")) != NULL) {
        rc = true;
        status_data[0] = 's';

        while (getline(ifp, InputBuffer)) {
            status_data[0] = 's';

            number_of_tokens = getTokens(InputBuffer, metadata_separator_char);

            if (number_of_tokens != NumberOfFields) {
                logger->error(1, "Number_of_tokens (%d) != NumberOfFields (%d)", number_of_tokens, NumberOfFields);
            }

            rc = true;

            for (i = 0; i < NumberOfFields; i++) {
                if (MetaDataFields[i].UseThisIndex) {
                    strcpy(fieldvalue, Tokens[i]);

                    //logger->log(5, "Field value[%d] = %s", i, fieldvalue);

                    if (MetaDataFields[i].Type == FIELD_TYPE_RECODEABLE) {
                        /* Get the index for each dimension */

                        key_index = MetaDataFields[i].KeyIndex;
                        number_of_entries = NumberOfKeyEntries[key_index];

                        key_entry = -1;

                        for (l = 0; l < number_of_entries; l++) {
                            if (strcmp(fieldvalue, KeyFields[key_index][l].key_value) == 0) {
                                key_entry = KeyFields[key_index][l].key_index;
                            }
                        }

                        if ((key_entry >= 0) && (key_entry < number_of_entries)) {
                            CurrentKeyIndex[key_index] = key_entry;
                        } else {
                            /*
                                                                                    if (rc)
                                                                                    {
                                                                                            logger->log(3, "Debug info:");
                                                                                            //logger->log(3, "Fieldstart = %d", fieldstart);
                                                                                            //logger->log(3, "Fieldend = %d", fieldend);
                                                                                            logger->log(3, "Key_entry = %d", key_entry);
                                                                                            logger->log(3, "Number_of_entries = %d", number_of_entries);

                                                                                            for (l=0; l<number_of_entries; l++)
                                                                                            {
                                                                                                    logger->log(3, "%s = %s", fieldvalue, KeyFields[key_index][l].key_value);
                                                                                            }
                                                                                    }

                                                                                    //logger->log(3, "(key_entry >= 0) && (key_entry < number_of_entries) failure");
                             */
                            rc = false;
                        }
                    } else if (MetaDataFields[i].Type == FIELD_TYPE_NUMERIC) {
                        /* Get the magnitude data */

                        if (sscanf(fieldvalue, "%lf", &magnitude_data) == 1) {
                        } else {
                            logger->error(1, "Unable to read magnitude data");
                        }
                    } else if (MetaDataFields[i].Type == FIELD_TYPE_STATUS) {
                        /* Get the status data */

                        if (sscanf(fieldvalue, "%s", status_data) == 1) {
                            //logger->log(5, "Status = %s", status_data);

                            // Map status data

                            switch (status_data[0]) {
                                case 'U':
                                case 'u':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                    status_data[0] = 'u';
                                    break;

                                case 'P':
                                case 'p':
                                case 'z':
                                case 'Z':
                                    status_data[0] = 'z';
                                    break;

                                case 'S':
                                case 's':
                                case '2':
                                    status_data[0] = 's';
                                    break;

                                case '1':
                                    switch (status_data[1]) {
                                        case '\0':
                                            status_data[0] = 's';
                                            break;

                                        case '1':
                                        case '2':
                                            status_data[0] = 'm';
                                            break;

                                        case '0':
                                        case '3':
                                        case '4':
                                            status_data[0] = 'z';
                                            break;

                                        default:
                                            status_data[0] = 'Y';
                                            break;
                                    }
                                    break;

                                default:
                                    status_data[0] = 'X';
                                    break;
                            }
                            status_data[1] = '\0';
                        } else {
                            logger->error(1, "Unable to read status data");
                        }
                    }
                }
            }
            /* Save the cell data */

            if (rc) {
                cell_index = GetCellNumber();

                cells[cell_index].nominal_value = cells[cell_index].nominal_value + magnitude_data;
                cells[cell_index].frequency++;

                if (cells[cell_index].largest_value_1 < magnitude_data) {
                    cells[cell_index].largest_value_3 = cells[cell_index].largest_value_2;
                    cells[cell_index].largest_value_2 = cells[cell_index].largest_value_1;
                    cells[cell_index].largest_value_1 = magnitude_data;
                } else if (cells[cell_index].largest_value_2 < magnitude_data) {
                    cells[cell_index].largest_value_3 = cells[cell_index].largest_value_2;
                    cells[cell_index].largest_value_2 = magnitude_data;
                } else if (cells[cell_index].largest_value_3 < magnitude_data) {
                    cells[cell_index].largest_value_3 = magnitude_data;
                }

                cells[cell_index].status = status_data[0];

                NumberOfRecordsRead++;
            }
        }

        fclose(ifp);
    } else {
        logger->error(1, "Tab Data File not found: %s", tabdatafilename);
    }

    rc = true;

    return rc;
}

void TabularData::Print2dMapping(const char* mapfilename) {
    int n;
    int i;
    int j;
    int k;
    int cell_index;
    FILE *ofp;
    int d[MAX_NO_KEY_FIELDS];
    char KeyName[MAX_NO_KEY_FIELDS][MAX_STRING_SIZE];

    if ((ofp = fopen(mapfilename, "w")) != NULL) {
        if (NumberOfDimensions == 2) {
            // Find which are being used

            j = 0;
            for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
                if (KeyIndexInUse[i]) {
                    d[j] = i;
                    j++;
                }
            }

            // Print out the mapping

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                strcpy(KeyName[1], KeyFields[d[1]][j].key_value);

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    strcpy(KeyName[0], KeyFields[d[0]][k].key_value);

                    CurrentKeyIndex[d[1]] = j;
                    CurrentKeyIndex[d[0]] = k;
                    cell_index = GetCellNumber();

                    fprintf(ofp, "%d", cell_index);

                    for (n = 0; n < NumberOfDimensions; n++) {
                        fprintf(ofp, ",%s", KeyName[n]);
                    }
                    fprintf(ofp, "\n");
                }
            }
        }
        fclose(ofp);
    }
}

void TabularData::Print3dMapping(const char* mapfilename) {
    int z;
    int i;
    int j;
    int k;
    int cell_index;
    FILE *ofp;
    int d[MAX_NO_KEY_FIELDS];
    char KeyName[MAX_NO_KEY_FIELDS][MAX_STRING_SIZE];

    if ((ofp = fopen(mapfilename, "w")) != NULL) {
        if (NumberOfDimensions == 3) {
            // Find which are being used

            j = 0;
            for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
                if (KeyIndexInUse[i]) {
                    d[j] = i;
                    j++;
                }
            }

            // Print out the mapping

            for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
                strcpy(KeyName[2], KeyFields[d[2]][i].key_value);

                for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                    strcpy(KeyName[1], KeyFields[d[1]][j].key_value);

                    for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                        strcpy(KeyName[0], KeyFields[d[0]][k].key_value);

                        CurrentKeyIndex[d[2]] = i;
                        CurrentKeyIndex[d[1]] = j;
                        CurrentKeyIndex[d[0]] = k;
                        cell_index = GetCellNumber();

                        fprintf(ofp, "%d", cell_index);

                        for (z = 0; z < NumberOfDimensions; z++) {
                            fprintf(ofp, ",%s", KeyName[z]);
                        }
                        fprintf(ofp, "\n");
                    }
                }
            }

        }
        fclose(ofp);
    }
}

void TabularData::Print4dMapping(const char* mapfilename) {
    int z;
    int i;
    int j;
    int k;
    int l;
    int cell_index;
    FILE *ofp;
    int d[MAX_NO_KEY_FIELDS];
    char KeyName[MAX_NO_KEY_FIELDS][MAX_STRING_SIZE];

    if ((ofp = fopen(mapfilename, "w")) != NULL) {
        if (NumberOfDimensions == 4) {
            // Find which are being used

            j = 0;
            for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
                if (KeyIndexInUse[i]) {
                    d[j] = i;
                    j++;
                }
            }

            // Print out the mapping

            for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                strcpy(KeyName[3], KeyFields[d[3]][l].key_value);

                for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
                    strcpy(KeyName[2], KeyFields[d[2]][i].key_value);

                    for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                        strcpy(KeyName[1], KeyFields[d[1]][j].key_value);

                        for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                            strcpy(KeyName[0], KeyFields[d[0]][k].key_value);

                            CurrentKeyIndex[d[3]] = l;
                            CurrentKeyIndex[d[2]] = i;
                            CurrentKeyIndex[d[1]] = j;
                            CurrentKeyIndex[d[0]] = k;
                            cell_index = GetCellNumber();

                            fprintf(ofp, "%d", cell_index);

                            for (z = 0; z < NumberOfDimensions; z++) {
                                fprintf(ofp, ",%s", KeyName[z]);
                            }
                            fprintf(ofp, "\n");
                        }
                    }
                }
            }

        }
        fclose(ofp);
    }
}

void TabularData::update4dWeightings() {
    int i;
    int j;
    int k;
    int l;
    int cell_index;
    int d[MAX_NO_KEY_FIELDS];
    char KeyName[MAX_NO_KEY_FIELDS][MAX_STRING_SIZE];

    if (NumberOfDimensions == 4) {
        // Find which are being used

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        // update weightings

        for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
            strcpy(KeyName[3], KeyFields[d[3]][l].key_value);

            for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
                strcpy(KeyName[2], KeyFields[d[2]][i].key_value);

                for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                    strcpy(KeyName[1], KeyFields[d[1]][j].key_value);

                    for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                        strcpy(KeyName[0], KeyFields[d[0]][k].key_value);

                        CurrentKeyIndex[d[3]] = l;
                        CurrentKeyIndex[d[2]] = i;
                        CurrentKeyIndex[d[1]] = j;
                        CurrentKeyIndex[d[0]] = k;
                        cell_index = GetCellNumber();

                        if (strcmp(KeyName[3], "wp") == 0) {
                            cells[cell_index].loss_of_information_weight = 0.0001;
                        }

                        if (strcmp(KeyName[3], "wp1") == 0) {
                            cells[cell_index].loss_of_information_weight = 0.0001;
                        }
                    }
                }
            }
        }
    }
}

void TabularData::Print5dMapping(const char* mapfilename) {
    int z;
    int i;
    int j;
    int k;
    int l;
    int m;
    int cell_index;
    FILE *ofp;
    int d[MAX_NO_KEY_FIELDS];
    char KeyName[MAX_NO_KEY_FIELDS][MAX_STRING_SIZE];

    if ((ofp = fopen(mapfilename, "w")) != NULL) {
        if (NumberOfDimensions == 5) {
            // Find which are being used

            j = 0;
            for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
                if (KeyIndexInUse[i]) {
                    d[j] = i;
                    j++;
                }
            }

            // Print out the mapping

            for (m = 0; m < NumberOfKeyEntries[d[4]]; m++) {
                strcpy(KeyName[4], KeyFields[d[4]][m].key_value);

                for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                    strcpy(KeyName[3], KeyFields[d[3]][l].key_value);

                    for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
                        strcpy(KeyName[2], KeyFields[d[2]][i].key_value);

                        for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                            strcpy(KeyName[1], KeyFields[d[1]][j].key_value);

                            for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                                strcpy(KeyName[0], KeyFields[d[0]][k].key_value);

                                CurrentKeyIndex[d[4]] = m;
                                CurrentKeyIndex[d[3]] = l;
                                CurrentKeyIndex[d[2]] = i;
                                CurrentKeyIndex[d[1]] = j;
                                CurrentKeyIndex[d[0]] = k;
                                cell_index = GetCellNumber();

                                fprintf(ofp, "%d", cell_index);

                                for (z = 0; z < NumberOfDimensions; z++) {
                                    fprintf(ofp, ",%s", KeyName[z]);
                                }
                                fprintf(ofp, "\n");
                            }
                        }
                    }
                }
            }

        }
        fclose(ofp);
    }
}

void TabularData::Print6dMapping(const char* mapfilename) {
    int z;
    int i;
    int j;
    int k;
    int l;
    int m;
    int n;
    int cell_index;
    FILE *ofp;
    int d[MAX_NO_KEY_FIELDS];
    char KeyName[MAX_NO_KEY_FIELDS][MAX_STRING_SIZE];

    if ((ofp = fopen(mapfilename, "w")) != NULL) {
        if (NumberOfDimensions == 6) {
            // Find which are being used

            j = 0;
            for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
                if (KeyIndexInUse[i]) {
                    d[j] = i;
                    j++;
                }
            }

            // Print out the mapping

            for (n = 0; n < NumberOfKeyEntries[d[5]]; n++) {
                strcpy(KeyName[5], KeyFields[d[5]][n].key_value);

                for (m = 0; m < NumberOfKeyEntries[d[4]]; m++) {
                    strcpy(KeyName[4], KeyFields[d[4]][m].key_value);

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        strcpy(KeyName[3], KeyFields[d[3]][l].key_value);

                        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
                            strcpy(KeyName[2], KeyFields[d[2]][i].key_value);

                            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                                strcpy(KeyName[1], KeyFields[d[1]][j].key_value);

                                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                                    strcpy(KeyName[0], KeyFields[d[0]][k].key_value);

                                    CurrentKeyIndex[d[5]] = n;
                                    CurrentKeyIndex[d[4]] = m;
                                    CurrentKeyIndex[d[3]] = l;
                                    CurrentKeyIndex[d[2]] = i;
                                    CurrentKeyIndex[d[1]] = j;
                                    CurrentKeyIndex[d[0]] = k;
                                    cell_index = GetCellNumber();

                                    fprintf(ofp, "%d", cell_index);

                                    for (z = 0; z < NumberOfDimensions; z++) {
                                        fprintf(ofp, ",%s", KeyName[z]);
                                    }
                                    fprintf(ofp, "\n");
                                }
                            }
                        }
                    }
                }
            }

        }
        fclose(ofp);
    }
}

void TabularData::SetTotalsFor1dTable() {
    int i;
    int j;
    int cell_index;
    int total_index;
    int d[MAX_NO_KEY_FIELDS];
    int level;
    double total;
    int freq;
    int count;

    if (NumberOfDimensions == 1) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        CurrentKeyIndex[d[0]] = 0;
        total_index = GetCellNumber();

        total = 0.0;
        freq = 0;
        count = 0;

        for (i = 1; i < NumberOfKeyEntries[d[0]]; i++) {
            if (KeyFields[d[0]][i].hierachy_level == 1) {
                CurrentKeyIndex[d[0]] = i;
                cell_index = GetCellNumber();
                total = total + cells[cell_index].nominal_value;
                freq = freq + cells[cell_index].frequency;
                count = count + cells[cell_index].number_of_contributing_cells;
            }
        }
        cells[total_index].nominal_value = total;
        cells[total_index].frequency = freq;
        cells[total_index].number_of_contributing_cells = count;
        cell_type[total_index] = MARGIN_CELL;

        if (KeyIsHierarchical[d[0]]) {
            for (level = 1; level < 5; level++) {
                total = 0.0;
                freq = 0;
                count = 0;

                for (i = NumberOfKeyEntries[d[0]] - 1; i >= 1; i--) {
                    CurrentKeyIndex[d[0]] = i;
                    cell_index = GetCellNumber();

                    if (KeyFields[d[0]][i].hierachy_level == level) {
                        total = total + cells[cell_index].nominal_value;
                        freq = freq + cells[cell_index].frequency;
                        count = count + cells[cell_index].number_of_contributing_cells;
                    } else if (KeyFields[d[0]][i].hierachy_level == (level + 1)) {
                        if (cells[cell_index].nominal_value < 0.000001) {
                            cells[cell_index].nominal_value = total;
                            cells[cell_index].frequency = freq;
                            cells[cell_index].number_of_contributing_cells = count;
                        }
                        cell_type[cell_index] = SUB_TOTAL_CELL;
                        total = 0.0;
                        freq = 0;
                        count = 0;
                    }
                }
            }
        }
    }
}

void TabularData::SetTotalsFor2dTable() {
    int i;
    int j;
    int cell_index;
    int total_index;
    int d[MAX_NO_KEY_FIELDS];
    int level;
    double total;
    int freq;
    int count;

    if (NumberOfDimensions == 2) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
            CurrentKeyIndex[d[0]] = j;

            CurrentKeyIndex[d[1]] = 0;
            total_index = GetCellNumber();

            total = 0.0;
            freq = 0;
            count = 0;

            for (i = 1; i < NumberOfKeyEntries[d[1]]; i++) {
                if (KeyFields[d[1]][i].hierachy_level == 1) {
                    CurrentKeyIndex[d[1]] = i;
                    cell_index = GetCellNumber();
                    total = total + cells[cell_index].nominal_value;
                    freq = freq + cells[cell_index].frequency;
                    count = count + cells[cell_index].number_of_contributing_cells;
                }
            }
            cells[total_index].nominal_value = total;
            cells[total_index].frequency = freq;
            cells[total_index].number_of_contributing_cells = count;
            cell_type[total_index] = MARGIN_CELL;

            /* Handle hierarchies */

            if (KeyIsHierarchical[d[1]]) {
                for (level = 1; level < 5; level++) {
                    total = 0.0;
                    freq = 0;
                    count = 0;

                    for (i = NumberOfKeyEntries[d[1]] - 1; i >= 1; i--) {
                        CurrentKeyIndex[d[1]] = i;
                        cell_index = GetCellNumber();

                        if (KeyFields[d[1]][i].hierachy_level == level) {
                            total = total + cells[cell_index].nominal_value;
                            freq = freq + cells[cell_index].frequency;
                            count = count + cells[cell_index].number_of_contributing_cells;
                        } else if (KeyFields[d[1]][i].hierachy_level == (level + 1)) {
                            if (cells[cell_index].nominal_value < 0.00001) {
                                cells[cell_index].nominal_value = total;
                                cells[cell_index].frequency = freq;
                                cells[cell_index].number_of_contributing_cells = count;
                            }
                            cell_type[cell_index] = SUB_TOTAL_CELL;
                            total = 0.0;
                            freq = 0;
                            count = 0;
                        }
                    }
                }
            }
        }

        for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
            CurrentKeyIndex[d[1]] = j;
            CurrentKeyIndex[d[0]] = 0;
            total_index = GetCellNumber();

            total = 0.0;
            freq = 0;
            count = 0;

            for (i = 1; i < NumberOfKeyEntries[d[0]]; i++) {
                if (KeyFields[d[0]][i].hierachy_level == 1) {
                    CurrentKeyIndex[d[0]] = i;
                    cell_index = GetCellNumber();
                    total = total + cells[cell_index].nominal_value;
                    freq = freq + cells[cell_index].frequency;
                    count = count + cells[cell_index].number_of_contributing_cells;
                }
            }
            cells[total_index].nominal_value = total;
            cells[total_index].frequency = freq;
            cells[total_index].number_of_contributing_cells = count;
            cell_type[total_index] = MARGIN_CELL;

            /* Handle hierarchies */

            if (KeyIsHierarchical[d[0]]) {
                for (level = 1; level < 5; level++) {
                    total = 0.0;
                    freq = 0;
                    count = 0;

                    for (i = NumberOfKeyEntries[d[0]] - 1; i >= 1; i--) {
                        CurrentKeyIndex[d[0]] = i;
                        cell_index = GetCellNumber();

                        if (KeyFields[d[0]][i].hierachy_level == level) {
                            total = total + cells[cell_index].nominal_value;
                            freq = freq + cells[cell_index].frequency;
                            count = count + cells[cell_index].number_of_contributing_cells;
                        } else if (KeyFields[d[0]][i].hierachy_level == (level + 1)) {
                            if (cells[cell_index].nominal_value < 0.00001) {
                                cells[cell_index].nominal_value = total;
                                cells[cell_index].frequency = freq;
                                cells[cell_index].number_of_contributing_cells = count;
                            }
                            cell_type[cell_index] = SUB_TOTAL_CELL;
                            total = 0.0;
                            freq = 0;
                            count = 0;
                        }
                    }
                }
            }
        }
    }
}

void TabularData::SetTotalsForAnIndex(int dimension) {
    int i;
    int cell_index;
    int total_index;
    int level;
    double total;
    int freq;
    int count;

    CurrentKeyIndex[dimension] = 0;
    total_index = GetCellNumber();

    total = 0.0;
    freq = 0;
    count = 0;

    for (i = 1; i < NumberOfKeyEntries[dimension]; i++) {
        if (KeyFields[dimension][i].hierachy_level == 1) {
            CurrentKeyIndex[dimension] = i;
            cell_index = GetCellNumber();
            total = total + cells[cell_index].nominal_value;
            freq = freq + cells[cell_index].frequency;
            count = count + cells[cell_index].number_of_contributing_cells;
        }
    }

    //if (cells[total_index].nominal_value < 0.00001)
    {
        cells[total_index].nominal_value = total;
    }
    cells[total_index].frequency = freq;
    cells[total_index].number_of_contributing_cells = count;
    cell_type[total_index] = MARGIN_CELL;

    /* Handle hierarchies */

    if (KeyIsHierarchical[dimension]) {
        for (level = 1; level < 5; level++) {
            total = 0.0;
            freq = 0;
            count = 0;

            for (i = NumberOfKeyEntries[dimension] - 1; i >= 0; i--) {
                CurrentKeyIndex[dimension] = i;
                cell_index = GetCellNumber();

                if (KeyFields[dimension][i].hierachy_level == level) {
                    total = total + cells[cell_index].nominal_value;
                    freq = freq + cells[cell_index].frequency;
                    count = count + cells[cell_index].number_of_contributing_cells;
                } else if (KeyFields[dimension][i].hierachy_level == (level + 1)) {
                    //if ((cells[cell_index].nominal_value < 0.00001)  || (total > 0.1))
                    {
                        //if (cells[cell_index].nominal_value < 0.00001)
                        {
                            cells[cell_index].nominal_value = total;
                        }
                        cells[cell_index].frequency = freq;
                        cells[cell_index].number_of_contributing_cells = count;
                    }
                    cell_type[cell_index] = SUB_TOTAL_CELL;
                    total = 0.0;
                    freq = 0;
                    count = 0;
                }
            }
        }
    }
}

void TabularData::SetTotalsFor3dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];

    if (NumberOfDimensions == 3) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                SetTotalsForAnIndex(d[2]);
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                SetTotalsForAnIndex(d[0]);
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                SetTotalsForAnIndex(d[1]);
            }
        }
    }
}

void TabularData::SetTotalsFor4dTable() {
    int i;
    int j;
    int k;
    int d[MAX_NO_KEY_FIELDS];

    if (NumberOfDimensions == 4) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    SetTotalsForAnIndex(d[3]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    SetTotalsForAnIndex(d[2]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    SetTotalsForAnIndex(d[1]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    SetTotalsForAnIndex(d[0]);
                }
            }
        }
    }
}

void TabularData::SetTotalsFor5dTable() {
    int i;
    int j;
    int k;
    int l;
    int d[MAX_NO_KEY_FIELDS];

    if (NumberOfDimensions == 5) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        CurrentKeyIndex[d[3]] = l;

                        SetTotalsForAnIndex(d[4]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[4]]; i++) {
            CurrentKeyIndex[d[4]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[2]]; l++) {
                        CurrentKeyIndex[d[2]] = l;

                        SetTotalsForAnIndex(d[3]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[4]]; j++) {
                CurrentKeyIndex[d[4]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[1]]; l++) {
                        CurrentKeyIndex[d[1]] = l;

                        SetTotalsForAnIndex(d[2]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[4]]; k++) {
                    CurrentKeyIndex[d[4]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[0]]; l++) {
                        CurrentKeyIndex[d[0]] = l;

                        SetTotalsForAnIndex(d[1]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[4]]; l++) {
                        CurrentKeyIndex[d[4]] = l;

                        SetTotalsForAnIndex(d[0]);
                    }
                }
            }
        }
    }
}

void TabularData::SetTotalsFor6dTable() {
    int i;
    int j;
    int k;
    int l;
    int m;
    int d[MAX_NO_KEY_FIELDS];

    if (NumberOfDimensions == 6) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        CurrentKeyIndex[d[3]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[4]]; m++) {
                            CurrentKeyIndex[d[4]] = m;

                            SetTotalsForAnIndex(d[5]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[5]]; i++) {
            CurrentKeyIndex[d[5]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[2]]; l++) {
                        CurrentKeyIndex[d[2]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[3]]; m++) {
                            CurrentKeyIndex[d[3]] = m;

                            SetTotalsForAnIndex(d[4]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[4]]; i++) {
            CurrentKeyIndex[d[4]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[5]]; j++) {
                CurrentKeyIndex[d[5]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[1]]; l++) {
                        CurrentKeyIndex[d[1]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[2]]; m++) {
                            CurrentKeyIndex[d[2]] = m;

                            SetTotalsForAnIndex(d[3]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[4]]; j++) {
                CurrentKeyIndex[d[4]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[5]]; k++) {
                    CurrentKeyIndex[d[5]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[0]]; l++) {
                        CurrentKeyIndex[d[0]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[1]]; m++) {
                            CurrentKeyIndex[d[1]] = m;

                            SetTotalsForAnIndex(d[2]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[4]]; k++) {
                    CurrentKeyIndex[d[4]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[5]]; l++) {
                        CurrentKeyIndex[d[5]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[0]]; m++) {
                            CurrentKeyIndex[d[0]] = m;

                            SetTotalsForAnIndex(d[1]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[4]]; l++) {
                        CurrentKeyIndex[d[4]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[5]]; m++) {
                            CurrentKeyIndex[d[5]] = m;

                            SetTotalsForAnIndex(d[0]);
                        }
                    }
                }
            }
        }
    }
}

bool TabularData::CountConsistencyEquationsForAnIndex(int dimension) {
    int i;
    int level;
    bool rc;
    bool something_to_print;

    rc = true;

    if ((! KeyIsHierarchical[dimension]) || (KeyHierarchyLevels[dimension] == 0)) {
        nsums++;
    }

    /* Handle hierarchies */

    if (KeyIsHierarchical[dimension]) {
        for (level = 2; level < KeyHierarchyLevels[dimension]; level++) {
            nsums++;
        }

        something_to_print = false;

        for (level = 1; level < KeyHierarchyLevels[dimension] - 1; level++) {
            for (i = 1; i < NumberOfKeyEntries[dimension]; i++) {
                if (KeyFields[dimension][i].hierachy_level == level) {
                    something_to_print = true;
                } else if (KeyFields[dimension][i].hierachy_level == (level + 1)) {
                    if (something_to_print) {
                        nsums++;
                        something_to_print = false;
                    }
                }
            }
            if (something_to_print) {
                nsums++;
                something_to_print = false;
            }
        }
    }

    return rc;
}

void TabularData::save_consistency_equation() {
    int j;
    int size;

    if (eqtn_number < nsums) {
        size = consistency_eqtn.size_of_eqtn;

        consistency_eqtns[eqtn_number].RHS = consistency_eqtn.RHS;
        consistency_eqtns[eqtn_number].size_of_eqtn = consistency_eqtn.size_of_eqtn;

        if ((size > 1) && (consistency_eqtn.plus_or_minus[0] == -1)) {
            consistency_eqtns[eqtn_number].cell_number = new int[size];
            consistency_eqtns[eqtn_number].plus_or_minus = new int[size];

            for (j = 0; j < size; j++) {
                consistency_eqtns[eqtn_number].cell_number[j] = consistency_eqtn.cell_number[j];
                consistency_eqtns[eqtn_number].plus_or_minus[j] = consistency_eqtn.plus_or_minus[j];
            }
        }
        eqtn_number++;
    }
}

bool TabularData::PrintConsistencyEquationsForAnIndex(int dimension) {
    int i;
    int cell_index;
    int total_index;
    int level;
    int size;
    bool rc;
    bool something_to_print;

    rc = true;

    if ((! KeyIsHierarchical[dimension]) || (KeyHierarchyLevels[dimension] == 0)) {
        CurrentKeyIndex[dimension] = 0;
        total_index = GetCellNumber();

        consistency_eqtn.RHS = 0.0;
        consistency_eqtn.size_of_eqtn = 1;
        consistency_eqtn.cell_number [0] = total_index;
        consistency_eqtn.plus_or_minus[0] = -1;

        for (i = 1; i < NumberOfKeyEntries[dimension]; i++) {
            if (KeyFields[dimension][i].hierachy_level == 1) {
                CurrentKeyIndex[dimension] = i;
                cell_index = GetCellNumber();

                size = consistency_eqtn.size_of_eqtn;
                consistency_eqtn.cell_number [size] = cell_index;
                consistency_eqtn.plus_or_minus[size] = 1;
                consistency_eqtn.size_of_eqtn = size + 1;
            }
        }

        save_consistency_equation();
    }

    /* Handle hierarchies */

    if (KeyIsHierarchical[dimension]) {
        for (level = 2; level < KeyHierarchyLevels[dimension]; level++) {
            CurrentKeyIndex[dimension] = 0;
            total_index = GetCellNumber();

            consistency_eqtn.RHS = 0.0;
            consistency_eqtn.size_of_eqtn = 1;
            consistency_eqtn.cell_number [0] = total_index;
            consistency_eqtn.plus_or_minus[0] = -1;

            for (i = 1; i < NumberOfKeyEntries[dimension]; i++) {
                if (KeyFields[dimension][i].hierachy_level == level) {
                    CurrentKeyIndex[dimension] = i;
                    cell_index = GetCellNumber();

                    size = consistency_eqtn.size_of_eqtn;
                    consistency_eqtn.cell_number [size] = cell_index;
                    consistency_eqtn.plus_or_minus[size] = 1;
                    consistency_eqtn.size_of_eqtn = size + 1;
                }
            }
            save_consistency_equation();
        }

        something_to_print = false;

        for (level = 1; level < KeyHierarchyLevels[dimension] - 1; level++) {
            something_to_print = false;

            consistency_eqtn.RHS = 0.0;
            consistency_eqtn.size_of_eqtn = 0;
            consistency_eqtn.plus_or_minus[0] = -1;

            for (i = 1; i < NumberOfKeyEntries[dimension]; i++) {
                CurrentKeyIndex[dimension] = i;
                cell_index = GetCellNumber();

                if (KeyFields[dimension][i].hierachy_level == level) {
                    size = consistency_eqtn.size_of_eqtn;
                    consistency_eqtn.cell_number [size] = cell_index;
                    consistency_eqtn.plus_or_minus[size] = 1;
                    consistency_eqtn.size_of_eqtn = size + 1;
                    something_to_print = true;
                } else if (KeyFields[dimension][i].hierachy_level == (level + 1)) {
                    if (something_to_print) {
                        save_consistency_equation();
                        something_to_print = false;
                    }

                    consistency_eqtn.RHS = 0.0;
                    consistency_eqtn.size_of_eqtn = 1;
                    consistency_eqtn.cell_number [0] = cell_index;
                    consistency_eqtn.plus_or_minus[0] = -1;
                }
            }
            if (something_to_print) {
                save_consistency_equation();
                something_to_print = false;
            }
        }
    }

    return rc;
}

void TabularData::CountConsistencyEquationsFor1dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 1) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        rc = CountConsistencyEquationsForAnIndex(d[0]);
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::PrintConsistencyEquationsFor1dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 1) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        rc = PrintConsistencyEquationsForAnIndex(d[0]);
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::CountConsistencyEquationsFor2dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 2) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            rc = CountConsistencyEquationsForAnIndex(d[1]);
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            rc = CountConsistencyEquationsForAnIndex(d[0]);
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::PrintConsistencyEquationsFor2dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 2) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up totals */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            rc = PrintConsistencyEquationsForAnIndex(d[1]);
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            rc = PrintConsistencyEquationsForAnIndex(d[0]);
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::CountConsistencyEquationsFor3dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 3) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                rc = CountConsistencyEquationsForAnIndex(d[2]);
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                rc = CountConsistencyEquationsForAnIndex(d[0]);
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                rc = CountConsistencyEquationsForAnIndex(d[1]);
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::PrintConsistencyEquationsFor3dTable() {
    int i;
    int j;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 3) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                rc = PrintConsistencyEquationsForAnIndex(d[2]);
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                rc = PrintConsistencyEquationsForAnIndex(d[0]);
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                rc = PrintConsistencyEquationsForAnIndex(d[1]);
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::CountConsistencyEquationsFor4dTable() {
    int i;
    int j;
    int k;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 4) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    rc = CountConsistencyEquationsForAnIndex(d[3]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    rc = CountConsistencyEquationsForAnIndex(d[2]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    rc = CountConsistencyEquationsForAnIndex(d[1]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    rc = CountConsistencyEquationsForAnIndex(d[0]);
                }
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::PrintConsistencyEquationsFor4dTable() {
    int i;
    int j;
    int k;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 4) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    rc = PrintConsistencyEquationsForAnIndex(d[3]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    rc = PrintConsistencyEquationsForAnIndex(d[2]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    rc = PrintConsistencyEquationsForAnIndex(d[1]);
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    rc = PrintConsistencyEquationsForAnIndex(d[0]);
                }
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::CountConsistencyEquationsFor5dTable() {
    int i;
    int j;
    int k;
    int l;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 5) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        CurrentKeyIndex[d[3]] = l;

                        rc = CountConsistencyEquationsForAnIndex(d[4]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[4]]; i++) {
            CurrentKeyIndex[d[4]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[2]]; l++) {
                        CurrentKeyIndex[d[2]] = l;

                        rc = CountConsistencyEquationsForAnIndex(d[3]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[4]]; j++) {
                CurrentKeyIndex[d[4]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[1]]; l++) {
                        CurrentKeyIndex[d[1]] = l;

                        rc = CountConsistencyEquationsForAnIndex(d[2]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[4]]; k++) {
                    CurrentKeyIndex[d[4]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[0]]; l++) {
                        CurrentKeyIndex[d[0]] = l;

                        rc = CountConsistencyEquationsForAnIndex(d[1]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[4]]; l++) {
                        CurrentKeyIndex[d[4]] = l;

                        rc = CountConsistencyEquationsForAnIndex(d[0]);
                    }
                }
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::PrintConsistencyEquationsFor5dTable() {
    int i;
    int j;
    int k;
    int l;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 5) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        CurrentKeyIndex[d[3]] = l;

                        rc = PrintConsistencyEquationsForAnIndex(d[4]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[4]]; i++) {
            CurrentKeyIndex[d[4]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[2]]; l++) {
                        CurrentKeyIndex[d[2]] = l;

                        rc = PrintConsistencyEquationsForAnIndex(d[3]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[4]]; j++) {
                CurrentKeyIndex[d[4]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[1]]; l++) {
                        CurrentKeyIndex[d[1]] = l;

                        rc = PrintConsistencyEquationsForAnIndex(d[2]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[4]]; k++) {
                    CurrentKeyIndex[d[4]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[0]]; l++) {
                        CurrentKeyIndex[d[0]] = l;

                        rc = PrintConsistencyEquationsForAnIndex(d[1]);
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[4]]; l++) {
                        CurrentKeyIndex[d[4]] = l;

                        rc = PrintConsistencyEquationsForAnIndex(d[0]);
                    }
                }
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::CountConsistencyEquationsFor6dTable() {
    int i;
    int j;
    int k;
    int l;
    int m;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 6) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        CurrentKeyIndex[d[3]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[4]]; m++) {
                            CurrentKeyIndex[d[4]] = m;

                            rc = CountConsistencyEquationsForAnIndex(d[5]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[5]]; i++) {
            CurrentKeyIndex[d[5]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[2]]; l++) {
                        CurrentKeyIndex[d[2]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[3]]; m++) {
                            CurrentKeyIndex[d[3]] = m;

                            rc = CountConsistencyEquationsForAnIndex(d[4]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[4]]; i++) {
            CurrentKeyIndex[d[4]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[5]]; j++) {
                CurrentKeyIndex[d[5]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[1]]; l++) {
                        CurrentKeyIndex[d[1]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[2]]; m++) {
                            CurrentKeyIndex[d[2]] = m;

                            rc = CountConsistencyEquationsForAnIndex(d[3]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[4]]; j++) {
                CurrentKeyIndex[d[4]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[5]]; k++) {
                    CurrentKeyIndex[d[5]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[0]]; l++) {
                        CurrentKeyIndex[d[0]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[1]]; m++) {
                            CurrentKeyIndex[d[1]] = m;

                            rc = CountConsistencyEquationsForAnIndex(d[2]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[4]]; k++) {
                    CurrentKeyIndex[d[4]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[5]]; l++) {
                        CurrentKeyIndex[d[5]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[0]]; m++) {
                            CurrentKeyIndex[d[0]] = m;

                            rc = CountConsistencyEquationsForAnIndex(d[1]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[4]]; l++) {
                        CurrentKeyIndex[d[4]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[5]]; m++) {
                            CurrentKeyIndex[d[5]] = m;

                            rc = CountConsistencyEquationsForAnIndex(d[0]);
                        }
                    }
                }
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

void TabularData::PrintConsistencyEquationsFor6dTable() {
    int i;
    int j;
    int k;
    int l;
    int m;
    int d[MAX_NO_KEY_FIELDS];
    bool rc;

    rc = true;

    if (NumberOfDimensions == 6) {
        /* Find which index is being used */

        j = 0;
        for (i = 0; i < MAX_NO_KEY_FIELDS; i++) {
            if (KeyIndexInUse[i]) {
                d[j] = i;
                j++;
            }
        }

        /* Set up ConsistencyEquations */

        for (i = 0; i < NumberOfKeyEntries[d[0]]; i++) {
            CurrentKeyIndex[d[0]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[1]]; j++) {
                CurrentKeyIndex[d[1]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[2]]; k++) {
                    CurrentKeyIndex[d[2]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[3]]; l++) {
                        CurrentKeyIndex[d[3]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[4]]; m++) {
                            CurrentKeyIndex[d[4]] = m;

                            rc = PrintConsistencyEquationsForAnIndex(d[5]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[5]]; i++) {
            CurrentKeyIndex[d[5]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[0]]; j++) {
                CurrentKeyIndex[d[0]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[1]]; k++) {
                    CurrentKeyIndex[d[1]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[2]]; l++) {
                        CurrentKeyIndex[d[2]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[3]]; m++) {
                            CurrentKeyIndex[d[3]] = m;

                            rc = PrintConsistencyEquationsForAnIndex(d[4]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[4]]; i++) {
            CurrentKeyIndex[d[4]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[5]]; j++) {
                CurrentKeyIndex[d[5]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[0]]; k++) {
                    CurrentKeyIndex[d[0]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[1]]; l++) {
                        CurrentKeyIndex[d[1]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[2]]; m++) {
                            CurrentKeyIndex[d[2]] = m;

                            rc = PrintConsistencyEquationsForAnIndex(d[3]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[3]]; i++) {
            CurrentKeyIndex[d[3]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[4]]; j++) {
                CurrentKeyIndex[d[4]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[5]]; k++) {
                    CurrentKeyIndex[d[5]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[0]]; l++) {
                        CurrentKeyIndex[d[0]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[1]]; m++) {
                            CurrentKeyIndex[d[1]] = m;

                            rc = PrintConsistencyEquationsForAnIndex(d[2]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[2]]; i++) {
            CurrentKeyIndex[d[2]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[3]]; j++) {
                CurrentKeyIndex[d[3]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[4]]; k++) {
                    CurrentKeyIndex[d[4]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[5]]; l++) {
                        CurrentKeyIndex[d[5]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[0]]; m++) {
                            CurrentKeyIndex[d[0]] = m;

                            rc = PrintConsistencyEquationsForAnIndex(d[1]);
                        }
                    }
                }
            }
        }

        for (i = 0; i < NumberOfKeyEntries[d[1]]; i++) {
            CurrentKeyIndex[d[1]] = i;

            for (j = 0; j < NumberOfKeyEntries[d[2]]; j++) {
                CurrentKeyIndex[d[2]] = j;

                for (k = 0; k < NumberOfKeyEntries[d[3]]; k++) {
                    CurrentKeyIndex[d[3]] = k;

                    for (l = 0; l < NumberOfKeyEntries[d[4]]; l++) {
                        CurrentKeyIndex[d[4]] = l;

                        for (m = 0; m < NumberOfKeyEntries[d[5]]; m++) {
                            CurrentKeyIndex[d[5]] = m;

                            rc = PrintConsistencyEquationsForAnIndex(d[0]);
                        }
                    }
                }
            }
        }
    }

    if (rc == false) {
        logger->error(1, "Out of memory whilst writing consistency equations");
    }
}

/*
void print_jj_file_cells()
{
        FILE *ofp;
        int i;

        if ( ( ofp = fopen( outjjfilename, "w" ) ) != NULL )
        {
                fprintf(ofp, "0\n");

                fprintf(ofp, " %d \n", ncells);

                for (i=0; i<ncells; i++)
                {
                        fprintf(ofp, " %d %f  %f %c %f %f %f %f %f\n", i, cells[i].nominal_value, cells[i].loss_of_information_weight, cells[i].status, cells[i].lower_bound, cells[i].upper_bound, cells[i].lower_protection_level, cells[i].upper_protection_level, cells[i].sliding_protection_level);
                }

                fclose(ofp);
        }
        else
        {
                logger->error(1, "Unable to open output (jj) file: %s", outjjfilename);
                exit(-1)
        }
}
 */


void TabularData::allocate_consistency_equation_memory() {
    int i;

    eqtn_number = 0;

    consistency_eqtns = new ConsistencyEquation[nsums];

    for (i = 0; i < nsums; i++) {
        consistency_eqtns[i].cell_number = NULL;
        consistency_eqtns[i].plus_or_minus = NULL;
    }
}

void TabularData::label_zeroed_cells() {
    int i;

    for (i = 0; i < ncells; i++) {
        if (cells[i].nominal_value < 0.001) {
            cells[i].status = 'z';
        }
    }
}

void TabularData::label_primary_cells() {
//    int i;
//    double protection;
    /*
            for (i=0; i<ncells; i++)
            {
                    if (cells[i].nominal_value >= 0.0001)
                    {
                            switch (cells[i].frequency)
                            {
                            case 0:
                                    break;

                            case 1:
                            case 2:
                            case 3:
                                    cells[i].status = 'u';
                                    cells[i].rule   = 'T';
                                    break;

                            default:

                                    // P% rule with P=10%

                                    if ((cells[i].largest_value_1 * 0.1) > (cells[i].nominal_value - cells[i].largest_value_1 - cells[i].largest_value_2))
                                    {
                                            cells[i].status = 'u';  // The n-2 smallest contributors sum to less than 10% of the largest.
                                            cells[i].rule   = 'P';
                                            protection = (cells[i].largest_value_1 * 0.1) - (cells[i].nominal_value - cells[i].largest_value_1 - cells[i].largest_value_2);

                                            if (protection < 1.0)
                                            {
                                                    protection = 1.0;
                                            }

                                            cells[i].lower_protection_level = protection;
                                            cells[i].upper_protection_level = protection;
                                    }
                                    break;
                            }

                            //if ((cells[i].nominal_value * 0.7) < (cells[i].largest_value_1 + cells[i].largest_value_2 + cells[i].largest_value_3))
                            //{
                            //	cells[i].status = 'u';  // The three largest contributors contribute over 70% of the total.
                            //}
                    }
            }
     */
}

void TabularData::set_weights_and_lower_and_upper_bounds() {
    int i;
    double upper_bound;
    double level;
    double weight;

    upper_bound = 2.0 * cells[0].nominal_value;

    for (i = 0; i < ncells; i++) {
        if (cells[i].nominal_value > 0.0001) {
            //weight = cells[i].nominal_value / 10.0;
            weight = log(cells[i].nominal_value);

            if (weight < 0.0001) {
                weight = 0.0001;
            }

            //if (cells[i].number_of_contributing_cells > 1)
            //{
            //	weight = weight + 10.0 * cells[i].number_of_contributing_cells;
            //}

            if (cell_type[i] == SUB_TOTAL_CELL) {
                weight = weight + 100;
            } else if (cell_type[i] == MARGIN_CELL) {
                weight = weight + 1000;
            }

            if (i == 0) {
                weight = weight + 4000;
            }

            cells[i].loss_of_information_weight = weight;
            //cells[i].loss_of_information_weight = cells[i].nominal_value / 10.0;

            cells[i].lower_bound = 0.0;
            cells[i].upper_bound = upper_bound;
            //cells[i].upper_bound = cells[i].nominal_value;

            level = cells[i].nominal_value / 10.0;

            if (level < 0.005) {
                level = 0.005;
            }
            cells[i].lower_protection_level = level;
            cells[i].upper_protection_level = level;
        }
        cells[i].sliding_protection_level = 0.0;
    }
}

int TabularData::suppress_marginals() {
    int number_of_marginals_suppressed;
    int i;
    int j;
    int sz;
    int cell;
    int marginal = 0;
    int cell_count;
    int marginal_count;
    int marginal_primaries;
    int cell_primaries;
    double max_pl;
    int max_cell;
    double sum_xi;

    number_of_marginals_suppressed = 0;

    for (i = 0; i < nsums; i++) {
        sz = consistency_eqtns[i].size_of_eqtn;

        if (sz > 0) {
            cell_count = 0;
            cell_primaries = 0;
            marginal_count = 0;
            marginal_primaries = 0;

            for (j = 0; j < sz; j++) {
                if (consistency_eqtns[i].plus_or_minus[j] == -1) {
                    marginal = consistency_eqtns[i].cell_number[j];
                    marginal_count++;

                    if ((cells[marginal].status == 'u') || (cells[marginal].status == 'v')) {
                        marginal_primaries++;
                    }
                } else {
                    cell = consistency_eqtns[i].cell_number[j];
                    cell_count++;

                    if ((cells[cell].status == 'u') || (cells[cell].status == 'v')) {
                        cell_primaries++;
                    }
                }
            }

            if (marginal_count != 1) {
                logger->error(1, "Too many marginals in constraint %d", i);
            } else if ((marginal_primaries == 0) && (cell_primaries >= 1)) {
                //logger->log(5, "Possible margin suppression identified in constraint %d", i);

                max_pl = 0.0;
                max_cell = 0;

                for (j = 0; j < sz; j++) {
                    cell = consistency_eqtns[i].cell_number[j];

                    if (((cells[cell].status == 'u') || (cells[cell].status == 'v')) && (consistency_eqtns[i].plus_or_minus[j] != -1)) {
                        if (max_pl < cells[cell].lower_protection_level) {
                            max_pl = cells[cell].lower_protection_level;
                            max_cell = cell;
                        }
                        if (max_pl < cells[cell].upper_protection_level) {
                            max_pl = cells[cell].upper_protection_level;
                            max_cell = cell;
                        }
                    }
                }

                if (cells[marginal].nominal_value < (cells[max_cell].nominal_value + max_pl)) {
                    //logger->log(5, "Margin suppression identified in cell %d", marginal);

                    sum_xi = 0.0;

                    for (j = 0; j < sz; j++) {
                        cell = consistency_eqtns[i].cell_number[j];

                        if ((cell != marginal) && (cell != max_cell)) {
                            sum_xi = sum_xi + cells[cell].nominal_value;
                        }
                    }

                    cells[marginal].status = 'v';
                    cells[marginal].lower_protection_level = cells[max_cell].lower_protection_level - sum_xi;
                    cells[marginal].upper_protection_level = cells[max_cell].upper_protection_level - sum_xi;

                    //logger->log(5, "Lower protection level set to %lf", cells[marginal].lower_protection_level);
                    //logger->log(5, "Upper protection level set to %lf", cells[marginal].upper_protection_level);

                    number_of_marginals_suppressed++;
                }
            }
        }
    }

    logger->log(3, "%d marginals suppressed", number_of_marginals_suppressed);

    return number_of_marginals_suppressed;
}

void TabularData::suppress_large_cells() {
    int number_of_large_cells_suppressed;
    int i;
    int j;
    int sz;
    int cell;
    int marginal = 0;
    int cell_count;
    int marginal_count;
    int marginal_primaries;
    int cell_primaries;
    double max_pl;
    int max_primary;
    double max_nv;
    int max_secondary;

    number_of_large_cells_suppressed = 0;

    for (i = 0; i < nsums; i++) {
        sz = consistency_eqtns[i].size_of_eqtn;

        if (sz > 0) {
            cell_count = 0;
            cell_primaries = 0;
            marginal_count = 0;
            marginal_primaries = 0;

            for (j = 0; j < sz; j++) {
                if (consistency_eqtns[i].plus_or_minus[j] == -1) {
                    marginal = consistency_eqtns[i].cell_number[j];
                    marginal_count++;

                    if ((cells[marginal].status == 'u') || (cells[marginal].status == 'v')) {
                        marginal_primaries++;
                    }
                } else {
                    cell = consistency_eqtns[i].cell_number[j];
                    cell_count++;

                    if ((cells[cell].status == 'u') || (cells[cell].status == 'v')) {
                        cell_primaries++;
                    }
                }
            }

            if (marginal_count != 1) {
                logger->error(1, "Too many marginals in constraint %d", i);
            } else if ((marginal_primaries == 0) && (cell_primaries >= 1)) {
                //logger->log(5, "Possible margin suppression identified in constraint %d", i);

                max_pl = 0.0;
                max_nv = 0.0;
                max_primary = 0;
                max_secondary = 0;

                for (j = 0; j < sz; j++) {
                    cell = consistency_eqtns[i].cell_number[j];

                    if (((cells[cell].status == 'u') || (cells[cell].status == 'v')) && (consistency_eqtns[i].plus_or_minus[j] != -1)) {
                        if (max_pl < cells[cell].lower_protection_level) {
                            max_pl = cells[cell].lower_protection_level;
                            max_primary = cell;
                        }
                        if (max_pl < cells[cell].upper_protection_level) {
                            max_pl = cells[cell].upper_protection_level;
                            max_primary = cell;
                        }
                    }

                    if ((cells[cell].status == 's') && (consistency_eqtns[i].plus_or_minus[j] != -1)) {
                        if (max_nv < cells[cell].nominal_value) {
                            max_nv = cells[cell].nominal_value;
                            max_secondary = cell;
                        }
                    }
                }

                if ((cells[marginal].nominal_value - cells[max_secondary].nominal_value) < (cells[max_primary].nominal_value + max_pl)) {
                    cells[max_secondary].status = 'v';
                    number_of_large_cells_suppressed++;
                }
            }
        }
    }

    logger->log(3, "%d large cells suppressed", number_of_large_cells_suppressed);
}

void TabularData::WriteTableData() {
    FILE *ofp;
    int i;
    int count_ordinary;
    int count_sub_total;
    int count_margin;
    int count_primaries;
    int count_secondaries;
    int count_zeroes;
    int count_threshold;
    int count_p_percent;

    count_ordinary = 0;
    count_sub_total = 0;
    count_margin = 0;
    count_primaries = 0;
    count_secondaries = 0;
    count_zeroes = 0;
    count_threshold = 0;
    count_p_percent = 0;

    for (i = 0; i < ncells; i++) {
        if (cell_type[i] == ORDINARY_CELL) {
            count_ordinary++;
        } else if (cell_type[i] == SUB_TOTAL_CELL) {
            count_sub_total++;
        } else if (cell_type[i] == MARGIN_CELL) {
            count_margin++;
        }

        if ((cells[i].status == 'u') || (cells[i].status == 'U')) {
            count_primaries++;
        } else if ((cells[i].status == 'm') || (cells[i].status == 'M')) {
            count_secondaries++;
        } else if (cells[i].status == 'z') {
            count_zeroes++;
        }

        if (cells[i].rule == 'T') {
            count_threshold++;
        } else if (cells[i].rule == 'P') {
            count_p_percent++;
        }
    }

    if ((ofp = fopen("Intermediate.txt", "a")) != NULL) {
        fprintf(ofp, "NumberOfCells = %d \n", ncells);
        fprintf(ofp, "NumberOfOrdinaryCells = %d \n", count_ordinary);
        fprintf(ofp, "NumberOfSubTotalCells = %d \n", count_sub_total);
        fprintf(ofp, "NumberOfMarginCells = %d \n", count_margin);
        fprintf(ofp, "NumberOfPrimaryCells = %d \n", count_primaries);
        fprintf(ofp, "NumberOfThresholdRule = %d \n", count_threshold);
        fprintf(ofp, "NumberOfP_PercentRule = %d \n", count_p_percent);
        fprintf(ofp, "NumberOfSecondaryCells = %d \n", count_secondaries);
        fprintf(ofp, "NumberOfZeroCells = %d \n", count_zeroes);
        fprintf(ofp, "NumberOfConstraintEquations = %d \n", nsums);

        fclose(ofp);
    }
}

int TabularData::find_dot_position(char *filename) {
    int dot_position;
    int i;
    char chr;

    dot_position = 0;
    i = 0;
    chr = filename[i];

    while (chr != '\0') {
        if (chr == '.') {
            dot_position = i;
        }
        i++;
        chr = filename[i];
    }

    return dot_position;
}

int TabularData::find_end_position(char *filename) {
    int end_position;
    int i;
    char chr;

    end_position = 0;
    i = 0;
    chr = filename[i];

    while (chr != '\0') {
        end_position = i;
        i++;
        chr = filename[i];
    }

    return end_position;
}

bool TabularData::file_exists(char *filename) {
    bool exists;
    FILE *ifp;

    exists = false;

    if ((ifp = fopen(filename, "r")) != NULL) {
        exists = true;

        fclose(ifp);
    }

    return exists;
}

void TabularData::PrintJJFile(const char* jjfilename) {
    FILE *ofp;
    int i;
    int j;
    int size;

    if ((ofp = fopen(jjfilename, "w")) != NULL) {
        logger->log(3, "Printing %s", jjfilename);

        fprintf(ofp, "0\n");

        fprintf(ofp, "%d\n", ncells);

        for (i = 0; i < ncells; i++) {
            fprintf(ofp, "%d %f %f %c %f %f %f %f %f\n", i, cells[i].nominal_value, cells[i].loss_of_information_weight, cells[i].status, cells[i].lower_bound, cells[i].upper_bound, cells[i].lower_protection_level, cells[i].upper_protection_level, cells[i].sliding_protection_level);
        }

        fprintf(ofp, "%d\n", nsums);

        for (i = 0; i < nsums; i++) {
            size = consistency_eqtns[i].size_of_eqtn;

            if (size > 0) {
                fprintf(ofp, "0 %d :", size);

                for (j = 0; j < size; j++) {
                    fprintf(ofp, " %d (%d)", consistency_eqtns[i].cell_number[j], consistency_eqtns[i].plus_or_minus[j]);
                }
                fprintf(ofp, "\n");
            } else {
                logger->log(3, "Consistency_eqtns[%d].size_of_eqtn == %d", i, size);
            }
        }

        logger->log(3, "Completed");

        fclose(ofp);
    } else {
        logger->error(1, "Unable to open output (jj) file: %s", jjfilename);
    }
}

int TabularData::GetNumberOfDimensions() {
    return NumberOfDimensions;
}

void TabularData::PrintJJFileMapping(const char* mapfilename) {
    switch (NumberOfDimensions) {
        case 1:
            logger->log(3, "1-d Mapping not available");
            break;

        case 2:
            Print2dMapping(mapfilename);
            break;

        case 3:
            Print3dMapping(mapfilename);
            break;

        case 4:
            Print4dMapping(mapfilename);
            break;

        case 5:
            Print5dMapping(mapfilename);
            break;

        case 6:
            Print6dMapping(mapfilename);
            break;

        default:
            logger->log(3, "Table of unknown dimension");
            break;
    }
}

int TabularData::GetFieldIndex(char* fieldname) {
    int i;

    if (NumberOfFields == 0) {
        logger->error(1, "Reading in Tabular data");
    }

    for (i = 0; i < NumberOfFields; i++) {
        if (strcmp(MetaDataFields[i].Name, fieldname) == 0) {
            return i;
        }
    }

    logger->error(1, "Partition parameter not found: %s", fieldname);

    // Keep the compiler from complaining
    return -1;
}

bool TabularData::IsHierarchical(char* fieldname) {
    int index;

    index = GetFieldIndex(fieldname);

    if ((index >= 0) && (index < NumberOfFields)) {
        return MetaDataFields[index].Hierarchical;
    } else {
        return false;
    }
}

bool TabularData::IsHierarchical(int index) {
    if ((index >= 0) && (index < NumberOfFields)) {
        return MetaDataFields[index].Hierarchical;
    } else {
        return false;
    }
}

FIELD_TYPE TabularData::GetFieldType(char* fieldname) {
    int index;

    index = GetFieldIndex(fieldname);

    if ((index >= 0) && (index < NumberOfFields)) {
        return MetaDataFields[index].Type;
    } else {
        return FIELD_TYPE_UNKNOWN;
    }
}

FIELD_TYPE TabularData::GetFieldType(int index) {
    if ((index >= 0) && (index < NumberOfFields)) {
        return MetaDataFields[index].Type;
    } else {
        return FIELD_TYPE_UNKNOWN;
    }
}

int TabularData::GetFieldSize(char* fieldname) {
    int index;
    int key_index;

    index = GetFieldIndex(fieldname);

    if ((index >= 0) && (index < NumberOfFields)) {
        if (MetaDataFields[index].Type == FIELD_TYPE_RECODEABLE) {
            key_index = MetaDataFields[index].KeyIndex;

            if ((key_index >= 0) && (key_index < MAX_NO_KEY_FIELDS)) {
                return NumberOfKeyEntries[key_index];
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

int TabularData::GetFieldSize(int index) {
    int key_index;

    if ((index >= 0) && (index < NumberOfFields)) {
        if (MetaDataFields[index].Type == FIELD_TYPE_RECODEABLE) {
            key_index = MetaDataFields[index].KeyIndex;

            if ((key_index >= 0) && (key_index < MAX_NO_KEY_FIELDS)) {
                return NumberOfKeyEntries[key_index];
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

int TabularData::get_highest_level_field_size(char* fieldname) {
    int index = GetFieldIndex(fieldname);

    if ((index < 0) || (index >= NumberOfFields)) {
        return 0;
    }

    if (MetaDataFields[index].Type != FIELD_TYPE_RECODEABLE) {
        return 0;
    }

    int key_index = MetaDataFields[index].KeyIndex;

    if ((key_index < 0) || (key_index >= MAX_NO_KEY_FIELDS)) {
        return 0;
    }

    int top;

    if (MetaDataFields[index].Hierarchical) {
        top = KeyFields[key_index][0].hierachy_level - 1;
    } else {
        top = 1;
    }

    int count = 0;
    for (int i = 0; i < NumberOfKeyEntries[key_index]; i++) {
        if (KeyFields[key_index][i].hierachy_level == top) {
            count++;
        }
    }

    return count;
}

int TabularData::GetHighestLevelFieldSize(int index) {
    int key_index;
    int top;
    int count;
    int i;

    count = 0;

    if ((index >= 0) && (index < NumberOfFields)) {
        if (MetaDataFields[index].Type == FIELD_TYPE_RECODEABLE) {
            key_index = MetaDataFields[index].KeyIndex;

            if ((key_index >= 0) && (key_index < MAX_NO_KEY_FIELDS)) {
                if (MetaDataFields[index].Hierarchical) {
                    top = KeyFields[key_index][0].hierachy_level - 1;
                } else {
                    top = 1;
                }

                for (i = 0; i < NumberOfKeyEntries[key_index]; i++) {
                    if (KeyFields[key_index][i].hierachy_level == top) {
                        count++;
                    }
                }

                return count;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

int TabularData::GetNumberOfCells() {
    return ncells;
}

void TabularData::CreatePartitionedHierarchyFiles(char* fieldname, int NumberOfPartitions) {
    FILE *ofp;
    int index;
    int key_index;
    int top;
    int count;
    int countHL;
    int i;
    int j;
    int partition_size;
    int numberOfAts;

    count = 1;
    countHL = 1;

    index = GetFieldIndex(fieldname);

    if ((index >= 0) && (index < NumberOfFields)) {
        if (MetaDataFields[index].Type == FIELD_TYPE_RECODEABLE) {
            key_index = MetaDataFields[index].KeyIndex;

            if ((key_index >= 0) && (key_index < MAX_NO_KEY_FIELDS)) {
                top = KeyFields[key_index][0].hierachy_level - 1;

                if (MetaDataFields[index].Hierarchical) {
                    partition_size = (GetHighestLevelFieldSize(index) + NumberOfPartitions - 1) / NumberOfPartitions;
                    logger->log(3, "Partition_size = %d", partition_size);

                    sprintf(outputfilename, "%s_%d.txt", fieldname, count);
                    logger->log(3, "Filename = %s", outputfilename);

                    if ((ofp = fopen(outputfilename, "w")) == NULL) {
                        logger->error(1, "Failed to create partitioned hierarchy files for %s", fieldname);
                    }

                    for (i = 1; i < NumberOfKeyEntries[key_index]; i++) {
                        if ((i > 1) && (KeyFields[key_index][i].hierachy_level == top)) {
                            if ((countHL % partition_size) == 0) {
                                fclose(ofp);

                                count++;

                                sprintf(outputfilename, "%s_%d.txt", fieldname, count);
                                logger->log(3, "Filename = %s", outputfilename);

                                if ((ofp = fopen(outputfilename, "w")) == NULL) {
                                    logger->error(1, "Failed to create partitioned hierarchy files for %s", fieldname);
                                }
                            }

                            countHL++;
                        }

                        numberOfAts = top - KeyFields[key_index][i].hierachy_level;

                        if (numberOfAts > 0) {
                            for (j = 0; j < numberOfAts; j++) {
                                fprintf(ofp, "%s", MetaDataFields[index].HierLeadString);
                            }
                        }
                        fprintf(ofp, "%s\n", KeyFields[key_index][i].key_value);
                    }

                    fclose(ofp);
                } else {
                    partition_size = (NumberOfKeyEntries[key_index] + NumberOfPartitions - 1) / NumberOfPartitions;

                    sprintf(outputfilename, "%s_%d.txt", fieldname, count);

                    if ((ofp = fopen(outputfilename, "w")) == NULL) {
                        logger->error(1, "Failed to create partitioned hierarchy files for %s", fieldname);
                    }

                    for (i = 1; i < NumberOfKeyEntries[key_index]; i++) {
                        if ((i > 1) && (((i - 1) % partition_size) == 0)) {
                            fclose(ofp);

                            count++;

                            sprintf(outputfilename, "%s_%d.txt", fieldname, count);

                            if ((ofp = fopen(outputfilename, "w")) == NULL) {
                                logger->error(1, "Failed to create partitioned hierarchy files for %s", fieldname);
                            }
                        }

                        fprintf(ofp, "%s\n", KeyFields[key_index][i].key_value);
                    }

                    fclose(ofp);
                }

                for (i = 0; i < NumberOfKeyEntries[key_index]; i++) {
                    if (KeyFields[key_index][i].hierachy_level == top) {
                        count++;
                    }
                }
            }
        }
    }
}

void TabularData::CreatePartitionedMetadataFiles(char* fieldname1, int NumberOfPartitions1, char* fieldname2, int NumberOfPartitions2) {
    FILE *ofp;
    int index1;
    int index2;
    int i;
    int j;
    int k;

    index1 = GetFieldIndex(fieldname1);
    index2 = GetFieldIndex(fieldname2);

    if ((index1 >= 0) && (index1 < NumberOfFields) && (index2 >= 0) && (index2 < NumberOfFields) && (index1 != index2)) {
        for (i = 0; i < NumberOfPartitions1; i++) {
            for (j = 0; j < NumberOfPartitions2; j++) {
                sprintf(outputfilename, "Metadata_%d.rda", i * NumberOfPartitions2 + j + 1);

                if ((ofp = fopen(outputfilename, "w")) != NULL) {
                    fprintf(ofp, " <SEPARATOR>  %c%c%c\n", '"', metadata_separator_char, '"');
                    fprintf(ofp, " <SAFE>  %c%c%c\n", '"', metadata_safe_char, '"');
                    fprintf(ofp, " <UNSAFE>  %c%c%c\n", '"', metadata_unsafe_char, '"');
                    fprintf(ofp, " <PROTECT>  %c%c%c\n", '"', metadata_protect_char, '"');

                    for (k = 0; k < NumberOfFields; k++) {
                        fprintf(ofp, "%s\n", MetaDataFields[k].Name);

                        switch (MetaDataFields[k].Type) {
                            case FIELD_TYPE_RECODEABLE:

                                fprintf(ofp, "  <RECODEABLE>\n");
                                fprintf(ofp, "  <TOTCODE> TOTAL\n");
                                if (k == index1) {
                                    fprintf(ofp, "  <HIERARCHICAL>\n");
                                    fprintf(ofp, "  <HIERCODELIST> %c%s_%d.txt%c\n", '"', fieldname1, i + 1, '"');
                                    fprintf(ofp, "  <HIERLEADSTRING> %c%s%c\n", '"', MetaDataFields[k].HierLeadString, '"');
                                } else if (k == index2) {
                                    fprintf(ofp, "  <HIERARCHICAL>\n");
                                    fprintf(ofp, "  <HIERCODELIST> %c%s_%d.txt%c\n", '"', fieldname2, j + 1, '"');
                                    fprintf(ofp, "  <HIERLEADSTRING> %c%s%c\n", '"', MetaDataFields[k].HierLeadString, '"');
                                } else {
                                    if (MetaDataFields[k].Hierarchical) {
                                        fprintf(ofp, "  <HIERARCHICAL>\n");
                                        fprintf(ofp, "  <HIERCODELIST> %c%s%c\n", '"', MetaDataFields[k].HierCodeList, '"');
                                        fprintf(ofp, "  <HIERLEADSTRING> %c%s%c\n", '"', MetaDataFields[k].HierLeadString, '"');
                                    }
                                }
                                break;

                            case FIELD_TYPE_NUMERIC:
                                fprintf(ofp, "  <NUMERIC>\n");
                                break;

                            case FIELD_TYPE_NUMERIC_SHADOW:
                                break;

                            case FIELD_TYPE_NUMERIC_COST:
                                break;

                            case FIELD_TYPE_NUMERIC_LOWERPL:
                                break;

                            case FIELD_TYPE_NUMERIC_UPPERPL:
                                break;

                            case FIELD_TYPE_FREQUENCY:
                                break;

                            case FIELD_TYPE_MAXSCORE:
                                break;

                            case FIELD_TYPE_STATUS:
                                fprintf(ofp, "  <STATUS>\n");
                                break;

                            default:
                                break;
                        }
                    }
                    fclose(ofp);
                }
            }
        }
    }
}

int TabularData::GetNumberOfPrimaryCells() {
    int i;
    int count;

    count = 0;

    for (i = 0; i < ncells; i++) {
        if (cells[i].status == 'u') {
            count++;
        }
    }

    return count;
}

void TabularData::get_directory(char *dir, char *path) {
    char *p = path;
    char *last = p;
    while (*p != '\0') {
        if (*p++ == DIRECTORY_SEPARATOR) {
            last = p;
        }
    }

    p = path;
    char *q = dir;
    while (p != last) {
        *q++ = *p++;
    }
    *q = '\0';
}

char* TabularData::get_tab_data_filename() {
    return tabdatafilename;
}
