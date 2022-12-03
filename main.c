/*
	SpiroMorph - Michael James Clift.

	#install sdl2
	sudo apt install libsdl2-dev libsdl2-2.0-0 -y;

	The following resources were used to help write this:
	https://www.geeksforgeeks.org/sdl-library-in-c-c-with-examples/
	https://dzone.com/articles/sdl2-pixel-drawing
*/

	#include <assert.h>
	#include <stdio.h>
	#include <stddef.h>
	#include <stdbool.h>
	#include <stdint.h>
	#include <inttypes.h>
	#include <math.h>

	#include <SDL.h>

	#include "main.h"
	#include "mainopt.h"

//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	static float element_radius;

	#define NO_FLAGS 0
	#define REND_INDEX_AUTO	-1

	struct element_struct
	{
		int 	freq;			// +/- integer factor of base circle (non-0)
		float 	amplitude;		// 0-1
		int 	phase_offset;	// 0-base_resolution-1
	};

	struct envelope_struct
	{
		float offset;		// 0 - 1
		bool reset;
	};

	typedef struct
	{
		float x;
		float y;
	} point_f;

	struct colour_struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************
 
//********************************************************************************************************
// Private variables
//********************************************************************************************************

	static float *sin_table;
	static struct colour_struct *colour_table;

	static struct element_struct *elements;
	static struct envelope_struct *envelopes;

	static int center_x;
	static int center_y;

	static struct mainopt_struct options;

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	static void spiromorph(void);

	static void generate_sin_table(void);
	static void draw_frame(SDL_Renderer* rend);
	static void draw_line_to(SDL_Renderer* rend, SDL_Point coord);
	static point_f calc_point_of_circle(int brads, float radius);
	static point_f calc_point_of_element(struct element_struct element, int base_angle);
	static point_f sum_points_f(point_f a, point_f b);
	static void draw_point_for_base_angle(SDL_Renderer* rend, int base_angle);
	static uint32_t get_elapsed_u32(uint32_t current);
	static float raised_cosine(float position);
	static int rand_in_range_i(int min, int max);
	static float rand_in_range_f(float min, float max);
	static void envelope_init(void);
	static void colour_table_init(void);

//********************************************************************************************************
// Public functions
//********************************************************************************************************

int main(int argc, char *argv[])
{
	int retval = 0;

	//	Parse options, 
	options = mainopt_parse(argc, argv);

	if(options.error)
		retval = -1;

	if(!options.finished)
		spiromorph();

	return retval;
}

