//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include <ope/btree.hh>

#include <ctime>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <util/ope-util.hh>
#include "tpcc_util.hh"


using boost::asio::ip::tcp;

int main()
{
  try
  {
    boost::asio::io_service io_service;

    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 13));


    tcp::socket socket(io_service);
    acceptor.accept(socket);

    std::string output = "";
    std::stringstream ss (output, std::stringstream::in | std::stringstream::out);
    for(;;)
    {

      boost::array<char, 128> buf;
      boost::system::error_code error;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (error == boost::asio::error::eof){
        //std::cout<<"EOF signalled"<<std::endl;
        break; // Connection closed cleanly by peer.
      }else if (error)
        throw boost::system::system_error(error); // Some other error.


      //std::cout.write(buf.data(), len);
      ss.write(buf.data(), len);

    }

    ss << " EOF";

    //std::cout << "End string: " << ss.str() << std::endl;

    std::string tmp;

    std::ofstream load_file;
    load_file.open("/var/lib/mysql/tpcc_ope/bulk_load.txt");

    while(true){
      ss >> tmp;
      if(tmp=="EOF") break;
      load_file << tmp << "\n";
    }

    load_file.close();

    Connect * dbconnect;
    dbconnect = new Connect( "localhost", "root", "letmein","tpcc_ope", 3306);

    dbconnect->execute("DROP TABLE "+ope_table);

    assert_s(dbconnect->execute("CREATE TABLE "+ope_table+" ("+ope_row+" BIGINT(64) UNSIGNED NOT NULL)"), "table creation failed");

    assert_s(dbconnect->execute("LOAD DATA INFILE 'bulk_load.txt' INTO TABLE "+ope_table+ " LINES TERMINATED BY '\n' ("+ope_row+ ")"), "updating ope_table failed");    

    struct timeval end_time;

    gettimeofday(&end_time, 0);
    std::cout<<end_time.tv_sec<<std::endl;
    std::cout<<end_time.tv_usec<<std::endl;

  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
