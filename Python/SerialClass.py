
import serial.tools.list_ports

class SerialComm:
    def __init__(self, baudRate=9600):
        self.ports = serial.tools.list_ports.comports()
        self.SerialInst = serial.Serial()
        self.baudRate = baudRate
        self.comm =""
        self.errorVar = 0

    def openSerialPort(self):
        portList = []

        for onePort in self.ports:
            portList.append(str(onePort))
            print(str(onePort))

        val = input("select port: COM")

        for x in range(0, (len(portList)+1)):
            if(x == (len(portList))):
                print("ComPort not found.")
                self.errorVar = 1

            else:
                if portList[x].startswith("COM" + str(val)):
                    portVar = "COM" + str(val)
                    print(portList[x])
                    self.errorVar = 0
                    break
        if(self.errorVar == 0):
            self.SerialInst.baudrate = self.baudRate
            self.SerialInst.port = portVar
            self.SerialInst.open()
            self.comm = portVar
        return(self.errorVar)

    def fetchData(self):
        while True:
            if self.SerialInst.in_waiting:
                packet = self.SerialInst.readline()
                GNSS_DATA = packet.decode("utf")
                # print(GNSS_DATA)
                break
        return(GNSS_DATA)