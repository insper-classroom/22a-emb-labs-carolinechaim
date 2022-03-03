/**
 * 5 semestre - Eng. da Computação - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Projeto 0 para a placa SAME70-XPLD
 *
 * Objetivo :
 *  - Introduzir ASF e HAL
 *  - Configuracao de clock
 *  - Configuracao pino In/Out
 *
 * Material :
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 */

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"




/************************************************************************/
/* defines                                                              */
/************************************************************************/

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

/*  Default pin configuration (no attribute). */
#define _PIO_DEFAULT             (0u << 0)
/*  The internal pin pull-up is active. */
#define _PIO_PULLUP              (1u << 0)
/*  The internal glitch filter is active. */
#define _PIO_DEGLITCH            (1u << 1)
/*  The internal debouncing filter is active. */
#define _PIO_DEBOUNCE            (1u << 3)


/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

void init(void);

/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void _pio_set(Pio *p_pio, const uint32_t ul_mask)
{
	p_pio->PIO_SODR = ul_mask;
}
void _pio_clear(Pio *p_pio, const uint32_t ul_mask){
	p_pio->PIO_CODR = ul_mask;
}

void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_up_enable){
	if (ul_pull_up_enable){
		p_pio ->PIO_PUER = ul_mask;
	}else{
		p_pio ->PIO_PUDR = ul_mask;
	}
}

void _pio_set_input(Pio *p_pio, const uint32_t ul_mask,const uint32_t ul_attribute)
{
	p_pio ->PIO_IFSCER = ul_mask;
}

void _pio_set_output(Pio *p_pio, const uint32_t ul_mask,
const uint32_t ul_default_level,
const uint32_t ul_multidrive_enable,
const uint32_t ul_pull_up_enable)
{
	

/* Enable multi-drive if necessary */
if (ul_multidrive_enable) {
	p_pio->PIO_MDER = ul_mask;
	} else {
	p_pio->PIO_MDDR = ul_mask;
}

/* Set default value */
if (ul_default_level) {
	p_pio->PIO_SODR = ul_mask;
	} else {
	p_pio->PIO_CODR = ul_mask;
}

/* Configure pin(s) as output(s) */
p_pio->PIO_OER = ul_mask;
p_pio->PIO_PER = ul_mask;
}
}

// Função de inicialização do uC

