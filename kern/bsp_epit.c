#include "imx6/imx6ul.h"
#include <inc/assert.h>
void epit1_init(unsigned int ms)
{
	#define ipg_clk 66000000 // 66M
	#define frac 100
	assert(ms >= 1 && ms <= 5000);
	EPIT1->CR = 0;
	EPIT1->CR = (1<<24 | frac << 4 | 1<<3 | 1<<2 | 1<<1);
	
	unsigned int value = (ipg_clk / frac / 1000) * ms ;
	EPIT1->LR = value;
	EPIT1->CMPR	= 0;	/* 比较寄存器，当计数器值和此寄存器值相等的话就会产生中断 */

	/* 使能GIC中对应的中断 			*/
	GIC_EnableIRQ(EPIT1_IRQn);

	/* 注册中断服务函数 			*/
	// system_register_irqhandler(EPIT1_IRQn, (system_irq_handler_t)epit1_irqhandler, NULL);	

	EPIT1->CR |= 1<<0;	/* 使能EPIT1 */ 
}

/*
 * @description			: EPIT中断处理函数
 * @param				: 无
 * @return 				: 无
 */
// void epit1_irqhandler(void)
// { 
// 	static unsigned char state = 0;

// 	state = !state;
// 	if(EPIT1->SR & (1<<0)) 			/* 判断比较事件发生 */
// 	{
// 		led_switch(LED0, state); 	/* 定时器周期到，反转LED */
// 	}
	
// 	EPIT1->SR |= 1<<0; 				/* 清除中断标志位 */
// }

