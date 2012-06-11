#include <crypto-old/CryptoManager.hh> /* various functions for EDB */
#include <crypto/blowfish.hh>
#include <util/params.hh>
#include <string.h>
#include <util/util.hh>
#include <errno.h>


using namespace std;
using namespace NTL;

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

} /* extern C */
/*
static uint64_t
getui(UDF_ARGS * args, int i)
{
    return (uint64_t) (*((ulonglong *) args->args[i]));
}
*/
static string
getstring(UDF_ARGS * args, int i)
{
    return string(args->args[i], args->lengths[i]);
}

extern "C" {
    
my_bool
create_ops_server_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;	
}

ulonglong
create_ops_server(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
			    char *error) {

    // get arguments from the SQL query
    string arg0 = getstring(args, 0); // a string, logfile path
    string arg1 = getstring(args, 1); 
    string arg2 = getstring(args, 2); 

    cerr << "arg0 is <" << arg0 << "> arg1 is <" << arg1
	 << "> arg2 is <" << arg2 << ">\n";

    string command = "/usr/lib/mysql/plugin/exampleserver";
    
    pid_t pid = fork();

    if (pid == 0) {
	// child

	// Frank, below replace exampleserver with the name of the
	// ope server. 
	errno = 0;
	execl(command.c_str(),
	      "exampleserver",
	      arg0.c_str(), 
	      arg1.c_str(),
	      arg2.c_str(),
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
	cout << "forked ops server; pid: " << pid << " \n";
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
