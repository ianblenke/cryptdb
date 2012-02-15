#!/usr/bin/env python

from constants import *
from math import log

DET_FLAG = 0x1
OPE_FLAG = 0x1 << 1
SWP_FLAG = 0x1 << 2
AGG_FLAG = 0x1 << 3

PK_FLAG = 0x1 << 4

LINEITEM = 0
NATION   = 1
PART     = 2
PARTSUPP = 3
REGION   = 4
SUPPLIER = 5
CUSTOMER = 6
ORDERS   = 7

TABLE_PREFIXES = [
    ('l',  LINEITEM),
    ('n',  NATION),
    ('p',  PART),
    ('ps', PARTSUPP),
    ('r',  REGION),
    ('s',  SUPPLIER),
    ('c',  CUSTOMER),
    ('o',  ORDERS),
  ]

### generate the x variables ###
ORIG_COLUMNS = [
    ### LINEITEM table ###
    ('l_orderkey', DET_FLAG, 4),
    ('l_partkey', DET_FLAG, 4),
    ('l_suppkey', DET_FLAG, 4),
    ('l_linenumber', DET_FLAG, 4),
    ('l_quantity', DET_FLAG, 8),
    ('l_extendedprice', DET_FLAG, 8),
    ('l_discount', DET_FLAG, 8),
    ('l_tax', DET_FLAG, 8),
    ('l_returnflag', DET_FLAG | OPE_FLAG, 1),
    ('l_linestatus', DET_FLAG | OPE_FLAG, 1),
    ('l_shipdate', DET_FLAG | OPE_FLAG, 3),
    ('l_commitdate', DET_FLAG, 3),
    ('l_receiptdate', DET_FLAG, 3),
    ('l_shipinstruct', DET_FLAG, 25),
    ('l_shipmode', DET_FLAG, 10),
    ('l_comment', DET_FLAG, 44),

    ### NATION table ###
    ('n_nationkey', DET_FLAG, 4),
    ('n_name', DET_FLAG | OPE_FLAG, 25),
    ('n_regionkey', DET_FLAG, 4),
    ('n_comment', DET_FLAG, 152),

    ### PART table ###
    ('p_partkey', DET_FLAG | OPE_FLAG, 4),
    ('p_name', DET_FLAG | SWP_FLAG, 55),
    ('p_mfgr', DET_FLAG, 25),
    ('p_brand', DET_FLAG, 10),
    ('p_type', DET_FLAG | SWP_FLAG, 25),
    ('p_size', DET_FLAG, 4),
    ('p_container', DET_FLAG, 10),
    ('p_retailprice', DET_FLAG, 8),
    ('p_comment', DET_FLAG, 23),

    ### PARTSUPP table ###
    ('ps_partkey', DET_FLAG | PK_FLAG, 4),
    ('ps_suppkey', DET_FLAG | PK_FLAG, 4),
    ('ps_availqty', DET_FLAG, 4),
    ('ps_supplycost', DET_FLAG | OPE_FLAG, 8),
    ('ps_comment', DET_FLAG, 199),

    ### REGION table ###
    ('r_regionkey', DET_FLAG, 4),
    ('r_name', DET_FLAG, 25),
    ('r_comment', DET_FLAG, 152),

    ### SUPPLIER table ###
    ('s_suppkey', DET_FLAG | PK_FLAG, 4),
    ('s_name', DET_FLAG | OPE_FLAG, 25),
    ('s_address', DET_FLAG, 40),
    ('s_nationkey', DET_FLAG, 4),
    ('s_phone', DET_FLAG, 15),
    ('s_acctbal', DET_FLAG | OPE_FLAG, 8),
    ('s_comment', DET_FLAG, 101),

    ### CUSTOMER table ###
    ('c_custkey', DET_FLAG | PK_FLAG, 4),
    ('c_name', DET_FLAG, 25),
    ('c_address', DET_FLAG, 40),
    ('c_nationkey', DET_FLAG, 4),
    ('c_phone', DET_FLAG, 15),
    ('c_acctbal', DET_FLAG, 8),
    ('c_mktsegment', DET_FLAG, 10),
    ('c_comment', DET_FLAG, 117),

    ### ORDERS table ###
    ('o_orderkey', DET_FLAG | PK_FLAG, 4),
    ('o_custkey', DET_FLAG, 4),
    ('o_orderstatus', DET_FLAG, 1),
    ('o_totalprice', DET_FLAG | OPE_FLAG, 8),
    ('o_orderdate', DET_FLAG | OPE_FLAG, 3),
    ('o_orderpriority', DET_FLAG, 15),
    ('o_clerk', DET_FLAG, 15),
    ('o_shippriority', DET_FLAG, 4),
    ('o_comment', DET_FLAG, 79),
]

### generate the y variables ###
VIRTUAL_COLUMNS = [
    ### LINEITEM table ###
    ('l_pack0', AGG_FLAG, 256),

    ### PARTSUPP table ###
    ('ps_pack0', AGG_FLAG, 256),
]

def get_orig_table_row_size(prefix):
    return sum([x[2] for x in ORIG_COLUMNS if x[0].startswith(prefix + '_')])

_cachedColumnSizeLookup = None
def get_column_size(cname):
    global _cachedColumnSizeLookup
    if not _cachedColumnSizeLookup:
        _cachedColumnSizeLookup = {}
        for entry in ORIG_COLUMNS + VIRTUAL_COLUMNS:
            entries = gen_variables_from_entry(entry, False)
            for entry in entries:
                _cachedColumnSizeLookup[entry[0]] = entry[1]
    return _cachedColumnSizeLookup[cname]

def hasAllBits(x, mask):
    return (x & mask) == mask

def gen_variables_from_entry(entry, allowForceDet=True):
    '''
    Takes a ('orig col name', flags, size) entry, and returns
    a (non-empty) list of ( ('1' | variables name), size )
    '''

    filters = {
        DET_FLAG : 'det',
        OPE_FLAG : 'ope',
        SWP_FLAG : 'swp',
        AGG_FLAG : 'agg',
    }

    size_fcns = {
        DET_FLAG : lambda x: x,
        OPE_FLAG : lambda x: 2 * x,
        SWP_FLAG : lambda x: x * 16.0 / 3.0,
        AGG_FLAG : lambda x: 256,
    }

    ForceDet = allowForceDet
    if hasAllBits(entry[1], DET_FLAG | OPE_FLAG):
        # we have a choice
        ForceDet = False

    ret = []
    for k, v in filters.iteritems():
        if entry[1] & k:
            name = '%s_%s' % (entry[0], v)
            if ForceDet and k == DET_FLAG: name = '1'
            ret.append( (name, size_fcns[k](entry[2])) )
    return ret

