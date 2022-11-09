
//********************************************************************************************************
// Public defines
//********************************************************************************************************

//	structure retuned by mainopt_parse
	struct mainopt_struct 
	{
		bool full_screen;
		int number_of_elements;
		int envelopes_in_phase;
		int element_freq_max;
		int base_resolution;
		int window_width;
		int window_height;
		float envelope_speed;
		float amplitude;
		bool finished;
		bool error;
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

//	Process command line options.
//	Small tasks, such as displaying help or version are handled internally.
	struct mainopt_struct mainopt_parse(int argc, char *argv[]);
