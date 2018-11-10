#!/usr/bin/python
# -*- coding: utf-8 -*-

# importing the required module 
import matplotlib.pyplot as plt 

def desenhaGrafico (_x,_y,tipo):
        # x axis values 
        x =  _x
        # corresponding y axis values 
        y = _y 
          
        # plotting the points  
        plt.plot(x, y) 
          
        # naming the x axis 
        plt.xlabel('x - axis') 
        # naming the y axis 
        plt.ylabel('y - axis') 
          
        # giving a title to my graph 
        plt.title('Grafico '+tipo+ '!') 
          
        # function to show the plot 
        plt.show()

#Lê o arquivo com os dados e transforma numa lista
def lerArquivo(path):
    dataLista=[]
    dadoLista=[]
    arq = open(path, 'r')
    texto = arq.readlines()
    for linha in texto :
        data,temperatura= linha.split(',')
        dataLista.append(data)
        dadoLista.append(float(temperatura))
    arq.close()
    return [dataLista,dadoLista]

#Criar gráfico temperatura
xT,yT = lerArquivo('dados_metereologicos_temperatura.txt')

desenhaGrafico(xT,yT,'Temperatura')

#Criar gráfico temperatura
xU,yU = lerArquivo('dados_metereologicos_umidade.txt')

desenhaGrafico(xU,yU, 'Umidade')

