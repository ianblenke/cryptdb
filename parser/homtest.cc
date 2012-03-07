#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <list>
#include <algorithm>

#include <unistd.h>
#include <parser/cdb_rewrite.hh>

using namespace std;
using namespace NTL;

int main(int argc, char **argv) {
  // a = 12345
  // b = 23456
  // c = 34567
  // d = 45678

  ZZ a, b, c, d, n1;
  a = to_ZZ(12345);
  b = to_ZZ(23456);
  c = to_ZZ(34567);
  d = to_ZZ(45678);

  static const size_t wordbits = sizeof(uint64_t) * 8;
  static const size_t slotbits = wordbits * 2;

  n1 = (a << (slotbits * 3)) |
       (b << (slotbits * 2)) |
       (c << (slotbits * 1)) |
       d;

  ZZ a1, n2;
  a1 = to_ZZ(11111);
  n2 = (a1 << (slotbits * 3)) |
       (a1 << (slotbits * 2)) |
       (a1 << (slotbits * 1)) |
       a1;

  n1 = n1 + n2;

  cout << "sizeof(n1): " << NumBytes(n1) << endl;

  ZZ mask;
  mask = to_ZZ((uint64_t)-1);
  cout << to_long(n1 & mask) << endl;
  cout << to_long((n1 >> (slotbits * 1)) & mask) << endl;
  cout << to_long((n1 >> (slotbits * 2)) & mask) << endl;
  cout << to_long((n1 >> (slotbits * 3)) & mask) << endl;

  return 0;
}
