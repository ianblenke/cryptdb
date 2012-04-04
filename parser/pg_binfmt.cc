#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

using namespace std;

template <typename R>
R resultFromStr(const string &r) {
    stringstream ss(r);
    R val;
    ss >> val;
    return val;
}

template <>
string resultFromStr(const string &r) { return r; }

enum data_type {
  _char, // 1 byte
  _short, // 2 bytes
  _int, // 4 bytes
  _long, // 8 bytes
  _binary, // variable length
};

static ostream& emit_u16_big_endian(ostream& o, uint16_t value) {
  o << (uint8_t) ((value >> 8) & (0xFF));
  o << (uint8_t) (value & 0xFF);
  return o;
}

static ostream& emit_u32_big_endian(ostream& o, uint32_t value) {
  o << (uint8_t) ((value >> 24) & (0xFF));
  o << (uint8_t) ((value >> 16) & (0xFF));
  o << (uint8_t) ((value >> 8) & (0xFF));
  o << (uint8_t) (value & 0xFF);
  return o;
}

static ostream& emit_u64_big_endian(ostream& o, uint64_t value) {
  o << (uint8_t) ((value >> 56UL) & (0xFF));
  o << (uint8_t) ((value >> 48UL) & (0xFF));
  o << (uint8_t) ((value >> 40UL) & (0xFF));
  o << (uint8_t) ((value >> 32UL) & (0xFF));
  o << (uint8_t) ((value >> 24UL) & (0xFF));
  o << (uint8_t) ((value >> 16UL) & (0xFF));
  o << (uint8_t) ((value >> 8UL) & (0xFF));
  o << (uint8_t) (value & 0xFF);
  return o;
}

static vector<data_type> descriptors;

static void emit_pg_row(ostream& o, const vector<string>& mysql_row) {
  assert(mysql_row.size() >= descriptors.size());
  emit_u16_big_endian(o, descriptors.size());
  for (size_t i = 0; i < descriptors.size(); i++) {
    switch (descriptors[i]) {
    case data_type::_char:
      {
        emit_u32_big_endian(o, 1);
        uint8_t v = resultFromStr<uint8_t>(mysql_row[i]);
        o << v;
      }
      break;
    case data_type::_short:
      {
        emit_u32_big_endian(o, 2);
        uint16_t v = resultFromStr<uint16_t>(mysql_row[i]);
        emit_u16_big_endian(o, v);
      }
      break;
    case data_type::_int:
      {
        emit_u32_big_endian(o, 4);
        uint32_t v = resultFromStr<uint32_t>(mysql_row[i]);
        emit_u32_big_endian(o, v);
      }
      break;
    case data_type::_long:
      {
        emit_u32_big_endian(o, 8);
        uint64_t v = resultFromStr<uint64_t>(mysql_row[i]);
        emit_u64_big_endian(o, v);
      }
      break;

    case data_type::_binary:
      {
        emit_u32_big_endian(o, mysql_row[i].size());
        o << mysql_row[i];
      }
      break;

    default: assert(false);
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    cerr << "[usage]: " << argv[0] << " fmt_args < mysql_input > pg_output" << endl;
    return 1;
  }

  string fmt_str = argv[1];
  for (size_t i = 0; i < fmt_str.size(); i++) {
    switch (fmt_str[i]) {
    case 'c': descriptors.push_back(data_type::_char); break;
    case 's': descriptors.push_back(data_type::_short); break;
    case 'i': descriptors.push_back(data_type::_int); break;
    case 'l': descriptors.push_back(data_type::_long); break;
    case 'b': descriptors.push_back(data_type::_binary); break;
    default:
      cerr << "unknown format char: " << fmt_str[i] << endl;
      return 1;
    }
  }

  // output header
  static const char hdr[] = {
    'P', 'G', 'C', 'O', 'P', 'Y', '\n', '\377', '\r', '\n', '\0',
  };
  cout << string(hdr, sizeof(hdr));
  emit_u32_big_endian(cout, 0);
  emit_u32_big_endian(cout, 0);

  // read all the values in
  char buf[8192];
  vector<string> elems;
  string cur_elem;
  bool onescape = false;
  while (cin.good()) {
      cin.read(buf, sizeof(buf));
      for (streamsize i = 0; i < cin.gcount(); i++) {
          if (onescape) {
              cur_elem.push_back(buf[i]);
              onescape = false;
              continue;
          }
          switch (buf[i]) {
          case '\\':
              onescape = true;
              break;
          case '|':
              elems.push_back(cur_elem);
              cur_elem.clear();
              break;
          case '\n':
              elems.push_back(cur_elem);
              cur_elem.clear();

              emit_pg_row(cout, elems);

              elems.clear();
              break;
          default:
              cur_elem.push_back(buf[i]);
              break;
          }
      }
  }
  assert(!onescape);

  // output footer
  emit_u16_big_endian(cout, -1);
  cout.flush();
  return 0;
}
