#include <asf.h>


#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

//PINO ECHO
#define ECHO				PIOD
#define	ECHO_ID			ID_PIOD
#define ECHO_IDX			26
#define ECHO_IDX_MASK	(1 << ECHO_IDX)	

//PINO TRIGGER
#define TRIG				PIOA
#define	TRIG_ID			ID_PIOA
#define TRIG_IDX			24
#define TRIG_IDX_MASK	(1 << TRIG_IDX)


//LED 1
#define LED_PI1			  PIOA
#define LED_PI1_ID		  ID_PIOA
#define LED_PI1_IDX		  0
#define LED_PI1_IDX_MASK  (1 << LED_PI1_IDX)   // Mascara para CONTROLARMOS o LED



// Configuracoes do BOTAO 1
#define BUT_PI1			  PIOD
#define BUT_PI1_ID        ID_PIOD
#define BUT_PI1_IDX	      28
#define BUT_PI1_IDX_MASK (1u << BUT_PI1_IDX)



//Variaveis globais
volatile double tempo = 0;
volatile double flag = 0;
volatile double dist = 0;
double freq = (float) 1/(2*0.000058);

char str[300];

void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void config(void) {
	
	pmc_enable_periph_clk(LED_PI1_ID);
	pio_set_output(LED_PI1, LED_PI1_IDX_MASK, 0, 0, 0);
	// Inicializa PIO do BOTAO 1
	pmc_enable_periph_clk(BUT_PI1_ID);
	// configura pino ligado ao bot?o como entrada com um pull-up.
	pio_set_input(BUT_PI1,BUT_PI1_IDX_MASK,PIO_DEFAULT);
	pio_pull_up(BUT_PI1,BUT_PI1_IDX_MASK,1);
	pio_set_debounce_filter(BUT_PI1, BUT_PI1_IDX_MASK, 60);
	
	
}



volatile double tempo_rtt = 0;
volatile char flag_rtc_alarm = 0;

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t seccond;
} calendar;

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;


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

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		//pin_toggle(LED_PI2, LED_PI2_IDX_MASK);    // BLINK Led
		tempo_rtt+=1;
	}
}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o c?digo para irq de segundo vem aqui
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o c?digo para irq de alame vem aqui
		flag_rtc_alarm = 1;
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
	rtc_set_time(rtc, t.hour, t.minute, t.seccond);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrup?c?o no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}
void callback(void){
		
	if (!flag) {
		RTT_init(freq, 0, 0);	
		flag = 1;	
	} 
	else {
		flag = 0;
		tempo = rtt_read_timer_value(RTT);
	}
}

void init(void) {
	//Initialize the board clock
	sysclk_init();
	config();
	
	// Desativa WatchDog Timer
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	pmc_enable_periph_clk(ECHO_ID);
	pio_set_input(ECHO,ECHO_IDX_MASK,PIO_DEFAULT);
	pmc_enable_periph_clk(TRIG_ID);
	pio_configure(TRIG, PIO_OUTPUT_0,TRIG_IDX_MASK, PIO_DEFAULT);
	
	pio_handler_set(ECHO,
		ECHO_ID,
		ECHO_IDX_MASK,
		PIO_IT_EDGE,
		callback);
		
	pio_enable_interrupt(ECHO, ECHO_IDX_MASK);
	pio_get_interrupt_status(ECHO);
	
	NVIC_EnableIRQ(ECHO_ID);
	NVIC_SetPriority(ECHO_ID, 4); // Prioridade 4

}

int main (void)
{
	board_init();
	sysclk_init();
	init();
	delay_init();

  // Init OLED
	gfx_mono_ssd1306_init();

  /* Insert application code here, after the board has been initialized. */
	while(1) {	
		if (!pio_get(BUT_PI1,PIO_INPUT, BUT_PI1_IDX_MASK)) {		
				pio_set(TRIG,TRIG_IDX_MASK);
				delay_us(10);
				pio_clear(TRIG,TRIG_IDX_MASK);
			}
		if (tempo != 0) {
			double  tempo_real = (float) tempo/freq;
			float distancia_cm = (340*tempo_real*100.0)/2.0;
			
			sprintf(str, "%6.2lf", distancia_cm);
			gfx_mono_draw_string(str, 0,16, &sysfont);
			tempo = 0;				
		}
			
	}
}
