#pragma once

#include <iostream>
#include <sstream>
#include <map>

#include <crypto/aes.hh>
#include <crypto/arc4.hh>
#include <crypto/ffx.hh>
#include <crypto/prng.hh>
#include <crypto/ope.hh>

#include <crypto-old/CryptoManager.hh>

#include <util/static_assert.hh>
#include <util/onions.hh>
#include <util/util.hh>

#include <NTL/ZZ.h>

#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

namespace {

template <typename T>
inline std::string to_hex(const T& t) {
    std::ostringstream s;
    s << std::hex << t;
    return s.str();
}

template <>
inline std::string to_hex(const std::string& input) {
    size_t len = input.length();
    const char* const lut = "0123456789ABCDEF";

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char) input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

inline long roundToLong(double x) {
    return ((x)>=0?(long)((x)+0.5):(long)((x)-0.5));
}

template <typename R>
R resultFromStr(const std::string &r) {
    std::stringstream ss(r);
    R val;
    ss >> val;
    return val;
}

template <>
std::string resultFromStr(const std::string &r) { return r; }

template <typename T>
inline std::string to_s(const T& t) {
    std::ostringstream s;
    s << t;
    return s.str();
}

void tokenize(const std::string &s,
              const std::string &delim,
              std::vector<std::string> &tokens) {
    size_t i = 0;
    while (true) {
        size_t p = s.find(delim, i);
        tokens.push_back(
                s.substr(
                    i, (p == std::string::npos ? s.size() : p) - i));
        if (p == std::string::npos) break;
        i = p + delim.size();
    }
}

int encode_yyyy_mm_dd(const std::string &datestr) {
  int year, month, day;
  int ret = sscanf(datestr.c_str(), "%d-%d-%d", &year, &month, &day);
  assert(ret == 3);
  assert(1 <= day && day <= 31);
  assert(1 <= month && month <= 12);
  assert(year >= 0);
  int encoding = day | (month << 5) | (year << 9);
  return encoding;
}

void extract_date_from_encoding(uint32_t encoding,
                                uint32_t &month,
                                uint32_t &day,
                                uint32_t &year) {
  static const uint32_t DayMask = 0x1F;
  static const uint32_t MonthMask = 0x1E0;
  static const uint32_t YearMask = ((uint32_t)-1) << 9;

  day = encoding & DayMask;
  month = (encoding & MonthMask) >> 5;
  year = (encoding & YearMask) >> 9;
}

std::string stringify_date_from_encoding(uint32_t encoding) {
  uint32_t month, day, year;
  extract_date_from_encoding(encoding, month, day, year);
  std::ostringstream oss;
  oss << year << "-" << month << "-" << day;
  return oss.str();
}

std::string join(const std::vector<std::string> &tokens, const std::string &sep) {
    std::ostringstream s;
    for (size_t i = 0; i < tokens.size(); i++) {
        s << tokens[i];
        if (i != tokens.size() - 1) s << sep;
    }
    return s.str();
}

std::string fieldname(size_t fieldnum, const std::string &suffix) {
    std::ostringstream s;
    s << "field" << fieldnum << suffix;
    return s.str();
}

std::string to_mysql_escape_varbin(
        const std::string &buf, char escape = '\\', char fieldTerm = '|', char newlineTerm = '\n') {
    std::ostringstream s;
    for (size_t i = 0; i < buf.size(); i++) {
        char cur = buf[i];
        if (cur == escape || cur == fieldTerm || cur == newlineTerm) {
            s << escape;
        }
        s << cur;
    }
    return s.str();
}

inline std::string str_reverse(const std::string& orig) {
    std::ostringstream buf;
    for (std::string::const_reverse_iterator it = orig.rbegin();
         it != orig.rend(); ++it) buf << *it;
    return buf.str();
}

uint64_t ZZToU64(NTL::ZZ val)
{
    uint64_t res = 0;
    uint64_t mul = 1;
    while (val > 0) {
        res = res + mul*(NTL::to_int(val % 10));
        mul = mul * 10;
        val = val / 10;
    }
    return res;
}

} // empty namespace

