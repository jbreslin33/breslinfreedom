echo ---------------------------- build linux---------------------
cmake . -DRUN_IN_PLACE=TRUE
make 

echo ----------------------------build windows -----------
sudo apt-get install mingw-w64
