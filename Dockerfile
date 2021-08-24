# our local base image
FROM ubuntu:20.04

LABEL description="Container for use with Visual Studio" 

# set timezone
ENV TZ=Etc/UTC 
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# install CMake
ENV version=3.21
ENV build=1
RUN apt-get update && apt-get install -y build-essential libtool autoconf unzip wget
RUN cd /tmp
RUN wget https://github.com/Kitware/CMake/releases/download/v${version}.${build}/cmake-${version}.${build}-linux-x86_64.sh
RUN mkdir /opt/cmake
RUN chmod +x cmake-$version.$build-linux-x86_64.sh
RUN yes | ./cmake-$version.$build-linux-x86_64.sh --prefix=/opt
RUN ln -s /opt/cmake-$version.$build-linux-x86_64/bin/* /usr/local/bin

# install build dependencies 
RUN apt-get update && apt-get install -y g++ rsync zip openssh-server ninja-build 
RUN apt install -y libssl-dev

# install dev dependencies 
RUN apt-get install -y vim

# configure SSH for communication with Visual Studio 
RUN mkdir -p /var/run/sshd

RUN echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config && \ 
   ssh-keygen -A 

# add files
ADD start_service.sh /tmp

# expose port 22 
EXPOSE 22
