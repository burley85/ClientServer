import mysqlx
from hashlib import sha256

class User:
    def __init__(self, username, password, email=None, fname=None, lname=None, id=None):
        self.username = username
        self.hashed_password = sha256(password.encode('utf-8')).hexdigest()
        self.email = email
        self.fname = fname
        self.lname = lname
        self.id = id

class Channel:
    def __init__(self, name, id=None):
        self.channel_name = name
        self.id = id

class Membership:
    def __init__(self, channel_id, user_id, perm_flags):
        self.channel_id = channel_id
        self.user_id = user_id
        self.perm_flags = perm_flags

class Message:
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
        self.session = mysqlx.get_session(host=host, port=port, user=user, password=password)

    def execute(self, command):
        """Execute an SQL command and return the result."""
        self.session.sql(command).execute()

    def disconnect(self):
        """Disconnect from the database."""
        self.session.close()

    def insert(self, obj):
        """Add an object to the database."""
        command = f'INSERT INTO {obj.__class__.__name__} {str(obj.__dict__.keys()).removeprefix("dict_keys").replace("[", "").replace("]","")} VALUES {str(obj.__dict__.values()).removeprefix("dict_values").replace("[", "").replace("]","")}'
        self.execute(command)