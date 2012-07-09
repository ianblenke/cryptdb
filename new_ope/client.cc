#include "client.hh"
#include <time.h>
#include <unistd.h>

using namespace std;

//0 == random
//1 == sorted incr
//2 == sorted decr
bool test_order(int num_vals, int sorted){

        ope_client<uint64_t>* my_client = new ope_client<uint64_t>();

        vector<uint64_t> inserted_vals;
        srand( time(NULL));
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
                if(DEBUG) cout<<"Enc complete for "<<val<<" with enc="<<enc_val<<endl;
                ostringstream o;
                o.str("");
                o<<enc_pair.first;
                string ope = o.str();

                o.str("");
                o<<enc_pair.second;
                string version = o.str();

                o.str("");
                o<<val;
                string name=o.str();
                dbconnect->execute("INSERT INTO emp VALUES ("+name+", "+ope+", "+version+")");

                int do_repeat = rand()%10+1;
                if(do_repeat<5){
                    int rep_index = rand()%(tmp_vals.size());
                    int rep_val = tmp_vals[rep_index];
                    pair<uint64_t,int> rep_pair= my_client->encrypt(rep_val);
                    if(rep_pair.first==0 && rep_pair.second==0){
                        cout<<"Error in rep_pair encryption, received 0,0 pair"<<endl;
                        return false;
                    }                

                    o.str("");
                    o<<rep_pair.first;
                    string rep_ope = o.str();

                    o.str("");
                    o<<rep_pair.second;
                    string rep_version = o.str();

                    o.str("");
                    o<<rep_val;
                    string rep_name=o.str();
                    dbconnect->execute("INSERT INTO emp VALUES ("+rep_name+", "+rep_ope+", "+rep_version+")");

                }

                //Check that all enc vals follow OPE properties
                //So if val1<val2, enc(val1)<enc(val2)
                sort(tmp_vals.begin(), tmp_vals.end());
                uint64_t last_val = 0;
                uint64_t last_enc = 0;
                uint64_t cur_val = 0;
                uint64_t cur_enc = 0;
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
        }

        cout<<"Sorted order "<<sorted<<" passes test"<<endl;

        //Kill server
        send(my_client->hsock, "0",1,0);
        close(my_client->hsock);
        delete my_client;

        return true;

}

int main(){
    //Build mask based on N
    mask=make_mask();
    
    dbconnect =new Connect( "localhost", "frank", "passwd","cryptdb", 3306);
    dbconnect->execute("select create_ops_server()");
/*        DBResult * result;
    dbconnect->execute("select get_ope_server()", result);
    ResType rt = result->unpack();
    if(rt.ok){
    cout<<ItemToString(rt.rows[0][0])<<endl;
    }
*/
    cout<<"Connected to database"<<endl;

    usleep(1000000);

    test_order(137,0);
    /*
    ope_client<uint64_t>* my_client = new ope_client<uint64_t>();

    int salary;
    string name;
    ostringstream o;
    while(true){
        cout<<"Enter an employee name: ";
        cin>>name;
        cout<<"Enter salary: ";
        cin>>salary;
        if(salary==-1){
            send(my_client->hsock, "0",1,0);
            close(my_client->hsock);
            break;
        }else{

            pair<uint64_t,int> enc_pair = my_client->encrypt(salary);
            if(enc_pair.first==0 && enc_pair.second==0){
                cout<<"Error in encryption, received 0,0 pair"<<endl;
                break;
            }
            cout<<"Encrypting "<<salary<<" to "<<enc_pair.first<<endl;
            o.str("");
            o<<enc_pair.first;
            string ope = o.str();

            o.str("");
            o<<enc_pair.second;
            string version = o.str();


            bool success = dbconnect->execute("INSERT INTO emp VALUES ('"+name+"', "+ope+", "+version+")");
            if(success) cout<<"Inserted data!"<<endl;
            else cout<<"Data insertion failed!"<<endl;

        }

    }
*/
}

