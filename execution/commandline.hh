#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include <getopt.h>

struct command_line_opts {
  static void parse_options(
      int argc,
      char* const* argv,
      command_line_opts& opts) {
    static struct option long_options[] =
      {
        {"hostname"            , required_argument , 0 , 'h'} ,
        {"username"            , required_argument , 0 , 'u'} ,
        {"password"            , required_argument , 0 , 'p'} ,
        {"database"            , required_argument , 0 , 'd'} ,
        {"port"                , required_argument , 0 , 'P'} ,
        {0, 0, 0, 0}
      };
    while (true) {
      int option_index = 0;
      int c = getopt_long(argc, argv, "h:u:p:d:P:c",
                          long_options, &option_index);
      if (c == -1) break;
      switch (c) {
      case 'h': opts.db_hostname         = std::string(optarg); break;
      case 'u': opts.db_username         = std::string(optarg); break;
      case 'p': opts.db_passwd           = std::string(optarg); break;
      case 'd': opts.db_database         = std::string(optarg); break;
      case 'P': opts.db_port             = atoi(optarg);        break;
      case '?': break;
      default:
        throw std::runtime_error("bad option");
      }
    }
  }

  command_line_opts() {}

  // supply default values
  command_line_opts(
    const std::string& db_hostname,
    const std::string& db_username,
    const std::string& db_passwd,
    const std::string& db_database,
    uint16_t db_port) :
    db_hostname(db_hostname),
    db_username(db_username),
    db_passwd(db_passwd),
    db_database(db_database),
    db_port(db_port) {}

  std::string db_hostname;
  std::string db_username;
  std::string db_passwd;
  std::string db_database;
  uint16_t    db_port;
};

namespace {
  std::ostream& operator<<(std::ostream& o, const command_line_opts& opts) {
    o << "{hostname=" << opts.db_hostname
      << ", username=" << opts.db_username
      << ", passwd=XXX, db_database="
      << opts.db_database
      << ", db_port=" << opts.db_port
      << "}";
    return o;
  }
}
