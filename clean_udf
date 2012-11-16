cd ~/ope/cryptdb/ ; \
make;\
sudo make install;\
mysql -u root -pletmein -h 127.0.0.1 cryptdb -e "drop function udftransform;";\
mysql -u root -pletmein -h 127.0.0.1 cryptdb -e "drop function set_udftransform;";\
/etc/init.d/mysql stop;\
sudo /etc/init.d/apparmor teardown;
