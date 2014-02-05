Build Instructions for mOPE:

Packages needed for Ubuntu:

  % apt-get install liblua5.1-0-dev libntl-dev libmysqlclient-dev \
                    libssl-dev libbsd-dev libevent-dev libglib2.0-dev \
                    libgmp-dev

You will need g++ version 4.6 or higher to build mOPE.  Ubuntu Oneiric
(11.10) includes g++ 4.6.1.  For earlier versions of Ubuntu, consider:

  % apt-add-repository ppa:ubuntu-toolchain-r/test
  % apt-get update
  % apt-get install gcc-4.6 g++-4.6

You will need to build mysql-5.5.14, which our mOPE server interfaces with:

  % wget http://es.csail.mit.edu/mysql-5.5.14.tar.gz
  % tar zxf mysql-5.5.14.tar.gz
  % cp -R .../cryptdb/parser/mysql_mods/* mysql-5.5.14/
  % rm mysql-5.5.14/sql/sql_yacc.{cc,h}
  % cd mysql-5.5.14
  % mkdir build
  % cd build
  % cmake -DWITH_EMBEDDED_SERVER=ON ..
  % make

Now, copy conf/config.mk.sample to conf/config.mk, and adjust the path
for MYSRC and MYSQL_PLUGIN_DIR.

------------------------------------------------------------------

util/ope_conf.hh contains all of the configurable parameters for mOPE. Set
appropriately before build.

To build mOPE, run:

  % make
  % sudo make install

This also copies the UDF library (edb.so) to the plugin directory specified in
conf/config.mk. Then, the SQL command to create a UDF function is of form:

  % CREATE FUNCTION func_name RETURNS return_type SONAME 'edb.so';

When using a MySQL database, make sure to install all UDFs needed before running
(see README for more details on UDF functionality).

run.sh is a script template for building and installing the UDFs. Modify as
appropriate.

---------------------------------------------------------------

To see how to run mOPE, see README.

