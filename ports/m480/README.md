MicroPython port to Nuvoton M480 (Cortex-M4 MCU)
================================================

&nbsp;
## Pin
### 输出
``` python
import machine
from machine import Pin

led = machine.Pin('PC6', Pin.OUT)
led.low()
led.high()
led.toggle()
led.value(1 - led.value())
```

&nbsp;
### 输入
``` python
key = machine.Pin('PC14', Pin.IN, pull=Pin.PULL_UP)
key.value()
```

&nbsp;
### 中断
``` python
led = machine.Pin('PC6', Pin.OUT)

key = machine.Pin('PC14', Pin.IN, pull=Pin.PULL_UP, irq=Pin.FALL_EDGE, callback=lambda key: led.toggle())
```

&nbsp;
### API
``` python
Pin(id, dir=Pin.IN, *, af=0, pull=Pin.PULL_NONE, irq=0, callback=None, priority=3)
```
|参数|含义|可取值|
|---|---|----|
|dir|GPIO方向|IN、OUT、OPEN_DRAIN|
|af|引脚功能，0表示GPIO| |
|pull|上下拉控制|PULL_UP、PULL_DOWN、PULL_NONE|
|irq|中断触发边沿|RISE_EDGE、FALL_EDGE、BOTH_EDGE、LOW_LEVEL、HIGH_LEVEL|
|callback|中断回调函数| |
|priority|中断优先级|0--7，数值越小优先级越高|


&nbsp;
## Timer
### 定时器
``` python
import machine
from machine import Pin, Timer

led = machine.Pin('PC6', Pin.OUT)

tmr = machine.Timer(0, 100000, callback=lambda tmr: led.toggle())   # 100000us
tmr.start()
```
定时器模式下单位为us，最大取值为0xFFFFFF = 16777215（约16秒）

&nbsp;
### 计数器
``` python
ctr = machine.Timer(1, 3, mode=Timer.COUNTER, pin='PC14', edge=Timer.EDGE_FALLING, callback=lambda ctr: led.toggle())
ctr.start()
```
计数器模式下单位为计数沿个数，最大取值为0xFFFFFF

&nbsp;
**如何查询计数器输入在哪个引脚上？**
``` python
for name in dir(machine.Pin):
    if 'TM1' in name:
        print(name)
```

&nbsp;
### 测脉宽
``` python
pin = machine.Pin('PA9', Pin.OUT)
pin.low()

cap = machine.Timer(1, 0xFFFFFF, mode=Timer.CAPTURE, pin='PA10', edge=Timer.EDGE_RISING_FALLING)
cap.start()

cap.cap_start()
pin.high(); time.sleep_ms(10); pin.low()
if cap.cap_done():
    cap.cap_value() / (machine.freq() / cap.prescale())
```
将PA9与PA10用杜邦线连接，在PA9上输出一个10ms宽的高脉冲，用Timer1捕获功能测量脉冲的宽度

&nbsp;
### API
``` python
Timer(id, period, *, mode=Timer.TIMER, pin=None, edge=Timer.EDGE_FALLING, trigger=0, prescale=None
                                       irq=Timer.IRQ_TIMEOUT, callback=None, priority=3)
```
|参数|含义|可取值|
|---|---|----|
|id| |0、1、2、3|
|period|周期|最大值0xFFFFFF|
|mode|工作模式|TIMER、COUNTER、CAPTURE|
|pin|计数和捕获引脚| |
|edge|计数和捕获边沿|EDGE_FALLING、EDGE_RISING、EDGE_FALLING_RISING、EDGE_RISING_FALLING|
|trigger|超时触发其他外设|TO_ADC、TO_DAC、TO_DMA、TO_PWM 及其或运算|
|prescale|预分频|1--256|
|irq|中断触发源|0、IRQ_TIMEOUT、IRQ_CAPTURE 及其或运算|
|callback|中断回调函数| |
|priority|中断优先级|0--7，数值越小优先级越高|
创建Timer实例时，若参数prescale取值为None，则Timer的预分频值如下：
- 定时器模式下：为 CyclesPerUs，因此定时单位为us
- 计数器模式或捕获模式下：为 1

