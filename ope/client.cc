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
string
handle_interaction(istringstream & iss, blowfish* bc){
    cerr<<"Called handle_interaction"<<endl;

    //ope_client<uint64_t, blowfish>* my_client = new ope_client<uint64_t, blowfish>(bc);    

    
    MsgType func_d;
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
        return o.str();
        
        //send(my_client->hsock, "0",1,0);

    }
    
    return "";
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
    struct sockaddr_in sadr;

    //Handle 1 client b/f quiting (can remove later)

    blowfish * bc = new blowfish(passwd);
    int csock = accept(sock, (struct sockaddr*) &sadr, &addr_size)

    if(csock < 0){
        assert_s(false, "Client failed to accept correctly");
    }

    int buflen = 1024;

    char buffer[buflen];

    string interaction_rslt="";


    while (true) {

        memset(buffer, 0, buflen);

        //Receive message to process
        if(recv(csock, buffer, buflen, 0) <=0){
            assert_s(false, "handle_server receive failed");
        }

        istringstream iss(buffer);

        interaction_rslt = handle_interaction(iss, bc);

        assert_s(interaction_rslt!="", "interaction error");


        if (send(csock, interaction_rslt.c_str(), interaction_rslt.size(),0) != (int)interaction_rslt.size()){
            assert_s(false, "handle_server send failed");
        }

        interaction_rslt = "";

    }

    close(csock);    
    cerr<<"Done with client, closing now\n";
    close(sock);
    delete bc;

}

