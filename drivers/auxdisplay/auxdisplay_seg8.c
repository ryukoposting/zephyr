#define DT_DRV_COMPAT epg_seg8

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#define SEG_MAP_BASE 0x1f
#define SEG_MAP(bits_, char_) [char_ - SEG_MAP_BASE] = (bits_)

LOG_MODULE_REGISTER(auxdisplay_seg8, CONFIG_AUXDISPLAY_LOG_LEVEL);

struct auxdisplay_seg8_data {
	char *digit_buf;
	int16_t cursor_x, cursor_y;
	uint8_t direction;
};

struct auxdisplay_seg8_config {
	struct auxdisplay_capabilities capabilities;
	struct gpio_dt_spec segment_gpios[8];
	struct gpio_dt_spec const *digit_select_gpios;
	int num_segment_gpios;
	int num_digit_select_gpios;
};

static bool seg8_light_seg(int seg, int chr)
{
	static uint8_t const map[] = {
    	/*      0bABCDEFG */
		SEG_MAP(0b0000001, '-'),
		SEG_MAP(0b0100101, '/'),
		SEG_MAP(0b1111110, '0'),
		SEG_MAP(0b0110000, '1'),
		SEG_MAP(0b1101101, '2'),
		SEG_MAP(0b1111001, '3'),
		SEG_MAP(0b0110011, '4'),
		SEG_MAP(0b1011011, '5'),
		SEG_MAP(0b1011111, '6'),
		SEG_MAP(0b1110000, '7'),
		SEG_MAP(0b1111111, '8'),
		SEG_MAP(0b1111011, '9'),
		SEG_MAP(0b1100101, '?'),
		SEG_MAP(0b1111101, '@'),
		SEG_MAP(0b1110111, 'A'),
		SEG_MAP(0b0011111, 'B'),
		SEG_MAP(0b0001101, 'C'),
		SEG_MAP(0b0111101, 'D'),
		SEG_MAP(0b1001111, 'E'),
		SEG_MAP(0b1000111, 'F'),
		SEG_MAP(0b1011110, 'G'),
		SEG_MAP(0b0110111, 'H'),
		SEG_MAP(0b0000110, 'I'),
		SEG_MAP(0b0111000, 'J'),
		SEG_MAP(0b1010111, 'K'),
		SEG_MAP(0b0001110, 'L'),
		SEG_MAP(0b1010100, 'M'),
		SEG_MAP(0b0010101, 'N'),
		SEG_MAP(0b0011101, 'O'),
		SEG_MAP(0b1100111, 'P'),
		SEG_MAP(0b1110011, 'Q'),
		SEG_MAP(0b1100110, 'R'),
		SEG_MAP(0b1011011, 'S'),
		SEG_MAP(0b0001111, 'T'),
		SEG_MAP(0b0111110, 'U'),
		SEG_MAP(0b0011100, 'V'),
		SEG_MAP(0b0101010, 'W'),
		SEG_MAP(0b0110111, 'X'),
		SEG_MAP(0b0111011, 'Y'),
		SEG_MAP(0b1101101, 'Z'),
		SEG_MAP(0b1001110, '['),
		SEG_MAP(0b0010011, '\\'),
		SEG_MAP(0b1111000, ']'),
		SEG_MAP(0b1100000, '^'),
		SEG_MAP(0b0001000, '_'),
	};

	/* map all lowercase letters to their uppercase counterparts
	 * map '{' and '}' to '[' and ']'
	 * map '~' to '^' (this one is a bit unfortunate, but necessary to
	 *     reduce the size of the segment mapping table by 29 bytes)
	 */
	if (chr >= 'a' && chr <= '~') {
        chr = chr - 'a' + 'A';
    }

	chr -= SEG_MAP_BASE;

	return chr > 0 && chr < sizeof(map) && (map[chr] & (1 << (6 - seg)));
}

static int seg8_set_segments(const struct auxdisplay_seg8_config *config,
	char c)
{
	int err;

	err = gpio_pin_set_dt(&config->segment_gpios[0], seg8_light_seg(0, c));
	if (err)
		return err;

	err = gpio_pin_set_dt(&config->segment_gpios[1], seg8_light_seg(1, c));
	if (err)
		return err;

	err = gpio_pin_set_dt(&config->segment_gpios[2], seg8_light_seg(2, c));
	if (err)
		return err;

	err = gpio_pin_set_dt(&config->segment_gpios[3], seg8_light_seg(3, c));
	if (err)
		return err;

	err = gpio_pin_set_dt(&config->segment_gpios[4], seg8_light_seg(4, c));
	if (err)
		return err;

	err = gpio_pin_set_dt(&config->segment_gpios[5], seg8_light_seg(5, c));
	if (err)
		return err;

	err = gpio_pin_set_dt(&config->segment_gpios[6], seg8_light_seg(6, c));
	return err;
}

