import socket
from db_classes import User, Channel, Membership, Message, DirectMessage, GroupMessage, Database
import sys

"""Middleman server for database API"""
host = sys.argv[1]
port = int(sys.argv[2])
db = Database("localhost", 33060, "root", "mysql")

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
    pause = input("Press enter to continue...")
    