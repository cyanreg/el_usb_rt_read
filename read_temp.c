/*
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

int quit = 0;

static void sighandler(int unused)
{
    quit = 1;
}

static unsigned int pack(uint8_t a, uint8_t b)
{
    unsigned int packed;
    packed = a;
    packed <<= 8;
    packed |= b;
    return packed;
}

int main(int argc, char **argv)
{
    int i, res, desc_size;
    int had_temp = 0, had_humidity = 0;
    unsigned int val;
    double temp;
    double humidity;
    uint8_t buf[2];
    char name[256];
    struct hidraw_devinfo info = {0};
    struct hidraw_report_descriptor rpt_desc = {0};

    char *device = NULL;
    char *log = NULL;
    FILE *log_f;

    if (argv[1])
        device = argv[1];
    if (argv[2])
        log = argv[2];

    if (!device || !log) {
        fprintf(stderr, "Usage: ./read_temp <hidraw file> <log file>\n");
        return 1;
    }

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Couldn't open device %s! Make sure to run as a superuser!\n", device);
        return 1;
    }

    if (log) {
        log_f = fopen(log, "w+");
        if (!log_f) {
            fprintf(stderr, "Unable to open log file %s!\n", log);
            close(fd);
            return 1;
        }
    }

    res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
    printf("(%i) Report Descriptor Size: %d\n", res, desc_size);

    rpt_desc.size = desc_size;
    res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
    printf("(%i) Report Descriptor: ", res);
    for (i = 0; i < rpt_desc.size/4; i++)
        printf("%hhx ", rpt_desc.value[i]);
    printf("...\n");

    res = ioctl(fd, HIDIOCGRAWNAME(256), name);
    printf("(%i) Name: %s\n", res, name);

    res = ioctl(fd, HIDIOCGRAWPHYS(256), name);
    printf("(%i) Raw Phys: %s\n", res, name);

    /* Get Raw Info */
    res = ioctl(fd, HIDIOCGRAWINFO, &info);
    printf("(%i) Raw Info: vendor=0x%04hx product=0x%04hx\n",
           res, info.vendor, info.product);

    signal(SIGINT, sighandler);

    while (!quit) {
        res = read(fd, buf, 3);
        if (res == 3) {
            if (buf[0] != 0x3)
                goto error;
            val = pack(buf[2], buf[1]);
            temp = -200 + val*0.1f;
            had_temp = 1;
        }
        if (res == 2) {
            if (buf[0] != 0x2)
                goto error;
            humidity = 0.5f*buf[1];
            had_humidity = 1;
        }
        if (had_temp && had_humidity) {
            printf("Temperature = %0.3f°C Humidity = %.1f\%\n", temp, humidity);
            if (log)
                fprintf(log_f, "%f %f\n", temp, humidity);
            had_temp = had_humidity = 0;
        }
    }

    close(fd);
    if (log)
        fclose(log_f);

    printf("Exiting\n");

    return 0;

error:
    fprintf(stderr, "Garbage read\n");
    close(fd);
    if (log)
        fclose(log_f);
    return 1;
}
