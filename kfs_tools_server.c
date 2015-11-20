#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdarg.h>

#include "internal_proto.h" 
#include "util.h"
#include "work.h"
#include "net.h"
#include "event.h"
#include "kfs.h"
#include "kfs_api.h"

#include "kfs_admin.h"
#include "kfs_export.h"
#include "kfs_tools.h"
#include "kfs_tools_server.h"

struct req_workspace_t  g_req_workspace[] = {
    {
        TOOLS_OPC_ADMIN,
        ADMIN_OPC_DEL_INFO,    
        kfs_tools_deleted_file_fn,
    },
    {
        TOOLS_OPC_ADMIN,
        ADMIN_OPC_TRUNC_INFO,    
        kfs_tools_truncate_file_fn,
    },    
    {
        TOOLS_OPC_ADMIN,
        ADMIN_OPC_FILE_MAP,    
        kfs_tools_filemap_fn,
    },
    {
        TOOLS_OPC_ADMIN,
        ADMIN_OPC_FORCE_DEL,    
        kfs_tools_force_delete_fn,
    },  
    {
        TOOLS_OPC_ADMIN,
        ADMIN_OPC_DEL_SHARE_TD,    
        kfs_tools_delete_share_td_fn,
    },   
    {
        TOOLS_OPC_EXPORT,
        EXPORT_OPC_CREATE,    
        kfs_tools_export_create_fn,
    },  
    {
        TOOLS_OPC_EXPORT,
        EXPORT_OPC_DROP,    
        kfs_tools_export_drop_fn,
    },
    {
        TOOLS_OPC_EXPORT,
        EXPORT_OPC_QUERY,
        kfs_tooks_export_query_fn,
    },
};

int kfs_tools_snprintf(char **buf, int *total,int *used,int step,const char *format,...){
    char *rebuf = NULL;
    va_list args;
    int nSize = 0;
    if(*buf == NULL || buf == NULL || total == NULL || used == NULL || step < 0) {
        sd_err("param err.\n");
        return -1;
    }
    if(*total - *used < step) {
        sd_warn("buffer need remalloc.\n");
        *total += step;
        rebuf= realloc(*buf, *total);
        if (!rebuf) {
            sd_err("malloc fail, error:%s", strerror(errno));
            free(*buf);
            *buf = NULL;
            *total = 0;
            *used  = 0;            
            return -1;
        }
        *buf =rebuf;
        memset((*buf)+(*used),0,(*total-*used));
    }

    va_start(args,format);
    nSize = vsnprintf((*buf)+(*used),(*total-*used),format,args);
    va_end(args);
    *used += nSize;
    sd_info("total %d,used %d,nsize %d.\n",*total,*used,nSize);
    return nSize;
}



int kfs_tools_write(int sockfd, void *buf, uint32_t len,
		    bool (*need_retry)(uint32_t), uint32_t epoch,
		    uint32_t max_count)
{
	int ret, repeat = max_count;
rewrite:
	ret = write(sockfd, buf, len);
	if (ret < 0) {
		if (errno == EINTR) {
            sd_err("try again");
			goto rewrite;
        }
		/*
		 * Since we set timeout for write, we'll get EAGAIN even for
		 * blocking sockfd.
		 */
		if (errno == EAGAIN && repeat &&
		    (need_retry == NULL || need_retry(epoch))) {
			repeat--;
            sd_err("try again");
			goto rewrite;
		}

		sd_err("failed to write to socket: %m");
		return 1;
	}

	len -= ret;
	buf = (char *)buf + ret;
	if (len) {
        sd_err("only send part data");
		goto rewrite;
	}
	return 0;
}


int  kfs_tools_read(int sockfd, void *buf, uint32_t len,
	    bool (*need_retry)(uint32_t epoch),
	    uint32_t epoch, uint32_t max_count)
{
	int ret, repeat = max_count;
reread:
	ret = read(sockfd, buf, len);
	if (ret == 0) {
		sd_debug("connection is closed (%d bytes left)", len);
		return 1;
	}
	if (ret < 0) {
        perror("now get some problem");
		if (errno == EINTR)
			goto reread;
		/*
		 * Since we set timeout for read, we'll get EAGAIN even for
		 * blocking sockfd.
		 */
		if (errno == EAGAIN && repeat &&
		    (need_retry == NULL || need_retry(epoch))) {
			repeat--;
			goto reread;
		}

		sd_err("failed to read from socket: %d, %m", ret);
		return 1;
	}

	len -= ret;
	buf = (char *)buf + ret;
	if (len) {
		goto reread;
    }
	return 0;
}

int kfs_tools_deleted_file_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    return kfs_deleted_file_fn(&(request->admin),res_len,res_data);
}
int kfs_tools_truncate_file_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    return kfs_truncate_file_fn(&(request->admin),res_len,res_data);
}
int kfs_tools_filemap_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    return kfs_filemap_fn(&(request->admin),res_len,res_data);
}
int kfs_tools_force_delete_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    return kfs_force_delete_fn(&(request->admin),res_len,res_data);
}
int kfs_tools_delete_share_td_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    return kfs_delete_share_td_fn(&(request->admin),res_len,res_data);
}

