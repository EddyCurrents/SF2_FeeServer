/*
 * Copyright (C) 2010,2011 Sergei Poselenov, Emcraft Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mdio-bitbang.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/dma-mapping.h>
#include <linux/ip.h>
#include <linux/icmp.h>

#include <asm/io.h>
#include <asm/setup.h>

#if defined(CONFIG_CORE10100_BUF_IN_ESRAM)
/* 
 * Use the SmartFusion embedded RAM for descriptors and buffers
 */
#define USE_INTRAM	(0x20000000 + CONFIG_CORE10100_BUF_IN_ESRAM_BASE)

#else
/*
 * Use the external RAM for descriptors and buffer
 */
#undef USE_INTRAM
#endif

#undef DEBUG /* define to enable debugging msgs */
#include "core10100.h"

MODULE_LICENSE("GPL");

/* Set the Management Data Clock high if level is one,
 * low if level is zero.
 */
void set_mdc(struct mdiobb_ctrl *ctrl, int level)
{
	struct core10100_dev *bp = container_of(ctrl, struct core10100_dev,
						core10100_mdio_ctrl);
	
	mii_set_mdc(level);
}

/* Configure the Management Data I/O pin as an input if
 * "output" is zero, or an output if "output" is one.
 */
void set_mdio_dir(struct mdiobb_ctrl *ctrl, int output)
{
	struct core10100_dev *bp = container_of(ctrl, struct core10100_dev,
						core10100_mdio_ctrl);
	if (output) 
		write_reg(CSR9, read_reg(CSR9) | CSR9_MDEN);
	else 
		write_reg(CSR9, read_reg(CSR9) & ~CSR9_MDEN);
}

/* Set the Management Data I/O pin high if value is one,
 * low if "value" is zero.  This may only be called
 * when the MDIO pin is configured as an output.
 */
void set_mdio_data(struct mdiobb_ctrl *ctrl, int value)
{
	struct core10100_dev *bp = container_of(ctrl, struct core10100_dev,
						core10100_mdio_ctrl);
	
	mii_set_mdio(value);
}

/* Retrieve the state Management Data I/O pin. */
int get_mdio_data(struct mdiobb_ctrl *ctrl)
{
	struct core10100_dev *bp = container_of(ctrl, struct core10100_dev,
						core10100_mdio_ctrl);

	return (mii_get_mdio() != 0);
}

struct mdiobb_ops  core10100_mdio_ops = {
	.owner = THIS_MODULE,
	.set_mdc       = set_mdc,
	.set_mdio_dir  = set_mdio_dir,
	.set_mdio_data = set_mdio_data,
	.get_mdio_data = get_mdio_data
};


/* Stop transmission and receiving */
static void stop_tx_rx(	struct core10100_dev *bp)
{
	int i;

	/* Stop transmission and receiving */
	write_reg(CSR6, read_reg(CSR6) & ~(CSR6_ST | CSR6_SR));

	/* Wait for the transmission and receiving processes stopped */
	for (i = 0; i < TIMEOUT_LOOPS; i++) {
		if (((read_reg(CSR5) >> CSR5_TS_SHIFT) & CSR5_TS_MASK) ==
			CSR5_TS_STOP &&
			((read_reg(CSR5) >> CSR5_RS_SHIFT) & CSR5_RS_MASK) ==
			CSR5_RS_STOP) {
			break;
		}
		udelay(TIMEOUT_UDELAY);
		/* WDT_RESET; */
	}
	write_reg(CSR5, (CSR5_TPS | CSR5_RPS));
	debug("%s: stopped TX RX\n", __func__);
}