def mult_terms(lhs, rhs):
    return "(%s) * (%s)" % (lhs, rhs)

def add_terms(lhs, rhs):
    return "(%s) + (%s)" % (lhs, rhs)

def add_terms_l(terms):
    return ' + '.join(['(%s)' % x for x in terms])

def scale(s, terms):
    return [mult_terms(s, x) for x in terms]

def flatten(lists):
    ret = []
    for l in lists:
        for e in l:
            ret.append(e)
    return ret

def gen_all_variables():
    '''
    Does NOT include non-variables
    '''
    return (
        ([x for x in flatten([gen_variables_from_entry(e) for e in ORIG_COLUMNS]) if x[0] != '1']),
        ([x for x in flatten([gen_variables_from_entry(e) for e in VIRTUAL_COLUMNS]) if x[0] != '1']))

def gen_variables_for_table(tblprefix):
    '''
    Includes non-variables, for length calculation
    '''
    cands = [x for x in ORIG_COLUMNS + VIRTUAL_COLUMNS if x[0].startswith(tblprefix + '_')]
    vs = [gen_variables_from_entry(x) for x in cands]
    return flatten(vs)

def gen_table_size_expr(numrows, prefix):
    tbl_vars        = gen_variables_for_table(prefix)
    row_length_expr = add_terms_l([mult_terms(x[0], str(x[1])) for x in tbl_vars])
    return mult_terms(str(numrows), row_length_expr)

# TODO: right now, assumes the index is over the DET column
# (even if the OPE column was chosen)
def gen_index_scan_size_expr(numrows, prefix):
    fill_factor = 2.0 / 3.0 # TODO: not sure if this is a good number to use

    pk_entries = [x for x in ORIG_COLUMNS if x[0].startswith(prefix + '_') and x[1] & PK_FLAG]

    assert len(pk_entries) > 0

    pk_entry = sum([x[2] for x in pk_entries])

    return str(pk_entry * numrows * (1.0 / fill_factor))

def compute_sort_cost(nelems, entry_size):
    NumFilesortPages = (nelems * entry_size) / INNODB_PAGE_SIZE

    SortEstimatePageIOs = \
        NumFilesortPages * log(NumFilesortPages, N_MERGE_BUFFERS)

    SortNumBytesXF = SortEstimatePageIOs * INNODB_PAGE_SIZE

    return (1.0 / ((READ_BW + WRITE_BW) / 2.0)) * SortNumBytesXF

def entry_size(column_names):
    return sum([get_column_size(c) for c in column_names])

def require_all_det_tbl(tbl):
    return [x[0] for x in gen_variables_for_table(tbl) if x[0].endswith('_det')]

def require_all_det_tbls(tbl_list):
    return flatten([require_all_det_tbl(t) for t in tbl_list])

### Query 1 ###
def query1_cost_functions(table_sizes):
    '''
    returns a list of (cost expr, set(required variables))
    '''

    numrows = table_sizes[LINEITEM]

    lineitem_vars = gen_variables_for_table('l')
    row_length_expr = add_terms_l([mult_terms(x[0], str(x[1])) for x in lineitem_vars])
    table_size_expr = mult_terms(str(numrows), row_length_expr)

    def plan1():
        '''
        the det-only plan

        SELECT l_returnflag_DET, l_linestatus_DET, l_quantity_DET, l_extendedprice_DET, l_discount_DET, l_tax_DET, l_shipdate_DET
        FROM lineitem_enc_noopt
        '''

        ResultSetEntry = ['l_returnflag_det', 'l_linestatus_det', 'l_quantity_det',
             'l_extendedprice_det', 'l_discount_det', 'l_tax_det',
             'l_shipdate_det']

        ProjRecordLength = entry_size(ResultSetEntry)
        DataSentBack = numrows * ProjRecordLength

        # formulate expr for row length
        encrypt_expr  = str(OPE_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), table_size_expr)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str((4 * DET_DEC) * numrows)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr, xfer_expr, decrypt_expr]),
            set(require_all_det_tbl('l')).intersection(set(ResultSetEntry)))

    def plan_opt(send_back_det):
        '''
        the optimized plan

        SELECT
          (l_returnflag_DET | l_returnflag_OPE),
          (l_linestatus_DET | l_linestatus_OPE),
          agg(l_bitpacked_AGG, X), count(*)
        FROM lineitem_enc
        WHERE l_shipdate_OPE <= Y
        GROUP BY l_returnflag_OPE, l_linestatus_OPE
        ORDER BY l_returnflag_OPE, l_linestatus_OPE
        '''

        ### Perfect statistics ###
        ResultCardinality = 10

        ProjRecordLength = entry_size(
            ['l_returnflag_det', 'l_linestatus_det', 'l_pack0_agg'] if send_back_det else \
            ['l_returnflag_ope', 'l_linestatus_ope', 'l_pack0_agg'])

        DataSentBack = ResultCardinality * ProjRecordLength

        # formulate expr for row length
        encrypt_expr  = str(OPE_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), table_size_expr)
        sort_expr     = str(compute_sort_cost(numrows, 258))
        agg_expr      = str(numrows * AGG_ADD)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = \
            str((2.0 * DET_DEC + AGG_DEC) * ResultCardinality) if send_back_det else \
            str((2.0 * OPE_DEC + AGG_DEC) * ResultCardinality)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         sort_expr, agg_expr, xfer_expr, decrypt_expr]),
            set(
              ['l_returnflag_det', 'l_linestatus_det',
               'l_returnflag_ope', 'l_linestatus_ope',
               'l_shipdate_ope', 'l_pack0_agg'] if send_back_det else \
              ['l_returnflag_ope', 'l_linestatus_ope',
               'l_shipdate_ope', 'l_pack0_agg']
              ))

    def plan2(): return plan_opt(False)
    def plan3(): return plan_opt(True)

    return [plan1(), plan2(), plan3()]

