CREATE TABLE region_enc (

  `r_regionkey_DET` integer unsigned DEFAULT NULL,

  `r_name_DET` binary(25) DEFAULT NULL,

  `r_comment_DET` varbinary(152) DEFAULT NULL,

  PRIMARY KEY (r_regionkey_DET)
) Engine=InnoDB;