&nbsp;
``` python
prescale([int])
```
获取或设置预分频，注意：定时器模式下修改预分频后，定时单位就不是us了

&nbsp;
``` python
period([int])
value()
```
获取或设置周期值，定时器模式下单位为us，计数器模式下单位为计数沿个数  
获取计数器当前值，定时器模式下单位为us，计数器模式下单位为计数沿个数

&nbsp;
``` python
cap_start([edge])
```
开启脉宽测量，测量完成后会自动关闭，若想再次测量需要再次调用此方法  
开启测量的同时可以通过edge参数指定要捕获的脉冲类型，注意通过此参数设定是持久性的，即后面不带参数调用cap_start()时使用新的设定

&nbsp;
``` python
cap_done()
cap_value()
```
cap_done()用于检查测量是否完成；cap_value()用于获取测量到的脉宽值

&nbsp;
``` python
irq_enable(irq)
irq_disable(irq)
irq_flags(): irq
```
irq取值为 IRQ_TIMEOUT、IRQ_CAPTURE 及其或运算


&nbsp;
## UART
``` python
import machine
from machine import UART

ser = machine.UART(1, 115200, rxd='PA2', txd='PA3')

ser.write('Hi from MicroPy')

while ser.any() < 7: pass
ser.read()
```
UART0用于终端交互，请勿使用

&nbsp;
### 中断接收
``` python
led = machine.Pin('PC6', Pin.OUT)

def on_ser_rx(ser):
    if ser.irq_flags() & ser.IRQ_RXAny:
        while ser.any():
            char = ser.readchar()
            os.dupterm().writechar(char)    # do something on received char

    if ser.irq_flags() & ser.IRQ_TIMEOUT:
        led.toggle()                        # do something when no data before timeout

ser = machine.UART(1, 57600, rxd='PA2', txd='PA3', irq=UART.IRQ_RXAny, callback=on_ser_rx)
```
注意1：中断服务程序中GC被锁，不能动态分配内存，因此callback函数中不能使用read和readline，也不能用print  
注意2：readchar底层利用了SysTick延时功能，因此UART1的中断优先级不能高于SysTick，SysTick的中断优先级为3

&nbsp;
### 中断发送
``` python
class tx_helper:
    @classmethod
    def send(cls, ser, msg):
        cls.Inx = 0
        cls.Msg = msg
        ser.irq_enable(ser.IRQ_TXEmpty)   # when want send message, enable irq

    @classmethod
    def on_tx(cls, ser):
        if ser.irq_flags() & ser.IRQ_TXEmpty:
            while not ser.full():
                if cls.Inx < len(cls.Msg):
                    ser.writechar(cls.Msg[cls.Inx])
                    cls.Inx += 1
                else:
                    ser.irq_disable(ser.IRQ_TXEmpty)   # when message sent, disable irq
                    break

ser = machine.UART(1, 115200, rxd='PA2', txd='PA3', irq=UART.IRQ_TXEmpty, callback=lambda ser: tx_helper.on_tx(ser))
tx_helper.send(ser, b'Hi from MicroPy on NuMicro M480\n')
```
注意：由于str的索引会导致动态内存分配，因此只能发送bytes

&nbsp;
### API
``` python
UART(id, baudrate, *, bits=8, parity=None, stop=1, rxd=None, txd=None,
                      irq=0, callback=None, priority=4, timeout=10)
```
|参数|含义|可取值|
|---|---|----|
|id| |1、2、3、4、5|
|baudrate|串口波特率| |
|bits|数据位宽|5、6、7、8|
|parity|校验位|'None'、'Odd'、'Even'、'One'、'Zero'|
|stop|停止位宽|1、2|
|rxd|UART_RXD引脚| |
|txd|UART_TXD引脚| |
|irq|中断触发源|0、IRQ_RXAny、IRQ_TXEmpty 及其或运算|
|callback|中断回调函数| |
|priority|中断优先级|0--7，数值越小优先级越高|
|timeout|timeout个字符时间未收到新数据触发超时|0--25|

