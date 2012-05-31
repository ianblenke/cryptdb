#pragma once

#include <vector>

namespace util {

template <typename T>
inline std::vector<T> prepend(const T& elem, const std::vector<T>& v) {
  std::vector<T> r;
  r.reserve(v.size() + 1);
  r.push_back(elem);
  r.insert(r.end(), v.begin(), v.end());
  return r;
}

}
