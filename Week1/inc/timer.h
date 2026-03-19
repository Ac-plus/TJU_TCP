#include "global.h"
#include "tju_packet.h"
#include <unistd.h>
#include "tju_tcp.h"

void clt_conn_timeout_handler(int signo, tju_tcp_t *sock)
{
    printf("CLIENT CONN TIME OUT!!!!\n");

   // sock->state = CLOSED;

    RETRANS = 1;
    TIMEOUT_FLAG = 1;
    printf("clt_conn_timeout_handler:进入客户端重传函数\n");
    printf("clt_conn_timeout_handler socket状态为%d\n", sock->state);

    sock->is_retransing = true;

    if (1) {  //重传第一个SYN
        tju_sock_addr local_addr, target_addr;
        target_addr.ip = inet_network("172.17.0.3");
        target_addr.port = 1234;
        local_addr.ip = inet_network("172.17.0.2");
        local_addr.port = 5678; //连接方进行connect连接的时候 内核中是随机分配一个可用的端口
        sock->established_local_addr = local_addr;
        sock->established_remote_addr = target_addr;

        // 下面是补充的部分
        uint32_t seq = CLIENT_CONN_SEQ;
        tju_packet_t *syn_packet = create_packet(local_addr.port, target_addr.port, seq, 0,
                                                        DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, SYN_FLAG_MASK, 1, 0, NULL, 0);
        char *msg1 = packet_to_buf(syn_packet);
        sendToLayer3(msg1, DEFAULT_HEADER_LEN);
        printf("重发SYN报文\n");
        start_clt_conn_timer(&sock);

        //sock->state = SYN_SENT;  /////////////////////////////
        int hashval = cal_hash(local_addr.ip, local_addr.port, target_addr.ip, target_addr.port);
        established_socks[hashval] = sock;
        return;
    }

    return;
}

void srv_conn_timeout_handler(int signo, tju_tcp_t *sock)
{
    printf("SERVER CONN TIME OUT!!!!\n");

    // sock->state = LISTEN;

    RETRANS = 1;
    TIMEOUT_FLAG = 1;
    printf("srv_conn_timeout_handler:进入服务端重传函数\n");
    printf("srv_conn_timeout_handler:socket状态为%d\n", sock->state);

    sock->is_retransing = true;

    
    if (1) {  //重传第二个SYN_ACK
        tju_tcp_t *new_conn = (tju_tcp_t *)malloc(sizeof(tju_tcp_t));
        memcpy(new_conn, sock, sizeof(tju_tcp_t));

        tju_sock_addr local_addr, remote_addr;
        remote_addr.ip = inet_network("172.17.0.2"); //具体的IP地址
        remote_addr.port = 5678;           //端口
        local_addr.ip = sock->bind_addr.ip;     //具体的IP地址
        local_addr.port = sock->bind_addr.port; //端口

        new_conn->established_local_addr = local_addr;
        new_conn->established_remote_addr = remote_addr;

        // 将连接socket的状态改为SYN_RECV 并存入LISTEN状态的socket的半连接队列中
        new_conn->state = SYN_RECV;
        enQueue(sock->incomplete_conn_queue, new_conn);

        // 向客户端发送SYN_ACK
        uint32_t seq = SERVER_CONN_SEQ;
        //uint32_t ack = get_seq(pkt) + 1;
        uint32_t ack = CLIENT_CONN_SEQ + 1;   //pkt_seq+1   
        uint8_t flags = SYN_FLAG_MASK | ACK_FLAG_MASK;  //////11111111? 00010010 01001000 
                                                            //两个标志位的掩码取或即可得到SYN&ACK==1的flags
        tju_packet_t *syn_ack_packet = create_packet(local_addr.port, remote_addr.port, seq, ack,
                                DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, flags, 1, 0, NULL, 0);
        char *msg = packet_to_buf(syn_ack_packet);
        sendToLayer3(msg, DEFAULT_HEADER_LEN);
        printf("重发syn_ack报文\n");
        start_srv_conn_timer(&sock);
        return;
    }

    return;
}

void start_clt_conn_timer(tju_tcp_t *sock)  //客户端三次握手计时器，每次超时都重传SYN
{
    //printf("start_clt_conn_timer: socket状态为%d\n", sock->state);
    struct itimerval tick;
    RETRANS = 0;
    signal(SIGALRM, clt_conn_timeout_handler);

    memset(&tick, 0, sizeof(tick));

    tick.it_value.tv_sec = 0;  //it_value才是计时器的倒计时间
    tick.it_value.tv_usec = 100000;
    tick.it_interval.tv_sec = 0;//2 * tick.it_value.tv_sec;  //it_interval是超时后复位的时间
    tick.it_interval.tv_usec = 100000; //2 * tick.it_value.tv_usec;

    if (setitimer(ITIMER_REAL, &tick, NULL) < 0)
        printf("Set client conn timer failed!\n");

    printf("START CLIENT CONN TIMER\n");

    return;
}

void start_srv_conn_timer(tju_tcp_t *sock)  //服务端三次握手计时器，每次超时都重传SYN|ACK
{
    //printf("start_srv_conn_timer: socket状态为%d\n", sock->state);
    struct itimerval tick;
    RETRANS = 0;
    signal(SIGALRM, srv_conn_timeout_handler);

    memset(&tick, 0, sizeof(tick));

    tick.it_value.tv_sec = 0;  //it_value才是计时器的倒计时间
    tick.it_value.tv_usec = 100000;
    tick.it_interval.tv_sec = 0;//2 * tick.it_value.tv_sec;  //it_interval是超时后复位的时间
    tick.it_interval.tv_usec = 100000;//2 * tick.it_value.tv_usec;

    if (setitimer(ITIMER_REAL, &tick, NULL) < 0)
        printf("Set servre conn timer failed!\n");

    printf("START SERVRE CONN TIMER\n");

    return;
}

