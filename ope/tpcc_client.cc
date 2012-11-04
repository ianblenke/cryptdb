//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "tpcc_util.hh"

#include <iostream>
#include <boost/asio.hpp>
#include <sstream>
#include <time.h>
#include <vector>
#include <util/ope-util.hh>
#include <crypto/blowfish.hh>
#include <crypto/BasicCrypto.hh>
#include <edb/Connect.hh>
#include <utility>
#include <algorithm>

using boost::asio::ip::tcp;

struct sort_first_int {
    bool operator()(const std::pair<uint64_t,int> &left, const std::pair<uint64_t,int> &right) {
        return left.first < right.first;
    }
};

struct sort_first_string {
    bool operator()(const std::pair<std::string,int> &left, const std::pair<std::string,int> &right) {
        return left.first < right.first;
    }
};

int main(int argc, char* argv[])
{

  struct timeval start_time;

  gettimeofday(&start_time, 0);
  std::cout<<start_time.tv_sec<<std::endl;
  std::cout<<start_time.tv_usec<<std::endl;

  Connect * dbconnect;
  dbconnect = new Connect( "localhost", "root", "letmein", "tpcc", 3306);

  DBResult * result;

  /*  int num_vals = 100;

    time_t seed;
    seed = time(NULL);
    std::cout<<"Seed is "<<seed<<std::endl;
    srand(seed);

    std::string query = "INSERT INTO test VALUES ";

    for(int i=0; i<num_vals; i++){
      uint64_t new_val = rand();
      std::stringstream ss;
      ss << new_val;

      query += "( "+ ss.str()+" ) ,";
    }

    query += " ( 1 )";

    assert_s(dbconnect->execute(query), "insertion failed!");*/

  assert_s(dbconnect->execute("SELECT "+ope_row+" FROM " + ope_table, result), "getting orig vals failed");

  ResType rt = result->unpack();

  std::vector<std::pair<uint64_t, int> > values_in_db;
  std::vector<std::pair<std::string, int> > strings_in_db;

  void* bc;

  if (ope_type == "INT"){

    bc = (blowfish *) new blowfish(passwd);
    values_in_db.resize(rt.rows.size());

  }else if (ope_type == "STRING"){

/*    bc = (AES *) new AES("1234567890123456");*/
    strings_in_db.resize(rt.rows.size());

  }

  AES_KEY * aes_key = get_AES_enc_key(passwd);

  for ( int rc=0; rc < (int) rt.rows.size(); rc++){
    std::stringstream ss;
    ss << ItemToString(rt.rows[rc][0]);

    if (ope_type == "INT"){
      uint64_t cur_val = 0;
      ss >> cur_val;
      values_in_db[rc] = std::make_pair(cur_val, rc);
    }else if (ope_type == "STRING") {
      std::string cur_val = ss.str();
      std::transform( cur_val.begin(), cur_val.end(), cur_val.begin(), ::tolower);
      strings_in_db[rc] = std::make_pair(cur_val, rc);
    }

  }

  if (ope_type == "INT"){
    std::stable_sort(values_in_db.begin(), values_in_db.end(), sort_first_int());
  }else if (ope_type == "STRING"){
    std::stable_sort(strings_in_db.begin(), strings_in_db.end(), sort_first_string());
  }

  std::string message = "";

  //std::vector<std::string> enc_vals;
  
  for (int rc = 0; rc < (int) rt.rows.size(); rc++) {
    std::stringstream ss;

    if (ope_type == "INT"){
    
      uint64_t det;
      ( (blowfish *) bc)->block_encrypt( &values_in_db[rc].first , &det);
      ss << det << " " << values_in_db[rc].second;

    }else if (ope_type == "STRING"){

      std::string det =
       encrypt_AES_CBC(strings_in_db[rc].first, aes_key, "0");
/*      if(std::find(enc_vals.begin(), enc_vals.end(), det) != enc_vals.end()){
        std::cout << cur_val <<std::endl;
        std::cout << det << std::endl;
      }
      enc_vals.push_back(det);*/

      //std::cout << det.size() << " " << det << " " << strings_in_db[rc].second << std::endl;

      ss << det.size() << " " << det << " " << strings_in_db[rc].second;
      
    }
    //ss << values_in_db[rc].first << " " << values_in_db[rc].second;
    message += ss.str() + " ";
  }


  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], "daytime");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    tcp::socket socket(io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
    if (error)
      throw boost::system::system_error(error);

    //std::cout <<"Sending message "<<message<<std::endl;
    boost::system::error_code ignored_error;
    boost::asio::write(socket, boost::asio::buffer(message),
        boost::asio::transfer_all(), ignored_error);

  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}