static void spiromorph(void)
{
	uint32_t render_flags = SDL_RENDERER_PRESENTVSYNC;
	float 	frame_time;
	float 	base_envelope = 0;
	float 	envelope;
	float 	second_time = 0;
	int i;
	int err;
	int frame_cnt = 0;
	bool finished = false;

	SDL_Window* 	win;
	SDL_Renderer* 	rend;
	SDL_Event 		event; 

	elements = malloc(sizeof(*elements)*options.number_of_elements);
	envelopes = calloc(sizeof(*envelopes), options.number_of_elements);
	sin_table = malloc(sizeof(*sin_table)*options.base_resolution);
	colour_table = malloc(sizeof(*colour_table)*options.base_resolution);

	center_x	= options.window_width / 2;
	center_y	= options.window_height / 2;
  	element_radius = (options.amplitude * 0.5 * options.window_height / ((options.number_of_elements - options.envelopes_in_phase + 1)*0.5 + (options.envelopes_in_phase - 1)*1.0));

	i = 0;
	while(i != options.number_of_elements)
		elements[i++] = (struct element_struct){.amplitude=1, .freq=1, .phase_offset=0};

	SDL_Log(MAIN_TITLE"  "MAIN_VERSION"  Mick Clift 2022\n");

	generate_sin_table();
	envelope_init();
	colour_table_init();

	err = SDL_Init(SDL_INIT_EVERYTHING);
	assert(!err);
 
 	if(options.full_screen)
		win = SDL_CreateWindow(MAIN_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, options.window_width, options.window_height, 0 + SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		win = SDL_CreateWindow(MAIN_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, options.window_width, options.window_height, 0);

	rend = SDL_CreateRenderer(win, REND_INDEX_AUTO, render_flags);

	while(!finished)
	{
		while(SDL_PollEvent(&event))
		{ 
			switch (event.type)
			{
			case SDL_QUIT: 
				// handling of close button 
				finished = true;
				break; 

			case SDL_KEYDOWN: 
				// keyboard API for key pressed 
				switch (event.key.keysym.scancode)
				{ 
				case SDL_SCANCODE_W: 
				case SDL_SCANCODE_UP: 
					break; 
				case SDL_SCANCODE_A: 
				case SDL_SCANCODE_LEFT: 
					break; 
				case SDL_SCANCODE_S: 
				case SDL_SCANCODE_DOWN: 
					break; 
				case SDL_SCANCODE_D: 
				case SDL_SCANCODE_RIGHT: 
					break; 
				};
			};
		};

		frame_time = (float)get_elapsed_u32(SDL_GetTicks()) / 1000.0;
		second_time += frame_time;
		if(second_time > 1.0)
		{
			second_time = 0;
			SDL_Log("fps = %i\n", frame_cnt);
			frame_cnt = 0;
		};

		i = 0;

		//rotate base envelope
		base_envelope += options.envelope_speed * frame_time;
		if(base_envelope >= 1.0)
			base_envelope -= 1.0;

		while(i != options.number_of_elements)
		{
			envelope = envelopes[i].offset + base_envelope;
			if(envelope >= 1.0)
				envelope -= 1.0;

			if(envelope < 0.5 && !envelopes[i].reset)
			{
				elements[i].freq = rand_in_range_i(1, options.element_freq_max);
				if(rand()&1)
					elements[i].freq *=-1;
				elements[i].phase_offset = rand_in_range_i(0, options.base_resolution);	
			};
			envelopes[i].reset = envelope < 0.5;
			elements[i].amplitude = raised_cosine(envelope);
			i++;
		};

		draw_frame(rend);
		SDL_RenderPresent(rend);
		frame_cnt++;  
	};
  
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
} 

//********************************************************************************************************
// Private functions
//********************************************************************************************************


static void envelope_init(void)
{
	int i = 0;
	while(i != options.number_of_elements)
		envelopes[i++].offset = 0;

	i = 0;
	while(i != options.number_of_elements - options.envelopes_in_phase + 1)
	{
		envelopes[i].offset = (float)i / (float)(options.number_of_elements - options.envelopes_in_phase + 1);
		i++;
	};
}

static float raised_cosine(float position)
{
	int brads;

	brads = (position * options.base_resolution);
	brads += options.base_resolution/4;
	brads &= options.base_resolution-1;

	return (sin_table[brads]/-2.0)+0.5;
}

static int rand_in_range_i(int min, int max)
{
	int value;

	value = rand() % abs(max-min);

	if(min < max)
		value += min;
	else
		value += max;

	return value;
}

static float rand_in_range_f(float min, float max)
{
	float value;

	value = (float)rand() / RAND_MAX;
	value *= fabs(max-min);

	if(min < max)
		value += min;
	else
		value += max;

	return value;
}

static void generate_sin_table(void)
{
	int brads = 0;
	float q;

	while(brads != options.base_resolution)
	{
		q = (float)brads / options.base_resolution;
		q *= 2 * M_PI;
		sin_table[brads] = sin(q);
		brads++;
	};
}

static void colour_table_init(void)
{
	int i = 0;
	float pos;

	while(i != options.base_resolution)
	{
		pos = i;
		pos /= (float)options.base_resolution;

		colour_table[i].r = 255.0*raised_cosine(pos + 0.0000);
		colour_table[i].g = 255.0*raised_cosine(pos + 0.3333);
		colour_table[i].b = 255.0*raised_cosine(pos + 0.6666);
		i++;
	};
}

static void draw_frame(SDL_Renderer* rend)
{
	int base_angle = 0;

	SDL_SetRenderDrawColor(rend, 0,0,0,SDL_ALPHA_OPAQUE);

	SDL_RenderClear(rend);
	draw_point_for_base_angle(rend, 0);

	SDL_SetRenderDrawColor(rend, 255,255,255,SDL_ALPHA_OPAQUE);
	base_angle = 1;
	while(base_angle != options.base_resolution)
	{
		draw_point_for_base_angle(rend, base_angle);
		base_angle++;
	};
}

static void draw_point_for_base_angle(SDL_Renderer* rend, int base_angle)
{
	SDL_Point point_final;
	point_f point = {0.0};
	int i;

	i = 0;
	while(i != options.number_of_elements)
	{
		point = sum_points_f(point, calc_point_of_element(elements[i], base_angle));
		i++;
	};

	SDL_SetRenderDrawColor(rend, colour_table[base_angle].r, colour_table[base_angle].g, colour_table[base_angle].b, SDL_ALPHA_OPAQUE);

	point_final.x = center_x + point.x;
	point_final.y = center_y + point.y;
	draw_line_to(rend, point_final);
}

static void draw_line_to(SDL_Renderer* rend, SDL_Point coord)
{
	static SDL_Point last_point = {0,0};

	SDL_RenderDrawLine(rend, last_point.x, last_point.y, coord.x, coord.y);
	last_point = coord;
}

static point_f calc_point_of_element(struct element_struct element, int base_angle)
{
	point_f point;
	int freq;
	float amplitude;

	freq = element.freq * base_angle;
	freq += element.phase_offset;

	amplitude = element.amplitude * element_radius;
	point = calc_point_of_circle(freq, amplitude);

	return point;
}

static point_f calc_point_of_circle(int brads, float radius)
{
	point_f point;

	brads &= options.base_resolution-1;
	point.x = sin_table[brads] * radius;

	brads += options.base_resolution/4;
	brads &= options.base_resolution-1;	
	point.y = sin_table[brads] * radius;		

	return point;
}

static point_f sum_points_f(point_f a, point_f b)
{
	point_f sum;

	sum.x = a.x + b.x;
	sum.y = a.y + b.y;

	return sum;
}

static uint32_t get_elapsed_u32(uint32_t current)
{
	static uint32_t last = 0;
	uint32_t elapsed;

	elapsed = current - last;
	last = current;

	return elapsed;
}

