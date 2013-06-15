/*
 * (C) Copyright 2011-2013
 * Emcraft Systems, <www.emcraft.com>
 * Yuri Tikhonov <yur@emcraft.com>
 * Alexander Potashev <aspotashev@emcraft.com>
 * Vladimir Khusainov <vlad@emcraft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <mach/iomux.h>
#include <mach/platform.h>
#include <mach/stm32.h>

/*
 * GPIO configuration mode
 */
#define STM32F2_GPIO_MODE_IN	0x00
#define STM32F2_GPIO_MODE_OUT	0x01
#define STM32F2_GPIO_MODE_AF	0x02
#define STM32F2_GPIO_MODE_AN	0x03

/*
 * GPIO output type
 */
#define STM32F2_GPIO_OTYPE_PP	0x00
#define STM32F2_GPIO_OTYPE_OD	0x01

/*
 * GPIO output maximum frequency
 */
#define STM32F2_GPIO_SPEED_2M	0x00
#define STM32F2_GPIO_SPEED_25M	0x01
#define STM32F2_GPIO_SPEED_50M	0x02
#define STM32F2_GPIO_SPEED_100M	0x03

/*
 * GPIO pullup, pulldown configuration
 */
#define STM32F2_GPIO_PUPD_NO	0x00
#define STM32F2_GPIO_PUPD_UP	0x01
#define STM32F2_GPIO_PUPD_DOWN	0x02

/*
 * AF5 selection
 */
#define STM32F2_GPIO_AF_SPI1	0x05
#define STM32F2_GPIO_AF_SPI2	0x05
#define STM32F2_GPIO_AF_SPI4	0x05
#define STM32F2_GPIO_AF_SPI5	0x05
#define STM32F2_GPIO_AF_SPI6	0x05

/*
 * AF6 selection
 */
#define STM32F2_GPIO_AF_SPI3	0x06

/*
 * AF7 selection
 */
#define STM32F2_GPIO_AF_USART1	0x07
#define STM32F2_GPIO_AF_USART2	0x07
#define STM32F2_GPIO_AF_USART3	0x07

/*
 * AF8 selection
 */
#define STM32F2_GPIO_AF_USART4	0x08
#define STM32F2_GPIO_AF_USART5	0x08
#define STM32F2_GPIO_AF_USART6	0x08

/*
 * AF11 selection
 */
#define STM32F2_GPIO_AF_MAC	0x0B

/*
 * AF12 selection
 */
#define STM32F2_GPIO_AF_FSMC	0x0C
#define STM32F2_GPIO_AF_SDIO	0x0C

/*
 * GPIO roles (alternative functions); role determines by whom GPIO is used
 */
enum stm32f2_gpio_role {
	STM32F2_GPIO_ROLE_USART1,	/* USART1			      */
	STM32F2_GPIO_ROLE_USART2,	/* USART2			      */
	STM32F2_GPIO_ROLE_USART3,	/* USART3			      */
	STM32F2_GPIO_ROLE_USART4,	/* USART4			      */
	STM32F2_GPIO_ROLE_USART5,	/* USART5			      */
	STM32F2_GPIO_ROLE_USART6,	/* USART6			      */
	STM32F2_GPIO_ROLE_ETHERNET,	/* MAC				      */
	STM32F2_GPIO_ROLE_SPI1,		/* SPI1				      */
	STM32F2_GPIO_ROLE_SPI2,		/* SPI2				      */
	STM32F2_GPIO_ROLE_SPI3,		/* SPI3				      */
	STM32F2_GPIO_ROLE_SPI4,		/* SPI4				      */
	STM32F2_GPIO_ROLE_SPI5,		/* SPI5				      */
	STM32F2_GPIO_ROLE_SPI6,		/* SPI6				      */
	STM32F2_GPIO_ROLE_SDIO,		/* SDIO				      */
	STM32F2_GPIO_ROLE_MCO,		/* MC external output clock	      */
	STM32F2_GPIO_ROLE_OUT,		/* General purpose output	      */
	STM32F2_GPIO_ROLE_IN		/* General purpose input	      */
};

