#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Raspberry Pi 3 Peripheral Base Address
#include <stdint.h>

#define BCM2837_PERI_BASE	0x3F000000
#define GPIO_BASE	(BCM2837_PERI_BASE + 0x200000)

#define BLOCK_SIZE	(4096)

int io_fd;
void *gpio_map;

// Pointer to the global GPIO map
volatile unsigned *gpio;

// GPIO setup macros
#define INP_GPIO(g)	*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)	*(gpio+((g)/10)) |= (1<<(((g)%10)*3))

#define GPIO_SET(g)	*(gpio + 7) = 1<<g	// sets bits which are 1
#define GPIO_CLR(g)	*(gpio + 10) = 1<<g	// clears bits which are 1

void setup_io() {
	/* Open /dev/gpiomem */
	if ((io_fd = open("/dev/gpiomem", O_RDWR | O_SYNC)) < 0) {
		printf("error opening /dev/gpiomem \n");
		exit(-1);
	}

	/* mmap GPIO */
	gpio_map = mmap(
		NULL,			// Any addess in our space will do
		BLOCK_SIZE,		// Map length
		PROT_READ|PROT_WRITE,	// Enable reading & writing to mapped memory
		MAP_SHARED,		// Shared with other processes
		io_fd,			// File to map
		GPIO_BASE		// Offset to GPIO peripheral
	);

	close(io_fd); // The mmap keeps the fd open

	if (gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}

	// Always use volatile pointer!
	gpio = (volatile unsigned *)gpio_map;
}

int pi3_gpios[] = {7, 8, 14, 15, 18, 23, 24, 25};
int digit_map[10][8] = {
	{1, 1, 1, 1, 1, 1, 0, 1},	// 0
	{1, 0, 1, 1, 0, 0, 0, 0},	// 1
	{1, 1, 0, 1, 0, 1, 1, 1},	// 2
	{1, 1, 1, 1, 0, 1, 1, 0},	// 3
	{1, 0, 1, 1, 1, 0, 1, 0},	// 4
	{0, 1, 1, 1, 1, 1, 1, 0},	// 5
	{0, 1, 1, 1, 1, 1, 1, 1},	// 6
	{1, 0, 1, 1, 0, 1, 0, 0},	// 7
	{1, 1, 1, 1, 1, 1, 1, 1},	// 8
	{1, 1, 1, 1, 1, 1, 1, 0},	// 9
};
int num_gpios = sizeof(pi3_gpios) / sizeof(pi3_gpios[0]);

int main(int argc, char **argv) {
	int i = 0;
	int init_idx = 0;
	int digit = 0;
	int digit_pin_idx = 0;

	// Initialize memory mapping
	setup_io();

	// Set all listed GPIOs to output mode
	for (int i = 0; i < num_gpios; i++) {
		int g = pi3_gpios[i];
		INP_GPIO(g);
		OUT_GPIO(g);
	}

	for (init_idx = 0; init_idx < num_gpios; init_idx++) {
		GPIO_CLR(pi3_gpios[init_idx]);
	}

	while (1) {
		printf ("%d \n", digit%10);

		for (digit_pin_idx = 0; digit_pin_idx < 8; digit_pin_idx++) {
			if (digit_map[digit%10][digit_pin_idx]) {
				GPIO_SET(pi3_gpios[digit_pin_idx]);
			}
		}

		sleep(1);

		digit++;

		for (init_idx = 0; init_idx < num_gpios; init_idx++) {
			GPIO_CLR(pi3_gpios[init_idx]);
		}


	}
	return 0;
}