static void core10100_adjust_link(struct net_device *dev)
{
	struct core10100_dev *bp = netdev_priv(dev);
	struct phy_device *phydev = bp->phy_dev;

	u8 tx_rx_stopped = 0;
	u32 status_change = 0;
	unsigned long flags;
	/* u8  link_stat; */

	spin_lock_irqsave(&bp->lock, flags);

	if (phydev->link) {

		/* Check whether link duplex has been changed */
		if (bp->duplex != phydev->duplex) {

			/* TX/RX have to be stopped for updating */
			if (bp->flags & TX_RX_ENABLED) {
				stop_tx_rx(bp);
				tx_rx_stopped = 1;
			}
			
			if (phydev->duplex) {
				bp->flags |= LINK_FD;
				write_reg(CSR6, read_reg(CSR6) | CSR6_FD);
			}
			else {
				bp->flags &= ~LINK_FD;
				write_reg(CSR6, read_reg(CSR6) & ~CSR6_FD);
			}
			
			bp->duplex = phydev->duplex;
			status_change = 1;
		}
		
		/* Check whether link speed has been changed */
		if (bp->speed != phydev->speed) {
			/* TX/RX have to be stopped for updating */
			if (tx_rx_stopped != 1 && 
				bp->flags & TX_RX_ENABLED) {
				stop_tx_rx(bp);
				tx_rx_stopped = 1;
			}

			/* Update MAC register */
			if (phydev->speed == 100) {
				bp->flags |= LINK_100;
				write_reg(CSR6, read_reg(CSR6) | CSR6_TTM);
			} else if (phydev->speed == 10) {
				bp->flags &= ~LINK_100;
				write_reg(CSR6, read_reg(CSR6) & ~CSR6_TTM);
			}
			else {
				printk(KERN_WARNING
				       "%s: Ack!  Speed (%d) is not "
				       "10/100/1000!\n", dev->name,
				       phydev->speed);
			}

			status_change = 1;
			bp->speed = phydev->speed;
		}
	}

	/* Check whether link up/down has been changed */
	if (phydev->link != bp->link) {
		if (phydev->link) {
			bp->flags |= LINK_UP;
			/* Perform auto-negotiation */
			/* phy_auto_negotiation(pd); */
		} else {
			bp->flags &= ~LINK_UP;
			bp->speed = 0;
			bp->duplex = -1;
		}
		
		bp->link = phydev->link;
		status_change = 1;
	}

	spin_unlock_irqrestore(&bp->lock, flags);

	if (status_change) {
		if (phydev->link)
			printk(KERN_INFO "%s: link up (%d/%s)\n",
				dev->name, phydev->speed,
				DUPLEX_FULL == phydev->duplex ? 
					"Full" : "Half");
		else
			printk(KERN_INFO "%s: link down\n", dev->name);
	}
	
	/* If TX/RX has been stopped, start them */
	if (tx_rx_stopped) {
		debug("%s: Start RX TX\n", __func__);
		write_reg(CSR6, read_reg(CSR6) | CSR6_ST | CSR6_SR);
	}
}

static int __init core10100_mii_init(struct net_device *dev)
{
	struct core10100_dev *bp = netdev_priv(dev);
	int ret;
	short phy_addr;
	struct phy_device *phydev = NULL;

	bp->mii_bus = alloc_mdio_bitbang(&bp->core10100_mdio_ctrl);

	if (!bp->mii_bus) {
		printk(KERN_INFO "alloc_mdio_bitbang failed!\n");
	}
	
	bp->mii_bus->name = "eth_mii_bus";	
	snprintf(bp->mii_bus->id, MII_BUS_ID_SIZE, "%x", 0);

	ret = mdiobus_register(bp->mii_bus);
	
	if (ret) {
		printk(KERN_INFO "mdiobus_register failed!\n");
	}
	
	/* find the first phy */
	for (phy_addr = 0; phy_addr < PHY_MAX_ADDR; phy_addr++) {
		if (bp->mii_bus->phy_map[phy_addr]) {
			phydev = bp->mii_bus->phy_map[phy_addr];
			printk(KERN_INFO "found PHY id 0x%x addr %d\n",
			       phydev->phy_id, phydev->addr);
			 break;
		}
	}
	
	if (!phydev) {
		printk(KERN_ERR "no PHY found\n");
		 return -ENODEV; 
		/* bp->phy_id = MSS_PHY_ADDRESS_AUTO_DETECT; */
	}

	phydev = phy_connect(dev, dev_name(&phydev->dev),
			     &core10100_adjust_link, 0,
			     PHY_INTERFACE_MODE_RMII);

	if (IS_ERR(phydev)) {
		printk(KERN_ERR "%s: Could not attach to PHY\n", dev->name);
		return PTR_ERR(phydev);
	}
	
	phydev->supported &= PHY_BASIC_FEATURES;

	bp->phy_id = phydev->phy_id;
	bp->link = 0;
	bp->speed = 0;
	bp->duplex = -1;
	bp->phy_dev = phydev;

	return 0;
}

static inline u32 find_next_desc(unsigned int cur, unsigned int size)
{
	return ((++cur) % size);
}

#ifdef DEBUG
static void dump_desc(struct rxtx_desc *tx_desc, char *s)
{
	printk("  DUMP of %sDESC @%#x\n", s, (int)tx_desc);
	printk("  OWNSTAT: %#x\n", tx_desc->own_stat);
	printk("  CNTLSIZ: %#x\n", tx_desc->cntl_size);
	printk("  BUF1   : %#x\n", (int)tx_desc->buf1);
	printk("  BUF2   : %#x\n", (int)tx_desc->buf2);
}
#endif

