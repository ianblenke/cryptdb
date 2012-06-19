#define DB_HOSTNAME "vise4.csail.mit.edu"
#define DB_USERNAME "stephentu"
#define DB_PASSWORD "letmein"

#define DB_DATABASE "tpch_10_00"
//#define DB_DATABASE "tpch_0_05"

#define DB_PORT     5432

#define CRYPTO_USE_OLD_OPE false

// this is not auto generated

//#define LINEITEM_ENC_NAME "lineitem_enc_cryptdb_greedy"
//#define CUSTOMER_ENC_NAME "customer_enc_cryptdb_greedy"
//#define PARTSUPP_ENC_NAME "partsupp_enc_cryptdb_greedy"
//#define ORDERS_ENC_NAME   "orders_enc_cryptdb_greedy"

#define LINEITEM_ENC_NAME "lineitem_enc_rowid"
#define CUSTOMER_ENC_NAME "customer_enc_rowid"
#define PARTSUPP_ENC_NAME "partsupp_enc_rowid"
#define ORDERS_ENC_NAME   "orders_enc"

#define PART_ENC_NAME     "part_enc"
#define REGION_ENC_NAME   "region_enc"
#define SUPPLIER_ENC_NAME "supplier_enc"
#define NATION_ENC_NAME   "nation_enc"
