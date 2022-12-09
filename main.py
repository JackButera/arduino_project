import socket
import threading
import array
import pickle
from flask import Flask


app = Flask(__name__)

# Using IPv4 and UDP protocol
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

ip = "192.168.1.180"
port = 5000

# Creating socket
s.bind((ip, port))

recvIP = "192.168.1.177"
recvPort = 8888

sendCommand = [0xAA, 0xFC, 177, 0x03]

def DCP_genCmndBCH(buff, count):
    nBCHpoly = 0xB8
    fBCHpoly = 0x7F
    i = 0
    bch = 0
    while i < count:
        bch ^= buff[i]
        j = 0
        while j < 8:
            if ((bch & 1) == 1):
                bch = (bch >> 1) ^ nBCHpoly
            else:
                bch >>= 1
            j+=1
        i+=1
    bch ^= fBCHpoly
    return bch

commandBCH = DCP_genCmndBCH(sendCommand, 4)
sendCommand.append(commandBCH)

byte_array = bytearray(sendCommand)

def getTempState(thresh):
    if (thresh == 0b0101):
        return "Major Under"
    elif (thresh == 0b0001):
        return "Minor Under"
    elif (thresh == 0b0000):
        return "Comfortable"
    elif (thresh == 0b0010):
        return "Minor Over"
    elif (thresh == 0b1010):
        return "Major Over"

def validateFAPacket(arr):
    recBCH = arr[4]
    checkBCH = DCP_genCmndBCH(arr, 4)
    if (recBCH == checkBCH):
        return True
    return False

def validateDataPacket(arr):
    recBCH = arr[7]
    checkBCH = DCP_genCmndBCH(arr, 7)
    if (recBCH == checkBCH):
        return True
    return False


# Function for receiving messages
def receiveMessages():
    while(True):
        data = s.recvfrom(1024)
        data = data[0]
        if (validateFAPacket(data[:5]) & validateDataPacket(data[5:])):
            state = getTempState(data[8])
            temp = data[9]
            print(f'Temperature State: {state}')
            print(f'Temperature: {temp}')
        else:
            print(f'Failed to validate BCH')
        

# Function for sending messages
def sendMessages():
    s.sendto(byte_array, (recvIP, recvPort))




def printit():
    threading.Timer(5.0, printit).start()
    receive = threading.Thread(target=receiveMessages)
    send = threading.Thread(target=sendMessages)
    receive.start()
    send.start()

printit()
    


# @app.route("/")
# def home():
    
#     return "Hello, World!"
    
# if __name__ == "__main__":
#     app.run(debug=True)