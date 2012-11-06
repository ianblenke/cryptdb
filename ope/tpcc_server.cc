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

#include <algorithm>
#include <utility>
#include <edb/Connect.hh>
#include "btree.hh"
#include "opetable.hh"
#include "tpcc_util.hh"

#include <ctime>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <util/ope-util.hh>

using boost::asio::ip::tcp;

struct sort_second {
    bool operator()(const std::pair<int,int> &left, const std::pair<int,int> &right) {
        return left.second < right.second;
    }
};

void
update_entries_int(OPETable<uint64_t> & ope_table, Node* curr_node, uint64_t v, uint64_t nbits);

void
update_table_int(OPETable<uint64_t> & ope_table, Node* btree);

void
update_entries_str(OPETable<std::string> & ope_table, Node* curr_node, uint64_t v, uint64_t nbits);

void
update_table_str(OPETable<std::string> & ope_table, Node* btree);

void parse_int_message(std::stringstream & ss, std::vector<int>& indexed_data, std::vector<std::string> & unique_data, std::vector<std::string> & bulk_data);

void parse_string_message(std::stringstream & ss, std::vector<int>& indexed_data, std::vector<std::string> & unique_data, std::vector<std::string> & bulk_data);

void
update_entries_int(OPETable<uint64_t> & ope_table, Node* curr_node, uint64_t v, uint64_t nbits){

    assert_s(nbits < (uint64_t) 64 - num_bits, "ciphertext too long!");
    bool leaf = true;

    for (unsigned int i=0; i<curr_node->m_count; i++) {
        if (curr_node->m_vector[i].has_subtree()) {  
            leaf = false;   
            //std::cout<<"Recursing "   <<curr_node->m_vector[i]<< " on "<<i <<" v="<<v<<std::endl;
            //std::cout<<" new v: "<<((v << num_bits) | i)<< " new nbits"<<nbits+num_bits<<std::endl;
            update_entries_int(ope_table, curr_node->m_vector[i].mp_subtree, 
              (v << num_bits) | i, 
              nbits+num_bits);
        }
    }

    if(leaf){

      uint64_t ope_enc;
      for (unsigned int i=1; i<curr_node->m_count; i++) {
          ope_enc = compute_ope(v, nbits, i-1);

          //std::cout<<"ope_enc for "<<curr_node->m_vector[i] << " is "<< ope_enc<< " w/ "<<v<<" : "<<nbits<<std::endl;
          uint64_t orig_val;
          std::stringstream ss;
          ss << curr_node->m_vector[i].m_key;
          ss >> orig_val; 
          //std::cout<<"Inserting ("<<orig_val<<", "<<ope_enc<<") into ope_table"<<std::endl;
          assert_s(ope_table.insert( orig_val, ope_enc), "inserted table value already existing!");            

      }

    }

}

void
update_entries_str(OPETable<std::string> & ope_table, Node* curr_node, uint64_t v, uint64_t nbits){

    assert_s(nbits < (uint64_t) 64 - num_bits, "ciphertext too long!");
    bool leaf = true;

    for (unsigned int i=0; i<curr_node->m_count; i++) {
        if (curr_node->m_vector[i].has_subtree()) {  
            leaf = false;   
            //std::cout<<"Recursing "   <<curr_node->m_vector[i]<< " on "<<i <<" v="<<v<<std::endl;
            //std::cout<<" new v: "<<((v << num_bits) | i)<< " new nbits"<<nbits+num_bits<<std::endl;
            update_entries_str(ope_table, curr_node->m_vector[i].mp_subtree, 
              (v << num_bits) | i, 
              nbits+num_bits);
        }
    }

    if(leaf){

      uint64_t ope_enc;
      for (unsigned int i=1; i<curr_node->m_count; i++) {
          ope_enc = compute_ope(v, nbits, i-1);

          //std::cout<<"ope_enc for "<<curr_node->m_vector[i] << " is "<< ope_enc<< " w/ "<<v<<" : "<<nbits<<std::endl;
          std::string orig_val = curr_node->m_vector[i].m_key;
          //std::cout<<"Inserting ("<<orig_val<<", "<<ope_enc<<") into ope_table"<<std::endl;
          assert_s(ope_table.insert( orig_val, ope_enc), "inserted table value already existing!");            
      }

    }

}

