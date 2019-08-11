import serial

bau = 2400
with serial.Serial('/dev/ttyXRUSB0', baudrate=bau, timeout=0.1) as ser:
	with serial.Serial('/dev/ttyXRUSB1', baudrate=bau, timeout=0.1) as ser2:
		print("Started")
		while(True):
			by = ser.read()
			print(by)
			ser2.write(by)
			by = ser2.read()
			print(by2)
			ser.write(by)


