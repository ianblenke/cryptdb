#pragma once

#include <crypto/aes.hh>
#include <crypto/ffx.hh>

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

/** same interface as CryptoManager::crypt(), but lets us try out new
 * encryptions */
class crypto_manager_stub {
public:
  crypto_manager_stub(CryptoManager* cm) : cm(cm) {}

  string crypt(AES_KEY * mkey, string data, fieldType ft,
               string fullfieldname,
               SECLEVEL fromlevel, SECLEVEL tolevel, bool & isBin,
               uint64_t salt) {
    assert(fromlevel != tolevel);
    if (fromlevel < tolevel) {
      // encryption
      if (ft == TYPE_INTEGER && getOnion(fromlevel) == oDET) {
        // we have a specialization
        return encrypt_stub(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
      }
      return cm->crypt(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
    } else {
      // decryption
      if (ft == TYPE_INTEGER && getOnion(fromlevel) == oDET) {
        return decrypt_stub(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
      }
      return cm->crypt(mkey, data, ft, fullfieldname, fromlevel, tolevel, isBin, salt);
    }
  }

private:
  string encrypt_stub(AES_KEY * mkey, string data, fieldType ft,
                      string fullfieldname,
                      SECLEVEL fromlevel, SECLEVEL tolevel, bool & isBin,
                      uint64_t salt) {
    assert(fromlevel < tolevel);
    onion o = getOnion(fromlevel);
    switch (ft) {
    case TYPE_INTEGER: {
      switch (o) {
      case oDET: {

        uint32_t val;
        if (fromlevel == SECLEVEL::PLAIN_DET) {
            int64_t r = resultFromStr<int64_t>(data);
            assert(r >= 0);

  // TODO: FIX HACK
#if defined(max)
#define _RESTORE_MAX
#undef max
#endif

            assert(uint64_t(r) <= numeric_limits<uint32_t>::max());

#if defined(_RESTORE_MAX)
#define max(a, b) ((a) < (b) ? (b) : (a))
#undef _RESTORE_MAX
#endif

            val = static_cast<uint32_t>(r);
            fromlevel = increaseLevel(fromlevel, ft, oDET);

            string _keydata = cm->getKey(mkey, "join", fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, 32, {});
            uint8_t ct[4];
            f.encrypt((const uint8_t*)&val, ct);
            val = *((uint32_t*)ct);

            if (fromlevel == tolevel) {
                return strFromVal(val);
            }
        } else {
            val = valFromStr(data);
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
            fromlevel = increaseLevel(fromlevel, ft, oDET);

            string _keydata = cm->getKey(mkey, fullfieldname, fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, 32, {});
            uint8_t ct[4];
            f.encrypt((const uint8_t*)&val, ct);
            val = *((uint32_t*)ct);

            if (fromlevel == tolevel) {
                return strFromVal(val);
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

        uint32_t val = valFromStr(data);
        if (fromlevel == SECLEVEL::DET) {
            string _keydata = cm->getKey(mkey, fullfieldname, fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, 32, {});
            uint8_t pt[4];
            f.decrypt((const uint8_t*)&val, pt);
            val = *((uint32_t*)pt);

            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                return strFromVal(val);
            }
        }

        if (fromlevel == SECLEVEL::DETJOIN) {
            string _keydata = cm->getKey(mkey, "join", fromlevel);
            AES key((const uint8_t*)_keydata.data(), _keydata.size());
            ffx2<AES> f(&key, 32, {});
            uint8_t pt[4];
            f.decrypt((const uint8_t*)&val, pt);
            val = *((uint32_t*)pt);

            fromlevel = decreaseLevel(fromlevel, ft, oDET);
            if (fromlevel == tolevel) {
                return strFromVal(val);
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