### Query 2 ###
def query2_cost_functions(table_sizes):
    '''
    returns a list of (cost expr, set(required variables))
    '''

    part_vars       = gen_variables_for_table('p')
    row_length_expr = add_terms_l([mult_terms(x[0], str(x[1])) for x in part_vars])
    table_size_expr = mult_terms(str(table_sizes[PART]), row_length_expr)

    def plan1():
        '''
        the det only plan:

        query:
        select
          s_acctbal, s_name, n_name, p_partkey, p_mfgr,
          s_address, s_phone, s_comment, [p_size, p_type, ps_supplycost]
        from
          part, supplier, partsupp, nation, region
        where
          p_partkey = ps_partkey
          and s_suppkey = ps_suppkey
          and s_nationkey = n_nationkey
          and n_regionkey = r_regionkey
          and r_name = 'ASIA'

        the rest is done server side processing:

        '''

        ### perfect statistics ###
        ResultSetNRows = 160240

        # we model (imperfectly) as a single sequential scan over
        # the outer-most join table ( use query opt to determine
        # join ordering )

        # in this case, explain tells us that PART will be sequentially scanned

        ResultSetEntry = [
              's_acctbal_det',
              's_name_det',
              'n_name_det',
              'p_partkey_det',
              'p_mfgr_det',
              's_address_det',
              's_phone_det',
              's_comment_det',
              'p_size_det',
              'p_type_det',
              'ps_supplycost_det']

        ProjRecordLength = entry_size(ResultSetEntry)

        DataSentBack = ResultSetNRows * ProjRecordLength

        encrypt_expr  = str(DET_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), table_size_expr)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str((11.0 * DET_DEC) * ResultSetNRows)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         xfer_expr, decrypt_expr]),
            set(require_all_det_tbls(['p', 's', 'ps', 'n', 'r'])).intersection(set(ResultSetEntry) ))

    def plan_opt(send_back_det):
        '''
        the "translate all ops" plan

        SELECT
          (s_acctbal_DET | s_acctbal_OPE),
          (s_name_DET | s_name_OPE),
          (n_name_DET | n_name_OPE),
          (p_partkey_DET | p_partkey_OPE),
          p_mfgr_DET, s_address_DET, s_phone_DET, s_comment_DET, p_type_DET
        FROM part_enc, supplier_enc, partsupp_enc, nation_enc, region_enc
        WHERE
          p_partkey_DET = ps_partkey_DET AND
          s_suppkey_DET = ps_suppkey_DET AND
          p_size_DET =   A   AND
          searchSWP(X, Y, p_type_SWP) = 1 AND
          s_nationkey_DET = n_nationkey_DET AND
          n_regionkey_DET = r_regionkey_DET AND
          r_name_DET = Z AND
          ps_supplycost_OPE = (
            SELECT
              min(ps_supplycost_OPE)
            FROM partsupp_enc, supplier_enc, nation_enc, region_enc
            WHERE
              p_partkey_DET = ps_partkey_DET AND
              s_suppkey_DET = ps_suppkey_DET AND
              s_nationkey_DET = n_nationkey_DET AND
              n_regionkey_DET = r_regionkey_DET AND
              r_name_DET =   Z  )
        ORDER BY
          s_acctbal_OPE DESC, n_name_OPE, s_name_OPE, p_partkey_OPE
        LIMIT 100

        note, once again, we model this query as only doing a single
        sequential scan over the part_enc table, and then
        doing a merge-sort (filesort) on the joined result.
        '''

        ### Perfect statistics ###
        NMatchingRows = 443
        # comes from ORDER BY clause
        SortEntrySize = entry_size(['s_acctbal_ope', 'n_name_ope', 's_name_ope', 'p_partkey_ope'])

        ResultSetEntry = \
           ['s_acctbal_det', 's_name_det', 'n_name_det', 'p_partkey_det',
            'p_mfgr_det', 's_address_det', 's_phone_det', 's_comment_det',
            'p_type_det'] if send_back_det else \
           ['s_acctbal_ope', 's_name_ope', 'n_name_ope', 'p_partkey_ope',
            'p_mfgr_det', 's_address_det', 's_phone_det', 's_comment_det',
            'p_type_det']

        ResultSetEntrySize = entry_size(ResultSetEntry)

        encrypt_expr  = str( 2 * DET_ENC + SWP_ENC )
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), table_size_expr)
        search_expr   = str( SWP_SEARCH * table_sizes[PART] )
        sort_expr     = str(compute_sort_cost(NMatchingRows, SortEntrySize))
        xfer_expr     = str((1.0 / NETWORK_BW) * ResultSetEntrySize * 100.0)
        decrypt_expr  = \
            str((9.0 * DET_DEC) * 100.0) if send_back_det else \
            str((4.0 * OPE_DEC + 5.0 * DET_DEC) * 100.0)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         search_expr, sort_expr, xfer_expr, decrypt_expr]),
            set(['p_type_swp', 'ps_supplycost_ope',
                 's_acctbal_ope', 'n_name_ope', 's_name_ope', 'p_partkey_ope',
                 's_acctbal_det', 'n_name_det', 's_name_det', 'p_partkey_det']) \
              if send_back_det else \
            set(['p_type_swp', 'ps_supplycost_ope',
                 's_acctbal_ope', 'n_name_ope', 's_name_ope', 'p_partkey_ope']))

    def plan2(): return plan_opt(False)
    def plan3(): return plan_opt(True)

    return [plan1(), plan2(), plan3()]