&nbsp;
``` python
baudrate([int])
```
获取或设置波特率

&nbsp;
``` python
read([nbytes])
readinto(buf[, nbytes])
readline()
```
若不指定读取个数，read和readinto读取所有已接收到的内容；readline一直读取直到遇到换行符

&nbsp;
``` python
write(buf)
```
buf是实现buffer协议的对象（如str、bytearray）

&nbsp;
``` python
irq_enable(irq)
irq_disable(irq)
irq_flags(): irq
```
irq取值为 IRQ_RXAny、IRQ_TXEmpty 及其或运算


&nbsp;
## SPI
### 读写闪存
``` python
import machine, time
from machine import Pin, SPI
from array import array

spi = machine.SPI(1, 1000000, mosi='PB4', miso='PB5', sck='PB3')
spi_ss = machine.Pin('PB2', Pin.OUT)
spi_ss.high()

GD25Q21_CMD_READ_CHIPID	  =	0x90
GD25Q21_CMD_READ_DATA	  = 0x03
GD25Q21_CMD_WRITE_PAGE	  = 0x02
GD25Q21_CMD_ERASE_SECTOR  = 0x20
GD25Q21_CMD_WRITE_ENABLE  = 0x06
GD25Q21_CMD_WRITE_DISABLE = 0x04

def flash_identify():
    spi_ss.low()
    spi.write(array('B', [GD25Q21_CMD_READ_CHIPID,  0x00, 0x00, 0x00]))
    memview = spi.read(2)
    spi_ss.high()

    return array('B', memview)

def flash_erase(addr):
    spi_ss.low()
    spi.write(array('B', [GD25Q21_CMD_WRITE_ENABLE]))
    spi_ss.high()

    spi_ss.low()
    spi.write(array('B', [GD25Q21_CMD_ERASE_SECTOR, (addr >> 16) & 0xFF, (addr >>  8) & 0xFF, (addr >>  0) & 0xFF]))
    spi_ss.high()

def flash_write(addr, buf):
    spi_ss.low()
    spi.write(array('B', [GD25Q21_CMD_WRITE_ENABLE]))
    spi_ss.high()

    spi_ss.low()
    spi.write(array('B', [GD25Q21_CMD_WRITE_PAGE,   (addr >> 16) & 0xFF, (addr >>  8) & 0xFF, (addr >>  0) & 0xFF]))
    spi.write(buf)
    spi_ss.high()

def flash_read(addr, cnt):
    spi_ss.low()
    spi.write(array('B', [GD25Q21_CMD_READ_DATA,    (addr >> 16) & 0xFF, (addr >>  8) & 0xFF, (addr >>  0) & 0xFF]))
    memview = spi.read(cnt)
    spi_ss.high()

    return array('B', memview)

flash_identify()
flash_erase(0x1000)
time.sleep(2)
flash_read(0x1000, 32)
flash_write(0x1000, array('B', [i for i in range(32)]))
time.sleep(1)
flash_read(0x1000, 32)
```

&nbsp;
### API
``` python
SPI(id, baudrate, *, polarity=SPI.IDLE_LOW, phase=SPI.EDGE_FIRST, slave=False,
                     msbf=True, bits=8, mosi=None, miso=None, sck=None, ss=None)
```
|参数|含义|可取值|
|---|---|----|
|id| |0、1、2、3|
|baudrate|时钟速率| |
|polarity|时钟极性|IDLE_LOW、IDLE_HIGH|
|phase|时钟相位|EDGE_FIRST、EDGE_SECOND|
|slave|是否为从机| |
|msbf|是否先传最高位| |
|bits|数据位宽|取值8--32|
|mosi|SPI_MOSI引脚| |
|miso|SPI_MOSI引脚| |
|sck|SPI_SCK引脚| |
|ss|SPI_SS引脚，从机模式下使用| |

&nbsp;
``` python
baudrate([int])
```
获取或设置波特率

