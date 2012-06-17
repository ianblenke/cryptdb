#pragma once

#include <string>
#include <iostream>
#include <cassert>
#include <cstdint>
#include <vector>

#include <util/likely.hh>

template <typename Key>
struct serializer {
  //static void write(std::ostream& o, const Key& k) {}
};

template <typename POD>
struct PODSerializer {
  static void write(std::ostream& o, const POD& k) {
    char buf[sizeof(POD)];
    POD* p = (POD*) buf;
    *p = k;
    o << std::string(buf, sizeof(POD));
  }
};

template <>
struct serializer<uint32_t> : public PODSerializer<uint32_t> {};

template <>
struct serializer<uint64_t> : public PODSerializer<uint64_t> {};

template <>
struct serializer<std::string> {
  static void write(std::ostream& o, const std::string& k) {
    PODSerializer<uint32_t>::write(o, k.size());
    o << k;
  }
};

template <typename T>
struct deserializer {
  // static void read(const uint8_t*& ptr, T& value);
};

template <typename POD>
struct PODDeserializer {
  static void read(const uint8_t*& ptr, POD& value) {
    POD* p = (POD*) ptr;
    value = *p;
    ptr += sizeof(POD);
  }
};

template <>
struct deserializer<uint32_t> : public PODDeserializer<uint32_t> {};

template <>
struct deserializer<uint64_t> : public PODDeserializer<uint64_t> {};

template <>
struct deserializer<std::string> {
  static void read(const uint8_t*& ptr, std::string& value) {
    uint32_t s;
    PODDeserializer<uint32_t>::read(ptr, s);
    value = std::string((const char *) ptr, s);
    ptr += s;
  }
};

// varint stuff

struct varint_serializer {

  // assumes values is sorted
  static void delta_encode_uvint64_vector(
      std::ostream& buf, const std::vector<uint64_t>& values) {
    if (values.empty()) return;
    // delta-encode the numbers
    uint64_t last = values.front();
    varint_serializer::write_uvint64(buf, last);
    for (auto it = values.begin() + 1; it != values.end(); ++it) {
      uint64_t cur = *it;
      uint64_t delta = cur - last;
      varint_serializer::write_uvint64(buf, delta);
      last = cur;
    }
  }

  static void delta_decode_uvint64_vector(
      const std::string& buf, std::vector<uint64_t>& values) {
    const uint8_t* p = (const uint8_t *) buf.data();
    const uint8_t* end = p + buf.size();
    delta_decode_uvint64_vector(p, end, values);
    assert(p == end);
  }

  static void delta_decode_uvint64_vector(
      const uint8_t*& p, const uint8_t* end,
      std::vector<uint64_t>& values, ssize_t max_elems = -1) {
    assert(max_elems == -1 || max_elems >= 0);
    size_t n_read = 0;
    uint64_t last = 0;
    if (p < end && (max_elems == -1 || n_read < size_t(max_elems))) {
      read_uvint64(p, last);
      n_read++;
      values.push_back(last);
    }
    while (p < end && (max_elems == -1 || n_read < size_t(max_elems))) {
      uint64_t delta;
      read_uvint64(p, delta);
      n_read++;
      last += delta;
      values.push_back(last);
    }
    assert(p == end || (max_elems >= 0 && n_read == size_t(max_elems)));
  }

  // assumes values is sorted
  static void delta_encode_uvint32_vector(
      std::ostream& buf, const std::vector<uint32_t>& values) {
    if (values.empty()) return;
    // delta-encode the numbers
    uint32_t last = values.front();
    varint_serializer::write_uvint32(buf, last);
    for (auto it = values.begin() + 1; it != values.end(); ++it) {
      uint32_t cur = *it;
      uint32_t delta = cur - last;
      varint_serializer::write_uvint32(buf, delta);
      last = cur;
    }
  }

  static void delta_decode_uvint32_vector(
      const std::string& buf, std::vector<uint32_t>& values) {
    const uint8_t* p = (const uint8_t *) buf.data();
    const uint8_t* end = p + buf.size();
    delta_decode_uvint32_vector(p, end, values);
    assert(p == end);
  }

  static void delta_decode_uvint32_vector(
      const uint8_t*& p, const uint8_t* end,
      std::vector<uint32_t>& values, ssize_t max_elems = -1) {
    assert(max_elems == -1 || max_elems >= 0);
    size_t n_read = 0;
    uint32_t last = 0;
    if (p < end && (max_elems == -1 || n_read < size_t(max_elems))) {
      read_uvint32(p, last);
      n_read++;
      values.push_back(last);
    }
    while (p < end && (max_elems == -1 || n_read < size_t(max_elems))) {
      uint32_t delta;
      read_uvint32(p, delta);
      n_read++;
      last += delta;
      values.push_back(last);
    }
    assert(p == end || (max_elems >= 0 && n_read == size_t(max_elems)));
  }

  // read unsigned varint32 from buffer. assumes the buffer will have enough size
  static void read_uvint32(const uint8_t*& p, uint32_t& value) {
    uint32_t b, result;

    b = *p++; result  = (b & 0x7F)      ; if (LIKELY(b < 0x80)) goto done;
    b = *p++; result |= (b & 0x7F) <<  7; if (LIKELY(b < 0x80)) goto done;
    b = *p++; result |= (b & 0x7F) << 14; if (LIKELY(b < 0x80)) goto done;
    b = *p++; result |= (b & 0x7F) << 21; if (LIKELY(b < 0x80)) goto done;
    b = *p++; result |=  b         << 28; if (LIKELY(b < 0x80)) goto done;

    assert(false); // should not reach here (improper encoding)

  done:
    value = result;
  }

  static void write_uvint32(std::ostream& buf, uint32_t value) {
    while (value > 0x7F) {
      buf.put((((uint8_t) value) & 0x7F) | 0x80);
      value >>= 7;
    }
    buf.put(((uint8_t) value) & 0x7F);
  }

  // read unsigned varint64 from buffer. assumes the buffer will have enough size
  static void read_uvint64(const uint8_t*& p, uint64_t& value) {
    uint32_t b;
    uint32_t part0 = 0, part1 = 0, part2 = 0;

    b = *p++; part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *p++; part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *p++; part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *p++; part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *p++; part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *p++; part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *p++; part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *p++; part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *p++; part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *p++; part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;

    assert(false); // should not reach here (improper encoding)

  done:
    value = (static_cast<uint64_t>(part0)      ) |
            (static_cast<uint64_t>(part1) << 28) |
            (static_cast<uint64_t>(part2) << 56);
  }

  static void write_uvint64(std::ostream& buf, uint64_t value) {
    while (value > 0x7F) {
      buf.put((((uint8_t) value) & 0x7F) | 0x80);
      value >>= 7;
    }
    buf.put(((uint8_t) value) & 0x7F);
  }
};
