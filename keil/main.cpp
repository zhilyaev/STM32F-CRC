// include register definition from HAL library
#include "stm32f3xx.h"

#define MAN_RCC_GLIOE_ON 0x00200000
#define MAN_GPIOE_MODER_RESET 0xFFFFFCFF
#define MAN_GPIOE_MODER_OUT 0x55555555


/**
 * Helper function to modify register bits.
 */
void modifyRegister(volatile uint32_t* regAddr, uint32_t mask, uint32_t value)
{
    uint32_t tmp;
    tmp = *regAddr;
    tmp &= ~mask;
    tmp |= value & mask;
    *regAddr = tmp;
}

/**
 * Helper function that waits till specified bits are set to expected value.
 */
void waitRegisterValue(volatile uint32_t* regAddr, uint32_t mask, uint32_t expectedValue)
{
    while ((*regAddr & mask) != expectedValue);
}

void SEND(uint8_t data) {
  waitRegisterValue(&(USART2->ISR), 0x0080, 0x0080);
  USART2->TDR = data;
}

uint8_t GET(){
	waitRegisterValue(&(USART2->ISR), 0x0020, 0x0020);
	uint32_t tmp = USART2->ISR;
	tmp &= 0x0008;
	if (tmp | 0)
		modifyRegister(&(USART2->ICR), 0x0008, 0);
	return USART2->RDR;
}


/**
 * Helper function for delay implementation.
 *
 * @note the delay depends on complicator optimization and CPU frequency.
 */
void delay(uint32_t num_ops)
{
    volatile uint32_t count = 0;
    while (count < num_ops) {
        count++;
    }
}

/**
 * Function to configure CPU frequency (SYSCLOCK).
 *
 * After configurration CPU frequency will be changed from 8 MHz to 48 MHz.
 */
void configClock()
{
    // HSI is enabled by default and used as SYSCLOCK.
    // So currect SYSCLOCK frequency is 8 MHz

    // 1. Configure PLL (phase-locked loop) to increase frequncy
    // 1.1 Set CFGR bits 15-16 to 0b00 use HSI as source clock for PLL
    //     note: f_PLL_input = freq_HSI / 2 = 4 MHz
    modifyRegister(&(RCC->CFGR), 0x00018000, 0x00000000);
    // 1.2 Set CFGR bits 18-21 to 0b1010 for PLL  multiplication factor 12
    //     note: f_PLL_output = f_PLL_input * 12 = 4 * 12 = 48 MHz
    modifyRegister(&(RCC->CFGR), 0x003C0000, 0x000C0000);
    // 1.3 Enable PLL (set CR bit 24 to 1).
    modifyRegister(&(RCC->CR), 0x01000000, 0x01000000);
    // 1.4 Wait till PLL is enable (check CR bit 25).
    waitRegisterValue(&(RCC->CR), 0x02000000, 0x02000000);
    // 2. Switch SYSCLK from HSI to PLL
    // 2.1 Set PLL as SYSCLK source (set CFGR bits 0-1 to 0b10).
    modifyRegister(&(RCC->CFGR), 0x00000003, 0x00000002);
    // 2.2 Wait end of SYSCLK source configuration (check that CFGR bits 2-3 is 0b10).
    waitRegisterValue(&(RCC->CFGR), 0x0000000C, 0x00000008);

    // update global variable with system frequency
    // note: we can use it check that set configure frequency correctly (the SystemCoreClock should be set to 0X02DC6C00)
    SystemCoreClockUpdate();
}

void ledInit(){
	RCC->AHBENR |= MAN_RCC_GLIOE_ON;  // connect port E to RCC
	GPIOE->MODER &= MAN_GPIOE_MODER_RESET;
	GPIOE->MODER |= MAN_GPIOE_MODER_OUT;
}

void ledOn(const unsigned char& led){
	GPIOE->ODR &= 0;
	std::uint32_t mask = (std::uint32_t)led << 8;
	GPIOE->ODR = mask;
}

void ledOff(){
	GPIOE->ODR &= 0;
}

void CRCinit(){
	RCC->AHBENR |= RCC_AHBENR_CRCEN;
	CRC->POL = 0xCB;
	CRC->CR = 0x11;
}

int main(void)
{
    // configure CPU frequency to 48 MHz
  configClock();
	ledInit();
	CRCinit();
	modifyRegister(&(RCC->AHBENR), 0x00020000, 0x00020000); // GPIOA
	modifyRegister(&(GPIOA->OSPEEDR), 0x000000F0, 0x000000F0); // high speed PA2, PA3
	modifyRegister(&(GPIOA->PUPDR), 0x000000F0, 0x00000000); // no push, no pull
	modifyRegister(&(GPIOA->OTYPER), 0x0000000C, 0x00000000); // push-pull
	modifyRegister(&(GPIOA->AFR[0]), 0x0000FF00, 0x00007700); // AF7 for GPIOA
	modifyRegister(&(GPIOA->MODER), 0x000000F0, 0x000000A0); // en AF
	modifyRegister(&(RCC->APB1ENR), 0x00020000, 0x00020000); // enable USART2 clock
	modifyRegister(&(USART2->CR1), 0x1000140C, 0x0000000C); // enable USART2 clock
	modifyRegister(&(RCC->CFGR3), 0x00030000, 0x00010000); // sysclk source for USART2
	modifyRegister(&(USART2->BRR), 0xFFFF, 0x0823); // baudrate
	modifyRegister(&(USART2->CR1), 0x0001, 0x0001); // UE = 1
	waitRegisterValue(&(USART2->ISR), 0x00600000, 0x00600000); // transmit, receive
	
	//waitRegisterValue(&(USART2->ISR), 0x0080, 0x0080);
  while (1){
		char c = GET();
		CRC->CR = 0x11;
		volatile std::uint8_t* dr = (volatile std::uint8_t*)&(CRC->DR);
		*dr = c;
		char res = CRC->DR;
		ledOn(res);
		SEND(res);
	}
}

