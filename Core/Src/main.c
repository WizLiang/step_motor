/* USER CODE BEGIN Header */
/***************************************************************************//**
  �ļ�: main.c
  ����: Zhengyu https://gzwelink.taobao.com
  �汾: V1.0.0
  ʱ��: 202101201
	ƽ̨:MINI-STM32F103RCT6������
	΢�ź�:gzwelink

*******************************************************************************/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "oled.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define uchar unsigned char
#define uint  unsigned int

typedef unsigned char u8;
typedef unsigned long u32;
void Init18b20 (void);
void WriteByte (unsigned char wr);  //���ֽ�д��
void read_bytes (unsigned char j);  //���ֽڶ�ȡ
unsigned char Temp_CRC (unsigned char j);//����CRC
void GemTemp (void);//��ȡ�¶�
void Config18b20 (void);//����18B20
void ReadID (void);//��ȡID
void TemperatuerResult(void);
void SystemClock_Config(void);
unsigned long Count;

const uint16_t  phasecw[4] ={GPIO_PIN_9,GPIO_PIN_8,GPIO_PIN_7,GPIO_PIN_6};//��ת�����ͨ����D-C-B-A
uchar phaseccw[4]={GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_5};//��ת�����ͨ����A-B-C-D
//�ȴ�us��




// 定义新的步进电机控制引脚
#define STEP_PIN1 GPIO_PIN_6 // PC6
#define STEP_PIN2 GPIO_PIN_7 // PC7
#define STEP_PIN3 GPIO_PIN_8 // PC8
#define STEP_PIN4 GPIO_PIN_9 // PC9
#define STEP_GPIO_PORT GPIOC

// 步进序列定义
const uint16_t step_sequence[4] = {
    STEP_PIN1, // Step 1: PC6
    STEP_PIN2, // Step 2: PC7
    STEP_PIN3, // Step 3: PC8
    STEP_PIN4  // Step 4: PC9
};

/* Private variables ---------------------------------------------------------*/
typedef enum {
    STEP_1 = 0,
    STEP_2,
    STEP_3,
    STEP_4
} StepSequence;

StepSequence current_step = STEP_1;
volatile uint32_t step_count = 0;
volatile uint8_t direction = 1; // 1: 顺时针, 0: 逆时针
unsigned char  flag;
unsigned int   Temperature;//�¶�
unsigned int	 temp;
unsigned char  temp_buff[9]; //�洢��ȡ���ֽڣ�read scratchpadΪ9�ֽڣ�read rom IDΪ8�ֽ�
unsigned char  id_buff[8];
unsigned char  *p;
unsigned char  crc_data;



void UpdateStepper(void) {
//    if(direction) {
//        current_step = (current_step + 1) % 4;
//    } else {
//        current_step = (current_step == 0) ? 3 : current_step - 1;
//    }
    
	
	  if(direction) {
        current_step = (StepSequence)(((int)current_step + 1) % 4);
    } else {
        current_step = (current_step == STEP_1) ? STEP_4 : (StepSequence)(current_step - 1);
    }
		if( 25 <= Temperature){
    // 设置对应的引脚高电平，其余低电平
    HAL_GPIO_WritePin(STEP_GPIO_PORT, STEP_PIN1 | STEP_PIN2 | STEP_PIN3 | STEP_PIN4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_GPIO_PORT, step_sequence[current_step], GPIO_PIN_SET);
		}
    
    step_count++;
    
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if(htim->Instance == TIM3) { // 使用 TIM3
        UpdateStepper();
    }
}

void AdjustStepperSpeed(void) {
    // 映射温度到定时器ARR范围
    // 18°C -> ARR = 1000 (较慢)
    // 30°C -> ARR = 200 (较快)
    
    
    uint16_t min_temp = 18;
    uint16_t max_temp = 33;
    uint16_t min_arr = 500;
    uint16_t max_arr = 10000;
    
    if(Temperature < min_temp) Temperature = min_temp;
    if(Temperature > max_temp) Temperature = max_temp;
    
    // 线性映射
    uint32_t desired_arr = max_arr - ((Temperature - min_temp) * (max_arr - min_arr)) / (max_temp - min_temp);
    
//    // 获取当前ARR值
//    uint32_t current_arr = __HAL_TIM_GET_AUTORELOAD(&htim3);
//    
//    // 平滑调整ARR值，避免频率突变
//    if(current_arr < desired_arr) {
//        current_arr += 1; // 增加ARR，减慢频率
//    }
//    else if(current_arr > desired_arr) {
//        current_arr -= 1; // 减少ARR，增快频率
//    }
    
    // 更新定时器ARR
//    __HAL_TIM_SET_AUTORELOAD(&htim3, current_arr);
		__HAL_TIM_SET_AUTORELOAD(&htim3, desired_arr);
    __HAL_TIM_SET_COUNTER(&htim3, 0); // 重置计数器
}

