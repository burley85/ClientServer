import mysqlx
from hashlib import sha256


class DatabaseObject:
    def __init__(self):
        pass
    def __str__(self):
        return f'{type(self).__name__} = {str(self.__dict__)}'

class DatabaseObjectList:
    def __init__(self, objType : type, objList : list = []):
        self.objType : type = objType
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
        self.id = id
    
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
        
class Channel(DatabaseObject):
    def __init__(self, name, id=None):
        self.channel_name = name
        self.id = id

    @classmethod
    def fromDict(cls, channel_dict : dict):
        if("channel_name" not in channel_dict):
            print("WARNING: Failed to create channel from dict:" + str(channel_dict))
            return None
        channel_name = channel_dict["channel_name"]
        id = channel_dict["id"] if "id" in channel_dict else None
        return Channel(channel_name, id)

class Membership(DatabaseObject):
    def __init__(self, channel_id, user_id, perm_flags):
        self.channel_id = channel_id
        self.user_id = user_id
        self.perm_flags = perm_flags

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

class Message(DatabaseObject):
    def __init__(self, sender_id, message_text, message_time, id=None):
        self.message_time = message_time
        self.message_text = message_text
        self.sender_id = sender_id
        self.id = id

class DirectMessage (Message):
    def __init__(self, sender_id, receiver_id, message_text, message_time, id=None):
        super().__init__(sender_id, message_text, message_time, id)
        self.receiver_id = receiver_id
    
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
    
class GroupMessage (Message):
    def __init__(self, sender_id, channel_id, message_text, message_time, id=None):
        super().__init__(sender_id, message_text, message_time, id)
        self.channel_id = channel_id

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
        self.sender_id = sender_id
        self.receiver_id = receiver_id
        self.channel_id = channel_id
        self.id = id

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

        return self.execute(command, *objDict.values()).get_affected_items_count() > 0

    def getUser(self, username = None, id = None):
        """Get a user from the database."""
        if(username == None and id == None): return None

        if(username != None):
            command = 'SELECT * FROM User WHERE username = ?'
            result = self.execute(command, username).fetch_all()
            
            if(len(result) != 1): return None

            if(id != None and result[0][0] != id): return None
            
        else:
            command = 'SELECT * FROM User WHERE id = ?'
            result = self.execute(command, id).fetch_all()
            
            if(len(result) != 1): return None
        
        return User(result[0][1], result[0][2], result[0][3], result[0][4], result[0][5], result[0][0])
    
    def getChannel(self, channel_id):
        """Get a channel from the database."""
        command = 'SELECT * FROM Channel WHERE id = ?'
        result = self.execute(command, channel_id).fetch_all()
        
        if(len(result) != 1): return None
        
        return Channel(result[0][1], result[0][0])

    def getLastChannel(self):
        """Get the last channel from the database."""
        command = 'SELECT * FROM Channel ORDER BY id DESC LIMIT 1;'
        result = self.execute(command).fetch_all()
        
        if(len(result) != 1): return None
        
        return Channel(result[0][1], result[0][0])
    
    def getMembership(self, channel_id, user_id):
        """Get a membership from the database."""
        command = 'SELECT * FROM Membership WHERE channel_id = ? AND user_id = ?'
        result = self.execute(command, channel_id, user_id).fetch_all()

        if(len(result) != 1): return None

        return Membership(result[0][0], result[0][1], result[0][2])
    
    def getUserChannels(self, user_id):
        """Get all channels a user is a member of."""
        command = 'SELECT * FROM Membership WHERE user_id = ?'
        result = self.execute(command, user_id).fetch_all()

        channels = DatabaseObjectList(Channel, [])

        for row in result:
            print("so far: " + str(channels))
            channel = self.getChannel(row[0])
            if(channel != None): channels.append(channel)
        
        return channels