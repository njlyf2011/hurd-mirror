/*
 * Mach device emulation definitions.
 *
 * Copyright (c) 1996 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *      Author: Shantanu Goel, University of Utah CSL
 */

#ifndef _MACHDEV_DEVICE_EMUL_H_
#define _MACHDEV_DEVICE_EMUL_H_

#include <mach.h>
#include <mach/notify.h>
#include <device/device_types.h>
#include <device/net_status.h>
#include <errno.h>

/* Each emulation layer provides these operations.  */
struct machdev_device_emulation_ops
{
  void (*init) (void);
  void (*reference) (void *);
  void (*dealloc) (void *);
  mach_port_t (*dev_to_port) (void *);
  io_return_t (*open) (mach_port_t, mach_msg_type_name_t,
		       dev_mode_t, const char *, device_t *,
		       mach_msg_type_name_t *type);
  io_return_t (*close) (void *);
  io_return_t (*write) (void *, mach_port_t, mach_msg_type_name_t,
			dev_mode_t, recnum_t, io_buf_ptr_t, unsigned, int *);
  io_return_t (*write_inband) (void *, mach_port_t, mach_msg_type_name_t,
			       dev_mode_t, recnum_t, const char *,
			       unsigned, int *);
  io_return_t (*read) (void *, mach_port_t, mach_msg_type_name_t,
		       dev_mode_t, recnum_t, int, io_buf_ptr_t *, unsigned *);
  io_return_t (*read_inband) (void *, mach_port_t, mach_msg_type_name_t,
			      dev_mode_t, recnum_t, int, char *, unsigned *);
  io_return_t (*set_status) (void *, dev_flavor_t, dev_status_t,
			     mach_msg_type_number_t);
  io_return_t (*get_status) (void *, dev_flavor_t, dev_status_t,
			     mach_msg_type_number_t *);
  io_return_t (*set_filter) (void *, mach_port_t, int, filter_t [], unsigned);
  io_return_t (*map) (void *, vm_prot_t, vm_offset_t,
		      vm_size_t, mach_port_t *, boolean_t);
  void (*no_senders) (mach_no_senders_notification_t *);
  io_return_t (*write_trap) (void *, dev_mode_t,
			     recnum_t, vm_offset_t, vm_size_t);
  io_return_t (*writev_trap) (void *, dev_mode_t,
			      recnum_t, io_buf_vec_t *, vm_size_t);
  void (*sync) (void);
};

#endif /* _MACHDEV_DEVICE_EMUL_H_ */
