//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "btree.hh"

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <ctime>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <util/ope-util.hh>

using boost::asio::ip::tcp;

void
update_path(std::vector<uint64_t> & ope_vec, Node* curr_node, uint64_t v, uint64_t nbits);

void
update_db(std::vector<uint64_t> & ope_vec, Node* btree);

void
update_path(std::vector<uint64_t> & ope_vec, Node* curr_node, uint64_t v, uint64_t nbits){
    
    bool leaf = true;

    for (unsigned int i=0; i<curr_node->m_count; i++) {
        if (curr_node->m_vector[i].has_subtree()) {  
            leaf = false;   
            std::cout<<"Recursing "   <<curr_node->m_vector[i]<< " on "<<i <<" v="<<v<<std::endl;
            std::cout<<" new v: "<<((v << num_bits) | i)<< " new nbits"<<nbits+num_bits<<std::endl;
            update_path(ope_vec, curr_node->m_vector[i].mp_subtree, 
              (v << num_bits) | i, 
              nbits+num_bits);
        }
    }

    if(leaf){

      uint64_t ope_enc;
      for (unsigned int i=1; i<curr_node->m_count; i++) {
          ope_enc = compute_ope<uint64_t>(v, nbits, i-1);

          std::cout<<"ope_enc for "<<curr_node->m_vector[i] << " is "<< ope_enc<< " w/ "<<v<<" : "<<nbits<<std::endl;

          ope_vec.push_back(ope_enc);
      }

    }

}

void
update_db(std::vector<uint64_t> & ope_vec, Node* btree){

  update_path(ope_vec, btree, 0, 0);

}


int main()
{
  try
  {
    boost::asio::io_service io_service;

    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 13));


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
        std::cout<<"EOF signalled"<<std::endl;
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
      bulk_data.push_back(tmp);
    }

    for(int i=0; i < (int) bulk_data.size(); i++){
      std::cout << bulk_data[i] << std::endl;
    }
    std::cout<<"Printing b_min_keys just b/c: "<<b_min_keys<<std::endl;

    RootTracker* root_tracker = new RootTracker();

    std::cout<<b_min_keys<<std::endl;

    Node* b_tree = build_tree_wrapper(bulk_data, *root_tracker, 0, bulk_data.size());

    std::vector<uint64_t> ope_vec;

    update_db(ope_vec, b_tree);

    for(int j = 0; j < (int) ope_vec.size(); j++){
      std::cout<< ope_vec[j]<<" "<<std::endl;
    }


  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}