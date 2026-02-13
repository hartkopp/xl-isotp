/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/* Copyright and detailed license information see isotp.c */

#ifndef _ISOTP_PROTOCOL_H_
#define _ISOTP_PROTOCOL_H_

#include <linux/socket.h>
#include <linux/printk.h>
#include <net/sock.h>
#include <linux/can.h>
#include <linux/can/isotp.h>
#include "isotp_defines.h"

int isotp_sendmsg(struct socket *sock, struct msghdr *msg, size_t size);
void isotp_send_cframe(struct isotp_sock *so);
void isotp_send_fc(struct sock *sk, int ae, u8 flowstatus);
void isotp_rcv_echo(struct sk_buff *skb, void *data);
void isotp_rcv(struct sk_buff *skb, void *skdata);
bool isotp_register_rxid(struct isotp_sock *so);
u8 padlen(u8 datalen);

static inline struct isotp_sock *isotp_sk(const struct sock *sk)
{
	return (struct isotp_sock *)sk;
}

static inline u32 isotp_bc_flags(struct isotp_sock *so)
{
	return so->opt.flags & ISOTP_ALL_BC_FLAGS;
}

/* increase (24 bit) tx generation value */
static u32 isotp_inc_tx_gen(u32 gen)
{
	return (gen + 1) & ISOTP_TX_RESULT_GEN_MASK;
}

/* store 8 bit error and 24 bit tx generation values in packed u32 element */
static u32 isotp_pack_tx_result(u32 gen, int err)
{
	return gen | ((u32)err << ISOTP_TX_RESULT_GEN_BITS);
}

/* get the 24 bit tx generation value from the tx result */
static u32 isotp_get_tx_gen(u32 gen_err)
{
	return gen_err & ISOTP_TX_RESULT_GEN_MASK;
}

/* get the 8 bit error value from the tx result */
static u32 isotp_get_tx_err(u32 gen_err)
{
	return (gen_err >> ISOTP_TX_RESULT_GEN_BITS) & ISOTP_TX_RESULT_ERR_MASK;
}

/* store transfer result in per-generation%4 so->tx_result[] slot */
static void isotp_set_tx_result(struct isotp_sock *so, u32 gen, int err)
{
	WRITE_ONCE(so->tx_result[gen % ISOTP_TX_RESULT_SLOTS],
		   isotp_pack_tx_result(gen, err));
}

/* fetch the result recorded for 'gen', as a (negative) errno (0 for success) */
static int isotp_get_tx_result(struct isotp_sock *so, u32 gen)
{
	u32 result = READ_ONCE(so->tx_result[gen % ISOTP_TX_RESULT_SLOTS]);

	if (isotp_get_tx_gen(result) != gen) {
		pr_notice_once("can-isotp: tx_result[] slot reused before read\n");

		/* report failure rather than risk a false success */
		return -ECOMM;
	}

	return -(isotp_get_tx_err(result));
}

/* true if done, shut down or superseded ('gen' is no longer the active
 * transfer). Reads tx.state first (acquire) so tx_gen/tx_result reads
 * below see at least what that state write published (common sequence).
 */
static bool isotp_tx_gen_done(struct isotp_sock *so, u32 gen)
{
	/* read tx.state first for the common sequence */
	u32 state = smp_load_acquire(&so->tx.state);

	return state == ISOTP_IDLE || state == ISOTP_SHUTDOWN ||
	       READ_ONCE(so->tx_gen) != gen;
}

/* CAN FD requires e.g. mandatory padding for TX_DL > 8 */
static inline bool fd_pdu(struct isotp_sock *so)
{
	return (so->ll.mtu == CANFD_MTU);
}

#endif /* _ISOTP_PROTOCOL_H_ */
