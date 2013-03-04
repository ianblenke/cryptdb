#include "server.hh"
#include <util/msgsend.hh>

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::vector;
using std::cerr;
using std::map;
using std::list;
using std::fstream;



TreeNode *
Server::interaction(string ciph,
		    uint & rindex, uint & nbits,
		    uint64_t & ope_path,
		    bool & equals, Tree * ope_tree) {

    if (DEBUG_COMM) cerr << "\n\n Start Interaction\n";
    TreeNode * root = ope_tree->get_root();
    assert_s(root != NULL, "root is null");
    TreeNode * curr = root;

    ope_path = 0;
    nbits = 0;

    while (true) {
	if (DEBUG_COMM) cerr << "** Interaction STEP \n";
    	// create message and send it to client
    	stringstream msg;
    	msg.clear();
    	msg << MsgType::INTERACT_FOR_LOOKUP << " ";
    	marshall_binary(msg, ciph);
	msg << " ";

	vector<string> keys = curr->get_keys();
    	uint len = keys.size();
    	msg << len << " ";

    	for (uint i = 0; i < len; i++) {
    	    marshall_binary(msg, keys[i]);
	    msg << " ";
    	}

	if (DEBUG_COMM) cerr << "sending " << msg.str() << "\n";

    	string _reply = send_receive(sock_cl, msg.str());
    	istringstream reply(_reply);

    	uint index;
    	bool equals = false;
    	reply >> index >> equals;

    	assert_s(len >= index, "index returned by client is incorrect");

	if (equals) {
	    rindex = index;
	    return curr;
	}

	TreeNode * subtree = curr->get_subtree(index);
	if (!subtree) {
	    if (DEBUG_COMM) cerr << "no subtree at index " << index << "\n";
	    rindex = index;
	    return curr;
	}
	
	curr = subtree;
    	ope_path = (ope_path << num_bits) | index;
    	nbits += num_bits;
    }
    
}


void 
Server::mope_handle_enc(int csock, istringstream & iss, string table_name,
			   Tree * ope_tree, OPETable<std::string> * ope_table) {

    string ciph = unmarshall_binary(iss);
    
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;

    stringstream response;
    response.clear();
    
    auto ts = ope_table->find(ciph);
    
    if (ts) { // found in OPE Table
    	if (DEBUG) {cerr << "found in ope table\n"; }

  	// if this is an insert, increase ref count
	if (WITH_DB) {
	    stringstream ss;
	    ss << "INSERT INTO " << table_name << " VALUES (0, " << ts->ope << ");";
	    assert_s(db->execute(ss.str()), "could not insert into table value found in ope table");
    	}
	
	response << ts->ope;

	if (MALICIOUS) {
            if(DEBUG) cerr << " Adding malicious proof to ope_table response" << endl;
	    response <<" " << PF_QUERY << " ";
	    ope_tree->merkle_proof(ts->n) >> response;
	}
   	
    } else { // not found in OPE Table
    	if (DEBUG) {cerr << "not in ope table \n"; }

    	uint index = 0;
    	uint64_t ope_path = 0;
    	bool equals = false;
    	uint nbits = 0;

    	TreeNode * tnode = interaction(ciph, index, nbits, ope_path, equals, ope_tree);

        assert_s(!equals, "equals should always be false");
    	
    	if (DEBUG) {cerr << "new ope_enc has "<< " path " << ope_path
    			 << " index " << index << "\n";}
    	
	
	UpdateMerkleProof proof;
	// insert in OPE Tree
	// ope_insert also updates ope_table
	ope_tree->insert(ciph, tnode, ope_path, nbits, index, proof);
	
	table_entry te = ope_table->get(ciph); // ciph must be in ope_table
	response << opeToStr(te.ope);
	if (MALICIOUS) {
	    response <<  " " << PF_INS << " ";
	    proof >> response;
	    response << " ";
	}
        
    }

    if (DEBUG_COMM) cerr << "replying to request \n";
    string res = response.str();
    uint reslen = res.size();
    // cerr << "sending\n" <<res << "\n";
    if (DEBUG_COMM) cerr << "sent " << reslen << " bytes \n";
    assert_s(xmsg_send(csock, res) == (int)reslen,
	     "problem with send");
    
}




