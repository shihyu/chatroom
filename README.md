简单的聊天室程序。界面基于GTK实现。客户端使用C语言编写，服务器使用Python编写。可实现跨网段，跨NAT聊天。

运行服务器程序
```
./server.py
```

编译客户端
下载im_client-0.1.tar.gz并解压
```
tar xzvf im_client-0.1.tar.gz
cd im_client-0.1
./configure LDFLAGS=-lpthread
make
sudo make install
```

运行客户端
```
im_client
```