### Query 11 ###
def query11_cost_functions(table_sizes):
    '''
    returns a list of (cost expr, set(required variables))
    '''

    ps_table_size_expr = gen_table_size_expr(table_sizes[PARTSUPP], 'ps')

    def plan1():
        '''
        the det only plan:

        query:
        SELECT
          ps_partkey_DET, ps_supplycost_DET, ps_availqty_DET
        FROM partsupp_enc_noopt, supplier_enc, nation_enc
        WHERE
          ps_suppkey_DET = s_suppkey_DET AND
          s_nationkey_DET = n_nationkey_DET AND
          n_name_DET = X

        (partsupp is the outer-most join table)

        the rest is done server side processing:
        '''

        ### perfect statistics ###
        ResultSetNRows = 33040

        ResultSetEntry = ['ps_partkey_det', 'ps_supplycost_det', 'ps_availqty_det']

        ProjRecordLength = entry_size(ResultSetEntry)

        DataSentBack = ResultSetNRows * ProjRecordLength

        encrypt_expr  = str(DET_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), ps_table_size_expr)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str((len(ResultSetEntry) * DET_DEC) * ResultSetNRows)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         xfer_expr, decrypt_expr]),
            set(require_all_det_tbls(['ps', 's', 'n'])).intersection(set(ResultSetEntry)))

    def plan2():
        '''
        the optimized plan

        query:
        SELECT
          ps_partkey_DET, agg(ps_pack0)
        FROM partsupp_enc, supplier_enc, nation_enc
        WHERE
          ps_suppkey_DET = s_suppkey_DET AND
          s_nationkey_DET = n_nationkey_DET AND
          n_name_DET = X
        GROUP BY ps_partkey_DET

        NOTE: our cost estimate assumes the outer relation (partsupp)
        is scanned via index scan. therefore no sorting is needed.

        '''

        ### perfect statistics ###
        ResultSetNRows = 31110

        ResultSetEntry = ['ps_partkey_det', 'ps_pack0_agg']

        ProjRecordLength = entry_size(ResultSetEntry)

        DataSentBack = ResultSetNRows * ProjRecordLength

        encrypt_expr  = str(DET_ENC)
        rtt_expr      = str(RTT)

        ### TODO: not sure which one is right to use here ###

        #index_seek_expr = 4.0 * str(SEEK) # TODO: don't just assume 4
        #index_scan_expr = mult_terms(str(1.0 / READ_BW), gen_index_scan_size_expr(table_sizes[PARTSUPP], 'ps'))
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), ps_table_size_expr)

        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str(( DET_DEC + AGG_DEC ) * ResultSetNRows)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         xfer_expr, decrypt_expr]),
            set(require_all_det_tbls(['ps', 's', 'n'])).intersection(set(ResultSetEntry)))

    return [plan1(), plan2()]

### Query 14 ###
def query14_cost_functions(table_sizes):
    '''
    returns a list of (cost expr, set(required variables))
    '''

    l_table_size_expr = gen_table_size_expr(table_sizes[LINEITEM], 'l')

    def plan1():
        '''
        the det only plan:

        query:
        SELECT p_type_DET, l_extendedprice_DET, l_discount_DET, l_shipdate_DET
        FROM lineitem_enc_noopt, part_enc
        WHERE
          l_partkey_DET = p_partkey_DET

        (lineitem is the outer-most join table)

        the rest is done server side processing:
        '''

        ### perfect statistics ###
        ResultSetNRows = 6001215

        ResultSetEntry = ['p_type_det', 'l_extendedprice_det', 'l_discount_det', 'l_shipdate_det']

        ProjRecordLength = entry_size(ResultSetEntry)

        DataSentBack = ResultSetNRows * ProjRecordLength

        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str(( 4.0 * DET_DEC ) * ResultSetNRows)

        return (
            add_terms_l([rtt_expr, seek_expr, seq_scan_expr,
                         xfer_expr, decrypt_expr]),
            set(require_all_det_tbls(['p', 'l'])).intersection(set(ResultSetEntry + ['p_partkey_det'])))

    def plan2():
        '''
        the no-agg (but all other opts) plan

        query:
        SELECT p_type_DET, l_extendedprice_DET, l_discount_DET
        FROM lineitem_enc_noopt, part_enc
        WHERE
          l_partkey_DET = p_partkey_DET AND
          l_shipdate_OPE >= X AND l_shipdate_OPE < Y

        (lineitem is the outer-most join table)

        the rest is done server side processing:
        '''

        ### perfect statistics ###
        ResultSetNRows = 76969

        ResultSetEntry = ['p_type_det', 'l_extendedprice_det', 'l_discount_det']

        ProjRecordLength = entry_size(ResultSetEntry)

        DataSentBack = ResultSetNRows * ProjRecordLength

        encrypt_expr  = str(2.0 * OPE_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str(( 3.0 * DET_DEC ) * ResultSetNRows)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         xfer_expr, decrypt_expr]),
            set(list(set(require_all_det_tbls(['p', 'l'])).intersection(set(ResultSetEntry + ['p_partkey_det']))) + ['l_shipdate_ope']))

    def plan3():
        '''
        the optimized plan

        query:
        SELECT
          agg(CASE WHEN searchSWP(..., ..., p_type_SWP) = 1 THEN l_pack0_AGG ELSE NULL END, ...),
          agg(l_pack0_AGG, ...)
        FROM lineitem_enc, part_enc
        WHERE
          l_partkey_DET = p_partkey_DET AND
          l_shipdate_OPE >= X AND
          l_shipdate_OPE < Y

        '''

        ### perfect statistics ###
        IntermediateNRows = 76969

        ResultSetNRows = 1
        ProjRecordLength = 512
        DataSentBack = ResultSetNRows * ProjRecordLength

        encrypt_expr  = str(2.0 * OPE_ENC + SWP_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
        agg_expr      = str(( 2.0 * AGG_ADD + SWP_SEARCH ) * IntermediateNRows)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str(( 2.0 * AGG_DEC ) * ResultSetNRows)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         agg_expr, xfer_expr, decrypt_expr]),
            set(['p_type_swp', 'l_pack0_agg', 'l_shipdate_ope', 'p_partkey_det']))

    return [plan1(), plan2(), plan3()]

