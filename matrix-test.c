/** \file
 * Test the matrix LCD PRU firmware with a multi-hue rainbow.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"


// Borrowed by OctoWS2811 rainbow test
static unsigned int
h2rgb(
	unsigned int v1,
	unsigned int v2,
	unsigned int hue
)
{
	if (hue < 60)
		return v1 * 60 + (v2 - v1) * hue;
	if (hue < 180)
		return v2 * 60;
	if (hue < 240)
		return v1 * 60 + (v2 - v1) * (240 - hue);

	return v1 * 60;
}


// Convert HSL (Hue, Saturation, Lightness) to RGB (Red, Green, Blue)
//
//   hue:        0 to 359 - position on the color wheel, 0=red, 60=orange,
//                            120=yellow, 180=green, 240=blue, 300=violet
//
//   saturation: 0 to 100 - how bright or dull the color, 100=full, 0=gray
//
//   lightness:  0 to 100 - how light the color is, 100=white, 50=color, 0=black
//
static uint32_t
makeColor(
	unsigned int hue,
	unsigned int saturation,
	unsigned int lightness
)
{
	unsigned int red, green, blue;
	unsigned int var1, var2;

	if (hue > 359)
		hue = hue % 360;
	if (saturation > 100)
		saturation = 100;
	if (lightness > 100)
		lightness = 100;

	// algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
	if (saturation == 0) {
		red = green = blue = lightness * 255 / 100;
	} else {
		if (lightness < 50) {
			var2 = lightness * (100 + saturation);
		} else {
			var2 = ((lightness + saturation) * 100) - (saturation * lightness);
		}
		var1 = lightness * 200 - var2;
		red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / 600000;
		green = h2rgb(var1, var2, hue) * 255 / 600000;
		blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / 600000;
	}
	return (red << 16) | (green << 8) | blue;
}



static uint32_t rainbowColors[180];


// phaseShift is the shift between each row.  phaseShift=0
// causes all rows to show the same colors moving together.
// phaseShift=180 causes each row to be the opposite colors
// as the previous.
//
// cycleTime is the number of milliseconds to shift through
// the entire 360 degrees of the color wheel:
// Red -> Orange -> Yellow -> Green -> Blue -> Violet -> Red
//
static void
rainbow(
	uint32_t * const pixels,
	unsigned num_leds,
	unsigned phaseShift,
	unsigned cycle
)
{
	const unsigned color = cycle % 180;

	for (unsigned x=0; x < num_leds; x++) {
		for (unsigned y=0; y < 16; y++) {
			const int index = (color + x + y*phaseShift/2) % 180;
                        const uint32_t in  = rainbowColors[index];
			uint8_t * const out = &pixels[x + y*num_leds];
#if 1
                        out[0] = ((in >> 0) & 0xFF); // * y / 16;
                        out[1] = ((in >> 8) & 0xFF); // * y / 16;
                        out[2] = ((in >> 16) & 0xFF); // * y / 16;
#else
                        //out[0] = ((in >> 0) & 0xFF);
                        //out[1] = ((in >> 8) & 0xFF);
                        //out[2] = ((in >> 16) & 0xFF);
                        out[0] = y + 3*x + cycle;
                        out[1] = y + 3*x + cycle;
                        out[2] = y + 3*x + cycle;
#endif
		}
	}
}


int
main(void)
{
	const int num_pixels = 256;
	ledscape_t * const leds = ledscape_init(num_pixels);
	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	// pre-compute the 180 rainbow colors
	for (int i=0; i<180; i++)
	{
		int hue = i * 2;
		int saturation = 100;
		int lightness = 50;
		rainbowColors[i] = makeColor(hue, saturation, lightness);
	}

	unsigned i = 0;
	while (1)
	{
		// Alternate frame buffers on each draw command
		const unsigned frame_num = i++ % 2;
		ledscape_frame_t * const frame
			= ledscape_frame(leds, frame_num);

		uint32_t * const p = (void*) frame;

		rainbow(p, num_pixels, 10, i);
		ledscape_draw(leds, frame_num);
		usleep(20000);

		// wait for the previous frame to finish;
		//const uint32_t response = ledscape_wait(leds);
		const uint32_t response = 0;
		time_t now = time(NULL);
		if (now != last_time)
		{
			printf("%d fps. starting %d previous %"PRIx32"\n",
				i - last_i, i, response);
			last_i = i;
			last_time = now;
		}

	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
