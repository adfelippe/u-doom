# u-doom
u-doom is a Doom port to u-blox's [ubxlib](https://github.com/u-blox/ubxlib). It was just a playground to test the Web Bluetooth API features and have fun with BLE. It shows how easy it is to use ubxlib even to port Doom for it.

As a proof of concept, the project provides a simple web page that uses the native JavaScript Web Bluetooth API to connect to an [EVK-NINA-W15](https://www.u-blox.com/en/product/evk-nina-w15). The EVK is actually configured and controlled by our Doom port, which instead of sending the game frames to a video driver, transmits them via BLE. Everything is done using ubxlib.

The port was done for both Windows and Linux.

## Doom port
I chose the [doomgeneric](https://github.com/adfelippe/doomgeneric) variant to port here since it's one of the easiest ones out there, with only 5 functions to be ported, which fits perfectly with ubxlib. These are the functions that must be ported:
* DG_Init
* DG_DrawFrame
* DG_SleepMs
* DG_GetTicksMs
* DG_GetKey

This is how each one of them work in this port:

|Function             |Description|
|---------------------|-----------|
|DG_Init              |Initialize ubxlib, hardware and networking.
|DG_DrawFrame         |Convert raw bitmap to PNG, split into smaller chunks and send them via BLE
|DG_SleepMs           |Platform-specific sleep
|DG_GetTicksMs        |Platform-specific get system tick in ms.
|DG_GetKey            |Read keys from a queue received from the remote Web BLE app

Please notice I forked the project in order to organize it a little bit and reduce the screen resolution to 320x200 to minimize the amount of data to be sent via BLE. Most of the compilation warnings were fixed and the shareware WAD file was added.

## EVK-NINA-W15
The EVK is controlled by ubxlib. It's configured with BLE SPS and the serial port runs at 961200 bps. Please see the disclaimer below for the implications of those.

## Web Bluetooth Application
The Web Bluetooth component is a JavaScript application (sort of) running natively in the browser. Using the native Web Bluetooth API, we can connect to the NINA-EVK using any of the computer's Bluetooth device. Once the connection is established, the EVK will start sending chopped frames, which the application will then reconstruct based on a simple protocol. Once a full frame is received, it'll display the image in the browser on a 2D canvas. The application also captures the pressed/unpressed keys and send them back to the EVK so the game can respond to those events. Only up, down, left, right, ctrl, enter and esc are mapped at the moment.

## How to compile and run
### Compile the port for Linux
```shell
user@~/workspace/u-doom/doom-port-linux $ git submodule update --init
user@~/workspace/u-doom/doom-port-linux $ mkdir build
user@~/workspace/u-doom/doom-port-linux $ cd build
user@~/workspace/u-doom/doom-port-linux/build $ cmake ..
user@~/workspace/u-doom/doom-port-linux/build $ make -j
```

### Compile the port for Windows
Well, here you're on your own, but the process is similar to Linux, except that you call `msbuild` instead of `make` and point to an sln project file created by CMake. It should work if you're lucky enough. :)

### Running the port
The binary to execute is called `u-doom`. In order to actually make Doom run properly, you need to pass a WAD file (where Doom stores all the game data) using the `-iwad` option. Since I added the shareware WAD file for easy testing, this should do:
```shell
user@~/workspace/u-doom/doom-port-linux/build $ ./u-doom -iwad ../../components/doomgeneric/wad/doom1.wad
```

### Running the Web Bluetooth Application
As I said, the Web app is sort of native. It can run natively and just opening the index.html from the web-ble folder will work, but if you want a fancy panel with colored buttons, you'll have to install and run node.js. From inside the same folder, `npm install` and `npm start` will do the job if node is installed. Then you access it on http://localhost:3000/.

## Disclaimer
This project was done for fun and to be presented at an internal embedded software conference at u-blox, therefore it's not meant to be playable in any way. The result is good enough given BLE limitations. On the transmission side, it was possible to reach 5 FPS, which would be very much playable, but for some reason I couldn't debug in time, the Web Bluetooth API drops most of the packets at that rate. As a workaround, each BLE packet is transmitted with an 3 ms delay, causing the frame rate to drop to maximum 3 FPS. It's still playable though.

## TO-DO
If anyone (or myself) wants to play around or fix things, these can be done:
* Investigate packet drop on Web App's side. All packets are received correctly by the PC's BLE device, but the API seems to have issues to consume all of them at fast rates. Maybe some queue getting full?
* Add all Doom keys (IDDQD and IDKFA would be nice to have).
* Squeeze out maximum performance on ubxlib's side.
* Some code clean up
