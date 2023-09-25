"""Middleman server for database API"""

import socket
import sys
from db_classes import *
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

def handle_request(db : Database, request_dict : dict) -> DatabaseObject | DatabaseObjectList | None:
    """Handles generic request from client"""
    if(request_dict["cmd"] == "create"): 
        request_dict.pop("cmd")
        #Convert request_dict to DatabaseObject
        obj = databaseObjectFromDict(request_dict)
        #Insert object into database and return success/fail
        if(obj is not None): return db.insert(obj)
        return None

    elif(request_dict["cmd"] == "read"): 
        request_dict.pop("cmd")
        return db.queryForDict(request_dict)

    elif(request_dict["cmd"] == "update"):
        print("WARNING: 'update' commands not implemented")
        return None
    elif(request_dict["cmd"] == "delete"): 
        print("WARNING: 'delete' commands not implemented")
        return None

    else: 
        print("WARNING: Invalid command: " + request_dict["cmd"])
        return None

def send_response(conn : socket.socket, response_obj : DatabaseObject | DatabaseObjectList | None | tuple[DatabaseObject, DatabaseObject]):
        #Format: "<Response_Length>\n<Response>"
        response_str = str(response_obj)
        length = len(response_str)
        response_str = str(length) + "\n" + response_str
        conn.sendall(bytes(response_str, 'utf-8'))

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
                try:
                    api_request = conn.recv(1048576)
                    if not api_request:
                        break
                    print("Received request: " + api_request.decode('utf-8'), file = fp)
                    
                    request_dict = request_bytes_to_dict(api_request)
                    if("cmd" in request_dict and "obj" in request_dict):
                        response_object = handle_request(db, request_dict)
                        if(type(response_object) == tuple):
                            response_object[0].__dict__.update(response_object[1].__dict__)
                            response_object = response_object[0].__dict__
                    else:
                        response_object = None
                        print("Warning: Invalid request: " + str(request_dict), file = fp)

                    print("Sending response: " + str(response_object))
                    send_response(conn, response_object)
                except Exception as e:
                    traceback.print_exception(type(e), e, e.__traceback__)

if(__name__ == "__main__"):
    #Catch any exceptions before exiting
    try:
        main()
    except Exception as e:
        traceback.print_exception(type(e), e, e.__traceback__)

    pause = input("Press enter to continue...")
