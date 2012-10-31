CREATE TABLE region_enc_cryptdb_opt (

  r_regionkey_DET bigint  NOT NULL,

  r_name_DET bytea NOT NULL,

  r_comment_DET bytea /* could be RAND */

  --PRIMARY KEY (r_regionkey_DET)
) ;

