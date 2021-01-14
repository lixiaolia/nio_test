#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define LOCALHOST       "0.0.0.0"
#define SERV_PORT       (6666)
#define MAX_EVENT       (128)
#define MAX_BUFSIZE     (512)

int setNonBlocking(int p_nSock)
{   
    int nOpts;
    nOpts = fcntl(p_nSock, F_GETFL);
    if(nOpts < 0)
    {   
        printf("[%s %d] Fcntl Sock GETFL fail!\n", __FUNCTION__, __LINE__);
        return -1;
    }   

    nOpts = nOpts | O_NONBLOCK;   
    if(fcntl(p_nSock, F_SETFL, nOpts) < 0)   
    {  
        printf("[%s %d] Fcntl Sock SETFL fail!\n", __FUNCTION__, __LINE__);
        return -1;   
    } 

    return 0;
}

int main(int argc,char *argv[])
{
    int sock_fd;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd <= 0) {
        printf("[%s %d] Socket Create fail return:%d!\n", __FUNCTION__, __LINE__, sock_fd);
        return 0;
    }

    struct sockaddr_in addr_serv; // 服务器地址
    memset(&addr_serv, 0x0, sizeof(addr_serv));

    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(SERV_PORT);
    addr_serv.sin_addr.s_addr = inet_addr(LOCALHOST);

    if (setNonBlocking(sock_fd) < 0) { return 0; }

    if (connect(sock_fd, (struct sockaddr *)&addr_serv, sizeof(struct sockaddr)) < 0)
    {
        printf("[%s %d] Client connect errno:%d!\n", __FUNCTION__, __LINE__, errno);
        if (errno != EINPROGRESS)
        {// 非阻塞，返回EINPROGRESS是正常的
            printf("[%s %d] Connnect Remote Server fail.\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    int nEpollfd;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENT];
    nEpollfd = epoll_create(MAX_EVENT);
    if (nEpollfd <= 0) {
        printf("[%s %d] Epoll Create fail return:%d!\n", __FUNCTION__, __LINE__, nEpollfd);
        return 0;
    }

    ev.data.fd = sock_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    if (epoll_ctl(nEpollfd, EPOLL_CTL_ADD, sock_fd, &ev) < 0)
    {
        printf("[%s %d] Epoll ctl error!\n", __FUNCTION__, __LINE__);
        return 0;
    }

    int i;
    int nEventNum;
    int nSendNum;
    int nRecvNum;
    char szSendBuf[] = "NonBlocking Epoll Test!";
    char szRecvBuf[MAX_BUFSIZE];

    for (;;)
    {
        nEventNum = epoll_wait(nEpollfd, events, MAX_EVENT, -1);
        printf("events num:%d\n", nEventNum);

        for (i = 0; i < nEventNum; i++)
        {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
            {// 服务端异常关闭
                printf("[%s %d] Epoll Event Error!\n", __FUNCTION__, __LINE__);
                close (events[i].data.fd);  
                continue;
            }

            if (events[i].events & EPOLLOUT)
            {
                printf("[%d] EPOLLOUT Event.\n", __LINE__);
                if ((sock_fd = events[i].data.fd) < 0) { continue; }

                nSendNum = send(sock_fd, szSendBuf, sizeof(szSendBuf), 0);
                if (nSendNum < 0)
                {
                    printf("[%s %d] Send Error", __FUNCTION__, __LINE__);
                    return 0;
                }

                // 修改sock_fd上要处理的事件为EPOLLIN
                ev.data.fd = sock_fd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(nEpollfd, EPOLL_CTL_MOD, sock_fd, &ev);
            }
            else if (events[i].events & EPOLLIN)
            {
                printf("[%d] EPOLLIN Event.\n", __LINE__);
                if ((sock_fd = events[i].data.fd) < 0) { continue; }

                memset(szRecvBuf, 0x0, sizeof(szRecvBuf));
                nRecvNum = recv(sock_fd, szRecvBuf, sizeof(szRecvBuf), 0);
                if (nRecvNum < 0)
                {
                    printf("[%s %d] Client Recv Data Error!\n", __FUNCTION__, __LINE__);
                    return 0;
                }

                printf("\n******************************\n");
                printf("**RecvData: %s\n", szRecvBuf);
                printf("******************************\n");

                // 修改sock_fd上要处理的事件为EPOLLOUT
                // ev.data.fd = sock_fd;
                // ev.events = EPOLLOUT | EPOLLET;
                // epoll_ctl(nEpollfd, EPOLL_CTL_MOD, sock_fd, &ev);

                // 只收一次，结束循环
                close(sock_fd);
                return 0;
                // printf("**exit..\n");
            }
        }
    }

    close(sock_fd);
    return 0;
}