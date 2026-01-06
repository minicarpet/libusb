/*
* Test suite program based of libusb-0.1-compat testlibusb
* Copyright (c) 2013 Nathan Hjelm <hjelmn@mac.ccom>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <string.h>
#include "libusb.h"
#include "libusbi.h"

int verbose = 0;

#ifdef __linux__
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static int test_wrapped_device(const char *device_name)
{
	libusb_device_handle *handle;
	int r, fd;

	fd = open(device_name, O_RDWR);
	if (fd < 0) {
		printf("Error could not open %s: %s\n", device_name, strerror(errno));
		return 1;
	}
	r = libusb_wrap_sys_device(NULL, fd, &handle);
	if (r) {
		printf("Error wrapping device: %s: %s\n", device_name, libusb_strerror(r));
		close(fd);
		return 1;
	}
	print_device(libusb_get_device(handle), handle);
	close(fd);
	return 0;
}
#else
static int test_wrapped_device(const char *device_name)
{
	(void)device_name;
	printf("Testing wrapped devices is not supported on your platform\n");
	return 1;
}
#endif

void my_lib_usb_log_cb(libusb_context* ctx, enum libusb_log_level level, const char* str)
{
	UNUSED(ctx);
	UNUSED(level);
	printf("%s", str);
}

int main(int argc, char *argv[])
{
	const char *device_name = NULL;
	libusb_device **devs;
	libusb_device* stored_device[10] = { NULL };
	libusb_device_handle* handle_device[10] = { NULL };
	size_t index = 0;
	ssize_t cnt;
	int r, i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			verbose = 1;
		} else if (!strcmp(argv[i], "-d") && (i + 1) < argc) {
			i++;
			device_name = argv[i];
		} else {
			printf("Usage %s [-v] [-d </dev/bus/usb/...>]\n", argv[0]);
			printf("Note use -d to test libusb_wrap_sys_device()\n");
			return 1;
		}
	}

	r = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if (r < 0)
		return r;

	libusb_set_log_cb(NULL, my_lib_usb_log_cb, LIBUSB_LOG_CB_GLOBAL);
	libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

	if (device_name) {
		r = test_wrapped_device(device_name);
	} else {
		for (size_t j = 0; j < 2; j++)
		{
			printf("get device list\n");
			cnt = libusb_get_device_list(NULL, &devs);
			if (cnt < 0) {
				libusb_exit(NULL);
				return 1;
			}

			for (i = 0; devs[i]; i++)
			{
				if (j == 0 && devs[i]->device_descriptor.idVendor == 0x13B9)
				{
					stored_device[index++] = devs[i];
					printf("ref hexagon device\n");
					libusb_ref_device(devs[i]);
				}
			}

			printf("free device list\n");
			libusb_free_device_list(devs, 1);
			printf("list ref devices\n");
			libusb_listref_devices(NULL);
		}
	}

	printf("open hexagon devices\n");
	for (size_t t = 0; t < index; t++)
	{
		r = libusb_open(stored_device[t], &handle_device[t]);
		libusb_listref_devices(NULL);
		if (r < 0)
		{
			printf("Failed to open device !");
			libusb_exit(NULL);
			return r;
		}
	}

	printf("list ref devices\n");
	libusb_listref_devices(NULL);

	printf("unref hexagon devices\n");
	for (size_t t = 0 ; t<index ; t++)
	{
		libusb_close(handle_device[t]);
		libusb_listref_devices(NULL);
		libusb_unref_device(stored_device[t]);
	}

	printf("list ref devices\n");
	libusb_listref_devices(NULL);
	libusb_exit(NULL);
	return r;
}
