
import matplotlib.pyplot as plt
import serial
import time

arduino = serial.Serial(port='COM10',  baudrate=500000, timeout=.25)


def Serial_write(x):
    arduino.write(bytes(x+"\r\n",  'utf-8'))
    time.sleep(0.05)
def Serial_Read():
    rx = arduino.readline()
    rx = rx.decode("utf-8").strip()
    return(rx)


plt.ion()

# num = input("Enter a number: ")
# print(num)''
# num = "H"
time.sleep(1)
Serial_write('H')
time.sleep(0.55)
value = Serial_Read()
print(value)
numCount = int(value)
frequency = numCount*[0]
mag = numCount*[0]

while True:
    value = Serial_Read()
    if(value == "H"):
        break


for i in range(numCount+1):
    value = Serial_Read()
    if(value == '!'):
        break
    freq_str, dbv_str = value.split(":")
    frequency[i] = float(freq_str.replace("kHz", ""))
    mag[i] = float(dbv_str.replace("dBV", ""))

graph = plt.plot(frequency, mag)[0]
plt.xlabel("kHz")
plt.ylabel("dBV")
plt.grid(True)
plt.show()
plt.pause(0.25)

while True:
    Serial_write('H')
    time.sleep(0.025)
    value = Serial_Read()
    while True:
        value = Serial_Read()
        if(value == "H"):
            break


    for i in range(numCount+1):
        value = Serial_Read()
        if(value == '!'):
            break
        freq_str, dbv_str = value.split(":")
        frequency[i] = float(freq_str.replace("kHz", ""))
        mag[i] = float(dbv_str.replace("dBV", ""))

    graph.remove()
    graph = plt.plot(frequency, mag, color = 'steelblue')[0]
    plt.xlabel("kHz")
    plt.ylabel("dBV")
    plt.grid(True)
    plt.show()
    plt.pause(0.05)