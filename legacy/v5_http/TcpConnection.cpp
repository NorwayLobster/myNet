 ///
 /// @file    TcpConnection.cc
 /// @author  lemon(haohb13@gmail.com)
 /// @date    2015-11-05 17:02:41
 ///

#include "TcpConnection.h"
#include "EpollPoller.h"
#include <string.h>
#include <stdio.h>


namespace wd
{

TcpConnection::TcpConnection(int sockfd, EpollPoller * loop)
: sockfd_(sockfd)
, sockIO_(sockfd)
, localAddr_(wd::Socket::getLocalAddr(sockfd))
, peerAddr_(wd::Socket::getPeerAddr(sockfd))
, isShutdownWrite_(false)
, loop_(loop)
{
	sockfd_.nonblock();
}


TcpConnection::~TcpConnection()
{
	if(!isShutdownWrite_)
	{
		isShutdownWrite_ = true;
		shutdown();
	}
	printf("~TcpConnection()\n");
}

std::string TcpConnection::receive()
{
	char buf[65536];
	memset(buf, 0, sizeof(buf));
	size_t ret = sockIO_.readline(buf, sizeof(buf));
	if(ret == 0)
	{
		return std::string();
	}
	else
		return std::string(buf);
}

void TcpConnection::send(const std::string & msg)
{
	sockIO_.writen(msg.c_str(), msg.size());
}

void TcpConnection::send(Buffer * buf)
{
	send(buf->retrieveAllAsString());
}

//针对php服务器
void TcpConnection::sendAndClose(const std::string & msg)
{
	send(msg);
	shutdown();
}

//它是在计算线程里面调用的
void TcpConnection::sendInLoop(const std::string & msg)
{
	loop_->runInLoop(std::bind(&TcpConnection::sendAndClose, this, msg));
}

void TcpConnection::shutdown()
{
	if(!isShutdownWrite_)
		sockfd_.shutdownWrite();
	isShutdownWrite_ = true;
}

std::string TcpConnection::toString()
{
	char str[100];
	snprintf(str, sizeof(str), "%s:%d -> %s:%d",
			 localAddr_.ip().c_str(),
			 localAddr_.port(),
			 peerAddr_.ip().c_str(),
			 peerAddr_.port());
	return std::string(str);
}


void TcpConnection::setConnectionCallback(const ConnectionCallback & cb)
{
	onConnectionCb_ = cb;
}

void TcpConnection::setMessageCallback(const MessageCallback & cb)
{
	onMessageCb_ = cb;
}

void TcpConnection::setCloseCallback(const CloseCallback & cb)
{
	onCloseCb_ = cb;
}

void TcpConnection::handleConnectionCallback()
{
	if(onConnectionCb_)
		onConnectionCb_(shared_from_this());
}

void TcpConnection::handleRead()
{
	int saveErrno = 0;
	ssize_t n = _inputBuffer.readFd(sockfd_.fd(), &saveErrno);
	if(n > 0 && onMessageCb_)
		onMessageCb_(shared_from_this(), &_inputBuffer);
}

void TcpConnection::handleCloseCallback()
{
	if(onCloseCb_)
		onCloseCb_(shared_from_this());
}

}// end of namespace wd
