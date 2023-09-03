FROM ubuntu:20.04

USER root

RUN apt-get update -y

RUN export DEBIAN_FRONTEND=noninteractive
RUN ln -fs /usr/share/zoneinfo/Australia/Canberra /etc/localtime

RUN apt-get install -y tzdata python3 python3-pip build-essential libprocps-dev man-db git
RUN dpkg-reconfigure --frontend noninteractive tzdata

RUN apt-get install -y valgrind\
	&& yes | unminimize\
	&& pip3 install --upgrade pip\
	&& pip3 install colour-valgrind
