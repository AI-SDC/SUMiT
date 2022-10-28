#include "stdafx.h"
#include <errno.h>
#include <string.h>
#include "ServerConnection.h"

#define MAX_LINE_LENGTH 8192

ServerConnection::ServerConnection(const char *host, const char *port) {
    sock = sys.open_socket(host, port);
}

ServerConnection::~ServerConnection() {
    sys.close_socket(sock);
}

void ServerConnection::write(const char *buffer, int length) {
    do {
        int bytes_written = sys.write_socket(sock, buffer, length);
        if (bytes_written == -1) {
            logger->error(1, "Error %d writing to socket", errno);
        }

        buffer += bytes_written;
        length -= bytes_written;
    } while (length);
}

void ServerConnection::write(const char *string) {
    write(string, (int)strlen(string));
}

void ServerConnection::put_file(const char* name, const char *filename, const char *query) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        logger->error(1, "Error %d reading content: %s", errno, filename);
    }

    int content_length = sys.get_file_size(fp);

    // Send request
    char request[128];
    sprintf(request, "PUT /%s?", name);
    write(request);
    write(query);
    sprintf(request, " HTTP/1.1\r\nUser-Agent: UWECellSuppression\r\nContent-length: %d\r\n\r\n", content_length);
    write(request);

    // Send content
    char buffer[1500];

    int bytes_remaining = content_length;
    int bytes_to_transfer;

    do {
        if (bytes_remaining > (int)sizeof(buffer)) {
            bytes_to_transfer = sizeof(buffer);
        } else {
            bytes_to_transfer = bytes_remaining;
        }

        int bytes_read = (int)fread(buffer, 1, bytes_to_transfer, fp);
        if (bytes_read != bytes_to_transfer) {
            logger->error(1, "Error %d reading content: %s", errno, filename);
        }

        write(buffer, bytes_read);

        bytes_remaining -= bytes_read;
    } while (bytes_remaining);

    fclose(fp);

    // Get reply header
    get_reply_header(201);
}

int ServerConnection::read_line(char *line) {
    int length = 0;
    bool done = false;
    do {
        char c;
        int rc = sys.read_socket(sock, &c, sizeof(c));
        if (rc < 0) {
            logger->error(1, "Error %d reading from socket", errno);
            done = true;
        } else if (rc > 0) {
            switch (c) {
                case '\r':
                    // Ignore
                    break;

                case '\n':
                    done = true;
                    break;

                default:
                    *line++ = c;
                    length++;

                    if (length == (MAX_LINE_LENGTH - 1)) {
                        logger->error(1, "Reply from server exceeded maximum line length");
                    }
            }
        }
    } while (! done);

    *line = '\0';

    return length;
}

int ServerConnection::get_reply_header(int expected_status) {
    // Read HTTP reply header
    int status = 0;
    int content_length = 0;
    bool done = false;
    char line[MAX_LINE_LENGTH];
    do {
        read_line(line);
        if (sscanf(line, "HTTP/1.1  %d", &status) == 1) {
            if (status != expected_status) {
                logger->error(1, "Server returned status %d", status);
            }
        } else if (sscanf(line, "Content-length: %d", &content_length) == 1) {
            // Consume remainder of reply header (to blank line)
            do {
                int length = read_line(line);
                if (length == 0) {
                    done = true;
                }
            } while (! done);
        }
    } while (! done);

    return content_length;
}

int ServerConnection::get(const char *resource, char *reply_buffer, int reply_buffer_length) {
    // Send request
    char request[128];
    sprintf(request, "GET /%s HTTP/1.1\r\n", resource);
    write(request);

    // Get reply header
    int content_length = get_reply_header(200);

    // Get reply content
    if (content_length < reply_buffer_length) {
        int bytes = sys.read_socket(sock, reply_buffer, content_length);
        if (bytes != content_length) {
            logger->error(1, "Error %d reading content from server", errno);
        }
        reply_buffer[bytes] = '\0';
    } else {
        logger->error(1, "Content length %d is too long", content_length);
        reply_buffer[0] = '\0';
    }

    return content_length;
}

int ServerConnection::get_file(const char *filename, const char *resource) {
    // Send request
    char request[128];
    sprintf(request, "GET /%s HTTP/1.1\r\n", resource);
    write(request);

    // Get reply header
    int content_length = get_reply_header(200);

    // Get reply content
    FILE *fp = fopen(filename, "w");

    char buffer[1500];

    int bytes_remaining = content_length;
    int bytes_to_transfer;

    do {
        if (bytes_remaining > (int)sizeof(buffer)) {
            bytes_to_transfer = sizeof(buffer);
        } else {
            bytes_to_transfer = bytes_remaining;
        }

        int bytes_read = sys.read_socket(sock, buffer, bytes_to_transfer);
        if (fp != NULL) {
            int bytes_written = (int)fwrite(buffer, 1, bytes_read, fp);
            if (bytes_written != bytes_read) {
                logger->error(1, "Unable to write content: %s", resource);
            }
        }

        bytes_remaining -= bytes_read;
    } while (bytes_remaining);

    if (fp != NULL) {
        fclose(fp);
    }

    return content_length;
}
