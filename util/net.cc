#include <util/net.hh>
#include <util/util.hh>

using namespace std;

int
create_and_bind(int host_port) {

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

    return hsock;
}

int
create_and_connect(string host_name, int host_port) {
    
    int hsock = socket(AF_INET, SOCK_STREAM,0);
    if(hsock==-1) cerr<<"Error initializing socket!"<<endl;
    
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero),0, 8);
    my_addr.sin_addr.s_addr = inet_addr(host_name.c_str());
    
    if (connect(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr))<0){
	assert_s(false, "Connect Failed!!");
    }

    return hsock;
}

std::string
send_receive(int sock, const string & msg) {

    send(sock, msg.c_str(), msg.size(), 0);
    
    uint buflen = 1024;
    char buffer[buflen];
	
    memset(buffer, '\0', buflen);
    
    recv(sock, buffer, buflen, 0);
    
    return string(buffer);
       
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

