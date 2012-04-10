CREATE TABLE region_enc (

  r_regionkey_DET bigint  NOT NULL,

  r_name_DET bytea NOT NULL,

  r_comment_DET bytea,

  PRIMARY KEY (r_regionkey_DET)
) ;