static int seg8_refresh(const struct auxdisplay_seg8_config *config,
	struct auxdisplay_seg8_data *data)
{
	int err = 0;

	for (int i = 0; !err && i < config->num_digit_select_gpios; ++i) {
		if (data->digit_buf[i]) {
			seg8_set_segments(config, data->digit_buf[i]);
		}
		data->digit_buf[i] = 0;
		gpio_pin_set_dt(&config->digit_select_gpios[i], 1);
		k_msleep(1);
		gpio_pin_set_dt(&config->digit_select_gpios[i], 0);
	}


	return 0;
}

static int auxdisplay_seg8_init(const struct device *dev)
{
	const struct auxdisplay_seg8_config *config = dev->config;
	struct auxdisplay_seg8_data *data = dev->data;

	int const num_digits_expected =
		config->capabilities.rows * config->capabilities.columns;

	if (num_digits_expected != config->num_digit_select_gpios) {
		LOG_ERR("Incorrect number of digit-select pins (expected %d)",
			num_digits_expected);
		return -EINVAL;
	}

	if (7 != config->num_segment_gpios && 8 != config->num_segment_gpios) {
		LOG_ERR("Incorrect number of segment pins (expected 7 or 8)");
		return -EINVAL;
	}

	int err = 0;

	for (int i = 0; !err && i < config->num_digit_select_gpios; ++i) {
		err = gpio_pin_configure_dt(&config->digit_select_gpios[i],
			GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
	}

	for (int i = 0; !err && i < config->num_segment_gpios; ++i) {
		err = gpio_pin_configure_dt(&config->segment_gpios[i],
			GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
	}

	if (err) {
		LOG_ERR("gpio init failed: %d", err);
		return err;
	}

	memset(data->digit_buf, ' ', config->num_digit_select_gpios);
	return seg8_refresh(config, data);
}

static int auxdisplay_seg8_capabilities_get(const struct device *dev,
	struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_seg8_config *config = dev->config;
	memcpy(capabilities, &config->capabilities, sizeof(*capabilities));
	return 0;
}

static int auxdisplay_seg8_clear(const struct device *dev)
{
	const struct auxdisplay_seg8_config *config = dev->config;
	struct auxdisplay_seg8_data *data = dev->data;
	memset(data->digit_buf, ' ', config->num_digit_select_gpios);
	data->cursor_x = 0;
	data->cursor_y = 0;
	return seg8_refresh(config, data);
}

static int auxdisplay_seg8_display_on(const struct device *dev)
{
	return -ENOTSUP;
}

static int auxdisplay_seg8_display_off(const struct device *dev)
{
	return -ENOTSUP;
}

static int auxdisplay_seg8_cursor_set_enabled(const struct device *dev,
	bool enabled)
{
	return -ENOTSUP;
}

static int auxdisplay_seg8_position_blinking_set_enabled(const struct device *dev,
	bool enabled)
{
	return -ENOTSUP;
}

static int auxdisplay_seg8_cursor_shift_set(const struct device *dev,
	uint8_t direction, bool display_shift)
{
	if (display_shift) {
		return -ENOTSUP;
	}

	struct auxdisplay_seg8_data *data = dev->data;
	data->direction = direction;

	return -ENOTSUP;
}

static int auxdisplay_seg8_cursor_position_set(const struct device *dev,
	enum auxdisplay_position type, int16_t x, int16_t y)
{
	const struct auxdisplay_seg8_config *config = dev->config;
	struct auxdisplay_seg8_data *data = dev->data;
	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		if (data->direction == AUXDISPLAY_DIRECTION_RIGHT) {
			x += (int16_t)data->cursor_x;
			y += (int16_t)data->cursor_y;
		} else {
			x -= (int16_t)data->cursor_x;
			y -= (int16_t)data->cursor_y;
		}
	}

	if (y >= config->capabilities.rows || y < 0) {
		return -EINVAL;
	} else if (x >= config->capabilities.columns || x < 0) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;

	return 0;
}

static int auxdisplay_seg8_cursor_position_get(const struct device *dev,
	int16_t *x, int16_t *y)
{
	struct auxdisplay_seg8_data *data = dev->data;
	*x = data->cursor_x;
	*y = data->cursor_y;
	return 0;
}

static int auxdisplay_seg8_custom_character_set(const struct device *dev,
	struct auxdisplay_character *character)
{
	return -ENOTSUP;
}

static int auxdisplay_seg8_write(const struct device *dev, const uint8_t *text,
	uint16_t len)
{
	const struct auxdisplay_seg8_config *config = dev->config;
	struct auxdisplay_seg8_data *data = dev->data;

	for (size_t i = 0; i < len; ++i) {
		size_t pos = data->cursor_x + (config->capabilities.columns * data->cursor_y);
		data->digit_buf[pos] = text[i];

		if (data->direction == AUXDISPLAY_DIRECTION_RIGHT) {
			++data->cursor_x;
			if (data->cursor_x >= config->capabilities.columns) {
				data->cursor_x = 0;
				++data->cursor_y;
			}
			if (data->cursor_y >= config->capabilities.rows) {
				data->cursor_y = 0;
			}
		} else {
			--data->cursor_x;
			if (data->cursor_x < 0) {
				data->cursor_x = config->capabilities.columns - 1;
				--data->cursor_y;
			}
			if (data->cursor_y < 0) {
				data->cursor_y = config->capabilities.rows - 1;
			}
		}
	}

	return seg8_refresh(config, data);
}


static const struct auxdisplay_driver_api auxdisplay_seg8_auxdisplay_api = {
	.display_on = auxdisplay_seg8_display_on,
	.display_off = auxdisplay_seg8_display_off,
	.cursor_set_enabled = auxdisplay_seg8_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_seg8_position_blinking_set_enabled,
	.cursor_shift_set = auxdisplay_seg8_cursor_shift_set,
	.cursor_position_set = auxdisplay_seg8_cursor_position_set,
	.cursor_position_get = auxdisplay_seg8_cursor_position_get,
	.capabilities_get = auxdisplay_seg8_capabilities_get,
	.clear = auxdisplay_seg8_clear,
	.custom_character_set = auxdisplay_seg8_custom_character_set,
	.write = auxdisplay_seg8_write,
};

/* TODO: use auxdisplay_capabilities.mode to define multiplexed vs. individual segment selector */
#define GPIO_DT_SPEC_GET_BY_IDX_COMMA(node_id, prop, idx) \
	GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx) ,

