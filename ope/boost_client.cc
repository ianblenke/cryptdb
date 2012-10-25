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

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{

  Connect * dbconnect;
  dbconnect = new Connect( "localhost", "root", "letmein","cryptdb", 3306);

  DBResult * result;

  dbconnect->execute("SELECT ope_enc FROM emp ORDER BY ope_enc", result);

  ResType rt = result->unpack();

  std::vector<uint64_t> values_in_db;
  values_in_db.resize(rt.rows.size());

  std::string message = "";

  for ( int rc=0; rc < (int) rt.rows.size(); rc++){
    uint64_t cur_val = 0;
    std::stringstream ss;
    ss << ItemToString(rt.rows[rc][0]);
    ss >> cur_val;
    
    message+= ItemToString(rt.rows[rc][0]) + " ";
    values_in_db[rc] = cur_val;

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

    std::cout <<"Sending message "<<message<<std::endl;
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