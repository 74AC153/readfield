#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

typedef enum {
	FIELD_U8, FIELD_S8,
	FIELD_U16, FIELD_S16,
	FIELD_U32, FIELD_S32,
	FIELD_U64, FIELD_S64,
	FIELD_STR,
	FIELD_FLT,
	FIELD_DBL,
	FIELD_LDBL
} field_t;

typedef enum {
	END_NATIVE,
	END_BIG,
	END_LITTLE
} endian_t;

typedef enum {
	DISP_HEX,
	DISP_DEC,
	DISP_ASCII
} disp_t;

void do_help(void)
{
	printf("All args are evaluated in the order specified\n");
	printf("usage:\n");
	printf("-h               this message\n");
	printf("-i <infile>      load specified input file\n");
	printf("-f <field type>  u8 s8 u16 s16 u32 s32 u64 s64 str flt dbl ldbl\n");
	printf("-e <endian>      b(ig) l(ittle) n(ative)\n");
	printf("-d <disp fmt>    h(ex) d(ec) a(scii)\n");
	printf("-o [+]<offset>   offset, '+' for relative to last specified\n");
	printf("-l <label>       field label\n");
	printf("-p <repeat>      print <repeat> times sequentially\n");
}

void print_char(int c)
{
	if(c >= 0x20 && c < 0x7F) {
		if(c == 0x5C) { printf("\\"); }
		else { printf("%c", c); }
	}
	else {
		if(c == 0) { printf("\\0"); }
		else if(c == 0x7) { printf("\\a"); }
		else if(c == 0x8) { printf("\\b"); }
		else if(c == 0x9) { printf("\\t"); }
		else if(c == 0xA) { printf("\\n"); }
		else if(c == 0xB) { printf("\\v"); }
		else if(c == 0xC) { printf("\\f"); }
		else if(c == 0xD) { printf("\\r"); }
		else { printf("\\0%o", c); }
	}
}

void print_ascii_str_direct(unsigned char *s)
{
	while(*s) {
		print_char(*s);
		s++;
	}
	print_char(*s);
}

void reverse_bytes(void *buf, size_t len)
{
	char *start, *end, temp;
	start = buf;
	end = buf + (len - 1);
	while(start < end) {
		temp = *start;
		*start = *end;
		*end = temp;
		start++;
		end--;
	} 
}

void conv_from_bigend(void *buf, size_t len)
{
	unsigned int test = 0xFF111111;
	if(((uint8_t *)&test)[0] != 0xFF) {
		reverse_bytes(buf, len);
	}
}

void conv_from_littleend(void *buf, size_t len)
{
	unsigned int test = 0x111111FF;
	if(((uint8_t *)&test)[0] != 0xFF) {
		reverse_bytes(buf, len);
	}
}

void conv_from_endian(endian_t end, void *buf, size_t len)
{
	if(end == END_BIG) {
		conv_from_bigend(buf, len);
	} else if(end == END_LITTLE) {
		conv_from_littleend(buf, len);
	}
}

