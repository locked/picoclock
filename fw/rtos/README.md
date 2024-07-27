mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DWIFI_SSID="<wifi ssid>" -DWIFI_PASSWORD="<wifi password>" -DSERVER_IP="x.x.x.x" -DSERVER_PORT="<server port>" -DPICO_BOARD=pico_w ..
make -j4
