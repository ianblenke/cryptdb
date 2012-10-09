#pragma once

const int OPE_SERVER_PORT = 1111;
const int OPE_CLIENT_PORT = 1112;

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>


// creates a socket and binds it on port
// and returns it
int
create_and_bind(int host_port);