int kfs_tools_export_create_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    int ret = 0;
    char * create_path = (char *)request->export.path;

    ret = kfs_create_export_path(create_path);
    if (ret != 0) {
        sd_err("kfs create export(%s) failed, ret(%d).", create_path, ret);
    }
    return 0;
}

int kfs_tools_export_drop_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    char * drop_path = (char *)request->export.path;
    int ret = 0;
    sd_info("start drop export, path(%s)", drop_path);
    
    /* delete the td zone of the share */
    /* ignore the character '/' */
    ret = kfs_api_delete_share_td(drop_path+1, true);
    if (ret != 0 && ret != SD_RES_NO_OBJ) {
        sd_err("kfs drop the td zone of export(%s) failed.", drop_path);
        return 1;
    }
    
    ret = kfs_drop_export_path(drop_path);
    if (ret != 0) {
        sd_err("kfs drop export(%s) failed, ret(%d).", drop_path, ret);
        return 1;
    }
    return 0;    
}

int kfs_tooks_export_query_fn(struct kfs_tools_req *request, int *res_len, void **res_data)
{
    int ret;
    struct export_share_info share_info;
    
    ret = kfs_api_query_share(&share_info);
    if (share_info.data_len) {
        *res_len = share_info.data_len;
        *res_data = share_info.data;
    } else {
        *res_len = 0;
    }

    return ret;
}

static int  kfs_tools_requst_work(int fd)
{
    int ret = 0,found = 0;
    int cnt = 0,i = 0;
    int res_len = 0;
    void *res_data=NULL;
    struct kfs_tools_req request;
    struct kfs_tools_res response;
    /* receive request  from client   */
    ret = kfs_tools_read(fd, &request, sizeof(request), NULL, 0, UINT32_MAX);
	if (ret) {
		sd_err("failed to read a request");
		return -1;
	}
    sd_info("parent_opcode:%d,child_opcode:%d",
        request.parent_opcode,
        request.child_opcode);
    
    /* process the request  */
    
    cnt = sizeof(g_req_workspace) / sizeof(struct req_workspace_t);
    for (i = 0; i < cnt; ++i) {
        if ((request.parent_opcode == g_req_workspace[i].parent_opcode) && 
            (request.child_opcode == g_req_workspace[i].child_opcode)) {
            found = 1;
            sd_info("find the command i=%d,cnt=%d pop_code:%d cop_code:%d",
                i, cnt, g_req_workspace[i].parent_opcode, g_req_workspace[i].child_opcode);
            ret = g_req_workspace[i].fn(&request, &res_len, &res_data);
            break;
        }
    }
    
    ret =(0 == found )?-2:ret;
    sd_info("now response to client");
    /* response to client */
    
    response.retcode = ret;
    response.res_len = res_len;
    ret = kfs_tools_write(fd,&response,sizeof(response),NULL,0,UINT32_MAX);
    sd_info("kfs_tools_write:ret:%d",ret);

    if (res_len) {
        ret = kfs_tools_write(fd, res_data, res_len,NULL,0,UINT32_MAX);
        sd_info("write return:%d, res_len:%d", ret, res_len);

        free(res_data);
    }
    return ret;

}

static void  kfs_tools_client_handler(int fd, int events, void *data)
{
    int ret = 0;
    sd_info("get something for client");
	if (events & (EPOLLERR | EPOLLHUP)) {
        sd_err("the connection seems to be dead");
        unregister_event(fd);
        close(fd);
        fd=0;
        return;
    }

	if (events & EPOLLIN) {
        sd_info("receiving data");
        ret = kfs_tools_requst_work(fd);
	}

	if (events & EPOLLOUT) {
	    sd_err("sending data");
	}
    
    sd_info("the connection now finish:%d",ret);
    // unregister_event(fd);
    // close(fd);
    // fd=0;
    return ;    
}

static void kfs_tools_listen_handler(int listen_fd, int events, void *data)
{
	struct sockaddr_storage from;
	socklen_t namesize;
	int fd = 0, ret = 0;

	namesize = sizeof(from);
	fd = accept(listen_fd, (struct sockaddr *)&from, &namesize);
	if (fd < 0) {
		sd_err("failed to accept a new connection: %d",fd);
		return;
	}
	ret = register_event(fd, kfs_tools_client_handler, NULL);
	if (ret) {
		sd_err("failed to accept a new connection(%d)",ret);
		return;
	}
	sd_info("accepted a new connection: %d", fd);
    return;
}

static int kfs_tools_listen_port_create(int fd, void *data)
{
	return register_event(fd, kfs_tools_listen_handler, data);
}

int kfs_tools_server_start()
{
    int ret = 0;
    unlink(KFS_UNIX_SOCKET);
    sd_info("kfs tools server start");
    ret = create_unix_domain_socket(KFS_UNIX_SOCKET, kfs_tools_listen_port_create, NULL);
    sd_info("kfs tools server start  finish");
    return ret;
}

int kfs_tools_server_stop()
{
    return SD_RES_SUCCESS;
}
