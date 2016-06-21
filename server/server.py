#!/usr/bin/python
#coding=utf8
import socket, threading, time, commands
links = {}

def tcplink(sock, addr):
    global links
    print 'Accept new connection from %s: %s...' % addr
    name = ''
    #数据格式 Type:|Nickname:|Time:|Length:|Data
    try:
        while True:
            message = sock.recv(1024)
            print message

            data = message.split(':|')
            #验证数据包格式为5部分，且校验消息数据长度
            if len(data) != 5 or len(data[4]) != int(data[3]):
                sock.send('format error')
                continue

            #心跳包
            if data[0] == 'heart':
                sock.send('heartbeat')

            #消息数据
            elif data[0] == 'message':
                if not links.has_key(data[1]):
                    sock.send('who r u?')
                    continue
                #广播消息,包括自己
                for item in links:
                    links[item].send(message)

            #注销
            elif data[0] == 'logout':
                if links.has_key(data[1]):
                    1/0
                    #del links[data[1]]
                    #sock.send('logout')
                    #sock.close()

            #登陆
            elif data[0] == 'login':
                if links.has_key(data[1]):
                    sock.send('failed')
                else:
                    info = "message:|system:|%d:|%d:|%s joins here" % (int(time.time()), len(data[1])+11, data[1])
                    for item in links:
                        links[item].send(info)
                    name = data[1]
                    links[name] = sock
                    sock.send('success')
    except:
        if len(name) > 0 and  links.has_key(name):
            del links[name]
            info = "message:|system:|%d:|%d:|%s leaves here" % (int(time.time()), len(name)+11, name)
            for item in links:
                links[item].send(info)
    sock.close()
    print 'Connection from %s:%s closed.' % addr

def run():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('127.0.0.1', 2333))
    s.listen(10)
    print 'Waiting for connection...'

    while True:
        sock, addr = s.accept()
        t = threading.Thread(target=tcplink, args=(sock, addr))
        t.start()

if __name__ == '__main__':
    try:
        run()
    except KeyboardInterrupt:
        pass