template <size_t n_bits>
struct max_size {
  static const bool is_valid = n_bits <= 64;
  // if n_bits > 64, value is the max value for uint64_t
  // (and is_valid is false)
  static const uint64_t value =
    ((0x1UL << (n_bits % 64)) - 1L) | ((n_bits / 64) ? (0x1UL << 63) : 0);
};

/** same interface as CryptoManager::crypt(), but lets us try out new
 * encryptions */
class crypto_manager_stub {
public:

  crypto_manager_stub(CryptoManager* cm, bool OldOpe, bool Profile = true)
      : cm(cm), OldOpe(OldOpe), Profile(Profile) {}
  ~crypto_manager_stub() {
      for (ope_cache_map::iterator it = ope_cache.begin();
           it != ope_cache.end(); ++it) delete it->second;

      // write enc stats to stderr
      if (Profile) {
          for (timer_stats_map::iterator it = encryption_stats.begin();
               it != encryption_stats.end(); ++it) {
              for (std::map<onion, stat_entry>::iterator iit = it->second.begin();
                   iit != it->second.end(); iit++) {
                  std::cerr
                      << fieldtype_to_string(it->first) << ":"
                      << onion_to_string(iit->first) << ":"
                      << iit->second.first << ":"
                      << iit->second.second << std::endl;
              }
          }
      }
  }

