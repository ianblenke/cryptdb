#pragma once

#include <string>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sstream>

const int OPE_SERVER_PORT = 1111;
const int OPE_SERVER_PORT2 = 1113;
const int DEFAULT_OPE_CLIENT_PORT = 1112; 

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

std::string
send_receive(int sock, const std::string & msg);

#define MsgType(m)    \
    m(ENC_INS)		   \
    m(QUERY)   \
    m(INTERACT_FOR_LOOKUP) \
    m(INVALID)

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

inline MsgType string_to_msg_type(const std::string &s)
{
#define __temp_m(n) if (s == #n) return MsgType::n;
MsgType(__temp_m)
#undef __temp_m
    // TODO: possibly raise an exception
    return MsgType::INVALID;
}

std::ostream &
operator<<(std::ostream & o, MsgType mt);

std::istream &
operator>>(std::istream & o, MsgType & mt);

//=========== Message formats ============

// INTERACT_FOR_LOOKUP
// server sends
// 1. type,
// 2. det_val to encrypt,
// 3. no keys at current node in tree,  det_val of each key
// client replies
// 1. the index of the item >= det_val and if there is no such value index = size
// 2. bool value if the index is in fact equal to det_val

// UDF
// ope_enc(det_val, i) -- i indicates whether the value should be inserted
