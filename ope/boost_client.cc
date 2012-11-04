//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/asio.hpp>
#include <sstream>
#include <util/ope-util.hh>
#include <edb/Connect.hh>
#include <crypto/ope.hh>
#include "tpcc_util.hh"
#include <NTL/ZZ.h>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{

  struct timeval start_time;

  gettimeofday(&start_time, 0);
  std::cout<<start_time.tv_sec<<std::endl;
  std::cout<<start_time.tv_usec<<std::endl;

  Connect * dbconnect;
  dbconnect = new Connect( "localhost", "root", "letmein","tpcc", 3306);

  DBResult * result;

  assert_s(dbconnect->execute("SELECT "+ope_row+" FROM " + ope_table, result), "getting orig vals failed");

  ResType rt = result->unpack();


  OPE* ope;
  if (ope_type == "INT")
    ope = new OPE(passwd, 32, 64);
  else if (ope_type == "STRING")
    ope = new OPE(passwd, 128, 256);

  std::string message = "";

  for ( int rc=0; rc < (int) rt.rows.size(); rc++){
    std::stringstream ss;
    ss << ItemToString(rt.rows[rc][0]);

    uint64_t ope_enc;
    if (ope_type == "INT"){
      int cur_val = 0;
      ss >> cur_val;
      NTL::ZZ enc = ope->encrypt(NTL::to_ZZ(cur_val));
      ope_enc = uint64FromZZ(enc);

      assert_s(ope->decrypt(enc) == NTL::to_ZZ(cur_val), "encryption for int not right");

    }else if (ope_type == "STRING") {
      std::string cur_val = ss.str();
      std::transform( cur_val.begin(), cur_val.end(), cur_val.begin(), ::tolower);
      int plain_size = 16;
      if ((int) cur_val.size() < plain_size)
          cur_val = cur_val + std::string(plain_size - cur_val.size(), 0);

      NTL::ZZ pv;

      for (uint i = 0; i < (uint) plain_size; i++) {
        pv = pv * 256 + (int)cur_val[i];
      }
      
      NTL::ZZ enc = ope->encrypt(pv);

      assert_s(ope->decrypt(enc) == pv, "encryption for int not right");

      ope_enc = uint64FromZZ(enc);
    }

    ss.clear();
    ss.str("");

    ss << ope_enc;

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