void 
Server::stope_handle_query(int csock, istringstream & iss, string table_name,
			   Tree * ope_tree, OPETable<std::string> * ope_table) {
    
    string ciph = unmarshall_binary(iss);
    
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;
    
    uint index = 0;
    uint64_t ope_path = 0;
    bool equals = false;
    uint nbits = 0;
    
    interaction(ciph, index, nbits, ope_path, equals, ope_tree);

    stringstream response;
    response.clear();
    
    uint64_t ope_enc = compute_ope(ope_path, nbits, index);
    response << opeToStr(ope_enc-1);
    
    if (DEBUG_COMM) cerr << "replying to request \n";
    string res = response.str();
    uint reslen = res.size();
    // cerr << "sending\n" <<res << "\n";
    if (DEBUG_COMM) cerr << "sent " << reslen << " bytes \n";
    assert_s(xmsg_send(csock, res) == (int)reslen,
	     "problem with send");
    
}


void 
Server::stope_handle_ins(int csock, istringstream & iss, string table_name,
			 Tree * ope_tree, OPETable<std::string> * ope_table) {
    
    string ciph = unmarshall_binary(iss);
    
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;

    string rowid;
    if (WITH_DB) {
	iss >> rowid;
    }

    stringstream response;
    response.clear();

    uint index = 0;
    uint64_t ope_path = 0;
    bool equals = false;
    uint nbits = 0;
    
    TreeNode * tnode = interaction(ciph, index, nbits, ope_path, equals, ope_tree);

    if (equals) {
	//increase refcount in OPE Table and insert in DB
	string treeciph = tnode->get_ciph(index);
	
	auto te = ope_table->get(treeciph);

	te.refcount++;

	if (WITH_DB) {
	    stringstream ss;
	    ss << "INSERT INTO " << table_name << " VALUES (0, " << te.ope << ", " << rowid << ");";
	    assert_s(db->execute(ss.str()), "could not insert into table value found in ope table");
	}

	uint64_t ope_enc = compute_ope(ope_path, nbits, index);
	response << opeToStr(ope_enc-1);

    } else {
	string treeciph = ciph;
	UpdateMerkleProof p;
	ope_tree->insert(ciph, tnode, ope_path, nbits, index, p, rowid);
	
	response << ope_table->get(treeciph).ope; // ciph must be in ope_table
    }

   
    if (DEBUG_COMM) cerr << "replying to request \n";
    string res = response.str();
    uint reslen = res.size();
    // cerr << "sending\n" <<res << "\n";
    if (DEBUG_COMM) cerr << "sent " << reslen << " bytes \n";
    assert_s(xmsg_send(csock, res) == (int)reslen,
	     "problem with send");
    
}

void 
Server::stope_handle_remove(int csock, istringstream & iss, string table_name,
			    Tree * ope_tree, OPETable<std::string> * ope_table) {

    string ciph = unmarshall_binary(iss);
    
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;

    string rowid;
    if (WITH_DB) {
	iss >> rowid;
    }

    stringstream response;
    response.clear();

    uint index = 0;
    uint64_t ope_path = 0;
    bool equals = false;
    uint nbits = 0;
    
    TreeNode * tnode = interaction(ciph, index, nbits, ope_path, equals, ope_tree);

    if (DEBUG) {cerr << "new ope_enc has "<< " path " << ope_path
			 << " index " << index << "\n";}
	
    if (!equals) {
	response << "0"; //elm is not int tree so not deleted
    } else {

	string treeciph = tnode->get_ciph(index);

		
	table_entry te = ope_table->get(treeciph); // ciph must be in ope_table
	response <<  opeToStr(te.ope);
	
	bool del_from_tree;

	if (te.refcount == 1) {
	    assert_s(ope_table->remove(treeciph), "could not remove from OPE Table");
	    del_from_tree = true;
	} else {
	    te.refcount--;
	    del_from_tree = false;
	}

	if (WITH_DB) {
	    stringstream query;
	    query << "DELETE FROM " << table_name <<
		" WHERE rowid " << rowid << " AND " << " ope = " << te.ope << ";";
	    assert_s(db->execute(query.str()), "cannot execute query");
	}
	
	if (del_from_tree) {
	    UpdateMerkleProof proof;
	    // insert in OPE Tree
	    // ope_insert also updates ope_table
	    ope_tree->remove(treeciph, tnode, ope_path, nbits, index, proof, rowid);
	}

    }


    if (DEBUG_COMM) cerr << "replying to request \n";
    string res = response.str();
    uint reslen = res.size();
    // cerr << "sending\n" <<res << "\n";
    if (DEBUG_COMM) cerr << "sent " << reslen << " bytes \n";
    assert_s(xmsg_send(csock, res) == (int)reslen,
	     "problem with send");

}


