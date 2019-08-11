import serial

bau = 2400
with serial.Serial('/dev/ttyXRUSB0', baudrate=bau) as ser:
    while(True):
        by = ser.read()
        print(by)
        ser.write(by)