void
update_table_int(OPETable<uint64_t> & ope_table, Node* btree){

  update_entries_int(ope_table, btree, 0, 0);

}

void
update_table_str(OPETable<std::string> & ope_table, Node* btree){

  update_entries_str(ope_table, btree, 0, 0);

}

void parse_int_message(std::stringstream & ss, std::vector<int>& indexed_data, 
  std::vector<std::string> & unique_data, std::vector<std::string> & bulk_data){

    std::string first, second;
    std::string last = "";

    while(true){
      ss >> first >> second;
      if(first=="EOF" || second=="EOF") break;

      std::stringstream stoi;      

      int index;
      stoi << second;
      stoi >> index;

      indexed_data.push_back( index );
      bulk_data.push_back( first );
      if (first == last) {
        continue;
      } else{
        last = first;
        unique_data.push_back(first);
      }
    }    
}

void parse_string_message(std::stringstream & ss, std::vector<int>& indexed_data, 
  std::vector<std::string> & unique_data, std::vector<std::string> & bulk_data){

    std::string len_str;
    std::string last = "";

    int counter = 0;
    while(true){
      counter++;
      ss >> len_str;
      if(len_str=="EOF") break;

      std::stringstream stoi;
      int len;
      stoi << len_str;
      stoi >> len;

      ss.get();

      char data[len];
      for(int i=0 ; i < len; i++){
              ss.get(data[i]);
      }

      std::string str_data (data, len);

      int index;
      ss >> index;

      //std::cout << len << " " << str_data << " " << index << std::endl;

      indexed_data.push_back( index );
      bulk_data.push_back( str_data );
      if ( str_data == last) {
        continue;
      } else{
/*        std::vector<std::string>::iterator it;        
        if( (it = std::find(unique_data.begin(), unique_data.end(), str_data )) != unique_data.end() ){
          std::cout <<"Strdata: "<< str_data<< " : " << *it << std::endl;
        }*/
        last = str_data;
        unique_data.push_back(str_data);

       }

    }

    assert_s(bulk_data.size() == unique_data.size(), "vector lengths off");


}

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

      ss.write(buf.data(), len);

    }
    ss << " EOF EOF";

    //std::cout << "End string: " << ss.str() << std::endl;

    std::vector<int> indexed_data;
    std::vector<std::string> unique_data;
    std::vector<std::string> bulk_data;

    if (ope_type == "INT")
      parse_int_message(ss, indexed_data, unique_data, bulk_data);
    else if (ope_type == "STRING")
      parse_string_message(ss, indexed_data, unique_data, bulk_data);


/*    for(int i=0; i < (int) unique_data.size(); i++){
      std::cout << unique_data[i] << std::endl;
    }*/

    RootTracker* root_tracker = new RootTracker();

    //std::cout<<"Unique: "<<std::endl;
/*    for(int i=0; i< (int) unique_data.size(); i++){
      std::cout<<unique_data[i].size()<<": " <<unique_data[i]<<std::endl;
    }*/

    Node* b_tree = build_tree_wrapper(unique_data, *root_tracker, 0, unique_data.size());

    std::vector< std::pair<uint64_t, int> > db_data;

    if (ope_type == "INT"){
      OPETable<uint64_t>* ope_lookup_table = new OPETable<uint64_t>();
      update_table_int(*ope_lookup_table, b_tree);
      for(int j = 0; j < (int) bulk_data.size(); j++){
        uint64_t key;
        std::stringstream map_key;
        map_key << bulk_data[j];
        map_key >> key;        
        db_data.push_back( std::make_pair(ope_lookup_table->get(key).ope, indexed_data[j]) );
      }

    }else if (ope_type == "STRING"){
      OPETable<std::string >* ope_lookup_table = new OPETable<std::string>();
      update_table_str(*ope_lookup_table, b_tree);    
      for(int j = 0; j < (int) bulk_data.size(); j++){
        std::string key = bulk_data[j];
        db_data.push_back( std::make_pair( ope_lookup_table->get(key).ope, indexed_data[j]) );
      }
 
    }
    
    std::sort(db_data.begin(), db_data.end(), sort_second());

    std::ofstream load_file;
    load_file.open("/var/lib/mysql/tpcc_ope/bulk_load.txt");

    for(int j = 0; j < (int) db_data.size(); j++ ){
      load_file << db_data[j].first << "\n";
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