&nbsp;
``` python
write(data): count
```
只写；data可以是一个整数，或实现buffer协议的对象（如str、bytearray、array.array）

&nbsp;
``` python
read(count): int or buf
readinto(buf): count
```
只读；读取一个数据时返回一个整数，读取多个数据时返回一个memoryview

&nbsp;
``` python
write_read(data): int or buf
write_readinto(wbuf, rbuf)
```
同时读写；参数同'write'，返回值同'read'


&nbsp;
## I2C
### 通用读写
``` python
import machine, time
from machine import I2C
from array import array

i2c = machine.I2C(0, 100000, sda='PA4', scl='PA5')
i2c.scan()

SLV_ADDR = 0x50   # AT24C02的地址
i2c.writeto(SLV_ADDR, array('B', [0x00, 0x37, 0x55, 0xAA, 0x78]))
time.sleep(1)
i2c.writeto(SLV_ADDR, array('B', [0x00]))   # dummy write, set read address
i2c.readfrom(SLV_ADDR, 4)
```

### 存储读写
``` python
i2c.mem_writeto(SLV_ADDR, 0x10, array('B', [0x37, 0x55, 0xAA, 0x78]))
time.sleep(1)
i2c.mem_readfrom(SLV_ADDR, 0x10, 4)
```

&nbsp;
### API
``` python
I2C(id, baudrate, *, slave=False, slave_addr=None, scl=None, sda=None)
```
|参数|含义|可取值|
|---|---|----|
|id| |0、1、2|
|baudrate|时钟速率| |
|slave|是否为从机|True、False|
|slave_addr|从机地址，从机模式下使用| |
|scl|I2C_SCL引脚| |
|sda|I2C_SDA引脚| |

&nbsp;
``` python
baudrate([int])
```
获取或设置波特率

&nbsp;
``` python
scan(start=0x10, end=0x7F): tuple
```
扫描 I2C 总线上的从机

&nbsp;
``` python
writeto(addr, data): count
```
data可以是一个整数，或实现buffer协议的对象（如str、bytearray、array.array）

&nbsp;
``` python
readfrom(addr, count): str
readfrom_into(addr, buf): count
```
读取

&nbsp;
``` python
mem_writeto(addr, memaddr, data, memaddr_len=1): count
```
data可以是一个整数，或实现buffer协议的对象（如str、bytearray、array.array）
memaddr_len是memaddr的长度，单位为byte

&nbsp;
``` python
mem_readfrom(addr, memaddr, count, memaddr_len=1): str
mem_readfrom_into(addr, memaddr, buf, memaddr_len=1): count
```
从指定存储地址读取


&nbsp;
## ADC
### 软件启动
``` python
import machine
from machine import ADC

adc = machine.ADC(0, chns=[0, 1, 2])

adc.start()
while adc.busy(): pass

adc.read(0)
adc.read(1)
adc.read(2)
```
CH0 on PB0、CH1 on PB1、...、CH15 on PB15

### 定时启动
``` python
adc = machine.ADC(0, chns=[1], trigger=ADC.TRIG_TIMER3)

tmr = machine.Timer(3, 1000, trigger=Timer.TO_ADC)   # 1000us
tmr.start()

buf = [0] * 500
for i in range(500):
    while not adc.any(1): pass
    buf[i] = adc.read(1)
```

&nbsp;
### API
``` python
ADC(id, chns, *, samprate=1000000, trigger=ADC.TRIG_SW, trigger_pin=None)
```
|参数|含义|可取值|
|---|---|----|
|id| |0|
|chns|转换通道| |
|samprate|转换速率|50000-5000000|
|trigger|启动触发|TRIG_SW、TRIG_FALLING、TRIG_RISING、TRIG_EDGE、TRIG_TIMER0、TRIG_TIMER1、TRIG_TIMER2、TRIG_TIMER3|
|trigger_pin|外部引脚触发启动时指定触发引脚| |

&nbsp;
``` python
samprate([int])
```
获取或设置转换速率

