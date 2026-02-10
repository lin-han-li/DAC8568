#include "Encoder.h"
#include "tim.h"


int32_t CNT=0;
int32_t LCNT=0;
int32_t Encoder_Get_Div4(void)
 
{		

LCNT=CNT;

CNT=__HAL_TIM_GET_COUNTER(&htim8)/2;

if(LCNT-CNT>10000)
{

return 1;

}

if(CNT-LCNT>10000)
{

return -1;

}
	/*除以4输出*/		
	if(CNT-LCNT)
	{
		 
		int32_t temp = (CNT-LCNT);
		
		LCNT = __HAL_TIM_GET_COUNTER(&htim8)/2;
		
		return temp;
	}
	return 0;
	
}

int16_t Encoder_Get(void)
{
	int16_t temp = __HAL_TIM_GET_COUNTER(&htim8);
__HAL_TIM_SetCounter(&htim8,0);
	return temp;
}



