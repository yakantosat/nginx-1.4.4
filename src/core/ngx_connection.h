
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;

struct ngx_listening_s {
    ngx_socket_t        fd;  // socket套接字句柄

    struct sockaddr    *sockaddr;  // 监听sockaddr地址
    socklen_t           socklen;    /* sockaddr长度 */
    size_t              addr_text_max_len; // 存储IP的字符串addr_text的最大长度，指定了addr_text所分配的内存大小
    ngx_str_t           addr_text;  // 以字符串形式存储IP地址

    int                 type;  // 套接字类型。例如，当type为SOCK_STREAM时，表示TCP

    /* TCP实现监听时的backlog队列，它表示允许正在通过三次握手建立TCP连接但还没有任何进程开始处理的连接最大个数 */
    int                 backlog;
    int                 rcvbuf; // 内核中对于这个套接字的接收缓冲区大小
    int                 sndbuf; // 内核中对于这个套接字的发送缓冲区大小
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;
    int                 keepintvl;
    int                 keepcnt;
#endif

    /* handler of accepted connection */
    ngx_connection_handler_pt   handler; // 当新的TCP连接成功建立后的处理方法

    /* 实际上框架并不使用servers指针，它更多是作为一个保留指针，目前主要用于HTTP或mail等模块，用于保存当前监听端口
    对应着的所有主机名 */
    void               *servers;  /* array of ngx_http_in_addr_t, for example */

    // log和logp都是可用的日志对象指针
    ngx_log_t           log;
    ngx_log_t          *logp;

    // 若为新的TCP连接创建内存池，则内存池的初始大小应该是pool_size
    size_t              pool_size;
    /* should be here because of the AcceptEx() preread */
    size_t              post_accept_buffer_size;
    /* should be here because of the deferred accept */
    /* TCP_DEFER_ACCEPT选项将在建立TCP连接成功且接收到用户的请求数据后，才向对监听套接字感兴趣的进程发送事件通知，
    而连接建立成功后，若post_accept_timeout秒后仍然没有收到用户数据，则内核直接丢弃连接 */
    ngx_msec_t          post_accept_timeout;

    ngx_listening_t    *previous; // 前一个ngx_listening_t结构，多个结构体之间由previous指针组成单链表
    ngx_connection_t   *connection; // 当前监听句柄对应着的ngx_connection_t结构体

    /* 标志位，为1表示在当前监听句柄有效，且执行ngx_init_cycle时不关闭监听端口，为0则正常关闭。由框架自动设置 */
    unsigned            open:1;
    
    /* 标志位，为1表示使用已有的ngx_cycle_t来初始化新的ngx_cycle_t结构体时，不关闭原先打开的监听端口，这对运行中
    升级程序很有用，remain为0时，表示正常关闭曾经打开的监听端口。由框架自动设置 */
    unsigned            remain:1;
    
    /* 标志位，为1时表示跳过设置当前ngx_listening_t中得套接字，为0则正常初始化套接字。框架自动设置 */
    unsigned            ignore:1;

    /* 表示是否已经绑定。实际上没有使用 */
    unsigned            bound:1;       /* already bound */
    
    /* 表示当前监听句柄是否来自前一个进程（如升级Nginx程序）,若为1，表示来自前一个进程。一般会保留之前已经
    设置好的套接字，不做改变 */
    unsigned            inherited:1;   /* inherited from previous process */
    unsigned            nonblocking_accept:1; // 未使用
    unsigned            listen:1; // 为1时，表示当前结构体对应的套接字已经监听
    unsigned            nonblocking:1; // 表示套接字是否阻塞，目前没有意义
    unsigned            shared:1;    /* shared between threads or processes */
    unsigned            addr_ntop:1; // 为1时，Nginx会讲网络地址转变为字符串形式的地址

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
    unsigned            keepalive:2;

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
#ifdef SO_ACCEPTFILTER
    char               *accept_filter;
#endif
#endif
#if (NGX_HAVE_SETFIB)
    int                 setfib;
#endif

};


typedef enum {
     NGX_ERROR_ALERT = 0,
     NGX_ERROR_ERR,
     NGX_ERROR_INFO,
     NGX_ERROR_IGNORE_ECONNRESET,
     NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;


typedef enum {
     NGX_TCP_NODELAY_UNSET = 0,
     NGX_TCP_NODELAY_SET,
     NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;


typedef enum {
     NGX_TCP_NOPUSH_UNSET = 0,
     NGX_TCP_NOPUSH_SET,
     NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;


#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01


struct ngx_connection_s {
    void               *data;
    ngx_event_t        *read;
    ngx_event_t        *write;

    ngx_socket_t        fd;

    ngx_recv_pt         recv;
    ngx_send_pt         send;
    ngx_recv_chain_pt   recv_chain;
    ngx_send_chain_pt   send_chain;

    ngx_listening_t    *listening;

    off_t               sent;

    ngx_log_t          *log;

    ngx_pool_t         *pool;

    struct sockaddr    *sockaddr;
    socklen_t           socklen;
    ngx_str_t           addr_text;

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

    struct sockaddr    *local_sockaddr;

    ngx_buf_t          *buffer;

    ngx_queue_t         queue;

    ngx_atomic_uint_t   number;

    ngx_uint_t          requests;

    unsigned            buffered:8;

    unsigned            log_error:3;     /* ngx_connection_log_error_e */

    unsigned            unexpected_eof:1;
    unsigned            timedout:1;
    unsigned            error:1;
    unsigned            destroyed:1;

    unsigned            idle:1;
    unsigned            reusable:1;
    unsigned            close:1;

    unsigned            sendfile:1;
    unsigned            sndlowat:1;
    unsigned            tcp_nodelay:2;   /* ngx_connection_tcp_nodelay_e */
    unsigned            tcp_nopush:2;    /* ngx_connection_tcp_nopush_e */

#if (NGX_HAVE_IOCP)
    unsigned            accept_context_updated:1;
#endif

#if (NGX_HAVE_AIO_SENDFILE)
    unsigned            aio_sendfile:1;
    ngx_buf_t          *busy_sendfile;
#endif

#if (NGX_THREADS)
    ngx_atomic_t        lock;
#endif
};


ngx_listening_t *ngx_create_listening(ngx_conf_t *cf, void *sockaddr,
    socklen_t socklen);
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t *cycle);
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_connection(ngx_connection_t *c);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t *c, ngx_str_t *s,
    ngx_uint_t port);
ngx_int_t ngx_connection_error(ngx_connection_t *c, ngx_err_t err, char *text);

ngx_connection_t *ngx_get_connection(ngx_socket_t s, ngx_log_t *log);
void ngx_free_connection(ngx_connection_t *c);

void ngx_reusable_connection(ngx_connection_t *c, ngx_uint_t reusable);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
