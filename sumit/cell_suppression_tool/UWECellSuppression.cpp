#include <stdafx.h>
#include <iostream>
#include <time.h>
#include <TabularData.h>
#include <PartitionData.h>
#include <CSVWriter.h>
#include <IncrementalGAProtection.h>
#include <GroupedGAProtection.h>
#include <Unpicker.h>
#include <Eliminate.h>
#include <ServerConnection.h>
#include <ProgressLog.h>
#include <Partitioning.h>
#include <NoPartitioning.h>
#include <LegacyTabularPartitioning.h>
#include <UWECellSuppression.h>
#include <NoPartitioning.h>
#include "optionparser.h"

#include "defaults.h"



#if USE_EXPERIMENTAL_GA
  #include <ConstructiveGAProtection.h>
  #include <LinRegressGAProtection.h>
  #include <fastEliminate.h>
#endif

#if USE_PATOH
 #include <PaToHPartitioning.h>
#endif

#if USE_KAHYPAR
#include <KaHyParPartitioning.h>
#endif

#if USE_HMETIS
#include <HMETISPartitioning.h>
#endif


struct Arg : public option::Arg {

    static option::ArgStatus Unknown(const option::Option& option, bool msg) {
        if (msg) {
            logger->error(1, "Unknown option: %s", option.name);
        }
        
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option& option, bool msg) {
        if (option.arg != 0) {
            return option::ARG_OK;
        }

        if (msg) {
            logger->error(1, "Option requires an argument: %s", option.name);
        }
        
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus NonEmpty(const option::Option& option, bool msg) {
        if ((option.arg != 0) && (option.arg[0] != 0)) {
            return option::ARG_OK;
        }

        if (msg) {
            logger->error(1, "Option requires a non-empty argument: %s", option.name);
        }
        
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option& option, bool msg) {
        if (option.arg != 0) {
            char* endptr = NULL;
            
            strtol(option.arg, &endptr, 10);
        
            if ((endptr != option.arg) && (*endptr == 0)) {
                return option::ARG_OK;
            }
        }
        
        if (msg) {
            logger->error(1, "Option requires a numeric argument: %s", option.name);
        }

        return option::ARG_ILLEGAL;
    }
};

enum optionIndex {
    UNKNOWN, CONSTRUCTIVE, CSV, DEBUGGING, HELP, CORES, GAELIMINATION, GROUPTHRESHOLD, ITERATIONS, LINREGRESS,LOGLEVEL, NOCOSTLIMIT, PARTITIONING, PARTITION1, PARTITION2, PORT, SERVER, SEED, SILENT, TABLE
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::Unknown, "Usage: UWECellSuppression [options]\n\nOptions:"},
    {  CONSTRUCTIVE, 0, "", "constructive", Arg::None,"\t--constructive\tSelect tree-based constructive algorithm"},
    { CORES, 0, "", "cores", Arg::Numeric, "\t--cores\tNumber of CPU cores to use (default automatic)."},
    { CSV, 0, "", "csv", Arg::None, "\t--csv  \tWrite CSV output file (default JJ files only)."},
    { DEBUGGING, 0, "d", "debug", Arg::None, "\t-d --debug\tSelect debug mode."},
    { GAELIMINATION, 0, "", "gaelimination", Arg::None, "\t--gaelimination\tSelect GA elimination."},
    { GROUPTHRESHOLD, 0, "", "groupthreshold", Arg::Numeric, "\t--groupthreshold\tThreshold for grouped protection (default 151)."},
    { HELP, 0, "h", "help", Arg::None, "\t-h --help\tPrint usage and exit."},
    { ITERATIONS, 0, "", "iterations", Arg::Numeric, "\t--iterations\tMaximum number of iterations (default unlimited)."},
    { LINREGRESS, 0, "", "linearregression", Arg::None,"\t--linregress\tSelect linear regression-based constructive algorithm"},
    { LOGLEVEL, 0, "l", "loglevel", Arg::Numeric, "\t-l --loglevel\tLogging level to use (default 0)."},
    { NOCOSTLIMIT, 0, "", "nocostlimit", Arg::None, "\t--nocostlimit\tAlways run the solver to completion."},
    { PARTITIONING, 0, "", "partitioning", Arg::NonEmpty, "\t--partitioning\tPartitioning algorithm (default none)."},
    { PARTITION1, 0, "", "part1", Arg::NonEmpty, "\t--part1\tPartition parameter 1 (legacy partitioning only)."},
    { PARTITION2, 0, "", "part2", Arg::NonEmpty, "\t--part2\tPartition parameter 2 (legacy partitioning only)."},
    { PORT, 0, "", "port", Arg::NonEmpty, "\t--port\tServer port (default 1081)."},
    { SEED, 0, "", "seed", Arg::Numeric, "\t--seed\tSeed for random number generator."},
    { SERVER, 0, "", "server", Arg::NonEmpty, "\t--server\tServer name or IP address (default localhost)."},
    { SILENT, 0, "s", "silent", Arg::None, "\t-s --silent\tNo console progress display."},
    { TABLE, 0, "", "table", Arg::NonEmpty, "\t--table\tTable input file (TAB or JJ format)."},
    { 0, 0, 0, 0, 0, 0}
};

