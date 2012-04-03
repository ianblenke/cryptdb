#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <list>
#include <algorithm>
#include <type_traits>

#include <unistd.h>

#include <parser/cdb_rewrite.hh>
#include <parser/cdb_helpers.hh>

using namespace std;

static CryptoManager cm("12345");
static crypto_manager_stub stub(&cm, false);

static void tokenize(const string &s,
                     const string &delim,
                     vector<string> &tokens) {
    size_t i = 0;
    while (true) {
        size_t p = s.find(delim, i);
        tokens.push_back(
                s.substr(
                    i, (p == string::npos ? s.size() : p) - i));
        if (p == string::npos) break;
        i = p + delim.size();
    }
}

static vector<size_t> positions;

static void process_input(const string& s) {
  vector<string> tokens;
  tokenize(s, "|", tokens);

  for (size_t i = 0; i < positions.size(); i++) {
    bool isBin = false;
    string ct0 =
      stub.crypt<4>(cm.getmkey(), tokens[positions[i]], TYPE_INTEGER,
                    fieldname(positions[i], "DET"), SECLEVEL::PLAIN_DET, SECLEVEL::DETJOIN,
                    isBin, 12345);
    tokens.push_back(ct0);
  }

  cout << join(tokens, "|") << endl;
}

int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    positions.push_back(atoi(argv[i]));
  }
  for (;;) {
      string s;
      getline(cin, s);
      if (!cin.good())
          break;
      process_input(s);
  }
  return 0;
}
