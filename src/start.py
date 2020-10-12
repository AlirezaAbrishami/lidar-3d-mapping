import RPi.GPIO as GPIO
import os
import time

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(33, GPIO.OUT)

def check(n):
    while True: # Run forever
        if GPIO.input(n) == GPIO.HIGH:
            print("Button was pushed!")
            return True;

while True:
    s = check(11);
    if s:
        os.system("./execute e")
        GPIO.output(33, 1)
    time.sleep(0.5)
    s = check(11)
    if s:
        os.system("./execute k")
        GPIO.output(33, 0);
    time.sleep(0.5)


