class NumericHistogram:
    def __init__(self, n_bins, min_, max_, is_numeric):
        if is_numeric and max_ - min_ + 1 < n_bins:
            self.n_bins    = max_ - min_ + 1
            self.start     = min_
            self.min_      = min_
            self.max_      = max_
            self.step_size = 1
        else:
            self.n_bins    = n_bins
            self.start     = min_
            self.min_      = min_
            self.max_      = max_
            self.step_size = (max_ - min_ + 1) / n_bins \
                    if is_numeric else (max_ - min_) / n_bins
        self.bins = [0 for i in xrange(self.n_bins)]

    def add_elem(self, elem):
        idx = self._indexForElem(elem)
        self.bins[idx] += 1

    def _indexForElem(self, elem):
        if elem < self.min_:
            print "elem < self.min_: ", elem, self.min_
        if elem > self.max_:
            print "elem > self.max_: ", elem, self.max_

        assert elem >= self.min_
        assert elem <= self.max_

        idx = int((elem - self.start) / self.step_size)
        idx = len(self.bins) - 1 if idx >= len(self.bins) else idx
        return idx

    ### returns the fraction of elems in the histogram that are elem ###
    def eq_search(self, elem):
        if elem < self.min_ or elem > self.max_: return 0.0
        return float(self.bins[self._indexForElem(elem)]) / float(self._binSum())

    def range_search(self, lhs, rhs):
        assert lhs < rhs
        if lhs < self.min_: lhs = self.min_
        if rhs > self.max_: rhs = self.max_

        lhs_idx = self._indexForElem(lhs)
        rhs_idx = self._indexForElem(rhs)

        return float(sum([ self.bins[i] for i in xrange(lhs_idx, rhs_idx + 1) ])) / float(self._binSum())

    def distinct_values(self):
        return len([1 for x in self.bins if x])

    def _binSum(self):
        return sum(self.bins)

    def __str__(self):
        intervals = []
        cur = self.start
        for i in xrange(self.n_bins):
            intervals.append(cur)
            cur += self.step_size
        elems = [(str(c), str(c + self.step_size if idx + 1 != self.n_bins else self.max_), v) \
                    for idx, (c, v) in zip(xrange(self.n_bins), zip(intervals, self.bins))]
        return '{%s}' % ', '.join(['[%s, %s) : %d' % e for e in elems if e[2]])

    def __repr__(self): return self.__str__()

class IntHistogram(NumericHistogram):
    def __init__(self, n_bins, min_, max_):
        NumericHistogram.__init__(self, n_bins, min_, max_, True)

class FloatHistogram(NumericHistogram):
    def __init__(self, n_bins, min_, max_):
        NumericHistogram.__init__(self, n_bins, min_, max_, False)

def base256_value(s, n):
  '''s[0] is most significant'''
  if len(s) == 0 or n == 0: return 0
  return ord(s[0]) * (256 ** (n - 1)) + base256_value(s[1:], n - 1)

def str_from_base256_value(v):
  s = ''
  while v:
    s += chr(v % 256)
    v /= 256
  return s[::-1] # reverse string

def compute_num_digits(begin, end, n):
  assert begin < end
  assert n > 0

  i = 1
  while True:
    # treat strs as an i-digit base 256 number
    a = base256_value(begin, i)
    b = base256_value(end, i)
    if b - a >= n: break
    i = i + 1

  return i

def mk_strs(begin, end, n):
  assert begin < end
  assert n > 0

  i = 1
  while True:
    # treat strs as an i-digit base 256 number
    a = base256_value(begin, i)
    b = base256_value(end, i)
    if b - a >= n: break
    i = i + 1

  assert a < b
  step_size = (b - a) / n

  return [str_from_base256_value(a + i * step_size) for i in xrange(n)]

class StringHistogram(NumericHistogram):
    def __init__(self, n_bins, min_, max_):
        self.n_digits = compute_num_digits(min_, max_, n_bins)
        a = base256_value(min_, self.n_digits)
        b = base256_value(max_, self.n_digits)
        NumericHistogram.__init__(self, n_bins, a, b, True)

    def add_elem(self, elem):
        v = base256_value(elem, self.n_digits)
        NumericHistogram.add_elem(self, v)

    def eq_search(self, elem):
        return NumericHistogram.eq_search(self, base256_value(elem, self.n_digits))

    def range_search(self, lhs, rhs):
        return NumericHistogram.range_search(
                self,
                base256_value(lhs, self.n_digits),
                base256_value(rhs, self.n_digits))

if __name__ == '__main__':
    i = IntHistogram(100, 1, 100)
    i.add_elem(1)
    i.add_elem(100)
    i.add_elem(50)

    import pickle
    with open('test.pickle', 'w') as fp:
        pickle.dump(i, fp)

    with open('test.pickle', 'r') as fp:
        i = pickle.load(fp)

    print i
