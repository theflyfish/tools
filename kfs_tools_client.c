
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include "internal_proto.h" 
#include "util.h"
#include "work.h"
#include "net.h"
#include "kfs.h"
#include "kfs_api.h"
#include "kfs_tools.h"


int kfs_tools_client_rpc(struct  kfs_tools_req *requst, void **resquest)
{
    int connect_fd;  
    int ret;
    char *res_data = NULL;
    struct kfs_tools_res response;
    struct sockaddr_un srv_addr;
    
    /* creat unix socket */
    connect_fd=socket(PF_UNIX,SOCK_STREAM,0);  
    if(connect_fd<0)  
    {  
        perror("cannot create communication socket");  
        return -1;  
    }     
    srv_addr.sun_family=AF_UNIX;  
    strcpy(srv_addr.sun_path,KFS_UNIX_SOCKET);  
    /* connect server */
    //ret = connect(connect_fd,(struct sockaddr *)&srv_addr,sizeof(srv_addr));  
    ret = connect(connect_fd,&srv_addr,sizeof(srv_addr));  
    if(ret==-1)  
    {  
        perror("cannot connect to the server");  
        close(connect_fd);  
        return -1;  
    }  
    /* send cmd to server */

    ret = kfs_tools_write(connect_fd,requst,sizeof(struct kfs_tools_req),NULL,0,UINT32_MAX);
    if (ret) {
        kft_err("client write error.");
        close(connect_fd);  
        return -1;
    }
    
    /* read result from  server */
    //ret = read(connect_fd,&response,sizeof(kfs_tools_res));
    ret = kfs_tools_read(connect_fd,&response, sizeof(struct kfs_tools_res) , NULL, 0, UINT32_MAX);
    if (ret) {
        kft_err("client read response error");
        close(connect_fd);
        return -1;
    }

    kft_info("result:%d res_len:%d", response.retcode, response.res_len);
    if (response.res_len) {
        res_data = xzalloc(response.res_len);
        if (!res_data) {
            kft_err("get response data fail, error:%s", strerror(errno));
            goto out;
        }

        ret = kfs_tools_read(connect_fd, res_data, response.res_len, NULL, 0, UINT32_MAX);
        if (ret) {
            free(res_data);
            kft_err("read response data fail, error:%s", strerror(errno));
            goto out;
        }

        *resquest = res_data; 
    } else {
        *resquest = NULL;
    }

out:
    /* release connect */
    close(connect_fd);
    return response.retcode;
}
