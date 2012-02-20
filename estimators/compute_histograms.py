from cost_functions import *
from db import *
from histograms import *

DB_NAME='tpch-1.00'

def get_histograms():
    conn = get_db_conn(DB_NAME)

    def columns_of(tbl_prefix):
        return [x for x in ORIG_COLUMNS if x[0].startswith(tbl_prefix + '_')]

    ident = lambda x: x

    converters = {
        TYPE_INT  : (ident,               lambda n, min_, max_: IntHistogram(n, min_, max_)),
        TYPE_DBL  : (lambda x: float(x),  lambda n, min_, max_: FloatHistogram(n, min_, max_)),
        TYPE_STR  : (ident,               lambda n, min_, max_: StringHistogram(n, min_, max_)),
        TYPE_DATE : (mysqlEncode,         lambda n, min_, max_: IntHistogram(n, min_, max_)),
    }

    histograms = {} # tbl_id -> { column_name : histogram }
    n_bins = 10000

    # ( table_name -> ( column_name -> histogram ) )
    for tbl_prefix, tbl_id, tbl_name in TABLE_PREFIXES:
        columns = columns_of(tbl_prefix)
        assert len(columns) > 0

        # first, general min/max table query
        terms = flatten([
            ['max(%s)' % x[0], 'min(%s)' % x[0]] if x[3] != TYPE_STR else \
            ['max(binary %s)' % x[0], 'min(binary %s)' % x[0]] for x in columns])
        query = 'SELECT %s, count(*) FROM %s' % (', '.join(terms), tbl_name)
        #print query

        c = conn.cursor()
        c.execute(query)
        res = c.fetchone()
        #print res

        column_hist = {}
        for column, i in zip(columns, xrange(len(columns))):
            max_idx = 2 * i
            min_idx = 2 * i + 1
            marshall, new_hist = converters[column[3]]
            max_ = marshall(res[max_idx])
            min_ = marshall(res[min_idx])
            column_hist[column[0]] = new_hist(n_bins, min_, max_)

        tbl_count = res[ len(columns) * 2 ]

        histograms[tbl_id] = column_hist

        column_names = [x[0] for x in columns]

        # now, sample each row in the table with independent probability 10%,
        # unless the table size is <= 1000, in which case look at teh entire
        # table
        query = \
            'SELECT %s FROM %s WHERE rand() < 0.1' % (','.join(column_names), tbl_name) \
                    if tbl_count > 1000 else \
            'SELECT %s FROM %s' % (','.join(column_names), tbl_name)

        c = conn.cursor()
        c.execute(query)

        for row in c:
            for column, i in zip(columns, xrange(len(columns))):
                try:
                    column_hist[column[0]].add_elem( converters[column[3]][0]( row[i] ) )
                except:
                    print "error with column", column[0], "value", row[i]

    return histograms

if __name__ == '__main__':
    h = get_histograms()
    with open('tpch_1.00_histograms.pickle', 'w') as fp:
        import cPickle
        cPickle.dump(h, fp)
