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


#define MsgType(m)    \
    m(ENC_INS)		   \
    m(QUERY)            
    
typedef enum class MsgType {
#define __temp_m(n) n,
MsgType(__temp_m)
#undef __temp_m
} MsgType;

//TODO: what is seclevel_last needed for?

const std::string mtnames[] = {
#define __temp_m(n) #n,
MsgType(__temp_m)
#undef __temp_m
};