static int core10100_check_rxframe(struct net_device *dev,
				struct rxtx_desc *rx_desc)
{
	int badframe = 0;
	/*
	  Check that the descriptor contains the whole packet,
	  i.e. the fist and last descriptor flags are set.
	*/
	if (!(rx_desc->own_stat & DESC_RFS) ||
		!(rx_desc->own_stat & DESC_RLS)) {
		printk("%s: receive error: not whole packet\n", __func__);
		dev->stats.rx_errors++;
		return 1;
	}

	if (rx_desc->own_stat & DESC_RTL) {
		printk("%s: received frame too long\n", __func__);
		dev->stats.rx_length_errors++;
		return 1;
	}

	if (rx_desc->own_stat & DESC_RLS) {
		/* The DESC_RES bit is valid only when the DESC_RLS is set */
#if 0
		if((rx_desc->own_stat & DESC_RES)) {
			printk(KERN_INFO "receive_frame error: "
				"DESC_RES flag is set, len %d\n", 
				(rx_desc->own_stat >> 16) & 0x3fff);
			dump_desc(rx_desc, "RX", 1);
			badframe = 1;
		}
#endif
		if (rx_desc->own_stat & DESC_RDE) {
			printk("%s: descriptor error (no Rx buffer avail)\n",
				 __func__);
			dev->stats.rx_fifo_errors++;
			badframe = 1;
		}
		if (rx_desc->own_stat & DESC_RRF) {
			printk("%s: runt frame (damaged)\n", __func__);
			dev->stats.rx_frame_errors++;
			badframe = 1;
		}

		if (rx_desc->own_stat & DESC_RCS) {
			debug("%s: collision\n", __func__);
			dev->stats.collisions++;
			badframe = 0;
		}

		if (rx_desc->own_stat & DESC_RRE) {
			printk("%s: RMII error\n", __func__);
			badframe = 1;
		}
		if (rx_desc->own_stat & DESC_RDB) {
			printk("%s: frame is not byte-aligned\n", __func__);
			dev->stats.rx_frame_errors++;
			badframe = 1;
		}
		if (rx_desc->own_stat & DESC_RCE) {
			printk("%s: frame CRC error\n", __func__);
			dev->stats.rx_crc_errors++;
			badframe = 1;
		}
		if (badframe)
			return 1;
	}
	
	if (rx_desc->own_stat & DESC_RZERO) {
		printk("%s: frame len error\n", __func__);
		dev->stats.rx_frame_errors++;
		return 1;
	}

#if 0
	if (rx_desc->own_stat & DESC_RLS) {
		if (rx_desc->own_stat & DESC_RMF) {
			printk("%s: Multicast Frame\n", __func__);
		}
		/* always set? */
		if (rx_desc->own_stat & DESC_RFT) {
			printk("%s: Not 802.3 frame type, len %d\n", 
				__func__, (rx_desc->own_stat >> 16) & 0x3fff);
		}
	}
#endif

	dev->stats.rx_packets++;
	return 0;
}

static void core10100_check_txframe(struct net_device *dev,
				struct rxtx_desc *tx_desc)
{
	if (tx_desc->cntl_size & DESC_TLS) {
		if (tx_desc->own_stat & (DESC_TLO | /*DESC_TNC |*/ DESC_TLC |
					 DESC_TEC | DESC_TUF | DESC_TDE)) {
			dev->stats.tx_errors++;
			if (tx_desc->own_stat & DESC_TLO) {
				printk("%s: carrier lost\n", __func__);
				dev->stats.tx_carrier_errors++;
			}
#if 0			/* Always set ??? */
			if (tx_desc->own_stat & DESC_TNC) {
				debug("%s: No Carrier\n", __func__);
				dev->stats.tx_carrier_errors++;
			}
#endif
			if (tx_desc->own_stat & DESC_TLC) {
				printk("%s: late collision\n", __func__);
				dev->stats.tx_window_errors++;
			}
			if (tx_desc->own_stat & DESC_TEC) {
				printk("%s: excessive collisions\n", __func__);
				dev->stats.tx_window_errors++;
			}
			if (tx_desc->own_stat & DESC_TUF) {
				printk("%s: buffer underflow\n", __func__);
				dev->stats.tx_fifo_errors++;
			}
			if (tx_desc->own_stat & DESC_TDE) {
				/* TBD - don't free skb? */
				printk("%s: frame deferred\n", __func__);
				dev->stats.collisions++;
			}
		} else {
			dev->stats.tx_packets++;
		}

	} else debug("%s: Not \"last\" Tx frame????\n", __func__);
}