/*
 * GPIO descriptor
 */
struct stm32f2_gpio_dsc {
	u32		port;		/* GPIO port			      */
	u32		pin;		/* GPIO pin			      */
};

/*
 * Register map bases
 */
static const unsigned long stm32_gpio_base[] = {
	STM32F2_GPIOA_BASE, STM32F2_GPIOB_BASE, STM32F2_GPIOC_BASE,
	STM32F2_GPIOD_BASE, STM32F2_GPIOE_BASE, STM32F2_GPIOF_BASE,
	STM32F2_GPIOG_BASE, STM32F2_GPIOH_BASE, STM32F2_GPIOI_BASE
};

/*
 * AF values (note, indexed by enum stm32f2_gpio_role)
 */
static const u32 af_val[] = {
	STM32F2_GPIO_AF_USART1, STM32F2_GPIO_AF_USART2, STM32F2_GPIO_AF_USART3,
	STM32F2_GPIO_AF_USART4, STM32F2_GPIO_AF_USART5, STM32F2_GPIO_AF_USART6,
	STM32F2_GPIO_AF_MAC, 
	STM32F2_GPIO_AF_SPI1, STM32F2_GPIO_AF_SPI2, STM32F2_GPIO_AF_SPI3,
	STM32F2_GPIO_AF_SPI4, STM32F2_GPIO_AF_SPI5, STM32F2_GPIO_AF_SPI6,
	STM32F2_GPIO_AF_SDIO,
	0
};

/*
 * Configure the specified GPIO for the specified role
 */
#ifndef CONFIG_ARCH_STM32F1
static int stm32f2_gpio_config(struct stm32f2_gpio_dsc *dsc,
			       enum stm32f2_gpio_role role)
{
	volatile struct stm32f2_gpio_regs	*gpio_regs;

	u32	otype, ospeed, pupd, mode, i;
	int	rv;

	/*
	 * Check params
	 */
	if (!dsc || dsc->port > 8 || dsc->pin > 15) {
		rv = -EINVAL;
		goto out;
	}

	/*
	 * Depending on the role, select the appropriate io params
	 */
	switch (role) {
	case STM32F2_GPIO_ROLE_USART1:
	case STM32F2_GPIO_ROLE_USART2:
	case STM32F2_GPIO_ROLE_USART3:
	case STM32F2_GPIO_ROLE_USART4:
	case STM32F2_GPIO_ROLE_USART5:
	case STM32F2_GPIO_ROLE_USART6:
	case STM32F2_GPIO_ROLE_SPI1:
	case STM32F2_GPIO_ROLE_SPI2:
	case STM32F2_GPIO_ROLE_SPI3:
	case STM32F2_GPIO_ROLE_SPI4:
	case STM32F2_GPIO_ROLE_SPI5:
	case STM32F2_GPIO_ROLE_SPI6:
		otype  = STM32F2_GPIO_OTYPE_PP;
		ospeed = STM32F2_GPIO_SPEED_50M;
		pupd   = STM32F2_GPIO_PUPD_UP;
		break;
	case STM32F2_GPIO_ROLE_ETHERNET:
	case STM32F2_GPIO_ROLE_MCO:
		otype  = STM32F2_GPIO_OTYPE_PP;
		ospeed = STM32F2_GPIO_SPEED_100M;
		pupd   = STM32F2_GPIO_PUPD_NO;
		break;
	case STM32F2_GPIO_ROLE_SDIO:
		otype  = STM32F2_GPIO_OTYPE_PP;
		ospeed = STM32F2_GPIO_SPEED_50M;
		pupd   = STM32F2_GPIO_PUPD_NO;
		break;
	case STM32F2_GPIO_ROLE_OUT:
		otype  = STM32F2_GPIO_OTYPE_PP;
		ospeed = STM32F2_GPIO_SPEED_50M;
		pupd   = STM32F2_GPIO_PUPD_NO;
		break;
	default:
		rv = -EINVAL;
		goto out;
	}

