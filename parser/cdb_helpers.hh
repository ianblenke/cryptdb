#pragma once

#include <iostream>

#include <crypto/aes.hh>
#include <crypto/arc4.hh>
#include <crypto/ffx.hh>
#include <crypto/prng.hh>

#include <util/static_assert.hh>

#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

template <typename R>
R resultFromStr(const string &r) {
    stringstream ss(r);
    R val;
    ss >> val;
    return val;
}

template <>
string resultFromStr(const string &r) { return r; }

template <typename T>
inline string to_s(const T& t) {
    ostringstream s;
    s << t;
    return s.str();
}

int encode_yyyy_mm_dd(const string &datestr);

int encode_yyyy_mm_dd(const string &datestr) {
  int year, month, day;
  int ret = sscanf(datestr.c_str(), "%d-%d-%d", &year, &month, &day);
  assert(ret == 3);
  assert(1 <= day && day <= 31);
  assert(1 <= month && month <= 12);
  assert(year >= 0);
  int encoding = day | (month << 5) | (year << 9);
  return encoding;
}

template <size_t n_bits>
struct max_size {
  static const uint64_t value = (0x1UL << n_bits) - 1L;
};

/** same interface as CryptoManager::crypt(), but lets us try out new
 * encryptions */
class crypto_manager_stub {
public:
  crypto_manager_stub(CryptoManager* cm) : cm(cm) {}

  template <size_t n_bytes = 4>
  string crypt(AES_KEY * mkey, string data, fieldType ft,
               string fullfieldname,
               SECLEVEL fromlevel, SECLEVEL tolevel, bool & isBin,
               uint64_t salt) {
    assert(fromlevel != tolevel);
    if (fromlevel < tolevel) {
      // encryption
      if ((ft == TYPE_INTEGER || ft == TYPE_TEXT) && getOnion(fromlevel) == oDET) {
        // we have a specialization
        return encrypt_stub<n_bytes>(
            mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
      }
      return cm->crypt(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
    } else {
      // decryption
      if ((ft == TYPE_INTEGER || ft == TYPE_TEXT) && getOnion(fromlevel) == oDET) {
        return decrypt_stub<n_bytes>(
            mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
      }
      return cm->crypt(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
    }
  }

private:

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

  string encrypt_string(const string& pt, const string& key) {
    streamrng<arc4> r(key);
    vector<uint8_t> pad = r.rand_vec<uint8_t>(pt.size());
    string ct;
    ct.resize(pad.size());
    for (size_t i = 0; i < pt.size(); i++) {
        ct[i] = pad[i] ^ pt[i];
    }
    return ct;
  }

  string decrypt_string(const string& ct, const string& key) {
    streamrng<arc4> r(key);
    vector<uint8_t> pad = r.rand_vec<uint8_t>(ct.size());
    string pt;
    pt.resize(pad.size());
    for (size_t i = 0; i < pt.size(); i++) {
        pt[i] = pad[i] ^ ct[i];
    }
    return pt;
  }

  template <size_t n_bytes>
  string encrypt_stub(AES_KEY * mkey, string data, fieldType ft,
                      string fullfieldname,
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
            assert(val <= max_size<n_bytes * 8>::value);

            fromlevel = increaseLevel(fromlevel, ft, oDET);

            string _keydata = cm->getKey(mkey, "join", fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, n_bytes * 8, {});

            buffer<n_bytes> buf;
            buf.data.u64 = 0;

            // x86 little endian makes this ok
            f.encrypt((const uint8_t*)&val, buf.data.u8p);
            val = buf.data.u64;
            assert(val <= max_size<n_bytes * 8>::value);

            if (fromlevel == tolevel) {
                isBin = false;
                return strFromVal(val);
            }
        } else {
            val = valFromStr(data);
            assert(val <= max_size<n_bytes * 8>::value);
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
            fromlevel = increaseLevel(fromlevel, ft, oDET);

            string _keydata = cm->getKey(mkey, fullfieldname, fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, n_bytes * 8, {});

            buffer<n_bytes> buf;
            buf.data.u64 = 0;

            f.encrypt((const uint8_t*)&val, buf.data.u8p);
            val = buf.data.u64;
            assert(val <= max_size<n_bytes * 8>::value);

            if (fromlevel == tolevel) {
                isBin = false;
                return strFromVal(val);
            }
        }

        // unsupported
        assert(false);
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
  string decrypt_stub(AES_KEY * mkey, string data, fieldType ft,
                      string fullfieldname,
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
        assert(val <= max_size<n_bytes * 8>::value);
        if (fromlevel == SECLEVEL::DET) {
            string _keydata = cm->getKey(mkey, fullfieldname, fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, n_bytes * 8, {});

            buffer<n_bytes> buf;
            buf.data.u64 = 0;

            f.decrypt((const uint8_t*)&val, buf.data.u8p);
            val = buf.data.u64;
            assert(val <= max_size<n_bytes * 8>::value);

            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                return strFromVal(val);
            }
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
            string _keydata = cm->getKey(mkey, "join", fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, n_bytes * 8, {});

            buffer<n_bytes> buf;
            buf.data.u64 = 0;
            f.decrypt((const uint8_t*)&val, buf.data.u8p);
            val = buf.data.u64;
            assert(val <= max_size<n_bytes * 8>::value);

            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
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

  CryptoManager* cm;
};