/* Handle Frame transmition */
static void tx_handler(struct net_device *dev)
{
	struct core10100_dev *bp = netdev_priv(dev);
	int i;

	spin_lock(&bp->lock);

	for (i = 0; i < TX_RING_SIZE; i++,
		bp->tx_dirty = find_next_desc(bp->tx_dirty, TX_RING_SIZE)) {
		if (bp->tx_descs[bp->tx_dirty].own_stat & DESC_OWN) {
			debug("%s: %d not ready, exiting, try %d\n",
				__func__, bp->tx_dirty, i);
			break;
		}

		if (bp->tx_cur == bp->tx_dirty && !bp->tx_full) {
			debug("%s: no more Tx descs, TX not full\n", __func__);
			break;
		}
#ifdef DEBUG
		dump_desc((struct rxtx_desc *)&bp->tx_descs[bp->tx_dirty],
			"TX");
#endif
		/* Update counters */
		core10100_check_txframe(dev, 
			(struct rxtx_desc *)&bp->tx_descs[bp->tx_dirty]);
		
		/* Free the skb buffer associated with the frame */
		debug("%s: freeing %d, tx_cur %d\n", 
			__func__, bp->tx_dirty, bp->tx_cur);
		dev_kfree_skb_any(bp->tx_skbs[bp->tx_dirty]);
		bp->tx_skbs[bp->tx_dirty] = NULL;
		/* Check and clear the "TX Full" condition */
		if (bp->tx_full) {
			debug("%s: Clearing TX full, starting TX queue\n",
				 __func__);
			bp->tx_full = 0;
			if (netif_queue_stopped(dev)) {
				netif_wake_queue(dev);
			}
		}
	}
	debug("%s: exiting, cur %d, dirty %d\n", 
		__func__, bp->tx_cur, bp->tx_dirty);
	spin_unlock(&bp->lock);
}


/* handle received frame */
static short rx_handler(struct net_device *dev)
{
	struct core10100_dev *bp = netdev_priv(dev);
	
	u32 j;
	unsigned long flags;
	u32 size = 0;
	struct sk_buff *skb;

	spin_lock_irqsave(&bp->lock, flags);

	/*
	  Check whether Core10/100 returns the descriptor to the host
	  i.e. a packet is received. Limit the loop by the ring size.
	*/
	for (j = 0; j < RX_RING_SIZE;
		j++, bp->rx_cur = find_next_desc(bp->rx_cur, RX_RING_SIZE)) {
		if (bp->rx_descs[bp->rx_cur].own_stat & DESC_OWN) {
			debug("%s: %d not ready, exiting, try %d\n", 
				__func__, bp->rx_cur, j);
			break;
		}
		debug("rx_cur = %d, i %d\n", bp->rx_cur, j);

		if (core10100_check_rxframe(dev, 
			(struct rxtx_desc *)&bp->rx_descs[bp->rx_cur])) {
			goto end_alloc;
		}

		/* Check the received packet size */
		size = (bp->rx_descs[bp->rx_cur].own_stat >> 16) & 0x3fff;
		if (size > CORE10100_MAX_DATA_SIZE_ALIGNED) {
			/* Drop the packet */
			printk("%s: pkt sz %d > bufsize %d", __func__, size,
				CORE10100_MAX_DATA_SIZE_ALIGNED);
			dev->stats.rx_dropped++;
			goto end_alloc;
		}

		if (size < sizeof(struct ethhdr)) {
			printk("%s: packet too small: %d\n", __func__, size);
			dev->stats.rx_dropped++;
			goto end_alloc;
		}
		dev->stats.rx_bytes += size;

		/* from fec.c */
		skb = dev_alloc_skb(size - 4 + NET_IP_ALIGN);
		if (unlikely(!skb)) {
			printk("%s: insufficient memory, dropping packet.\n",
					dev->name);
			dev->stats.rx_dropped++;
			goto end_alloc;
		} else {
			skb_reserve(skb, NET_IP_ALIGN);
			debug("%s:skb_put(%d-4)\n", __func__, size);
			skb_put(skb, size - 4);	/* Make room */
			skb_copy_to_linear_data(skb, 
				bp->rx_buffs[bp->rx_cur], size - 4);
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);
		}
end_alloc:
		/* Prepare the packet for the following receiving */
		bp->rx_descs[bp->rx_cur].cntl_size =
			DESC_RCH | CORE10100_MAX_DATA_SIZE_ALIGNED;
		/* Give the descriptor ownership to Core */
		bp->rx_descs[bp->rx_cur].own_stat = DESC_OWN;
	}

