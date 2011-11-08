create table lineitem_enc_opt_compact (

  -- field 4
  `l_quantity_AGG` varbinary(256) DEFAULT NULL,
  `l_quantity_SALT` bigint(20) unsigned DEFAULT NULL,

  -- field 5
  `l_extendedprice_AGG` varbinary(256) DEFAULT NULL,
  `l_extendedprice_SALT` bigint(20) unsigned DEFAULT NULL,

  -- field 6
  `l_discount_AGG` varbinary(256) DEFAULT NULL,
  `l_discount_SALT` bigint(20) unsigned DEFAULT NULL,

  -- field 7
  `l_tax_AGG` varbinary(256) DEFAULT NULL,
  `l_tax_SALT` bigint(20) unsigned DEFAULT NULL,

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

  -- field 16 (l_disc_price)
  `l_disc_price_AGG` varbinary(256) DEFAULT NULL,
  `l_disc_price_SALT` bigint(20) unsigned DEFAULT NULL,

  -- field 17 (l_charge)
  `l_charge_AGG` varbinary(256) DEFAULT NULL,
  `l_charge_SALT` bigint(20) unsigned DEFAULT NULL

) ENGINE=InnoDB;
