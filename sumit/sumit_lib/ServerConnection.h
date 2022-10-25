#pragma once

#include "stdafx.h"

class ServerConnection {

public:
    ServerConnection(const char *host, const char *port);
    ~ServerConnection();
    void put_file(const char* name, const char *filename, const char *session);
    int get(const char *request, char *reply_buffer, int reply_buffer_length);
    int get_file(const char *filename, const char *session);
    
private:
    SOCKET sock;

    void write(const char *buffer, int length);
    void write(const char *string);
    int read_line(char *line);
    int get_reply_header(int expectedStatus);
    
};

