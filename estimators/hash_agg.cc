#include <cstdio>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <map>
#include <set>
#include <pthread.h>

#include <execution/common.hh>
#include <execution/tuple.hh>

#include <util/util.hh>

using namespace std;

template <typename K, typename V>
static void dump_map_stats(const unordered_map<K, V>& m, bool bucket_dump = false)
{
  cout << "******* map_stats *******" << endl;
  cout << "num_buckets: " << m.bucket_count() << endl;
  for (size_t i = 0 ; i < m.bucket_count(); i++) {
    cout << "buckets[" << i << "]: " << m.bucket_size(i) << endl;
  }
  std::hash<K> hasher;
  for (size_t i = 0 ; i < m.bucket_count(); i++) {
    cout << "buckets[" << i << "]:" << endl;
    for ( auto local_it = m.begin(i); local_it!= m.end(i); ++local_it )
      std::cout << " " << local_it->first << "(" << hasher(local_it->first) << ")" << endl;
  }
}

//namespace std {
//  static bool operator<(const db_tuple& a, const db_tuple& b) {
//    size_t as = a.columns.size();
//    size_t bs = b.columns.size();
//    if (as < bs) {
//      return true;
//    } else if (as > bs) {
//      return false;
//    }
//
//    for (size_t i = 0; i < as; i++) {
//      db_elem e = a.columns[i] < b.columns[i];
//      bool eb = e.unsafe_cast_bool();
//      if (eb) {
//        return true;
//      }
//      e = a.columns[i] > b.columns[i];
//      eb = e.unsafe_cast_bool();
//      if (eb) {
//        return false;
//      }
//    }
//    return false;
//  }
//}

int main(void)
{
  unordered_map< db_tuple, vector< vector<db_elem> > > m;
  //map< db_tuple, vector< vector<db_elem> > > m;

  // assume each elem has K keys and N non-key values

  const size_t K = 2;
  const size_t N = 4;

  // create dataset - assume uniform distribution of keys

  vector< db_tuple > tuples;
  const size_t nelem = 2000000;
  for (size_t i = 0; i < nelem; i++) {
    db_tuple t;
    t.columns.reserve(K + N);
    for (size_t j = 0; j < (K + N); j++) {
      //t.columns.push_back(db_elem((int64_t) rand()));
      t.columns.push_back(db_elem((int64_t) i));

    }
    tuples.push_back(move(t));
  }

  cout << "starting test" << endl;

  //vector< db_tuple > keys;
  //for (size_t idx = 0; idx < 10; idx++) {
  //  db_tuple group_key;
  //  group_key.columns.reserve(K);
  //  for (size_t i = 0; i < K; i++)
  //    group_key.columns.push_back( tuples[idx].columns[i] );
  //  auto it = m.find(group_key);
  //  vector< vector<db_elem> > *group = NULL;
  //  if (it == m.end()) {
  //    group = &m[group_key];
  //    group->resize( N );
  //    for (size_t i = 0; i < N; i++)
  //      group->operator[](i).reserve(tuples.size());
  //  } else {
  //    group = &it->second;
  //  }
  //  keys.push_back(group_key);
  //}

  //for (size_t i = 0; i < keys.size(); i++)
  //  cout << "keys[i].hash(): " << keys[i].hash() << endl;

  //for (size_t i = 0; i < keys.size(); i++)
  //  for (size_t j = 0; j < keys.size(); j++) {
  //    cout << "keys[i]: " << keys[i] << ", keys[j]: " << keys[j] << endl;
  //    cout << "keys[i] == keys[j]" << (keys[i] == keys[j]) << endl;
  //  }

  //dump_map_stats(m, true);

  {
    // do group by, and time it
    Timer timer;
    //size_t cnt = 0;
    for (auto &t : tuples) {
      db_tuple group_key;
      group_key.columns.reserve(K);
      for (size_t i = 0; i < K; i++)
        group_key.columns.push_back( t.columns[i] );
      //cout << "group_key: " << group_key << ", hash: " << group_key.hash() << endl;
      auto it = m.find(group_key);
      vector< vector<db_elem> > *group = NULL;
      if (it == m.end()) {
        group = &m[group_key];
        group->resize( N );
      } else {
        group = &it->second;
      }
      for (size_t i = K; i < (K + N); i++) {
        group->operator[](i - K).push_back( t.columns[i] );
      }

      //if (((++cnt) % 1000) == 0) {
      //  dump_map_stats(m);
      //}
    }
    double tt = timer.lap_ms() / 1000.0;
    cout << "rate: " << (double(nelem)/tt) << " (rows/sec)" << endl;
  }
  return 0;
}
