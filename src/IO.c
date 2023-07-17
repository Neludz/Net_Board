
#include <stdint.h>
#include <stdbool.h>

#include <stm32f1xx.h>
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_usart.h>
#include <IO.h>
#include <main.h>
#include <modbus_hard.h>
//--------------X macros---------------------------------------------------------
const tGPIO_Line IOs[NUM_IO] =
{
#define X_IO(a,b,c,d,e,f,g)	{b,c,d+e,f,g},
    IO_TABLE
#undef X_IO
};

//---------------------------------------------------------------------------------
void IO_SetLine(tIOLine Line, bool State)
{
    if (State)
        IOs[Line].GPIOx->BSRR = 1 << (IOs[Line].GPIO_Pin);
    else
        IOs[Line].GPIOx->BRR = 1 << (IOs[Line].GPIO_Pin);
}
//---------------------------------------------------------------------------------
bool IO_GetLine(tIOLine Line)
{
    if (Line < NUM_IO)
        return (((IOs[Line].GPIOx->IDR) & (1<<(IOs[Line].GPIO_Pin))) != 0);
    else
        return false;
}

//---------------------------------------------------------------------------------
bool IO_GetLineActive(tIOLine Line)
{
    if (Line < NUM_IO)
    {
        bool pin_set = (((IOs[Line].GPIOx->IDR) & (1<<(IOs[Line].GPIO_Pin))) ? true : false);
        return (pin_set == ( IOs[Line].ActiveState ? true : false));
    }
    else
        return false;
}
//---------------------------------------------------------------------------------
void IO_SetLineActive(tIOLine Line, bool State)
{
    if (State ^ IOs[Line].ActiveState)
    {
        IOs[Line].GPIOx->BRR = 1 << (IOs[Line].GPIO_Pin);   //reset
    }
    else
    {
        IOs[Line].GPIOx->BSRR = 1 << (IOs[Line].GPIO_Pin);  //set
    }
}

//---------------------------------------------------------------------------------
void IO_ConfigLine(tIOLine Line, uint8_t Mode, uint8_t State)
{
    if(IOs[Line].GPIO_Pin < 8)
    {
        IOs[Line].GPIOx->CRL &=   ~(0x0F << (IOs[Line].GPIO_Pin * 4));
        IOs[Line].GPIOx->CRL |=  Mode<<(IOs[Line].GPIO_Pin * 4);
    }
    else
    {
        IOs[Line].GPIOx->CRH &=   ~(0x0F << ((IOs[Line].GPIO_Pin - 8)* 4));
        IOs[Line].GPIOx->CRH |=    Mode<<((IOs[Line].GPIO_Pin - 8)* 4);
    }

    IOs[Line].GPIOx->ODR &= ~(1<<IOs[Line].GPIO_Pin);
    IOs[Line].GPIOx->ODR |= State<<IOs[Line].GPIO_Pin;
}
//---------------------------------------------------------------------------------
void IO_Init(void)
{
//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    RCC->APB2ENR	|= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR 	|= RCC_APB2ENR_IOPBEN;
//RCC->APB2ENR	|= RCC_APB2ENR_IOPCEN;
//RCC->APB2ENR	|= RCC_APB2ENR_IOPDEN;
//RCC->APB2ENR	|= RCC_APB2ENR_IOPEEN;
//RCC->APB2ENR	|= RCC_APB2ENR_IOPFEN;
//RCC->APB2ENR	|= RCC_APB2ENR_IOPGEN;

    RCC->APB2ENR	|= RCC_APB2ENR_AFIOEN;

// for PA15
 AFIO->MAPR|=AFIO_MAPR_SWJ_CFG_JTAGDISABLE;


// Set all pins
    for (int Line = 0; Line < NUM_IO; Line++)
    {
        IO_ConfigLine(Line, IOs[Line].MODE, IOs[Line].DefState);
    }
}

//---------------------------------------------------------------------------------
//-------------------------------------------------------------------------
void flash_btock(void)
{
    if (!(FLASH->OBR & FLASH_OBR_RDPRT))
    {
        mh_Factory();	//when first start -> set MBbuf

        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;

        FLASH->OPTKEYR = FLASH_KEY1;
        FLASH->OPTKEYR = FLASH_KEY2;
        FLASH->CR |= FLASH_CR_OPTER;
        FLASH->CR|= FLASH_CR_STRT;
        while ((FLASH->SR & FLASH_SR_BSY) != 0 );

        FLASH->CR |= FLASH_CR_LOCK;
    }
}
//-------------------------------------------------------------------------
void IO_Init_IWDG(uint16_t tw) // Параметр tw от 7мс до 26200мс
{
// Для IWDG_PR=7 Tmin=6,4мс RLR=Tмс*40/256
    IWDG->KR=0x5555; // Ключ для доступа к таймеру
    IWDG->PR=7; // Обновление IWDG_PR
    IWDG->RLR=tw*40/256; // Загрузить регистр перезагрузки
    IWDG->KR=0xAAAA; // Перезагрузка
    IWDG->KR=0xCCCC; // Пуск таймера
}
//-------------------------------------------------------------------------
// Функция перезагрузки сторожевого таймера IWDG
void IO_IWDG_res(void)
{
    IWDG->KR=0xAAAA; // Перезагрузка
}



