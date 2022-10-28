#include "stdafx.h"

#ifdef _WIN32
#include <direct.h>
#include <ws2tcpip.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#else
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#endif

#include <string.h>
#include <errno.h>
#include <time.h>

System::System() {
#ifdef _WIN32
    // Initialise winsock
    WSADATA wsaData;
    int result;

    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        // Cannot use logger here as this constructor may be called before the logger is initialised
        printf("Error: WSAStartup failed: %d\n", result);
        exit(1);
    }

    milliseconds = 0;
#else
    // Sleep methods
    req.tv_sec = 0;
    req.tv_nsec = 0;
#endif
}

System::~System() {
#ifdef _WIN32
    // Terminate Winsock
    WSACleanup();
#endif
}

void System::initialise_sleep(int milliseconds) {
#ifdef _WIN32
    this->milliseconds = milliseconds;
#else
    if (milliseconds < 1000) {
         req.tv_sec = 0;
         req.tv_nsec = milliseconds * 1000000;
    } else {
         req.tv_sec = milliseconds / 1000;
         req.tv_nsec = (milliseconds % 1000) * 1000000;
    }
#endif
}

void System::sleep() {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    nanosleep(&req, &rem);
#endif
}

char* System::time_string(time_t* tm) {
    static char str[100];
    time_t t;

    if (tm) {
        t =* tm;
    } else {
        t = time(NULL);
    }

#ifdef _WIN32
    strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S %z", localtime(&t));
#else
    strftime(str, sizeof(str), "%F %T%z", localtime(&t));
#endif

    return str;
}

void System::make_tempfile(char* filename, int size) {
    if (size < MAX_FILENAME_SIZE) {
        logger->error(9, "Temporary file name buffer is too small (%d)", size);
    }

#ifdef _WIN32
    if (! GetTempFileNameA(".", "UWE",  0,  filename)) {
        logger->error(2, "Unable to generate temporary file name");
    }
#else
    strcpy(filename, "UWECSPXXXXXX");
    int f;

    for (int i = 6; i < 12; i++) {
        filename[i] = 'X';
    }

    if ((f = mkstemp(filename)) == -1) {
        logger->error(2, "Unable to generate temporary file name");
    } else {
        close(f);
    }
#endif
}

SOCKET System::open_socket(const char* host, const char* port) {
    struct addrinfo hints;
    struct addrinfo* result;
    struct addrinfo* rp;
    int status;
    SOCKET sock = -1;

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#ifdef _WIN32
    hints.ai_protocol = IPPROTO_TCP;
#endif

    status = getaddrinfo(host, port, &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        logger->error(3, "Error getting address: %ls", gai_strerror(status));
#else
        logger->error(3, "Error getting address: %s", gai_strerror(status));
#endif
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }

        if (connect(sock, rp->ai_addr, (int)rp->ai_addrlen) != -1) {
            break;
        }
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }

    if (rp == NULL) {
        logger->error(4, "Error %d connecting to %s", errno, host);
    }

    freeaddrinfo(result);

    return sock;
}

void System::close_socket(SOCKET sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int System::read_socket(SOCKET sock, char* buffer, int length) {
    return (int)recv(sock, buffer, length, 0);
}

int System::write_socket(SOCKET sock, const char* buffer, int length) {
    return (int)send(sock, buffer, length, 0);
}

// Note that this method resets the read position to the start of the file
int System::get_file_size(FILE* fp) {
#ifdef _WIN32
    // Need to read the file to enumerate the content length due to CRLF translation
    int size = 0;
    boolean done = false;
    do {
        if (fgetc(fp) != EOF) {
            size++;
        }
        else if (ferror(fp)) {
            logger->error(5, "Error %d enumerating file size", errno);
        }
        else {
            done = true;
        }
    } while (! done);
#else
    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
#endif

    fseek(fp, 0, SEEK_SET);

    return size;
}

void System::copy_file(const char* dest, const char* src) {
    FILE* fp_src = fopen(src, "r");
    if (fp_src == NULL) {
        logger->error(6, "Error %d opening file: %s", errno, src);
    }

    FILE* fp_dest = fopen(dest, "w");

    int content_length = this->get_file_size(fp_src);

    char buffer[4096];

    int bytes_remaining = content_length;
    int bytes_to_transfer;

    do {
        if (bytes_remaining > (int)sizeof(buffer)) {
            bytes_to_transfer = sizeof(buffer);
        } else {
            bytes_to_transfer = bytes_remaining;
        }

        int bytes_read = (int)fread(buffer, 1, bytes_to_transfer, fp_src);
        if (bytes_read != bytes_to_transfer) {
            logger->error(7, "Error %d reading file: %s", errno, src);
        }

        if (fp_dest != NULL) {
            int bytes_written = (int)fwrite(buffer, 1, bytes_read, fp_dest);
            if (bytes_written != bytes_read) {
                logger->error(8, "Unable to write to file: %s", dest);
            }
        }

        bytes_remaining -= bytes_read;
    } while (bytes_remaining);

    fclose(fp_dest);
    fclose(fp_src);
}

int System::string_case_compare(const char* s1, const char* s2) {
#ifdef _WIN32
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

void System::remove_file(const char* filename) {
    if (remove(filename) == -1) {
        logger->log(3, "Error %d removing file: %s", errno, filename);
    }
}

char* System::get_current_working_directory(char* buffer, int size) {
#ifdef _WIN32
    return _getcwd(buffer, size);
#else
    return getcwd(buffer, size);
#endif
}

char* System::base_name(const char* path) {
    static char base[MAX_FILENAME_SIZE];

#ifdef _WIN32
    _splitpath(path, NULL, NULL, base, NULL);

#else
    // Get the base name with extension
    strcpy(base, basename((char* )path));

    // Remove the extension
    char* p = base + strlen(base) - 1;
    while (*p != '.') {
        p--;
    }
    if (*p == '.') {
        *p = '\0';
    }
#endif

    return base;
}

int System::exit_status(int status) {
#ifdef _WIN32
    return status;
#else
    return WEXITSTATUS(status);
#endif
}

int System::get_process_id() {
#ifdef _WIN32
    return _getpid();
#else
    return getpid();
#endif
}

int System::file_number(FILE *fp) {
#ifdef _WIN32
    return _fileno(fp);
#else
    return fileno(fp);
#endif
}

int System::duplicate(int fd) {
#ifdef _WIN32
    return _dup(fd);
#else
    return dup(fd);
#endif
}

int System::duplicate2(int fd1, int fd2) {
#ifdef _WIN32
    return _dup2(fd1, fd2);
#else
    return dup2(fd1, fd2);
#endif
}

int System::close(int fd) {
#ifdef _WIN32
    return _close(fd);
#else
    return ::close(fd);
#endif
}