void Delay_us(unsigned long i)
{
	unsigned long j;
	for(;i>0;i--)
	{
			for(j=12;j>0;j--);
	}
}

//��ת
void MotorCW(void)
{
 uchar i;
 temp = 30-Temperature;
 for(i=0;i<4;i++)
  {

		HAL_GPIO_WritePin(GPIOC,phasecw[i],GPIO_PIN_SET);//�����
    HAL_Delay((Temperature<30) ? temp : 1);//�ȴ�5ms
		HAL_GPIO_WritePin(GPIOC,phasecw[i],GPIO_PIN_RESET);//�����
  }
}


//CRC���
const unsigned char  CrcTable [256]={
0,  94, 188,  226,  97,  63,  221,  131,  194,  156,  126,  32,  163,  253,  31,  65,
157,  195,  33,  127,  252,  162,  64,  30,  95,  1,  227,  189,  62,  96,  130,  220,
35,  125,  159,  193,  66,  28,  254,  160,  225,  191,  93,  3,  128,  222,  60,  98,
190,  224,  2,  92,  223,  129,  99,  61,  124,  34,  192,  158,  29,  67,  161,  255,
70,  24,  250,  164,  39,  121,  155,  197,  132,  218,  56,  102,  229,  187,  89,  7,
219,  133, 103,  57,  186,  228,  6,  88,  25,  71,  165,  251,  120,  38,  196,  154,
101,  59, 217,  135,  4,  90,  184,  230,  167,  249,  27,  69,  198,  152,  122,  36,
248,  166, 68,  26,  153,  199,  37,  123,  58,  100,  134,  216,  91,  5,  231,  185,
140,  210, 48,  110,  237,  179,  81,  15,  78,  16,  242,  172,  47,  113,  147,  205,
17,  79,  173,  243,  112,  46,  204,  146,  211,  141,  111,  49,  178,  236,  14,  80,
175,  241, 19,  77,  206,  144,  114,  44,  109,  51,  209,  143,  12,  82,  176,  238,
50,  108,  142,  208,  83,  13,  239,  177,  240,  174,  76,  18,  145,  207,  45,  115,
202,  148, 118,  40,  171,  245,  23,  73,  8,  86,  180,  234,  105,  55,  213, 139,
87,  9,  235,  181,  54,  104,  138,  212,  149,  203,  41,  119,  244,  170,  72,  22,
233,  183,  85,  11,  136,  214,  52,  106,  43,  117,  151,  201,  74,  20,  246,  168,
116,  42,  200,  150,  21,  75,  169,  247,  182,  232,  10,  84,  215,  137,  107,  53}; 


/************************************************************
*Function:18B20��ʼ��
*parameter:
*Return:
*Modify:
*************************************************************/
void Init18b20 (void)
{

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_SET);
	Delay_us(2); //��ʱ2΢��
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_RESET);
	Delay_us(490);   //delay 530 uS//80
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_RESET);
	Delay_us(100);   //delay 100 uS//14
	if(HAL_GPIO_ReadPin(GPIOA ,GPIO_PIN_0)== 0)
		flag = 1;   //detect 1820 success!
	else
		flag = 0;    //detect 1820 fail!
	Delay_us(480);        //��ʱ480΢��
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_SET);
}

/************************************************************
*Function:��18B20д��һ���ֽ�
*parameter:
*Return:
*Modify:
*************************************************************/
void WriteByte (unsigned char  wr)  //���ֽ�д��
{
	unsigned char  i;
	for (i=0;i<8;i++)
	{
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_RESET);
    Delay_us(2);
		if(wr&0x01)	 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_SET);
		else   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_RESET);
		Delay_us(45);   //delay 45 uS //5
		
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_SET);
		wr >>= 1;
	}
}