bool debugging = false;
Logger* logger = NULL;
System sys;
ProgressLog* plog = NULL;

#define SAVE_BEST 1
int save_best= SAVE_BEST;

bool silent = false;

char tableFilename[MAX_FILENAME_SIZE];
char tabdataFilename[MAX_FILENAME_SIZE];
char metadataFilename[MAX_FILENAME_SIZE];

char recombined_jj_filename[MAX_FILENAME_SIZE];

char partitioning_algorithm[MAX_KEY_SIZE];
char partition_by_1[MAX_KEY_SIZE];
char partition_by_2[MAX_KEY_SIZE];

char server[MAX_HOST_NAME_SIZE];
char port[MAX_PORT_NUMBER_SIZE];

unsigned int seed = 0;
unsigned int cores = 0;
int groupThreshold = 151;
bool gaElimination = false;
bool gaConstructive = false;
bool gaLinRegress = false;
int iterations = -1;
int total_counted_evals=0;
int logLevel = 0;
bool csv_output = false;
bool no_cost_limit = false;

bool tabular_format = false; // Whether the input file is in TAB format (else JJ)
bool legacy_partitioning = false; // Whether legacy tabular partitioning is in use

int GetRequiredTimeUnits(int NumberOfPrimaries) {
    if (NumberOfPrimaries < 70) {
        return 1;
    } else if (NumberOfPrimaries < 85) {
        return 2;
    } else if (NumberOfPrimaries < 100) {
        return 4;
    } else if (NumberOfPrimaries < 120) {
        return 6;
    } else if (NumberOfPrimaries < 135) {
        return 8;
    } else if (NumberOfPrimaries < 150) {
        return 13;
    } else if (NumberOfPrimaries < 200) {
        return 35;
    } else if (NumberOfPrimaries < 250) {
        return 50;
    } else if (NumberOfPrimaries < 300) {
        return 70;
    } else if (NumberOfPrimaries < 350) {
        return 80;
    } else if (NumberOfPrimaries < 400) {
        return 90;
    } else if (NumberOfPrimaries < 500) {
        return 120;
    } else if (NumberOfPrimaries < 600) {
        return 130;
    } else if (NumberOfPrimaries < 700) {
        return 140;
    } else if (NumberOfPrimaries < 800) {
        return 150;
    } else if (NumberOfPrimaries < 1000) {
        return 200;
    } else if (NumberOfPrimaries < 2000) {
        return 250;
    } else {
        return 300;
    }
}

