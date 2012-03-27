CREATE TABLE region_enc (

  `r_regionkey_DET` integer unsigned NOT NULL,

  `r_name_DET` binary(25) NOT NULL,

  `r_comment_DET` varbinary(152),

  PRIMARY KEY (r_regionkey_DET)
) Engine=InnoDB;

