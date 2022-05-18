/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"

LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg50);
LV_FONT_DECLARE(dseg40);
LV_FONT_DECLARE(dseg30);
// global
static  lv_obj_t * labelBtn1;
static  lv_obj_t * labelMenu;
static  lv_obj_t * labelClk;
static  lv_obj_t * labelbtnUp;
static  lv_obj_t * labelbtnDown;
static lv_obj_t * labelFloor;
static lv_obj_t * labelSetValue;
static lv_obj_t * labelClock;


typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

calendar calendario = {2022, 5, 16, 20, 16, 8, 10};

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;

volatile char flag_rtc = 0;

QueueHandle_t xQueueRtc;
SemaphoreHandle_t xSemaphoreRtc;


/*************************************************s***********************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_RTC_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_RTC_PRIORITY            (tskIDLE_PRIORITY)


extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void menu_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void clk_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void up_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
    if(code == LV_EVENT_CLICKED) {
	    c = lv_label_get_text(labelSetValue);
	    temp = atoi(c);
	    lv_label_set_text_fmt(labelSetValue, "%02d", temp + 1);
    }
}

static void down_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
    if(code == LV_EVENT_CLICKED) {
	    c = lv_label_get_text(labelSetValue);
	    temp = atoi(c);
	    lv_label_set_text_fmt(labelSetValue, "%02d", temp - 1);
    }
}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o c?digo para irq de segundo vem aqui
		flag_rtc = 1;
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreRtc, &xHigherPriorityTaskWoken);
		xQueueSendFromISR(xQueueRtc, (void *)&flag_rtc, &xHigherPriorityTaskWoken);
		
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o c?digo para irq de alame vem aqui
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}


void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

void task_rtc() {
	RTC_init(RTC, ID_RTC, calendario, RTC_IER_SECEN);
	int32_t delayticks = 0;
	
	for(;;) {
		if (xQueueReceive(xQueueRtc, &delayticks, (TickType_t) 0)){
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			char *c;
			c = lv_label_get_text(labelClock);
			lv_label_set_text_fmt(labelClock, "%02d:%02d", current_hour, current_min);
			delay_ms(500);
			lv_label_set_text_fmt(labelClock, "%02d %02d", current_hour, current_min);
			delay_ms(500);
			lv_label_set_text_fmt(labelClock, "%02d:%02d", current_hour, current_min);
		}
	}
}

void lv_termostato(void) {
	

	
	//POWER BUTTON 
	
		static lv_style_t style;
		lv_style_init(&style);
		lv_style_set_bg_color(&style, lv_color_black());

	    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	    lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 5, 0);
		lv_obj_add_style(btn1, &style, 0);
		lv_obj_set_width(btn1, 60);  lv_obj_set_height(btn1, 60);

	    labelBtn1 = lv_label_create(btn1);
	    lv_label_set_text(labelBtn1, "[  "LV_SYMBOL_POWER"  |");
	    lv_obj_center(labelBtn1);
		
	//MENU BUTTON
	
		static lv_style_t style_menu;
		lv_style_init(&style_menu);
		lv_style_set_bg_color(&style_menu, lv_color_black());

		lv_obj_t * btnMenu = lv_btn_create(lv_scr_act());
		lv_obj_add_event_cb(btnMenu, menu_handler, LV_EVENT_ALL, NULL);
		lv_obj_align_to(btnMenu,btn1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
		lv_obj_add_style(btnMenu, &style_menu, 0);
		lv_obj_set_width(btnMenu, 60);  lv_obj_set_height(btnMenu, 60);

		labelMenu = lv_label_create(btnMenu);
		lv_label_set_text(labelMenu, "M  |");
		lv_obj_center(labelMenu);		
	
	// CLK BUTTON
	
	static lv_style_t style_clk;
	lv_style_init(&style_clk);
	lv_style_set_bg_color(&style_clk, lv_color_black());

	lv_obj_t * btnClk = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnClk, clk_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btnClk, btnMenu, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_obj_add_style(btnClk, &style_clk, 0);
	lv_obj_set_width(btnClk, 60);  lv_obj_set_height(btnClk, 60);

	labelClk = lv_label_create(btnClk);
	lv_label_set_text(labelClk, LV_SYMBOL_SETTINGS"  ]");
	lv_obj_center(labelClk);
	
	// UP BUTTON
	
	static lv_style_t style_up;
	lv_style_init(&style_up);
	lv_style_set_bg_color(&style_up, lv_color_black());

	lv_obj_t * btnUp = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnUp, up_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btnUp, btnClk, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
	lv_obj_add_style(btnUp, &style_up, 0);
	lv_obj_set_width(btnUp, 60);  lv_obj_set_height(btnUp, 60);

	labelbtnUp = lv_label_create(btnUp);
	lv_label_set_text(labelbtnUp, "[ "LV_SYMBOL_UP);
	lv_obj_center(labelbtnUp);

	// DOWN BUTTON
	
	static lv_style_t style_down;
	lv_style_init(&style_down);
	lv_style_set_bg_color(&style_down, lv_color_black());

	lv_obj_t * btnDown = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnDown, down_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btnDown, btnUp, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_obj_add_style(btnDown, &style_down, 0);
	lv_obj_set_width(btnDown, 60);  lv_obj_set_height(btnDown, 60);

	labelbtnDown = lv_label_create(btnDown);
	lv_label_set_text(labelbtnDown, LV_SYMBOL_DOWN" ]");
	lv_obj_center(labelbtnDown);	
	
	//FLOOR
	
    labelFloor = lv_label_create(lv_scr_act());
    lv_obj_align(labelFloor, LV_ALIGN_LEFT_MID, 35 , -25);
    lv_obj_set_style_text_font(labelFloor, &dseg70, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(labelFloor, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text_fmt(labelFloor, "%02d", 23);	
	
	
	//SETVALUE
	
	labelSetValue = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelSetValue, labelFloor, LV_ALIGN_OUT_RIGHT_TOP, 50, 0);
	lv_obj_set_style_text_font(labelSetValue, &dseg50, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelSetValue, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelSetValue, "%02d", 23);
	
	
	//CLOCK
	
	labelClock = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelClock, labelSetValue, LV_ALIGN_TOP_MID, -40, -50);
	lv_obj_set_style_text_font(labelClock, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelClock, lv_color_white(), LV_STATE_DEFAULT);
	
}


/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;

	 lv_termostato();


	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	
	xSemaphoreRtc = xSemaphoreCreateBinary();
	if (xSemaphoreRtc == NULL)
	printf("Criação do semáforo falhou");
	
	xQueueRtc = xQueueCreate(32, sizeof(uint32_t));
	if (xQueueRtc == NULL)
	printf("Criação da Queue falhou");
	
	
	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
	if (xTaskCreate(task_rtc, "RTC", TASK_RTC_SIZE, NULL, TASK_RTC_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create RTC task\r\n");
	}
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}
