#pragma once

#include <vector>

namespace util {

template <typename T>
inline std::vector<T> prepend(const T& elem, const std::vector<T>& v)
{
  std::vector<T> r;
  r.reserve(v.size() + 1);
  r.push_back(elem);
  r.insert(r.end(), v.begin(), v.end());
  return r;
}

template <typename T>
inline std::set<T> vec_to_set(const std::vector<T>& v)
{
  return std::set<T>(v.begin(), v.end());
}

}
