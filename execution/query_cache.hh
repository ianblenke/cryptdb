#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include <execution/tuple.hh>

/** Not a threadsafe class */
class query_cache {
public:
  typedef std::unordered_map< std::string, std::vector< db_tuple > > cache_map;
  cache_map cache;
};