bool parseInputs(int argc, char *argv[])
{
    bool ok = true;
    // Skip the program name
    argc--;
    argv++;
    
    option::Stats stats(usage, argc, argv);
    
#if defined ( _WIN32) || defined (__clang__)
    // Use calloc() to allocate 0-initialized memory. It's not the same as properly constructed elements, but good enough
    option::Option* options = (option::Option*)calloc(stats.options_max, sizeof (option::Option));
    option::Option* buffer = (option::Option*)calloc(stats.buffer_max, sizeof (option::Option));
#else
    // C99 VLAs for C++ with proper constructor calls
    option::Option options[stats.options_max];
    option::Option buffer[stats.buffer_max];
#endif
    
    option::Parser parse(usage, argc, argv, options, buffer);
    
    if (parse.error()) {
        logger->error(1, "Unable to parse command line arguments");
    }
    
    if (options[HELP] || argc == 0) {
        int columns = getenv("COLUMNS") ? atoi(getenv("COLUMNS")) : 80;
        option::printUsage(fwrite, stderr, usage, columns);
        ok = false;
        return ok;
    }
    
    for (int i = 0; i < parse.optionsCount(); ++i) {
        option::Option& opt = buffer[i];
        
        switch (opt.index()) {
                
            case CONSTRUCTIVE:
                logger->log(1,"Tree-Based Constructive");
                gaConstructive = true;
                break;
                
            case CORES:
                logger->log(1, "Cores: %s", opt.arg);
                sscanf(opt.arg, "%u", &cores);
                break;
                
            case CSV:
                logger->log(1, "CSV output");
                csv_output = true;
                break;
                
            case DEBUGGING:
                logger->log(1, "Debug mode");
                debugging = true;
                break;
                
            case GAELIMINATION:
                logger->log(1, "GA elimination");
                gaElimination = true;
                break;
                
            case GROUPTHRESHOLD:
                logger->log(1, "Group threshold: %s", opt.arg);
                sscanf(opt.arg, "%u", &groupThreshold);
                break;
                
            case ITERATIONS:
                logger->log(1, "Iterations: %s", opt.arg);
                sscanf(opt.arg, "%u", &iterations);
                break;
            case LINREGRESS:
                logger->log(1,"LinearRegression-based constructive");
                gaLinRegress=true;
                break;
                
            case LOGLEVEL:
                logger->log(1, "Log level: %s", opt.arg);
                sscanf(opt.arg, "%u", &logLevel);
                logger->setLevel(logLevel);
                break;
                
            case NOCOSTLIMIT:
                logger->log(1, "No cost limit");
                no_cost_limit = true;
                break;
                
            case PARTITIONING:
                logger->log(1, "Partitioning: %s", opt.arg);
                if (strlen(opt.arg) < MAX_KEY_SIZE) {
                    strcpy(partitioning_algorithm, opt.arg);
                } else {
                    logger->error(1, "Partitioning algorithm key name is too long");
                }
                break;
                
            case PARTITION1:
                logger->log(1, "Part1: %s", opt.arg);
                if (strlen(opt.arg) < MAX_KEY_SIZE) {
                    strcpy(partition_by_1, opt.arg);
                } else {
                    logger->error(1, "Partition 1 key name is too long");
                }
                break;
                
            case PARTITION2:
                logger->log(1, "Part2: %s", opt.arg);
                if (strlen(opt.arg) < MAX_KEY_SIZE) {
                    strcpy(partition_by_2, opt.arg);
                } else {
                    logger->error(1, "Partition 2 key name is too long");
                }
                break;
                
            case PORT:
                logger->log(1, "Port: %s", opt.arg);
                if (strlen(opt.arg) < MAX_PORT_NUMBER_SIZE) {
                    strcpy(port, opt.arg);
                } else {
                    logger->error(1, "Port key name is too long");
                }
                break;
                
            case SERVER:
                logger->log(1, "Server: %s", opt.arg);
                if (strlen(opt.arg) < MAX_HOST_NAME_SIZE) {
                    strcpy(server, opt.arg);
                } else {
                    logger->error(1, "Server key name is too long");
                }
                break;
                
            case SEED:
                logger->log(1, "Seed: %s", opt.arg);
                sscanf(opt.arg, "%u", &seed);
                break;
                
            case SILENT:
                logger->log(1, "Silent");
                silent = true;
                break;
                
            case TABLE:
                logger->log(1, "Table file: %s", opt.arg);
                size_t len = strlen(opt.arg);
                if (len < MAX_FILENAME_SIZE) {
                    strcpy(tableFilename, opt.arg);
                    char ext[5];
                    for (int i = 0; i <= 4; i++) {
                        ext[i] = tableFilename[len - 4 + i];
                    }
                    if (sys.string_case_compare(ext, ".tab") == 0) {
                        strcpy(tabdataFilename, opt.arg);
                        strcpy(metadataFilename, opt.arg);
                        strcpy(metadataFilename + len - 4, ".rda");
                        strcpy(tableFilename, sys.base_name(tableFilename));
                        strcat(tableFilename, ".jj");
                        tabular_format = true;
                    }
                } else {
                    logger->error(1, "Table file name is too long");
                }
                break;
        }
    }
    
    for (int i = 0; i < parse.nonOptionsCount(); ++i) {
        logger->log(1, "Non-option argument #%d is %s", i, parse.nonOption(i));
    }
    
    if (csv_output && (! tabular_format)) {
        logger->error(1, "CSV output is only available from TAB format input");
    }
    
    
    return ok;
    
}

