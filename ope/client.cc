#include "client.hh"


//Make mask of num_bits 1's
uint64_t make_mask(){
    uint64_t cur_mask = 1ULL;
    for(int i=1; i<num_bits; i++){
        cur_mask = cur_mask | (1ULL<<i);
    }
    return cur_mask;
}

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

template<class V, class BlockCipher>
V
ope_client<V, BlockCipher>::decrypt(uint64_t ct) const {
    char buffer[10240];

    uint64_t nbits = 64 - ffsl((uint64_t)ct);
    uint64_t v= ct>>(64-nbits); //Orig v
    uint64_t path = v>>num_bits; //Path part of v
    int index = (int) v & mask; //Index (last num_bits) of v
    if(DEBUG) cout<<"Decrypt lookup with path="<<path<<" nbits="<<nbits-num_bits<<" index="<<index<<endl;
    //vector<V> vec = s->lookup(path, nbits- num_bits);

    ostringstream o;
    o.str("");
    o.clear();
    o<<"2 "<<path<<" "<<(nbits-num_bits);
    string msg = o.str();
    if(DEBUG_COMM) cout<<"Sending decrypt msg: "<<msg<<endl;
    send(hsock, msg.c_str(), msg.size(),0);
    recv(hsock, buffer, 10240, 0);

    if(DEBUG_BTREE) cout<<"Decrypt recv="<<buffer<<endl;
    istringstream iss_tmp(buffer);
    vector<V> xct_vec;
    V xct;
    string checker="";
    while(true){
        iss_tmp >> checker;
        if(checker==";") break;
        stringstream tmpss;
        tmpss<<checker; 
        tmpss>>xct;
        if(DEBUG_BTREE) cout<<"decrypt xct: "<<tmpss.str()<<endl;       
        

        xct_vec.push_back(xct);
    }        
    return block_decrypt(xct_vec[index]);

}

template<class V, class BlockCipher>
void
ope_client<V, BlockCipher>::delete_value(V pt){
    uint64_t ct = encrypt(pt);


    uint64_t nbits = 64 - ffsl((uint64_t)ct);
    uint64_t v= ct>>(64-nbits); //Orig v
    uint64_t path = v>>num_bits; //Path part of v
    int index = (int) v & mask; //Index (last num_bits) of v

    ostringstream o;
    o.str("");
    o.clear();
    o<<"4 "<<path<<" "<<nbits-num_bits<<" "<<index;
    string msg = o.str();
    if(DEBUG) cout<<"Deleting pt="<<pt<<" with msg "<<msg<<endl;

    char buffer[10240];
    memset(buffer, '\0', 10240);

    send(hsock, msg.c_str(), msg.size(), 0);
    recv(hsock, buffer, 10240, 0);

#if MALICIOUS
    if(DEBUG_BTREE){
        cout<<"Delete_value buffer recv: "<<buffer<<endl;
    }
    const string tmp_merkle_hash = cur_merkle_hash;
    string new_merkle_hash;
    DelMerkleProof dmp;
    stringstream iss(buffer);
    iss>>new_merkle_hash;
    string tmp_hash_str (buffer, 10240);
    size_t hash_end = tmp_hash_str.find("hash_end");
    string new_hash_str (buffer, hash_end);
    cur_merkle_hash = new_hash_str;
    iss>>dmp;
      char tmp_hash[10240];
    strcpy(tmp_hash, buffer);
    size_t end = 
    *space='\0';
    string tmp_hash_str (tmp_hash);
    cur_merkle_hash=tmp_hash_str;  
    //cur_merkle_hash=new_merkle_hash;   

    if(DEBUG_BTREE){
        cout<<"Delete_value new mh="<<cur_merkle_hash<<endl;
        cout<<"DMP="<<dmp<<endl;
    }
    if(!verify_del_merkle_proof(dmp, "to_delete", tmp_merkle_hash, new_merkle_hash)){
        cout<<"Deletion merkle proof failed!"<<endl;
        exit(-1);
    }
    if(DEBUG_BTREE) cout<<"Deletion merkle proof succeeded"<<endl;

#endif
}


