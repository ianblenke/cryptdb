#pragma once

#include <string>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>

const int OPE_SERVER_PORT = 1111;
const int OPE_CLIENT_PORT = 1112;

const std::string OPE_SERVER_HOST = "127.0.0.1"; // we will need to get these in
						 // as inputs instead
const std::string OPE_CLIENT_HOST = "127.0.0.1";


// creates a socket and binds it on port
// and returns it
int
create_and_bind(int host_port);

// creates a socket and connects through it
int
create_and_connect(std::string host_name, int host_port);

// network message types
typedef enum MsgType {UDF_ENC, START_INTERACT};
