MicroPython port to MediaTek MT7687 (Cortex-M4 WiFi SoC)
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

## Special Pin
GPIO37: 0 Flash normal mode   1 Flash recovery mode
GPIO59: SWDIO
GPIO60: SWCLK


## Debug Enable (Memory Access Enable)
monitor reset
monitor memU32 0x8300F050 = 0x76371688
monitor memU32 0x8300F050 = 0x76371688
monitor memU32 0x8300F050 = 0x76371688
