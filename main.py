from machine import Pin, I2C, PWM, Timer
import time
import ssd1306


# ==========================
# Pin Configuration
# ==========================

FREQ_PIN = 27
TEST_PIN = 18

SDA_PIN = 21
SCL_PIN = 22


# ==========================
# OLED Setup
# ==========================

i2c = I2C(
    0,
    scl=Pin(SCL_PIN),
    sda=Pin(SDA_PIN),
    freq=400000
)

oled = ssd1306.SSD1306_I2C(
    128,
    64,
    i2c,
    0x3C
)


# ==========================
# Frequency Variables
# ==========================

pulse_count = 0
last_time = 0

frequency = 0

freq_min = 999999
freq_max = 0


# ==========================
# Test Frequency Generator
# GPIO18
# ==========================

test_pwm = PWM(
    Pin(TEST_PIN)
)

test_pwm.freq(1000)       # 1kHz
test_pwm.duty(512)        # 50% duty



# ==========================
# Frequency Interrupt
# GPIO27
# ==========================

def freq_interrupt(pin):

    global pulse_count
    global last_time

    now = time.ticks_us()

    # Noise filter
    if time.ticks_diff(now,last_time) > 20:
        pulse_count += 1
        last_time = now



freq_pin = Pin(
    FREQ_PIN,
    Pin.IN,
    Pin.PULL_UP
)


freq_pin.irq(
    trigger=Pin.IRQ_RISING,
    handler=freq_interrupt
)



# ==========================
# Timer Measurement
# ==========================

def measure(timer):

    global pulse_count
    global frequency

    count = pulse_count
    pulse_count = 0

    if count < 2:
        frequency = 0

    else:
        # 500ms gate
        frequency = count * 2



timer = Timer(0)

timer.init(
    period=500,
    mode=Timer.PERIODIC,
    callback=measure
)



# ==========================
# Format Frequency
# ==========================

def format_freq(f):

    if f < 1:
        return "No Signal"

    elif f >= 1000000:
        return "{:.2f}MHz".format(f/1000000)

    elif f >= 1000:
        return "{:.3f}kHz".format(f/1000)

    else:
        return "{:.1f}Hz".format(f)



# ==========================
# OLED Display
# ==========================

def display():

    global freq_min
    global freq_max


    oled.fill(0)


    oled.text(
        "FREQUENCY",
        15,
        0
    )


    value = format_freq(frequency)


    oled.text(
        value,
        5,
        20
    )


    if frequency > 0:

        if frequency < freq_min:
            freq_min = frequency

        if frequency > freq_max:
            freq_max = frequency



    oled.text(
        "MIN:",
        0,
        45
    )

    oled.text(
        format_freq(freq_min),
        35,
        45
    )


    oled.text(
        "MAX:",
        0,
        55
    )

    oled.text(
        format_freq(freq_max),
        35,
        55
    )


    oled.show()



# ==========================
# Main Loop
# ==========================

while True:


    print(
        "Frequency:",
        format_freq(frequency)
    )


    display()


    time.sleep(0.5)
