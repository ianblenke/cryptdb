make clean; make; sudo make install; mysql -u frank -ppasswd -e "use mysql; drop function ope_enc;" ; sudo service mysql restart; sudo /etc/init.d/apparmor teardown; mysql -u frank -ppasswd -e "use mysql; create function ope_enc returns integer soname 'edb.so';"