	/*
	 * Get reg base
	 */
	gpio_regs = (struct stm32f2_gpio_regs *)stm32_gpio_base[dsc->port];

	/*
	 * Enable GPIO clocks
	 */
	STM32_RCC->ahb1enr |= 1 << dsc->port;

	if (role != STM32F2_GPIO_ROLE_MCO &&
	    role != STM32F2_GPIO_ROLE_OUT &&
	    role != STM32F2_GPIO_ROLE_IN) {

		/*
		 * Connect PXy to the specified controller (role)
		 */
		i = (dsc->pin & 0x07) * 4;
		gpio_regs->afr[dsc->pin >> 3] &= ~(0xF << i);
		gpio_regs->afr[dsc->pin >> 3] |= af_val[role] << i;
	}

	i = dsc->pin;

	/*
	 * Output mode configuration
	 */
	gpio_regs->otyper &= ~(0x1 << i);
	gpio_regs->otyper |= otype << i;

	i = dsc->pin * 2;

	/*
	 * Set mode
	 */
	if (role == STM32F2_GPIO_ROLE_OUT) {
		mode = STM32F2_GPIO_MODE_OUT;
	}
	else if (role == STM32F2_GPIO_ROLE_IN) {
		mode = STM32F2_GPIO_MODE_IN;
	}
	else {
		mode = STM32F2_GPIO_MODE_AF;
	}
	gpio_regs->moder &= ~(0x3 << i);
	gpio_regs->moder |= mode << i;

	/*
	 * Speed mode configuration
	 */
	gpio_regs->ospeedr &= ~(0x3 << i);
	gpio_regs->ospeedr |= ospeed << i;

	/*
	 * Pull-up, pull-down resistor configuration
	 */
	gpio_regs->pupdr &= ~(0x3 << i);
	gpio_regs->pupdr |= pupd << i;

	rv = 0;
out:
	return rv;
}
#endif /* !CONFIG_ARCH_STM32F1 */

/*
 * Initialize the GPIO Alternative Functions of the STM32.
 */
void __init stm32_iomux_init(void)
{
	struct stm32f2_gpio_dsc		gpio_dsc;
	int				platform;

	/*
	 * Configure IOs depending on the board we're running on, and
	 * the configuration options we're using.
	 * Let's control platform strictly: if some of it does not need to
	 * play with iomux, it must be present in switch below (otherwise,
	 * the warning message will be printed-out)
	 */
	platform = stm32_platform_get();
	switch (platform) {
#ifndef CONFIG_ARCH_STM32F1
	/* STM32F2-based platforms */
	case PLATFORM_STM32_STM3220G_EVAL:
	case PLATFORM_STM32_STM3240G_EVAL:
	case PLATFORM_STM32_STM_SOM:
#if defined(CONFIG_STM32_USART1)
		gpio_dsc.port = 0;
		gpio_dsc.pin  = 9;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_USART1);

		gpio_dsc.port = 0;
		gpio_dsc.pin  = 10;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_USART1);
#endif
#if defined(CONFIG_STM32_USART3)
		gpio_dsc.port = 2;
		gpio_dsc.pin  = 10;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_USART3);

		gpio_dsc.port = 2;
		gpio_dsc.pin  = 11;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_USART3);
#endif
#if defined(CONFIG_STM32_MAC)
		do {
			static struct stm32f2_gpio_dsc mii_gpio[] = {
				{0,  1}, {0,  2}, {0,  7},
				{1,  5}, {1,  8},
				{2,  1}, {2,  2}, {2,  3}, {2,  4}, {2,  5},
				{6, 11}, {6, 13}, {6, 14},
				{7,  2}, {7,  3}, {7,  6}, {7,  7},
				{8, 10}
			};
			static struct stm32f2_gpio_dsc rmii_gpio[] = {
				{0,  1}, {0,  2}, {0,  7},
				{2,  1}, {2,  4}, {2,  5},
				{6, 11}, {6, 13}, {6, 14},
			};
			int	i;

			if (platform == PLATFORM_STM32_STM_SOM) {
				for (i = 0; i < ARRAY_SIZE(rmii_gpio); i++) {
					stm32f2_gpio_config(&rmii_gpio[i],
						STM32F2_GPIO_ROLE_ETHERNET);
				}
			} else {
				for (i = 0; i < ARRAY_SIZE(mii_gpio); i++) {
					stm32f2_gpio_config(&mii_gpio[i],
						STM32F2_GPIO_ROLE_ETHERNET);
				}
			}
		} while (0);
