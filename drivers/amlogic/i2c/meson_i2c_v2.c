// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2017 - Beniamino Galvani <b.galvani@gmail.com>
 */
#include <common.h>
#include <asm/arch-meson/i2c.h>
#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <i2c.h>

#define I2C_TIMEOUT_MS		100

/* CFG RDY fields */
#define RDY_TEE_ONLY		BIT(1)
#define RDY_IF			BIT(0)

/* CFG I2C fields */
#define I2C_RX_THR		GENMASK(23, 16)
#define I2C_TX_THR		GENMASK(15, 8)
#define I2C_DMA			BIT(4)
#define I2C_HW_NEG		BIT(3)
#define I2C_HW_POS		BIT(2)
#define I2C_JOIN_RX		BIT(1)
#define I2C_JOIN_TX		BIT(0)

/* CFG BUS fields */
#define BUS_SPEED_MODE		BIT(17)
#define BUS_SLAVE_MODE		BIT(16)
#define BUS_FILTLE_MASK		GENMASK(15, 12)
#define BUS_RATIO_MASK		GENMASK(11, 0)

/* CFG START fields */
#define START_STRAT		BIT(31)
#define START_LEN		GENMASK(23, 12)
#define START_10_BITS		BIT(11) /*only 7bit now*/
#define START_SLAVE_ADDR	GENMASK(10, 1)
#define START_READ		BIT(0)

/* CFG TX_RX fields */
#define TX_RX_EMPTY		BIT(9)
#define TX_RX_FULL		BIT(8)
#define TX_RX_DATA		GENMASK(7, 0)

/*CGF IRQ fields */
#define RCH_ERROR		BIT(0)
#define WCH_ERROR		BIT(1)
#define NCK_ERROR		BIT(2)
#define RX_EMPTY		BIT(3)
#define TX_FULL			BIT(4)
#define RX_FIFO_READ		BIT(5)
#define TX_FIFO_WRITE		BIT(6)
#define PHY_DONE		BIT(7)
/*
 * when phy error occur, no transfer done irq.
 */
#define TRANS_DONE		(PHY_DONE | NCK_ERROR)
#define TRANS_ERROR		(RCH_ERROR | WCH_ERROR | NCK_ERROR)

// #define I2C_PXP
// #define M_DEBUG
struct i2c_regs {
	u32 cfg_rdy;
	u32 cfg_i2c;
	u32 cfg_start;
	u32 cfg_bus;
	u32 tx_rd_addr;
	u32 tx_wr_addr;
	u32 rx_rd_addr;
	u32 rx_wr_addr;
	u32 cfg_tx;
	u32 cfg_rx;
	u32 reserved1;
	u32 reserved2;
	u32 cfg_irq_state;
	u32 cfg_irq_en;
	u32 cfg_shake;
};

struct meson_i2c_data {
	u32 clkin_rate;
};

struct meson_i2c {
	struct clk clk;
	struct i2c_regs *regs;
	struct i2c_msg *msg;	/* Current I2C message */
	bool last;		/* Whether the message is the last */
	uint count;		/* Number of bytes in the current transfer */
	uint pos;		/* Position of current transfer in message */
	uint fifo_depth;
	struct meson_i2c_data *data;
	uint speed;
};

#ifdef M_DEBUG
static void meson_i2c_dump_reg(struct meson_i2c *i2c, const char func[23], int line)
{
	u32 data;
	int i;

	for (i =0 ; i < 8; i++) {
		data = readl((unsigned char *)i2c->regs + i * 4);
		printf("meson i2c: i2c reg0x%p : 0x%x\n, line:%d",
			(unsigned char *)i2c->regs + i * 4, data, line);
	} //avoid read RX/TX reg, beacasue this operation will change data in these regs.
	for (i = 0 ; i < 3; i++) {
		data = readl((unsigned char *)i2c->regs + 0x30 + i * 4);
		printf("meson i2c: i2c reg0x%p : 0x%x\n, line:%d",
			(unsigned char *)i2c->regs + i * 4, data, line);
	}

}
#endif

/*
 * Retrieve data for the current transfer (which can be at most 8
 * bytes) from the device internal buffer.
 */
static void meson_i2c_push_data_to_user(struct meson_i2c *i2c, u8 data)
{
	u8 *buf = i2c->msg->buf + i2c->pos;

	i2c->count = 1;
	*buf = data;
	debug("meson i2c: read data %08x\n", *buf);
}

/*
 * Write data for the current transfer (which can be at most 8 bytes)
 * to the device internal buffer.
 */
