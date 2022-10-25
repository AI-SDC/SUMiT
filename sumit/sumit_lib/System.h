#pragma once

#ifdef _WIN32

#if _MSC_VER>=1900
#include <inttypes.h>
#else
// Visual Studio 2010 and 2012 don't have inttypes.h
#define PRIdFAST64   "lld"
#endif

#include <winsock2.h>

// Note that MAX_PATH is invalid if an extended length path is specified using the "\\?\" prefix
#define MAX_FILENAME_SIZE MAX_PATH
#define DIRECTORY_SEPARATOR '\\'
#else
#include <inttypes.h>
#include <limits.h>
#include <time.h>

// Note that PATH_MAX is not guaranteed on Linux and longer paths can be created
// There is no easy way round this except by creating buffers dynamically
#define MAX_FILENAME_SIZE PATH_MAX
#define DIRECTORY_SEPARATOR '/'

#define SOCKET int
#endif

#include <stdio.h>

class System {

public:
    System();
    ~System();
    void initialise_sleep(int milliseconds);
    void sleep();
    char* time_string(time_t* tm);
    void make_tempfile(char* filename, int size);
    SOCKET open_socket(const char* host, const char* port);
    void close_socket(SOCKET sock);
    int read_socket(SOCKET sock, char* buffer, int length);
    int write_socket(SOCKET sock, const char* buffer, int length);
    int get_file_size(FILE* fp);
    void copy_file(const char* dest, const char* src);
    int string_case_compare(const char* s1, const char* s2);
    void remove_file(const char* filename);
    char* get_current_working_directory(char* buffer, int size);
    char* base_name(const char* path);
    int exit_status(int status);
    int get_process_id();
    int file_number(FILE *fp);
    int duplicate(int fd);
    int duplicate2(int fd1, int fd2);
    int close(int fd);

private:
#ifdef _WIN32
    int milliseconds;
#else
    struct timespec req;
    struct timespec rem;
#endif

};
