#include "client.hh"

using namespace std;

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
    char buffer[1024];

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
    recv(hsock, buffer, 1024, 0);

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
    pair<uint64_t, int> enc = encrypt(pt);
    uint64_t ct = enc.first;

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

    char buffer[1024];
    memset(buffer, '\0', 1024);

    send(hsock, msg.c_str(), msg.size(), 0);
    recv(hsock, buffer, 1024, 0);

    if(DEBUG_BTREE){
        cout<<"Delete_value buffer recv: "<<buffer<<endl;
    }
    const string tmp_merkle_hash = cur_merkle_hash;
    string new_merkle_hash;
    DelMerkleProof dmp;
    stringstream iss(buffer);
    iss>>new_merkle_hash;
    string tmp_hash_str (buffer, 1024);
    size_t hash_end = tmp_hash_str.find("hash_end");
    string new_hash_str (buffer, hash_end);
    cur_merkle_hash = new_hash_str;
/*      char tmp_hash[1024];
    strcpy(tmp_hash, buffer);
    size_t end = 
    *space='\0';
    string tmp_hash_str (tmp_hash);
    cur_merkle_hash=tmp_hash_str;   */    
    //cur_merkle_hash=new_merkle_hash;

    iss>>dmp;   

    if(DEBUG_BTREE){
        cout<<"Delete_value new mh="<<cur_merkle_hash<<endl;
        cout<<"DMP="<<dmp<<endl;
    }
    if(!verify_del_merkle_proof(dmp, "to_delete", tmp_merkle_hash, new_merkle_hash)){
        cout<<"Deletion merkle proof failed!"<<endl;
        exit(-1);
    }
    if(DEBUG_BTREE) cout<<"Deletion merkle proof succeeded"<<endl;

}


