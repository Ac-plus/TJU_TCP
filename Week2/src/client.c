#include "tju_tcp.h"
#include <string.h>


int main(int argc, char **argv) {
    // 开启仿真环境 
    startSimulation();

    tju_tcp_t* my_socket = tju_socket();
    // printf("my_tcp state %d\n", my_socket->state);
    
    tju_sock_addr target_addr;
    target_addr.ip = inet_network("172.17.0.3");
    target_addr.port = 1234;

    tju_connect(my_socket, target_addr);
    printf("my_socket state %d\n", my_socket->state);      

    // uint32_t conn_ip;
    // uint16_t conn_port;

    // conn_ip = my_socket->established_local_addr.ip;
    // conn_port = my_socket->established_local_addr.port;
    // printf("my_socket established_local_addr ip %d port %d\n", conn_ip, conn_port);

    // conn_ip = my_socket->established_remote_addr.ip;
    // conn_port = my_socket->established_remote_addr.port;
    // printf("my_socket established_remote_addr ip %d port %d\n", conn_ip, conn_port);

    sleep(3);

    for(int i=0; i<50000; i++) {
        char buf[18];
        sprintf(buf , "test message%d\n", i);
        tju_send(my_socket, buf, 18);  //把my_socket传下来的数据放进发送缓冲区
        printf("[RDT TEST] client send %s", buf);
        //sleep(1);  //test_line:更好的观察输出信息的顺序
    }

    tju_send(my_socket, "hello world", 12);
    tju_send(my_socket, "hello tju", 10);

    char buf[2021];
    tju_recv(my_socket, (void*)buf, 12);  //把接收缓冲区里的数据向上传给my_socket
    printf("client recv %s\n", buf);

    tju_recv(my_socket, (void*)buf, 10);
    printf("client recv %s\n", buf);

    sleep(100);

    return EXIT_SUCCESS;
}
