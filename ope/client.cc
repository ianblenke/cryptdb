#include "client.hh"

#include <signal.h>

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

    MsgType func_d;
    iss>>func_d;
  
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
        
    }
    
    return "";
  
}

static int sock = 0;
static int csock = 0;
 
static void
cleanup(int signum) {

    close(csock);
    close(sock);
    cerr << "cleaned-up sockets \n";
}

int main(int argc, char ** argv){


    assert_s(argc <= 2, "usage: client [port] ");
    if (argc == 2) {
	OPE_CLIENT_PORT = atoi(argv[1]);
    }
    cerr << "OPE CLIENT PORT " << OPE_CLIENT_PORT << "\n";
	
    //Socket connection
    sock = create_and_bind(OPE_CLIENT_PORT);

    signal(SIGQUIT, cleanup);
        
    //Start listening
    int listen_rtn = listen(sock, 10);
    if (listen_rtn < 0) {
	cerr << "Error listening to socket" << endl;
    }
    cerr << "Listening \n";

    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;

    csock = accept(sock, (struct sockaddr*) &sadr, &addr_size);    
    assert_s(csock >= 0, "Client failed to accept connection");
    cerr << "accepted connection (from server hopefully)\n";
   
    int buflen = 1024;

    char buffer[buflen];

    string interaction_rslt="";

    blowfish * bc = new blowfish(passwd);
    
    while (true) {

        memset(buffer, 0, buflen);

	cerr << "waiting to receive \n";
        //Receive message to process
        assert_s(recv(csock, buffer, buflen, 0) > 0, "receive gets  <=0 bytes");

	cerr << "received " << buffer << "\n";
        istringstream iss(buffer);

        interaction_rslt = handle_interaction(iss, bc);

        assert_s(interaction_rslt!="", "interaction error");

	cerr << "sending " << interaction_rslt << "\n";
        assert_s(send(csock, interaction_rslt.c_str(), interaction_rslt.size(),0)
		 == (int)interaction_rslt.size(),
		 "send failed");

        interaction_rslt = "";

    }

    close(csock);    
    cerr<<"Done with client, closing now\n";
    close(sock);
    delete bc;

}

