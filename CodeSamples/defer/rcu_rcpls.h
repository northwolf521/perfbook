/*
 * rcu_rcpls.h: simple user-level implementation of RCU based on per-thread
 * pairs of global reference counters, but that is also capable of
 * sharing grace periods between multiple updates.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (c) 2008 Paul E. McKenney, IBM Corporation.
 */

#include "rcu_pointer.h"

DEFINE_SPINLOCK(rcu_gp_lock);
DEFINE_PER_THREAD(int [2], rcu_refcnt);
long rcu_idx;
DEFINE_PER_THREAD(int, rcu_nesting);
DEFINE_PER_THREAD(int, rcu_read_idx);

static void rcu_init(void)
{
	int t;

	rcu_idx = 0;
	init_per_thread(rcu_nesting, 0);
	for_each_thread(t) {
		per_thread(rcu_refcnt, t)[0] = 0;
		per_thread(rcu_refcnt, t)[1] = 0;
	}
}

static void rcu_read_lock(void)
{
	int i;
	int n;

	n = __get_thread_var(rcu_nesting);
	if (n == 0) {
		i = ACCESS_ONCE(rcu_idx) & 0x1;
		__get_thread_var(rcu_read_idx) = i;
		__get_thread_var(rcu_refcnt)[i]++;
	}
	__get_thread_var(rcu_nesting) = n + 1;
	smp_mb();
}

static void rcu_read_unlock(void)
{
	int i;
	int n;

	smp_mb();
	n = __get_thread_var(rcu_nesting);
	if (n == 1) {
		 i = __get_thread_var(rcu_read_idx);
		__get_thread_var(rcu_refcnt)[i]--;
	}
	__get_thread_var(rcu_nesting) = n - 1;
}

extern void synchronize_rcu(void);