void Standard_Inits(void)
{
    logger = new Logger(1, "Log.txt");
    logger->log(1, "UWECellSuppression v%s", VERSION);
    
    if (sizeof(int) < 4) {
        logger->error(1, "Integer size must be at least four bytes");
    }
    
    strcpy(server, "localhost");
    strcpy(port, "1081");
    
    tableFilename[0] = '\0';
    metadataFilename[0] = '\0';
    
    partitioning_algorithm[0] = '\0';
    partition_by_1[0] = '\0';
    partition_by_2[0] = '\0';

}

PartitionData*  makePartitionFiles(int *number_of_partitions)
{
    int numbermade=0;
    logger->log(1, "Partitioning");
    
    Partitioning* partitioning;
    
    // Convert TAB file input data to JJ format
    if (tabular_format) {
        TabularData* tabular = new TabularData(metadataFilename, tabdataFilename);
        tabular->PrintJJFile(tableFilename);
        tabular->PrintJJFileMapping("Mapping.txt");
        delete tabular;
    }
    
    // Select partitioning algorithm
    if (sys.string_case_compare(partitioning_algorithm, "legacy") == 0) {
        if (tabular_format) {
            partitioning = new LegacyTabularPartitioning(metadataFilename, tabdataFilename, partition_by_1, partition_by_2);
            legacy_partitioning = true;
        } else {
            logger->error(1, "Legacy partitioning is only available for TAB format input");
            
            // Keep the compiler from complaining
            partitioning = NULL;
        }
    }
    else if (sys.string_case_compare(partitioning_algorithm, "hmetis") == 0) {
        #if USE_HMETIS
          partitioning = new HMETISPartitioning(tableFilename);
        #else
          logger->error(1, "Third party partitioning algorithms not supported in this version");
          logger->error(1, "Not dividingthe table into partitions");
          partitioning = new NoPartitioning(tableFilename);
        #endif
    }
    else if (sys.string_case_compare(partitioning_algorithm, "kahypar") == 0) {
        #if USE_KAHYPAR
        partitioning = new KaHyParPartitioning(tableFilename);
        #else
        logger->error(1, "Third party partitioning algorithms not supported in this version");
        logger->error(1, "Not dividingthe table into partitions");
        partitioning = new NoPartitioning(tableFilename);
        #endif
    }
    else if (sys.string_case_compare(partitioning_algorithm, "patoh") == 0) {
        #if USE_PATOH
           #ifdef _WIN32
            logger->error(1, "PaToH partitioning is not available under Windows");
           #else
             partitioning = new PaToHPartitioning(tableFilename);
          #endif
        #else
        logger->error(1, "Third party partitioning algorithms not supported in this version");
        logger->error(1, "Not dividingthe table into partitions");
        partitioning = new NoPartitioning(tableFilename);
        #endif

    }
    else if ((sys.string_case_compare(partitioning_algorithm, "none") == 0)
               || (sys.string_case_compare(partitioning_algorithm, "no") == 0)
               || (partitioning_algorithm[0] == '\0')) {
        partitioning = new NoPartitioning(tableFilename);
    } 
    else {
        logger->error(1, "Unknown partitioning algorithm %s", partitioning_algorithm);
        
        // Keep the compiler from complaining
        partitioning = NULL;
    }
    
    numbermade = partitioning->get_number_of_partitions();
    
    PartitionData* partition = new PartitionData[numbermade];
    
    for (int i = 0; i < numbermade; i++) {
        logger->log(2, "Partition %d", i + 1);
        
        partition[i].initialise(i + 1);
        
        partitioning->write_partitioned_jj_file(i, partition[i].in_jj_file);
        
        // The call to get_number_of_cells must only occur after the call to write_partitioned_jj_file
        logger->log(2, "Partition %d size %d", i + 1, partitioning->get_number_of_cells(i));
        
        // The call to get_number_of_primary_cells must only occur after the call to write_partitioned_jj_file
        partition[i].number_of_primary_cells = partitioning->get_number_of_primary_cells(i);
        
        // To cover the case where the GA is not run on a partition, create an initial output file equal to the input file
        sys.copy_file(partition[i].out_jj_file, partition[i].in_jj_file);
    }
    
    delete partitioning;

    //Send results back to main()_
    *number_of_partitions = numbermade;
    return partition;
}

