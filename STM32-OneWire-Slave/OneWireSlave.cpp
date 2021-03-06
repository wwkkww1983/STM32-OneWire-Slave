#include "OneWireSlave.h"

OneWireSlave::OneWireSlave() {

}
OneWireSlave::~OneWireSlave() {
	
}
inline uint8_t OneWireSlave::getPinNumber(uint16_t pin)
{
	pin -= 1;
	pin = pin - ((pin >> 1) & 0x55555555);
	pin = (pin & 0x33333333) + ((pin >> 2) & 0x33333333);
	pin = (pin + (pin >> 4)) & 0x0F0F0F0F;
	pin = pin + (pin >> 8);
	pin = pin + (pin >> 16);
	pin &= 0x0000003F;
	return pin;
}
inline uint8_t OneWireSlave::calculateCRC8(uint8_t seed, uint8_t inData)
{
	uint8_t bitsLeft = 0;
	uint8_t tempp = 0;
	for (bitsLeft = 8; bitsLeft > 0; bitsLeft--)
	{
		tempp = ((seed ^ inData) & 0x01);
		if (tempp == 0) seed >>= 1;
		else
		{
			seed ^= 0x18;
			seed >>= 1;
			seed |= 0x80;
		}
		inData >>= 1;
	}
	return seed;
}

void OneWireSlave::setROM(uint8_t rom[6]) {
	uint8_t cnt = 0;
	this->ROM[0] = 0x01;
	uint8_t crc = calculateCRC8(0, this->ROM[0]);
	for (uint8_t i = 1; i < 7; i++)
	{
		this->ROM[i] = rom[i - 1];
		crc = calculateCRC8(crc, this->ROM[i]);
	}
	ROM[7] = crc;
}

inline void OneWireSlave::SET_PIN_MODE_OUT()
{
	if (this->GPIO_InitStructure.GPIO_Pin < GPIO_Pin_8)
	{
		this->GPIO->CRL &= ~regGPIO._GPIO_CNF;
		this->GPIO->CRL |= regGPIO._GPIO_MODE;
	}
	else
	{
		this->GPIO->CRH &= ~regGPIO._GPIO_CNF;
		this->GPIO->CRH |= regGPIO._GPIO_MODE;
	}
}

inline void OneWireSlave::SET_PIN_MODE_IN() {
	if (this->GPIO_InitStructure.GPIO_Pin < GPIO_Pin_8)
	{
		this->GPIO->CRL &= ~(regGPIO._GPIO_CNF | regGPIO._GPIO_MODE);
		this->GPIO->CRL |= regGPIO._GPIO_CNF_0;
	}
	else
	{
		this->GPIO->CRH &= ~(regGPIO._GPIO_CNF | regGPIO._GPIO_MODE);
		this->GPIO->CRH |= regGPIO._GPIO_CNF_0;
	}
}
inline void OneWireSlave::SET_PIN_VALUE(bool bitVal) {
	GPIO_WriteBit(this->GPIO, this->GPIO_InitStructure.GPIO_Pin, (BitAction)bitVal);
}

