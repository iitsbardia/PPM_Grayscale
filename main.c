#include "protocol.h"
#include "worker.h"
#include "ppm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

// global arrays to keep track of worker communication channels and PIDs
static int job_write[NUM_WORKERS];
static int result_read[NUM_WORKERS];
static int worker_pids[NUM_WORKERS];

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

// spawns a worker process and sets up communication channels
static void spawn_worker(int id) {
    int job_pipe[2], res_pipe[2];
    int i;
    pipe(job_pipe);
    pipe(res_pipe);

    int pid = fork();
    if (pid == 0) { // child process: worker
        // close unused pipe ends
        close(job_pipe[1]);
        close(res_pipe[0]);
        for (i = 0; i < id; i++) {
            close(job_write[i]);
            close(result_read[i]);
        }
        // run the worker loop
        worker_run(id, job_pipe[0], res_pipe[1]);
        exit(0);
    }
    // parent process: close unused pipe ends
    close(job_pipe[0]);
    close(res_pipe[1]);
    // save communication channels and worker PID
    job_write[id]   = job_pipe[1];
    result_read[id] = res_pipe[0];
    worker_pids[id] = pid;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.ppm\n", argv[0]);
        return 1;
    }

    ppm_image_t img;
    if (ppm_read(argv[1], &img) != 0) {
        fprintf(stderr, "Error: could not read '%s'\n", argv[1]);
        return 1;
    }

    printf("Successfully read '%s'\n", argv[1]);

    // spawn worker processes
    for (int i = 0; i < NUM_WORKERS; i++){
        spawn_worker(i);
    }
    // divide the image into strips and send them to the workers
    int rows_per_strip = img.height / NUM_WORKERS;

    for (int i = 0; i < NUM_WORKERS; i++) {
        int row_start  = i * rows_per_strip;
        int row_end;
        if (i == NUM_WORKERS - 1) {
            // last worker gets everything up to the bottom of the image
            row_end = img.height;
        } else {
            // every other worker gets exactly rows_per_strip rows
            row_end = row_start + rows_per_strip;
        }
        int strip_h    = row_end - row_start;
        int strip_bytes = img.width * strip_h * 3;

        // create a job_msg_t for the strip and write it + the pixel data to the worker
        job_msg_t job;
        job.job_id      = i;
        job.width       = img.width;
        job.height      = strip_h;
        job.strip_bytes = strip_bytes;

        write_all(job_write[i], &job, sizeof(job_msg_t));
        write_all(job_write[i], img.pixels + row_start * img.width * 3, strip_bytes);
    }

    // read processed strips from workers and write them to the output image
    unsigned char *out_pixels = malloc(img.width * img.height * 3);
    if (!out_pixels) {
        fprintf(stderr, "Error: out of memory\n");
        return 1;
    }

    for (int collected = 0; collected < NUM_WORKERS; collected++) {
        result_msg_t res;
        // read a result_msg_t from the worker
        if (read_all(result_read[collected], &res, sizeof(result_msg_t)) != 0) {
            fprintf(stderr, "Error reading result from worker %d\n", collected);
            continue; // skip to the next worker
        }
        // check for worker error
        if (res.status != 0) {
            fprintf(stderr, "[FAIL] worker %d reported error %d\n", res.worker_id, res.status);
            continue; // skip to the next worker
        }

        // calculate the output image offset
        int row_start = res.job_id * rows_per_strip;
        unsigned char *dst = out_pixels + row_start * img.width * 3;
        // read the processed strip from the worker
        if (read_all(result_read[collected], dst, res.strip_bytes) != 0) {
            fprintf(stderr, "Error reading pixel data from worker %d\n", collected);
            continue; // skip to the next worker
        }

        // Successfull
        printf("[OK] strip %d back from worker %d\n", res.job_id, res.worker_id);
    }
    // send shutdown message to workers
    job_msg_t stop;
    memset(&stop, 0, sizeof(stop));
    stop.job_id = JOB_SHUTDOWN;
    for (int i = 0; i < NUM_WORKERS; i++)
        write_all(job_write[i], &stop, sizeof(stop));
    // wait for all workers to exit
    for (int i = 0; i < NUM_WORKERS; i++)
        waitpid(worker_pids[i], NULL, 0);

    // create the output image struct
    ppm_image_t out_img;
    out_img.width   = img.width;
    out_img.height  = img.height;
    out_img.max_val = img.max_val;
    out_img.pixels  = out_pixels;
    // write the output image to a file
    if (ppm_write("out.ppm", &out_img) != 0) {
        fprintf(stderr, "Error: could not write output\n");
        ppm_free(&img);
        free(out_pixels);
        return 1;
    }
    // Successfull
    printf("Done! Grayscale image written to out.ppm\n");
    ppm_free(&img);
    free(out_pixels);
    return 0;
}
