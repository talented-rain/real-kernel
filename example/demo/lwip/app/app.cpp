/*
 * User Thread Instance (lwip task) Interface
 *
 * File Name:   app.c
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2024.11.21
 *
 * Copyright (c) 2024   Yang Yujun <yujiantianhu@163.com>
 *
 */

/*!< The globals */
#include <platform/fwk_fcntl.h>
#include <platform/net/fwk_ether.h>
#include <platform/net/fwk_if.h>

#include "app.h"

using namespace stream;

/*!< The defines */
#define DEFAULT_IP_ADDRESS          "192.168.253.206"
#define DEFAULT_IP_MASK             "255.255.255.0"
#define DEFAULT_GW_ADDRESS          "192.168.253.1"

/*!< The globals */

/*!< API functions */
/*!
 * @brief  start up
 * @param  sprt_dctrl
 * @retval none
 * @note   none
 */
void lwip_task_startup(crt_lwip_data_t &sgrt_data)
{
    struct fwk_sockaddr_in sgrt_local;
    struct fwk_sockaddr_in sgrt_ip, sgrt_gw, sgrt_netmask;
    kint32_t sockfd;
    kint32_t retval;

    sgrt_ip.sin_addr.s_addr = fwk_inet_addr(DEFAULT_IP_ADDRESS);
    sgrt_gw.sin_addr.s_addr = fwk_inet_addr(DEFAULT_GW_ADDRESS);
    sgrt_netmask.sin_addr.s_addr = fwk_inet_addr(DEFAULT_IP_MASK);

    retval = net_link_up("lo", &sgrt_ip, &sgrt_gw, &sgrt_netmask);
    if (retval)
        return;

    sockfd = net_socket(NET_AF_INET, NR_SOCK_DGRAM, 0);
    if (sockfd < 0)
        goto fail1;

    sgrt_local.sin_port = mrt_htons(7);
    sgrt_local.sin_family = NET_AF_INET;
    sgrt_local.sin_addr.s_addr = fwk_inet_addr(DEFAULT_IP_ADDRESS);
    memset(sgrt_local.zero, 0, sizeof(sgrt_local.zero));

    retval = socket_bind(sockfd, (struct fwk_sockaddr *)&sgrt_local, sizeof(struct fwk_sockaddr));
    if (retval)
        goto fail2;

    sgrt_data.fd = sockfd;
    return;

fail2:
    virt_close(sockfd);
fail1:
    net_link_down("lo");
}

/*!
 * @brief  main
 * @param  args
 * @retval none
 * @note   none
 */
void lwip_task(crt_lwip_data_t &sgrt_data)
{
    struct fwk_sockaddr_in sgrt_remote;
    const kchar_t *msg = "HeavenFox OS will be all the best!";
    fwk_socklen_t addrlen;
    kssize_t len;

    if (sgrt_data.fd < 0)
        return;

    sgrt_remote.sin_port = mrt_htons(7);
    sgrt_remote.sin_family = NET_AF_INET;
    sgrt_remote.sin_addr.s_addr = fwk_inet_addr(DEFAULT_IP_ADDRESS);
    memset(sgrt_remote.zero, 0, sizeof(sgrt_remote.zero));

    len = socket_sendto(sgrt_data.fd, msg, strlen(msg) + 1, 0, 
                    (struct fwk_sockaddr *)&sgrt_remote, sizeof(struct fwk_sockaddr));
    if (len <= 0)
    {
        cout << "send msg failed!" << endl;
        return;
    }

    /*!< blocking */
    len = socket_recvfrom(sgrt_data.fd, sgrt_data.rx_buffer, 128, 0, 
                    (struct fwk_sockaddr *)&sgrt_remote, &addrlen);
    if (len <= 0)
    {
        cout << "recv msg failed!" << endl;
        return;
    }

    sgrt_data.rx_buffer[len] = '\0';
//  cout << "recv data is: " << sgrt_data.rx_buffer << endl;
}

/*!< end of file */
