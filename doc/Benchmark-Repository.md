# Memory Repository Benchmark

A benchmark has been conducted on the memory repository.

## Environment

The benchmark was conducted on a virtual machine with a 2.4 GHz core, <2 GB RAM and running 64-bit Linux 4.8.0. 

The code was compiled with GCC 6.2.0 and the following flags `-std=c++14 -Wall -Werror -O3 -DNDEBUG`

## Test Data

The benchmark generated a set of random data.

| |Number of|
|:---|---:|
|Users|10 000|
|Discussion threads|10 000|
|Discussion messages (with a mean length of 1.000 bytes, σ = 200)|500 000|
|Discussion tags|100|
|Discussion categories| 100|
|Discussion category parent-child relationships|20|
|Discussion tags/category|1 .. 4|
|Discussion tags/thread|1 .. 4| 

> No IP or browser user-agent information has been included in the benchmarks

## Results

After populating the repository with the above data, the application's memory usage was `VmData:   938252 kB`

The following table shows how many requests could be processed by the repository. 
Requests consists of sending a command id, a vector of arguments and waiting for a JSON-serialized reply to be written 
to a memory stream.

Each operation has been performed 1 000 times. Spikes greater than 10 times the average have been ignored.

| |Operations / second|σ|
|---|---:|---:|
|Adding a new user|96 427|14 866|
|Adding a new discussion thread|66 615|10 359|
|Adding a new message to an existing discussion thread|8 219|2 365|
|Get first page of users by name|51 276|7 218|
|Get fourth page of users by name|50 639|6 882|
|Get first page of users by last seen|51 761|5 280|
|Get fourth page of users by last seen|52 341|8 303|
|Get first page of discussion threads by name|3 578| 962|
|Get fourth page of discussion threads by name|3 613|1 057|
|Get first page of discussion threads by message count descending|2 939| 902|
|Get fourth page of discussion threads by message count descending|3 112| 966|
|Get first page of messages of discussion threads|10 413|1 557|
|Get first page of discussion threads with tag by name|2 631| 910|
|Get first page of discussion threads of category by name|2 408| 826|
