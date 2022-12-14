from flask import Flask, jsonify, render_template, request
from flask_mail import Mail, Message
import socket
import array
import select
import smtplib
from threading import *
import time

app = Flask(__name__)

# Using IPv4 and UDP protocol
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

ip = "192.168.1.180"
port = 5000

recvIP = "192.168.1.177"
recvPort = 8888

# Email info
app.config['MAIL_SERVER']='localhost'
app.config['MAIL_PORT'] = 2525
mail = Mail(app) 

lastAlarmState = "none"

sendCommand1 = [0xAA, 0xFC, 177, 0x03]
sendCommand2 = [0xAA, 0xFC, 170, 0x03]
# BCH checksum
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

# Adding checksum to command array
commandBCH = DCP_genCmndBCH(sendCommand1, 4)
sendCommand1.append(commandBCH)
commandBCH = DCP_genCmndBCH(sendCommand2, 4)
sendCommand2.append(commandBCH)

sendCommand1 = bytearray(sendCommand1)
sendCommand2 = bytearray(sendCommand2)

# Return string for temperature threshold
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

# Validates response packet from Arduino
def validateFAPacket(arr):
    recBCH = arr[4]
    checkBCH = DCP_genCmndBCH(arr, 4)
    if (recBCH == checkBCH):
        return True
    return False

# Validates data packet from Arduino
def validateDataPacket(arr):
    recBCH = arr[7]
    checkBCH = DCP_genCmndBCH(arr, 7)
    if (recBCH == checkBCH):
        return True
    return False


# Function for sending messages
def sendMessages(i, c):
    if (c == 1):
        s.sendto(sendCommand1, (recvIP, recvPort))

    elif (c == 2):
        s.sendto(sendCommand2, ('192.168.1.170', 7777)) 

returnMessage = ''

def listenUDP():
    with app.app_context():
        # global s
        while True:
            data, addr = s.recvfrom(1024)
            recIP = addr[0]
            # print(addr[0])
            # gotIP = s.getpeername()[0]
            # print(data[12])
            # print(DCP_genCmndBCH(data[5:], 7))
            # # if (validateFAPacket(data[:5]) & validateDataPacket(data[5:])):
            state = getTempState(data[8])
            temp = data[9]
            global lastAlarmState
            if (state != lastAlarmState):
                lastAlarmState = state
                msg = Message('Hello', sender = 'yourId@gmail.com', recipients = ['someone1@gmail.com'])
                msg.subject = "Alarm State " + state
                mail.send(msg)
            
            global returnMessage
            returnMessage += recIP + ' Temperature State: ' + str(state) + ' -> Temperature: '+ str(temp) + ' *F '
            # print(returnMessage)

T = Thread(target = listenUDP)
# T.setDaemon(True)
T.start()
        


    

## Sets ip address, only runs once when program is first run
@app.before_first_request
def do_something_only_once():
    s.bind((ip, port))


# Gets new values for temperature state and temperature
@app.route('/_stuff', methods = ['GET'])
def stuff():
    global returnMessage
    
    sendMessages('192.168.1.177', 1)
    sendMessages('192.168.1.170', 2)
    cpy = returnMessage
    returnMessage = ""
    return jsonify(result=cpy)



# Displays temperature state and temperature
@app.route('/')
def Home():
    
    return render_template('dy1.html')

# Runs flask
if __name__ == "__main__":
    
    app.run(debug=True)