&nbsp;
``` python
channel([chns])
trigger([trigger[, trigger_pin]])
```
获取或设置转换通道  
获取或设置启动触发

&nbsp;
``` python
start()
busy()
any(chn)
read(chn)
```
软件启动模式下，start()用来启动转换；busy()检测转换是否完成；any()检测指定通道是否有转换结果可读


&nbsp;
## DAC
### 输出电平
``` python
import machine
from machine import DAC

dac = machine.DAC(0)
dac.write(1/3.3 * 4095)
```
DAC0 on PB12；DAC1 on PB13

&nbsp;
### 输出波形
``` python
import math, array

buf = array.array('H', [0]*250)
for i in range(250):
    buf[i] = int(math.sin(2*math.pi/250*i) * 2047 + 2048)

dac = machine.DAC(0, trigger=DAC.TRIG_TIMER2, data=buf)

tmr = machine.Timer(2, 1000, trigger=Timer.TO_DAC)   # 1000us
tmr.start()
```
指定波形数据输出完后需要重新调用 dma_write() 更新波形数据

&nbsp;
### API
``` python
DAC(id, *, trigger=DAC.TRIG_WRITE, data=None)
```
|参数|含义|可取值|
|---|---|----|
|id| |0、1|
|trigger|输出更新触发|TRIG_WRITE、TRIG_FALLING、TRIG_RISING、TRIG_TIMER0、TRIG_TIMER1、TRIG_TIMER2、TRIG_TIMER3|
|data|用于非TRIG_WRITE触发时| |

&nbsp;
``` python
write(int)
```
使DAC输出指定电平，只有当“trigger == TRIG_WRITE”时才能使用此方法

&nbsp;
``` python
dma_write(buf)
dma_done(): bool
```
使DAC输出一组电平，只有当“trigger != TRIG_WRITE”时才能使用此方法  
dma_write(buf)设置数据输出完成后，dma_done()返回true，需要重新调用 dma_write() 更新波形数据


&nbsp;
## PWM
共4路PWM，前两路PWM0、PWM1是BPWM，后两路PWM2、PWM3是EPWM
- BWPM：1个计数器、6个比较器，6路输出，同频、占空比可不同
- EPWM：6个计数器、6个比较器，6路输出，可组合成3路互补输出

&nbsp;
### BWPM
``` python
import machine
from machine import PWM

pwm = machine.PWM(PWM.BPWM0, clkdiv=192, period=1000)
pwm.channel(PWM.CH2, duty=300, pin='PA2')
pwm.channel(PWM.CH3, duty=700, pin='PA3')
pwm.start()
```

&nbsp;
### EPWM（独立输出）
``` python
pwm = machine.PWM(PWM.EPWM0, clkdiv=192)
pwm.channel(PWM.CH2, period=1000, duty=500, pin='PA3')
pwm.channel(PWM.CH3, period=2000, duty=500, pin='PA2')
pwm.start() or
pwm.start([PWM.CH2, PWM.CH3])
```

&nbsp;
### EPWM（互补输出）
``` python
pwm = machine.PWM(PWM.EPWM0, clkdiv=192)
pwm.channel(PWM.CH0, period=1000, duty=500, deadzone=25, pin='PA5', pinN='PA4')
pwm.channel(PWM.CH2, period=2000, duty=500, deadzone=25, pin='PA3', pinN='PA2')
pwm.start() or
pwm.start([PWM.CH0, PWM.CH2])
```
只要参数 deadzone 值不为 None，就设置为互补输出  
只有CH0、CH2和CH4可以设置为互补输出，且CH0为互补输出时CH1被用作其互补输出，CH2与CH3、CH4与CH5有相同关系

&nbsp;
### API
``` python
PWM(id, *, clkdiv=192, period=1000)
```
|参数|含义|可取值|
|---|---|----|
|id| |BPWM0、BPWM1、EPWM0、EPWM1|
|clkdiv|预分频|1--256|
|period|周期时长|1--65536|

