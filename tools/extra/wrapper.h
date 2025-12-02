#ifndef __IOCTL_WRAPPER_H__
#define __IOCTL_WRAPPER_H__

#include <linux/types.h>

//struct hfi_msg_session_property_info_pkt {
//        struct hfi_session_hdr_pkt {
//                struct hfi_pkt_hdr {
//                        uint32_t size;
//                        uint32_t pkt_type;
//                } hdr;
//                uint32_t session_id;
//        } shdr;
//        uint32_t num_properties;
//        uint32_t property;
//        uint8_t* data;
//};
//
//struct hfi_buffer_requirements {
//        uint32_t type;
//        uint32_t size;
//        uint32_t region_size;
//        uint32_t hold_count;
//        uint32_t count_min;
//        uint32_t count_actual;
//        uint32_t contiguous;
//        uint32_t alignment;
//};

struct args_data {
        struct hfi_buffer_requirements reqs;
        struct hfi_msg_session_property_info_pkt info;
};

//struct hfi_frame_data {
//        uint32_t buffer_type;
//        uint32_t device_addr;
//        uint32_t extradata_addr;
//        uint64_t timestamp;
//        uint32_t flags;
//        uint32_t offset;
//        uint32_t alloc_len;
//        uint32_t filled_len;
//        uint32_t mark_target;
//        uint32_t mark_data;
//        uint32_t clnt_data;
//        uint32_t extradata_size;
//};


#define DEVICE_NAME "ioctl_wrapper"
#define CLASS_NAME "wrapper_class"
#define IOCTL_CMD_WRAPPER _IOW(0, 0, struct args_data*)
#define POOL_SIZE 10
#define BUFFER_SIZE sizeof(struct args_data) + sizeof(struct hfi_buffer_requirements)
#define HFI_BUG 0x1014

#endif