static void meson_i2c_put_data(struct meson_i2c *i2c, u8 *buf, int len)
{
	int i;

	if (!buf) {//this i2c module when trans 0 byte, must put at least 1.
		writel(0x00,  &i2c->regs->cfg_tx);
		return;
	}
	for (i = 0; i < len; i++, buf++) {
		writel(*buf, &i2c->regs->cfg_tx);
		debug("meson i2c: write data %08x sequence %d\n", *buf, i);
	}
}

/*
 * Prepare the next transfer: pick the next 8 bytes in the remaining
 * part of message and write tokens and data (if needed) to the
 * device.
 */
static void meson_i2c_prepare_xfer(struct meson_i2c *i2c)
{
	bool write = !(i2c->msg->flags & I2C_M_RD);

	if (write) {
		i2c->count = min(i2c->msg->len - i2c->pos, i2c->fifo_depth);
		meson_i2c_put_data(i2c, i2c->msg->buf + i2c->pos, i2c->count);
	}
}

static void meson_i2c_do_start(struct meson_i2c *i2c, struct i2c_msg *msg)
{
	unsigned int reg_val, len = msg->len;

	reg_val = (msg->addr << 1) +
				((msg->flags & I2C_M_RD) ? START_READ : 0) +
				(len << 12);
	clrsetbits_le32(&i2c->regs->cfg_start, START_SLAVE_ADDR | START_READ | START_LEN,
					reg_val);
#ifdef M_DEBUG
	meson_i2c_dump_reg(i2c, __func__, __LINE__);
#endif

}

static void meson_i2c_reset_fifo(struct meson_i2c *i2c)
{
	writel(0x00, &i2c->regs->tx_rd_addr);
	writel(0x00, &i2c->regs->tx_wr_addr);
	writel(0x00, &i2c->regs->rx_rd_addr);
	writel(0x00, &i2c->regs->rx_wr_addr);
}

static int meson_i2c_xfer_msg(struct meson_i2c *i2c, struct i2c_msg *msg,
			      int last)
{
	ulong start;
	int32_t rd_val;

	debug("meson i2c: %s addr 0x%x len %u\n",
	      (msg->flags & I2C_M_RD) ? "read" : "write",
	      msg->addr, msg->len);

	i2c->msg = msg;
	i2c->last = last;
	i2c->pos = 0;
	i2c->count = 0;
	//fill trans len, slave addr and R/W cmd for this msg
	meson_i2c_do_start(i2c, msg);
	/* start the transfer */
	setbits_le32(&i2c->regs->cfg_start, START_STRAT);
	do {
		if (msg->flags & I2C_M_RD) {//for rx

			start = get_timer(0);
			do {
				rd_val = readl(&i2c->regs->cfg_rx);
				if (get_timer(start) > I2C_TIMEOUT_MS) {
					clrbits_le32(&i2c->regs->cfg_start, START_STRAT);
					meson_i2c_reset_fifo(i2c);
					writel(0xffff, &i2c->regs->cfg_irq_state);//clear irq bit
					printf("meson i2c: rx timeout\n");
					return -ETIMEDOUT;
				}
			} while(rd_val & TX_RX_EMPTY);//waiting data
			meson_i2c_push_data_to_user(i2c, rd_val & TX_RX_DATA);
		} else { //for tx
			// setbits_le32(&i2c->regs->cfg_start, START_STRAT);
			meson_i2c_prepare_xfer(i2c);
			start = get_timer(0);
			if (i2c->count == i2c->fifo_depth) {/*wait fifo empty*/
				while (!(readl(&i2c->regs->cfg_tx) & TX_RX_EMPTY)) {
					/*time out*/
					if (get_timer(start) > I2C_TIMEOUT_MS) {
						clrbits_le32(&i2c->regs->cfg_start, START_STRAT);
						meson_i2c_reset_fifo(i2c);
						writel(0xffff, &i2c->regs->cfg_irq_state);
						printf("meson i2c: tx timeout\n");
						return -ETIMEDOUT;
					}
				}
			}
		}
		if (readl(&i2c->regs->cfg_irq_state) & TRANS_ERROR) {
			debug("meson i2c: error 0x%x\n", readl(&i2c->regs->cfg_irq_state));
			writel(0xffff, &i2c->regs->cfg_irq_state);
			meson_i2c_reset_fifo(i2c);
			return -EREMOTEIO;
		}
		i2c->pos += i2c->count;
	} while (i2c->pos < msg->len);
	//wait trans done
	start = get_timer(0);
	while (!(readl(&i2c->regs->cfg_irq_state) & TRANS_DONE)) {
		if (get_timer(start) > I2C_TIMEOUT_MS) {
			clrbits_le32(&i2c->regs->cfg_start, START_STRAT);
			meson_i2c_reset_fifo(i2c);
			writel(0xffff, &i2c->regs->cfg_irq_state);//clear irq bit
			printf("meson i2c: rx timeout\n");
			return -ETIMEDOUT;
		}
	}
	clrbits_le32(&i2c->regs->cfg_start, START_STRAT);
	if (readl(&i2c->regs->cfg_irq_state) & TRANS_ERROR) {
		debug("meson i2c: error 0x%x\n", readl(&i2c->regs->cfg_irq_state));
		writel(0xffff, &i2c->regs->cfg_irq_state);//clear irq bit
		meson_i2c_reset_fifo(i2c);
		return -EREMOTEIO;
	}
	writel(0xffff, &i2c->regs->cfg_irq_state);//clear irq bit
	meson_i2c_reset_fifo(i2c);
	return 0;
}

