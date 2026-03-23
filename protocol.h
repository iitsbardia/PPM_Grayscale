#ifndef PROTOCOL_H
#define PROTOCOL_H

#define JOB_SHUTDOWN -1
#define NUM_WORKERS 4
#define MAX_PATH_LEN 512

/* JOB MESSAGE: Parent -> Worker */
typedef struct {
    int job_id;
    int width;
    int height;
    int strip_bytes;
} job_msg_t;

/* RESULT MESSAGE: Worker -> Parent */
typedef struct {
    int job_id;
    int worker_id;
    int status;
    int strip_bytes;
} result_msg_t;

#endif
