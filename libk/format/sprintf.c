// libk/format/sprintf.c

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include <attributes.h>
#include <string.h>
#include <format.h>

// Very minimal sprintf functions.

// These routines account for a very large quantity of code in this kernel, thus we've
// added SMALL

typedef enum printf_type_enum {
	PRINTF_TYPE_PERCENT,  // '%'
	PRINTF_TYPE_D_SINT,   // 'i', 'd'
	PRINTF_TYPE_D_UINT,   // 'u'
	PRINTF_TYPE_LH_UINT,  // 'x'
	PRINTF_TYPE_STR,      // 's'
	PRINTF_TYPE_CHAR,     // 'c'
	PRINTF_TYPE_PTR       // 'p'
} printf_type_t;

typedef enum printf_length_enum {
	PRINTF_LEN_DEFAULT,    // ""
	PRINTF_LEN_CHAR,       // "hh"
	PRINTF_LEN_SHORT,      // "h"
	PRINTF_LEN_LONG,       // "l"
	PRINTF_LEN_LONG_LONG,  // "ll"
	PRINTF_LEN_SIZE,       // "z"
	PRINTF_LEN_PTRDIFF     // "t"
} printf_length_t;

typedef struct PACKED printf_flags_struct {
	uint8_t left_align     : 1;  // '-'
	uint8_t sign_positive  : 1;  // '+'
	uint8_t space_positive : 1;  // ' '
	uint8_t pad_zero       : 1;  // '0'
	uint8_t _spaceing      : 4;
} printf_flags_t;

typedef struct printf_placeholder_struct {
	printf_flags_t flags;
	printf_length_t length;
	unsigned int width;
	printf_type_t type;
} printf_placeholder_t;

static printf_placeholder_t printf_placeholder_prototype = {
	.flags = {
		.left_align = 0b0,
		.sign_positive = 0b0,
		.space_positive = 0b0,
		.pad_zero = 0b0
	},
	.length = PRINTF_LEN_DEFAULT,
	.width = 0,
};

// Return number of characters in number until invalid character.
SMALL
static inline size_t printf_get_num (const char *str, unsigned int *num_ptr) {
	const char *cur = str;

	do switch (*cur) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			*num_ptr *= 10;

			switch (*cur) {
				case '1':
					*num_ptr += 1;
					break;
				case '2':
					*num_ptr += 2;
					break;
				case '3':
					*num_ptr += 3;
					break;
				case '4':
					*num_ptr += 4;
					break;
				case '5':
					*num_ptr += 5;
					break;
				case '6':
					*num_ptr += 6;
					break;
				case '7':
					*num_ptr += 7;
					break;
				case '8':
					*num_ptr += 8;
					break;
				case '9':
					*num_ptr += 9;
					break;
			}
			break;
		default:
			goto PRINTF_GET_NUM_RET;
	} while (*++cur);

PRINTF_GET_NUM_RET:
	return cur - str;
}

static inline size_t add_sign (
		char *dest,
		const printf_placeholder_t *placeholder
) {
	if (placeholder->flags.sign_positive)
		*dest = '+';
	else if (placeholder->flags.space_positive)
		*dest = ' ';
	else
		return 0;

	return 1;
}

static inline size_t maybe_sign (
		char *dest,
		const printf_placeholder_t *placeholder,
		intmax_t v
) {
	if (0 > v)
		return 0;

	return add_sign (dest, placeholder);
}

// Returns length of placeholder, including the first '%'.
// Returns zero if placeholder is invalid.
SMALL
static inline size_t parse_placeholder (
		const char *format,  // Points to first '%' character.
		printf_placeholder_t *placeholder_ptr
) {
	const char *cur = format;

	cur += 1;

	// A terminating null byte will actually travel all the way down to the type logic,
	// where it will fail as an invalid type.

	if ('%' == *cur) {
		placeholder_ptr->type = PRINTF_TYPE_PERCENT;

		return 2;
	}

	// Flags loop
	do switch (*cur) {
		case '-':
			placeholder_ptr->flags.left_align = 0b1;
			break;
		case '+':
			placeholder_ptr->flags.sign_positive = 0b1;
			break;
		case ' ':
			placeholder_ptr->flags.space_positive = 0b1;
			break;
		case '0':
			placeholder_ptr->flags.pad_zero = 0b1;
			break;
		default:
			goto PARSE_PLACEHOLDER_WIDTH;
	} while (*++cur);


PARSE_PLACEHOLDER_WIDTH:
	cur += printf_get_num (cur, &placeholder_ptr->width);

	// Length
	switch (*cur) {
		case 'h':
		case 'l':
		case 'z':
		case 'j':
		case 't':
			cur += 1;

			switch (*cur) {
				case 'h':
					if ('h' == *cur) {
						cur += 1;
						placeholder_ptr->length = PRINTF_LEN_CHAR;
					} else {
						placeholder_ptr->length = PRINTF_LEN_SHORT;
					}
					break;
				case 'l':
					if ('l' == *cur) {
						cur += 1;
						placeholder_ptr->length = PRINTF_LEN_LONG_LONG;
					} else {
						placeholder_ptr->length = PRINTF_LEN_LONG;
					}
					break;
				case 'z':
					placeholder_ptr->length = PRINTF_LEN_SIZE;
					break;
				case 't':
					placeholder_ptr->length = PRINTF_LEN_PTRDIFF;
					break;
			}
			break;
	}

	// Type
	switch (*cur) {
		case 'd':
		case 'i':
			placeholder_ptr->type = PRINTF_TYPE_D_SINT;
			break;
		case 'u':
			placeholder_ptr->type = PRINTF_TYPE_D_UINT;
			break;
		case 'x':
			placeholder_ptr->type = PRINTF_TYPE_LH_UINT;
			break;
		case 's':
			placeholder_ptr->type = PRINTF_TYPE_STR;
			break;
		case 'c':
			placeholder_ptr->type = PRINTF_TYPE_CHAR;
			break;
		case 'p':
			placeholder_ptr->type = PRINTF_TYPE_PTR;
			break;
		default:
			// Invalid type.
			return 0;
	}

	cur += 1;

	return cur - format;
}