template<class V, class BlockCipher>
pair<uint64_t, int>
ope_client<V, BlockCipher>::insert(uint64_t v, uint64_t nbits, uint64_t index, V pt, V det) const{
    if(DEBUG) cout<<"pt: "<<pt<<" det: "<<det<<"  not in tree. "<<nbits<<": "<<" v: "<<v<<endl;
    
    //s->insert(v, nbits, index, pt);
    ostringstream o;
    o.str("");
    o.clear();
    o<<"3 "<<v<<" "<<nbits<<" "<<index<<" "<<det;
    string msg = o.str();
    if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;

    char buffer[1024];
    memset(buffer, '\0', 1024);
    send(hsock, msg.c_str(), msg.size(),0);
    recv(hsock, buffer, 1024, 0);

    if(DEBUG_BTREE) cout<<"Insert buffer="<<buffer<<endl;
    const string tmp_merkle_hash = cur_merkle_hash;
    string new_merkle_hash;
    InsMerkleProof imp;
    stringstream iss(buffer);
    iss>>new_merkle_hash;
    string tmp_hash_str (buffer, 1024);
    size_t hash_end = tmp_hash_str.find("hash_end");
    string new_hash_str (buffer, hash_end-1);
    cur_merkle_hash = new_hash_str;
/*      char tmp_hash[1024];
    strcpy(tmp_hash, buffer);
    char* space = strchr(tmp_hash,' ');
    *space='\0';
    string tmp_hash_str (tmp_hash);
    cur_merkle_hash=tmp_hash_str;*/
    iss>>imp;
    
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
    return encrypt(pt);
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
pair<uint64_t, int>
ope_client<V, BlockCipher>::encrypt(V pt) const{

    uint64_t v = 0;
    uint64_t nbits = 0;

    V det = block_encrypt(pt);

    if(DEBUG_COMM) cout<<"Encrypting pt: "<<pt<<" det: "<<det<<endl;

    char buffer[1024];
    memset(buffer, '\0', 1024);

    //table_storage test = s->lookup(pt);
    ostringstream o;
    o<<"1 "<<det;

    string msg = o.str();
    if(DEBUG_COMM) cout<<"Sending init msg: "<<msg<<endl;

    send(hsock, msg.c_str(), msg.size(), 0);
    recv(hsock, buffer, 1024, 0);
    uint64_t early_v, early_pathlen, early_index, early_version;

    istringstream iss(buffer);
    iss>>early_v;
    iss>>early_pathlen;
    iss>>early_index;
    iss>>early_version;

    if(DEBUG_COMM) cout<<"Received early: "<<early_v<<" : "<<early_pathlen<<" : "<<early_index<<endl;
    if(early_v!=(uint64_t)-1 && early_pathlen!=(uint64_t)-1 && early_index!=(uint64_t)-1){
        if(DEBUG_COMM) cout<<"Found match from table early!"<<endl;

        v = early_v;
        nbits = early_pathlen+num_bits;
        if(DEBUG_COMM) cout<<"Found "<<pt<<" with det="<<det<<" in table w/ v="<<v<<" nbits="<<nbits<<" index="<<early_index<<endl;
        v = (v<<num_bits) | early_index;  
        return make_pair((v<<(64-nbits)) | (mask<<(64-num_bits-nbits)),
            early_version);
    }
    for (;;) {
        if(DEBUG) cout<<"Do lookup for "<<pt<<" with det: "<<det<<" v: "<<v<<" nbits: "<<nbits<<endl;
        //vector<V> xct = s->lookup(v, nbits);
        o.str("");
        o.clear();
        o<<"2 "<<v<<" "<<nbits;
        msg = o.str();
        if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;
        send(hsock, msg.c_str(), msg.size(), 0);
        memset(buffer, 0, 1024);
        recv(hsock, buffer, 1024, 0);
        if(DEBUG_COMM || DEBUG_BTREE) cout<<"Received during iterative lookup: "<<buffer<<endl;
        string check(buffer);           

        if(check=="ope_fail"){
            return insert(v, nbits, 0, pt, det);

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

        //Throws ope_lookup_failure if xct size < N, meaning can insert at node
        int pi;
        try{
            pi = predIndex(xct_vec, pt);
        }catch(ope_lookup_failure&){
            int index;
            for(index=0; index<(int) xct_vec.size(); index++){
                if(pt<xct_vec[index]) break;
            }
            return insert(v, nbits, index, pt, det);
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
    return make_pair(0,0);
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

//0 == random
//1 == sorted incr
//2 == sorted decr
bool test_order(int num_vals, int sorted, bool deletes){

    blowfish* bc = new blowfish("frankli714");

    ope_client<uint64_t, blowfish>* my_client = new ope_client<uint64_t, blowfish>(bc);

    vector<uint64_t> inserted_vals;
    
    time_t seed;
    seed = time(NULL);
    cout<<"Seed is "<<seed<<endl;
    srand( seed);

    //srand(0);

    //Build vector of all inserted values
    for(int i=0; i<num_vals; i++){
            inserted_vals.push_back((uint64_t) rand());
    }
    if(sorted>0){
            sort(inserted_vals.begin(), inserted_vals.end());
    }
    if(sorted>1){
            reverse(inserted_vals.begin(), inserted_vals.end());
    }

    //Vector to hold already inserted values
    vector<uint64_t> tmp_vals;

    int delete_delay=0;

    for(int i=0; i<num_vals; i++){
            uint64_t val = inserted_vals[i];
            tmp_vals.push_back(val);
            pair<uint64_t,int> enc_pair= my_client->encrypt(val);
            if(enc_pair.first==0 && enc_pair.second==0){
                cout<<"Error in encryption, received 0,0 pair"<<endl;
                return false;
            }                
            uint64_t enc_val = enc_pair.first;

            //Check that the enc_val can be decrypted back to val
            if(val!=my_client->decrypt(enc_val)) {
                    cout<<"Decryption wrong! "<<val<<" : "<<enc_val<<endl;
                    return false;
            }                       
            if(DEBUG) {cout<<"Enc complete for "<<val<<" with enc="<<enc_val<<endl;cout<<endl;}
            ostringstream o;
            o.str("");
            o.clear();
            o<<enc_pair.first;
            string ope = o.str();

            o.str("");
            o.clear();
            o<<enc_pair.second;
            string version = o.str();

            o.str("");
            o.clear();
            o<<val;
            string name=o.str();

            dbconnect->execute("INSERT INTO emp VALUES ("+name+", "+ope+", "+version+")");

            int do_repeat = rand()%10+1;
            if(do_repeat<3){
                int rep_index = rand()%(tmp_vals.size());
                int rep_val = tmp_vals[rep_index];
                pair<uint64_t,int> rep_pair= my_client->encrypt(rep_val);
                if(rep_pair.first==0 && rep_pair.second==0){
                    cout<<"Error in rep_pair encryption, received 0,0 pair"<<endl;
                    return false;
                }                

                o.str("");
                o.clear();
                o<<rep_pair.first;
                string rep_ope = o.str();

                o.str("");
                o.clear();
                o<<rep_pair.second;
                string rep_version = o.str();

                o.str("");
                o.clear();
                o<<rep_val;
                string rep_name=o.str();
                dbconnect->execute("INSERT INTO emp VALUES ("+rep_name+", "+rep_ope+", "+rep_version+")");

            }

            if(deletes){
                if(delete_delay==0){
                    int do_delete = rand()%10+1;
                    if(do_delete<2){
                        delete_delay = rand()%(num_vals/2);
                        int num_dels = rand()%(tmp_vals.size()+1);
                        cout<<"Starting deletion "<<num_dels<<" of "<<tmp_vals.size()<<endl;
                        for(int d=0;d<num_dels; d++){
                            int del_index = rand()%(tmp_vals.size());
                            int del_val = tmp_vals[del_index];
                            my_client->delete_value(del_val);   
                            tmp_vals.erase(tmp_vals.begin()+del_index);        
                        }
                        cout<<"Ending deletion"<<endl;

                    }                    
                }else{
                    delete_delay--;
                }
    
            }

            //Check that all enc vals follow OPE properties
            //So if val1<val2, enc(val1)<enc(val2)
            sort(tmp_vals.begin(), tmp_vals.end());
            uint64_t last_val = 0;
            uint64_t last_enc = 0;
            uint64_t cur_val = 0;
            uint64_t cur_enc = 0;
            if(DEBUG) cout<<"Starting ope testing"<<endl;
            for(int j=0; j<(int) tmp_vals.size(); j++){
                    cur_val = tmp_vals[j];
                    cur_enc = my_client->encrypt(cur_val).first;          
                    if(cur_val>=last_val && cur_enc>=last_enc){
                            last_val = cur_val;
                            last_enc = cur_enc;
                    }else{
                            cout<<"j: "<<j<<" cur:"<<cur_val<<": "<<cur_enc<<" last: "<<last_val<<": "<<last_enc<<endl;
                            return false;
                    }
                    if(DEBUG) cout<<"Decrypting "<<cur_enc<<endl;
                    if(cur_val!=my_client->decrypt(cur_enc)) {
                            cout<<"No match with "<<j<<endl;
                            return false;
                    }
            }               
            if(DEBUG) cout<<"Done with ope testing\n"<<endl;
    }
    if(DEBUG){
        for(uint64_t i=0; i<tmp_vals.size(); i++){
            cout<<tmp_vals[i]<<" ";
        }
        cout<<endl;        
    }


    cout<<"Sorted order "<<sorted<<" passes test"<<endl;

    //Kill server
    send(my_client->hsock, "0",1,0);
    close(my_client->hsock);
    delete my_client;
    delete bc;

    return true;

}

int main(){
    //Build mask based on N
    mask=make_mask();
    
    dbconnect =new Connect( "localhost", "frank", "passwd","cryptdb", 3306);
    //dbconnect->execute("select create_ops_server()");
/*        DBResult * result;
    dbconnect->execute("select get_ope_server()", result);
    ResType rt = result->unpack();
    if(rt.ok){
    cout<<ItemToString(rt.rows[0][0])<<endl;
    }
*/
    cout<<"Connected to database"<<endl;

    usleep(1000000);

    test_order(78,0, true);
    
 /*   blowfish* bc = new blowfish("frankli714");
    ope_client<uint64_t, blowfish>* my_client = new ope_client<uint64_t, blowfish>(bc);

    uint64_t name;
    int do_enc;
    ostringstream o;
    while(true){
        cout<<"Enter plaintext val: ";
        cin>>name;
        cout<<"Encrypt (1), decrypt (2) or delete (0)?";
        cin>>do_enc;
        if(name== (uint64_t) -1){
            send(my_client->hsock, "0",1,0);
            close(my_client->hsock);
            break;
        }else{
            if(do_enc==1){
                pair<uint64_t,int> enc_pair = my_client->encrypt(name);
                if(enc_pair.first==0 && enc_pair.second==0){
                    cout<<"Error in encryption, received 0,0 pair"<<endl;
                    break;
                }
                cout<<"Encrypting "<<name<<" to "<<enc_pair.first<<endl;
                o.str("");
                o<<name;
                string name_str = o.str();

                o.str("");
                o<<enc_pair.first;
                string ope = o.str();

                o.str("");
                o<<enc_pair.second;
                string version = o.str();


                bool success = dbconnect->execute("INSERT INTO emp VALUES ("+name_str+", "+ope+", "+version+")");
                if(success) cout<<"Inserted data!"<<endl;
                else cout<<"Data insertion failed!"<<endl;                
            }else if(do_enc==0){
                my_client->delete_value(name);
            }else if(do_enc==2){
                pair<uint64_t,int> enc_pair = my_client->encrypt(name);
                if(enc_pair.first==0 && enc_pair.second==0){
                    cout<<"Error in encryption, received 0,0 pair"<<endl;
                    break;
                }
                cout<<"Name is "<<name<<" decrypts to "<<my_client->decrypt(enc_pair.first)<<endl;
            }else{
                cout<<"Not a valid operation code, please retry"<<endl;
            }

        }

    }*/

}