&nbsp;
``` python
channel(chn, *, duty=500, period=1000, deadzone=None, pin=None, pinN=None)
```
配置通道
|参数|含义|可取值|
|---|---|----|
|chn| |CH0、CH1、CH2、CH3、CH4、CH5|
|duty|高电平时长|1--65536|
|period|周期时长|1--65536|
|deadzone|死区时长|1--4096 或 None|
|pin|通道输出引脚| |
|pinN|通道互补输出引脚| |

&nbsp;
``` python
start([chns])
stop([chns])
```
- BPWM：无参数
- EPWM：无参数时启动/停止所有已配置通道，有参数时启动/停止指定通道

&nbsp;
``` python
period([period])       # for BPWM
period(chn[, period])  # for EPWM
duty(chn[, duty])
deadzone(chn[, deadzone])
```
查询或设置周期时间长度、高电平时间长度、死区时间长度


&nbsp;
## CAN
### 自测
``` python
import machine
from machine import CAN

can = machine.CAN(0, 100000, mode=CAN.LOOPBACK, rxd='PA4', txd='PA5')
can.send(0x137, b'\x11\x22\x33\x44\x55\x66\x77\x88')
while not can.any(): pass
can.read()
```
将PA4和PA5短接，执行上面的自测代码

&nbsp;
### 接收
``` python
can = machine.CAN(0, 100000, rxd='PA4', txd='PA5')
can.filter([CAN.FRAME_STD, 0x137, 0x7FE), (CAN.FRAME_STD, 0x154, 0x7FF), (CAN.FRAME_EXT, 0x1137, 0x1FFFFFFC)])
while not can.any(): pass
can.read()
```
当消息满足“ID & mask == check & mask”时被接收，因此上面的设置接收ID等于0x137、0x136和0x154的标准帧消息，及ID等于0x1137、0x1136、0x1135和0x1134的扩展帧消息

&nbsp;
### API
``` python
CAN(id, baudrate, *, mode=CAN.NORMAL, rxd=None, txd=None, irq=0, callback=None, priority=4)
```
|参数|含义|可取值|
|---|---|----|
|id| |0、1|
|baudrate|位速率|1000-1000000|
|mode|工作模式|NORMAL、LISTEN、LOOPBACK|
|rxd|接收引脚| |
|txd|发送引脚| |
|irq|中断触发源| |
|callback|中断回调函数| |
|priority|中断优先级|0--7，数值越小优先级越高|

&nbsp;
``` python
baudrate([int])
```
获取或设置波特率

&nbsp;
``` python
full()
send(id, data, *, format=CAN.FRAME_STD, remoteReq=False, retry=True)
```
full()返回False表示可以发送，send()用于发送数据，参数data可以是一个整数或buffer

&nbsp;
``` python
any()
read()
```
any()返回True表示有数据可读，read()用于读出接收到的数据，返回值格式为（format, id, bytes, is_remote)

&nbsp;
``` python
state()
```
查询模块当前状态，可取值及其解释如下：
|状态|含义|
|---|---|
|STAT_EACT|ERROR ACTIVE,  1 error_counter <   96|
|STAT_EWARN|ERROR WARNING, 1 error_counter >=  96|
|STAT_EPASS|ERROR PASSIVE, 1 error_counter >= 127|
|STAT_BUSOFF|BUS OFF,       1 error_counter >= 255|

&nbsp;
``` python
filter([filter])
```
读取或设置接收过滤器，滤波器格式为：[(CAN.FRAME_STD, check11b, mask11b), (CAN.FRAME_EXT, check29b, mask29b)]，列表中最多可以有8个滤波器


&nbsp;
## RTC
``` python
rtc = machine.RTC()
rtc.datetime([2020, 5, 11, 0, 14, 34, 00])
rtc.datetime()
```
datetime()用于查询或设置当前时间，参数依次为：年、月、日、DayOfWeek、时、秒，设置时参数中DayOfWeek可以是任意值，因为代码会自动计算它的正确值


&nbsp;
## RNG
``` python
rng = machine.RNG()
rng.get()
```
