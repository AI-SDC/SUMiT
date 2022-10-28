#pragma once

#include "stdafx.h"

#define MAX_SIZE_OF_EQTNS 100000

#define MAX_BUFFER_SIZE   500
#define MAX_STRING_SIZE   200
#define MAX_KEY_SIZE      100

#define MAX_NO_METADATA_FIELDS  30
#define MAX_NO_KEY_FIELDS       6
#define MAX_NO_KEY_ENTRIES      20000

#define ORDINARY_CELL      0
#define SUB_TOTAL_CELL     1
#define MARGIN_CELL        2

enum FIELD_TYPE {
    FIELD_TYPE_UNKNOWN,
    FIELD_TYPE_RECODEABLE,
    FIELD_TYPE_NUMERIC,
    FIELD_TYPE_NUMERIC_SHADOW,
    FIELD_TYPE_NUMERIC_COST,
    FIELD_TYPE_NUMERIC_LOWERPL,
    FIELD_TYPE_NUMERIC_UPPERPL,
    FIELD_TYPE_FREQUENCY,
    FIELD_TYPE_MAXSCORE,
    FIELD_TYPE_STATUS
};

class TabularData {

public:
    TabularData(char* metadatafilename, char* tabdatafilename);
    ~TabularData(void);
    void PrintJJFile(const char* jjfilename);
    void PrintJJFileMapping(const char* mapfilename);
    int GetNumberOfDimensions();
    int GetFieldIndex(char* fieldname);
    bool IsHierarchical(char* fieldname);
    bool IsHierarchical(int index);
    FIELD_TYPE GetFieldType(char* fieldname);
    FIELD_TYPE GetFieldType(int index);
    int GetFieldSize(char* fieldname);
    int GetFieldSize(int index);
    int get_highest_level_field_size(char* fieldname);
    int GetHighestLevelFieldSize(int index);
    int GetNumberOfCells();
    void CreatePartitionedHierarchyFiles(char* fieldname, int NumberOfPartitions);
    void CreatePartitionedMetadataFiles(char* fieldname1, int NumberOfPartitions1, char* fieldname2, int NumberOfPartitions2);
    void UpdateStatusFromCSVFile(char* csvfilename);
    int GetNumberOfPrimaryCells();
    char* get_tab_data_filename();

private:
    struct MetaDataField {
        char Name[MAX_STRING_SIZE];
        FIELD_TYPE Type;
        int NumberOfDecimals;
        char TotalCode[MAX_STRING_SIZE];
        bool Hierarchical;
        int HierarchicalLevels[5];
        int NumberOfHierarchies;
        char CodeList[MAX_STRING_SIZE];
        bool CodeListPresent;
        char HierCodeList[MAX_STRING_SIZE];
        bool HierCodeListPresent;
        char HierLeadString[MAX_STRING_SIZE];
        bool HierLeadStringPresent;
        int Distance[5];
        int KeyIndex;
        int DataIndex;
        bool UseThisIndex;
    };

    struct KeyField {
        char key_value[MAX_KEY_SIZE];
        int key_index;
        int hierachy_level;
    };

    struct Cell {
        double nominal_value;
        double largest_value_1;
        double largest_value_2;
        double largest_value_3;
        int frequency;
        int number_of_contributing_cells;
        double loss_of_information_weight;
        char status;
        char rule;
        double lower_bound;
        double upper_bound;
        double lower_protection_level;
        double upper_protection_level;
        double sliding_protection_level;
    };

    struct CellBounds {
        double lower;
        double upper;
    };

    struct SingleConsistencyEquation {
        double RHS;
        int size_of_eqtn;
        int cell_number [MAX_SIZE_OF_EQTNS];
        int plus_or_minus[MAX_SIZE_OF_EQTNS];
    };

    struct ConsistencyEquation {
        double RHS;
        int size_of_eqtn;
        int *cell_number;
        int *plus_or_minus;
    };

    char outputfilename[MAX_FILENAME_SIZE];

    char InputBuffer [MAX_BUFFER_SIZE];

    // Meta data variables
    char metadata_separator_char;
    char metadata_safe_char;
    char metadata_unsafe_char;
    char metadata_protect_char;

    struct MetaDataField MetaDataFields[MAX_NO_METADATA_FIELDS];

    int NumberOfKeyFields;
    int NumberOfDataFields;
    int NumberOfFields;

