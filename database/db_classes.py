import mysqlx
from hashlib import sha256

class DatabaseObject:
    def __init__(self):
        pass
    def __str__(self):
        return str(self.__dict__)

class User(DatabaseObject):
    def __init__(self, username, pword, email=None, fname=None, lname=None, id=None):
        self.username = username
        self.pword = pword
        self.email = email
        self.fname = fname
        self.lname = lname
        self.id = id

class Channel(DatabaseObject):
    def __init__(self, name, id=None):
        self.channel_name = name
        self.id = id

class Membership(DatabaseObject):
    def __init__(self, channel_id, user_id, perm_flags):
        self.channel_id = channel_id
        self.user_id = user_id
        self.perm_flags = perm_flags

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

class GroupMessage (Message):
    def __init__(self, sender_id, channel_id, message_text, message_time, id=None):
        super().__init__(sender_id, message_text, message_time, id)
        self.channel_id = channel_id

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

    def execute(self, command):
        """Execute an SQL command and return the result."""
        print(command)
        self.session.sql(command).execute()

    def disconnect(self):
        """Disconnect from the database."""
        self.session.close()

    def insert(self, obj : DatabaseObject):
        """Add an object to the database."""
        objDict = {}
        for key, value in obj.__dict__.items():
            if(value != None): objDict[key] = value

        keyStr = str(objDict.keys()).removeprefix("dict_keys").replace("[", "").replace("]","").replace("\'","")
        valStr = str(objDict.values()).removeprefix("dict_values").replace("[", "").replace("]","")
        command = f'INSERT INTO {obj.__class__.__name__} {keyStr} VALUES {valStr}'
        self.execute(command)

    def getUser(self, username, pword):
        """Get a user from the database. Return None if username and password do not match."""
        command = f'SELECT * FROM User WHERE username="{username}" AND pword="{pword}"'
        result = self.session.sql(command).execute().fetch_all()
        if(len(result) == 0):
            return None
        if(len(result) > 1):
            raise Exception("Multiple users with the same username and pword.")
        return User(result[0][1], result[0][2], result[0][3], result[0][4], result[0][5], result[0][0])