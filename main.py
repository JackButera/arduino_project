from flask import Flask, jsonify, render_template, request
from flask_mail import Mail, Message
import socket
import array
import select
import smtplib
from threading import *

app = Flask(__name__)

# Using IPv4 and UDP protocol
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# hostname and port
ip = "192.168.1.180"
port = 5000

# Email info
app.config['MAIL_SERVER']='localhost'
app.config['MAIL_PORT'] = 2525
mail = Mail(app) 

# Initializing the return message to be displayed on webpage
returnMessage = ''

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

# Generates the send command array
def genSendCommand(ip):
    sendCommand = [0xAA, 0xFC, ip, 0x04]
    commandBCH = DCP_genCmndBCH(sendCommand, 4)
    sendCommand.append(commandBCH)
    sendCommand = bytearray(sendCommand)
    return sendCommand

# Array of arduinos to send commands to
arduinoArray = [["192.168.1.177", 8888, genSendCommand(177), "none"], 
                ["192.168.1.170", 7777, genSendCommand(170), "none"]]

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


# Sends commands to all arduinos in arduino array
def sendMessages():
    for dest in arduinoArray:
        s.sendto(dest[2], (dest[0], dest[1]))


# Thread to constantly listen for udp packets
def listenUDP():
    with app.app_context():
        while True:
            data, addr = s.recvfrom(1024) # Waiting for packet
            recIP = addr[0]
            state = getTempState(data[8])
            temp = data[9]
            for ard in arduinoArray: # Sends emails for temp state
                if (recIP == ard[0]): # Checking IP (which arduino)
                    if (state != ard[3]): # Checking last temp state
                        ard[3] = state
                        msg = Message('Hello', sender = 'yourId@gmail.com', recipients = ['someone1@gmail.com'])
                        msg.subject = recIP +" Alarm State " + state
                        mail.send(msg)

            global returnMessage
            returnMessage += recIP + ' Temperature State: ' + str(state) + ' -> Temperature: '+ str(temp) + ' *F '

# Starting the listening thread
T = Thread(target = listenUDP)
T.start()

# Sets ip address, only runs when program is first run
@app.before_first_request
def do_something_only_once():
    s.bind((ip, port))


# Called every five seconds to send command messages out to arduinos and return responses
@app.route('/_stuff', methods = ['GET'])
def stuff():
    global returnMessage
    sendMessages()
    cpy = returnMessage # copies return message for return
    returnMessage = "" # clears return message for new 5 seconds
    return jsonify(result=cpy)



# Displays temperature state and temperature
@app.route('/')
def Home():
    return render_template('dy1.html')

# Runs flask
if __name__ == "__main__":
    app.run(debug=True)