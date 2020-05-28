# server_client_chat_v1
A simple server client chat

# OS
Ubuntu 18.04.4 LTS
Linux 4.15.0-99-generic

# GCC 
gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0

# Participantes 
Guilherme de Pinho Montemovo        nºUSP 9779461

# Para utilizar o programa:

Primeiro execute o comando:
- make

Para executar o client 1:
- make run_client1

Para executar o client 2 (em um novo terminal):
- make run_client2
- ip a ser conectado

# Comentários

O client 1 atua como servidor, no caso ele conecta a todas interfaces disponíves na máquina.

O client 2 necessida de um IP (do client 1) para se conectar. No caso de conexão local, utilizar o IP do localhost (Ex. 0.0.0.0).
