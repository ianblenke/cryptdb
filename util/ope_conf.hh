#pragma once

#include <string>

/*** Configurable mOPE parameters *****/

// For all following defines: 0 = No, 1 = Yes

// Do you want to run connected to a MySQL database backend?
#define WITH_DB 0
// Set the appropriate values for the database (the database with name db_name
// must already exist)
const std::string db_host = "localhost";
const std::string db_user = "root";
const std::string db_passwd = "letmein";
const std::string db_name = "cryptdb";
const int db_port = 3306;

// Do you want to compile the code such that the client and OPE server/database
// are on separate machines, and communication will go over the network?
#define WITH_NET 0
//List the port the OPE server and client will communicate on
const int OPE_SERVER_PORT = 3111;
const int OPE_CLIENT_PORT = 3112;
//List the IP address of the OPE server and client.
#if WITH_NET
const std::string OPE_SERVER_HOST = ""; // set as appropriate
const std::string OPE_CLIENT_HOST = "";
#else
const std::string OPE_SERVER_HOST = "127.0.0.1";
const std::string OPE_CLIENT_HOST = "127.0.0.1";
#endif

// if ope mode is true then trees can no longer use the order operation
// among keys because the real keys are encrypted
#define OPE_MODE 1


// Controls debugging output
#define DEBUG_BARE 0
#define DEBUG 0
#define DEBUG_COMM 0
#define DEBUG_BTREE 0
#define DEBUG_PROOF 0
#define DEBUG_EXP 0
#define DEBUG_TRANSF 0
#define DEBUG_UDF 0

//hardcoding passwords for crypto algorithms just for our eval. Set as desired.
const std::string passwd = "opeopeopeopeopeo";

/**** B tree parameters ****/

// size of key in B tree
const uint b_key_size = 50; //bytes of max size of key_size

const int invalid_index = -1;

const unsigned int b_max_keys = 4;  // max elements in a node


