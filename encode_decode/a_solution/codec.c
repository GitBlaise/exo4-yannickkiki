/* Copyright (c) 2018, Judicael Zounmevo
 * Educational material for CECEONAT, 2018.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>

// Assuming x86_64. If not it would still work with
// possibly a slight performance loss. WORD_SIZE and CACHE_LINE_SIZE can
// otherwise be discovered by sysconf on *nix systems.
#define CACHE_LINE_SIZE 64  //Should be true for most (if not all) x86_64
#define WORD_SIZE	8  

#define UNUSED(x) (void)(x) //Crush unused var warning 
														//when using severe compiling options
														//... which you should always use.
#define FREE(x) do { free((x)); (x) = NULL; }while(FALSE)
typedef enum BOOL
{
	FALSE,
	TRUE = !FALSE
}BOOL;

BOOL is_power_of_2(size_t n)
{
	if(n <=1)
		return FALSE;
	return (n & (n-1)) != 0;
}


#ifdef _MSC_VER
int posix_memalign(void** ptr, size_t align, size_t size)
{
	*ptr = _aligned_malloc(size, align);
	if(ptr != NULL)
		return 0;
	if((align%sizeof(void*)) != 0 || !is_power_of_2(align))
		return EINVAL;
	return ENOMEM;
}
#endif
										 
// Make <name> a cache line-sized cache-aligned buffer of <type>
#define STACK_CACHE_ALLIGNED_BUF(type, name)														\
	uintptr_t __hidden_buf ## name[2*CACHE_LINE_SIZE/sizeof(uintptr_t)];	\
	type* name;																														\
	{																																			\
		uintptr_t* buf_ptr = __hidden_buf ## name;													\
		while( (((size_t)buf_ptr) % CACHE_LINE_SIZE) != 0) ++buf_ptr;				\
		name = (type*)(buf_ptr);																						\
	}																												

#define UNDEFINED_CODEC (-1)


struct codec_t;
typedef void (*codec_function_t)(const char* input_file, void** args,
		BOOL args_is_filename);
typedef void (*codec_cleanup_func_t)(struct codec_t*);
void nop(struct codec_t* codec) { UNUSED(codec); }
typedef struct codec_t
{
	codec_function_t encode;
	codec_function_t decode;
	codec_cleanup_func_t cleanup;
	void* args;  
	const char* name;
	const char* description;
}codec_t;

typedef enum e_option_kind_t
{
	UNDEFINED_OPTION_KIND,
	LIST,
	ENCODE,
	DECODE,
}e_option_kind_t;

typedef struct option_t
{
	e_option_kind_t kind;
	int codec_number;
	char* input_filename;
	void* args; //For codec functions that require it. (e.g. to hold passwords)
	BOOL args_is_filename; // if TRUE, args is a filename to read
											// the actual args from
}option_t;

typedef struct password_t
{
	uint8_t* buf;
	int64_t length;
	int64_t allocated_length;
}password_t;

#define DIE(...) \
	do																																		\
	{																																			\
		printf("\033[1;31m"); printf(__VA_ARGS__); printf("\033[0;m\n\n");	\
		fflush(stdout);																											\
		exit(EXIT_FAILURE);																									\
	}while(FALSE)

#define DIE_IF(cond, ...) do { if((cond)) DIE(__VA_ARGS__); } while(FALSE)

void print_help(const char* app_name)
{
	printf("\n%s - Encode or decode files with one of a few supported codecs.\n"
			"The codec application is non-destructive for the input file.\n\n"
			"- For an input file named \"my_file\", the produced encoded file is\n"
			"  named \"enc_my_file\" and the resulting decoded file is named \n"
			"  \"dec_my_file\".\n"
			"- For encode (-e) and decode (-d) operations, the filename must be\n"
			"  the last argument on the command line."
			"\n\nSYNOPSIS\n"
			"\t%s [OPTIONS [input_filename]]"
			"\n\nOPTIONS\n"
			"\t-h\n\t\tDisplay this help.\n\n"
			"\t-l\n\t\tList available codecs.\n\n"
			"\t-c codec_name\n\t\tSpecify codec name.\n\n"
			"\t-e\n\t\tEncode. -e and -d are mutually exclusive.\n\n"
			"\t-d\n\t\tDecode. -e and -d are mutually exclusive.\n\n"
			"\t-p passphrase\n"
			"\t\tSome codecs require a passphrase (quote-enclosed).\n"
			"\t\tThe same passphrase provided for encoding must be used\n"
			"\t\tfor decoding later on.\n\n"
			"\t-P passphrase_file\n"
			"\t\tThis option is like -p; but allows the passphrase to be\n"
		  "\t\tread from a file.\n"
			"\n\nEXAMPLES:\n"
			"\t%s -e -c inv my_file\n"
			"\t%s -e -c 1 my_file  # Here 1 is the number of the inv codec.\n"
			"\t%s -d -c inv my_other_file\n"
			"\t%s -d -c 1 my_other_file\n"
			"\t%s -e -c 2 -p \"This is my passphrase\" my_file\n"
			"\t%s -e -c 2 -P passphrase_filename my_file\n\n\n",
			app_name, app_name, app_name, app_name, app_name, app_name,
			app_name, app_name);
}

/*Returns -1 if name is not found.
 * */