#define AUXDISPLAY_SEG8_DEVICE(inst) \
	static struct gpio_dt_spec const auxdisplay_seg8_digit_select_gpios_##inst[] = {\
		DT_INST_FOREACH_PROP_ELEM(inst, digit_select_gpios, GPIO_DT_SPEC_GET_BY_IDX_COMMA)\
	};\
	static char auxdisplay_seg8_digit_buf_##inst[DT_INST_PROP_LEN(inst, digit_select_gpios)];\
	static struct auxdisplay_seg8_data auxdisplay_seg8_data_##inst = { \
		.digit_buf = auxdisplay_seg8_digit_buf_##inst,\
		.cursor_x = 0,\
		.cursor_y = 0,\
		.direction = AUXDISPLAY_DIRECTION_RIGHT,\
	}; \
	static struct auxdisplay_seg8_config auxdisplay_seg8_config_##inst = { \
		.capabilities = { \
			.columns = DT_INST_PROP(inst, columns), \
			.rows = DT_INST_PROP(inst, rows), \
			.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED, \
			.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED, \
			.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED, \
			.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED, \
			.custom_characters = 0, \
		}, \
		.segment_gpios = { \
			DT_INST_FOREACH_PROP_ELEM(inst, segment_gpios, GPIO_DT_SPEC_GET_BY_IDX_COMMA) \
		}, \
		.digit_select_gpios = auxdisplay_seg8_digit_select_gpios_##inst,\
		.num_segment_gpios = DT_INST_PROP_LEN(inst, segment_gpios), \
		.num_digit_select_gpios = DT_INST_PROP_LEN(inst, digit_select_gpios), \
	};\
	DEVICE_DT_INST_DEFINE(inst, \
		&auxdisplay_seg8_init, \
		NULL, \
		&auxdisplay_seg8_data_##inst, \
		&auxdisplay_seg8_config_##inst, \
		POST_KERNEL, \
		CONFIG_AUXDISPLAY_INIT_PRIORITY, \
		&auxdisplay_seg8_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_SEG8_DEVICE)
