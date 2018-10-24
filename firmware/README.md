# RC car firmware

## Build and flash
First, you need to get RTOS sources and toolchain for building:
1. Toolchain: [https://github.com/espressif/ESP8266_RTOS_SDK#get-toolchain](https://github.com/espressif/ESP8266_RTOS_SDK#get-toolchain)
2. RTOS SDK v3.0: [https://github.com/espressif/ESP8266_RTOS_SDK/tree/release/v3.0](https://github.com/espressif/ESP8266_RTOS_SDK/tree/release/v3.0)
3. Additionaly you may need to have `esptool` python library. You can install it with `pip`. Also it's important to use python 2.7, not sure but 3.* didn't work for me.

Second, you have to set up env variables to build project:
1. `$IDF_PATH` must contain path to RTOS SDK root
2. `$PATH` must be appended with toolchain `bin` directory

After that you need to perform `make menuconfig` and set port associated with your ESP microcontroller, and perform `make flash && make monitor` to build everything, flash build to the ESP and launch monitor to have access to the logs.

## How to use
Current implementation should work something like this:
1. Power on ESP
2. Connect to `RC CAR` access point without password
3. Establish TCP connection to `$GATEWAY_IP$:359` (it's likely `192.168.91.1:359` if you don't change anything) and invoke remote `connect` command with
    ```json
    {"rpcversion": "2.0", "method": "connect", "params": {"ssid": "$SSID", "pass": "$PASSWORD"}}^D
    ```
    `^D` is `EOT`, for instance, if you use `netcat` you can simply write it with next sequence `^V^D`
4. After that ESP will connect to the access point you've provided and will wait for UDP 4-byte datagrams on `$OBTAINED_IP$:359`. For now it's not possible to get info about STA connection via JSON RPC so you can find IP in logs or in your access point dashboard.

Each UDP message must contain 4 bytes and it will be interpreted like [forward, backward, left, right], if multiple mutually exclusive values is non-zero `forward` and `left` will have more priority.

## Diving into
In general, firmware contains multiple important parts:
1. Wifi system event loop
2. Access point server that handles JSON RPC requests. Main purpose: configuring microcontroller and getting info about connect, server statuses and so on
3. Station UDP server that handles datagrams to control car

To begin code investigation you have to take a look at next files:
1. `user_main.c`: entry point of the firmware, it contains mostly initialization and tasks startup logic, wifi system event loop handler
2. `config.h`: global compile-time firmware configuration (AP name, AP pass, server ports and IPs etc)
3. `ap_server.c`: access point server implementation
4. `ctrl_server.c`:  station UDP server
5. `rpc_dispatcher.c`: JSON RPC request dispatcher, it's used by `ap_server`
