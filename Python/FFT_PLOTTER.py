
import matplotlib.pyplot as plt
import serial
import time
import struct

arduino = serial.Serial(port='COM4',  baudrate=1000000, timeout=0.1)
time.sleep(2)
arduino.reset_input_buffer()
arduino.reset_output_buffer()

plt.ion()

def Serial_write(x):
    arduino.write(bytes(x+"\r\n",  'utf-8'))
    time.sleep(0.05)
def Serial_Read():
    rx = arduino.readline()
    rx = rx.decode("utf-8").strip()
    return(rx)

def fft_request():
    arduino.write(b'S')
    time_out = time.time() + 5  # 5 seconds timeout
    while(1):
        handskake = arduino.read(1)
        handskake = handskake.decode("utf-8").strip()
        if handskake == "K":
            return True
        
        if time.time() > time_out:
            print("Timeout waiting for handshake")
            return False
        
def request_fft_status():
    time_out = time.time() + 30
    while(1):
        status = arduino.read(1)
        status = status.decode("utf-8").strip()
        if status == "K":            
            return True
        
        if time.time() > time_out:
            print("Timeout waiting for data ready signal")
            return False


def read_fft_header(*,print_header=False):
    while(1):
        byte = arduino.read(1)
        if (byte == b'D'):
            print
            break

    bin_bytes = arduino.read(2)
    if(len(bin_bytes) < 2):
        print("Error reading header bytes")
        return None
    
    bins = struct.unpack('<H', bin_bytes)[0]
    

    bin_bytes = arduino.read(4)
    if(len(bin_bytes) < 4):
        print("Error reading header bytes")
        return None
    
    smpl_rate = struct.unpack('<f', bin_bytes)[0]
    

    frq_resolution = (smpl_rate/2) / bins

    
    if print_header:
        print(f"Expecting {bins} FFT data points (i.e., {bins * 2} bytes total)")
        print(f"Sampling rate received: {smpl_rate} kHz")
        print("Resolution is: ", frq_resolution, "Hz")
    return [bins, smpl_rate, frq_resolution]


def receive_fft_data(bins,*, print_header=False):
    bytes_to_read = bins*2
    data_bytes = b''

    while(len(data_bytes) < bytes_to_read):
        chunk = arduino.read(bytes_to_read - len(data_bytes))
        if not chunk:
            print("Timeout while reading FFT data")
            break
        data_bytes += chunk

    fmt = '<' + 'H' * bins
    if print_header:
        print(f"Expected bytes: {bytes_to_read}, received bytes: {len(data_bytes)}")
    data = struct.unpack(fmt, data_bytes)
    scale = -819.1875
    data = [x/scale for x in data]

    return list(data)



if fft_request():
    print("Handshake successful")
    print(request_fft_status())
    fft_header = (read_fft_header(print_header=True))
    fft_mag = (receive_fft_data(fft_header[0], print_header=True))
    fft_frq = [0]*fft_header[0]
    for i in range(fft_header[0]):
        fft_frq[i] = i * fft_header[2]


fig, ax = plt.subplots()
graph, = ax.plot(fft_frq, fft_mag, color='steelblue')
plt.ylim(-80, 0)

plt.xlabel("kHz")
plt.ylabel("dBV")
plt.grid(True)
graph.set_xdata(fft_frq)
graph.set_ydata(fft_mag)
fig.canvas.draw()
fig.canvas.flush_events()
time.sleep(0.5)

while(1):
    
    if fft_request():
        request_fft_status()
        fft_header = (read_fft_header())
        fft_mag = (receive_fft_data(fft_header[0]))
        fft_frq = [0]*fft_header[0]
        for i in range(fft_header[0]):
            fft_frq[i] = i * fft_header[2]

    
    graph.set_ydata(fft_mag)
    fig.canvas.draw()
    fig.canvas.flush_events()
    time.sleep(0.002)