void AllocatePartitionRuntimes(PartitionData * partition, int number_of_partitions )
{

// Calculate the execution time for each partition
int total_required_time_units = 0;
int total_allocated_execution_time = 0;
    int allocated_execution_time;
    
for (int i = 0; i < number_of_partitions; i++) {
    total_required_time_units = total_required_time_units + GetRequiredTimeUnits(partition[i].number_of_primary_cells);
}
int time_per_unit = TOTAL_EXECUTION_TIME / total_required_time_units;

for (int i = 0; i < number_of_partitions; i++) {
    allocated_execution_time = time_per_unit * GetRequiredTimeUnits(partition[i].number_of_primary_cells);
    
    if (allocated_execution_time < 1000) {
        allocated_execution_time = 1000;
    }
    
    partition[i].execution_time_seconds = allocated_execution_time;
    logger->log(3, "Partition %d allocated %d seconds execution time", i, allocated_execution_time);
    
    total_allocated_execution_time = total_allocated_execution_time + allocated_execution_time;
}
logger->log(3, "Total_allocated_execution_time: %d", total_allocated_execution_time);
}


void  CreateAndTestFirstSetOfSolutionsForPartition(PartitionData *partition, int i){
logger->log(1, "Protect partition %d", i + 1);


#if USE_EXPERIMENTAL_GA

if (gaConstructive==true  ) {
    partition[i].protection = new ConstructiveGAProtection(server, port, partition[i].in_jj_file, partition[i].out_jj_file, partition[i].sam_file, seed, cores, partition[i].execution_time_seconds, gaElimination);
}
else if (gaLinRegress==true ) {
        partition[i].protection = new LinRegressGAProtection(server, port, partition[i].in_jj_file, partition[i].out_jj_file, partition[i].sam_file, seed, cores, partition[i].execution_time_seconds, gaElimination);
    }
else if (partition[i].number_of_primary_cells < groupThreshold)
{
    partition[i].protection = new IncrementalGAProtection(server, port, partition[i].in_jj_file, partition[i].out_jj_file, partition[i].sam_file, seed, cores, partition[i].execution_time_seconds, gaElimination);
}
else
{
    partition[i].protection = new GroupedGAProtection(server, port, partition[i].in_jj_file, partition[i].out_jj_file, partition[i].sam_file, seed, cores, partition[i].execution_time_seconds, gaElimination);
}

#else
if (partition[i].number_of_primary_cells < groupThreshold)
{
    partition[i].protection = new IncrementalGAProtection(server, port, partition[i].in_jj_file, partition[i].out_jj_file, partition[i].sam_file, seed, cores, partition[i].execution_time_seconds, gaElimination);
}
else
{
    partition[i].protection = new GroupedGAProtection(server, port, partition[i].in_jj_file, partition[i].out_jj_file, partition[i].sam_file, seed, cores, partition[i].execution_time_seconds, gaElimination);
}
#endif    
    
    
    
    
partition[i].cost = partition[i].protection->fitness();
}