    struct KeyField KeyFields[MAX_NO_KEY_FIELDS][MAX_NO_KEY_ENTRIES];

    int NumberOfKeyEntries[MAX_NO_KEY_FIELDS];

    int KeyEntryOffsets [MAX_NO_KEY_FIELDS];

    int CurrentKeyIndex [MAX_NO_KEY_FIELDS];

    bool KeyIndexInUse [MAX_NO_KEY_FIELDS];
    bool KeyIsHierarchical [MAX_NO_KEY_FIELDS];
    int KeyHierarchyLevels[MAX_NO_KEY_FIELDS];


    //struct Cell cells[MAX_NO_OF_CELLS];
    struct Cell *cells;

    //struct CellBounds cell_bounds[MAX_NO_OF_CELLS];
    struct CellBounds *cell_bounds;

    //int    cell_type[MAX_NO_OF_CELLS];
    int *cell_type;

    struct SingleConsistencyEquation consistency_eqtn;

    ConsistencyEquation *consistency_eqtns;

    time_t start_seconds;
    time_t stop_seconds;

    int NumberOfDimensions;
    int NumberOfRecordsRead;
    int ncells;
    int nsums;
    int eqtn_number;

    char Tokens[MAX_NO_METADATA_FIELDS][MAX_STRING_SIZE];

    char tabdatafilename[MAX_FILENAME_SIZE];
    char metadatafilename[MAX_FILENAME_SIZE];

    int getTokens(char* instring, char separator);
    void init_metadata_variables();
    void init_metadata_work_area();
    void initialise_cells();
    void initialise_consistency_equations();
    void print_metadata_variables();
    void SetupIndexingOffsets();
    int GetCellNumber();
    bool getline(FILE *fp, char *buffer);
    bool AttributeMatches(const char *buffer, const char *attribute);
    void strcpy_without_quotes(char *s, const char *t);
    int sscanf2(char* instring, const char* formatstring, char* token1, char* token2);
    bool ReadMetaData();
    bool ValidMetadata();
    bool AddKeyToIndex(int keyindex, char *key, int hierarchy);
    void SortKeyFields();
    int string_length(char *s);
    bool lead_match(char *lead, char *s);
    bool AddKeyFieldHierarchies();
    void AddKeyFieldIndexes();
    void AddTotalCodesToIndex();
    bool ParseKeyFields();
    void PrintKeyFileds(const char* outfilename);
    void PrintHighLevelSuppression();
    void DetectProblems();
    void DisplayFieldInformation();
    bool ReadCellData();
    void Print2dMapping(const char* mapfilename);
    void Print3dMapping(const char* mapfilename);
    void Print4dMapping(const char* mapfilename);
    void update4dWeightings();
    void Print5dMapping(const char* mapfilename);
    void Print6dMapping(const char* mapfilename);
    void SetTotalsFor1dTable();
    void SetTotalsFor2dTable();
    void SetTotalsForAnIndex(int dimension);
    void SetTotalsFor3dTable();
    void SetTotalsFor4dTable();
    void SetTotalsFor5dTable();
    void SetTotalsFor6dTable();
    bool CountConsistencyEquationsForAnIndex(int dimension);
    void save_consistency_equation();
    bool PrintConsistencyEquationsForAnIndex(int dimension);
    void CountConsistencyEquationsFor1dTable();
    void PrintConsistencyEquationsFor1dTable();
    void CountConsistencyEquationsFor2dTable();
    void PrintConsistencyEquationsFor2dTable();
    void CountConsistencyEquationsFor3dTable();
    void PrintConsistencyEquationsFor3dTable();
    void CountConsistencyEquationsFor4dTable();
    void PrintConsistencyEquationsFor4dTable();
    void CountConsistencyEquationsFor5dTable();
    void PrintConsistencyEquationsFor5dTable();
    void CountConsistencyEquationsFor6dTable();
    void PrintConsistencyEquationsFor6dTable();
    void allocate_consistency_equation_memory();
    void label_zeroed_cells();
    void label_primary_cells();
    void set_weights_and_lower_and_upper_bounds();
    int suppress_marginals();
    void suppress_large_cells();
    void WriteTableData();
    int find_dot_position(char *filename);
    int find_end_position(char *filename);
    bool file_exists(char *filename);
    void get_directory(char *dir, char *path);

};
