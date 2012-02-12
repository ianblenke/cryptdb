#!/usr/bin/env python

from constants import *
from math import log

DET_FLAG = 0x1
OPE_FLAG = 0x1 << 1
SWP_FLAG = 0x1 << 2
AGG_FLAG = 0x1 << 3

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
    ('p_name', DET_FLAG, 55),
    ('p_mfgr', DET_FLAG, 25),
    ('p_brand', DET_FLAG, 10),
    ('p_type', DET_FLAG | SWP_FLAG, 25),
    ('p_size', DET_FLAG, 4),
    ('p_container', DET_FLAG, 10),
    ('p_retailprice', DET_FLAG, 8),
    ('p_comment', DET_FLAG, 23),

    ### PARTSUPP table ###
    ('ps_partkey', DET_FLAG, 4),
    ('ps_suppkey', DET_FLAG, 4),
    ('ps_availqty', DET_FLAG, 4),
    ('ps_supplycost', DET_FLAG | OPE_FLAG, 8),
    ('ps_comment', DET_FLAG, 199),

    ### REGION table ###
    ('r_regionkey', DET_FLAG, 4),
    ('r_name', DET_FLAG, 25),
    ('r_comment', DET_FLAG, 152),

    ## SUPPLIER table ###
    ('s_suppkey', DET_FLAG, 4),
    ('s_name', DET_FLAG | OPE_FLAG, 25),
    ('s_address', DET_FLAG, 40),
    ('s_nationkey', DET_FLAG, 4),
    ('s_phone', DET_FLAG, 15),
    ('s_acctbal', DET_FLAG | OPE_FLAG, 8),
    ('s_comment', DET_FLAG, 101),
]

### generate the y variables ###
VIRTUAL_COLUMNS = [
    ### LINEITEM table ###
    ('l_pack0', AGG_FLAG, 256),

    ### PARTSUPP table ###
    ('ps_supplycost', AGG_FLAG, 256),
]

def hasAllBits(x, mask):
    return (x & mask) == mask

def gen_variables_from_entry(entry):
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

    ForceDet = True
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

### Query 1 ###
def query1_cost_functions(numrows):
    '''
    returns a list of (cost expr, set(required variables))
    '''

    lineitem_vars = gen_variables_for_table('l')
    row_length_expr = add_terms_l([mult_terms(x[0], str(x[1])) for x in lineitem_vars])
    table_size_expr = mult_terms(str(numrows), row_length_expr)

    def plan1():
        '''the naive plan'''

        ProjRecordLength = 34
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
            set(['l_shipdate_ope']))

    def plan2():
        '''the optimized plan'''

        ProjRecordLength = 262
        ResultCardinality = 10
        DataSentBack = ResultCardinality * ProjRecordLength

        NumFilesortPages = (numrows * 258.0) / INNODB_PAGE_SIZE

        SortEstimatePageIOs = \
            NumFilesortPages * log(NumFilesortPages, N_MERGE_BUFFERS)

        SortNumBytesXF = SortEstimatePageIOs * INNODB_PAGE_SIZE

        # formulate expr for row length
        encrypt_expr  = str(OPE_ENC)
        rtt_expr      = str(RTT)
        seek_expr     = str(SEEK)
        seq_scan_expr = mult_terms(str(1.0 / READ_BW), table_size_expr)
        sort_expr     = str((1.0 / ((READ_BW + WRITE_BW) / 2.0)) * SortNumBytesXF)
        agg_expr      = str(numrows * AGG_ADD)
        xfer_expr     = str((1.0 / NETWORK_BW) * DataSentBack)
        decrypt_expr  = str((2.0 * DET_DEC + AGG_DEC) * ResultCardinality)

        return (
            add_terms_l([encrypt_expr, rtt_expr, seek_expr, seq_scan_expr,
                         sort_expr, agg_expr, xfer_expr, decrypt_expr]),
            set(['l_returnflag_ope', 'l_linestatus_ope','l_shipdate_ope', 'l_pack0_agg']))

    return [plan1(), plan2()]

def get_cost_functions():
    return [ query1_cost_functions(6000000) ]



if __name__ == '__main__':

    cost_fcns = get_cost_functions()

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
        a0 = mkZeroRow()
        for pid in xrange(len(query_plans)):
            vname = 'P%d%d' % (qid, pid)
            a0[key[vname]] = 1
        a_eq.append( a0 )
        b_eq.append( 1  )

    # if given choice, x_DET/x_OPE must be at least one
    for entry in ORIG_COLUMNS:
        assert entry[1] & DET_FLAG
        if hasAllBits(entry[1], DET_FLAG | OPE_FLAG):
            # x_DET + x_OPE >= 1
            # => -x_DET + -x_OPE <= -1
            a0 = mkZeroRow()
            a0[key['%s_det' % entry[0]]] = -1
            a0[key['%s_ope' % entry[0]]] = -1
            a_lt.append( a0 )
            b_lt.append( -1 )

    # emit cost function file (cost_function.m)
    # matlab is index from 1, not zero
    fp = open('cost_function.m', 'w')
    print >>fp, 'function [ cost ] = cost_function ( x )'
    for entry in a + b:
        print >>fp, '  %s = x(%d);' % (entry[0], key[entry[0]] + 1)
    for query_plans, qid in zip(cost_fcns, xrange(len(cost_fcns))):
        for plan, pid in zip(query_plans, xrange(len(query_plans))):
            vname = 'P%d%d' % (qid, pid)
            print >>fp, '  %s = x(%d);' % (vname, key[vname] + 1)
    cost_fcn_expr = add_terms_l(queries)
    print >>fp, '  cost = %s;' % cost_fcn_expr
    print >>fp, 'end'
    fp.close()

    query_vars = []
    for query_plans, qid in zip(cost_fcns, xrange(len(cost_fcns))):
        for plan, pid in zip(query_plans, xrange(len(query_plans))):
            vname = 'P%d%d' % (qid, pid)
            query_vars.append(vname)

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
    print >>fp, 'x0 = %s;' % matlabify(mkZeroRow())
    print >>fp, 'A = %s;' % matlabify(a_lt)
    print >>fp, 'b = %s;' % matlabify(b_lt)
    print >>fp, 'Aeq = %s;' % matlabify(a_eq)
    print >>fp, 'beq = %s;' % matlabify(b_eq)
    print >>fp, 'lb = %s;' % matlabify(mkZeroRow())
    print >>fp, 'ub = %s;' % matlabify(mkOneRow())
    print >>fp, "opts = optimset('Algorithm', 'active-set');"
    print >>fp, 'x = fmincon(@cost_function,x0,A,b,Aeq,beq,lb,ub,[],opts);'
    print >>fp, 'interpret_results(x);'
    fp.close()
