#ifndef KFS_TOOLS_SERVER_H_
#define KFS_TOOLS_SERVER_H_

int kfs_tools_deleted_file_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_truncate_file_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_filemap_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_force_delete_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_delete_share_td_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_export_create_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_export_drop_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_export_create_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tools_export_drop_fn(struct kfs_tools_req *request, int *res_len, void **res_data);
int kfs_tooks_export_query_fn(struct kfs_tools_req *request, int *res_len, void **res_data);

#endif