void UnpickFile(char *jj_filename, double cost)
{
    Unpicker* unpicker = new Unpicker(jj_filename);
    unpicker->Attack();
    char log_message[200];
    char outfilename[256];
    plog->write_log_file(jj_filename, unpicker->GetNumberOfCells(), unpicker->GetNumberOfPrimaryCells(), unpicker->GetNumberOfSecondaryCells(), unpicker->GetNumberOfPrimaryCells_ValueKnownExactly(), unpicker->GetNumberOfPrimaryCells_ValueKnownWithinProtection(), total_counted_evals, cost);
    
    if (unpicker->GetNumberOfPrimaryCells_ValueKnownExactly() > 0) {
        sprintf(log_message, "ERROR> Protection failure, %d primary cells known exactly", unpicker->GetNumberOfPrimaryCells_ValueKnownExactly());
        plog->write_log_message(log_message);
        sprintf(outfilename,"ExposedFully_%s.txt",jj_filename);
        unpicker->print_exact_exposure(outfilename);
    } else if (unpicker->GetNumberOfPrimaryCells_ValueKnownWithinProtection() > 0) {
        sprintf(log_message, "WARNING> Protection failure, %d primary cells known within limits", unpicker->GetNumberOfPrimaryCells_ValueKnownWithinProtection());
        plog->write_log_message(log_message);
        sprintf(&outfilename[0],"ExposedPartially_%s.txt",jj_filename);
        unpicker->print_partial_exposure(outfilename);
    }
    
    plog->write_newline();
    delete unpicker;
}
void RecombinePartitionsAndTestResultforThisIteration( PartitionData *partition,int number_of_partitions, int iteration)
{
    logger->log(1, "Recombine partitions");
    
    sprintf(recombined_jj_filename, "Recombined_%05d.jj", iteration);
    
    
    
    if (legacy_partitioning) {
        // Write recombined JJ file
        TabularData* recombined_tabular = new TabularData(metadataFilename, tabdataFilename);
        
        for (int i = 0; i < number_of_partitions; i++) {
            recombined_tabular->UpdateStatusFromCSVFile(partition[i].csv_file);
        }
        
        recombined_tabular->PrintJJFile(recombined_jj_filename);
        
        delete recombined_tabular;
        
        // Write recombined CSV file
        if (csv_output) {
            char recombined_csv_filename[sizeof("Recombined_XXXXX.csv")];
            sprintf(recombined_csv_filename, "Recombined_%05d.csv", iteration);
            CSVWriter* jj_format_data = new CSVWriter(recombined_jj_filename);
            jj_format_data->write_csv_file("Mapping.txt", recombined_csv_filename);
            delete jj_format_data;
        }
    }
    else {
        // Write recombined JJ file
        JJData* recombined_jj = new JJData(tableFilename);
        
        for (int i = 0; i < number_of_partitions; i++) {
            recombined_jj->recombine(partition[i].out_jj_file);
        }
        
        recombined_jj->write_jj_file(recombined_jj_filename);
        
        delete recombined_jj;
    }
    
    // Unpick recombined if necessary
    logger->log(2, "Unpick recombined");
    double combined_cost=0.0;
    for (int i = 0; i < number_of_partitions; i++)
        combined_cost+= partition[i].cost;
    UnpickFile(recombined_jj_filename, combined_cost);
    logger->log(2,"done");
}


void CleanupPartitions(PartitionData * partition, int number_of_partitions){
    for (int i = 0; i < number_of_partitions; i++) {
        delete partition[i].protection;
        partition[i].protection = NULL;
        
        // Tidy up legacy partitioning temporary files
        if ((! debugging) && (legacy_partitioning)) {
            sys.remove_file(partition[i].csv_file);
            sys.remove_file(partition[i].map_file);
            sys.remove_file(partition[i].metadata_file);
        }
    }
    
    // Tidy up temporary files
    if (! debugging) {
        for (int i = 0; i < number_of_partitions; i++) {
            sys.remove_file(partition[i].in_jj_file);
            sys.remove_file(partition[i].out_jj_file);
        }
    }
    
    delete[] partition;
}

void RemoveUnneededFiles(void){
    // Tidy up tabular format temporary files
    if (! debugging) {
        if (tabular_format) {
            sys.remove_file("Mapping.txt");
        }
        
        if (legacy_partitioning) {
            // Remove the hierarchy files created by legacy partitioning
            // This is a hack because the intention is to remove legacy partitioning!
            
#ifdef _WIN32
            const char* rm = "del";
#else
            const char* rm = "rm";
#endif
            char cmd[MAX_KEY_SIZE + 10];
            
            sprintf(cmd, "%s %s*.txt", rm, partition_by_1);
            int rc = system(cmd);
            
            if (rc != 0) {
                logger->log(3, "Error %d removing hierarchy files: %s", rc, partition_by_1);
            }
            
            sprintf(cmd, "%s %s*.txt", rm, partition_by_2);
            rc = system(cmd);
            
            if (rc != 0) {
                logger->log(3, "Error %d removing hierarchy files: %s", rc, partition_by_2);
            }
        }
    }
}

