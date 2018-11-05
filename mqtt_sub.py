#!/usr/bin/python
# -*- coding: utf-8 -*-

import serial # if you have not already done so
import paho.mqtt.client as mqtt
import time


    

broker = "iot.eclipse.org"
contexto1 = "casa134/quarto/temperatura"
contexto2 = "casa134/quarto/umidade"

#arduino = serial.Serial("COM9",9600)


def on_message_bytes(mosq, obj, msg):
    # This callback will only be called for messages with topics that match
    # $SYS/broker/bytes/#
    data = time.strftime("%d/%m/%Y")

    escrevaCSV(data,str(msg.payload))
    print("Mensagem: " + str(msg.payload))

    '''
    if str(msg.payload)=="1":
        print ("The LED is on...")
        time.sleep(1) 
        arduino.write('H') 
        
    if str(msg.payload)=="0":
        print ("The LED is Off...")
        time.sleep(1) 
        arduino.write('L') 
    '''    
        
def escrevaCSV (data, valor):
   file = open("dados_metereologicos.txt","a")
   file.write(data+","+valor+"\n")
   file.close()
    

mqttc = mqtt.Client()

#Chamada do callback para retornar a mensagem 
mqttc.message_callback_add(contexto1, on_message_bytes)
mqttc.message_callback_add(contexto2, on_message_bytes)

mqttc.connect(broker, 1883, 60)

#Incrição no contexto que será onbservado
mqttc.subscribe(contexto1, 0)
mqttc.subscribe(contexto2, 0)

mqttc.loop_forever()
