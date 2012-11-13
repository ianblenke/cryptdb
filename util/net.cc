#include <util/net.hh>
#include <util/util.hh>
#include <sys/types.h>
#include <sys/socket.h>
#include <util/ope-util.hh>
#include <util/msgsend.hh>

using namespace std;

int
create_and_bind(int host_port) {

    if (DEBUG_BARE) cerr << "Create and bind on port " << host_port << "\n";

    int hsock = socket(AF_INET, SOCK_STREAM, 0);
    assert_s(hsock >= 0, "Error initializing socket");

    // allow reuse of this socket in case we restarted server
    int optval;
    // set SO_REUSEADDR on a socket to true (1):
    optval = 1;
    setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //Bind to socket
    int bind_rtn = bind(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr));
    assert_s(bind_rtn>=0, "Error binding to socket");

    return hsock;
}

int
create_and_connect(string host_name, int host_port, bool fail) {
    if (DEBUG_BARE) cerr << "Create and connect \n";
    int hsock = socket(AF_INET, SOCK_STREAM, 0);
    assert_s(hsock>=0, "Error initializing socket!");
    
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero),0, 8);
    my_addr.sin_addr.s_addr = inet_addr(host_name.c_str());

    if (DEBUG_BARE) cerr << "trying to connect to " << host_name << " " << host_port << "\n";
    if (connect(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr))<0){
	if (fail) {
	    assert_s(false, "cannot connect");
	} else {
	    return -1;
	}
    }
    if (DEBUG_BARE) cerr << "connected \n";

    return hsock;
}

std::string
send_receive(int sock, const string & msg) {

    if (DEBUG_COMM) {cerr << "send/receive; ------ \n to send  " << msg << "\n";}
    assert_s(xmsg_send(sock, msg) == (int)msg.size(),
	     "error with send");
    
    string res;
    if (DEBUG_COMM) { cerr << "waiting for receive\n";}
    assert_s(xmsg_recv(sock, &res) > 0,
	     "error with receive");
    if (DEBUG_COMM) {cerr << "received " << res << "\n -------- \n";}

    return res;
}


ostream &
operator<<(ostream & o, MsgType mt) {
    o << mtnames[(int)mt];
    return o;
}

istream & 
operator>>(istream & o, MsgType & mt) {
    string s;
    o >> s;
    mt = string_to_msg_type(s);
    return o;
}