size_t do_print(
	void *base,
	uint64_t offset,
	field_t field_type,
	endian_t endian_mode,
	disp_t disp_fmt,
	char *label)
{
	size_t status = 0;
	union {
		uint8_t ui8;
		int8_t i8;
		uint16_t ui16;
		int16_t i16;
		uint32_t ui32;
		int32_t i32;
		uint64_t ui64;
		int64_t i64;
		float f;
		double d;
		long double ld;
	} value;

	printf("0x%16.16" PRIx64 ": ", offset);
	switch(field_type) {
	case FIELD_U8:
		memcpy(&value.ui8, base + offset, sizeof(value.ui8));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx8, value.ui8);
		else if(disp_fmt == DISP_DEC) printf("%" PRIu8, value.ui8);
		else if(disp_fmt == DISP_ASCII) print_char(value.ui8);
		status = sizeof(value.ui8);
		break;

	case FIELD_S8:
		memcpy(&value.i8, base + offset, sizeof(value.i8));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx8, value.i8);
		else if(disp_fmt == DISP_DEC) printf("%" PRId8, value.i8);
		else if(disp_fmt == DISP_ASCII) print_char(value.i8);
		status = sizeof(value.i8);
		break;

	case FIELD_U16:
		memcpy(&value.ui16, base + offset, sizeof(value.ui16));
		conv_from_endian(endian_mode, &value.ui16, sizeof(value.ui16));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx16, value.ui16);
		else if(disp_fmt == DISP_DEC) printf("%" PRIu16, value.ui16);
		else if(disp_fmt == DISP_ASCII) print_char(value.ui16);
		status = sizeof(value.ui16);
		break;

	case FIELD_S16:
		memcpy(&value.i16, base + offset, sizeof(value.i16));
		conv_from_endian(endian_mode, &value.i16, sizeof(value.i16));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx16, value.i16);
		else if(disp_fmt == DISP_DEC) printf("%" PRId16, value.i16);
		else if(disp_fmt == DISP_ASCII) print_char(value.i16);
		status = sizeof(value.i16);
		break;

	case FIELD_U32:
		memcpy(&value.ui32, base + offset, sizeof(value.ui32));
		conv_from_endian(endian_mode, &value.ui32, sizeof(value.ui32));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx32, value.ui32);
		else if(disp_fmt == DISP_DEC) printf("%" PRIu32, value.ui32);
		else if(disp_fmt == DISP_ASCII) print_char(value.ui32);
		status = sizeof(value.ui32);
		break;

	case FIELD_S32:
		memcpy(&value.i32, base + offset, sizeof(value.i32));
		conv_from_endian(endian_mode, &value.i32, sizeof(value.i32));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx32, value.i32);
		else if(disp_fmt == DISP_DEC) printf("%" PRId32, value.i32);
		else if(disp_fmt == DISP_ASCII) print_char(value.i32);
		status = sizeof(value.i32);
		break;

	case FIELD_U64:
		memcpy(&value.ui64, base + offset, sizeof(value.ui64));
		conv_from_endian(endian_mode, &value.ui64, sizeof(value.ui64));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx64, value.ui64);
		else if(disp_fmt == DISP_DEC) printf("%" PRIu64, value.ui64);
		else if(disp_fmt == DISP_ASCII) print_char(value.ui64);
		status = sizeof(value.ui64);
		break;

	case FIELD_S64:
		memcpy(&value.i64, base + offset, sizeof(value.i64));
		conv_from_endian(endian_mode, &value.i64, sizeof(value.i64));
		if(disp_fmt == DISP_HEX) printf("0x%" PRIx64, value.i64);
		else if(disp_fmt == DISP_DEC) printf("%" PRId64, value.i64);
		else if(disp_fmt == DISP_ASCII) print_char(value.i64);
		status = sizeof(value.i64);
		break;

	case FIELD_STR:
		print_ascii_str_direct(base + offset);
		status = strlen(base) + 1;
		break;

	case FIELD_FLT:
		memcpy(&value.f, base + offset, sizeof(value.f));
		conv_from_endian(endian_mode, &value.f, sizeof(value.f));
		if(disp_fmt == DISP_HEX) printf("0x%a", value.f);
		else if(disp_fmt == DISP_DEC) printf("%e", value.f);
		else if(disp_fmt == DISP_ASCII) printf("ascii?");
		status = sizeof(value.f);
		break;

	case FIELD_DBL:
		memcpy(&value.d, base + offset, sizeof(value.d));
		conv_from_endian(endian_mode, &value.d, sizeof(value.d));
		if(disp_fmt == DISP_HEX) printf("0x%a", value.d);
		else if(disp_fmt == DISP_DEC) printf("%e", value.d);
		else if(disp_fmt == DISP_ASCII) printf("ascii?");
		status = sizeof(value.d);
		break;

	case FIELD_LDBL:
		memcpy(&value.ld, base + offset, sizeof(value.ld));
		conv_from_endian(endian_mode, &value.ld, sizeof(value.ld));
		if(disp_fmt == DISP_HEX) printf("0x%La", value.ld);
		else if(disp_fmt == DISP_DEC) printf("%Le", value.ld);
		else if(disp_fmt == DISP_ASCII) printf("ascii?");
		status = sizeof(value.ld);
		break;

	default:
		break;
	}

	if(label) printf("  (%s)", label);
	printf("\n");
	return status;
}

