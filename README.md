# Chatroom

## Getting Started

<<<<<<< HEAD
This chatroom is a simple chatroom written in c++, socket amd multithread. 
=======
This is a simple chatroom written in c++, socket amd multithread. This is the second project of the 108-2 Operating System course.
>>>>>>> f4c0239b1e374c0a9261220bbdfa07b5db8df076

### Build

The makefile provided under the repo will create ```bin``` and ```obj``` directory. The compiled object file will be stored in ```obj``` and the executables will be stored in ```bin```.

Build command:
```
make
```

### Run server

```
cd ./bin
./server
```

### Run client

The first argument of client is the ipaddress to the server and the second argument is the user name. The maximum length of user name is 16 characters.
```
cd ./bin
./client ${IPaddress to server} ${name}
```

## Authors

* **CY Yang** - [NekoSaiKou](https://github.com/NekoSaiKou)
* **Peter**   - [peter1719](https://github.com/peter1719)
