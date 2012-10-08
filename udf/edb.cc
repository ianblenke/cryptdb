
#include <crypto/blowfish.hh>
#include <util/params.hh>
#include <string.h>
#include <util/util.hh>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

using namespace std;
using namespace NTL;
const int N = 4;
const double alpha = 0.3;
const int num_bits = (int) ceil(log2(N+1.0));
uint64_t mask;

uint64_t make_mask();

uint64_t make_mask(){
    uint64_t cur_mask = 1ULL;
    for(int i=1; i<num_bits; i++){
        cur_mask = cur_mask | (1ULL<<i);
    }
    return cur_mask;
}

extern "C" {

typedef unsigned long long ulonglong;
typedef long long longlong;


#include <mysql.h>
#include <ctype.h>


//UDF that creates the ops server
my_bool  create_ops_server_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
ulonglong create_ops_server(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
                         char *error);
void  create_ops_server_deinit(UDF_INIT *initid);


// UDF that interacts with ops server with respect to some encryption
my_bool  get_ops_server_init(UDF_INIT *initid, UDF_ARGS *args,
                               char *message);
void     get_ops_server_deinit(UDF_INIT *initid);
char *   get_ops_server(UDF_INIT *initid, UDF_ARGS *args, char *result,
                          unsigned long *length, char *is_null, char *error);

// UDF to request OPE encryption
my_bool  ope_enc_init(UDF_INIT *initid, UDF_ARGS *args,
                               char *message);
void     ope_enc_deinit(UDF_INIT *initid);
long long   ope_enc(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

} /* extern C */
/*
static uint64_t
getui(UDF_ARGS * args, int i)
{
    return (uint64_t) (*((ulonglong *) args->args[i]));
}
*/
/*static string
getstring(UDF_ARGS * args, int i)
{
    return string(args->args[i], args->lengths[i]);
}*/

extern "C" {
    
my_bool  ope_enc_init(UDF_INIT *initid, UDF_ARGS *args, char *message){
    return 0;
}
void ope_enc_deinit(UDF_INIT *initid){
    return;
}

long long ope_enc(UDF_INIT *initid, UDF_ARGS *args,
    char *is_null, char *error){
    long long det_val;
    det_val = *((long long*) args->args[0]);


    cerr << "inside UDF \n";

    char mode;
    mode = *((char*) args->args[1]);

    bool imode;
    if(mode=='i'){
        imode=true;
    }else{
        imode=false;
    }
    cerr<<"mode: "<<mode<<" imode: "<<imode<<endl;

    mask=make_mask();

    int host_port; 
    string host_name; 

    int hsock;
    struct sockaddr_in my_addr;
    while(true){
        host_port = 1111;
        host_name = "127.0.0.1";
        hsock = socket(AF_INET, SOCK_STREAM,0);
        if(hsock==-1) cerr<<"Error initializing socket!"<<endl;
        

        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(host_port);
        memset(&(my_addr.sin_zero),0, 8);
        my_addr.sin_addr.s_addr = inet_addr(host_name.c_str());

        if(connect(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr))<0){
            cerr<<"Connect Failed!!"<<endl;
        }

        char buffer[1024];
    
        memset(buffer, '\0', 1024);

        ostringstream o;
        o<<"1 "<<imode<<" "<<det_val;
        string msg = o.str();
        send(hsock, msg.c_str(), msg.size(), 0);
        recv(hsock, buffer, 1024, 0);
        uint64_t early_v, early_pathlen, early_index;
        uint64_t v = 0;
        uint64_t nbits = 0;

        istringstream iss(buffer);
        iss>>early_v;
        iss>>early_pathlen;
        iss>>early_index;
        close(hsock);
        if(early_v!=(uint64_t)-1 && early_pathlen!=(uint64_t)-1 && early_index!=(uint64_t)-1){
            cerr<<"In early return"<<endl;
            v = early_v;
            nbits = early_pathlen+num_bits;
            v = (v<<num_bits) | early_index;  
            uint64_t rtn_val = (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));
            cerr<<"Returning "<<rtn_val<<endl;
            return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));
        }else{
            cerr<<"No early return, must insert"<<endl;
            host_port = 1112;
            host_name = "127.0.0.1";

            hsock = socket(AF_INET, SOCK_STREAM,0);
            if(hsock==-1) cout<<"Error initializing socket!"<<endl;
            

            my_addr.sin_family = AF_INET;
            my_addr.sin_port = htons(host_port);
            memset(&(my_addr.sin_zero),0, 8);
            my_addr.sin_addr.s_addr = inet_addr(host_name.c_str());

            if(connect(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr))<0){
                cout<<"Connect failed!"<<endl;
            }

            memset(buffer, '\0', 1024);

            ostringstream o_client;
            if(imode) o_client<<"14 "<<det_val;
            else o_client<<"15 "<<det_val;
            string client_msg = o_client.str();
            send(hsock, client_msg.c_str(), client_msg.size(), 0);
            recv(hsock, buffer, 1024, 0); 
            istringstream iss(buffer);
            cerr<<"Buffer for "<<det_val<<" is "<<buffer<<endl;
            string output;
            iss>>output;
            cerr<<"Output = "<<output<<endl;
            if(output=="DONE"){
                if(!imode) cerr<<"Output done but imode = false"<<endl;
                cerr<<"Received done!"<<endl;
            }else if(output=="15"){
                if(imode) cerr<<"Output 15 but imode = true"<<endl;
                cerr<<"In 15 waa??"<<endl;
                uint64_t ope_rtn;
                iss>>ope_rtn;
                cout<<"Return ope_rtn="<<ope_rtn<<endl;
                close(hsock);
                return ope_rtn;
            }else{
                cerr<<"Received weird string: "<<buffer<<endl;
            }
            close(hsock);
        }        
    }

}

my_bool
create_ops_server_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;	
}

ulonglong
create_ops_server(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
			    char *error) {

    cerr<<"Calling create_ops_server function\n";

    string command = "/usr/lib/mysql/plugin/server";
    
    pid_t pid = fork();

    if (pid == 0) {
	// child
    cerr<<"In child now \n";
	// Frank, below replace exampleserver with the name of the
	// ope server. 
	errno = 0;
	execl(command.c_str(),
	      "server",
/*	      arg0.c_str(), 
	      arg1.c_str(),
	      arg2.c_str(),*/
	      (char *) 0);
	//error, execl should not return
	cerr << "child failed to execute " << strerror(errno) << errno << " \n";
	exit(-1);
    }

    //back in parent
    
    if (pid < 0) {
	//failed to fork
	cerr << "failed to fork ops server \n";
	return -1;
    } else {
	cerr << "forked ops server; pid: " << pid << " \n";
	return 0;
    }
}

void
create_ops_server_deinit(UDF_INIT *initid) {
}

    
my_bool
get_ops_server_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;
}

void
get_ops_server_deinit(UDF_INIT *initid)
{
    /*
     * in mysql-server/sql/item_func.cc, udf_handler::fix_fields
     * initializes initid.ptr=0 for us.
     */
    if (initid->ptr)
        delete[] initid->ptr;
}

char *
get_ops_server(UDF_INIT *initid, UDF_ARGS *args,
                 char *result, unsigned long *length,
                 char *is_null, char *error) {
   
    string addr = "127.0.0.1:1111";
    char * res = (char*) addr.c_str();
    initid->ptr = res;
    memcpy(res, "127.0.0.1:1111", strlen(res));
    *length = strlen(res);
  
    return (char*) initid->ptr;
}

    
} /* extern "C" */