int main(int argc, char *argv[])
{
	int opt;
	int status = 0;
	
	int infd = -1; uint64_t infile_len; void *infile;
	field_t field_type = FIELD_U8;
	endian_t endian_mode = END_NATIVE;
	disp_t disp_fmt = DISP_HEX;
	uint64_t offset = 0;
	char *label = NULL;
	
	while((opt = getopt(argc, argv, "hi:f:e:d:o:l:p:")) != -1) {
		switch(opt) {
		case 'h':
			do_help();
			status = -1;
			goto finish;

		case 'i': {
			struct stat sb;
			if(infd >= 0) {
				if((status = munmap(infile, infile_len))) {
					perror("munmap()");
					goto finish;
				}
			}
			if(0 > (infd = open(optarg, O_RDONLY))) {
				perror("open()");
				status = infd;
				goto finish;
			}
			if((status = fstat(infd, &sb))) {
				perror("fstat()");
				goto finish;
			}
			infile_len = sb.st_size;
			infile = mmap(NULL, infile_len, PROT_READ, MAP_PRIVATE, infd, 0);
			if(infile == MAP_FAILED) {
				perror("mmap()");
				status = -1;
				goto finish;
			}
			printf("file %s:\n", optarg);
			break;
		}
		case 'f':
			if(strcasecmp(optarg, "U8") == 0) field_type = FIELD_U8;
			else if(strcasecmp(optarg, "S8") == 0) field_type = FIELD_S8;
			else if(strcasecmp(optarg, "U16") == 0) field_type = FIELD_U16;
			else if(strcasecmp(optarg, "S16") == 0) field_type = FIELD_S16;
			else if(strcasecmp(optarg, "U32") == 0) field_type = FIELD_U32;
			else if(strcasecmp(optarg, "S32") == 0) field_type = FIELD_S32;
			else if(strcasecmp(optarg, "U64") == 0) field_type = FIELD_U64;
			else if(strcasecmp(optarg, "S64") == 0) field_type = FIELD_S64;
			else if(strcasecmp(optarg, "STR") == 0) field_type = FIELD_STR;
			else if(strcasecmp(optarg, "FLT") == 0) field_type = FIELD_FLT;
			else if(strcasecmp(optarg, "DBL") == 0) field_type = FIELD_DBL;
			else if(strcasecmp(optarg, "LDBL") == 0) field_type = FIELD_LDBL;
			else {
				printf("bad field type specifier: %s\n", optarg);
				goto finish;
			}
			break;
				
		case 'e':
			if(strncasecmp(optarg, "N", 1) == 0) endian_mode = END_NATIVE;
			else if(strncasecmp(optarg, "L", 1) == 0) endian_mode = END_LITTLE;
			else if(strncasecmp(optarg, "B", 1) == 0) endian_mode = END_BIG;
			else {
				printf("bad endian specifier: %s\n", optarg);
				goto finish;
			}
			break;

		case 'd':
			if(strcasecmp(optarg, "H"/*hex*/) == 0) disp_fmt = DISP_HEX;
			else if(strcasecmp(optarg, "D"/*dec*/) == 0) disp_fmt = DISP_DEC;
			else if(strcasecmp(optarg, "A"/*ascii*/) == 0) disp_fmt = DISP_ASCII;
			else {
				printf("bad display specifier: %s\n", optarg);
				goto finish;
			}
			break;

		case 'o': {
			bool relative = false;
			ssize_t val;
			errno = 0;
			if(optarg[0] == '+')
				relative = true;

			errno = 0;
			if(relative)
				val = strtoll(optarg + 1, NULL, 0);
			else
				val = strtoll(optarg, NULL, 0);
			if(errno == ERANGE) {
				printf("bad offset specifier: %s\n", optarg);
				goto finish;
			}

			if(relative)
				offset += val;
			else 
				offset = val;

			if(offset > infile_len) {
				printf("offset exceeds infile bounds: %" PRIu64 "\n", offset);
				goto finish;
			}
			break;
		}

		case 'l':
			label = optarg;
			break;

		case 'p': {
			size_t repeat = 1, n = 0;
			errno = 0;
			repeat = strtoll(optarg, NULL, 0);
			if(repeat == 0 || errno == ERANGE) {
				printf("bad repeat specifier: %s\n", optarg);
				goto finish;
			}

			while(repeat--) {
				offset += n;
				n = do_print(infile, offset, field_type, endian_mode,
				             disp_fmt, label);
			}
			break;
		}

		default:
			break;
		}
	}

finish:
	return status;
}