### Query 18 ###
def query18_cost_functions(table_sizes):
    '''
    returns a list of (cost expr, set(required variables))
    '''

    l_table_size_expr = gen_table_size_expr(table_sizes[LINEITEM], 'l')
    o_table_size_expr = gen_table_size_expr(table_sizes[ORDERS], 'o')

    def plan1():
        '''
        query1:
        select l_orderkey_DET, l_quantity_DET from lineitem

        query2:
        select
            c_name_DET, c_custkey_DET, o_orderkey_DET,
            o_orderdate_DET, o_totalprice_DET, agg(l_pack0_AGG, ...)
        from
            customer, orders, lineitem
        where
            o_orderkey_DET in (...)
            and c_custkey_DET = o_custkey_DET
            and o_orderkey_DET = l_orderkey_DET
        group by
            c_name_DET,
            c_custkey_DET,
            o_orderkey_DET,
            o_orderdate_DET,
            o_totalprice_DET
        order by
            o_totalprice_OPE desc,
            o_orderdate_OPE
        limit 100

        (orders is outer-join table)
        '''

        def q1_expr():
            ResultSetEntry = ['l_orderkey_det', 'l_quantity_det']
            ResultSetEntrySize = entry_size(ResultSetEntry)

            rtt_expr      = str(RTT)
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
            xfer_expr     = str((1.0 / NETWORK_BW) * ResultSetEntrySize * table_sizes[LINEITEM])
            decrypt_expr  = str(DET_DEC * table_sizes[LINEITEM])

            return add_terms_l([rtt_expr, seek_expr, seq_scan_expr, xfer_expr, decrypt_expr])

        def q2_expr():
            ### perfect statistics ###
            ResultSetNElems = 9
            NSortEntries = 63

            ResultSetEntry = [
                'c_name_det', 'c_custkey_det', 'o_orderkey_det',
                'o_orderdate_det', 'o_totalprice_det', 'l_pack0_agg' ]
            ResultSetEntrySize = entry_size(ResultSetEntry)

            SortEntry = [
                'o_totalprice_ope',
                'o_orderdate_ope',
                'c_name_det',
                'c_custkey_det',
                'o_orderkey_det',
                'l_pack0_agg',
            ]
            SortEntrySize = entry_size(SortEntry)

            rtt_expr      = str(RTT)
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), o_table_size_expr)
            sort_expr     = str(compute_sort_cost(NSortEntries, SortEntrySize))
            xfer_expr     = str((1.0 / NETWORK_BW) * ResultSetEntrySize * ResultSetNElems)
            decrypt_expr  = str((5.0 * DET_DEC + AGG_DEC)*ResultSetNElems)

            return add_terms_l([rtt_expr, seek_expr, seq_scan_expr, sort_expr, xfer_expr, decrypt_expr])

        return (
            add_terms(q1_expr(), q2_expr()),
            set(require_all_det_tbls(['l', 'o', 'c'])).intersection(
              set(['l_orderkey_det', 'l_quantity_det',
                   'c_name_det', 'c_custkey_det', 'o_orderkey_det',
                   'o_orderdate_det', 'o_totalprice_det', 'o_custkey_det'])))

    def plan2():
        '''
        query1:
        select l_orderkey_DET, agg(l_pack0_AGG, ...) from lineitem group by l_orderkey_DET

        query2:
        select
            c_name_DET, c_custkey_DET, o_orderkey_DET,
            o_orderdate_DET, o_totalprice_DET, agg(l_pack0_AGG, ...)
        from
            customer, orders, lineitem
        where
            o_orderkey_DET in (...)
            and c_custkey_DET = o_custkey_DET
            and o_orderkey_DET = l_orderkey_DET
        group by
            c_name_DET,
            c_custkey_DET,
            o_orderkey_DET,
            o_orderdate_DET,
            o_totalprice_DET
        order by
            o_totalprice_OPE desc,
            o_orderdate_OPE
        limit 100
        '''

        def q1_expr():
            ### perfect statistics ###
            ResultSetNElems = 1500000

            ResultSetEntry = ['l_orderkey_det', 'l_pack0_agg']
            ResultSetEntrySize = entry_size(ResultSetEntry)

            rtt_expr      = str(RTT)
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
            agg_expr      = str(AGG_ADD * table_sizes[LINEITEM])
            xfer_expr     = str((1.0 / NETWORK_BW) * ResultSetEntrySize * ResultSetNElems)
            decrypt_expr  = str(AGG_DEC * ResultSetNElems)

            return add_terms_l([rtt_expr, seek_expr, seq_scan_expr, agg_expr, xfer_expr, decrypt_expr])

        def q2_expr():
            ### perfect statistics ###
            ResultSetNElems = 9
            NSortEntries = 63

            ResultSetEntry = [
                'c_name_det', 'c_custkey_det', 'o_orderkey_det',
                'o_orderdate_det', 'o_totalprice_det', 'l_pack0_agg' ]
            ResultSetEntrySize = entry_size(ResultSetEntry)

            SortEntry = [
                'o_totalprice_ope',
                'o_orderdate_ope',
                'c_name_det',
                'c_custkey_det',
                'o_orderkey_det',
                'l_pack0_agg',
            ]
            SortEntrySize = entry_size(SortEntry)

            rtt_expr      = str(RTT)
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), o_table_size_expr)
            sort_expr     = str(compute_sort_cost(NSortEntries, SortEntrySize))
            xfer_expr     = str((1.0 / NETWORK_BW) * ResultSetEntrySize * ResultSetNElems)
            decrypt_expr  = str((5.0 * DET_DEC + AGG_DEC)*ResultSetNElems)

            return add_terms_l([rtt_expr, seek_expr, seq_scan_expr, sort_expr, xfer_expr, decrypt_expr])

        return (
            add_terms(q1_expr(), q2_expr()),
            set(require_all_det_tbls(['l', 'o', 'c'])).intersection(
              set(['l_orderkey_det', 'l_quantity_det',
                   'c_name_det', 'c_custkey_det', 'o_orderkey_det',
                   'o_orderdate_det', 'o_totalprice_det', 'o_custkey_det'])))

    return [plan1(), plan2()]

