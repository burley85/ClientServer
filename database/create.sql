create database if not exists GroupMessaging;

use GroupMessaging;

create table if not exists User (
    id int auto_increment,
    primary key(id),
    username varchar(20) not null,
    pass varchar(20) not null,
    email varchar(20) not null,
    fname varchar(20),
    lname varchar(20)
);

create table if not exists Channel (
    id int auto_increment,
    primary key(id),
    name varchar(20) not null
);

create table if not exists Membership (
    channelid int not null,
    userid int not null,
    role varchar(20) not null,
    primary key(channelid, userid),
    foreign key(channelid) references Channel(id),
    foreign key(userid) references User(id)
);

create table if not exists DirectMessage (
    id int auto_increment,
    primary key(id),
    timestamp timestamp not null,
    senderid int not null,
    receiverid int not null,
    message varchar(256) not null,
    foreign key(senderid) references User(id),
    foreign key(receiverid) references User(id)
);

create table if not exists GroupMessage (
    id int auto_increment,
    primary key(id),
    timestamp timestamp not null,
    senderid int not null,
    channelid int not null,
    message varchar(256) not null,
    foreign key(senderid) references User(id)
);

/*
drop table User, GroupMessage, DirectMessage, Membership, Channel;
*/