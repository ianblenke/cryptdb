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

static vector<size_t> positions;

static void process_input(const string& s) {
  vector<string> tokens;
  tokenize(s, "|", tokens);

  for (size_t i = 0; i < positions.size(); i++) {
    bool isBin = false;
#ifdef ALL_SAME_KEY
    size_t field_pos_key = 0;
    SECLEVEL field_sec_level = SECLEVEL::DET;
#else
    size_t field_pos_key = positions[i];
    SECLEVEL field_sec_level = SECLEVEL::DETJOIN;
#endif /* ALL_SAME_KEY */
    string ct0 =
      stub.crypt<4>(cm.getmkey(), tokens[positions[i]], TYPE_INTEGER,
                    fieldname(field_pos_key, "DET"), SECLEVEL::PLAIN_DET, field_sec_level,
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