void OneWireSlave::init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin_x, uint32_t EXTI_Line) {
	/* Check the parameters */
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	if (GPIOx == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		this->portSource = GPIO_PortSourceGPIOA;
	}
	if (GPIOx == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		this->portSource = GPIO_PortSourceGPIOB;
	}
	if (GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		this->portSource = GPIO_PortSourceGPIOC;
	}
	if (GPIOx == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
		this->portSource = GPIO_PortSourceGPIOD;
		
	}
	if (GPIOx == GPIOE)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		this->portSource = GPIO_PortSourceGPIOE;
	}
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	this->pinSource = getPinNumber(GPIO_Pin_x);
	uint8_t lsh = 4 * pinSource;
	this->regGPIO._GPIO_CNF =  GPIO_CRL_CNF0 << lsh;
	this->regGPIO._GPIO_CNF_0 = GPIO_CRL_CNF0_0 << lsh;
	this->regGPIO._GPIO_MODE =  GPIO_CRL_MODE0 << lsh;
	
	this->GPIO = GPIOx;
	this->GPIO_InitStructure.GPIO_Pin = GPIO_Pin_x;
	this->GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	this->GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(this->GPIO, &this->GPIO_InitStructure);
	
	SET_PIN_MODE_IN();
	
	this->NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
	this->NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
	this->NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
	this->NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	GPIO_EXTILineConfig(this->portSource, this->pinSource);
	this->EXTI_InitStruct.EXTI_Line = EXTI_Line;
	this->EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	this->EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	this->EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_Init(&EXTI_InitStruct);
}
void OneWireSlave::reset(void) {
	this->counterReciveCMD = 0;
	this->reciveCMD = 0;
	this->counterBitWrite = 0;
	this->isMasterReset = false;
	this->isSearchRom = false;
	this->searchRomState = 0;
	this->isCmdReceive = false;
}
void OneWireSlave::setIsEnable(bool isEnable)
{
	this->isEnable = isEnable;
}

inline void OneWireSlave::presence() {
	this->EXTI_InitStruct.EXTI_LineCmd = DISABLE;
	EXTI_Init(&EXTI_InitStruct);
	SET_PIN_MODE_OUT();
	SET_PIN_VALUE(false);
	delay_us(65);
	SET_PIN_MODE_IN();
	this->EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
}

inline void OneWireSlave::write_bit(bool bit)
{
	this->NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStruct);
	if (!bit)
	{
		SET_PIN_VALUE(bit);
		SET_PIN_MODE_OUT();
	}
}


void OneWireSlave::interrupt() {
	if (EXTI_GetITStatus(this->EXTI_InitStruct.EXTI_Line) != RESET) {
		if (this->isEnable)
		{
			bool statePin = GPIO_ReadInputDataBit(this->GPIO, this->GPIO_InitStructure.GPIO_Pin);
			if (!statePin) 
			{
				reset_delay();
				if (this->isMasterRead) 
				{
					if (this->isCmdReceive)
					{
						this->isWriteBit = true;
						uint8_t bytePosition = this->counterBitWrite / 8;
						uint8_t bitPosition = this->counterBitWrite % 8;
						bool sendBit = bool((1 << bitPosition)  &  this->ROM[bytePosition]);		
					
						switch (this->searchRomState)
						{
						case 0:
							this->write_bit(sendBit);
							break;
						case 1:
							this->write_bit(!sendBit);
							break;
						}
					}
				}
			}
			else 
			{
				uint32_t delay = get_delay();
				if (delay > 400) this->isMasterReset = true;
				else
				{
					bool reciveBit = ((delay < 20) ? 1 : 0);
				
					if (this->isCmdReceive)
					{
						if (!this->isMasterRead)
						{
							uint8_t bytePosition = this->counterBitWrite / 8;
							uint8_t bitPosition = this->counterBitWrite % 8;
							bool sendBit = ((1 << bitPosition)  &  this->ROM[bytePosition]);
							if (reciveBit == sendBit)
							{
								this->counterBitWrite++;
								this->isMasterRead = true;
							}
						}
					}
					else
					{
						this->reciveCMD |= (reciveBit << this->counterReciveCMD);
						if (this->counterReciveCMD < 7) this->counterReciveCMD++;
						else if (this->reciveCMD == 0xF0) 
						{
							this->isCmdReceive = true;
							this->isMasterRead = true;
						}
					}
				}
			}
		}
		EXTI_ClearITPendingBit(this->EXTI_InitStruct.EXTI_Line);
	}
}

uint8_t OneWireSlave::listener(void) {
	if (this->isMasterReset)
	{	
		this->reset();
		if(this->isEnable) this->presence();
	}
	if (this->isWriteBit)
	{
		this->isWriteBit = false;
		delay_us(20);
		SET_PIN_MODE_IN();
		this->NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStruct);
		if (searchRomState == 1)
		{
			searchRomState = 0;
			this->isMasterRead = false;
		}
		else
		{
			searchRomState = 1;
		}
	}
	return 0;
}
