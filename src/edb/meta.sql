-- Copyright 2011 M.I.T., all rights reserved.

-- Persistent representation of EDBProxy meta information

DROP TABLE IF EXISTS column_info;
DROP TABLE IF EXISTS index_info;
DROP TABLE IF EXISTS table_info;

-- Persistent representation for TabledMetadata in edb/util.h
--
CREATE TABLE table_info
   ( id bigint NOT NULL auto_increment PRIMARY KEY
   , name varchar(64) NOT NULL
   , anon_name varchar(64) NOT NULL
   , salt_name varchar(4096) NOT NULL
   , auto_inc_field varchar(64)
   , auto_inc_value bigint

   , UNIQUE INDEX idx_table_name( name )
);

-- Persistent representation for FieldMetadata in edb/util.h
--
CREATE TABLE column_info
   ( id bigint NOT NULL auto_increment PRIMARY KEY
   , table_id bigint NOT NULL
   , field_type int NOT NULL
   , mysql_field_type int NOT NULL
   , name varchar(64) NOT NULL
   , anon_det_name varchar(64)
   , anon_ope_name varchar(64)
   , anon_agg_name varchar(64)
   , anon_swp_name varchar(64)
   , salt_name varchar(4096)
   , is_encrypted tinyint NOT NULL
   , can_be_null tinyint NOT NULL
   , has_ope tinyint NOT NULL
   , has_agg tinyint NOT NULL
   , has_search tinyint NOT NULL
   , has_salt tinyint NOT NULL
   , ope_used tinyint NOT NULL
   , agg_used tinyint NOT NULL
   , search_used tinyint NOT NULL
   , update_set_performed tinyint NOT NULL
   , sec_level_ope enum
         ( 'INVALID'
         , 'PLAIN'
         , 'PLAIN_DET'
         , 'DETJOIN'
         , 'DET'
         , 'SEMANTIC_DET'
         , 'PLAIN_OPE'
         , 'OPEJOIN'
         , 'OPE'
         , 'SEMANTIC_OPE'
         , 'PLAIN_AGG'
         , 'SEMANTIC_AGG'
         , 'PLAIN_SWP'
         , 'SWP'
         , 'SEMANTIC_VAL'
         , 'SECLEVEL_LAST'
         ) NOT NULL DEFAULT 'INVALID'
   , sec_level_det enum
         ( 'INVALID'
         , 'PLAIN'
         , 'PLAIN_DET'
         , 'DETJOIN'
         , 'DET'
         , 'SEMANTIC_DET'
         , 'PLAIN_OPE'
         , 'OPEJOIN'
         , 'OPE'
         , 'SEMANTIC_OPE'
         , 'PLAIN_AGG'
         , 'SEMANTIC_AGG'
         , 'PLAIN_SWP'
         , 'SWP'
         , 'SEMANTIC_VAL'
         , 'SECLEVEL_LAST'
         ) NOT NULL DEFAULT 'INVALID'

   , INDEX idx_column_name( name )
   , FOREIGN KEY( table_id ) REFERENCES table_info( id ) ON DELETE CASCADE
);

-- Persistent representation for IndexMetadata in edb/util.h
--
CREATE TABLE index_info
   ( id bigint NOT NULL auto_increment PRIMARY KEY
   , table_id bigint NOT NULL
   , name varchar(64) NOT NULL
   , anon_name varchar(64) NOT NULL

   , FOREIGN KEY( table_id ) REFERENCES table_info( id ) ON DELETE CASCADE
);