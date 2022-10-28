#include "stdafx.h"
#include <string.h>
#include <stdlib.h>
#include "Solver.h"
#include "ServerConnection.h"

Solver::Solver(const char *host, const char *port) {
    this->host = host;
    this->port = port;

    ServerConnection *server = new ServerConnection(host, port);
    server->get("session", session, sizeof(session));
    delete server;

    if ((session[0] == '0') && (session[1] == '\0')) {
        // No session available
        throw SESSION_EXCEPTION;
    }
}

Solver::~Solver() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "close?session=%s", session);
    server->get(request, reply, sizeof(reply));
    delete server;
}

char *Solver::getSession() {
    return session;
}


void Solver::run_model(const char *injjfilename, const char *perm_filename, const char *protection_type, const char *model_type, double max_cost) {
    char query[8192] = {'\0'};

    // Transfer input JJ file to server
    ServerConnection *server = new ServerConnection(host, port);

    strcat(query, "session=");
    strcat(query, session);

    server->put_file("file", injjfilename, query);

    delete server;

    // Transfer permutation file to server and run model
    server = new ServerConnection(host, port);

    sprintf(query, "session=%s&protection=%s&model=%s&maxcost=%lf", session, protection_type, model_type, max_cost);

    server->put_file("perm", perm_filename, query);

    delete server;
}

// The remote solver interprets a max_cost of zero to mean unlimited cost (and hence no early termination)
void Solver::runProtection(const char *injjfilename, const char *perm_filename, int protection_type, int model_type, double max_cost) {
    char *protection = NULL;
    char *model = NULL;

    switch (protection_type) {
        case INDIVIDUAL_PROTECTION:
            protection = (char *)"individual";
            break;
        case GROUP_PROTECTION:
            protection = (char *)"group";
            break;
        default:
            logger->error(1, "Unknown protection type");
    }

    switch (model_type) {
//        case FULL_MODEL:
//            model = (char *)"full";
//            break;
        case YPLUS_MODEL:
            model = (char *)"yplus";
            break;
        case YMINUS_MODEL:
            model = (char *)"yminus";
            break;
        default:
            logger->error(1, "Unknown model type");
    }

    run_model(injjfilename, perm_filename, protection, model, max_cost);
}

int Solver::getLimit() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "limit");
    server->get(request, reply, sizeof(reply));
    delete server;

    int limit = 0;
    if (sscanf(reply, "%d", &limit) != 1) {
        logger->error(1, "Invalid limit response from server");
    }

    return limit;
}

int Solver::getCores() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "cores");
    server->get(request, reply, sizeof(reply));
    delete server;

    int cores = 0;
    if (sscanf(reply, "%d", &cores) != 1) {
        logger->error(1, "Invalid cores response from server");
    }

    return cores;
}

int Solver::getProtocol() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "protocol");
    server->get(request, reply, sizeof(reply));
    delete server;

    int protocol = 0;
    if (sscanf(reply, "%d", &protocol) != 1) {
        logger->error(1, "Invalid protocol response from server");
    }

    return protocol;
}

int Solver::getStatus() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "status?session=%s", session);
    server->get(request, reply, sizeof(reply));
    delete server;

    int status;
    if (sscanf(reply, "%d", &status) != 1) {
        logger->error(1, "Invalid status response from server (\"%s\")", reply);
    }

    return status;
}

double Solver::getResult() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "result?session=%s", session);
    server->get(request, reply, sizeof(reply));
    delete server;

    double result;
    if (sscanf(reply, "%lf", &result) != 1) {
        logger->error(1, "Invalid result response from server (\"%s\")", reply);
    }

    return result;
}

int Solver::getElapsedTime() {
    char request[128];
    char reply[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "time?session=%s", session);
    server->get(request, reply, sizeof(reply));
    delete server;

    int elapsedTime;
    if (sscanf(reply, "%d", &elapsedTime) != 1) {
        logger->error(1, "Invalid time response from server (\"%s\")", reply);
    }

    return elapsedTime;
}

void Solver::getCostFile(const char* filename) {
    char request[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "costs?session=%s", session);
    server->get_file(filename, request);
    delete server;
}

void Solver::getJJFile(const char* filename) {
    char request[128];

    ServerConnection *server = new ServerConnection(host, port);
    sprintf(request, "file?session=%s", session);
    server->get_file(filename, request);
    delete server;
}
