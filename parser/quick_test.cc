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
#include <gmp.h>

#include <parser/cdb_rewrite.hh>
#include <parser/cdb_helpers.hh>
#include <parser/encdata.hh>

#include <crypto/paillier.hh>
#include <crypto/prng.hh>

#include <util/cryptdb_log.hh>

using namespace std;
using namespace NTL;

#ifdef NDEBUG
#warning "assertions are turned off- quick_test will do nothing"
#endif /* NDEBUG */

static CryptoManager cm("12345");
static crypto_manager_stub stub(&cm, false);

static void test_big_ope(uint64_t a, uint64_t b, bool test_decrypt = true)
{
    bool isBin = false;
    uint64_t i = a;
    string ct = stub.crypt<8>(cm.getmkey(), to_s(i), TYPE_INTEGER,
                           "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                           isBin, 12345);
    assert(isBin);
    assert(ct.size() <= 16);

    if (test_decrypt) {
        string pt = stub.crypt<8>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        //cerr << "i: " << i << ", pt: " << pt << endl;
        assert(to_s(i) == pt);
    }

    uint64_t i1 = b;
    string ct1 = stub.crypt<8>(cm.getmkey(), to_s(i1), TYPE_INTEGER,
                           "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                           isBin, 12345);
    assert(isBin);
    assert(ct1.size() <= 16);

    if (test_decrypt) {
        string pt1 = stub.crypt<8>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(to_s(i1) == pt1);
    }

    // test OPE properties

    // 1) pad
    ct.resize(16); ct1.resize(16);

    // 2) reverse
    ct = str_reverse(ct); ct1 = str_reverse(ct1);

    // 3) check OPE properties

    assert((i < i1) == (memcmp(ct.data(), ct1.data(), 16) < 0));
}

template <size_t SlotSize>
static inline void insert_into_slot(ZZ& z, long value, size_t slot) {
    z |= (to_ZZ(value) << (SlotSize * slot));
}

static void test_row_pack_hom_sum(
    const vector<uint32_t>& rows) {

  static const size_t BitsPerAggField = 83;
  static const size_t FieldsPerAgg = 1024 / BitsPerAggField;

  vector<string> packed_cts;
  size_t nAggs = rows.size() / FieldsPerAgg +
      (rows.size() % FieldsPerAgg ? 1 : 0);
  for (size_t i = 0; i < nAggs; i++) {
      size_t base = i * FieldsPerAgg;
      ZZ z;
      for (size_t j = 0; j < min(FieldsPerAgg, rows.size() - base); j++) {
          size_t row_id = base + j;
          insert_into_slot<BitsPerAggField>(z, rows[row_id], j);
      }
      string e0 = cm.encrypt_Paillier(z);
      e0.resize(256);
      packed_cts.push_back(e0);
  }

  vector<ZZ> sums;
  sums.resize(FieldsPerAgg, to_ZZ(1));

  string pkinfo = cm.getPKInfo();
  ZZ n2;
  ZZFromBytes(n2, (const uint8_t *) pkinfo.data(), pkinfo.size());

  uint64_t actual = 0;
  for (size_t i = 0; i < rows.size(); i++) {
    if ((i % 2) == 0) {
      size_t block_id = i / FieldsPerAgg;
      size_t block_offset = i % FieldsPerAgg;
      ZZ &sum = sums[block_offset];
      ZZ e;
      ZZFromBytes(
          e,
          (const uint8_t *) packed_cts[block_id].data(),
          256);
      MulMod(sum, sum, e, n2);
      actual += rows[i];
    }
  }

  uint64_t s = 0;
  ZZ mask = to_ZZ(1); mask <<= BitsPerAggField; mask -= 1;
  for (size_t i = 0; i < sums.size(); i++) {
    ZZ m;
    cm.decrypt_Paillier(StringFromZZ(sums[i]), m);
    uint64_t e = to_long( (m >> (BitsPerAggField * i)) & mask );
    s += e;
  }

  cerr << "actual: " << actual << ", got: " << s << endl;
  assert(actual == s);
}

static void test_larger_hom() {
    static const size_t ctbits = 1256 * 2;
    auto sk = Paillier_priv::keygen(ctbits / 2, ctbits / 8);
    Paillier_priv pp(sk);

    auto pk = pp.pubkey();
    Paillier p(pk);

    for (size_t i = 0; i < 100; i++) {
      urandom u;
      ZZ pt0 = u.rand_zz_mod(to_ZZ(1) << 1253);
      ZZ pt1 = u.rand_zz_mod(to_ZZ(1) << 1253);

      ZZ ct0 = p.encrypt(pt0);
      ZZ ct1 = p.encrypt(pt1);
      ZZ sum = p.add(ct0, ct1);
      assert(pp.decrypt(ct0) == pt0);
      assert(pp.decrypt(ct1) == pt1);
      assert(pp.decrypt(sum) == (pt0 + pt1));
    }

    {
      ZZ pt0 = (to_ZZ(1) << (1253)) - 1;
      ZZ pt1 = (to_ZZ(1) << (1253)) - 1;

      ZZ ct0 = p.encrypt(pt0);
      ZZ ct1 = p.encrypt(pt1);
      ZZ sum = p.add(ct0, ct1);
      assert(pp.decrypt(ct0) == pt0);
      assert(pp.decrypt(ct1) == pt1);
      assert(pp.decrypt(sum) == (pt0 + pt1));
    }
}

static void test_readin_mpz() {
    gmp_randstate_t state;
    gmp_randinit_default(state);
    mpz_t a, b;
    mpz_init(a);
    mpz_init(b);
    for (size_t i = 0; i < 1000; i++) {
      mpz_urandomb(a, state, 314 * 8);
      string a0 = StringFromMPZ(a);
      a0.resize(a0.size() + 2);// test corner case
      MPZFromBytes(b, (const uint8_t *) a0.data(), a0.size());
      assert( mpz_cmp(a, b) == 0 );

      //gmp_printf("a: %Zd\n", a);
      //gmp_printf("b: %Zd\n", b);
    }
    mpz_clear(a);
    mpz_clear(b);
    gmp_randclear(state);
}

static void test_copy_zz() {
    urandom u;
    for (size_t i = 0; i < 10; i++) {
      ZZ z0 = u.rand_zz_mod(to_ZZ(1) << 1024);
      string s0 = StringFromZZFast(z0);
      ZZ z1 = ZZFromStringFast(s0);
      assert( z0 == z1 );
    }
}

static void test_zz_from_compatibility() {
    urandom u;

    {
      ZZ z0 = u.rand_zz_mod(to_ZZ(1) << 1024);

      string s0 = StringFromZZ(z0);
      assert( s0.size() <= 128 );
      s0.resize( 128 ); // pad

      ZZ t0 = ZZFromString(s0);
      ZZ t1 = ZZFromStringFast(s0);

      assert( t0 == t1 );
    }

    {
      ZZ z0 = u.rand_zz_mod(to_ZZ(1) << (1256 * 2));

      string s0 = StringFromZZ(z0);
      assert( s0.size() <= (1256 * 2 / 8) );
      s0.resize( 320 ); // pad + align

      ZZ t0 = ZZFromString(s0);
      ZZ t1 = ZZFromStringFast(s0);

      assert( t0 == t1 );
    }
}

int main(int argc, char **argv) {

    test_readin_mpz();

    // test copying a ZZ object directly
    test_copy_zz();

    test_zz_from_compatibility();

    // agg tests
    test_row_pack_hom_sum({ 1, 2, 3, 4 });
    test_row_pack_hom_sum({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 });
    test_row_pack_hom_sum({
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        1, 2, 3, 4, 5, 6, 7, 8, 9, });
    test_row_pack_hom_sum(
      {17, 36, 8, 28, 24, 32, 38, 45, 49, 27, 2,
      28, 26, 30, 15, 26, 50, 37, 12, 9});

    test_larger_hom();

    // det tests

    // test int type
    {
        bool isBin = false;
        string ct = stub.crypt(cm.getmkey(), "34512", TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(uint32_t(-1)));
        string pt = stub.crypt(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(!isBin);
        assert("34512" == pt);
    }

    // test char type
    {
        bool isBin = false;
        char c = 'K';
        string ct = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(uint8_t(-1)));
        string pt = stub.crypt<1>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);

        assert(!isBin);
        uint64_t v = valFromStr(pt);
        assert(c == ((char)v));
    }

    // test date type
    {
        bool isBin = false;
        int encoding = encode_yyyy_mm_dd("1988-12-10");
        string ct = stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= max_size<3 * 8>::value);
        string pt = stub.crypt<3>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(to_s(encoding) == pt);
    }

    // test str type
    {
        bool isBin = false;
        string teststr = "testing 123";
        string ct = stub.crypt(cm.getmkey(), teststr, TYPE_TEXT,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(ct.size() == teststr.size());
        string pt = stub.crypt(cm.getmkey(), ct, TYPE_TEXT,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(pt == teststr);
    }

    // ope tests

    // test int type
    {
        bool isBin = false;
        uint32_t i = 93254;
        string ct = stub.crypt<4>(cm.getmkey(), to_s(i), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(-1));
        string pt = stub.crypt<4>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(to_s(i) == pt);

        uint32_t i1 = 1193254;
        string ct1 = stub.crypt<4>(cm.getmkey(), to_s(i1), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct1) <= uint64_t(-1));
        string pt1 = stub.crypt<4>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(to_s(i1) == pt1);

        // test OPE properties
        assert((i < i1) == (resultFromStr<uint64_t>(ct) < resultFromStr<uint64_t>(ct1)));
    }

    // test big int type
    {
        test_big_ope(99999999999999UL, 88888888888888UL);
        test_big_ope(88888888888888UL, 99999999999999UL);
        test_big_ope(12234542345UL, 823982315UL);
        test_big_ope(13932UL, 1UL);
        test_big_ope(9084234534UL, 9023859023UL);
        test_big_ope(1UL, 2UL);
        test_big_ope(18328954UL, 289345808UL);
        test_big_ope(9085908UL, 23285230346UL);
        test_big_ope(123UL, 123UL);
    }

    // test char type
    {
        bool isBin = false;
        char c = 'k';
        string ct = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(uint16_t(-1)));
        string pt = stub.crypt<1>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);

        assert(!isBin);
        uint64_t v = valFromStr(pt);
        assert(c == ((char)v));

        char c1 = 'q';
        string ct1 = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c1)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct1) <= uint64_t(uint16_t(-1)));
        string pt1 = stub.crypt<1>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);

        assert(!isBin);
        uint64_t v1 = valFromStr(pt1);
        assert(c1 == ((char)v1));

        assert((c < c1) == (resultFromStr<uint64_t>(ct) < resultFromStr<uint64_t>(ct1)));
    }

    // test date type
    {
        bool isBin = false;
        int encoding = encode_yyyy_mm_dd("1988-12-10");
        string ct = stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= max_size<3 * 8 * 2>::value);
        string pt = stub.crypt<3>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(to_s(encoding) == pt);
        assert(stringify_date_from_encoding(resultFromStr<uint32_t>(pt)) ==
               "1988-12-10");

        int encoding1 = encode_yyyy_mm_dd("1988-12-13");
        string ct1 = stub.crypt<3>(cm.getmkey(), to_s(encoding1), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct1) <= max_size<3 * 8 * 2>::value);
        string pt1 = stub.crypt<3>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(to_s(encoding1) == pt1);
        assert(stringify_date_from_encoding(resultFromStr<uint32_t>(pt1)) ==
               "1988-12-13");

        assert((encoding < encoding1) ==
               (resultFromStr<uint64_t>(ct) < resultFromStr<uint64_t>(ct1)));
    }

    return 0;
}