SMALL
int vsprintf (char *str, const char *format, va_list ap) {
	char *cur = str;

	while (*format) {
		if ('%' != *format) {
			*cur++ = *format++;
			continue;
		}

		size_t placeholder_len;

		printf_placeholder_t placeholder = printf_placeholder_prototype;

		placeholder_len = parse_placeholder (format, &placeholder);

		if (0 == placeholder_len) {
			*cur++ = *format++;
			continue;
		}

		format += placeholder_len;

		int int_base = 0;
		bool int_signed = false;

		int as_int;
		long as_long;
		long long as_llong;
		size_t as_size;
		ptrdiff_t as_ptrdiff;

		switch (placeholder.type) {
			case PRINTF_TYPE_PERCENT:
				*cur++ = '%';
				break;
			case PRINTF_TYPE_D_SINT:
			case PRINTF_TYPE_D_UINT:
			case PRINTF_TYPE_LH_UINT:
#pragma GCC diagnostic ignored "-Wswitch"
				switch (placeholder.type) {
					case PRINTF_TYPE_D_SINT:
					case PRINTF_TYPE_D_UINT:
						int_base = 10;
						if (PRINTF_TYPE_D_SINT == placeholder.type)
							int_signed = true;
						else
							int_signed = false;
						break;
					case PRINTF_TYPE_LH_UINT:
						int_base = 16;
						int_signed = false;
				}
#pragma GCC diagnostic pop

				if (int_signed) {
					switch (placeholder.length) {
						case PRINTF_LEN_CHAR:
						case PRINTF_LEN_SHORT:
						case PRINTF_LEN_DEFAULT:
							as_int = va_arg(ap, int);
							cur += maybe_sign (cur, &placeholder, as_int);
							itoa (as_int, cur, int_base);
							break;
						case PRINTF_LEN_LONG:
							as_long = va_arg(ap, long);
							cur += maybe_sign (cur, &placeholder, as_long);
							ltoa (as_long, cur, int_base);
							break;
						case PRINTF_LEN_LONG_LONG:
							as_llong = va_arg(ap, long);
							cur += maybe_sign (cur, &placeholder, as_llong);
							lltoa (as_llong, cur, int_base);
							break;
						// Assume the worst of these last few.
						case PRINTF_LEN_SIZE:
							as_size = va_arg(ap, size_t);
							cur += maybe_sign (cur, &placeholder, as_size);
							lltoa (as_size, cur, int_base);
							break;
						case PRINTF_LEN_PTRDIFF:
							as_ptrdiff = va_arg(ap, ptrdiff_t);
							cur += maybe_sign (cur, &placeholder, as_ptrdiff);
							lltoa (as_ptrdiff, cur, int_base);
							break;
					}

					if ('-' == *cur)
						cur += 1;
				} else {
					cur += add_sign (cur, &placeholder);

					switch (placeholder.length) {
						case PRINTF_LEN_CHAR:
						case PRINTF_LEN_SHORT:
						case PRINTF_LEN_DEFAULT:
							uitoa (va_arg(ap, unsigned int), cur, int_base);
							break;
						case PRINTF_LEN_LONG:
							ultoa (va_arg(ap, unsigned long), cur, int_base);
							break;
						case PRINTF_LEN_LONG_LONG:
							ulltoa (va_arg(ap, unsigned long long), cur, int_base);
							break;
						// Assume the worst of these last few.
						case PRINTF_LEN_SIZE:
							ulltoa (va_arg(ap, size_t), cur, int_base);
							break;
						case PRINTF_LEN_PTRDIFF:
							ulltoa (va_arg(ap, ptrdiff_t), cur, int_base);
							break;
					}
				}

				if (placeholder.width) {
					char padc;

					if (placeholder.flags.pad_zero)
						padc = '0';
					else
						padc = ' ';

					if (placeholder.flags.left_align)
						strpadr (cur, padc, placeholder.width);
					else
						strpadl (cur, padc, placeholder.width);
				}

				cur += strlen (cur);
				break;
			case PRINTF_TYPE_STR:
				strcpy (cur, va_arg(ap, char *));
				cur += strlen (cur);
				break;
			case PRINTF_TYPE_CHAR:
				*cur++ = (char)va_arg(ap, int);
				*cur   = '\0';
				break;
			case PRINTF_TYPE_PTR:
				strcpy (cur, "*0x");
				cur += 3;
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
				ulltoa ((unsigned long long)va_arg(ap, void *), cur, 16);
#pragma GCC diagnostic pop
				strpadl (cur, '0', 2 * sizeof(void *));
				cur += 2 * sizeof(void *);
				break;
		}
	}

	if (*cur)
		*cur = '\0';

	return cur - str;
}

int sprintf (char *str, const char *format, ...) {
	va_list ap;

	va_start(ap, format);

	int ret = vsprintf (str, format, ap);

	va_end(ap);

	return ret;
}

// vim: set ts=4 sw=4 noet syn=c:
