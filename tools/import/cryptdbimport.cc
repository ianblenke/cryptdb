#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <rewrite_main.hh>
#include <Connect.hh>
#include <cryptdbimport.hh>

static void do_display_help(const char *arg)
{
    cout << "CryptDBImport" << endl;
    cout << "Use: " << arg << " [OPTIONS]" << endl;
    cout << "OPTIONS are:" << endl;
    cout << "-u<username>: MySQL server username" << endl;
    cout << "-p<password>: MySQL server password" << endl;
    cout << "-n: Do not execute queries. Only show stdout." << endl;
    cout << "-s<file>: MySQL's .sql dump file, originated from \"mysqldump\" tool." << endl;
    cout << "To generate DB's dump file use mysqldump, e.g.:" << endl;
    cout << "$ mysqldump -u user -ppassword --all-databases >dumpfile.sql" << endl;
    exit(0);
}


static bool 
ignore_line(const string& line)
{
    static const string begin_match("--");

    return (line.compare(0,2,begin_match) == 0); 
}

void
Import::printOutOnly(void)
{
    std::string line;
    string s("");
    ifstream input(this->filename);

    assert(input.is_open() == true); 

    while(std::getline(input, line )){
        if(ignore_line(line))
            continue;

        if (!line.empty()){
            char lastChar = *line.rbegin();
            if(lastChar == ';'){
                cout << s + line << endl;
                s.clear();
                continue;
            }
            s += line;
        }
    }
}

void
Import::executeQueries(Rewriter& r)
{
    std::string line;
    string s("");
    ifstream input(this->filename);

    assert(input.is_open() == true); 

    while(std::getline(input, line )){
        if(ignore_line(line))
            continue;

#define TMP_HACK
#ifdef TMP_HACK
        // Avoid SQL statements that are under impl. & fixes
        if(line.compare(0,15,"CREATE DATABASE") == 0)
            continue;
        if(line.compare(0,3,"USE") == 0)
            continue;
        if(line.compare(0,13,"DROP DATABASE") == 0)
            continue;
        if(line.compare(0,4,"LOCK") == 0)
            continue;
        if(line.compare(0,6,"UNLOCK") == 0)
            continue;
#endif
        if (!line.empty()){
            char lastChar = *line.rbegin();
            if(lastChar == ';'){
                s += line;
                cout << s << endl;
                executeQuery(r, s, true);
                s.clear();
                continue;
            }
            s += line;
        }
    }
}


int main(int argc, char **argv)
{
    int c, threads = 1, optind = 0;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"show", required_argument, 0, 's'},
        {"password", required_argument, 0, 'p'},
        {"user", required_argument, 0, 'u'},
        {"noexec", required_argument, 0, 'n'},
        {"threads", required_argument, 0, 't'},
        {NULL, 0, 0, 0},
    };

    string username("");
    string password("");
    bool exec = true;

    while(1)
    {
        c = getopt_long(argc, argv, "hs:p:u:t:n", long_options, &optind);
        if(c == -1)
            break;

        switch(c)
        {
            case 'h':
                do_display_help(argv[0]);
            case 's':
                {
                    Import import(optarg);
                    if(exec == true){
                        ConnectionInfo ci("localhost", username, password);
                        Rewriter r(ci, "/var/lib/shadow-mysql", "cryptdbtest", false, true);

                        // Execute queries
                        import.executeQueries(r);
                    } else
                        import.printOutOnly();
                }
                break;
            case 'p':
                password = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            case 't':
                threads = atoi(optarg);
                (void)threads;
                break;
            case 'n':
                exec = false;
                break;
            case '?':
                break;
            default:
                break;

        }
    }
    return 0;
}
