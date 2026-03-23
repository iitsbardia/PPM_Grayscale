# Parallel Grayscale

Converts a PPM image to grayscale using a pool of 4 worker processes that run concurrently. The parent splits the image into horizontal strips, sends each strip to a worker over a pipe, and reassembles the results into the output file.

## How it works

The parent process forks 4 workers at startup — all before any work is dispatched, so they are all alive and waiting at the same time. The image is divided into 4 equal horizontal strips (the last strip absorbs any leftover rows). Each worker receives its strip as raw pixel bytes over a pipe, applies grayscale using the standard luminosity formula, and writes the processed pixels back to the parent over a separate result pipe. The parent reassembles the strips in order and writes the final PPM file.

All data transfer between parent and workers happens over pipes. No shared memory, signals, or temporary files are used.

## Files

```
main.c      — parent process: spawns workers, splits image, collects results
worker.c    — worker process: reads strip, applies grayscale, writes result
ppm.c       — PPM read/write and grayscale implementation
ppm.h       — ppm_image_t struct and function declarations
worker.h    — worker_run() declaration
protocol.h  — job_msg_t and result_msg_t pipe message structs
```

## Build

```bash
make
```


## Usage

```bash
./grayscale <input_file.ppm>
```

The input must be a binary PPM file (P6 format). The output is written as a grayscale P6 PPM where each pixel's RGB channels are set to the same luminance value.

## Example output

```
Loaded 'photo.ppm': 1920 x 1080
Sent strip 0 (rows 0-269) to worker 0
Sent strip 1 (rows 270-539) to worker 1
Sent strip 2 (rows 540-809) to worker 2
Sent strip 3 (rows 810-1079) to worker 3
[OK]   strip 0 returned from worker 0
[OK]   strip 1 returned from worker 1
[OK]   strip 2 returned from worker 2
[OK]   strip 3 returned from worker 3
Grayscale image written to 'output.ppm'
```

## Notes

- Number of workers is defined as `NUM_WORKERS 4` in `protocol.h` and can be changed there
- Images with fewer than 4 rows are not supported
- Tested on macOS and Linux
