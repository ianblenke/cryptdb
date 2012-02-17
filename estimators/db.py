import sys

### hacks
sys.path.append('/usr/share/pyshared')
sys.path.append('/usr/lib/pyshared/python2.6')

import MySQLdb

def get_db_conn(db):
    return MySQLdb.connect(unix_socket='/tmp/mysql.sock', user='root', passwd='', db=db)