void init(void){
	// Initialize the board clock
	sysclk_init();

	// Desativa WatchDog Timer
	WDT->WDT_MR = WDT_MR_WDDIS;
	// Ativa PIOs
	pmc_enable_periph_clk(LED_PIO_ID);
	pmc_enable_periph_clk(BUT_PIO_ID);
	
	pmc_enable_periph_clk(LED_PIO_ID_1);
	pmc_enable_periph_clk(LED_PIO_ID_2);
	pmc_enable_periph_clk(LED_PIO_ID_3);
	
	pmc_enable_periph_clk(BUT_PIO_ID_1);
	pmc_enable_periph_clk(BUT_PIO_ID_2);
	pmc_enable_periph_clk(BUT_PIO_ID_3);
	
	//Inicializa PIO como saída
	pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 0, 0, 0);
	pio_set_output(LED_PIO_1, LED_PIO_IDX_MASK_1, 0, 0, 0);
	pio_set_output(LED_PIO_2, LED_PIO_IDX_MASK_2, 0, 0, 0);
	pio_set_output(LED_PIO_3, LED_PIO_IDX_MASK_3, 0, 0, 0);

	
	//Inicializa PIO como entrada
	_pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_DEFAULT);
	_pio_set_input(BUT_PIO_1, BUT_PIO_IDX_MASK_1, PIO_DEFAULT);
	_pio_set_input(BUT_PIO_2, BUT_PIO_IDX_MASK_2, PIO_DEFAULT);
	_pio_set_input(BUT_PIO_3, BUT_PIO_IDX_MASK_3, PIO_DEFAULT);
	
	
	//Inicializa PULLUP 
	_pio_pull_up(BUT_PIO,BUT_PIO_IDX_MASK, 1);
	_pio_pull_up(BUT_PIO_1,BUT_PIO_IDX_MASK_1, 1);
	_pio_pull_up(BUT_PIO_2,BUT_PIO_IDX_MASK_2, 1);
	_pio_pull_up(BUT_PIO_3,BUT_PIO_IDX_MASK_3, 1);
}
/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
int main(void)
{

	init();


	// SUPER LOOP
	// aplicacoes embarcadas no devem sair do while(1).
	while(1) {
		
		if(!pio_get(BUT_PIO_3, PIO_INPUT, BUT_PIO_IDX_MASK_3)) {
			// Pisca LED
			for (int i=0; i<5; i++) {
				
				_pio_clear(LED_PIO_3, LED_PIO_IDX_MASK_3);  // Limpa o pino LED_PIO_PIN
				delay_ms(100);                  // delay
				_pio_set(LED_PIO_3, LED_PIO_IDX_MASK_3);  // Ativa o pino LED_PIO_PIN
				delay_ms(100);                         // delay
				// delay
			}
			} else  {
			// Ativa o pino LED_IDX (par apagar)
			_pio_set(LED_PIO_3, LED_PIO_IDX_MASK_3);
		}
		
		if(!pio_get(BUT_PIO_2, PIO_INPUT, BUT_PIO_IDX_MASK_2)) {
			// Pisca LED
			for (int i=0; i<5; i++) {
				
				_pio_clear(LED_PIO_2, LED_PIO_IDX_MASK_2);  // Limpa o pino LED_PIO_PIN
				delay_ms(100);                  // delay
				_pio_set(LED_PIO_2, LED_PIO_IDX_MASK_2);  // Ativa o pino LED_PIO_PIN
				delay_ms(100);                         // delay
				// delay
			}
			} else  {
			// Ativa o pino LED_IDX (par apagar)
			_pio_set(LED_PIO_2, LED_PIO_IDX_MASK_2);
		}
		// Verifica valor do pino que o botão está conectado
		if(!pio_get(BUT_PIO_1, PIO_INPUT, BUT_PIO_IDX_MASK_1)) {
			// Pisca LED
			for (int i=0; i<5; i++) {
				
				_pio_clear(LED_PIO_1, LED_PIO_IDX_MASK_1);  // Limpa o pino LED_PIO_PIN
				delay_ms(100);                  // delay
				_pio_set(LED_PIO_1, LED_PIO_IDX_MASK_1);  // Ativa o pino LED_PIO_PIN
				delay_ms(100);                         // delay
				// delay
			}
			} else  {
			// Ativa o pino LED_IDX (par apagar)
			_pio_set(LED_PIO_1, LED_PIO_IDX_MASK_1);
		}
		if(!pio_get(BUT_PIO, PIO_INPUT, BUT_PIO_IDX_MASK)) {
			// Pisca LED
			for (int i=0; i<5; i++) {
				
				_pio_clear(LED_PIO_1, LED_PIO_IDX_MASK_1);
				_pio_clear(LED_PIO_2, LED_PIO_IDX_MASK_2);
				_pio_clear(LED_PIO_3, LED_PIO_IDX_MASK_3);
				_pio_clear(LED_PIO, LED_PIO_IDX_MASK);  // Limpa o pino LED_PIO_PIN
				delay_ms(100);      
				_pio_set(LED_PIO, LED_PIO_IDX_MASK);                      // delay
				_pio_set(LED_PIO_1, LED_PIO_IDX_MASK_1);  
				_pio_set(LED_PIO_2, LED_PIO_IDX_MASK_2); 
				_pio_set(LED_PIO_3, LED_PIO_IDX_MASK_3);   // Ativa o pino LED_PIO_PIN
				delay_ms(100);                         // delay				
                   // delay
			}
			} else  {
			// Ativa o pino LED_IDX (par apagar)
			_pio_set(LED_PIO, LED_PIO_IDX_MASK);
			_pio_set(LED_PIO_1, LED_PIO_IDX_MASK_1);
			_pio_set(LED_PIO_2, LED_PIO_IDX_MASK_2);
			_pio_set(LED_PIO_3, LED_PIO_IDX_MASK_3);
		}
	}
	// Nunca devemos chegar aqui !
	return 0;
}