  template <size_t n_bytes = 4>
  std::string crypt(AES_KEY * mkey, std::string data, fieldType ft,
               std::string fullfieldname,
               SECLEVEL fromlevel, SECLEVEL tolevel, bool & isBin,
               uint64_t salt) {
    assert(fromlevel != tolevel);

    bool specialize =
      ((ft == TYPE_INTEGER) &&
       (getOnion(fromlevel) == oDET || getOnion(fromlevel) == oOPE)) ||
      (ft == TYPE_TEXT && getOnion(fromlevel) == oDET);

    if (fromlevel < tolevel) {
      // encryption
      measure_time t(ft, getOnion(fromlevel), encryption_stats, Profile);
      if (specialize) {
        return encrypt_stub<n_bytes>(
            mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
      }
      return cm->crypt(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
    } else {
      // decryption
      if (specialize) {
        return decrypt_stub<n_bytes>(
            mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
      }
      return cm->crypt(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
    }
  }

  std::string encrypt_Paillier(uint64_t val) {
      measure_time t(TYPE_INTEGER, oAGG, encryption_stats, Profile);
      return cm->encrypt_Paillier(val);

  }

  std::string encrypt_Paillier(const NTL::ZZ& val) {
      measure_time t(TYPE_INTEGER, oAGG, encryption_stats, Profile);
      return cm->encrypt_Paillier(val);
  }

private:

    // timer stats
    // (field type -> (onion -> (n times, total enc (ms))))
    typedef std::pair< uint64_t, double > stat_entry;
    typedef std::map<
        fieldType,
        std::map<onion, stat_entry> > timer_stats_map;

    timer_stats_map encryption_stats;

    struct measure_time {
        measure_time(fieldType f, onion o, timer_stats_map& stats, bool Profile)
            : f(f), o(o), stats(&stats), Profile(Profile) {}

        ~measure_time() {
            if (Profile) {
                double elp = t.lap_ms();
                std::pair< uint64_t, double >& p = stats->operator[](f)[o];
                p.first++;
                p.second += elp;
            }
        }

        Timer t;
        const fieldType f;
        const onion o;
        timer_stats_map* const stats;
        const bool Profile;
    };

    typedef ope::OPE new_ope;
    typedef std::pair<std::string, size_t> ope_cache_key;
    typedef std::map<ope_cache_key, new_ope*> ope_cache_map;
    ope_cache_map ope_cache;

    new_ope* get_ope_object(const std::string& key, size_t nbytesp) {
        ope_cache_key ckey(key, nbytesp);
        auto i = ope_cache.find(ckey);
        if (i == ope_cache.end()) {
            new_ope* r = new new_ope(key, nbytesp * 8, nbytesp * 8 * 2);
            ope_cache[ckey] = r;
            return r;
        }
        return i->second;
    }

  template <size_t n_bytes>
  struct buffer {
    union {
      uint8_t u8p[n_bytes];
      uint64_t u64;
    } data;

    static void ctAsserts() {
      buffer<n_bytes> __attribute__((unused)) b;
      _static_assert(sizeof(b.data) == sizeof(uint64_t));
      _static_assert(&b.data.u8p == &b.data);
    }
  };

  std::string encrypt_string(const std::string& pt, const std::string& key) {
    streamrng<arc4> r(key);
    std::vector<uint8_t> pad = r.rand_vec<uint8_t>(pt.size());
    std::string ct;
    ct.resize(pad.size());
    for (size_t i = 0; i < pt.size(); i++) {
        ct[i] = pad[i] ^ pt[i];
    }
    return ct;
  }

  std::string decrypt_string(const std::string& ct, const std::string& key) {
    streamrng<arc4> r(key);
    std::vector<uint8_t> pad = r.rand_vec<uint8_t>(ct.size());
    std::string pt;
    pt.resize(pad.size());
    for (size_t i = 0; i < pt.size(); i++) {
        pt[i] = pad[i] ^ ct[i];
    }
    return pt;
  }

  template <size_t n_bytes>
  std::string encrypt_stub(AES_KEY * mkey, std::string data, fieldType ft,
                      std::string fullfieldname,
                      SECLEVEL fromlevel, SECLEVEL tolevel, bool & isBin,
                      uint64_t salt) {
    _static_assert(n_bytes <= sizeof(uint64_t));
    assert(fromlevel < tolevel);
    onion o = getOnion(fromlevel);
    switch (ft) {
    case TYPE_INTEGER: {
      switch (o) {
      case oDET: {

        uint64_t val;
        if (fromlevel == SECLEVEL::PLAIN_DET) {
          int64_t r = resultFromStr<int64_t>(data);
          assert(r >= 0); // no negative data for now

          val = static_cast<uint64_t>(r);
          assert(max_size<n_bytes * 8>::is_valid);
          assert(val <= max_size<n_bytes * 8>::value);

          fromlevel = increaseLevel(fromlevel, ft, oDET);

          std::string _keydata = cm->getKey(mkey, "join", fromlevel);
          AES key((const uint8_t*)_keydata.data(), _keydata.size());
          ffx2<AES> f(&key, n_bytes * 8, {});

          buffer<n_bytes> buf;
          buf.data.u64 = 0;

          // x86 little endian makes this ok
          f.encrypt((const uint8_t*)&val, buf.data.u8p);
          val = buf.data.u64;
          assert(max_size<n_bytes * 8>::is_valid);
          assert(val <= max_size<n_bytes * 8>::value);

          if (fromlevel == tolevel) {
            isBin = false;
            return strFromVal(val);
          }
        } else {
          val = valFromStr(data);
          assert(max_size<n_bytes * 8>::is_valid);
          assert(val <= max_size<n_bytes * 8>::value);
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
          fromlevel = increaseLevel(fromlevel, ft, oDET);

          std::string _keydata = cm->getKey(mkey, fullfieldname, fromlevel);
          AES key((const uint8_t*)_keydata.data(), _keydata.size());
          ffx2<AES> f(&key, n_bytes * 8, {});

          buffer<n_bytes> buf;
          buf.data.u64 = 0;

          f.encrypt((const uint8_t*)&val, buf.data.u8p);
          val = buf.data.u64;
          assert(max_size<n_bytes * 8>::is_valid);
          assert(val <= max_size<n_bytes * 8>::value);

          if (fromlevel == tolevel) {
            isBin = false;
            return strFromVal(val);
          }
        }

        // unsupported
        assert(false);
      }
        case oOPE: {

            uint64_t val;
            // plain_ope -> opejoin is a no-op...
            if (fromlevel == SECLEVEL::PLAIN_OPE) {
                val = valFromStr(data);

                fromlevel = increaseLevel(fromlevel, ft, oOPE);
                if (fromlevel == tolevel) {
                    isBin = false;
                    return strFromVal(val);
                }
            } else {
                val = valFromStr(data);
            }

            if (fromlevel == SECLEVEL::OPEJOIN) {
                fromlevel = increaseLevel(fromlevel, ft, oOPE);
                _static_assert(sizeof(long) == sizeof(uint64_t));
                if (OldOpe) {
                    OPE ope(cm->getKey(mkey, fullfieldname, fromlevel),
                            n_bytes * 8,
                            n_bytes * 8 * 2);
                    if (n_bytes <= 4) {
                        val = ope.encrypt(val);
                        if (fromlevel == tolevel) {
                            isBin = false;
                            return strFromVal(val);
                        }
                    } else {
                        NTL::ZZ plaintext;
                        plaintext = val;
                        std::string ciphertext = ope.encrypt(plaintext);
                        if (fromlevel == tolevel) {
                            isBin = true;
                            return ciphertext;
                        }
                    }
                } else {
                    new_ope* ope =
                        get_ope_object(cm->getKey(mkey, fullfieldname, fromlevel), n_bytes);
                    if (n_bytes <= 4) {
                        NTL::ZZ e = ope->encrypt(NTL::to_ZZ(val));
                        //assert(NumBits(e) <= (n_bytes * 2));
                        val = to_long(e); // is only correct for 64-bit machines
                                          // TODO: fix
                        if (fromlevel == tolevel) {
                            isBin = false;
                            return strFromVal(val);
                        }
                    } else {
                        NTL::ZZ plaintext;
                        plaintext = val; // is only correct for 64-bit machines...
                                         // TODO: fix
                        NTL::ZZ ctxt = ope->encrypt(plaintext);
                        if (fromlevel == tolevel) {
                            // CT larger than long int, so need to use binary
                            isBin = true;
                            return StringFromZZ(ctxt);
                        }
                    }
                }
            }

            // unsupported
            assert(false);

            return "";
        }
      default: assert(false);
      }
    }
    case TYPE_TEXT: {
        switch (o) {
        case oDET: {
            // TODO: this is not cryptographically secure, but we use
            // it for now, just to measure performance

            if (fromlevel == SECLEVEL::PLAIN_DET) {
                fromlevel  = increaseLevel(fromlevel, ft, oDET);
                data = encrypt_string(data, cm->getKey(mkey, "join", fromlevel));
                if (fromlevel == tolevel) {
                    isBin = true;
                    return data;
                }
            } else {
                // no-op
            }

            if (fromlevel == SECLEVEL::DETJOIN) {
                fromlevel = increaseLevel(fromlevel, ft, oDET);
                data = encrypt_string(data, cm->getKey(mkey, fullfieldname, fromlevel));
                if (fromlevel == tolevel) {
                    isBin = true;
                    return data;
                }
            }

            // unsupported
            assert(false);

        }
        default: assert(false);
        }
    }
    default: assert(false);
    }
    return "";
  }

  template <size_t n_bytes>
  std::string decrypt_stub(AES_KEY * mkey, std::string data, fieldType ft,
                      std::string fullfieldname,
                      SECLEVEL fromlevel, SECLEVEL tolevel, bool & isBin,
                      uint64_t salt) {
    assert(fromlevel > tolevel);
    onion o = getOnion(fromlevel);
    switch (ft) {
    case TYPE_INTEGER: {
      switch (o) {
      case oDET: {
        if (fromlevel == SECLEVEL::SEMANTIC_DET) {
          // unsupported
          assert(false);
        }

        uint64_t val = valFromStr(data);
        assert(max_size<n_bytes * 8>::is_valid);
        assert(val <= max_size<n_bytes * 8>::value);
        if (fromlevel == SECLEVEL::DET) {
            std::string _keydata = cm->getKey(mkey, fullfieldname, fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, n_bytes * 8, {});

            buffer<n_bytes> buf;
            buf.data.u64 = 0;

            f.decrypt((const uint8_t*)&val, buf.data.u8p);
            val = buf.data.u64;
            assert(max_size<n_bytes * 8>::is_valid);
            assert(val <= max_size<n_bytes * 8>::value);

            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                isBin = false;
                return strFromVal(val);
            }
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
            std::string _keydata = cm->getKey(mkey, "join", fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, n_bytes * 8, {});

            buffer<n_bytes> buf;
            buf.data.u64 = 0;
            f.decrypt((const uint8_t*)&val, buf.data.u8p);
            val = buf.data.u64;
            assert(max_size<n_bytes * 8>::is_valid);
            assert(val <= max_size<n_bytes * 8>::value);

            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                isBin = false;
                return strFromVal(val);
            }
        }
      }
      case oOPE: {
          if (fromlevel == SECLEVEL::SEMANTIC_OPE) {
            // unsupported
            assert(false);
          }

          uint64_t val;
          if (fromlevel == SECLEVEL::OPE) {
              _static_assert(sizeof(long) == sizeof(uint64_t));
              if (OldOpe) {
                  OPE ope(cm->getKey(mkey, fullfieldname, fromlevel),
                          n_bytes * 8,
                          n_bytes * 8 * 2);
                  if (n_bytes <= 4) {
                      val = valFromStr(data);
                      val = ope.decrypt(val);
                      assert(max_size<n_bytes * 8>::is_valid);
                      assert(val <= max_size<n_bytes * 8>::value);
                      fromlevel = decreaseLevel(fromlevel, ft, oOPE);
                      if (fromlevel == tolevel) {
                          isBin = false;
                          return strFromVal(val);
                      }
                  } else {
                      val = ope.decryptToU64(data);
                      assert(max_size<n_bytes * 8>::is_valid);
                      assert(val <= max_size<n_bytes * 8>::value);
                      fromlevel = decreaseLevel(fromlevel, ft, oOPE);
                      if (fromlevel == tolevel) {
                          isBin = false;
                          return strFromVal(val);
                      }
                  }
              } else {
                  new_ope* ope =
                      get_ope_object(cm->getKey(mkey, fullfieldname, fromlevel), n_bytes);
                  if (n_bytes <= 4) {
                      val = valFromStr(data);
                      NTL::ZZ ct;
                      ct = val; // only for 64-bit
                      NTL::ZZ pt = ope->decrypt(ct);
                      val = to_long(pt);
                      //assert(NTL::NumBits(pt) <= n_bytes);
                      assert(max_size<n_bytes * 8>::is_valid);
                      assert(val <= max_size<n_bytes * 8>::value);
                      fromlevel = decreaseLevel(fromlevel, ft, oOPE);
                      if (fromlevel == tolevel) {
                          isBin = false;
                          return strFromVal(val);
                      }
                  } else {
                      NTL::ZZ ct = ZZFromString(data);
                      NTL::ZZ pt = ope->decrypt(ct);
                      //assert(NTL::NumBits(pt) <= n_bytes);
                      val = to_long(pt);
                      assert(max_size<n_bytes * 8>::is_valid);
                      assert(val <= max_size<n_bytes * 8>::value);
                      fromlevel = decreaseLevel(fromlevel, ft, oOPE);
                      if (fromlevel == tolevel) {
                          isBin = false;
                          return strFromVal(val);
                      }
                  }
              }
          } else {
              val = valFromStr(data);
          }

          if (fromlevel == SECLEVEL::OPEJOIN) {
              fromlevel = decreaseLevel(fromlevel, ft, oOPE);
              if (fromlevel == tolevel) {
                  isBin = false;
                  return strFromVal(val);
              }
          }

      }
      default: assert(false);
      }
    }
  case TYPE_TEXT: {
    switch (o) {
    case oDET: {
        if (fromlevel == SECLEVEL::SEMANTIC_DET) {
          // unsupported
          assert(false);
        }

        if (fromlevel == SECLEVEL::DET) {
            data = decrypt_string(data, cm->getKey(mkey, fullfieldname, fromlevel));
            fromlevel  = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                isBin = true;
                return data;
            }
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
            data = decrypt_string(data, cm->getKey(mkey, "join", fromlevel));
            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                isBin = true;
                return data;
            }
        }
    }
    default: assert(false);
    }
  }
    default: assert(false);
    }
    return "";
  }

