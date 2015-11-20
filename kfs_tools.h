#ifndef KFS_TOOLS_H_
#define KFS_TOOLS_H_


#include "kfs.h"
#include "kfs_api.h"

#define KFS_UNIX_SOCKET "/var/run/kfs.socket"

//#define KFS_TOOLS_DEBUG


#define kft_err(format,args...) \
do{ \
    fprintf(stderr, "kft_err[%s:%d]:"format"\n",__FUNCTION__,__LINE__,##args);\
}while(0)


#ifdef KFS_TOOLS_DEBUG
#define kft_info(format,args...) \
do{ \
    fprintf(stderr, "kft_info[%s:%d]:"format"\n",__FUNCTION__,__LINE__,##args);\
}while(0)
#else  
#define kft_info(format,args...)  do{}while(0)
#endif

typedef enum kfs_tools_retcode{
    KFS_TOOLS_SUCCESS=0,
    KFS_TOOLS_FAILED,
    KFS_TOOLS_INVALID,
    KFS_TOOLS_UNKNOWN,
    KFS_TOOLS_MAX,
}kfs_tools_retcode_e;


typedef enum kfs_tools_opc{
    TOOLS_OPC_EXPORT=0,
    TOOLS_OPC_ADMIN,
    TOOLS_OPC_MAX,
}kfs_tools_opc_e;

typedef enum kfs_admin_opc{
    ADMIN_OPC_DEL_INFO=0,
    ADMIN_OPC_TRUNC_INFO,
    ADMIN_OPC_FILE_MAP,
    ADMIN_OPC_FORCE_DEL,
    ADMIN_OPC_DEL_SHARE_TD,
    ADMIN_OPC_MAX,
}kfs_admin_opc_e;

typedef enum kfs_export_opc{
    EXPORT_OPC_CREATE=0,
    EXPORT_OPC_DROP,
    EXPORT_OPC_QUERY,
    EXPORT_OPC_MAX,
}kfs_export_opc_e;


typedef enum _kfs_async_op_type{
    ASYNC_DELETE_FILE,
    ASYNC_TRUNCATE_FILE,
} kfs_async_op_type;


struct admin_params
{
    char addr[256];
    int port;
    uint64_t ino;
    kfs_async_op_type op_type;
    char filename[MAX_NAME_LEN];
    char sharename[MAX_NAME_LEN];
    bool help;
};
struct export_params
{
    uint32_t pathlen; 
    uint8_t  path[MAX_PATH_LEN];
};


/* kfs tools request */
struct kfs_tools_req {
    uint8_t     proto_ver;
	uint8_t		parent_opcode;
    uint8_t     child_opcode;
	uint16_t	flags;
    uint32_t	epoch;
	uint32_t    data_length;   
    union {
        struct admin_params  admin;
        struct export_params export;
        uint32_t  padding[8];
    };
};

/* kfs tools response */
struct kfs_tools_res {
    uint32_t opcode;
    uint32_t retcode;
    uint32_t res_len;
    uint32_t padding[8];
};

struct req_workspace_t {
    uint8_t		parent_opcode;
    uint8_t     child_opcode;
    int (*fn)(struct kfs_tools_req *request, int *res_len, void **res_data);
};



int kfs_tools_snprintf(char **buf, int *total,int *used,int step,const char *format,...);
int kfs_tools_write(int sockfd, void *buf, uint32_t len,bool (*need_retry)(uint32_t epoch), uint32_t epoch,uint32_t max_count);
int kfs_tools_read(int sockfd, void *buf, uint32_t len,bool (*need_retry)(uint32_t epoch),uint32_t epoch, uint32_t max_count);
int kfs_tools_server_start();
int kfs_tools_server_stop();
int kfs_tools_client_rpc(struct kfs_tools_req * requst, void **response);




#endif
