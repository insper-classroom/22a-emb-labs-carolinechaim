/************************************************************************/
/* includes                                                             */
/************************************************************************/
#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/

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

// Configuracoes Led-placa
#define LED_PIO           PIOC                 // periferico que controla o LED
#define LED_PIO_ID        ID_PIOC                  // ID do periférico PIOC (controla LED)
#define LED_PIO_IDX       8                    // ID do LED no PIO
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX)   // Mascara para CONTROLARMOS o LED


/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/
volatile char but_flag;
volatile char but_3_flag;
volatile char parar;

/************************************************************************/
/* prototype                                                            */
/************************************************************************/
void io_init(void);
void pisca_led(int t);

/************************************************************************/
/* handler / callbacks                                                  */
/************************************************************************/

void but_1_callback(void)
{
	if (pio_get(BUT_PIO_1, PIO_INPUT, BUT_PIO_IDX_MASK_1)) {
		but_flag = 0;
		} else {
		but_flag = 1;
	}
}

void but_2_callback(void)
{
	if(parar == 0){
		parar = 1;
		} else{
		parar = 0;
	}
}

void but_3_callback(void)
{
	but_3_flag = 1;
}



/************************************************************************/
/* funções                                                              */
/************************************************************************/

// pisca led N vez no periodo T
void pisca_led(int t){
	for (int i=0;i<30;i++){
		if (but_flag)
			break;
		pio_clear(LED_PIO, LED_PIO_IDX_MASK);
		delay_us(t/2);
			if (but_flag)
			break;
		pio_set(LED_PIO, LED_PIO_IDX_MASK);
		delay_us(t);
	}
}

void io_init(void)
{

	pmc_enable_periph_clk(LED_PIO_ID);
	pmc_enable_periph_clk(BUT_PIO_ID_1);
	pmc_enable_periph_clk(BUT_PIO_ID_2);
	pmc_enable_periph_clk(BUT_PIO_ID_3);
	
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_PIO_IDX_MASK, PIO_DEFAULT);
	pio_configure(BUT_PIO_1, PIO_INPUT, BUT_PIO_IDX_MASK_1, PIO_PULLUP);
	pio_configure(BUT_PIO_2, PIO_INPUT, BUT_PIO_IDX_MASK_2, PIO_PULLUP);
	pio_configure(BUT_PIO_3, PIO_INPUT, BUT_PIO_IDX_MASK_3, PIO_PULLUP);
	
	pio_handler_set(BUT_PIO_1,
	BUT_PIO_ID_1,
	BUT_PIO_IDX_MASK_1,
	PIO_IT_EDGE,
	but_1_callback);

	pio_handler_set(BUT_PIO_2,
	BUT_PIO_ID_2,
	BUT_PIO_IDX_MASK_2,
	PIO_IT_RISE_EDGE,
	but_2_callback);

	pio_handler_set(BUT_PIO_3,
	BUT_PIO_ID_3,
	BUT_PIO_IDX_MASK_3,
	PIO_IT_RISE_EDGE,
	but_3_callback);
	
	

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT_PIO_1, BUT_PIO_IDX_MASK_1);
	pio_get_interrupt_status(BUT_PIO_1);

	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO_ID_1);
	NVIC_SetPriority(BUT_PIO_ID_1, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT_PIO_ID_2);
	NVIC_SetPriority(BUT_PIO_ID_2, 4); // Prioridade 4

	NVIC_EnableIRQ(BUT_PIO_ID_3);
	NVIC_SetPriority(BUT_PIO_ID_3, 4); // Prioridade 4
}


int main (void)
{
	double freq = 100;
	double tempo =  1000 / (1.0/freq);
	char str[10];
	
	// Desativa watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	board_init();
	sysclk_init();
	delay_init();
	
io_init();

  // Init OLED
	gfx_mono_ssd1306_init();
  
  // Escreve na tela um circulo e um texto

  /* Insert application code here, after the board has been initialized. */
	while(1) {

		
		if(but_flag == 1){
			delay_ms(300);
			if(but_flag == 1){
				freq += 100;
			}
			else{
				freq += 100;
			}
			sprintf(str, "%6.0lf", freq);
			gfx_mono_draw_string(str, 0, 0, &sysfont);	
			tempo = 1000 / (1/freq);
			pisca_led(tempo);		
		}
		
		if(but_3_flag == 1){
			freq -= 100;
			but_3_flag = 0;
			sprintf(str, "%6.0lf", freq);
			gfx_mono_draw_string(str, 0, 0, &sysfont);
			tempo = 1000 / (1/freq);
			pisca_led(tempo);
		}
		
		if(parar == 0){
			
			pisca_led(tempo);
			gfx_mono_generic_draw_filled_rect(10,20,30,10, 0);
			pio_set(LED_PIO_ID, LED_PIO_IDX_MASK);
			} else {
			if(but_flag == 0){
				pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
			}
		}		

			
			 
 	}
}
