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
    if type == "get_channels": response_obj = handle_get_channels_request(db, request_dict)
    if type == "create_channel": response_obj = handle_create_channel_request(db, request_dict)
    if type == "join_channel": response_obj = handle_join_channel_request(db, request_dict)


    if response_obj: print(response_obj, file = fp)
    return response_obj

def handle_login_request(db : Database, request_dict : dict):
    """Handles login request from client"""
    username = request_dict["username"]
    pword = request_dict["pword"]

    user = db.getUser(username)
    if(user == None or user.pword != pword):
        return None
    return user

def handle_register_request(db : Database, request_dict: dict):
    """Handles register request from client"""
    username = request_dict["username"]
    
    #Check if username is already taken
    if(db.getUser(username) != None):
        print("Warning: Username already taken", file = fp)
        return None
    
    pword = request_dict["pword"]
    email = request_dict["email"].replace("%40", "@")
    fname = request_dict["fname"]
    lname = request_dict["lname"]
    user = User(username, pword, email, fname, lname)
    if(not db.insert(user)): return None
    user = db.getUser(username)
    if(not user): print("Warning: Failed to register user with username " + username , file = fp)
    return user

def handle_get_channels_request(db : Database, request_dict : dict):
    user = db.getUser(request_dict["username"])
    if(not user):
        print("Warning: Failed to find user with username " + request_dict["username"], file = fp)
        return None

    return db.getChannels(user.id)

def handle_create_channel_request(db : Database, request_dict : dict):
    """Handles create channel request from client"""
    channel_name = request_dict["channelName"]
    owner = db.getUser(request_dict["username"])
    if(not owner):
        print("Warning: Failed to find user with username " + request_dict["username"], file = fp)
        return None
    
    #Add channel
    channel = Channel(channel_name)
    if(db.insert(channel)): channel = db.getChannel(channel_name)
    if(not channel or channel.id == None):
        print("Warning: Failed to create channel with name " + channel_name, file = fp)
        return None

    #Add user-channel membership
    perms = int("11111111", 2)
    ownership = Membership(channel.id, owner.id, perms)
    if(not db.insert(ownership)):
        print(f'Warning: Failed to add user-channel membership for owner {owner.username} and channel {channel.channel_name}', file = fp)
        return None
    
    return channel

def handle_join_channel_request(db : Database, request_dict : dict):
    """Handles join channel request from client"""
    perms = 0
    if("perms" in request_dict): perms = request_dict["perms"]
    channel = db.getChannel(request_dict["channelID"])
    if(not channel):
        print("Warning: Failed to find channel with id " + request_dict["channelID"], file = fp)
        return None
    user = db.getUser(request_dict["username"])
    if(not user):
        print("Warning: Failed to find user with username " + request_dict["username"], file = fp)
        return None

    #Add user-channel membership
    membership = Membership(channel.id, user.id, perms)
    if(not db.insert(membership)): return None
    membership = db.getMembership(channel.id, user.id)
    if(not membership):
        print(f'Warning: Failed to add user-channel membership for user {user.username} and channel {channel.channel_name}', file = fp)
        return None

    return membership

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
