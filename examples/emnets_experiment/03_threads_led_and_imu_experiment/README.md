Blinky!
=======

This is a basic example that blinks an LED, if available. (If no LED is present or configured, it
will print "Blink!" via stdio instead.)

This is mostly useful for boards without stdio to check if a new port of RIOT works. For that
reason, this example has only an optional dependency on timer drivers. Hence, this application only
needs a working GPIO driver and is likely the first milestone when porting RIOT to new MCUs.



连接LED GND, VCC到对应的GND，3V3或者5V的引脚， IN 连接到自定义的GPIO引脚，如这里使用的是GPIO12.

运行以下代码，即可完成烧写
```bash
sudo chmod 777 /dev/ttyUSB0
make BOARD=esp32-wroom-32 flash term
```