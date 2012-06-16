// WARNING: This is an auto-generated file
#include "db_config.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <edb/ConnectNew.hh>
#include <util/util.hh>

static std::string
join(const std::vector<std::string>& tokens, const std::string &sep) {
  std::ostringstream s;
  for (size_t i = 0; i < tokens.size(); i++) {
    s << tokens[i];
    if (i != tokens.size() - 1) s << sep;
  }
  return s.str();
}

static void query_0(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select l_returnflag, l_linestatus, fast_sum(l_quantity) as sum_qty, fast_sum(l_extendedprice) as sum_base_price, fast_sum((l_extendedprice) * ((100) - (l_discount))) as sum_disc_price, fast_sum(((l_extendedprice) * ((100) - (l_discount))) * ((100) + (l_tax))) as sum_charge, fast_sum(l_quantity) / count(*) as avg_qty, fast_sum(l_extendedprice) / count(*) as avg_price, fast_sum(l_discount) / count(*) as avg_disc, count(*) as count_order from " LINEITEM_NAME " where (l_shipdate) <= (date '1998-01-01') group by l_returnflag, l_linestatus order by l_returnflag ASC, l_linestatus ASC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << "|";
    std::cout << row[3].data;
    std::cout << "|";
    std::cout << row[4].data;
    std::cout << "|";
    std::cout << row[5].data;
    std::cout << "|";
    std::cout << row[6].data;
    std::cout << "|";
    std::cout << row[7].data;
    std::cout << "|";
    std::cout << row[8].data;
    std::cout << "|";
    std::cout << row[9].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_1(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select s_acctbal, s_name, n_name, p_partkey, p_mfgr, s_address, s_phone, s_comment from " PART_NAME ", " SUPPLIER_NAME ", " PARTSUPP_NAME ", " NATION_NAME ", " REGION_NAME " where ((((((((p_partkey) = (ps_partkey)) and ((s_suppkey) = (ps_suppkey))) and ((p_size) = (36))) and ((p_type) like ('%STEEL'))) and ((s_nationkey) = (n_nationkey))) and ((n_regionkey) = (r_regionkey))) and ((r_name) = ('ASIA'))) and ((ps_supplycost) = ((select min(ps_supplycost) from " PARTSUPP_NAME ", " SUPPLIER_NAME ", " NATION_NAME ", " REGION_NAME " where (((((p_partkey) = (ps_partkey)) and ((s_suppkey) = (ps_suppkey))) and ((s_nationkey) = (n_nationkey))) and ((n_regionkey) = (r_regionkey))) and ((r_name) = ('ASIA'))))) order by s_acctbal DESC, n_name ASC, s_name ASC, p_partkey ASC limit 100", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << "|";
    std::cout << row[3].data;
    std::cout << "|";
    std::cout << row[4].data;
    std::cout << "|";
    std::cout << row[5].data;
    std::cout << "|";
    std::cout << row[6].data;
    std::cout << "|";
    std::cout << row[7].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_2(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select l_orderkey, fast_sum((l_extendedprice) * ((100) - (l_discount))) as revenue, o_orderdate, o_shippriority from " CUSTOMER_NAME ", " ORDERS_NAME ", " LINEITEM_NAME " where (((((c_mktsegment) = ('FURNITURE')) and ((c_custkey) = (o_custkey))) and ((l_orderkey) = (o_orderkey))) and ((o_orderdate) < (date '1995-03-12'))) and ((l_shipdate) > (date '1995-03-12')) group by l_orderkey, o_orderdate, o_shippriority order by revenue DESC, o_orderdate ASC limit 10", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << "|";
    std::cout << row[3].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_3(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select o_orderpriority, count(*) as order_count from " ORDERS_NAME " where (((o_orderdate) >= (date '1994-11-01')) and ((o_orderdate) < ((date '1994-11-01') + (interval '3' MONTH)))) and (exists ( (select * from " LINEITEM_NAME " where ((l_orderkey) = (o_orderkey)) and ((l_commitdate) < (l_receiptdate))) )) group by o_orderpriority order by o_orderpriority ASC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_4(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select n_name, fast_sum((l_extendedprice) * ((100) - (l_discount))) as revenue from " CUSTOMER_NAME ", " ORDERS_NAME ", " LINEITEM_NAME ", " SUPPLIER_NAME ", " NATION_NAME ", " REGION_NAME " where (((((((((c_custkey) = (o_custkey)) and ((l_orderkey) = (o_orderkey))) and ((l_suppkey) = (s_suppkey))) and ((c_nationkey) = (s_nationkey))) and ((s_nationkey) = (n_nationkey))) and ((n_regionkey) = (r_regionkey))) and ((r_name) = ('MIDDLE EAST'))) and ((o_orderdate) >= (date '1993-01-01'))) and ((o_orderdate) < ((date '1993-01-01') + (interval '1' YEAR))) group by n_name order by revenue DESC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_5(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select fast_sum((l_extendedprice) * (l_discount)) as revenue from " LINEITEM_NAME " where ((((l_shipdate) >= (date '1993-01-01')) and ((l_shipdate) < ((date '1993-01-01') + (interval '1' YEAR)))) and (((l_discount) >= ((5) - (1))) and ((l_discount) <= ((5) + (1))))) and ((l_quantity) < (2500))", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_6(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select supp_nation, cust_nation, l_year, fast_sum(volume) as revenue from ( select n1.n_name as supp_nation, n2.n_name as cust_nation, extract(YEAR from l_shipdate) as l_year, (l_extendedprice) * ((100) - (l_discount)) as volume from " SUPPLIER_NAME ", " LINEITEM_NAME ", " ORDERS_NAME ", " CUSTOMER_NAME ", " NATION_NAME " n1, " NATION_NAME " n2 where (((((((s_suppkey) = (l_suppkey)) and ((o_orderkey) = (l_orderkey))) and ((c_custkey) = (o_custkey))) and ((s_nationkey) = (n1.n_nationkey))) and ((c_nationkey) = (n2.n_nationkey))) and ((((n1.n_name) = ('SAUDI ARABIA')) and ((n2.n_name) = ('ARGENTINA'))) or (((n1.n_name) = ('ARGENTINA')) and ((n2.n_name) = ('SAUDI ARABIA'))))) and (((l_shipdate) >= (date '1995-01-01')) and ((l_shipdate) <= (date '1996-12-31'))) ) as shipping group by supp_nation, cust_nation, l_year order by supp_nation ASC, cust_nation ASC, l_year ASC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << "|";
    std::cout << row[3].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_7(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select o_year, (fast_sum(case when (nation) = ('ARGENTINA') then volume else 0 end)) / (fast_sum(volume)) as mkt_share from ( select extract(YEAR from o_orderdate) as o_year, (l_extendedprice) * ((100) - (l_discount)) as volume, n2.n_name as nation from " PART_NAME ", " SUPPLIER_NAME ", " LINEITEM_NAME ", " ORDERS_NAME ", " CUSTOMER_NAME ", " NATION_NAME " n1, " NATION_NAME " n2, " REGION_NAME " where ((((((((((p_partkey) = (l_partkey)) and ((s_suppkey) = (l_suppkey))) and ((l_orderkey) = (o_orderkey))) and ((o_custkey) = (c_custkey))) and ((c_nationkey) = (n1.n_nationkey))) and ((n1.n_regionkey) = (r_regionkey))) and ((r_name) = ('AMERICA'))) and ((s_nationkey) = (n2.n_nationkey))) and (((o_orderdate) >= (date '1995-01-01')) and ((o_orderdate) <= (date '1996-12-31')))) and ((p_type) = ('PROMO ANODIZED BRASS')) ) as all_nations group by o_year order by o_year ASC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_8(ConnectNew& conn) {
  conn.execute("SET enable_indexscan TO FALSE");
  conn.execute("SET enable_nestloop TO FALSE");
  DBResultNew* dbres;
  conn.execute("select " NATION_NAME ", o_year, fast_sum(amount) as sum_profit from ( select n_name as " NATION_NAME ", extract(YEAR from o_orderdate) as o_year, ((l_extendedprice) * ((100) - (l_discount))) - ((ps_supplycost) * (l_quantity)) as amount from " PART_NAME ", " SUPPLIER_NAME ", " LINEITEM_NAME ", " PARTSUPP_NAME ", " ORDERS_NAME ", " NATION_NAME " where (((((((s_suppkey) = (l_suppkey)) and ((ps_suppkey) = (l_suppkey))) and ((ps_partkey) = (l_partkey))) and ((p_partkey) = (l_partkey))) and ((o_orderkey) = (l_orderkey))) and ((s_nationkey) = (n_nationkey))) and ((p_name) like ('%sky%')) ) as profit group by " NATION_NAME ", o_year order by " NATION_NAME " ASC, o_year DESC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  conn.execute("SET enable_indexscan TO TRUE");
  conn.execute("SET enable_nestloop TO TRUE");
}
static void query_9(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select c_custkey, c_name, fast_sum((l_extendedprice) * ((100) - (l_discount))) as revenue, c_acctbal, n_name, c_address, c_phone, c_comment from " CUSTOMER_NAME ", " ORDERS_NAME ", " LINEITEM_NAME ", " NATION_NAME " where ((((((c_custkey) = (o_custkey)) and ((l_orderkey) = (o_orderkey))) and ((o_orderdate) >= (date '1994-08-01'))) and ((o_orderdate) < ((date '1994-08-01') + (interval '3' MONTH)))) and ((l_returnflag) = ('R'))) and ((c_nationkey) = (n_nationkey)) group by c_custkey, c_name, c_acctbal, c_phone, n_name, c_address, c_comment order by revenue DESC limit 20", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << "|";
    std::cout << row[3].data;
    std::cout << "|";
    std::cout << row[4].data;
    std::cout << "|";
    std::cout << row[5].data;
    std::cout << "|";
    std::cout << row[6].data;
    std::cout << "|";
    std::cout << row[7].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_10(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select ps_partkey, fast_sum((ps_supplycost) * (ps_availqty)) as value from " PARTSUPP_NAME ", " SUPPLIER_NAME ", " NATION_NAME " where (((ps_suppkey) = (s_suppkey)) and ((s_nationkey) = (n_nationkey))) and ((n_name) = ('ARGENTINA')) group by ps_partkey having (fast_sum((ps_supplycost) * (ps_availqty))) > ((select (fast_sum((ps_supplycost) * (ps_availqty))) * (1.0E-4) from " PARTSUPP_NAME ", " SUPPLIER_NAME ", " NATION_NAME " where (((ps_suppkey) = (s_suppkey)) and ((s_nationkey) = (n_nationkey))) and ((n_name) = ('ARGENTINA')))) order by value DESC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_11(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select l_shipmode, fast_sum(case when ((o_orderpriority) = ('1-URGENT')) or ((o_orderpriority) = ('2-HIGH')) then 1 else 0 end) as high_line_count, fast_sum(case when ((o_orderpriority) <> ('1-URGENT')) and ((o_orderpriority) <> ('2-HIGH')) then 1 else 0 end) as low_line_count from " ORDERS_NAME ", " LINEITEM_NAME " where ((((((o_orderkey) = (l_orderkey)) and (l_shipmode in ( 'AIR', 'FOB' ))) and ((l_commitdate) < (l_receiptdate))) and ((l_shipdate) < (l_commitdate))) and ((l_receiptdate) >= (date '1993-01-01'))) and ((l_receiptdate) < ((date '1993-01-01') + (interval '1' YEAR))) group by l_shipmode order by l_shipmode ASC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_12(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select ((100.0) * (fast_sum(case when (p_type) like ('PROMO%') then (l_extendedprice) * ((100) - (l_discount)) else 0 end))) / (fast_sum((l_extendedprice) * ((100) - (l_discount)))) as promo_revenue from " LINEITEM_NAME ", " PART_NAME " where (((l_partkey) = (p_partkey)) and ((l_shipdate) >= (date '1996-07-01'))) and ((l_shipdate) < ((date '1996-07-01') + (interval '1' MONTH)))", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_13(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select l_partkey, (0.2) * (avg(l_quantity)) from " LINEITEM_NAME ", " PART_NAME " where (((p_partkey) = (l_partkey)) and ((p_brand) = ('Brand#45'))) and ((p_container) = ('LG BOX')) group by l_partkey", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_14(ConnectNew& conn) {
  // NOTE: we use a custom implementation here, because postgres
  // handles this version better.
  std::vector<std::string> l_orderkeys;
  {
    DBResultNew* dbres;
    const char *q =
        "select "
        "    l_orderkey "
        "from "
        "    " LINEITEM_NAME " "
        "group by "
        "    l_orderkey having "
        "        sum(l_quantity) > (315 * 100)";
    conn.execute(q, dbres);
    ResType res = dbres->unpack();
    assert(res.ok);
    l_orderkeys.reserve(res.rows.size());
    for (auto &row : res.rows) {
      l_orderkeys.push_back(row[0].data);
    }
    delete dbres;
  }
  if (l_orderkeys.empty()) return;
  DBResultNew* dbres;
  std::ostringstream buf;
  buf << "select c_name, c_custkey, o_orderkey, o_orderdate, o_totalprice, fast_sum(l_quantity) from " CUSTOMER_NAME ", " ORDERS_NAME ", " LINEITEM_NAME " where ((o_orderkey in (" << join(l_orderkeys, ",") <<" )) and ((c_custkey) = (o_custkey))) and ((o_orderkey) = (l_orderkey)) group by c_name, c_custkey, o_orderkey, o_orderdate, o_totalprice order by o_totalprice DESC, o_orderdate ASC limit 100";
  conn.execute(buf.str(), dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << "|";
    std::cout << row[3].data;
    std::cout << "|";
    std::cout << row[4].data;
    std::cout << "|";
    std::cout << row[5].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_15(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select fast_sum((l_extendedprice) * ((100) - (l_discount))) as revenue from " LINEITEM_NAME ", " PART_NAME " where ((((((((((p_partkey) = (l_partkey)) and ((p_brand) = ('Brand#23'))) and (p_container in ( 'SM CASE', 'SM BOX', 'SM PACK', 'SM PKG' ))) and ((l_quantity) >= (1000))) and ((l_quantity) <= ((1000) + (1000)))) and (((p_size) >= (1)) and ((p_size) <= (5)))) and (l_shipmode in ( 'AIR', 'AIR REG' ))) and ((l_shipinstruct) = ('DELIVER IN PERSON'))) or (((((((((p_partkey) = (l_partkey)) and ((p_brand) = ('Brand#32'))) and (p_container in ( 'MED BAG', 'MED BOX', 'MED PKG', 'MED PACK' ))) and ((l_quantity) >= (2000))) and ((l_quantity) <= ((2000) + (1000)))) and (((p_size) >= (1)) and ((p_size) <= (10)))) and (l_shipmode in ( 'AIR', 'AIR REG' ))) and ((l_shipinstruct) = ('DELIVER IN PERSON')))) or (((((((((p_partkey) = (l_partkey)) and ((p_brand) = ('Brand#34'))) and (p_container in ( 'LG CASE', 'LG BOX', 'LG PACK', 'LG PKG' ))) and ((l_quantity) >= (2000))) and ((l_quantity) <= ((2000) + (1000)))) and (((p_size) >= (1)) and ((p_size) <= (15)))) and (l_shipmode in ( 'AIR', 'AIR REG' ))) and ((l_shipinstruct) = ('DELIVER IN PERSON')))", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
static void query_16(ConnectNew& conn) {
  DBResultNew* dbres;
  conn.execute("select cntrycode, count(*) as numcust, fast_sum(abs(abal)) as totacctbal from ( select substring(c_phone from 1 for 2) as cntrycode, c_acctbal as abal from " CUSTOMER_NAME " where ((substring(c_phone from 1 for 2) in ( '41', '26', '36', '27', '38', '37', '22' )) and ((c_acctbal) > ((select avg(c_acctbal) from " CUSTOMER_NAME " where (substring(c_phone from 1 for 2) in ( '41', '26', '36', '27', '38', '37', '22' )))))) and (not ( exists ( (select * from " ORDERS_NAME " where (o_custkey) = (c_custkey)) ) )) ) as custsale group by cntrycode order by cntrycode ASC", dbres);
  ResType res = dbres->unpack();
  assert(res.ok);
  for (auto &row : res.rows) {
    std::cout << row[0].data;
    std::cout << "|";
    std::cout << row[1].data;
    std::cout << "|";
    std::cout << row[2].data;
    std::cout << std::endl;
  }
  std::cout << "(" << res.rows.size() << " rows)" << std::endl;
  delete dbres;
}
int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "[Usage]: " << argv[0] << " [query num]" << std::endl;
    return 1;
  }
  int q = atoi(argv[1]);
  PGConnect pg(DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PORT, true);
  switch (q) {
    case 0: query_0(pg); break;
    case 1: query_1(pg); break;
    case 2: query_2(pg); break;
    case 3: query_3(pg); break;
    case 4: query_4(pg); break;
    case 5: query_5(pg); break;
    case 6: query_6(pg); break;
    case 7: query_7(pg); break;
    case 8: query_8(pg); break;
    case 9: query_9(pg); break;
    case 10: query_10(pg); break;
    case 11: query_11(pg); break;
    case 12: query_12(pg); break;
    case 13: query_13(pg); break;
    case 14: query_14(pg); break;
    case 15: query_15(pg); break;
    case 16: query_16(pg); break;
    default: assert(false);
  }
  return 0;
}