template<class V, class BlockCipher>
void
ope_client<V, BlockCipher>::insert(uint64_t v, uint64_t nbits, uint64_t index, V pt, V det) const{
    if(DEBUG) cout<<"pt: "<<pt<<" det: "<<det<<"  not in tree. "<<nbits<<": "<<" v: "<<v<<endl;
    
    //s->insert(v, nbits, index, pt);
    ostringstream o;
    o.str("");
    o.clear();
    o<<"3 "<<v<<" "<<nbits<<" "<<index<<" "<<det;
    string msg = o.str();
    if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;

    char buffer[10240];
    memset(buffer, '\0', 10240);
    send(hsock, msg.c_str(), msg.size(),0);
    recv(hsock, buffer, 10240, 0);

#if MALICIOUS
    if(DEBUG_BTREE) cout<<"Insert buffer="<<buffer<<endl;
    const string tmp_merkle_hash = cur_merkle_hash;
    string new_merkle_hash;
    InsMerkleProof imp;
    stringstream iss(buffer);
    iss>>new_merkle_hash;
    string tmp_hash_str (buffer, 10240);
    size_t hash_end = tmp_hash_str.find("hash_end");
    string new_hash_str (buffer, hash_end-1);
    cur_merkle_hash = new_hash_str;
    iss>>imp;
      char tmp_hash[10240];
    strcpy(tmp_hash, buffer);
    char* space = strchr(tmp_hash,' ');
    *space='\0';
    string tmp_hash_str (tmp_hash);
    cur_merkle_hash=tmp_hash_str;
    
    if(DEBUG_BTREE){
        cout<<"Insert New mh="<<cur_merkle_hash<<endl;
        cout<<"Insert proof="<<imp<<endl;
    }
    if(!verify_ins_merkle_proof(imp, tmp_merkle_hash, new_merkle_hash)){
        cout<<"Insertion merkle proof failed!"<<endl;
        exit(-1);
    }
    if(DEBUG_BTREE) cout<<"Insert mp succeeded"<<endl;
    //relabeling may have been triggered so we need to lookup value again
    //todo: optimize by avoiding extra tree lookup
    //return encrypt(pt);
#endif
}

    /* Encryption is the path bits (aka v) bitwise left shifted by num_bits
     * so that the last num_bits can indicate the index of the value pt at
     * at the node found by the path
     */

    /*Server protocol code:
     * lookup(encrypted_plaintext) = 1
     * lookup(v, nbits) = 2
     * insert(v, nbits, index, encrypted_laintext) = 3
     * delete(v, nbits, index) = 4
    */

