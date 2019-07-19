MicroPython port to MT7687 WiFi SoC
===================================


## PIN
``` python
led = machine.Pin(35, machine.Pin.OUT)
```


## Timer
``` python
 tmr = machine.Timer(0, 1000, callback=lambda tmr, led=led:led.toggle())
```


## PWM
``` python
pwm = machine.PWM(21, 1000, 200)
pwm.start()
```