/************************************************************
*Function:��18B20��һ���ֽ�
*parameter:
*Return:
*Modify:
*************************************************************/
unsigned char ReadByte (void)     //��ȡ���ֽ�
{
	unsigned char  i,u=0;
	for(i=0;i<8;i++)
	{		
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_RESET);
		Delay_us (2);
		u >>= 1;
	
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,GPIO_PIN_SET);
		Delay_us (4);
		if(HAL_GPIO_ReadPin(GPIOA , GPIO_PIN_0) == 1)
		u |= 0x80;
		Delay_us (65);
	}
	return(u);
}

/************************************************************
*Function:��18B20
*parameter:
*Return:
*Modify:
*************************************************************/
void read_bytes (unsigned char  j)
{
	 unsigned char  i;
	 for(i=0;i<j;i++)
	 {
		  *p = ReadByte();
		  p++;
	 }
}

/************************************************************
*Function:CRCУ��
*parameter:
*Return:
*Modify:
*************************************************************/
unsigned char Temp_CRC (unsigned char j)
{
   	unsigned char  i,crc_data=0;
  	for(i=0;i<j;i++)  //���У��
    	crc_data = CrcTable[crc_data^temp_buff[i]];
    return (crc_data);
}

/************************************************************
*Function:��ȡ�¶�
*parameter:
*Return:
*Modify:
*************************************************************/
void GemTemp (void)
{
   read_bytes (9);
   if (Temp_CRC(9)==0) //У����ȷ
   {
	  Temperature = temp_buff[1]*0x100 + temp_buff[0];
		Temperature /= 16;
		Delay_us(10);
    }
}

/************************************************************
*Function:�ڲ�����
*parameter:
*Return:
*Modify:
*************************************************************/
void Config18b20 (void)  //�������ñ����޶�ֵ�ͷֱ���
{
     Init18b20();
     WriteByte(0xcc);  //skip rom
     WriteByte(0x4e);  //write scratchpad
     WriteByte(0x19);  //����
     WriteByte(0x1a);  //����
     WriteByte(0x7f);     //set 12 bit (0.125)
     Init18b20();
     WriteByte(0xcc);  //skip rom
     WriteByte(0x48);  //�����趨ֵ
     Init18b20();
     WriteByte(0xcc);  //skip rom
     WriteByte(0xb8);  //�ص��趨ֵ
}

/************************************************************
*Function:��18B20ID
*parameter:
*Return:
*Modify:
*************************************************************/
void ReadID (void)//��ȡ���� id
{
	Init18b20();
	WriteByte(0x33);  //read rom
	read_bytes(8);
}

/************************************************************
*Function:18B20IDȫ����
*parameter:
*Return:
*Modify:
*************************************************************/
void TemperatuerResult(void)
{
  	p = id_buff;
  	ReadID();
  Config18b20();
	Init18b20 ();
	HAL_Delay(20);
	WriteByte(0xcc);   //skip rom
	WriteByte(0x44);   //Temperature convert

	Init18b20 ();
	HAL_Delay(20);
	WriteByte(0xcc);   //skip rom
	WriteByte(0xbe);   //read Temperature
	p = temp_buff;
	GemTemp();
}


void GetTemp()
{       
   if(Count == 2) //ÿ��һ��ʱ���ȡ�¶�
	{  
		 Count=0;
	   TemperatuerResult();
	}
	Count++;

}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	//uint16_t i;
	unsigned int cnt=0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();//OLED��ʼ��  
	OLED_Clear();//����
	
	OLED_ShowString(0, 0, "The temperature is ");
	GetTemp();               //��ȡ�¶�
	GetTemp();     

  /* 启动定时器中断 */
  HAL_TIM_Base_Start_IT(&htim3);
    
  /* 初始步进电机状态 */
  step_count = 0;
  direction = 1;//�ٴλ�ȡ�¶�
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
while(1){
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
//		for(int i = 0;i <10;i++)
//		MotorCW();   //��ת
    AdjustStepperSpeed();    // 根据温度调整步进频率
		break;

		//HAL_Delay(1000);//�ȴ�1S 
  }
	uint cnt_div;
	while(1){
 		GetTemp();		//�ٴλ�ȡ�¶�
	  //OLED_Clear();            //OLED����  
	  OLED_ShowNum(100,6,Temperature,3,16);//��ʾ�¶�
		cnt_div= (Temperature > 24 && Temperature <30)?(50 -Temperature): 10 ;
		if(Temperature<=24 || (cnt++)%5000==0 )
		OLED_Fill(100, 6, 127, 7, 0);
		break;
	}
}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
