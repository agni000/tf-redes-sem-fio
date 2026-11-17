import socket
import datetime
import csv
import os

HOST = '0.0.0.0'
PORT = 9090
CSV_FILENAME = "medicoes_rede.csv"

# Cria o arquivo CSV se não existir
if not os.path.exists(CSV_FILENAME):
    with open(CSV_FILENAME, mode='a', newline='') as file:
        writer = csv.writer(file, delimiter=';')
        writer.writerow(["Instante_Chegada", "Dados_do_Pacote"])

# SOCK_DGRAM indica que estamos usando UDP
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
    server.bind((HOST, PORT))
    print(f"Coletor C aguardando dados via UDP na porta {PORT}...")
    
    while True:
        # Recebe os dados e o endereço (IP, Porta) de quem enviou
        data, addr = server.recvfrom(1024)
        texto = data.decode('utf-8').strip()
        
        instante_chegada = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        print(f"[{instante_chegada}] Chegou de {addr}: {texto}")
        
        with open(CSV_FILENAME, mode='a', newline='') as file:
            writer = csv.writer(file, delimiter=';')
            writer.writerow([instante_chegada, texto])
        
        # Devolve a resposta usando o mesmo protocolo sem conexão
        server.sendto(b"ACK_DO_LINUX\n", addr)
