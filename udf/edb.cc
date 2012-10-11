
#include <crypto/blowfish.hh>
#include <util/params.hh>
#include <string.h>
#include <util/util.hh>
#include <errno.h>
#include <util/net.hh>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <util/net.hh>

using namespace std;
using namespace NTL;

static int sock_server = create_and_connect(OPE_SERVER_HOST, OPE_SERVER_PORT);

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

static int sock_ser = create_and_connect(OPE_SERVER_HOST, OPE_SERVER_PORT);
    
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

	MsgType mt;
	if (mode=='i'){ // we need to insert value
	    mt = MsgType::ENC_INS;

	} else { // we do not insert value, just for query
	    mt = MsgType::QUERY;
	}
	
	cerr << "mode: " << mode << " msgtype: " << mt << endl;


        char buffer[1024];
    
        memset(buffer, '\0', 1024);

        ostringstream msg;
        msg << mt << " " << det_val;

	string res = send_receive(sock_server, msg.str());
	istringstream iss(res);
	cerr << "Result for " << det_val << " is " << res << endl;
	
	uint64_t ope_rtn;
	iss >> ope_rtn;
	cerr << "Return ope_rtn=" << ope_rtn << endl;
              
	return ope_rtn;
	
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