template<class V, class BlockCipher>
void
ope_client<V, BlockCipher>::encrypt(V det) const{

    uint64_t v = 0;
    uint64_t nbits = 0;

    V pt = block_decrypt(det);

    if(DEBUG_COMM) cout<<"Encrypting pt: "<<pt<<" det: "<<det<<endl;

    char buffer[10240];
    memset(buffer, '\0', 10240);

    //table_storage test = s->lookup(pt);
    ostringstream o;
    string msg;

    for (;;) {
        if(DEBUG) cout<<"Do lookup for "<<pt<<" with det: "<<det<<" v: "<<v<<" nbits: "<<nbits<<endl;
        //vector<V> xct = s->lookup(v, nbits);
        o.str("");
        o.clear();
        o<<"2 "<<v<<" "<<nbits;
        msg = o.str();
        if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;
        send(hsock, msg.c_str(), msg.size(), 0);
        memset(buffer, 0, 10240);
        recv(hsock, buffer, 10240, 0);
        if(DEBUG_COMM || DEBUG_BTREE) cout<<"Received during iterative lookup: "<<buffer<<endl;
        string check(buffer);           

        if(check=="ope_fail"){
            insert(v, nbits, 0, pt, det);
            return;

        }               

        stringstream iss_tmp(buffer);
        vector<V> xct_vec;
        V xct;
        string checker="";
        while(true){
            iss_tmp >> checker;
            if(checker==";") break;
            if(DEBUG_BTREE) cout<<"xct_vec: "<<checker<<endl;
            stringstream tmpss;
            tmpss<<checker;
            tmpss>>xct;
            

            //predIndex later assumes vector is of original plaintext vals
            xct_vec.push_back(block_decrypt(xct));
        }
#if MALICIOUS
        MerkleProof mp;
        if(DEBUG_BTREE) cout<<"Remaining mp info = "<<iss_tmp.str()<<endl;
        for(int i=0; i<(int) xct_vec.size(); i++){
            iss_tmp >> mp;
            if(DEBUG_BTREE) cout<<"MP "<<i<<"="<<mp<<endl;
            const string tmp_merkle_hash = cur_merkle_hash;
            if(!verify_merkle_proof(mp, tmp_merkle_hash)){
                cout<<"Merkle hash during lookup didn't match!"<<endl;
                exit(-1);
            }
            if(DEBUG_BTREE) cout<<"MP "<<i<<" succeeded"<<endl;
        }
#endif

        //Throws ope_lookup_failure if xct size < N, meaning can insert at node
        int pi;
        try{
            pi = predIndex(xct_vec, pt);
        }catch(ope_lookup_failure&){
            int index;
            for(index=0; index<(int) xct_vec.size(); index++){
                if(pt<xct_vec[index]) break;
            }
            insert(v, nbits, index, pt, det);
            return;
        }
        nbits+=num_bits;
        if (pi==-1) {
            //pt already exists at node
            int index;
            for(index=0; index< (int) xct_vec.size(); index++){
                //Last num_bits are set to represent index of pt at node
                if(xct_vec[index]==pt) v = (v<<num_bits) | index;
            }
            break;
        }else {
            v = (v<<num_bits) | pi;
        }
        //Check that we don't run out of bits!
        if(nbits > 63) {
            cout<<"nbits larger than 64 bits!!!!"<<endl;
        }           
    }
    /*If value was inserted previously, then should get result
     *from server ope_table. If not, then it will insert. So never
     *should reach this point (which means no ope_table result or insertion)
     */
    cout<<"SHOULD NEVER REACH HERE!"<<endl;
    exit(1);
    return;
    //FL todo: s->update_table(block_encrypt(pt),v,nbits);
/*      if(DEBUG) cout<<"Encryption of "<<pt<<" has v="<< v<<" nbits="<<nbits<<endl;
    return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));*/
}

template<class V, class BlockCipher>
V
ope_client<V, BlockCipher>::block_decrypt(V ct) const {
    V pt;
    b->block_decrypt((const uint8_t *) &ct, (uint8_t *) &pt);
    return pt;
}

template<class V, class BlockCipher>
V
ope_client<V, BlockCipher>::block_encrypt(V pt) const {
    V ct;
    b->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
    return ct;
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
    if(func_d==14){
        uint64_t det_val=0;
        iss>>det_val;
        if(DEBUG_COMM) cout<<"Client det_val "<<det_val<<endl;
        my_client->encrypt(det_val);
        ostringstream o;
        o.str("");
        o.clear();
        o<<"DONE";
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
    int host_port = 1112;
    int hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock ==-1){
            cout<<"Error initializing socket"<<endl;
    }
    cerr<<"Init socket \n";
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    cerr<<"Sockaddr \n";
    //Bind to socket
    int bind_rtn = bind(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr));
    if(bind_rtn<0){
            cerr<<"Error binding to socket"<<endl;
    }
    cerr<<"Bind \n";
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
                cout<<"Error accepting!"<<endl;
            }
            //i--;
    }
    cerr<<"Done with client, closing now\n";
    close(hsock);

}

