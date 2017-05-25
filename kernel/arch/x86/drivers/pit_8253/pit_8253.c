// kernel/arch/x86/drivers/pit_8253/pit_8253.c

#include <stdint.h>

#include <asm/io.h>
#include <dev/dev_types.h>
#include <mm/page.h>
#include <time/time.h>
#include <pic_8259/pic_8259.h>
#include <pit_8253/pit_8253.h>

// PIT channel 8-bit IO ports.
#define PIT_8253_CH0_PORT 0x40
#define PIT_8253_CH1_PORT 0x41
#define PIT_8253_CH2_PORT 0x42

// PIT command/mode set 8-bit IO port.
#define PIT_8253_MODE_PORT 0x43

// PIT numeric operation mode 16-bit vs 4-decimal.
typedef enum pit_8253_numeric_enum {
	pit_8253_bin = 0b0,
	pit_8253_bcd = 0b1
} pit_8253_numeric_t;

typedef enum pit_8253_operation_enum {
// Interupt on Terminal Count
	pit_8253_int_term_cnt   = 0b000,
// Hardware Re-triggerable One-shot
	pit_8253_hw_one_shot    = 0b001,
// Rate Generator
	pit_8253_rate_gen       = 0b010,
// Square Wave Generator
	pit_8253_square_gen     = 0b011,
// Software Triggered Strobe
	pit_8253_sw_strobe      = 0b100,
// Hardware Triggered Strobe
	pit_8253_hw_strobe      = 0b101,
// Duplicate of Rate Generator
	pit_8253_rate_gen_dup   = 0b110,
// Duplicate of Square Wave Generator
	pit_8253_square_gen_dup = 0b111
} pit_8253_operation_t;

typedef enum pit_8253_access_enum {
// PIT Latch Count Value Command
	pit_8253_latch_cnt = 0b00,
// PIT Low-byte only
	pit_8253_lobyte    = 0b01,
// PIT High-byte only
	pit_8253_hibyte    = 0b10,
// PIT High and Low bytes
	pit_8253_hilobyte  = 0b11
} pit_8253_access_t;

typedef enum pit_8253_channel_enum {
	pit_8253_ch0 = 0b00,
	pit_8253_ch1 = 0b01,
	pit_8253_ch2 = 0b10
// The PIT 8254 has a read back mode here, it will not be supported.
} pit_8253_channel_t;

typedef struct __attribute__((packed)) pit_8253_mode_struct {
	pit_8253_numeric_t   numeric   : 1;
	pit_8253_operation_t operation : 3;
	pit_8253_access_t    access    : 2;
	pit_8253_channel_t   channel   : 2;
} pit_8253_mode_t;

static page_t *pit_8253_next_page_mapping (dev_driver_t *, page_t *, bool *);

static dev_driver_t pit_8253_driver = {
	.pimpl = NULL,
	.dev = {
		.maj = dev_maj_arch,
		.min = dev_min_arch_pit_8253,
	},
	.next_page_mapping = pit_8253_next_page_mapping
};

#pragma GCC diagnostic ignored "-Wunused-parameter"
static page_t *pit_8253_next_page_mapping (
		dev_driver_t *this,
		page_t *last_page,
		bool *need_write
) {
	return NULL;
}
#pragma GCC diagnostic pop

static char pit_8253_divider_low (pit_8253_divider_t divider) {
	return 0xff & divider;
}

static char pit_8253_divider_high (pit_8253_divider_t divider) {
	return 0xff & divider >> 8;
}

// The best choice for divider to for an interval.
static pit_8253_divider_t pit_8253_divider (time_t t) {
	time_nanos_t tn = time_nanos (t);

	if (tn <= time_nanos (PIT_8253_INTERVAL_MIN))
		return PIT_8253_DIVIDER_MIN;
	if (tn >= time_nanos (PIT_8253_INTERVAL_MAX))
		return PIT_8253_DIVIDER_MAX;

	return (pit_8253_divider_t)(tn * PIT_8253_BASE_MEGAHZ / 1000);
}

void pit_8253_set_interval (time_t t) {
	const pit_8253_mode_t mode = {
		.numeric   = pit_8253_bin,
		.operation = pit_8253_square_gen,
		.access    = pit_8253_hilobyte,
		.channel   = pit_8253_ch0
	};

	pit_8253_divider_t divider = pit_8253_divider (t);

	const bool was_masked = pic_8259_is_masked (0);
	pic_8259_mask (0);

	outb (*(char *)&mode, PIT_8253_MODE_PORT);

	outb (pit_8253_divider_low  (divider), PIT_8253_CH0_PORT);
	outb (pit_8253_divider_high (divider), PIT_8253_CH0_PORT);

	if (!was_masked)
		pic_8259_unmask (0);
}

dev_driver_t *pit_8253_get_device_driver () {
	return &pit_8253_driver;
}


// vim: set ts=4 sw=4 noet syn=c:
