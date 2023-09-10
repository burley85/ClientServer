CREATE DATABASE IF NOT EXISTS GroupMessaging;

USE GroupMessaging;


drop table User, GroupMessage, DirectMessage, Membership, Channel, Invitation;


CREATE TABLE IF NOT EXISTS User (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    username VARCHAR(31) NOT NULL UNIQUE,
    pword VARCHAR(32) NOT NULL,
    email VARCHAR(63) NOT NULL,
    fname VARCHAR(31),
    lname VARCHAR(31)
);

CREATE TABLE IF NOT EXISTS Channel (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    channel_name VARCHAR(31) NOT NULL
);

CREATE TABLE IF NOT EXISTS Membership (
    channel_id INT NOT NULL,
    user_id INT NOT NULL,
    /* permflags interpreted as array of 8 bools in the following order:
    delete channel, change priveleges, ban user, pin message, delete other's message,
    edit/delete own message, invite users, send message */
    perm_flags TINYINT UNSIGNED NOT NULL,
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
    message_text VARCHAR(255) NOT NULL,
    FOREIGN KEY(sender_id) REFERENCES User(id),
    FOREIGN KEY(receiver_id) REFERENCES User(id)
);

CREATE TABLE IF NOT EXISTS GroupMessage (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    message_time TIMESTAMP NOT NULL,
    sender_id INT NOT NULL,
    channel_id INT NOT NULL,
    message_text VARCHAR(255) NOT NULL,
    FOREIGN KEY(sender_id) REFERENCES User(id),
    FOREIGN KEY(channel_id) REFERENCES Channel(id)
);

CREATE TABLE IF NOT EXISTS Invitation (
    id INT AUTO_INCREMENT,
    PRIMARY KEY(id),
    sender_id INT NOT NULL,
    receiver_id INT NOT NULL,
    channel_id INT NOT NULL,
    FOREIGN KEY(sender_id) REFERENCES User(id),
    FOREIGN KEY(receiver_id) REFERENCES User(id),
    FOREIGN KEY(channel_id) REFERENCES Channel(id)
);


INSERT INTO User (username, pword, email, fname, lname) VALUES ('admin', 'admin', 'burley.85@osu.edu', 'dylan', 'burley');

/*
drop table User, GroupMessage, DirectMessage, Membership, Channel;
*/