### Query 20 ###
def query20_cost_functions(table_sizes):
    '''
    returns a list of (cost expr, set(required variables))
    '''
    p_table_size_expr = gen_table_size_expr(table_sizes[PART], 'p')
    l_table_size_expr = gen_table_size_expr(table_sizes[LINEITEM], 'l')
    s_table_size_expr = gen_table_size_expr(table_sizes[SUPPLIER], 's')

    def plan1():
        '''
        the det only plan:

        Here, we apply a greedy heuristic in generating this det-only plan,
        which says greedily evaluate sub-queries independently, if possible

        query1:
        select p_partkey_DET from PART;

        query2:
        select ps_partkey_DET, ps_suppkey_DET, ps_availqty_DET, l_shipdate_DET
        from PARTSUPP, LINEITEM
        where
          ps_partkey_DET = l_partkey_DET and
          ps_suppkey_DET = l_suppkey_DET and
          ps_partkey_DET in (x0, x1, ..., xN) and

        (lineitem is the outer-most join table)

        query3:
        select s_name_DET, s_address_DET from SUPPLIER, NATION
        where
          s_suppkey_DET in (x0, x1, ..., xN) and
          s_nationkey_DET = n_nationkey_DET and
          n_name_DET = X

        the rest is done server side processing:
        '''

        def q1_expr():
            ResultSetNRows = table_sizes[PART]
            ResultSetEntry = ['p_partkey_det']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack = ResultSetNRows * ProjRecordLength

            rtt_expr      = str(RTT)
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), p_table_size_expr)
            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str(DET_DEC * ResultSetNRows)

            return add_terms_l([rtt_expr, seek_expr, seq_scan_expr, xfer_expr, decrypt_expr])

        def q2_expr():
            ### perfect statistics ###
            ResultSetNRows   = 321993
            ResultSetEntry   = ['ps_partkey_det', 'ps_suppkey_det', 'ps_availqty_det', 'l_shipdate_det']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack     = ResultSetNRows * ProjRecordLength

            rtt_expr      = str(RTT)
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str(2.0 * DET_DEC * ResultSetNRows)

            return add_terms_l([rtt_expr, seek_expr, seq_scan_expr, xfer_expr, decrypt_expr])

        def q3_expr():
            ### perfect statistics ###
            ResultSetNRows   = 404
            ResultSetEntry   = ['s_name_det', 's_address_det']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack     = ResultSetNRows * ProjRecordLength

            rtt_expr      = str(RTT)
            idx_seek_expr = str(4.0 * SEEK) # assume 4 seeks to leaf node
            idx_scan_expr = mult_terms(str(1.0 / READ_BW), gen_index_scan_size_expr(table_sizes[SUPPLIER], 's'))
            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str((2.0 * DET_DEC) * ResultSetNRows)

            return add_terms_l([rtt_expr, idx_seek_expr, idx_scan_expr, xfer_expr, decrypt_expr])

        return (
            add_terms_l([q1_expr(), q2_expr(), q3_expr()]),
            set(require_all_det_tbls(['p', 'ps', 'l', 's', 'n'])).intersection(
              set(['p_partkey_det',
                   'ps_partkey_det', 'ps_suppkey_det', 'ps_availqty_det', 'l_shipdate_det',
                   'l_partkey_det', 'l_suppkey_det', 's_name_det', 's_address_det',
                   's_suppkey_det', 's_nationkey_det', 'n_nationkey_det', 'n_name_det'])))

    def plan2():
        '''
        the optimized plan (minus agg)

        query1:
        select ps_partkey_DET, ps_suppkey_DET, ps_availqty_DET, l_quantity_DET
        from PARTSUPP, LINEITEM
        where
            ps_partkey_DET = l_partkey_DET and
            ps_suppkey_DET = l_suppkey_DET and
            ps_partkey_DET in (
              select p_partkey_DET from PART where searchSWP(.., ..., p_name_SWP) = 1
            )
            and l_shipdate_OPE >= X
            and l_shipdate_OPE < Y

        lineitem is the outer join table

        query2:
        select s_name_DET, s_address_DET
        from SUPPLIER, NATION
        where
          s_suppkey_DET in (x0, x1, ..., xN) and
          s_nationkey_DET = n_nationkey_DET and
          n_name_DET = X
        order by s_name_OPE

        '''

        def q1_expr():
            ### perfect statistics ###
            ResultSetNRows   = 48822
            ResultSetEntry   = ['ps_partkey_det', 'ps_suppkey_det', 'ps_availqty_det', 'l_quantity_det']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack     = ResultSetNRows * ProjRecordLength

            encrypt_expr  = str(SWP_ENC + OPE_ENC * 2)
            rtt_expr      = str(RTT)

            # inner query
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), p_table_size_expr)
            search_expr   = str(table_sizes[PART] * SWP_SEARCH)

            # outer query
            seek_expr1     = str(SEEK)
            seq_scan_expr1 = mult_terms(str(1.0 / READ_BW), l_table_size_expr)

            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str(2.0 * DET_DEC * ResultSetNRows)

            return add_terms_l([ encrypt_expr, rtt_expr, seek_expr, seq_scan_expr, search_expr,
                                 seek_expr1, seq_scan_expr1, xfer_expr, decrypt_expr ])

        def q2_expr():
            ### perfect statistics ###
            ResultSetNRows   = 404
            ResultSetEntry   = ['s_name_det', 's_address_det']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack     = ResultSetNRows * ProjRecordLength

            rtt_expr      = str(RTT)
            idx_seek_expr = str(4.0 * SEEK) # assume 4 seeks to leaf node
            idx_scan_expr = mult_terms(str(1.0 / READ_BW), gen_index_scan_size_expr(table_sizes[SUPPLIER], 's'))
            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str((2.0 * DET_DEC) * ResultSetNRows)

            # TODO: currently ignoring sort cost (since it should be relatively cheap)

            return add_terms_l([rtt_expr, idx_seek_expr, idx_scan_expr, xfer_expr, decrypt_expr])

        return (
            add_terms_l([q1_expr(), q2_expr()]),
            set(require_all_det_tbls(['p', 'ps', 'l', 's', 'n'])).intersection(
              set(['p_partkey_det',
                   'ps_partkey_det', 'ps_suppkey_det', 'ps_availqty_det', 'l_quantity_det',
                   'l_partkey_det', 'l_suppkey_det', 's_name_det', 's_address_det',
                   's_suppkey_det', 's_nationkey_det', 'n_nationkey_det', 'n_name_det'])).union(set(
                  ['p_name_swp', 'l_shipdate_ope', 's_name_ope'])))

    def plan3():
        '''
        the optimized plan (including agg)

        query1:
        select ps_suppkey_DET, ps_availqty_DET, agg(l_pack0_AGG,...)
        from PARTSUPP, LINEITEM
        where
            ps_partkey_DET = l_partkey_DET and
            ps_suppkey_DET = l_suppkey_DET and
            ps_partkey_DET in (
              select p_partkey_DET from PART where searchSWP(.., ..., p_name_SWP) = 1
            )
            and l_shipdate_OPE >= X
            and l_shipdate_OPE < Y
        group by ps_partkey_DET, ps_suppkey_DET

        lineitem is the outer join table

        query2:
        select s_name_DET, s_address_DET
        from SUPPLIER, NATION
        where
          s_suppkey_DET in (x0, x1, ..., xN) and
          s_nationkey_DET = n_nationkey_DET and
          n_name_DET = X
        order by s_name_OPE

        '''

        def q1_expr():
            ### perfect statistics ###
            ResultSetNRows   = 29096
            ResultSetEntry   = ['ps_suppkey_det', 'ps_availqty_det', 'l_pack0_agg']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack     = ResultSetNRows * ProjRecordLength

            SortEntry    = ['ps_partkey_det', 'ps_suppkey_det', 'ps_availqty_det', 'l_pack0_agg']
            SortNEntries = 48822

            encrypt_expr  = str(SWP_ENC + OPE_ENC * 2)
            rtt_expr      = str(RTT)

            # inner query
            seek_expr     = str(SEEK)
            seq_scan_expr = mult_terms(str(1.0 / READ_BW), p_table_size_expr)
            search_expr   = str(table_sizes[PART] * SWP_SEARCH)

            # outer query
            seek_expr1     = str(SEEK)
            seq_scan_expr1 = mult_terms(str(1.0 / READ_BW), l_table_size_expr)
            sort_expr1     = str(compute_sort_cost(SortNEntries, entry_size(SortEntry)))

            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str((DET_DEC + AGG_DEC) * ResultSetNRows)

            return add_terms_l([ encrypt_expr, rtt_expr, seek_expr, seq_scan_expr, search_expr,
                                 seek_expr1, seq_scan_expr1, sort_expr1, xfer_expr, decrypt_expr ])

        def q2_expr():
            ### perfect statistics ###
            ResultSetNRows   = 404
            ResultSetEntry   = ['s_name_det', 's_address_det']
            ProjRecordLength = entry_size(ResultSetEntry)
            DataSentBack     = ResultSetNRows * ProjRecordLength

            rtt_expr      = str(RTT)
            idx_seek_expr = str(4.0 * SEEK) # assume 4 seeks to leaf node
            idx_scan_expr = mult_terms(str(1.0 / READ_BW), gen_index_scan_size_expr(table_sizes[SUPPLIER], 's'))
            xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
            decrypt_expr  = str((2.0 * DET_DEC) * ResultSetNRows)

            # TODO: currently ignoring sort cost (since it should be relatively cheap)

            return add_terms_l([rtt_expr, idx_seek_expr, idx_scan_expr, xfer_expr, decrypt_expr])

        return (
            add_terms_l([q1_expr(), q2_expr()]),
            set(require_all_det_tbls(['p', 'ps', 'l', 's', 'n'])).intersection(
              set(['p_partkey_det',
                   'ps_partkey_det', 'ps_suppkey_det', 'ps_availqty_det', 'l_quantity_det',
                   'l_partkey_det', 'l_suppkey_det', 's_name_det', 's_address_det',
                   's_suppkey_det', 's_nationkey_det', 'n_nationkey_det', 'n_name_det'])).union(set(
                  ['p_name_swp', 'l_shipdate_ope', 's_name_ope', 'l_pack0_agg'])))

    return [plan1(), plan2(), plan3()]