void Elimination(char *recombined_jj_filename){
    logger->log(1, "Eliminate");
    
    double cost_after_elimination = 0.0;
#if USE_SMALLER_ELIMINATION
    CEliminate* eliminate = new CEliminate(recombined_jj_filename);
#else
    Eliminate* eliminate = new Eliminate(recombined_jj_filename);
#endif
    eliminate->RemoveExcessSuppression();
    cost_after_elimination = eliminate->getCost();
    eliminate->WriteJJFile("Eliminated.jj");
    delete eliminate;
    
    // Unpick eliminated
    logger->log(1, "Unpick eliminated");
    char elimname[256];
    sprintf(elimname,"Eliminated.jj");
    UnpickFile(elimname, cost_after_elimination);
    
    
    // Write eliminated CSV file
    if (tabular_format && csv_output) {
        CSVWriter* jj_format_data = new CSVWriter("Eliminated.jj");
        jj_format_data->write_csv_file("Mapping.txt", "Eliminated.csv");
        
        delete jj_format_data;
    }
}

int main(int argc, char* argv[]) {
    int number_of_partitions=0;
    time_t start_time = time(NULL);
    
    Standard_Inits();//sets up log files
    //read input and set values of variables for table file name, task etc etc
    if (parseInputs(argc, argv) != true)
        return 0;
    
    plog = new ProgressLog("ProgressLog.txt");
    // Switch off console output if required
    logger->consoleSummary(! silent);

    // Create one or more partitions: each with its own JJ and mapping files, and holding its own state
    PartitionData* partition = makePartitionFiles(&number_of_partitions);
    AllocatePartitionRuntimes( partition,  number_of_partitions );
    
   
    // Initial evaluation of each partition to create first set of partial solutions
    for (int i = 0; i < number_of_partitions; i++) {
            CreateAndTestFirstSetOfSolutionsForPartition(partition, i);
    }

    
    //this is the main iteration loop of the algorithm
    // Each partition gets further runtime to produce a better partial solution,
    //as long as there is run time left *for that partition*
    bool done;
    int iteration = 0;
    do {
        logger->log(1, "Iteration %d", iteration);
        total_counted_evals=0;
        done = true;
        for (int i = 0; i < number_of_partitions; i++) {

            //each partition gets its own allocation of runtime since some may be faster to iterate
            if (! partition[i].protection->time_to_terminate()) {
                if (iteration > 0) {
                    //run next generation of EA optimisation to create new partial solutions
                    partition[i].protection->protect(! no_cost_limit);
                    partition[i].cost = partition[i].protection->fitness();
                }
                done = false;
                total_counted_evals = total_counted_evals + partition[i].protection->number_of_evaluations();
            }
            
            
            //unpick partition (which may be the only one) as a safety check
            logger->log(1, "Unpick partition %d", i + 1);
            UnpickFile(partition[i].out_jj_file, partition[i].cost);
            
 
            // Create partitioned CSV files if using legacy parittioning
            if (legacy_partitioning){
                CSVWriter* jj_format_data = new CSVWriter(partition[i].out_jj_file);
                jj_format_data->write_csv_file(partition[i].map_file, partition[i].csv_file);
                delete jj_format_data;
            }
        }

        // having created partial solution for each partition, put them back together to create new candidate solution
        //this will just do the reporting if there is only one partition
         RecombinePartitionsAndTestResultforThisIteration( partition,number_of_partitions, iteration);


        if (iteration == iterations) {
            done = true;
        }
        
        iteration++;
    } while (! done);

    CleanupPartitions(partition, number_of_partitions);//do this here as the post processign can be memory-hungry
    
    // Elimination Post processing to optimise final solution
    Elimination(recombined_jj_filename);
 

    // Finish up
    RemoveUnneededFiles();
    logger->log(1, "Total elapsed time %d seconds", (long)(time(NULL) - start_time));
    logger->log(1, "Done");
    delete logger;
    delete plog;
    
    return 0;
}