#endif
#if defined(CONFIG_STM32_SPI1)
#error		IOMUX for STM32_SPI1 undefined
#endif
#if defined(CONFIG_STM32_SPI2)
		gpio_dsc.port = 8;	/* CLCK */
		gpio_dsc.pin  = 1;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_SPI2);

		gpio_dsc.port = 1;	/* DI */
		gpio_dsc.pin  = 14;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_SPI2);

		gpio_dsc.port = 1;	/* DO */
		gpio_dsc.pin  = 15;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_SPI2);

		gpio_dsc.port = 1;	/* CS */
		gpio_dsc.pin  = 9;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_OUT);
#endif
#if defined(CONFIG_STM32_SPI3)
#error		IOMUX for STM32_SPI3 undefined
#endif
#if defined(CONFIG_STM32_SPI4)
#error		IOMUX for STM32_SPI4 undefined
#endif
#if defined(CONFIG_STM32_SPI5)
		gpio_dsc.port = 7;	/* CLCK */
		gpio_dsc.pin  = 6;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_SPI5);

		gpio_dsc.port = 5;	/* DI */
		gpio_dsc.pin  = 8;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_SPI5);

		gpio_dsc.port = 5;	/* DO */
		gpio_dsc.pin  = 9;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_SPI5);

		gpio_dsc.port = 7;	/* CS */
		gpio_dsc.pin  = 5;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_OUT);
#endif
#if defined(CONFIG_STM32_SPI6)
#error		IOMUX for STM32_SPI6 undefined
#endif
#if defined(CONFIG_MMC_ARMMMCI) || defined(CONFIG_MMC_ARMMMCI_MODULE)
		do {
			static struct stm32f2_gpio_dsc sdcard_gpio[] = {
				{2,  8}, {2,  9}, {2, 10}, {2, 11},
				{2, 12}, {3,  2}
			};
			int	i;

			for (i = 0; i < ARRAY_SIZE(sdcard_gpio); i++) {
				stm32f2_gpio_config(&sdcard_gpio[i],
						    STM32F2_GPIO_ROLE_SDIO);
			}
		} while (0);
#endif /* CONFIG_MMC_ARMMMCI */

#if defined(CONFIG_GPIOLIB) && defined(CONFIG_GPIO_SYSFS)

	/*
	 * Pin configuration for the User LED of the SOM-BSB-EXT baseboard.
	 * !!! That GPIO may have other connections on other baseboards.
	 */
	if (platform == PLATFORM_STM32_STM_SOM) {
		/* PB2 = LED DS4 */
		gpio_dsc.port = 1;
		gpio_dsc.pin  = 2;
		stm32f2_gpio_config(&gpio_dsc, STM32F2_GPIO_ROLE_OUT);
	}

#endif /* CONFIG_GPIOLIB */

		break;
#else
	/* STM32F1-based platforms */
	case PLATFORM_STM32_SWISSEMBEDDED_COMM:
		/*
		 * Rely on the IOMUX configuration initialized by the bootloader
		 */
		break;
#endif
	default:
		printk(KERN_WARNING "%s: unsupported platform %d\n", __func__,
			platform);
		/*
		 * Just to avoid compilation warns in case of configuration
		 * which doesn't need iomux, 'use' gpio_dsc var
		 */
		gpio_dsc.port = gpio_dsc.pin = 0;
		break;
	}
}
