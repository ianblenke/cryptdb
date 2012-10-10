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

void handle_udf(void* lp){
    cout<<"Called handle_udf"<<endl;
    int* csock = (int*) lp;

    blowfish* bc = new blowfish("frankli714");

    ope_client<uint64_t, blowfish>* my_client = new ope_client<uint64_t, blowfish>(bc);    

    char buffer[1024];

    memset(buffer, 0, 1024);

    //Receive message to process
    recv(*csock, buffer, 1024, 0);
    int func_d;
    istringstream iss(buffer);
    iss>>func_d;
    if(DEBUG_COMM) cout<<"Client sees func_d="<<func_d<<" and buffer "<<buffer<<endl;
    bool imode;
    if(func_d==14){
        imode=true;
    }else if(func_d==15){
        imode=false;
    }else{
        cout<<"ERROR didn't receive 14 or 15"<<endl;
        exit(-1);
    }
    if(func_d==14 || func_d==15){
        uint64_t det_val=0;
        iss>>det_val;
        if(DEBUG_COMM) cout<<"Client det_val "<<det_val<<endl;
        uint64_t tmp_ope = my_client->encrypt(det_val, imode);
        if(tmp_ope == (uint64_t)-1 ) {
	    cout<<"WTF error in encrypt"<<endl;
	}
        ostringstream o;
        o.str("");
        o.clear();
        if(tmp_ope==0) {
	    o<<"DONE";
	}
        else {
	    o<<"15 "<<tmp_ope;
	}
        string rtn_str = o.str();
        send(*csock, rtn_str.c_str(), rtn_str.size(),0);
        send(my_client->hsock, "0",1,0);
    }
    free(csock);
    delete my_client;
    delete bc;    
}


int main(){
    //Build mask based on N
    mask=make_mask();

    //Socket connection
    int hsock = create_and_bind(OPE_CLIENT_PORT);
    
    //Start listening
    int listen_rtn = listen(hsock, 10);
    if(listen_rtn<0){
	cerr<<"Error listening to socket"<<endl;
    }
    cerr<<"Listening \n";

    socklen_t addr_size=sizeof(sockaddr_in);
    int* csock;
    struct sockaddr_in sadr;
    int i=5;
    //Handle 1 client b/f quiting (can remove later)
    while(i>0){
	cerr<<"Listening..."<<endl;
	csock = (int*) malloc(sizeof(int));
	if((*csock = accept(hsock, (struct sockaddr*) &sadr, &addr_size))!=-1){
	    //Pass connection and messages received to handle_client
	    handle_udf((void*)csock);
	}
	else{
	    std::cout<<"Error accepting!"<<endl;
	}
	//i--;
    }
    cerr<<"Done with client, closing now\n";
    close(hsock);

}

