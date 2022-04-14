#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/


typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;


// Configuracoes Led-placa
#define LED_PIO           PIOC                 // periferico que controla o LED
#define LED_PIO_ID        ID_PIOC                  // ID do periférico PIOC (controla LED)
#define LED_PIO_IDX       8                    // ID do LED no PIO
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX)   // Mascara para CONTROLARMOS o LED


// Configuracoes Led-1
#define LED_PIO_1 PIOA
#define LED_PIO_ID_1 7
#define LED_PIO_IDX_1 0
#define LED_PIO_IDX_MASK_1 (1u << LED_PIO_IDX_1) // esse já está pronto.

// Configuracoes Led-2
#define LED_PIO_2 PIOC
#define LED_PIO_ID_2 8
#define LED_PIO_IDX_2 30
#define LED_PIO_IDX_MASK_2 (1u << LED_PIO_IDX_2) // esse já está pronto.

// Configuracoes Led-3
#define LED_PIO_3 PIOB
#define LED_PIO_ID_3 6
#define LED_PIO_IDX_3 2
#define LED_PIO_IDX_MASK_3 (1u << LED_PIO_IDX_3) // esse já está pronto.


// Configuracoes do botao PLACA
#define BUT_PIO PIOA
#define BUT_PIO_ID ID_PIOA
#define BUT_PIO_IDX 11
#define BUT_PIO_IDX_MASK (1u << BUT_PIO_IDX) // esse já está pronto.

// Configuracoes do botao 1
#define BUT_PIO_1 PIOD
#define BUT_PIO_ID_1 ID_PIOD
#define BUT_PIO_IDX_1 28
#define BUT_PIO_IDX_MASK_1 (1u << BUT_PIO_IDX_1) // esse já está pronto.

// Configuracoes do botao 2
#define BUT_PIO_2 PIOC
#define BUT_PIO_ID_2 ID_PIOC
#define BUT_PIO_IDX_2 31
#define BUT_PIO_IDX_MASK_2 (1u << BUT_PIO_IDX_2) // esse já está pronto.

// Configuracoes do botao 3
#define BUT_PIO_3 PIOA
#define BUT_PIO_ID_3 ID_PIOA
#define BUT_PIO_IDX_3 19
#define BUT_PIO_IDX_MASK_3 (1u << BUT_PIO_IDX_3) // esse já está pronto.


/************************************************************************/
/* VAR globais                                                          */
/************************************************************************/
volatile char flag_rtc_alarm = 0;

/************************************************************************/
/* PROTOTYPES                                                           */
/************************************************************************/

void LED_init(int estado);
void pin_toggle(Pio *pio, uint32_t mask);
void pisca_led(int n, int t);
void gfx_mono_draw_string(const char *str, const gfx_coord_t x, const gfx_coord_t y, const struct font *font);

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);

/************************************************************************/
/* Handlers                                                             */
/************************************************************************/

/**
*  Interrupt handler for TC1 interrupt.
*/
void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO_1, LED_PIO_IDX_MASK_1);  
}


void TC2_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 2);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_PIO_IDX_MASK);  
}


void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		RTT_init(4, 0, RTT_MR_RTTINCIEN);
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		pin_toggle(LED_PIO_2, LED_PIO_IDX_MASK_2);    // BLINK Led
	}

}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
		flag_rtc_alarm = 1;
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}





/************************************************************************/
/* Funcoes                                                              */
/************************************************************************/

/**
* @Brief Inicializa o pino do LED
*/

void pisca_led (int n, int t) {
	for (int i=0;i<n;i++){
		pio_clear(LED_PIO_IDX_3, LED_PIO_IDX_MASK_3);
		delay_ms(t);
		pio_set(LED_PIO_3, LED_PIO_IDX_MASK_3);
		delay_ms(t);
	}
}

void LED_init(int estado) {
	pmc_enable_periph_clk(LED_PIO_ID_1);
	pio_set_output(LED_PIO_1, LED_PIO_IDX_MASK_1, estado, 0, 0);
	
    pmc_enable_periph_clk(LED_PIO_ID_3);
    pio_set_output(LED_PIO_3, LED_PIO_IDX_MASK_3, estado, 0, 0 );

	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_PIO_IDX_MASK, estado, 0, 0);
		
	pmc_enable_periph_clk(LED_PIO_ID_2);
	pio_configure(LED_PIO_2, PIO_OUTPUT_1, LED_PIO_IDX_MASK_2, PIO_DEFAULT);
};

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

/**
* @Brief Inverte o valor do pino 0->1/ 1->0
*/
void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}


void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}


static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
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

/************************************************************************/
/* Main                                                         */
/************************************************************************/

int main (void)
{
	/* Initialize the SAM system */
	sysclk_init();
	board_init();
	char str[25];
	gfx_mono_ssd1306_init();

	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;

	/* Configura Leds */
	LED_init(1);
	

	
	


	/**
	 * Configura timer TC0, canal 1 
	 * e inicializa contagem 
	 */
	TC_init(TC0, ID_TC1, 1, 4);
	tc_start(TC0, 1);
	
	TC_init(TC0, ID_TC2, 2, 5);
	tc_start(TC0, 2);
	
	RTT_init(4, 16, RTT_MR_ALMIEN);     
	
	/** Configura RTC */
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN);
	
    /* Leitura do valor atual do RTC */
    uint32_t current_hour, current_min, current_sec;
    uint32_t current_year, current_month, current_day, current_week;
    rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
    rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
    
    /* configura alarme do RTC para daqui 20 segundos */
    rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
    rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 5);

	while (1) {
      if(flag_rtc_alarm){
	      pisca_led(5, 200);
	      flag_rtc_alarm = 0;
      }
			rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			sprintf(str, "%02d:%02d:%02d", current_hour, current_min, current_sec);
			gfx_mono_draw_string(str, 0, 0, &sysfont);
            pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
