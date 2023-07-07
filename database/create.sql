CREATE DATABASE IF NOT EXISTS GroupMessaging;

USE GroupMessaging;

CREATE TABLE IF NOT EXISTS User (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    username VARCHAR(20) NOT NULL,
    pass VARCHAR(20) NOT NULL,
    email VARCHAR(20) NOT NULL,
    fname VARCHAR(20),
    lname VARCHAR(20)
);

CREATE TABLE IF NOT EXISTS Channel (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    channel_name VARCHAR(20) NOT NULL
);

CREATE TABLE IF NOT EXISTS Membership (
    channel_id INT NOT NULL,
    user_id INT NOT NULL,
    /* permflags interpreted as array of 8 bools in the following order:
    delete channel, change priveleges, ban user, pin message, delete other's message,
    edit/delete own message, invite users, send message */
    perm_flags TINYINT NOT NULL,
    PRIMARY KEY(channel_id, user_id),
    FOREIGN KEY(channel_id) REFERENCES Channel(id),
    FOREIGN KEY(user_id) REFERENCES User(id)
);

CREATE TABLE IF NOT EXISTS DirectMessage (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    message_time TIMESTAMP NOT NULL,
    sender_id INT NOT NULL,
    receiver_id INT NOT NULL,
    message_text VARCHAR(256) NOT NULL,
    FOREIGN KEY(sender_id) REFERENCES User(id),
    FOREIGN KEY(receiver_id) REFERENCES User(id)
);

CREATE TABLE IF NOT EXISTS GroupMessage (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    message_time TIMESTAMP NOT NULL,
    sender_id INT NOT NULL,
    channel_id INT NOT NULL,
    message_text VARCHAR(256) NOT NULL,
    FOREIGN KEY(sender_id) REFERENCES User(id)
    FOREIGN KEY(channel_id) REFERENCES Channel(id)
);

INSERT INTO TABLE User (username, pass, email, fname, lname) VALUES ('admin', 'admin', 'burley.85@osu.edu', 'dylan', 'burley');

/*
drop table User, GroupMessage, DirectMessage, Membership, Channel;
*/