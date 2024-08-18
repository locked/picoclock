mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DWIFI_SSID="<wifi ssid>" -DWIFI_PASSWORD="<wifi password>" -DSERVER_IP="<server ip>" -DSERVER_PORT="<server port>" -DPICO_BOARD=pico_w ..
make -j4
