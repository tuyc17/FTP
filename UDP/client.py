import socket
 
size = 8192
 
try:
    while True:
        msg = input()
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(msg.encode('utf-8'), ('localhost', 9876))
        print(sock.recv(size).decode('utf-8'))
        sock.close()
except Exception as e:
    print(e)
    print("cannot reach the server")