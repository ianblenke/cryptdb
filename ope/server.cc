#include "server.hh"

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::vector;
using std::cerr;
using std::map;



TreeNode *
Server::interaction(string ciph,
		    uint & rindex, uint & nbits,
		    uint64_t & ope_path,
		    bool & equals) {

    cerr << "\n\n Start Interaction\n";
    TreeNode * root = ope_tree->get_root();
    assert_s(root != NULL, "root is null");
    TreeNode * curr = root;

    ope_path = 0;
    nbits = 0;

    while (true) {
	cerr << "** Interaction STEP \n";
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

	cerr << "sending " << msg.str() << "\n";

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
	    cerr << "no subtree at index " << index << "\n";
	    rindex = index;
	    return curr;
	}
	
	curr = subtree;
    	ope_path = (ope_path << num_bits) | index;
    	nbits += num_bits;
    }
    
}


void 
Server::handle_enc(int csock, istringstream & iss, bool do_ins) {
   
    string ciph = unmarshall_binary(iss);
    
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;


    stringstream response;
    response.clear();
    
    auto ts = ope_table->find(ciph);
    
    if (ts) { // found in OPE Table
    	if (DEBUG) {cerr << "found in ope table"; }

  	// if this is an insert, increase ref count
    	if (do_ins) {
    	    ts->refcount++;
    	}
	response << ts->ope << " ";

	if (MALICIOUS) {
	    ope_tree->merkle_proof(ts->n) >> response;
	}
   	
    } else { // not found in OPE Table
    	if (DEBUG) {cerr << "not in ope table \n"; }

    	uint index = 0;
    	uint64_t ope_path = 0;
    	bool equals = false;
    	uint nbits = 0;

    	TreeNode * tnode = interaction(ciph, index, nbits, ope_path, equals);

        assert_s(!equals, "equals should always be false");
    	
    	if (DEBUG) {cerr << "new ope_enc has "<< " path " << ope_path
    			 << " index " << index << "\n";}
    	if (do_ins) {

	    UpdateMerkleProof proof;
    	    // insert in OPE Tree
            // ope_insert also updates ope_table
    	    ope_tree->insert(ciph, tnode, ope_path, nbits, index, proof);

            table_entry te = ope_table->get(ciph); // ciph must be in ope_table
            response << opeToStr(te.ope);
	    if (MALICIOUS) {
		response <<  " ";
		proof >> response;
	    }
        } else{

            uint64_t ope_enc = compute_ope(ope_path, nbits, index);

            //Fr: Subtract 1 b/c if ope points to existing value in tree, want this ope value
            //to be less than // Ra: ??
            response << opeToStr(ope_enc-1);
	    if (MALICIOUS) {
		response << " ";
		ope_tree->merkle_proof(tnode) >> response;
	    }
        }
    }
    cerr << "replying to request \n";
    string res = response.str();
    uint reslen = res.size();
    // cerr << "sending\n" <<res << "\n";
    cerr << "sent " << reslen << " bytes \n";
    assert_s(send(csock, res.c_str(), reslen, 0) == (int)reslen,
	     "problem with send");
    
}

void
Server::dispatch(int csock, istringstream & iss) {
   
    MsgType msgtype;
    iss >> msgtype;

    if (DEBUG_COMM) {cerr << "message type is " << mtnames[(int) msgtype] << endl;}

    switch (msgtype) {
    case MsgType::ENC_INS: {
	return handle_enc(csock, iss, true);
    }
    case MsgType::QUERY: {
	return handle_enc(csock, iss, false);
    }
    default: {}
    }

    assert_s(false, "invalid message type in dispatch");
    
}

Server::Server() {
    if (WITH_DB) {  
	db = new Connect( "localhost", "root", "letmein","cryptdb", 3306);
    }
    
    ope_table = new OPETable<string>();
    ope_tree = (Tree *) new BTree(ope_table, db);

    sock_cl = create_and_connect(OPE_CLIENT_HOST, OPE_CLIENT_PORT);
    sock_req = create_and_bind(OPE_SERVER_PORT);
    
  
}

Server::~Server() {
    close(sock_cl);
    close(sock_req);
    delete ope_tree;
    delete ope_table;
    if (WITH_DB) {
	delete db;
    }
}

void Server::work() {
    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;
 
    assert_s(!WITH_DB, "for UDF the server should accept a few times");
    int listen_rtn = listen(sock_req, 10);
    if (listen_rtn < 0) {
	cerr<<"Error listening to socket"<<endl;
    }
    cerr<<"Listening \n";
  

    int csock = accept(sock_req, (struct sockaddr*) &sadr, &addr_size);
    assert_s(csock >= 0, "Server accept failed!");
    cerr << "Server received request \n";
    
       
    uint buflen = 10240;
    char buffer[buflen];

    while (true) {
    	cerr<<"Waiting for connection..."<<endl;

        memset(buffer, 0, buflen);
	
	uint len = recv(csock, buffer, buflen, 0);
	if (len  == 0) {
	    cerr << "client closed connection\n";
	    close(csock);
	    break;
	}
	
        if (DEBUG_COMM) {cerr << "received msg " << buffer << endl;}
        istringstream iss(buffer);
	
	//Pass connection and messages received to handle_client
	dispatch(csock, iss);
    }

    cerr << "Server exits!\n";

}


