#include <ctime>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <util/util.hh>

using namespace std;

int main(int argc, char **argv) {

    cerr << "starting example server \n";
    
    if (argc != 4) {
	cerr << "USAGE: ./exampleserver path-to-put-a-file somearg otherarg";
	cerr << "given arg count = " << argc << " and args ";
	for (int i = 0; i <argc; i++) {
	    cerr << " <" << argv[i] << "> ";
	}
	cerr << "\n";
	return 0;
    }

    string path = argv[1];
    path += "/mylogfile";
    string rm_command = "rm -rf " + path + ";";
    string touch_command = "touch " + path + ";";
          
    assert_s(system(rm_command.c_str())>=0, "could not remove file with -rf");
    assert_s(system(touch_command.c_str())>=0, "could not create file");


    uint arg2 = atoi(argv[2]);
    string arg3 = argv[3];

    stringstream ss;

    time_t currenttime = time(NULL);
    ss << "Server started at time: " << asctime(localtime(&currenttime)) << " with arguments " << arg2 << " " << arg3 << " " << "\n";

    ofstream f;
    f.open(path, ios::app);
    assert_s(f.is_open(), "could not open file");
    f << ss.str();
    f.flush();
						
    uint counter = 0;
       
    //loop indefinitely (I am actually exiting after a counter of 1000)
    while (true) {
	chrono::seconds duration(5);
	this_thread::sleep_for(duration);

	counter++;
	f << counter << "\n";
	f.flush();
	cerr << counter << "\n";

	if (counter > 2) {
	    f << "done! \n";
	    f.close();
	    exit(0);
	}
    }
}
