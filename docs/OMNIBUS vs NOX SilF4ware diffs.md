OMNIBUS vs NOX SilF4ware diffs

C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Inc\main.h
	GPIO pin defines

C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Inc\stm32f4xx_it.h
	DMA stream used
	OMNIBUS
		void DMA1_Stream4_IRQHandler(void);
	NOX
		void DMA1_Stream6_IRQHandler(void);

C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Src\blackbox.c
	OMNIBUS
		extern void MX_UART4_Init( void );
		MX_UART4_Init();

		extern UART_HandleTypeDef huart4;
		// HAL_UART_IRQHandler( &huart4 ); // Resets huart->gState to HAL_UART_STATE_READY
		huart4.gState = HAL_UART_STATE_READY; // Do it directly to save flash space.
		HAL_UART_Transmit_DMA( &huart4, bb_buffer, sizeof( bb_buffer ) );

	NOX
		extern void MX_USART2_UART_Init( void );
		MX_USART2_UART_Init();

		extern UART_HandleTypeDef huart2;
		// HAL_UART_IRQHandler( &huart2 ); // Resets huart->gState to HAL_UART_STATE_READY
		huart2.gState = HAL_UART_STATE_READY; // Do it directly to save flash space.
		HAL_UART_Transmit_DMA( &huart2, bb_buffer, sizeof( bb_buffer ) );

C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Src\config.h
	OMNIBUS
		#define SENSOR_ROTATE_90_CCW
	NOX
		#define SENSOR_ROTATE_180


C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Src\main.c
	Many changes regarding

	* UART/USART used (for blackbox logging)
		OMNIBUS - UART4
		NOX - USART2

	* Clock config
		OMNIBUS - 168Mhz
		NOX - 100 Mhz....but I need to change this to 96Mhz for USB VCP

	* ADC channel
		OMNIBUS 12
		NOX 5

	* TIM2
		OMNIBUS Prescaler set to 84
		NOX Prescaler set to 100 , but I'll need to change this to 96 due to clock config change

	* DMA set IRQ priority and enables IRQ
		OMNIBUS Uses DMA1_Stream4
		OMNIBUS Uses DMA1_Stream6

	* GPIO setup

C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Src\stm32f4xx_hal_msp.c

	MSP - MCU Specific Package
	This is the processor (and peripheral/resource) specific initialization code

C:\Projects\Hobbies\RC\STM32F405-Omnibus\Core\Src\stm32f4xx_it.c

	These are the interrupt service routines

Drivers folder...
	OMNIBUS has this file
	C:\Projects\Hobbies\RC\STM32F405-Omnibus\Drivers\CMSIS\Device\ST\STM32F4xx\Include\stm32f405xx.h
	NOX has this file
	C:\Projects\Hobbies\RC\STM32F411-NOXE\Drivers\CMSIS\Device\ST\STM32F4xx\Include\stm32f411xe.h

Core/Inc/
	main.h
	stm32f4xx_it.h

	