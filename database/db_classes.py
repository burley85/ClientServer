import mysqlx
from hashlib import sha256

class DatabaseObject:
    def __init__(self):
        pass
    def __str__(self):
        return f'{type(self).__name__} = {str(self.__dict__)}'
    def __eq__(self, other):
        if(type(self) != type(other)): return False
        return self.__dict__ == other.__dict__

class DatabaseObjectList:
    def __init__(self, objType : type, objList : list = []):
        self.objType = objType
        self.list = objList
    
    def __str__(self):
        listDictStr = "{'objType': \"" + str(self.objType.__name__) + "\", 'list': ["
        for obj in self.list:
            print(obj)
            print(listDictStr)
            if(obj is not self.list[0]): listDictStr += ", "
            listDictStr += str(obj).split(" = ")[1]
        listDictStr += "]}"

        return f'{type(self).__name__} = {listDictStr}'
    
    def append(self, obj : DatabaseObject):
        if(type(obj) != self.objType): return False
        self.list.append(obj)
        return True
    
class User(DatabaseObject):
    def __init__(self, username, pword, email=None, fname=None, lname=None, id=None):
        self.username = username
        self.pword = pword
        self.email = email
        self.fname = fname
        self.lname = lname
        self.id = int(id) if id != None else None
    
    @classmethod
    def fromDict(cls, user_dict : dict):
        if("username" not in user_dict or "pword" not in user_dict):
            print("WARNING: Failed to create user from dict:" + str(user_dict))
            return None
        username = user_dict["username"]
        pword = user_dict["pword"]
        email = user_dict["email"] if "email" in user_dict else None
        fname = user_dict["fname"] if "fname" in user_dict else None
        lname = user_dict["lname"] if "lname" in user_dict else None
        id = user_dict["id"] if "id" in user_dict else None
        return User(username, pword, email, fname, lname, id)
        
    @classmethod
    def fromList(cls, user_list):
        if(len(user_list) != 6):
            print("WARNING: Failed to create user from list:" + str(user_list))
            return None
        return User(user_list[1], user_list[2], user_list[3], user_list[4], user_list[5], user_list[0])

class Channel(DatabaseObject):
    def __init__(self, name, id=None):
        self.channel_name = name
        self.id = int(id) if id != None else None

    @classmethod
    def fromDict(cls, channel_dict : dict):
        if("channel_name" not in channel_dict):
            print("WARNING: Failed to create channel from dict:" + str(channel_dict))
            return None
        channel_name = channel_dict["channel_name"]
        id = channel_dict["id"] if "id" in channel_dict else None
        return Channel(channel_name, id)
    
    @classmethod
    def fromList(cls, channel_list):
        if(len(channel_list) != 2):
            print("WARNING: Failed to create user from list:" + str(channel_list))
            return None
        return Channel(channel_list[1], channel_list[0])

class Membership(DatabaseObject):
    def __init__(self, channel_id, user_id, perm_flags):
        self.channel_id = int(channel_id)
        self.user_id = int(user_id)
        self.perm_flags = int(perm_flags)

    @classmethod
    def fromDict(cls, membership_dict):
        if("channel_id" not in membership_dict or "user_id" not in membership_dict
        or "perm_flags" not in membership_dict):
            print("WARNING: Failed to create membership from dict:" + str(membership_dict))
            return None
        channel_id = membership_dict["channel_id"]
        user_id = membership_dict["user_id"]
        perm_flags = membership_dict["perm_flags"]
        return Membership(channel_id, user_id, perm_flags)
    
    @classmethod
    def fromList(cls, membership_list):
        if(len(membership_list) != 3):
            print("WARNING: Failed to create user from list:" + str(membership_list))
            return None
        return Membership(membership_list[0], membership_list[1], membership_list[2])

class Message(DatabaseObject):
    def __init__(self, sender_id, message_text, message_time, id=None):
        self.message_time = message_time
        self.message_text = message_text
        self.sender_id = int(sender_id)
        self.id = int(id) if id != None else None

class DirectMessage (Message):
    def __init__(self, sender_id, receiver_id, message_text, message_time, id=None):
        super().__init__(sender_id, message_text, message_time, id)
        self.receiver_id = int(receiver_id)
    
    @classmethod
    def fromDict(cls, message_dict):
        if("sender_id" not in message_dict or "receiver_id" not in message_dict
        or "message_text" not in message_dict or "message_time" not in message_dict):
            print("WARNING: Failed to create direct message from dict:" + str(message_dict))
            return None
        sender_id = message_dict["sender_id"]
        receiver_id = message_dict["receiver_id"]
        message_text = message_dict["message_text"]
        message_time = message_dict["message_time"]
        id = message_dict["id"] if "id" in message_dict else None
        return DirectMessage(sender_id, receiver_id, message_text, message_time, id)
    
    @classmethod
    def fromList(cls, message_list):
        if(len(message_list) != 5):
            print("WARNING: Failed to create user from list:" + str(message_list))
            return None
        return DirectMessage(message_list[0], message_list[1], message_list[2])
    
class GroupMessage (Message):
    def __init__(self, sender_id, channel_id, message_text, message_time, id=None):
        super().__init__(sender_id, message_text, message_time, id)
        self.channel_id = int(channel_id)

    @classmethod
    def fromDict(cls, message_dict):
        if("sender_id" not in message_dict or "channel_id" not in message_dict
        or "message_text" not in message_dict or "message_time" not in message_dict):
            print("WARNING: Failed to create group message from dict:" + str(message_dict))
            return None
        sender_id = message_dict["sender_id"]
        channel_id = message_dict["channel_id"]
        message_text = message_dict["message_text"]
        message_time = message_dict["message_time"]
        id = message_dict["id"] if "id" in message_dict else None
        return GroupMessage(sender_id, channel_id, message_text, message_time, id)

