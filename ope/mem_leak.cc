#include <edb/Connect.hh>
#include <util/util.hh>

int main(){
    
    Connect* db = new Connect( "localhost", "root", "letmein","cryptdb", 3306);
    for(int i=0; i< 100000; i++){
        db->execute("select count(*) from testope;");
    }
    delete db;
}