create table lineitem_enc_opt_all (

  -- field 8
  `l_returnflag_DET` bigint(20) unsigned DEFAULT NULL,
  `l_returnflag_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_returnflag_SALT` bigint(20) unsigned DEFAULT NULL,

  -- field 9
  `l_linestatus_DET` bigint(20) unsigned DEFAULT NULL,
  `l_linestatus_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_linestatus_SALT` bigint(20) unsigned DEFAULT NULL,

  -- field 10 
  `l_shipdate_year_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_shipdate_month_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_shipdate_day_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_shipdate_SALT` bigint(20) unsigned DEFAULT NULL,

  -- bit-packed AGGs
  `l_bitpacked_AGG` varbinary(256) DEFAULT NULL

) ENGINE=InnoDB;

