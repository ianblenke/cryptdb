#!/bin/sh

make; sudo make install; mysql -u root -pletmein -e "use mysql; drop function
ope_enc;" ; mysql -u root -pletmein -e "drop database db_name;"; mysql -u root
-pletmein -e "create database db_name; use db_name; create table t_name(a int, b int, c int);"; /etc/init.d/mysql restart; sudo /etc/init.d/apparmor teardown; mysql -u root -pletmein -e "use mysql; create function ope_enc returns integer soname 'edb.so';"

