import pigpio
import time

pi = pigpio.pi()

SERVO_PIN = 18
BUTTON_PIN = 26
LED_PIN = 19
LOCK_HZ = 500
UNLOCK_HZ = 1400


def unlock():
    print "UNLOCK"
    pi.set_servo_pulsewidth(SERVO_PIN, UNLOCK_HZ)

def lock():
    print "LOCK"
    pi.set_servo_pulsewidth(SERVO_PIN, LOCK_HZ)

def unlock_lock():
    unlock()
    pi.write(LED_PIN, 1)
    time.sleep(3)
    pi.write(LED_PIN, 0)
    lock()

def pin_setup():
    pi.set_mode(BUTTON_PIN, pigpio.INPUT)
    pi.set_pull_up_down(BUTTON_PIN, pigpio.PUD_UP)
    pi.set_mode(LED_PIN, pigpio.OUTPUT)
    lock()
    pulse_led(5)

def pulse_led(times):
    for each in range(times):
        pi.write(LED_PIN, 1)
        time.sleep(0.5)
        pi.write(LED_PIN, 0)
        time.sleep(0.5)


def main():
    try:
        pin_setup()
        while True:
            if not pi.read(BUTTON_PIN):
                unlock_lock()
            time.sleep(0.5)
    except KeyboardInterrupt:
        pi.stop()


if __name__ == '__main__':
    main()
