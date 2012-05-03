#include <iostream>
#include "ope_client.hh"
#include <edb/Connect.hh>

using namespace std;

int main(){

        Connect* dbconnect =new Connect( "localhost", "username", "passwd","databasename", 3306);
        /*DBResult * result;
        dbconnect->execute("select * from test", result);
        ResType rt = result->unpack();
        if(rt.ok){
                for(int i=0; i< (int) rt.rows.size(); i++){
                        cout<<i<<" ";
                        for(int j=0; j< (int) rt.rows[i].size(); j++){
                                cout<<ItemToString(rt.rows[i][j])<<" ";
                        }
                        cout<<"\n";
                }
        }
*/
        cout<<"Connected to database"<<endl;
        not_a_cipher nac = not_a_cipher();
        ope_client<uint64_t,not_a_cipher> client = ope_client<uint64_t, not_a_cipher>(&nac);

        int i;
        string name;
        while(true){
                cout<<"Enter an employee name: ";
                cin>>name;
                cout<<"Enter salary: ";
                cin>>i;
                if(i==-1){
                        send(client.hsock, "0",1,0);
                        close(client.hsock);
                        break;
                }else{
                        ostringstream o;
                        pair<uint64_t, uint64_t> enc_pair = client.encrypt(i);
                        cout<<"Encrypting "<<i<<" to "<<enc_pair.first<<", "<<enc_pair.second<<endl;
                        o<<enc_pair.first;
                        string ope = o.str();

                        o.str("");
                        o<<enc_pair.second;
                        string det = o.str();

                        bool success = dbconnect->execute("INSERT INTO emp VALUES ('"+name+"', '"+ope+"', '"+det+"')");
                        if(success) cout<<"Inserted data!"<<endl;
                        else cout<<"Data insertion failed!"<<endl;

                }

        }
}