int get_codec_number(const char* codec_name_or_num,
		const codec_t codecs[], int num_codecs)
{
	int i;
	for(i=0; i<num_codecs; i++)
	{
		if(strcmp(codecs[i].name, codec_name_or_num) == 0 || 
				i + 1 == atoi(codec_name_or_num))
			return i;
	}
	return -1;
}

/*Return the password as a cache-aligned byte-array.
 * */
password_t* read_password_from_file(const char* filename)
{
	password_t* pass = malloc(sizeof(password_t));
	FILE* file = fopen(filename, "rb");
	DIE_IF(file == NULL, 
			"Failed to open password file %s! \"%s\"", filename, strerror(errno));
	fseek(file, 0L, SEEK_END);
	pass->length = ftell(file);
	pass->allocated_length = pass->length;
	while((pass->allocated_length % CACHE_LINE_SIZE) != 0)
		++pass->allocated_length;
	fseek(file, 0L, SEEK_SET);
	int status = posix_memalign((void**)(&pass->buf), CACHE_LINE_SIZE,
			pass->allocated_length);
	DIE_IF(status != 0, "Posix_memalign failed for passphrase reading");
	fread((void*)(pass->buf), pass->length, 1, file);
	fclose(file);
	return pass;
}

password_t* make_password_from_cmdline(const char* password)
{
	password_t* pass = malloc(sizeof(password_t));
	pass->length = strlen(password);
	pass->allocated_length = pass->length;
	while((pass->allocated_length % CACHE_LINE_SIZE) != 0) 
		++pass->allocated_length;
	int status = posix_memalign((void**)(&pass->buf), CACHE_LINE_SIZE,
			pass->allocated_length);
	DIE_IF(status != 0, "Posix_memalign failed for passphrase reading");
	memcpy(pass->buf, password, pass->length);
	return pass;
}



