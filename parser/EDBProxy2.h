#ifndef _EDBProxy2_H
#define _EDBProxy2_H



#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <list>
#include <stdio.h>

#include <sql_select.h>
#include <sql_delete.h>
#include <sql_insert.h>
#include <sql_update.h>

#include <util/errstream.hh>
#include <util/cleanup.hh>
#include <util/rob.hh>
#include <parser/stringify.hh>


using namespace std;


class EDBProxy2 {
 public:

    list<string> rewriteEncryptQuery(const string & db, const string &query);

    ~EDBProxy2();


 private:


};

#endif   /* _EDBProxy2_H */
