#include "worker.h"
#include "protocol.h"
#include "ppm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* helper function to read n bytes */
static int read_all(int fd, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int r = read(fd, (char *)buf + total, n - total);
        if (r <= 0) return -1;
        total += r;
    }
    return 0;
}

/* helper function to write n bytes */
static int write_all(int fd, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int w = write(fd, (char *)buf + total, n - total);
        if (w <= 0) return -1;
        total += w;
    }
    return 0;
}

/*
 * Reads a job_msg_t + pixel strip from job_fd,
 * applies grayscale, and writes a result_msg_t and processed strip to result_fd.
 * Loops until it receives JOB_SHUTDOWN.
 */
void worker_run(int worker_id, int job_fd, int result_fd) {  
    job_msg_t job;
    result_msg_t result;
    int status;
    unsigned char *pixels;

    while (1) {
        // read a job_msg_t and pixel strip from job_fd
        if (read_all(job_fd, &job, sizeof(job_msg_t)) != 0)
            break;

        if (job.job_id == JOB_SHUTDOWN) // break the loop and exit
            break;

        printf("[worker %d] got strip %d (%d bytes)\n", worker_id, job.job_id, job.strip_bytes);

        // allocate memory for the pixel strip and read it from job_fd
        pixels = malloc(job.strip_bytes);
        status = 0;

        // check for malloc failure
        if (!pixels) {
            status = errno;
        // check for read failure
        } else if (read_all(job_fd, pixels, job.strip_bytes) != 0) {
            status = errno;
            free(pixels);
            pixels = NULL;
        }
        // no error, apply grayscale
        if (status == 0) { 
            ppm_grayscale(pixels, job.width * job.height);
        }
        // write a result_msg_t
        result.job_id      = job.job_id;
        result.worker_id   = worker_id;
        result.status      = status;
        if (status == 0) {
            result.strip_bytes = job.strip_bytes;
        } else { // error, set strip_bytes to 0
            result.strip_bytes = 0;
        }
        write_all(result_fd, &result, sizeof(result_msg_t));
        // no error, write the processed strip to result_fd
        if (status == 0 && pixels) { 
            write_all(result_fd, pixels, job.strip_bytes);
        }

        free(pixels);
    }

    close(job_fd);
    close(result_fd);
}
