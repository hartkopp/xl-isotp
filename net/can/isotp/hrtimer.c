// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
// Copyright and detailed license information see isotp.c

#include <linux/hrtimer.h>
#include <linux/wait.h>
#include <net/sock.h>
#include "isotp_defines.h"
#include "isotp_protocol.h"
#include "isotp_hrtimer.h"

enum hrtimer_restart isotp_rx_timer_handler(struct hrtimer *hrtimer)
{
	struct isotp_sock *so = container_of(hrtimer, struct isotp_sock,
					     rxtimer);
	struct sock *sk = &so->sk;

	if (so->rx.state == ISOTP_WAIT_DATA) {
		/* we did not get new data frames in time */

		/* report 'connection timed out' */
		sk->sk_err = ETIMEDOUT;
		if (!sock_flag(sk, SOCK_DEAD))
			sk_error_report(sk);

		/* reset rx state */
		so->rx.state = ISOTP_IDLE;
	}

	return HRTIMER_NORESTART;
}

/* shared by so->txtimer's and so->echotimer's callbacks. Both timers get
 * cancelled under so->sm_lock elsewhere, so this must stay lock-free to
 * avoid deadlocking with that; uses so->tx_gen instead to avoid tainting
 * a new transfer with an error from the one that just timed out.
 */
static enum hrtimer_restart isotp_tx_timeout(struct isotp_sock *so)
{
	struct sock *sk = &so->sk;
	u32 gen = READ_ONCE(so->tx_gen);
	u32 old_state = READ_ONCE(so->tx.state);

	/* don't handle timeouts in IDLE or SHUTDOWN state */
	if (old_state == ISOTP_IDLE || old_state == ISOTP_SHUTDOWN)
		return HRTIMER_NORESTART;

	/* only claim the timeout if the state is still unchanged */
	if (cmpxchg(&so->tx.state, old_state, ISOTP_IDLE) != old_state)
		return HRTIMER_NORESTART;

	/* we did not get any flow control or echo frame in time */

	if (READ_ONCE(so->tx_gen) == gen) {
		/* report 'communication error on send' */
		sk->sk_err = ECOMM;
		if (!sock_flag(sk, SOCK_DEAD))
			sk_error_report(sk);
	}

	wake_up_interruptible(&so->wait);

	return HRTIMER_NORESTART;
}

/* so->txtimer: fires when a Flow Control frame does not arrive in time */
enum hrtimer_restart isotp_tx_timer_handler(struct hrtimer *hrtimer)
{
	struct isotp_sock *so = container_of(hrtimer, struct isotp_sock,
					     txtimer);

	return isotp_tx_timeout(so);
}

/* so->echotimer: fires when a sent CF/SF's local echo does not arrive */
enum hrtimer_restart isotp_echo_timer_handler(struct hrtimer *hrtimer)
{
	struct isotp_sock *so = container_of(hrtimer, struct isotp_sock,
					     echotimer);

	return isotp_tx_timeout(so);
}

enum hrtimer_restart isotp_txfr_timer_handler(struct hrtimer *hrtimer)
{
	struct isotp_sock *so = container_of(hrtimer, struct isotp_sock,
					     txfrtimer);

	/* start echo timeout handling and cover below protocol error */
	hrtimer_start(&so->echotimer, ktime_set(ISOTP_ECHO_TIMEOUT, 0),
		      HRTIMER_MODE_REL_SOFT);

	/* cfecho should be consumed by isotp_rcv_echo() here */
	if (so->tx.state == ISOTP_SENDING && !so->cfecho)
		isotp_send_cframe(so);

	return HRTIMER_NORESTART;
}
