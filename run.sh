#!/bin/sh

make; sudo make install; mysql -u root -pletmein -e "use mysql; drop function ope_enc;" ; mysql -u root -pletmein -e "drop database cryptdb;"; mysql -u root -pletmein -e "create database cryptdb; use cryptdb; create table emp(a int, b int, c int);"; /etc/init.d/mysql restart; sudo /etc/init.d/apparmor teardown; mysql -u root -pletmein -e "use mysql; create function ope_enc returns integer soname 'edb.so';"

