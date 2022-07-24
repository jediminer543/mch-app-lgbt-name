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
xQueueHandle buttonQueue;

typedef enum { LGBT_THEME_GAY = 0, LGBT_THEME_LES, LGBT_THEME_TRA, LGBT_THEME_BIS, LGBT_THEME_LAST } lgbt_theme_t;

typedef struct lgbt_flag {
	pax_col_t 	color;
	int 		count;
	lgbt_flag*	next;
} lgbt_flag_t;

pax_col_t[] gay = { 
	pax_col_rgb(209, 34, 41), pax_col_rgb(246, 138, 30),
	pax_col_rgb(253, 224, 26), pax_col_rgb(0, 121, 64),
	pax_col_rgb(36, 64, 142), pax_col_rgb(115, 41, 130)
};

pax_col_t[] les = { 
	pax_col_rgb(213, 45, 0), pax_col_rgb(239, 118, 39),
	pax_col_rgb(255, 154, 86), pax_col_rgb(255, 255, 255),
	pax_col_rgb(209, 98, 164), pax_col_rgb(181, 86, 144),
	pax_col_rgb(163, 2, 98)
};

pax_col_t[] tra = { 
	pax_col_rgb(91, 206, 250),
	pax_col_rgb(245, 169, 184),
	pax_col_rgb(255, 255, 255)
};

pax_col_t[] bis = { 
	pax_col_rgb(214, 2, 112),
	pax_col_rgb(155, 79, 150),
	pax_col_rgb(0, 56, 168)
};

lgbt_flag_t gay_flag = { gay[5], 1, NULL };
gay_flag = { gay[4], 1, &gay_flag };
gay_flag = { gay[3], 1, &gay_flag };
gay_flag = { gay[2], 1, &gay_flag };
gay_flag = { gay[1], 1, &gay_flag };
gay_flag = { gay[0], 1, &gay_flag };

lgbt_flag_t les_flag = { les[6], 1, NULL };
les_flag = { les[5], 1, &les_flag };
les_flag = { les[4], 1, &les_flag };
les_flag = { les[3], 1, &les_flag };
les_flag = { les[2], 1, &les_flag };
les_flag = { les[1], 1, &les_flag };
les_flag = { les[0], 1, &les_flag };

lgbt_flag_t tra_flag = { tra[0], 1, NULL };
tra_flag = { tra[1], 1, &tra_flag };
tra_flag = { tra[2], 1, &tra_flag };
tra_flag = { tra[1], 1, &tra_flag };
tra_flag = { tra[0], 1, &tra_flag };

lgbt_flag_t bis_flag = { bis[2], 2, NULL };
bis_flag = { bis[1], 1, &bis_flag };
bis_flag = { bis[0], 2, &bis_flag };

const lgbt_flag_t[] flags = {
	gay_flag,
	les_flag,
	tra_flag,
	bis_flag
}

#include <esp_log.h>
static const char *TAG = "MCH LGBTQ+ ";

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
        ESP_LOGE(TAG, "Error reading nickname: %s", esp_err_to_name(res));
        buffer = strdup("BADGE.TEAM");
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
  
    
    ESP_LOGI(TAG, "Welcome to the template app!");

    // Initialize the screen, the I2C and the SPI busses.
    bsp_init();

    // Initialize the RP2040 (responsible for buttons, etc).
    bsp_rp2040_init();
    
    // This queue is used to receive button presses.
    buttonQueue = get_rp2040()->queue;
    
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
		do { length += current_flag.count } while ((current_flag = current_flag.next) != NULL);
		current_flag = flags[theme];
		float step = screen_h/length;
		float current_pos = 0;
		do {
			pax_simple_rect(&buf, current_flag.color, 0, current_pos, screen_w, step*current_flag.count);
			current_pos += step*current_flag.count;
		} while ((current_flag = current_flag.next) != NULL);
		
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
                        quit = true;
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
		sleep_time = esp_timer_get_time() / 1000 + SLEEP_DELAY;
		ESP_LOGI(TAG, "Recheduled sleep in %d millis", SLEEP_DELAY);
    }
}
