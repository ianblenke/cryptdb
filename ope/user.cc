#include <algorithm>
#include <edb/Connect.hh>
#include <crypto/blowfish.hh>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <util/ope-util.hh>

using namespace std; 

void test_order(int num_vals, int order);

int main(){

    test_order(10, 0);

    blowfish* bc = new blowfish(passwd);

    uint64_t pt=0;
    uint64_t ct=0;

    while (true){
	cout<<"What det enc do you want? ";
	cin>>pt;
	if(pt==(uint64_t) -1) {cout<<"Closing"<<endl; break;}
	bc->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
	cout<<pt<<" det enc to "<<ct<<endl;
	pt=0;
	ct=0;
    }

/*
  blowfish* bc = new blowfish(passwd);

  Connect * dbconnect;
  dbconnect = new Connect( "localhost", "frank", "passwd","cryptdb", 3306);

  uint64_t pt=0;
  uint64_t ct=0;
  stringstream ss;
	
  while(1){
  cout<<"Insertion value: "<<endl;
  cin>>pt;
  if(pt==(uint64_t) -1) { cout<<"Closing"<<endl; break;}
  ss.str("");
  ss.clear();		
  bc->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
  cout<<pt<<" det enc to "<<ct<<endl;	
  ss<<"INSERT INTO emp VALUES ("<<pt<<", ope_enc("<<ct<<"), 0)";
  assert_s(dbconnect->execute(ss.str()), "user could not execute query");
  pt=0;
  ct=0;
  }

  return 0;
*/
}

void test_order(int num_vals, int order){

    blowfish* bc = new blowfish(passwd);

    Connect * dbconnect;
    dbconnect = new Connect( "localhost", "root", "letmein","cryptdb", 3306);
    vector<uint64_t> inserted_vals;

    int count = 0;
    time_t seed;
    seed = time(NULL);
    cout<<"Seed is "<<seed<<endl;
    srand(seed);

    for(int i=0; i<num_vals; i++){
	inserted_vals.push_back((uint64_t) rand());
    }
    if(order>0){
	sort(inserted_vals.begin(), inserted_vals.end());
	cerr << "increasing order \n";
    }
    if(order>1){
	reverse(inserted_vals.begin(), inserted_vals.end());
	cerr << "decreasing order \n";
    }	

    //vector<uint64_t> tmp_vals[num_vals];

    uint64_t pt=0;
    uint64_t ct=0;
    stringstream ss;

    for(int i=0; i<num_vals; i++){
    	pt = inserted_vals[i];
	//     tmp_vals[i] = pt;
	ss.str("");
        ss.clear();
	bc->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
	ss<<"INSERT INTO emp VALUES ("<<pt<<", ope_enc("<<ct<<",'i'), 0)";
	assert_s(dbconnect->execute(ss.str()), "user could not execute query " + ss.str());
	count++;
	pt=0;
	ct=0;

/*	int do_repeat = rand()%10+1;
	if(do_repeat<3){
	    int repeat_index = rand()%(tmp_vals.size());
	    pt = tmp_vals[repeat_index];
	    ss.str("");
	    ss.clear();
	    bc->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
	    ss<<"INSERT INTO emp VALUES ("<<pt<<", ope_enc("<<ct<<",'i'), 0)";
	    assert_s(dbconnect->execute(ss.str()), "user could not execute query");
	    count++;
	    pt=0;
	    ct=0;} }
    cout<<"Values:"<<endl;
    //sort(inserted_vals.begin(), inserted_vals.end());
    for(int i=0; i<num_vals; i++){
    	cout<<inserted_vals[i]<<", ";
    }
    cout<<endl;
    cout<<"Count="<<count<<endl;
}