def get_cost_functions(table_sizes):
    return [
             query1_cost_functions(table_sizes),
             query2_cost_functions(table_sizes),
             query11_cost_functions(table_sizes),
             query14_cost_functions(table_sizes),
             query18_cost_functions(table_sizes),
             query20_cost_functions(table_sizes),
           ]

if __name__ == '__main__':

    TPCH_SCALE = 1.0

    table_sizes = {
        LINEITEM : TPCH_SCALE * 6001215,
        NATION   : 25,
        PART     : TPCH_SCALE * 200000,
        PARTSUPP : TPCH_SCALE * 800000,
        REGION   : 5,
        SUPPLIER : TPCH_SCALE * 10000,
        CUSTOMER : TPCH_SCALE * 150000,
        ORDERS   : TPCH_SCALE * 1500000,
    }

    cost_fcns = get_cost_functions(table_sizes)

    S = 3.0

    # assign each variable an ID from 0 to len(vars) - 1
    a, b = gen_all_variables()

    cnt = 0
    key = {}
    for entry in a:
        assert entry[0] not in key
        key[entry[0]] = cnt
        cnt += 1
    for entry in b:
        assert entry[0] not in key
        key[entry[0]] = cnt
        cnt += 1

    for query_plans, qid in zip(cost_fcns, xrange(len(cost_fcns))):
        for plan, pid in zip(query_plans, xrange(len(query_plans))):
            vname = 'P%d%d' % (qid, pid)
            assert vname not in key
            key[vname] = cnt
            cnt += 1

    def mkZeroRow():
        r = []
        for i in xrange(cnt): r.append(0)
        return r

    def mkOneRow():
        r = []
        for i in xrange(cnt): r.append(1)
        return r

    # constraints
    a_lt = []
    b_lt = []

    a_eq = []
    b_eq = []

    c_lt = []
    c_eq = []

    def matlabify(m):
        if not len(m): return '[]';
        if hasattr(m[0], '__iter__'):
            return '[%s]' % ';'.join([matlabify(x) for x in m])
        return '[%s]' % ','.join([str(x) for x in m])

    # list of ( query_cost_function_expr )
    queries = []
    for query_plans, qid in zip(cost_fcns, xrange(len(cost_fcns))):
        for plan, pid in zip(query_plans, xrange(len(query_plans))):
            vname = 'P%d%d' % (qid, pid)
            queries.append( mult_terms(vname, plan[0]) )

            # each Pij requires some of the variables to be true
            for var in plan[1]:
                # x_ijk >= Pij
                # => Pij - x_ijk <= 0
                a0 = mkZeroRow()
                a0[key[vname]] = 1
                a0[key[var]] = -1
                a_lt.append( a0 )
                b_lt.append( 0  )

        # each query must pick exactly one plan
        # sum Pij = 1
        #a0 = mkZeroRow()
        #for pid in xrange(len(query_plans)):
        #    vname = 'P%d%d' % (qid, pid)
        #    a0[key[vname]] = 1
        #a_eq.append( a0 )
        #b_eq.append( 1  )

        # squares constraint
        c_eq.append(add_terms_l(['P%d%d^2' % (qid, pid) for pid in xrange(len(query_plans))] + ['-1']))


    # if given choice, x_DET/x_OPE must be at least one
    for entry in ORIG_COLUMNS:
        assert entry[1] & DET_FLAG
        if hasAllBits(entry[1], DET_FLAG | OPE_FLAG):
            # x_DET + x_OPE >= 1
            # => -x_DET + -x_OPE <= -1
            #a0 = mkZeroRow()
            #a0[key['%s_det' % entry[0]]] = -1
            #a0[key['%s_ope' % entry[0]]] = -1
            #a_lt.append( a0 )
            #b_lt.append( -1 )

            c_lt.append('-(%s_det^2) - (%s_ope^2) + 1' % (entry[0], entry[0]))

    # space budget constraint
    b_rhs = 0
    a0 = mkZeroRow()
    total_rows = sum(table_sizes.values())
    for tbl, tbl_id in TABLE_PREFIXES:
        nf = table_sizes[tbl_id] / total_rows
        b_rhs += S * get_orig_table_row_size(tbl) * nf
        for entry in gen_variables_for_table(tbl):
            if entry[0] == '1':
                b_rhs -= entry[1] * nf
            else:
                a0[key[entry[0]]] = entry[1] * nf
    a_lt.append( a0    )
    b_lt.append( b_rhs )

    def output_all_vars(fp):
        for entry in a + b:
            print >>fp, '  %s = x(%d);' % (entry[0], key[entry[0]] + 1)
        for query_plans, qid in zip(cost_fcns, xrange(len(cost_fcns))):
            for plan, pid in zip(query_plans, xrange(len(query_plans))):
                vname = 'P%d%d' % (qid, pid)
                print >>fp, '  %s = x(%d);' % (vname, key[vname] + 1)

    fp = open('table_size_test.m', 'w')
    print >>fp, 'function [ size_orig, size_enc ] = cost_function ( x )'

    print >>fp, '  size_orig =', sum([get_orig_table_row_size(tbl) * table_sizes[tbl_id] for tbl, tbl_id in TABLE_PREFIXES]), ';'

    output_all_vars(fp)

    print >>fp, '  size_enc =', add_terms_l([
      mult_terms(
        entry[0],
        str(entry[1] * table_sizes[tbl_id]))
      for tbl, tbl_id in TABLE_PREFIXES \
      for entry in gen_variables_for_table(tbl)]), ';'

    print >>fp, 'end'
    fp.close()

    fp = open('mycon.m', 'w')
    print >>fp, 'function [ c, ceq ] = cost_function ( x )'

    output_all_vars(fp)
    print >>fp, '  c   = %s;' % matlabify(c_lt)
    print >>fp, '  ceq = %s;' % matlabify(c_eq)

    print >>fp, 'end'
    fp.close()

    # emit cost function file (cost_function.m)
    # matlab is index from 1, not zero
    fp = open('cost_function.m', 'w')
    print >>fp, 'function [ cost ] = cost_function ( x )'

    print >>fp, '  bias = @(t) -10000.0 * (t - 0.5).^2 + 2500.0;'
    output_all_vars(fp)
    cost_fcn_expr = add_terms_l(queries)
    print >>fp, '  cost = sum(bias(x)) + %s;' % cost_fcn_expr

    print >>fp, 'end'
    fp.close()

    query_vars = \
        ['P%d%d' % (qid, pid) for query_plans, qid in zip(cost_fcns, xrange(len(cost_fcns))) \
                              for plan, pid in zip(query_plans, xrange(len(query_plans)))]

    # emit the interpretation script
    fp = open('interpret_results.m', 'w')
    print >>fp, 'function [] = interpret_results ( x )'
    print >>fp, '  name = {%s};' % ','.join(["'%s'" % s for s in ([entry[0] for entry in a + b] + query_vars)])
    print >>fp, '  for i=[1:length(x)]'
    print >>fp, "    disp(sprintf('%%s: %%f', %s, %s));" % ('name{i}', 'x(i)')
    print >>fp, '  end'
    print >>fp, 'end'
    fp.close()

    # emit the script to run
    fp = open('opt_problem.m', 'w')

    #print >>fp, 'x0 = %s;' % matlabify(mkOneRow())
    print >>fp, 'x0 = zeros(1, %d);' % len(mkOneRow())
    print >>fp, 'x0(:) = 0.5;' # start everything at 1/2

    print >>fp, 'A = %s;' % matlabify(a_lt)
    print >>fp, 'b = %s;' % matlabify(b_lt)
    print >>fp, 'Aeq = %s;' % matlabify(a_eq)
    print >>fp, 'beq = %s;' % matlabify(b_eq)
    print >>fp, 'lb = %s;' % matlabify(mkZeroRow())
    print >>fp, 'ub = %s;' % matlabify(mkOneRow())
    print >>fp, "opts = optimset('Algorithm', 'active-set');"
    print >>fp, 'x = fmincon(@cost_function,x0,A,b,Aeq,beq,lb,ub,@mycon,opts);'
    print >>fp, '[x, fval, exitflag] = fmincon(@cost_function,x0,A,b,Aeq,beq,lb,ub,@mycon,opts);'
    print >>fp, 'N_TRIES=3;'
    print >>fp, 'i=N_TRIES;'
    print >>fp, 'while exitflag ~= 1 && i > 0'
    print >>fp, '    [x, fval, exitflag] = fmincon(@cost_function,x,A,b,Aeq,beq,lb,ub,@mycon,opts);'
    print >>fp, '    i = i - 1;'
    print >>fp, 'end'
    print >>fp, 'interpret_results(x);'
    print >>fp, 'if exitflag ~= 1'
    print >>fp, "    disp(sprintf('WARNING: Did not converge after %d iterations', N_TRIES));"
    print >>fp, 'end'
    fp.close()

# vim: set sw=4
