#include "client.hh"


static inline int
ffsl(uint64_t ct)
{
    int bit;

    if (ct == 0)
        return (0);
    for (bit = 1; !(ct & 1); bit++)
        ct = (uint64_t)ct >> 1;
    return (bit+num_bits-1);
}

static
void
handle_server(int csock, blowfish* bc){
    cout<<"Called handle_udf"<<endl;

    //ope_client<uint64_t, blowfish>* my_client = new ope_client<uint64_t, blowfish>(bc);    

    char buffer[1024];

    memset(buffer, 0, 1024);

    //Receive message to process
    if(recv(csock, buffer, 1024, 0) <=0){
        assert_s(false, "handle_server receive failed");
    }
    MsgType func_d;
    istringstream iss(buffer);
    iss>>func_d;
    if(DEBUG_COMM) cout<<"Client sees func_d="<<func_d<<" and buffer "<<buffer<<endl;
  
    assert_s(func_d == MsgType::INTERACT_FOR_LOOKUP, "Incorrect function type in handle_server!");
    if (func_d == MsgType::INTERACT_FOR_LOOKUP){
        
        uint64_t det, pt, tmp_pt;
        int size, index;

        iss >> det >> size;

        bc->block_decrypt((const uint8_t *) &det, (uint8_t *) &pt);

        for(index=0; index<size; index++){
	    uint64_t tmp_key;
	    iss >> tmp_key;
            bc->block_decrypt((const uint8_t *) &tmp_key, (uint8_t *) &tmp_pt);

            if (tmp_pt >= pt){
                break;
            }

        }

        stringstream o;
        o << index << " " << (tmp_pt==pt);
        string rtn_str = o.str();
        if (send(csock, rtn_str.c_str(), rtn_str.size(),0) != (int)rtn_str.size()){
            assert_s(false, "handle_server send failed");
        }
        //send(my_client->hsock, "0",1,0);

    }
    

    //delete my_client;
    //delete bc;    
}


int main(){

    //Socket connection
    int sock = create_and_bind(OPE_CLIENT_PORT);
    
    //Start listening
    int listen_rtn = listen(sock, 10);
    if (listen_rtn < 0) {
	cerr << "Error listening to socket" << endl;
    }
    cerr << "Listening \n";

    socklen_t addr_size = sizeof(sockaddr_in);
    int* csock;
    struct sockaddr_in sadr;

    //Handle 1 client b/f quiting (can remove later)

    blowfish* bc = new blowfish(passwd);

    while (true) {
    	cerr<<"Listening..."<<endl;
    	csock = (int*) malloc(sizeof(int));
    	if((*csock = accept(hsock, (struct sockaddr*) &sadr, &addr_size))!=-1){
    	    //Pass connection and messages received to handle_client
    	    handle_server(*csock, bc);
    	}
    	else{
    	    std::cout<<"Error accepting!"<<endl;
    	}
        free(csock);
    }
    cerr<<"Done with client, closing now\n";
    close(hsock);
    delete bc;

}