  // copied form crypto-old/CryptoManager.cc
  static onion getOnion(SECLEVEL l1)
  {
      switch (l1) {
      case SECLEVEL::PLAIN_DET: {return oDET; }
      case SECLEVEL::DETJOIN: {return oDET; }
      case SECLEVEL::DET: {return oDET; }
      case SECLEVEL::SEMANTIC_DET: {return oDET; }
      case SECLEVEL::PLAIN_OPE: {return oOPE; }
      case SECLEVEL::OPEJOIN: {return oOPE; }
      case SECLEVEL::OPE: {return oOPE; }
      case SECLEVEL::SEMANTIC_OPE: {return oOPE; }
      case SECLEVEL::PLAIN_AGG: {return oAGG; }
      case SECLEVEL::SEMANTIC_AGG: {return oAGG; }
      case SECLEVEL::PLAIN_SWP: {return oSWP; }
      case SECLEVEL::SWP: {return oSWP; }
      case SECLEVEL::PLAIN: {return oNONE; }
      default: {return oINVALID; }
      }
      return oINVALID;
  }

  // copied form crypto-old/CryptoManager.cc
  static SECLEVEL decreaseLevel(SECLEVEL l, fieldType ft,  onion o)
  {
      switch (o) {
      case oDET: {
          switch (l) {
          case SECLEVEL::SEMANTIC_DET: {return SECLEVEL::DET; }
          case SECLEVEL::DET: {
              return SECLEVEL::DETJOIN;
          }
          case SECLEVEL::DETJOIN: {return SECLEVEL::PLAIN_DET; }
          default: {
              assert_s(false, "cannot decrease level");
              return SECLEVEL::INVALID;
          }
          }
      }
      case oOPE: {
          switch (l) {
          case SECLEVEL::SEMANTIC_OPE: {return SECLEVEL::OPE; }
          case SECLEVEL::OPE: {
              if (ft == TYPE_INTEGER) {
                  return SECLEVEL::OPEJOIN;
              } else {
                  return SECLEVEL::PLAIN_OPE;
              }
          }
          case SECLEVEL::OPEJOIN: {return SECLEVEL::PLAIN_OPE;}
          default: {
              assert_s(false, "cannot decrease level");
              return SECLEVEL::INVALID;
          }
          }
      }
      case oAGG: {
          switch (l) {
          case SECLEVEL::SEMANTIC_AGG: {return SECLEVEL::PLAIN_AGG; }
          default: {
              assert_s(false, "cannot decrease level");
              return SECLEVEL::INVALID;
          }
          }
      }
      case oSWP: {
              assert_s(l == SECLEVEL::SWP, "cannot decrease level for other than level SWP on the SWP onion");
              return SECLEVEL::PLAIN_SWP;
      }
      default: {
          assert_s(false, "cannot decrease level");
          return SECLEVEL::INVALID;
      }
      }

  }