#ifdef DEBUG
	/* Paranoia */
	if (j == RX_RING_SIZE &&
	!(bp->rx_descs[find_next_desc(bp->rx_cur, RX_RING_SIZE)].own_stat & DESC_OWN)) {
		debug("%s: Looped the whole rx_desc ring, and nxt %d is READY!\n", __func__, find_next_desc(bp->rx_cur, RX_RING_SIZE));
	}
#endif

	/* Receive poll demand */
	write_reg(CSR2, 1);
	spin_unlock_irqrestore(&bp->lock, flags);	

	return 0;
}

static irqreturn_t core10100_interrupt (int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct core10100_dev *bp = netdev_priv(dev);
	unsigned int handled = 0;
	u32 intr_status;

	intr_status = read_reg(CSR5);
	/* Transmit */
	if (intr_status & CSR5_TI) {			
		debug(KERN_NOTICE "received TX irq\n");
		tx_handler(dev);
		handled = 1;
	}

	/* Receive  */
	if (intr_status & CSR5_RI) {
		rx_handler(dev);
		handled = 1;
	}

	write_reg(CSR5, intr_status);

	return IRQ_RETVAL(handled);
}

static int core10100_open(struct net_device *dev)
{
	struct core10100_dev *bp = netdev_priv(dev); 	       
	
	bp->link = 0;
	bp->duplex = -1;
	bp->speed = -1;
	
	phy_start_aneg(bp->phy_dev);

	/* schedule a link state check */
	phy_start(bp->phy_dev);

	netif_start_queue(dev);

	return 0;
}

static int core10100_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	netif_carrier_off(dev);

	return 0;
}

static netdev_tx_t core10100_start_xmit(struct sk_buff *skb,
					struct net_device *dev)

{
	struct core10100_dev *bp = netdev_priv(dev);
	unsigned long flags;
	char *p = skb->data;

	if (!bp->link) {
		/* Link is down or autonegotiation is in progress. */
		printk("%s: No link, exiting\n", __func__);
		return NETDEV_TX_BUSY;
	}

	spin_lock_irqsave(&bp->lock, flags);

	/* Take the next free Tx desc */
	if (bp->tx_descs[bp->tx_cur].own_stat & DESC_OWN) {
		debug("%s: Tx queue full, tx_cur %d, tx_dirty %d, exiting\n", 
			__func__, bp->tx_cur, bp->tx_dirty);
		spin_unlock_irqrestore(&bp->lock, flags);
		return NETDEV_TX_BUSY;
	}

	debug("%s: tx_cur %d, len %d\n", __func__, bp->tx_cur, skb->len);

	if (skb->len > 0x7ff) {
		/* TBD - allocate more than one Tx desc for such packets */
		printk("%s: Data len %d > desc max len %d???\n", 
			__func__, skb->len, 0x7ff);
		spin_unlock_irqrestore(&bp->lock, flags);
		return NETDEV_TX_BUSY;
	}

	if (1 && (int)skb->data & 0x3) {
		debug("%s: data buffer not aligned, using internal buffer\n", 
			__func__);
		skb_copy_from_linear_data(skb, bp->tx_buffs[bp->tx_dirty], 
					skb->len);
		p = bp->tx_buffs[bp->tx_dirty];
	}

	dev->stats.tx_bytes += skb->len;

	/*
	Prepare the descriptors as follows:
	 - set the last descriptor flag
	 - set the first descriptor flag
	 - set the packet length
	*/

	bp->tx_descs[bp->tx_cur].cntl_size = 
		/*DESC_TIC |*/ DESC_TCH | DESC_TLS | DESC_TFS | skb->len;

	/* make descriptor pointer to point to skb buf. 
	 * No alignment restrictions for Tx buffers. */
	bp->tx_descs[bp->tx_cur].buf1 = p;

	/* Save the skb being sent */
	bp->tx_skbs[bp->tx_cur] = skb;

#ifdef DEBUG
	dump_desc((struct rxtx_desc *)&bp->tx_descs[bp->tx_cur], "TX");
#endif
	/* Give the current descriptor ownership to Core10/100.	*/

	bp->tx_descs[bp->tx_cur].own_stat = DESC_OWN;
	/* Start transmission */
	write_reg(CSR6, read_reg(CSR6) | CSR6_ST);

