#pragma once

#include <string>
#include <iostream>
#include <cstdint>

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