  // copied form crypto-old/CryptoManager.cc
  static SECLEVEL increaseLevel(SECLEVEL l, fieldType ft, onion o)
  {
      switch (o) {
      case oDET: {
          switch (l) {
          case SECLEVEL::DET:         return SECLEVEL::SEMANTIC_DET;
          case SECLEVEL::DETJOIN:     return SECLEVEL::DET;
          case SECLEVEL::PLAIN_DET:   return SECLEVEL::DETJOIN;
          default: {
              assert_s(false, "cannot increase level");
              return SECLEVEL::INVALID;   // unreachable
          }
          }
      }
      case oOPE: {
          switch (l) {
          case SECLEVEL::OPE: {return SECLEVEL::SEMANTIC_OPE; }
          case SECLEVEL::PLAIN_OPE: {
              if (ft == TYPE_INTEGER) {
                  return SECLEVEL::OPEJOIN;
              } else {
                  return SECLEVEL::OPE;
              }
          }
          case SECLEVEL::OPEJOIN: {return SECLEVEL::OPE;}
          default: {
              assert_s(false, "cannot increase level");
              return SECLEVEL::INVALID;
          }
          }
      }
      case oAGG: {
          switch (l) {
          case SECLEVEL::PLAIN_AGG: {return SECLEVEL::SEMANTIC_AGG; }
          default: {
              assert_s(false, "cannot increase level");
              return SECLEVEL::INVALID;
          }
          }
      }
      case oSWP: {
          assert_s(l == SECLEVEL::PLAIN_SWP,  "cannot increase beyond SWP");
          return SECLEVEL::SWP;
      }
      default: {
          assert_s(false, "cannot increase level");
          return SECLEVEL::INVALID;
      }
      }

  }

public:
  CryptoManager* cm;

private:
  const bool OldOpe;
  const bool Profile;
};
