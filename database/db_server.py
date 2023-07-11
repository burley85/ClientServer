import socket
from db_classes import User, Channel, Membership, Message, DirectMessage, GroupMessage, Database

"""Middleman server for database API"""
db = Database("localhost", 33060, "root", "mysql")

host = "192.168.1.232"
port = 9001

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((host, port))
sock.listen()
print(f"Listening on {host}:{port}")
while True:
    conn, addr = sock.accept()
    with conn:
        print(f"Connected by {addr}")
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print("Received data: " + data.decode('utf-8'))
            conn.sendall(data)