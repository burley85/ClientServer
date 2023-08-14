"""Middleman server for database API"""

import socket
import sys
from db_classes import User, Channel, Membership, Message, DirectMessage, GroupMessage, Database, DatabaseObject
import traceback
import urllib.parse

fp = open("APIServer.log", "w")
fp = sys.stdout

def request_bytes_to_dict(request : bytes):
    """Converts query string to dictionary"""
    query_str = urllib.parse.unquote(request)
    request_dict = {}
    for pair in query_str.split("&"):
        key, value = pair.split("=")
        request_dict[key] = value

    print(request_dict, file = fp)
    return request_dict

def handle_request(db : Database, type, request_dict) -> DatabaseObject | None:
    """Handles generic request from client"""
    response_obj = None

    if type == "login": response_obj = handle_login_request(db, request_dict)
    if type == "register": response_obj = handle_register_request(db, request_dict)
    
    if response_obj: print(response_obj, file = fp)
    return response_obj

def handle_login_request(db : Database, request_dict : dict):
    """Handles login request from client"""
    username = request_dict["username"]
    pword = request_dict["pword"]
    user = db.getUser(username, pword)
    return user

def handle_register_request(db : Database, request_dict: dict):
    """Handles register request from client"""
    username = request_dict["username"]
    pword = request_dict["pword"]
    email = request_dict["email"].replace("%40", "@")
    fname = request_dict["fname"]
    lname = request_dict["lname"]
    user = User(username, pword, email, fname, lname)
    db.insert(user)
    return user

def send_response(conn, response_obj : DatabaseObject | None):
        conn.sendall(bytes(str(response_obj), 'utf-8'))

def main():
    host = sys.argv[1]
    port = int(sys.argv[2])
    db = Database("localhost", 33060, "root", "mysql")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((host, port))
    sock.listen()
    print(f"Listening on {host}:{port}", file = fp)

    while True:
        conn, addr = sock.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                api_request = conn.recv(1024)
                if not api_request:
                    break
                print("Received request: " + api_request.decode('utf-8'), file = fp)
                
                request_dict = request_bytes_to_dict(api_request)
                response_object = handle_request(db, request_dict["type"], request_dict)

                send_response(conn, response_object)

if(__name__ == "__main__"):
    #Catch any exceptions before exiting
    try:
        main()
    except Exception as e:
        traceback.print_exception(type(e), e, e.__traceback__)

    pause = input("Press enter to continue...")
