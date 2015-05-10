HttpDownloadServer
==================
A cross-platform http static server based on boost asio example http server2. Now it's supporting large file(> 4G) download.

how to build?
============

Boost should be installed.

windows
-------
Open .sln in vs2008, change include dir to yours, then compile.

linux
-----
```shell
g++ *.cpp -o server -pthread  -lboost_system -lboost_filesystem -lboost_thread -static -O2 -DNDEBUG
```