class Invitation(DatabaseObject):
    def __init__(self, sender_id, receiver_id, channel_id, id=None):
        self.sender_id = int(sender_id)
        self.receiver_id = int(receiver_id)
        self.channel_id = int(channel_id)
        self.id = int(id) if id != None else None

    @classmethod
    def fromDict(cls, invitation_dict):
        if("sender_id" not in invitation_dict or "receiver_id" not in invitation_dict
        or "channel_id" not in invitation_dict):
            print("WARNING: Failed to create invitation from dict:" + str(invitation_dict))
            return None
        sender_id = invitation_dict["sender_id"]
        receiver_id = invitation_dict["receiver_id"]
        channel_id = invitation_dict["channel_id"]
        id = invitation_dict["id"] if "id" in invitation_dict else None
        return Invitation(sender_id, receiver_id, channel_id, id)

class Database:
    def __init__(self, host, port, user, password):
        self.host = host
        self.port = port
        self.user = user
        self.password = password
        try:
            self.session = mysqlx.get_session(host=host, port=port, user=user, password=password)
        except:
            print("ERROR: Failed to connect to mysql server.")
            exit()
        
        self.execute("USE GroupMessaging;")

    def execute(self, command, *args):
        """Execute an SQL command and return the result."""
        stmt = self.session.sql(command)
        if(len(args) > 0): stmt = stmt.bind(args)

        print(stmt.sql)
        print(args)
        return stmt.execute()

    def disconnect(self):
        """Disconnect from the database."""
        self.session.close()

    def insert(self, obj : DatabaseObject):
        """Add an object to the database."""
        objDict = {}
        for key, value in obj.__dict__.items():
            if(value != None): objDict[key] = value

        keyStr = str(objDict.keys()).removeprefix("dict_keys").replace("[", "").replace("]","").replace("\'","")
        
        command = f'INSERT INTO {obj.__class__.__name__} {keyStr} VALUES (?'
        for i in range(len(objDict) - 1):
            command += ",?"
        command += ")"

        #Execute the command
        try:
            result = self.execute(command, *objDict.values())
            if(result.get_affected_items_count() == 0): raise Exception("No rows affected.")

        except Exception as e:
            print(f'ERROR: {e}: Failed to insert object ({obj}) into database.')
            return None
        
        #Retrieve the last inserted object
        if(hasattr(obj, "id")):
            #Get id of last inserted object and query for that id
            id = result.get_autoincrement_value()
            last_object = self.queryForDict({"obj": type(obj).__name__, "id": id})
            obj.id = id
        else:
            #Query for the object that was just inserted
            objDict["obj"] = type(obj).__name__
            last_object = self.queryForDict(objDict)

        #Verify that the object retrieved is the same as the one inserted
        if(type(last_object) == DatabaseObjectList):
            print(f'ERROR: Multiple objects retrieved from database match object inserted ({obj})')
            return None
        if(last_object != obj):
            print(f'ERROR: Object retrieved from database ({last_object}) does not match object inserted ({obj})')
            return None
        return obj

    def queryForDict(self, request_dict : dict) -> DatabaseObjectList | DatabaseObject | None:
        if("obj" not in request_dict or 
           request_dict["obj"] not in globals() or
           globals()[request_dict["obj"]] not in dbObjectsTypes):
            print("WARNING: Failed to query for dict:" + str(request_dict))
            return None
        
        obj_type = globals()[request_dict["obj"]]
        request_dict.pop("obj")


        # TODO: FIX THIS
        # if "user_id" in request_dict:
        #     #Depending on the object type, the user_id may be used for a different purpose
        #     if(obj_type == User): 
        #         if("id" not in request_dict):
        #             request_dict["id"] = request_dict.pop("user_id")
        #     elif(obj_type == Membership): pass
        #     elif(obj_type == GroupMessage):
        #         if("sender_id" not in request_dict):
        #             request_dict["sender_id"] = request_dict.pop("user_id")
        #     request_dict.pop("user_id")

        command = "SELECT * FROM " + obj_type.__name__
        values = []
        if(len(request_dict) > 0):
            command += " WHERE "
            for key, value in request_dict.items():
                command += key + " = ? AND "
                values.append(value)
            command = command.removesuffix(" AND ")
        result = self.execute(command, *values)
        rows = result.fetch_all()
        
        #Create a list of dicts from the result
        objDictList = []
        for row in rows:
            objDict = {}
            for i in range(len(row._fields)):
                label = result.columns[i].column_label
                objDict[label] = row[i]
            objDictList.append(objDict)

        if(len(rows) == 0): return None
        if(len(objDictList) == 1): return obj_type.fromDict(objDictList[0])
        #Create a list of objects from the list of dicts
        objList = []
        for objDict in objDictList:
            objList.append(obj_type.fromDict(objDict))
        return DatabaseObjectList(obj_type, objList)

dbObjectsTypes = [User, Channel, Membership, DirectMessage, GroupMessage, Invitation]

def databaseObjectFromDict(obj_dict : dict) -> DatabaseObject | None:
    if("obj" not in obj_dict or
    obj_dict["obj"] not in globals() or
    globals()[obj_dict["obj"]] not in dbObjectsTypes):
        print("WARNING: Failed to create object from dict:" + str(obj_dict))
        return None
    
    obj_type : type = globals()[obj_dict["obj"]]
    return obj_type.fromDict(obj_dict)