FROM ubuntu:18.10
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libicu-dev \
    libboost1.67-all-dev    

RUN mkdir /forum && mkdir /forum/repos

RUN cd /forum/repos/ && git clone https://github.com/danij/Forum.git
RUN cd /forum/repos/Forum/ && git checkout initial
RUN mkdir /forum/repos/Forum-RelWithDebInfo
RUN cd /forum/repos/Forum-RelWithDebInfo/ && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo /forum/repos/Forum/ && make
 