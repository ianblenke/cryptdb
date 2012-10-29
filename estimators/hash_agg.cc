#include <cstdio>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <set>
#include <pthread.h>

#include <execution/common.hh>
#include <execution/tuple.hh>

#include <util/util.hh>

using namespace std;

int main(void)
{
  unordered_map< db_tuple, vector< vector<db_elem> > > m;

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
      t.columns.push_back(db_elem((int64_t) i % 10));

    }
    tuples.push_back(move(t));
  }

  {
    // do group by, and time it
    Timer timer;
    for (auto &t : tuples) {
      db_tuple group_key;
      group_key.columns.reserve(K);
      for (size_t i = 0; i < K; i++)
        group_key.columns.push_back( t.columns[i] );
      auto it = m.find(group_key);
      vector< vector<db_elem> > *group = NULL;
      if (it == m.end()) {
        group = &m[group_key];
        group->resize( N );
        for (size_t i = 0; i < N; i++)
          group->operator[](i).reserve(tuples.size());
      } else {
        group = &it->second;
      }
      for (size_t i = K; i < (K + N); i++) {
        group->operator[](i - K).push_back( t.columns[i] );
      }
    }
    double tt = timer.lap_ms() / 1000.0;
    cout << "rate: " << (double(nelem)/tt) << " (rows/sec)" << endl;
  }
  return 0;
}
