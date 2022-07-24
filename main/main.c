/*
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

// This file contains a simple Hello World app which you can base you own
// native Badge apps on.

#include "main.h"

static pax_buf_t buf;
xQueueHandle button_queue;

typedef enum { LGBT_THEME_GAY = 0, LGBT_THEME_LES, LGBT_THEME_TRA, LGBT_THEME_BIS, LGBT_THEME_LAST } lgbt_theme_t;

typedef struct lgbt_flag {
	pax_col_t 	color;
	int 		count;
	struct lgbt_flag*	next;
} lgbt_flag_t;

#define const_pax_col_rgb(r, g, b) 0xff000000 | (r << 16) | (g << 8) | b

const pax_col_t gay[] = { 
	const_pax_col_rgb(209, 34, 41), const_pax_col_rgb(246, 138, 30),
	const_pax_col_rgb(253, 224, 26), const_pax_col_rgb(0, 121, 64),
	const_pax_col_rgb(36, 64, 142), const_pax_col_rgb(115, 41, 130)
};

const pax_col_t les[] = { 
	const_pax_col_rgb(213, 45, 0), const_pax_col_rgb(239, 118, 39),
	const_pax_col_rgb(255, 154, 86), const_pax_col_rgb(255, 255, 255),
	const_pax_col_rgb(209, 98, 164), const_pax_col_rgb(181, 86, 144),
	const_pax_col_rgb(163, 2, 98)
};

const pax_col_t tra[] = { 
	const_pax_col_rgb(91, 206, 250),
	const_pax_col_rgb(245, 169, 184),
	const_pax_col_rgb(255, 255, 255)
};

const pax_col_t bis[] = { 
	const_pax_col_rgb(214, 2, 112),
	const_pax_col_rgb(155, 79, 150),
	const_pax_col_rgb(0, 56, 168)
};

const lgbt_flag_t gay_flag_1 = { gay[5], 1, NULL };
const lgbt_flag_t gay_flag_2 = { gay[4], 1, &gay_flag_1 };
const lgbt_flag_t gay_flag_3 = { gay[3], 1, &gay_flag_2 };
const lgbt_flag_t gay_flag_4 = { gay[2], 1, &gay_flag_3 };
const lgbt_flag_t gay_flag_5 = { gay[1], 1, &gay_flag_4 };
const lgbt_flag_t gay_flag = { gay[0], 1, &gay_flag_5 };

const lgbt_flag_t les_flag_1 = { les[6], 1, NULL };
const lgbt_flag_t les_flag_2 = { les[5], 1, &les_flag_1 };
const lgbt_flag_t les_flag_3 = { les[4], 1, &les_flag_2 };
const lgbt_flag_t les_flag_4 = { les[3], 1, &les_flag_3 };
const lgbt_flag_t les_flag_5 = { les[2], 1, &les_flag_4 };
const lgbt_flag_t les_flag_6 = { les[1], 1, &les_flag_5 };
const lgbt_flag_t les_flag = { les[0], 1, &les_flag_6 };

const lgbt_flag_t tra_flag_1 = { tra[0], 1, NULL };
const lgbt_flag_t tra_flag_2 = { tra[1], 1, &tra_flag_1 };
const lgbt_flag_t tra_flag_3 = { tra[2], 1, &tra_flag_2 };
const lgbt_flag_t tra_flag_4 = { tra[1], 1, &tra_flag_3 };
const lgbt_flag_t tra_flag = { tra[0], 1, &tra_flag_4 };

const lgbt_flag_t bis_flag_1 = { bis[2], 2, NULL };
const lgbt_flag_t bis_flag_2 = { bis[1], 1, &bis_flag_1 };
const lgbt_flag_t bis_flag = { bis[0], 2, &bis_flag_2 };

const lgbt_flag_t flags[] = {
	gay_flag,
	les_flag,
	tra_flag,
	bis_flag
};

#include <esp_log.h>
static const char *TAG = "MCH-LGBTQ-Nametag";

// From: https://github.com/badgeteam/mch2022-firmware-esp32/blob/9aceecb2be831289d9ac4b80afe07d5dc7910523/main/nametag.c#L28
#define SLEEP_DELAY 10000
//From: https://github.com/badgeteam/mch2022-firmware-esp32/blob/9aceecb2be831289d9ac4b80afe07d5dc7910523/main/nametag.c#L122
char* read_nickname() {
    char *buffer = NULL;

    nvs_handle_t handle;
    esp_err_t    res = nvs_open("owner", NVS_READWRITE, &handle);

    // Read nickname.
    size_t required = 0;
    res             = nvs_get_str(handle, "nickname", NULL, &required);
    if (res) {
        //ESP_LOGE(TAG, "Error reading nickname: %s", esp_err_to_name(res));
        //buffer = strdup("BADGE.TEAM");
		buffer = "BADGE.TEAM";
    } else {
        buffer           = malloc(required + 1);
        buffer[required] = 0;
        res              = nvs_get_str(handle, "nickname", buffer, &required);
        if (res) {
            *buffer = 0;
        }
    }
    nvs_close(handle);

    return buffer;
}

// Updates the screen with the latest buffer.
void disp_flush() {
    ili9341_write(get_ili9341(), buf.buf);
}

// Exits the app, returning to the launcher.
void exit_to_launcher() {
    REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    esp_restart();
}

void app_main() {
  
    
    //ESP_LOGI(TAG, "Welcome to the template app!");

    // Initialize the screen, the I2C and the SPI busses.
    bsp_init();

    // Initialize the RP2040 (responsible for buttons, etc).
    bsp_rp2040_init();
    
    // This queue is used to receive button presses.
    button_queue = get_rp2040()->queue;
    
	float screen_w = 320;
    float	screen_h = 240;
	
    // Initialize graphics for the screen.
    pax_buf_init(&buf, NULL, screen_w, screen_h, PAX_BUF_16_565RGB);
    
    // Initialize NVS.
    nvs_flash_init();
    
    // Initialize WiFi. This doesn't connect to Wifi yet.
    wifi_init();
    
	lgbt_theme_t theme = LGBT_THEME_TRA;
	
    while (1) {
        int hue = esp_random() & 255;
        pax_col_t col = pax_col_hsv(hue, 255 /*saturation*/, 255 /*brighness*/);
        pax_background(&buf, col);
        
        lgbt_flag_t current_flag = flags[theme];
		int length = 0;
		while (current_flag != NULL) { 
			length += current_flag.count; current_flag = *current_flag.next; 
		}
		current_flag = flags[theme];
		float step = screen_h/length;
		float current_pos = 0;
		while (current_flag != NULL) {
			pax_simple_rect(&buf, current_flag.color, 0, current_pos, screen_w, step*current_flag.count);
			current_pos += step*current_flag.count;
			current_flag = *current_flag.next;
		} 
		
		char* text = read_nickname();
        const pax_font_t *font = pax_font_saira_condensed;
        pax_vec1_t        dims = pax_text_size(font, font->default_size, text);
        pax_draw_text(
            &buf, // Buffer to draw to.
            0xff000000, // color
            font, font->default_size, // Font and size to use.
            // Position (top left corner) of the app.
            (buf.width  - dims.x) / 2.0,
            (buf.height - dims.y) / 2.0,
            // The text to be rendered.
            text
        );

        // Draws the entire graphics buffer to the screen.
        disp_flush();
        
        // Wait for button presses and do another cycle.
        
        // Structure used to receive data.
        rp2040_input_message_t msg;
        if (xQueueReceive(button_queue, &msg, pdMS_TO_TICKS(SLEEP_DELAY + 10))) {
            if (msg.state) {
                switch (msg.input) {
                    case RP2040_INPUT_BUTTON_BACK:
                    case RP2040_INPUT_BUTTON_HOME:
                        exit_to_launcher();
                        break;
                    //case RP2040_INPUT_BUTTON_MENU:
                    //    edit_nickname(button_queue, pax_buffer, ili9341);
                    //    free(buffer);
                    //    buffer = read_nickname();
                    //    break;
                    case RP2040_INPUT_BUTTON_SELECT:
                        theme = (theme + 1) % LGBT_THEME_LAST;
                        break;
                    default:
                        break;
                }
            }
		}
		//uint64_t sleep_time = esp_timer_get_time() / 1000 + SLEEP_DELAY;
		//ESP_LOGI(TAG, "Recheduled sleep in %d millis", SLEEP_DELAY);
    }
}
