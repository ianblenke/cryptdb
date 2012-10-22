//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <sstream>
#include <vector>
#include <boost/array.hpp>
#include "btree.hh"

using boost::asio::ip::tcp;



int main()
{
  try
  {
    boost::asio::io_service io_service;

    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 13));

    for (;;)
    {
      tcp::socket socket(io_service);
      acceptor.accept(socket);
      

      std::vector<std::string> bulk_data;

      std::string output = "";
      std::stringstream ss (output, std::stringstream::in | std::stringstream::out);
      for(;;)
      {

        boost::array<char, 128> buf;
        boost::system::error_code error;

        size_t len = socket.read_some(boost::asio::buffer(buf), error);

        if (error == boost::asio::error::eof){
          std::cout<<"EOF error"<<std::endl;
          break; // Connection closed cleanly by peer.
        }else if (error)
          throw boost::system::system_error(error); // Some other error.


        //std::cout.write(buf.data(), len);
        ss.write(buf.data(), len);

      }

      ss << " EOF";

      std::cout << "End string: " << ss.str() << std::endl;

      std::string tmp;

      while(true){
        ss >> tmp;
        if(tmp=="EOF") break;
/*        std::stringstream iss;
        iss << tmp;
        uint64_t tmp_val;
        iss >> tmp_val;*/
        bulk_data.push_back(tmp);
      }

      for(int i=0; i < (int) bulk_data.size(); i++){
        std::cout << bulk_data[i] << std::endl;
      }
      std::cout<<"Printing b_min_keys just b/c: "<<b_min_keys<<std::endl;

      RootTracker* root_tracker = new RootTracker();

      build_tree_wrapper(bulk_data, *root_tracker, 0, bulk_data.size()-1);

    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}