	/* Transmit poll demand */
	write_reg(CSR1, CSR1_TPD);
	dev->trans_start = jiffies;

	/* Advance ptr to next free Tx descriptor */
	bp->tx_cur = find_next_desc(bp->tx_cur, TX_RING_SIZE);
	if (bp->tx_cur == bp->tx_dirty) {
		debug("%s: TX full, stop Tx queue, curr %d, dirty %d\n", 
			__func__,
			bp->tx_cur, bp->tx_dirty);
		bp->tx_full = 1;
		netif_stop_queue(dev);
	}

	/* Paranoia, check nxt  */
	if (bp->tx_descs[bp->tx_cur].own_stat & DESC_OWN) {
		debug("%s: Warning! No free descriptors? tx_cur %d, tx_dirty %d\n", __func__,
			   bp->tx_cur, bp->tx_dirty);
	}
    
	spin_unlock_irqrestore(&bp->lock, flags);

	return NETDEV_TX_OK;
}


int core10100_mac_addr(struct net_device *dev, void *p)
{
	int i;
	struct core10100_dev *bp = netdev_priv(dev);

	memcpy(bp->mac, p, sizeof(bp->mac));

	/* Fill all the entries of the mac filter */

	for (i = 0; i < 192; i += 12) {
		memcpy(bp->mac_filter + i, bp->mac, 6);
	}

	/*
	  Setup TX descriptor for the setup frame as follows:
	  - owned by Core
	  - chained
	  - buffer1 points to tx_buf
	  - buffer2 points to the descriptor itself
	  - perfect filtering
	  - length is 192
	*/
	
	bp->tx_mac->own_stat = DESC_OWN;
	bp->tx_mac->cntl_size = DESC_TCH | DESC_SET | 192;
	bp->tx_mac->buf1 = bp->mac_filter;
	bp->tx_mac->buf2 = (struct rxtx_desc *) bp->tx_mac;
	write_reg(CSR4, (unsigned long) bp->tx_mac);
	
	
	/* <TODO>: fix nasty busy loops to proper waits */
	
	/* Start transmission */
	write_reg(CSR6, read_reg(CSR6) | CSR6_ST);
	
	/* Wait for the packet transmission end */
	for (i = 0; i < TIMEOUT_LOOPS; i++) {
		/* Transmit poll demand */
		write_reg(CSR1, CSR1_TPD);
		/* Wait until Core10/100 returns the descriptor ownership */
		if (!(bp->tx_mac->own_stat & DESC_OWN)) {
			break;
		}
		udelay(TIMEOUT_UDELAY);
	}
	
	if (i == TIMEOUT_LOOPS) {
		printk(KERN_ERR "MAC addr setup TX timeout\n");
		return !0;
	}

	/* Stop transmission */
	write_reg(CSR6, read_reg(CSR6) & ~CSR6_ST);
	/* Wait for the transmission process stopped */
	for (i = 0; i < TIMEOUT_LOOPS; i++) {
		if (((read_reg(CSR5) >> CSR5_TS_SHIFT) & CSR5_TS_MASK) ==
		    CSR5_TS_STOP) {
			break;
		}
		udelay(TIMEOUT_UDELAY);
	}
    
    if (i == TIMEOUT_LOOPS) {
	    printk(KERN_ERR "Can not stop TX!\n");
	    return !0;
    }
    write_reg(CSR5, CSR5_TPS);

    /* Restore the real TX descriptors pointers */
    write_reg(CSR4, (unsigned long)&bp->tx_descs[0]);

    return 0;
    
}

/* Init the adapter */
static int __init core10100_init(struct core10100_dev *bp)
{
	int i;
	
	/* Reset the controller  */
	write_reg(CSR0,  read_reg(CSR0) | CSR0_SWR);
	
	/* Wait for reset  */
	for (i = 0; i < TIMEOUT_LOOPS; i++) {
		if (!(read_reg(CSR0) & CSR0_SWR)) {
			break;
		}
		udelay(TIMEOUT_UDELAY);
	}
	
	if (i == TIMEOUT_LOOPS) {
		printk(KERN_INFO "core10100: SWR timeout\n");
		return !0;
	}
	
	/* Setup the little endian mode for the data descriptors  */
	write_reg(CSR0, read_reg(CSR0) & ~CSR0_DBO);

	bp->tx_cur = 0;
	
	return 0;
}

static const struct net_device_ops core10100_netdev_ops = {
	.ndo_open		= core10100_open,
	.ndo_stop		= core10100_close,
	.ndo_start_xmit		= core10100_start_xmit,
	.ndo_set_mac_address    = core10100_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_change_mtu		= eth_change_mtu,
};