void
Server::dispatch(int csock, istringstream & iss) {
   
    MsgType msgtype;
    iss >> msgtype;

    string table_name;
    if (WITH_DB) {
	iss >> table_name;
    } else {
	table_name = auto_table;
    }
    
    assert_s(tables.find(table_name) != tables.end(), "invalid table name " + table_name);
    tablemeta * tm = tables[table_name];
    Tree * ope_tree = tm->ope_tree;
    OPETable<std::string> * ope_table = tm->ope_table;


    if (DEBUG_COMM) {cerr << "message type is " << mtnames[(int) msgtype] << endl;}

    switch (msgtype) {
    case MsgType::ENC: {
	assert_s(!STOPE, "must be in mOPE mode");
	return mope_handle_enc(csock, iss, table_name, ope_tree, ope_table);
    }
    case MsgType::INS: {
	assert_s(STOPE && !MALICIOUS, "must be in stOPE mode");
	return stope_handle_ins(csock, iss, table_name, ope_tree, ope_table);
    }
    case MsgType::QUERY: {
	assert_s(STOPE && !MALICIOUS, "must be in stOPE mode");
	return stope_handle_query(csock, iss, table_name, ope_tree, ope_table);
    }
    case MsgType::REMOVE: {
	assert_s(STOPE && !MALICIOUS, "must be in stOPE mode");
	return stope_handle_remove(csock, iss, table_name, ope_tree, ope_table);
    }
    default: {}
    }

    assert_s(false, "invalid message type in dispatch");
    
}
Tree *
Server::ope_tree() {
    assert_s(tables.size() == 1, "invalid no, of tables for get_ope_Tree");
    return tables.begin()->second->ope_tree;
}
Server::Server(bool malicious, bool stope, int cport, int sport, list<string> * itables, bool db_updates) {
    MALICIOUS = malicious;
    STOPE = stope;

    if (WITH_DB) {  
	db = new Connect( "localhost", "root", "letmein","cryptdb", 3306);
	assert_s(itables, "lacking tables info");
    } else {
	db = NULL;
	itables = new list<string>({auto_table});
    }
    
    for (auto table : *itables) {
	OPETable<string> *ope_table = new OPETable<string>();
	Tree * ope_tree = (Tree*) new BTree(ope_table, db, malicious, table, "opefield", db_updates);
	tablemeta * tm = new tablemeta(ope_tree, ope_table);
	tables[table] = tm;
    }
    if(!WITH_DB){
        delete itables;
    }
    sock_cl = create_and_connect(OPE_CLIENT_HOST, cport);
    sock_req = create_and_bind(sport);
    
}

Server::~Server() {
    close(sock_cl);
    close(sock_req);
    for (auto tm : tables) {
	delete tm.second;
    }
    tables.clear();
    if (WITH_DB) {
	delete db;
    }
}

uint64_t
Server::store_tree(){
    fstream f;
    f.open("tree.dump", std::ios::out | std::ios::binary | std::ios::trunc);
    ( (BTree*) ope_tree())->store_tree(f);
    f.close();
    struct stat filestatus;
    stat( "tree.dump", &filestatus );
    return filestatus.st_size;
}

uint 
Server::num_rewrites(){
    assert_s(!WITH_DB && tables.size() == 1, " num_rewrites only for non DB case");
    
    return tables.begin()->second->ope_tree->num_rewrites();
}

void Server::work() {
    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;
 
    int listen_rtn = listen(sock_req, 10);
    if (listen_rtn < 0) {
	cerr<<"Error listening to socket"<<endl;
    }
    if (DEBUG_BARE) cerr<<"Listening \n";
  

    int csock = accept(sock_req, (struct sockaddr*) &sadr, &addr_size);
    assert_s(csock >= 0, "Server accept failed!");
    if (DEBUG_COMM) cerr << "Server received connection \n";
    
       
    while (true) {
    	if (DEBUG_COMM) cerr<<"Waiting for request..."<<endl;

	std::string buffer;
	uint len = xmsg_recv(csock, &buffer);
	if (len  == 0) {
	    if (DEBUG_COMM) cerr << "client closed connection\n";
	    close(csock);
	    break;
	}
	
        if (DEBUG_COMM) {cerr << "received msg " << buffer << endl;}
        istringstream iss(buffer);
	
	//Pass connection and messages received to handle_client
	dispatch(csock, iss);
    }

    if (DEBUG_COMM) cerr << "Server exits!\n";

}


