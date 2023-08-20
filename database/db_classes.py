import mysqlx
from hashlib import sha256

class DatabaseObject:
    def __init__(self):
        pass
    def __str__(self):
        return f'{type(self).__name__} = {str(self.__dict__)}'

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

    def getUser(self, username):
        """Get a user from the database."""
        command = 'SELECT * FROM User WHERE username = ?'
        result = self.execute(command, username).fetch_all()
        
        if(len(result) > 1): print("WARNING: Multiple users with the same username.")
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