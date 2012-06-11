#pragma once

#include <map>
#include <set>
#include <vector>
#include <utility>

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
inline std::vector<T> append(const std::vector<T>& v, const T& elem)
{
  std::vector<T> r;
  r.reserve(v.size() + 1);
  r.insert(r.end(), v.begin(), v.end());
  r.push_back(elem);
  return r;
}

template <typename To, typename From>
inline std::vector<To*> change_ptr_vec_type(const std::vector<From*>& v)
{
  std::vector<To*> r;
  r.reserve(v.size());
  for (auto p : v) r.push_back(static_cast<To*>(p));
  return r;
}

template <typename T>
inline std::set<T> vec_to_set(const std::vector<T>& v)
{
  return std::set<T>(v.begin(), v.end());
}

template <typename K, typename V>
std::map<K, V> map_from_pair_vec(const std::vector< std::pair<K, V> >& v)
{
  return std::map<K, V>(v.begin(), v.end());
}

}
