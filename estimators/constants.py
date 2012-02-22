# ALL TIME UNITS IN SECONDS
# ALL SIZE UNITS IN BYTES

RTT  = 1.0 / 1000.0
SEEK = 4.167 / 1000.0

# BYTES/SEC
READ_BW  = 300.0 * (2 ** 20)
WRITE_BW = 100.0 * (2 ** 20)

# BYTES/SEC
NETWORK_BW = 10.0 * (2 ** 20)

# data from estimators/crypto (data in ms)
"""
det_encrypt           : 0.0151060000
det_decrypt           : 0.0172640000
ope_encrypt           : 13.3586000000
ope_decrypt           : 9.4752400000
agg_encrypt           : 9.9578400000
agg_decrypt           : 0.6981700000
agg_add               : 0.0052300000
swp_encrypt_for_query : 0.0037320000
swp_search            : 0.0035200000
"""

# CRYPTO UNITS IN SEC/OPERATION
DET_ENC    = 0.0151  / 1000.0
DET_DEC    = 0.0173  / 1000.0
OPE_ENC    = 13.359  / 1000.0
OPE_DEC    = 9.475   / 1000.0
AGG_DEC    = 0.6982  / 1000.0
AGG_ADD    = 0.00523 / 1000.0
SWP_ENC    = 0.00373 / 1000.0
SWP_SEARCH = 0.00352 / 1000.0

INNODB_PAGE_SIZE = 16 * (2 ** 10)
N_MERGE_BUFFERS  = 7
