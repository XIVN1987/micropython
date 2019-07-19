MicroPython port to NuMicro M480 (Cortex-M4 MCU)
================================================


## Pin
``` python
led = machine.Pin('PB14', machine.Pin.OUT)
led.low()
led.high()
led.toggle()
​
key = machine.Pin('PC14', machine.Pin.IN)
key.value()
```


## UART
``` python
ser = machine.UART(1, 115200, rxd='PA2', txd='PA3')
ser.write('Hi from MicroPy')
ser.read(ser.any())
```
UART0用于终端交互，请勿使用

串口引脚也可以通过如下方式指定：
``` python
machine.Pin('PA2', af=machine.Pin.PA2_UART1_RXD)
machine.Pin('PA3', af=machine.Pin.PA3_UART1_TXD)
​
ser = machine.UART(1, 115200)
```

### 怎样查找外设的功能引脚在哪个IO上呢？
``` python
for name in dir(machine.Pin):
    if 'UART1' in name:
        print(name)
```


## Timer
``` python
tmr = machine.Timer(0, 1000000, callback=lambda tmr, led=led: led.toggle())
tmr.start()
```


## SPI
``` python
spi = machine.SPI(1, 1000000, polarity=0, phase=1, msbf=True, bits=8, mosi='PB4', miso='PB5', clk='PB3')
​
spi_cs = machine.Pin('PA4', machine.Pin.OUT)
spi_cs.low()
​
spi.write('Hi from MicroPy')
​
bytes = spi.read(4)
    
buf = bytearray(16)
spi.readinto(buf)
​
s = 'Hi from MicroPy'
bytes = spi.write_read(s)
​
s = 'Hi from MicroPy'
buf = bytearray(len(s))
spi.write_readinto(s, buf)
​
spi_cs.high()
```


## I2C
``` python
i2c = machine.I2C(0, 100000, sda='PA4', scl='PA5')
i2c.scan()
```


## ADC
``` python
adc = machine.ADC(0)
adc.samp_config(machine.ADC.SAMP_0, machine.ADC.CHN_0)
adc.samp_config(machine.ADC.SAMP_1, machine.ADC.CHN_0)
adc.samp_config(machine.ADC.SAMP_2, machine.ADC.CHN_1)
​
adc.start()
adc.read()
```

## DAC
### 输出电平
``` python
dac = machine.DAC(0)
dac.write(1/3.3 * 4095)
DAC0 B12（板上标URX）；DAC1 B13（板上标UTX）
```

### 输出波形
``` python
dac = machine.DAC(0, trigger=machine.DAC.TRIG_TIMER0, data=array.array('H', [1000, 2000, 3000, 2000, 1000]))
tmr = machine.Timer(0, 1000, trigger=machine.Timer.TO_DAC)
tmr.start()
```


## PWM
共4路PWM，前两路PWM0、PWM1是BPWM，后两路PWM2、PWM3是EPWM
BWPM：1个计数器、6个比较器，6路输出，同频、占空比可不同
EPWM：6个计数器、6个比较器，6路输出，可组合成3路互补输出

### BWPM
``` python
pwm = machine.PWM(0, clkdiv=192, period=1000)
pwm.chn_config(machine.PWM.CH_2, duty=300, pin='PA2')
pwm.chn_config(machine.PWM.CH_3, duty=700, pin='PA3')
​
pwm.start()
```

修改频率、占空比
``` python
pwm.period(2000)
pwm.duty(machine.PWM.CH_2, 1500)
```

### EPWM
