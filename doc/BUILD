Packages needed for Ubuntu:

  % apt-get install liblua5.1-0-dev libntl-dev libmysqlclient-dev \
                    libssl-dev libbsd-dev libevent-dev libglib2.0-dev \
                    libtbb-dev

You will need to install mysql-proxy from source, because
versions prior to 0.9.0 have various bugs that break CryptDB:

  % bzr branch lp:mysql-proxy
  % cd mysql-proxy
  % sh ./autogen.sh
  % ./configure --enable-maintainer-mode --with-lua=lua5.1
  % make
  % make install

If your build of mysql-proxy fails with an error about g_debug_hexdump
being undefined, try applying the following patch, and then re-doing
the above steps starting from autogen.sh:

  % patch -p0 <<"EOM"
=== modified file 'src/Makefile.am'
--- src/Makefile.am 2010-10-28 11:58:51 +0000
+++ src/Makefile.am 2012-01-10 15:49:27 +0000
@@ -23,7 +23,7 @@
 
 BUILD_CPPFLAGS = $(LUA_CFLAGS) $(MYSQL_CFLAGS) $(GLIB_CFLAGS) $(GMODULE_CFLAGS) $(GTHREAD_CFLAGS)
 BUILD_CFLAGS   = -DLUAEXTDIR="\"$(luaextdir)\""  -DPLUGINDIR="\"$(plugindir)\""
-BUILD_LDADD    = $(GLIB_LIBS) $(GMODULE_LIBS) libmysql-chassis.la $(GTHREAD_LIBS) libmysql-proxy.la
+BUILD_LDADD    = $(GLIB_LIBS) $(GMODULE_LIBS) libmysql-chassis.la libmysql-chassis-glibext.la $(GTHREAD_LIBS) libmysql-proxy.la
 
 BUILT_SOURCES =
 if USE_WRAPPER_SCRIPT
EOM

You will need g++ version 4.6 or higher to build CryptDB.  Ubuntu Oneiric
(11.10) includes g++ 4.6.1.  For earlier versions of Ubuntu, consider:

  % apt-add-repository ppa:ubuntu-toolchain-r/test
  % apt-get update
  % apt-get install gcc-4.6 g++-4.6

You will need to build mysql-5.5.14, because we use its parser in our
newer implementation:

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
for MYSRC.

------------------------------------------------------------------

To build CryptDB and install the UDF module for MySQL, run:

  % make
  % sudo make install

---------------------------------------------------------------

To see how to run CryptDB, see README.

