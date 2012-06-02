#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#include <parser/cdb_helpers.hh>
#include <util/static_assert.hh>

/**
 * A set of helpers to make code-gen easier. These helpers are very
 * explicit (probably too much code duplication here)
 */

// ----------------- DET --------------------

template <typename T, size_t n_bytes = sizeof(T)>
inline uint64_t encrypt_at_most_u64_det(
    CryptoManager* cm, T value, size_t field_pos, bool join)
{
  _static_assert(sizeof(T) <= 8);
  _static_assert(n_bytes <= sizeof(T));
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string enc_value =
    cm_stub.crypt<n_bytes>(
        cm->getmkey(), to_s(value), TYPE_INTEGER,
        fieldname(field_pos, "DET"), SECLEVEL::PLAIN_DET,
        join ? SECLEVEL::DETJOIN : SECLEVEL::DET,
        isBin, 12345);
  assert(!isBin);
  return resultFromStr<uint64_t>(enc_value);
}

template <typename T, size_t n_bytes = sizeof(T)>
inline uint64_t decrypt_at_most_u64_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  _static_assert(sizeof(T) <= 8);
  _static_assert(n_bytes <= sizeof(T));
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string dec_value =
    cm_stub.crypt<n_bytes>(
        cm->getmkey(), value, TYPE_INTEGER,
        fieldname(field_pos, "DET"), level,
        SECLEVEL::PLAIN_DET, isBin, 12345);
  return resultFromStr<uint64_t>(dec_value);
}

inline uint64_t encrypt_u8_det(
    CryptoManager* cm, uint8_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u64_det(cm, value, field_pos, join);
}

inline uint64_t decrypt_u8_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_det<uint8_t>(cm, value, field_pos, level);
}

inline uint64_t encrypt_u32_det(
    CryptoManager* cm, uint32_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u64_det(cm, value, field_pos, join);
}

inline uint64_t decrypt_u32_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_det<uint32_t>(cm, value, field_pos, level);
}

inline uint64_t encrypt_date_det(
    CryptoManager* cm, uint32_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u64_det<uint32_t, 3>(cm, value, field_pos, join);
}

inline uint64_t decrypt_date_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_det<uint32_t, 3>(cm, value, field_pos, level);
}

inline uint64_t encrypt_u64_det(
    CryptoManager* cm, uint64_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u64_det(cm, value, field_pos, join);
}

inline uint64_t decrypt_u64_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_det<uint64_t>(cm, value, field_pos, level);
}

inline std::string encrypt_string_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, bool join)
{
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string enc_value =
    cm_stub.crypt(
        cm->getmkey(), value, TYPE_TEXT,
        fieldname(field_pos, "DET"), SECLEVEL::PLAIN_DET,
        join ? SECLEVEL::DETJOIN : SECLEVEL::DET,
        isBin, 12345);
  assert(isBin);
  return enc_value;
}

inline std::string decrypt_string_det(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string dec_value =
    cm_stub.crypt(
        cm->getmkey(), value, TYPE_TEXT,
        fieldname(field_pos, "DET"), level,
        SECLEVEL::PLAIN_DET, isBin, 12345);
  return dec_value;
}

// ----------------- OPE --------------------

template <typename T, size_t n_bytes = sizeof(T)>
inline uint64_t encrypt_at_most_u32_ope(
    CryptoManager* cm, T value, size_t field_pos, bool join)
{
  _static_assert(sizeof(T) <= 4);
  _static_assert(n_bytes <= sizeof(T));
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string enc_value =
    cm_stub.crypt<n_bytes>(
        cm->getmkey(), to_s(value), TYPE_INTEGER,
        fieldname(field_pos, "OPE"), SECLEVEL::PLAIN_OPE,
        join ? SECLEVEL::OPEJOIN : SECLEVEL::OPE,
        isBin, 12345);
  assert(!isBin);
  return resultFromStr<uint64_t>(enc_value);
}

template <typename T, size_t n_bytes = sizeof(T)>
inline uint64_t decrypt_at_most_u64_ope(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  _static_assert(sizeof(T) <= 8);
  _static_assert(n_bytes <= sizeof(T));
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string dec_value =
    cm_stub.crypt<n_bytes>(
        cm->getmkey(), value, TYPE_INTEGER,
        fieldname(field_pos, "OPE"), level,
        SECLEVEL::PLAIN_OPE, isBin, 12345);
  return resultFromStr<uint64_t>(dec_value);
}

inline uint64_t encrypt_u8_ope(
    CryptoManager* cm, uint8_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u32_ope(cm, value, field_pos, join);
}

inline uint64_t decrypt_u8_ope(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_det<uint8_t>(cm, value, field_pos, level);
}

inline uint64_t encrypt_u32_ope(
    CryptoManager* cm, uint32_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u32_ope(cm, value, field_pos, join);
}

inline uint64_t decrypt_u32_ope(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_ope<uint32_t>(cm, value, field_pos, level);
}

inline uint64_t encrypt_date_ope(
    CryptoManager* cm, uint32_t value, size_t field_pos, bool join)
{
  return encrypt_at_most_u32_ope<uint32_t, 3>(cm, value, field_pos, join);
}

inline uint64_t decrypt_date_ope(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_ope<uint32_t, 3>(cm, value, field_pos, level);
}

inline std::string encrypt_u64_ope(
    CryptoManager* cm, uint64_t value, size_t field_pos, bool join)
{
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string enc_value =
    cm_stub.crypt<8>(
        cm->getmkey(), to_s(value), TYPE_INTEGER,
        fieldname(field_pos, "OPE"), SECLEVEL::PLAIN_OPE,
        join ? SECLEVEL::OPEJOIN : SECLEVEL::OPE,
        isBin, 12345);
  assert(isBin);
  return enc_value;
}

inline uint64_t decrypt_u64_ope(
    CryptoManager* cm, const std::string& value, size_t field_pos, SECLEVEL level)
{
  return decrypt_at_most_u64_ope<uint64_t>(cm, value, field_pos, level);
}

inline uint64_t encrypt_string_ope(
    CryptoManager* cm, const std::string& value, size_t field_pos, bool join)
{
  crypto_manager_stub cm_stub(cm, false);
  bool isBin = false;
  std::string enc_value =
    cm_stub.crypt(
        cm->getmkey(), value, TYPE_TEXT,
        fieldname(field_pos, "OPE"), SECLEVEL::PLAIN_OPE,
        SECLEVEL::OPE /* OPEJOIN does not exist for strings, in current impl... */,
        isBin, 12345);
  assert(!isBin);
  // TODO: OPE string is currenlty just a prefix match...
  return resultFromStr<uint64_t>(enc_value);
}

// no string decrypt ope for now (b/c our encrypt is lossy)