void parse_cmd(int argc, char** argv, codec_t codecs[], int num_codecs,
		option_t* options)
{
	/*NOTE: Ideally, parse_cmd should be using getopts from unistd.h.
	 * getopts is not available on Windows by default; so resorting to some 
	 * quick hack instead.
	 * */
	int i;
	char opt;
	if(argc == 1)
	{
		print_help(argv[0]);
		exit(EXIT_SUCCESS);
	}
	for(i=1; i<argc; i++)
	{
		if(strcmp(argv[i], "-h") == 0)
		{
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}
	for(i=1; i<argc; i++)
	{
		if(argv[i][0] == '-')
		{
			DIE_IF(argv[i][1] == 0, "Option expected after \'-\'");
			opt = argv[i][1];
			DIE_IF(argv[i][2] != 0, "Options must be a single character");
			switch(opt)
			{
				case 'l':
					DIE_IF(i < argc-1, 
							"The -l option is exclusive and takes no argument");
					options->kind = LIST;
					break;
				case 'e':
				case 'd':
					DIE_IF((options->kind == ENCODE && opt == 'd') ||
							(options->kind == DECODE && opt == 'e'),
							"-e and -d are mutually exclusive");
					options->kind = opt == 'e' ? ENCODE : DECODE;
					break;
				case 'c':
					DIE_IF(++i == argc, "Codec name or number required");
					options->codec_number = get_codec_number(argv[i], codecs, 
							num_codecs);
					DIE_IF(options->codec_number == UNDEFINED_CODEC, "Unknown codec");
					break;
				case 'p':
					DIE_IF(++i == argc, "Passphrase required after -p option");
					DIE_IF(argv[i][0] == 0, "Empty passphrase!");
					options->args = strdup(argv[i]);
					options->args_is_filename = FALSE;
					break;
				case 'P':
					DIE_IF(++i == argc, "Passphrase file required after -P option");
					DIE_IF(argv[i][0] == 0, "Empty passphrase filename!");
					options->args = strdup(argv[i]);
					options->args_is_filename = TRUE;
					break;
				default:
					DIE("Unsupported option encountered!");
			}
		}
		else //The input file name (last expected argument if applicable)
		{
			DIE_IF(i < argc-1,
					"A single input file name is required as LAST argument!");
			options->input_filename = strdup(argv[i]);
		}
	}
	
	//Sanity check! ... among other things ...
	if(options->kind == ENCODE || options->kind == DECODE)
	{
		DIE_IF(options->input_filename == NULL, "Input file name required!");
		DIE_IF(options->codec_number == UNDEFINED_CODEC,
				"Codec name or number required!");
		codecs[options->codec_number].args = options->args;
		options->args = NULL;
	}
	DIE_IF(options->kind == UNDEFINED_OPTION_KIND,
			"No operation specified! Try with -h for help");
}

#ifdef __TRACE
#define TRACE(...) do { printf(__VA_ARGS__); fflush(stdout); } while(FALSE)
#else
#define TRACE(...)
#endif


/*Caller frees it!
 * */
char* make_derived_filename(const char* prefix, const char* root)
{
	char* derived = malloc(strlen(prefix) + strlen(root) + 1);
	sprintf(derived, "%s%s", prefix, root);
	return derived;
}

typedef struct file_manip_data_t
{
	char* out_filename;
	FILE* in_file;
	FILE* out_file;
	int64_t length; // in bytes
}file_manip_data_t;

typedef enum e_file_op_t
{
	FO_ENCODE,
	FO_DECODE
}e_file_op_t;

file_manip_data_t get_file_namip_data(
		const char* input_filename, e_file_op_t op_type)
{
	FILE* input_file = fopen(input_filename, "rb");
	DIE_IF(input_file == NULL,
			"Opening %s failed! \"%s\"", input_filename, strerror(errno));
	char* output_filename = 
		make_derived_filename(op_type == FO_ENCODE ? "enc_" : "dec_",
				input_filename);
	FILE* output_file = fopen(output_filename, "wb");
	DIE_IF(output_file == NULL,
			"Opening %s failed! \"%s\"", input_filename, strerror(errno));
	fseek(input_file, 0L, SEEK_END);
	int64_t length = ftell(input_file);
	fseek(input_file, 0L, SEEK_SET);
	file_manip_data_t fmdata = {output_filename, input_file,
		output_file, length};
	return fmdata;
}

void free_file_manip_data(file_manip_data_t* fmdata)
{
	fclose(fmdata->in_file);
	fclose(fmdata->out_file);
	FREE(fmdata->out_filename);
}

void free_passworded_codec(codec_t* codec)
{
	if(codec->args)
	{
		FREE(((password_t*)codec->args)->buf);
		FREE(codec->args);
	}
}

void inv_encode_internal(const char* input_filename, void** args,
		e_file_op_t file_op)
{
	UNUSED(args);
	file_manip_data_t fmdata = get_file_namip_data(input_filename, file_op);
	if(fmdata.length == 0) // input file is empty
	{
		free_file_manip_data(&fmdata);
		return;
	}

	STACK_CACHE_ALLIGNED_BUF(uint8_t, chunk); //chunk is uint8_t*
	int residual_length = fmdata.length; //length yet to be encoded
	int chunk_size = residual_length < CACHE_LINE_SIZE ? 1 : CACHE_LINE_SIZE;
	while(residual_length != 0)
	{
		(void)fread((void*)(chunk), chunk_size, 1, fmdata.in_file);
		DIE_IF(ferror(fmdata.in_file) != 0, "Error reading input file");

		if(chunk_size == CACHE_LINE_SIZE)
		{
			//invert WORD_SIZE at a time.
			//Note: CACHE_LINE_SIZE is guaranteed to be a multiple of WORD_SIZE.
			int i = CACHE_LINE_SIZE/WORD_SIZE - 1;
			while(i >= 0)
			{
				((size_t*)chunk)[i]	= ~(((size_t*)chunk)[i]);
				i--;
			}
		}
		else //1 byte simple inversion
			*chunk = ~(*chunk);

		fwrite((void*)(chunk), chunk_size, 1, fmdata.out_file);
		DIE_IF(ferror(fmdata.out_file) != 0, "Error writing output file");
		residual_length -= chunk_size;
		if(residual_length < CACHE_LINE_SIZE)
			chunk_size = 1;
	}
	free_file_manip_data(&fmdata);
}

void inv_encode(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	UNUSED(args_is_filename);
	inv_encode_internal(input_filename, args, FO_ENCODE);
}
void inv_decode(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	UNUSED(args_is_filename);
	inv_encode_internal(input_filename, args, FO_DECODE);
}

void passwd_encode_internal(const char* input_filename,
		password_t* pass, e_file_op_t file_op)
{
	file_manip_data_t fmdata = get_file_namip_data(input_filename, file_op);
	if(fmdata.length == 0) // input file is empty
	{
		free_file_manip_data(&fmdata);
		return;
	}

	if(pass->length < pass->allocated_length)
	{
		/*Replicate the password content linearly until it reaches
		 * allocated_length
		 **/
		int residual_lenth = pass->allocated_length - pass->length;
		int i, j;
		uint8_t* residue = &pass->buf[pass->length];
		for(i=0, j=0; j<residual_lenth; j++)
			residue[j] = pass->buf[(i++)%pass->length];
	}
	size_t* password_bits = (size_t*)pass->buf;
	int64_t passwlength_in_word_count = pass->allocated_length/WORD_SIZE;

	STACK_CACHE_ALLIGNED_BUF(uint8_t, chunk); //chunk is uint8_t*
	int residual_length = fmdata.length;
	int chunk_size = residual_length < CACHE_LINE_SIZE ? 1 : CACHE_LINE_SIZE;
	int j = 0;
	while(residual_length != 0)
	{
		(void)fread((void*)(chunk), chunk_size, 1, fmdata.in_file);
		DIE_IF(ferror(fmdata.in_file) != 0, "Error reading input file");

		if(chunk_size == CACHE_LINE_SIZE)
		{
			//invert WORD_SIZE at a time.
			//Note: CACHE_LINE_SIZE is guaranteed to be a multiple of WORD_SIZE.
			int i = CACHE_LINE_SIZE/WORD_SIZE - 1;
			while(i >= 0)
			{
				(((size_t*)chunk)[i])	^=
					password_bits[(j++)%passwlength_in_word_count];
				i--;
			}
		}
		else //1 byte at a time
			*chunk ^= *(uint8_t*)password_bits;

		(void)fwrite((void*)(chunk), chunk_size, 1, fmdata.out_file);
		DIE_IF(ferror(fmdata.out_file) != 0, "Error writing output file");
		residual_length -= chunk_size;
		if(residual_length < CACHE_LINE_SIZE)
			chunk_size = 1;
	}
	free_file_manip_data(&fmdata);
}

void passwd_encode1_internal(const char* input_filename, void** args,
		BOOL args_is_filename, e_file_op_t file_op)
{
	DIE_IF(args == NULL || *args == NULL, "A password is required!");
	password_t* pass = args_is_filename ?
		read_password_from_file((const char*)(*args)) :
		make_password_from_cmdline((const char*)(*args));
	free(*args);
	*args = pass;
	passwd_encode_internal(input_filename, pass, file_op);
}
void passwd_encode1(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	passwd_encode1_internal(input_filename, args, args_is_filename, FO_ENCODE);
}
void passwd_decode1(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	passwd_encode1_internal(input_filename, args, args_is_filename, FO_DECODE);
}


#if defined(__linux__) && defined(SHASUM)
#include <unistd.h>
#define HIDDEN_PASSWORD_FILENAME ".codec_password"
password_t* make_password_digest_from_shell(const char* first_format_arg,
		const char* shell_command_format)
{
	const char* format = shell_command_format;
	int length;
	{
		char buf[3];
		length = snprintf(buf, 2, format, first_format_arg,
				HIDDEN_PASSWORD_FILENAME);
	}
	char* buf = (char*)malloc(++length);
	snprintf(buf, length, format, first_format_arg, HIDDEN_PASSWORD_FILENAME);
	int status = system(buf);
	free(buf);
	DIE_IF(status != 0, "Something went wrong when dealing with the passowrd");
	password_t* pass = read_password_from_file(HIDDEN_PASSWORD_FILENAME);
	status = unlink(HIDDEN_PASSWORD_FILENAME);
	DIE_IF(status != 0, "Something went wrong when dealing with the passowrd");
	return pass;
}
password_t* make_password_digest_from_password_file(const char* filename)
{
	return make_password_digest_from_shell(filename,
			"cat %s | shasum -a 512 | cut -d ' ' -f1 > %s");
}

password_t* make_password_digest_from_password_args(const char* password)
{
	return make_password_digest_from_shell(password,
			"echo \"%s\" | shasum -a 512 | cut -d ' ' -f1 > %s");
}

void passwd_encode2_internal(const char* input_filename, void** args,
		BOOL args_is_filename, e_file_op_t file_op)
{
	DIE_IF(args == NULL || *args == NULL, "A password is required!");
	password_t* pass = args_is_filename ?
		make_password_digest_from_password_file((const char*)(*args)) :
		make_password_digest_from_password_args((const char*)(*args));
	free(*args);
	*args = pass;
	passwd_encode_internal(input_filename, pass, file_op);
}
void passwd_encode2(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	passwd_encode2_internal(input_filename, args, args_is_filename, FO_ENCODE);
}
void passwd_decode2(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	passwd_encode2_internal(input_filename, args, args_is_filename, FO_DECODE);
}
#endif


void passwd_encode_stub(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	UNUSED(input_filename);
	DIE_IF(args == NULL || *args == NULL, "A password is required!");
	printf("\nThis is not implemented. Its stub exists just to show that one\n"
			"can have any number of passworded schemes.\n"
			"Left as exercice to whomever is willing to play with it!\n\n\n");

	/*Examples of algorithms abound! The sky is the limit ...
	 * 1- 
	 *		- Cut the password (or passphrase) in ~half. 
	 *		- Swap both halfs; and invert the second half
	 *		- XOR both halves it with the original data
	 * 2-
	 *		- Reverse the password (byte per byte)
	 *		- XOR it with the original data
	 * 3-
	 *		- Flip every other bit of the password
	 *		- XOR it with the original data
	 * 4-
	 *		- Do whatever reversible transformation to the password
	 *		- Reverse the whole data byte per byte or machine word per machine word
	 *		- If the password is at least double the word size (WS), take only the
	 *			portion that is 2 WS. Let's call that portion P1; and the rest	P2.
	 *			Extend P2 with itself until it is a multiple of 2 WS.
	 *			- Take P1 as destination and linearly XOR P2%(2 word sizes) with 
	 *				P1 until until P2 is depleted.
	 *				- Basically, while(more of P2){P1 ^= next_2_WS_WORTH_OF_P2;}
	 *			- XOR each word size of the data with half of P1.
	 *				- Alternate with each half of P1
	 *			- Revert (byte per byte) the bits of the resulting data
	 *			- Invert the thing <- final encoded data.
	 *		- Else
	 *			- XOR the password with the bit-inverted data
	 *	5-
	 *		etc.
	 *	
	 * */
}

void passwd_decode_stub(const char* input_filename, void** args,
		BOOL args_is_filename)
{
	UNUSED(input_filename);
	DIE_IF(args == NULL || *args == NULL, "A password is required!");
	printf("\nThis is not implemented. Its stub exists just to show that one\n"
			"can have any number of passworded schemes.\n"
			"Left as exercice to whomever is willing to play with it!\n\n\n");
}

void free_passwd_encode_stub_codec(codec_t* codec)
{
	FREE(codec->args);
}

void list_codecs(const codec_t codecs[], int num_codecs)
{
	int i;
	for(i=0; i<num_codecs; i++)
		printf("%d- %s\n\t%s\n\n", i + 1, codecs[i].name, codecs[i].description);
}

int main(int argc, char** argv)
{
	codec_t all_codecs[] = 
	{
		{inv_encode, inv_decode, nop, NULL, "inv", "bit-inverting codec"},
		{passwd_encode1, passwd_decode1, free_passworded_codec,
			NULL, "passphrased-1", "Codec with passphrase type 1"},
#if defined(__linux__) && defined(SHASUM)
		{passwd_encode2, passwd_decode2, free_passworded_codec,
			NULL, "passphrased-2", 
			"Codec with passphrase type 2 (more secure and available on Linux only)"},
#endif
		{passwd_encode_stub, passwd_decode_stub,
			free_passwd_encode_stub_codec,
			NULL, "passphrased-stub", "Codec with passphrase stub"}
	};
	const int num_codecs = sizeof(all_codecs)/sizeof(codec_t);

	option_t options = {UNDEFINED_OPTION_KIND, UNDEFINED_CODEC,
		NULL, NULL, FALSE};
	parse_cmd(argc, argv, all_codecs, num_codecs, &options);
	switch(options.kind)
	{
		case LIST:
			list_codecs(all_codecs, num_codecs); 
			break;
		case ENCODE:
			all_codecs[options.codec_number].encode(options.input_filename,
					&all_codecs[options.codec_number].args, options.args_is_filename);
			break;
		case DECODE:
			all_codecs[options.codec_number].decode(options.input_filename,
					&all_codecs[options.codec_number].args, options.args_is_filename);
			break;
		default:
			DIE("Unsupported option kind. It's a bug if this happens -_-");
	}

	//Clean things up!
	int i;
	for(i=0; i<num_codecs; i++)
		all_codecs[i].cleanup(&(all_codecs[i]));
	if(options.input_filename != NULL)
		FREE(options.input_filename);

	return EXIT_SUCCESS;
}