static int __init core10100_probe(struct platform_device *pd)
{
	struct net_device *dev;
	struct core10100_dev *bp;
	u32 mem_base, mem_size, a;
	u16 irq;
#ifdef USE_INTRAM
	char *p = (char *)USE_INTRAM;
#endif
	int err = -ENXIO;
	struct resource *res;
	u8 core_mac_addr[6];
	char *ptr, *ptr_end;
	u8 ethaddr[18];

	res = platform_get_resource(pd, IORESOURCE_MEM, 0);
	if (!res) {
	     dev_err(&pd->dev, "no mmio resource defined\n");
	     goto err_out;
	}

	mem_base = res->start;
	mem_size = resource_size(res);
	
	irq = platform_get_irq(pd, 0);
	
	dev = alloc_etherdev(sizeof(*bp));
	
	if (!dev) {
	     dev_err(&pd->dev, "etherdev alloc failed, aborting.\n");
	     goto err_out;
	}

	dev->netdev_ops = &core10100_netdev_ops;

	bp = netdev_priv(dev);
	bp->dev = dev;
	
	
	SET_NETDEV_DEV(dev, &pd->dev);
	spin_lock_init(&bp->lock); 
	
	dev->irq = irq;
	
	err = request_irq(dev->irq, core10100_interrupt, 0, "core10100", dev);

	if (err) {
		dev_err(&dev->dev, "Unable to request IRQ %d (error %d)\n",
			irq, err);
		goto err_out;
	}

	bp->base = ioremap(mem_base, mem_size);
	
	printk(KERN_INFO "Found Core10100 MAC at 0x%x, irq %d\n", 
		(unsigned int) bp->base, dev->irq);

	bp->core10100_mdio_ctrl.ops = &core10100_mdio_ops;

	core10100_mii_init(dev);

	core10100_init(bp);

	memset(core_mac_addr, 0, sizeof(core_mac_addr));

	if ((ptr = strnstr(boot_command_line, "ethaddr=", COMMAND_LINE_SIZE))) {
		int i;
		memcpy(ethaddr, ptr + strlen("ethaddr="), sizeof(ethaddr));
		ptr_end = ethaddr;
		for (i = 0; i <= 5; i++) {
			core_mac_addr[i] = simple_strtol(ptr_end, &ptr_end, 16) |
				simple_strtol(ptr_end, &ptr_end, 16) << 4;
			ptr_end++; /* skip ":" in  ethaddr */
		}
	}

	if (!is_valid_ether_addr(core_mac_addr)) {
		printk(KERN_ERR "MAC address is not set or invalid, using random.\n");
		random_ether_addr(core_mac_addr);
	}
	memcpy(bp->dev->dev_addr, core_mac_addr, sizeof(core_mac_addr));
	err = register_netdev(dev);
	
	if (err) {
		dev_err(&pd->dev, "Cannot register net device, aborting.\n");
		goto err_out;
	}

	pd->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	
#ifndef USE_INTRAM
	/*alloc rx/tx descriptors in DMA-coherent memory*/
	bp->rx_descs =
		dma_alloc_coherent(&pd->dev,
				   sizeof(struct rxtx_desc) * RX_RING_SIZE,
				   &bp->rx_dma_handle,
				   GFP_DMA);

	bp->tx_descs =
		dma_alloc_coherent(&pd->dev,
				   sizeof(struct rxtx_desc) * TX_RING_SIZE,
				   &bp->tx_dma_handle,
				   GFP_DMA);

	/*alloc mac tx descriptor and buffer*/
	
	bp->tx_mac =
		dma_alloc_coherent(&pd->dev,
				   sizeof(struct rxtx_desc),
				   &bp->tx_mac_dma_handle,
				   GFP_DMA);

	
	bp->mac_filter =
		dma_alloc_coherent(&pd->dev,
				   192,
				   &bp->mac_filter_dma_handle,
				   GFP_DMA);
#else
	bp->rx_descs = (struct rxtx_desc *)p;
	p += sizeof(struct rxtx_desc) * RX_RING_SIZE;

	bp->tx_descs = (struct rxtx_desc *)p;
	p += sizeof(struct rxtx_desc) * TX_RING_SIZE;

	bp->tx_mac = (struct rxtx_desc *)p;
	p += sizeof(struct rxtx_desc);

	bp->mac_filter = p;
	p += 192;

#endif
	
	if (!(bp->rx_descs && bp->tx_descs
	      && bp->mac_filter && bp->tx_mac)) {
		dev_err(&pd->dev, "unable to alloc dma memory!\n");
		goto err_out;
	}

	/* No automatic polling */
	write_reg(CSR0, read_reg(CSR0) &~ CSR0_TAP_MASK);

	/*
	  Setup RX descriptor as follows (the only descriptor is used):
	  - owned by Core
	  - chained
	  - buffer size is CORE10100_MAX_DATA_SIZE_ALIGNED
	  - buffer1 points to rx_buf
	  - buffer2 points to the descriptor itself
	*/
	
	for (a = 0; a < RX_RING_SIZE; a++) {
		/* Give the ownership to the MAC */
		bp->rx_descs[a].own_stat = DESC_OWN;

		/*
		  The size field of the descriptor is 10 bits in size,
		  so lets check that the
		  CORE10100_MAX_DATA_SIZE_ALIGNED is not bigger than 2047
		*/

		bp->rx_descs[a].cntl_size =
			DESC_RCH |
			(CORE10100_MAX_DATA_SIZE_ALIGNED > 0x7FF ?
			 0x7FF : CORE10100_MAX_DATA_SIZE_ALIGNED);

#ifndef USE_INTRAM
		dma_alloc_coherent(&pd->dev,
				   CORE10100_MAX_DATA_SIZE_ALIGNED,
				   (dma_addr_t *)&bp->rx_buffs[a],
				   GFP_DMA);
#else
		bp->rx_buffs[a] = p;
		p += CORE10100_MAX_DATA_SIZE_ALIGNED;
#endif
		bp->rx_descs[a].buf1 = bp->rx_buffs[a];
		bp->rx_descs[a].buf2 = (void *)&bp->rx_descs[find_next_desc(a, RX_RING_SIZE)];
#ifdef DEBUG
		dump_desc((struct rxtx_desc *)&bp->rx_descs[a], "RX");
#endif
	}
	bp->rx_cur = 0;
	write_reg(CSR3, (u32) bp->rx_descs);
	
	/*
	  Setup TX descriptors as follows
	  - chained
	  - buffer1 inited to NULL, will be set to data in start_xmit()
	  - buffer2 points to the following itself
	*/

	for (a = 0; a < TX_RING_SIZE; a++) {
		/* Give the ownership to the host */
		bp->tx_descs[a].own_stat = 0;
		bp->tx_descs[a].cntl_size = 0; /* Will be set in start_xmit() */
		bp->tx_skbs[a] = NULL; /* Ditto */

#ifndef USE_INTRAM
		dma_alloc_coherent(&pd->dev,
				   CORE10100_MAX_DATA_SIZE_ALIGNED,
				   (dma_addr_t *)&bp->tx_buffs[a],
				   GFP_DMA);
#else
		bp->tx_buffs[a] = p;
		p += CORE10100_MAX_DATA_SIZE_ALIGNED;
#endif
		bp->tx_descs[a].buf1 = NULL;
		bp->tx_descs[a].buf2 = (void *)&bp->tx_descs[find_next_desc(a, TX_RING_SIZE)];
		if ((int)bp->tx_buffs[a] & 0x3) {
			printk("%s: Warning: Internal tx buf is not aligned!\n", __func__);
		}
	}

	bp->tx_cur = 0;
	bp->tx_dirty = 0;
	
	write_reg(CSR4, (u32) bp->tx_descs);

	/* enable normal interrupts */
	write_reg(CSR7, CSR7_NIE | read_reg(CSR7));

	/* enable tx and rx interrupts */
	write_reg(CSR7, CSR7_RIE | read_reg(CSR7));
	write_reg(CSR7, CSR7_TIE | read_reg(CSR7));
	
	/* setup mac address */
	core10100_mac_addr(dev, (void *) core_mac_addr);
	
	/* Start transmission and receiving */
	write_reg(CSR6, read_reg(CSR6) | CSR6_ST | CSR6_SR);
	bp->flags |= TX_RX_ENABLED;
	
	return 0;
	
err_out:
	return err;
}

static struct platform_driver core10100_platform_driver = {
	.probe = core10100_probe,
	.driver = {
		.name = "core10100",
		.owner = THIS_MODULE
	}
};

static int __init core10100_modinit(void) {
	platform_driver_register(&core10100_platform_driver);	
	return 0;
}

static void __exit core10100_modexit(void) {
	platform_driver_unregister(&core10100_platform_driver);
}

module_init(core10100_modinit);
module_exit(core10100_modexit);
