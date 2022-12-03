/*
    Process applications command line options
*/

//	System <includes>
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <unistd.h>
    #include <getopt.h>

//	Application "includes"
    #include "main.h"
    #include "mainopt.h"

//*******************************************************
// Defines
//*******************************************************

//	For conversions where no conversion type is given
    #define EOL_DEFAULT "\n"

//	Valid short options
    #define VALIDOPTS	"Vhe:p:x:y:wa:f:s:"

//	Short ID's for long options, avoid conflict with above characters
    #define OPT_VERSION			'V'
    #define OPT_HELP			'h'
    #define OPT_FULL_SCREEN		'w'
    #define OPT_ELEMENTS		'e'
    #define OPT_ENV_IN_PHASE	'p'
    #define OPT_F_MAX			'f'
    #define OPT_BASE_RES		'r'
    #define OPT_XRES			'x'
    #define OPT_YRES			'y'
    #define OPT_SPEED			's'
    #define OPT_AMPLITUDE		'a'

    #define MSG_USAGE "\n\
Usage: "MAIN_TITLE" [OPTIONS] \n\
\n\
\n\
  -w, --fullscreen      run in fullscreen\n\
  -e, --elements=N      number of elements (1 -> N) default 5\n\
  -p, --inphase=N       number of envelopes in phase (1 -> number of elements) default 2\n\
  -f, --fmax=N          highest frequency (1 -> N) default 8\n\
  -r, --loopres=N       loop resolution (256 -> N) default 1024 *must be power of 2*\n\
  -x, --xres=N          X window size, default 1000\n\
  -y, --yres=N          Y window size, default 1000\n\
  -s, --speed=N         Envelope speed, default 0.15\n\
  -a, --amplitude=N     Amplitude adjustment, default 1.0\n\
\n\
\n\
  -h, --help            display this help\n\
  -V, --version         display version information\n\
\n\
Examples:\n\
    "MAIN_TITLE" --xres=1920 --yres=1080 --fullscreen\n\
    "MAIN_TITLE" --loopres=4096 --elements=12 --inphase=1 --amplitude=2.0\n\
\n\
"

    #define MSG_VERSION "\n\
"MAIN_TITLE" "MAIN_VERSION" \n\
Copyright (C) 2022  Michael Clift\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software, and you are welcome to redistribute it.\n\
This program comes with ABSOLUTELY NO WARRANTY.\n\
\n\
"

//*******************************************************
// Variables
//*******************************************************

    static struct mainopt_struct opt_defaults = 
    {
        .full_screen = false,
        .number_of_elements = 5,
        .envelopes_in_phase = 2,
        .element_freq_max	= 8,
        .base_resolution = 1024,
        .window_width	 = 1000,
        .window_height = 1000,
        .envelope_speed = 0.15,
        .amplitude = 1.0,
        .finished = false,
    };

    static struct option long_options[] = {
        {"fullscreen", no_argument,       NULL, OPT_FULL_SCREEN},
        {"elements",   required_argument, NULL, OPT_ELEMENTS},
        {"inphase",    required_argument, NULL, OPT_ENV_IN_PHASE},
        {"fmax",       required_argument, NULL, OPT_F_MAX},
        {"loopres",    required_argument, NULL, OPT_BASE_RES},
        {"xres",       required_argument, NULL, OPT_XRES},
        {"yres",       required_argument, NULL, OPT_YRES},
        {"speed" ,     required_argument, NULL, OPT_SPEED},
        {"amplitude",  required_argument, NULL, OPT_AMPLITUDE},
        {"help" ,      no_argument, NULL, OPT_HELP},
        {"version" ,   no_argument, NULL, OPT_VERSION},
        {0,0,0,0},
    };

//*******************************************************
// Prototypes
//*******************************************************

// 	Process a single option
    static void process_option(struct mainopt_struct *retval, int x, const char* argument);

//	Check options are valid
    static void validate_options(struct mainopt_struct *retval);

    static bool isPowerOfTwo(int x);

//*******************************************************
// Functions
//*******************************************************

struct mainopt_struct mainopt_parse(int argc, char *argv[])
{
    int x = 0;
    bool finished = false;

    struct mainopt_struct retval = opt_defaults;

    do
    {
        x = getopt_long(argc, argv, VALIDOPTS, long_options, NULL);
        if(x != -1)
        {
            process_option(&retval, x, optarg);
            if(retval.finished)		//(program may finish after displaying help or version)
                finished = true;
        }
        else 
            finished = true;

    }while(!finished);

    if(!retval.finished)
        validate_options(&retval);

    return retval;
}

//*******************************************************
// Private Functions
//*******************************************************

// process a single option
static void process_option(struct mainopt_struct *retval, int x, const char* argument)
{
//	Invalid option
    if(x == '?')
    {
        retval->finished = true;
        retval->error = true;
    }
//	Show help message
    if(x == OPT_HELP)
    {
        printf(MSG_USAGE);
        retval->finished = true;
    }
//	Show version message
    else if(x == OPT_VERSION)
    {
        printf(MSG_VERSION);
        retval->finished = true;
    }
//	Full screen
    else if(x == OPT_FULL_SCREEN)
    {
        retval->full_screen = true;
    }
    else if(x == OPT_ELEMENTS)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->number_of_elements = atoi(argument);
    }
    else if(x == OPT_ENV_IN_PHASE)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->envelopes_in_phase = atoi(argument);
    }
    else if(x == OPT_F_MAX)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->element_freq_max = atoi(argument);
    }
    else if(x == OPT_BASE_RES)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->base_resolution = atoi(argument);
    }
    else if(x == OPT_XRES)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->window_width = atoi(argument);
    }
    else if(x == OPT_YRES)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->window_height = atoi(argument);
    }
    else if(x == OPT_SPEED)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->envelope_speed = atof(argument);
    }
    else if(x == OPT_AMPLITUDE)
    {
        if(!argument)
            retval->finished = true;
        else
            retval->amplitude = atof(argument);
    };
}

static void validate_options(struct mainopt_struct *retval)
{
    if(retval->number_of_elements == 0)
    {
        fprintf(stderr, "*** ERROR Number of elements must not be 0\n");
        retval->finished = true;
        retval->error = true;
    };

    if(retval->envelopes_in_phase > retval->number_of_elements)
    {
        fprintf(stderr, "*** ERROR Envelopes in phase must not exceed number of elements (%i)\n", retval->number_of_elements);
        retval->finished = true;
        retval->error = true;
    };

    if(retval->envelopes_in_phase == 0)
        retval->envelopes_in_phase = 1;

    if(retval->element_freq_max == 0)
        retval->element_freq_max = 1;

    if(!isPowerOfTwo(retval->base_resolution))
    {
        fprintf(stderr, "*** ERROR Loop resolution must be a power of 2 ie. 512, 1024, 2048, 4096, 8192...\n");
        retval->finished = true;
        retval->error = true;
    };

    if(retval->window_width < 10)
        retval->window_width = 10;

    if(retval->window_height < 10)
        retval->window_height = 10;
}

static bool isPowerOfTwo(int x)
{
    return x && (!(x & (x - 1)));
}