static int meson_i2c_xfer(struct udevice *bus, struct i2c_msg *msg,
			  int nmsgs)
{
	struct meson_i2c *i2c = dev_get_priv(bus);
	int i, ret = 0;

	for (i = 0; i < nmsgs; i++) {
		ret = meson_i2c_xfer_msg(i2c, msg + i, i == nmsgs - 1);
		if (ret)
			return ret;
	}

	return 0;
}

static int meson_i2c_set_speed(struct meson_i2c *i2c)
{
	unsigned long clk_rate = MESON_I2C_CLK_RATE;
	unsigned int div;

	div = DIV_ROUND_UP(clk_rate, i2c->speed);

	/* clock divider has 12 bits */
	if (div >= (1 << 12)) {
		printf("meson i2c: requested bus frequency too low\n");
		div = (1 << 12) - 1;
	} else if (div <= 8) {
		//if b_ratio<=8, chip will set SCL = clk/8
		printf("meson i2c: requested bus frequency too high\n");
	}
#ifdef I2C_PXP
	div = 0x40;
#endif
	if (i2c->speed > 100000)
		clrbits_le32(&i2c->regs->cfg_bus, BUS_SPEED_MODE);
	else
		clrsetbits_le32(&i2c->regs->cfg_bus, BUS_SPEED_MODE, BUS_SPEED_MODE);
	/*CFG_BUS reg:11 - 0 bits*/
	clrsetbits_le32(&i2c->regs->cfg_bus, BUS_RATIO_MASK, div);
	debug(" meson i2c %s: clk %lu, freq %u, div %u\n",
		__func__, clk_rate, i2c->speed, div);

	return 0;
}

static int meson_i2c_set_bus_speed(struct udevice *bus, unsigned int speed)
{
	struct meson_i2c *i2c = dev_get_priv(bus);

	i2c->speed = speed;
	meson_i2c_set_speed(i2c);

	return 0;
}

static void meson_i2c_init(struct meson_i2c *i2c)
{
	clrbits_le32(&i2c->regs->cfg_start, START_STRAT);
	clrsetbits_le32(&i2c->regs->cfg_bus, BUS_FILTLE_MASK, 1 << 12);
	writel(0xffff, &i2c->regs->cfg_irq_state);//clr irq, specific code.
	/*
	 * enable irq, we must enable phy done irq at least.
	 */
	writel(0x9b, &i2c->regs->cfg_irq_en);
	//set fifo depth
	clrsetbits_le32(&i2c->regs->cfg_i2c, I2C_RX_THR | I2C_TX_THR, 16 << 16 | 16 << 8);
	i2c->fifo_depth = 16;
	meson_i2c_reset_fifo(i2c);
	meson_i2c_set_speed(i2c);
}

static int meson_i2c_probe(struct udevice *bus)
{
	struct meson_i2c *i2c = dev_get_priv(bus);

	i2c->data = (struct meson_i2c_data *)dev_get_driver_data(bus);
	meson_i2c_init(i2c);

	return 0;
}

static int meson_i2c_ofdata_to_platdata(struct udevice *dev)
{
	struct meson_i2c *i2c = dev_get_priv(dev);

	i2c->regs = dev_read_addr_ptr(dev);
	debug("i2c->reg %p\n", i2c->regs);

	return 0;
}

static const struct meson_i2c_data i2c_meson_data = {
	.clkin_rate = MESON_I2C_CLK_RATE,
};

static const struct dm_i2c_ops meson_i2c_ops = {
	.xfer          = meson_i2c_xfer,
	.set_bus_speed = meson_i2c_set_bus_speed,
};

static const struct udevice_id meson_i2c_ids[] = {
	{ .compatible = "amlogic,meson-i2c-v2", .data = (long)&i2c_meson_data },
	{ }
};

U_BOOT_DRIVER(i2c_meson_v2) = {
	.name = "i2c_meson_v2",
	.id   = UCLASS_I2C,
	.of_match = meson_i2c_ids,
	.of_to_plat = meson_i2c_ofdata_to_platdata,
	.probe = meson_i2c_probe,
	.priv_auto = sizeof(struct meson_i2c),
	.ops = &meson_i2c_ops,
};
