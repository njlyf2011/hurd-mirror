/* libdiskfs implementation of fs.defs: file_set_translator
   Copyright (C) 1992,93,94,95,96,99,2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "priv.h"
#include "fs_S.h"
#include <hurd/paths.h>
#include <hurd/fsys.h>

/* Implement file_set_translator as described in <hurd/fs.defs>. */
kern_return_t
diskfs_S_file_set_translator (struct protid *cred,
			      int passive_flags,
			      int active_flags,
			      int killtrans_flags,
			      char *passive,
			      u_int passivelen,
			      fsys_t active)
{
  struct node *np;
  error_t error;
  mach_port_t control = MACH_PORT_NULL;

  if (!cred)
    return EOPNOTSUPP;

  if (!(passive_flags & FS_TRANS_SET) && !(active_flags & FS_TRANS_SET))
    return 0;

  if ((passive_flags & FS_TRANS_SET) && diskfs_check_readonly ())
    return EROFS;

  if (passive && passive[passivelen - 1])
    return EINVAL;

  np = cred->po->np;

  mutex_lock (&np->lock);

  error = fshelp_isowner (&np->dn_stat, cred->user);
  if (error)
    {
      mutex_unlock (&np->lock);
      return error;
    }

  if ((active_flags & FS_TRANS_SET)
      && ! (active_flags & FS_TRANS_ORPHAN))
    {
      error = fshelp_fetch_control (&np->transbox, &control);
      if (error)
	{
	  mutex_unlock (&np->lock);
	  return error;
	}

      if ((control != MACH_PORT_NULL) && ((active_flags & FS_TRANS_EXCL) == 0))
	{
	  mutex_unlock (&np->lock);
	  error = fsys_goaway (control, killtrans_flags);
	  mach_port_deallocate (mach_task_self (), control);
	  if (error && (error != MIG_SERVER_DIED)
	      && (error != MACH_SEND_INVALID_DEST))
	    return error;
	  error = 0;
	  mutex_lock (&np->lock);
	}
      else if (control != MACH_PORT_NULL)
	mach_port_deallocate (mach_task_self (), control);
    }

  /* Handle exclusive passive bit *first*.  */
  if ((passive_flags & FS_TRANS_SET)
      && (passive_flags & FS_TRANS_EXCL)
      && (np->dn_stat.st_mode & S_IPTRANS))
    {
      mutex_unlock (&np->lock);
      return EBUSY;
    }

  if (active_flags & FS_TRANS_SET)
    {
      error = fshelp_set_active (&np->transbox, active,
				 active_flags & FS_TRANS_EXCL);
      if (error)
	{
	  mutex_unlock (&np->lock);
	  return error;
	}
    }

  /* Set passive translator */
  if (passive_flags & FS_TRANS_SET)
    {
      if (!(passive_flags & FS_TRANS_FORCE))
	{
	  /* Handle the short-circuited translators */
	  mode_t newmode = 0;

	  if (diskfs_shortcut_symlink && !strcmp (passive, _HURD_SYMLINK))
	    newmode = S_IFLNK;
	  else if (diskfs_shortcut_chrdev && !(strcmp (passive, _HURD_CHRDEV)))
	    newmode = S_IFCHR;
	  else if (diskfs_shortcut_blkdev && !strcmp (passive, _HURD_BLKDEV))
	    newmode = S_IFBLK;
	  else if (diskfs_shortcut_fifo && !strcmp (passive, _HURD_FIFO))
	    newmode = S_IFIFO;
	  else if (diskfs_shortcut_ifsock && !strcmp (passive, _HURD_IFSOCK))
	    newmode = S_IFSOCK;

	  if (newmode)
	    {
	      if (S_ISDIR (np->dn_stat.st_mode))
		{
		  /* We can't allow this, because if the mode of the directory
		     changes, the links will be lost.  Perhaps it might be
		     allowed for empty directories, but that's too much of a
		     pain.  */
		  mutex_unlock (&np->lock);
		  return EISDIR;
		}
	      if (newmode == S_IFBLK || newmode == S_IFCHR)
		{
		  /* Find the device number from the arguments
		     of the translator. */
		  int major, minor;
		  char *arg;

		  arg = passive + strlen (passive) + 1;
		  assert (arg <= passive + passivelen);
		  if (arg == passive + passivelen)
		    {
		      mutex_unlock (&np->lock);
		      return EINVAL;
		    }
		  major = strtol (arg, 0, 0);

		  arg = arg + strlen (arg) + 1;
		  assert (arg < passive + passivelen);
		  if (arg == passive + passivelen)
		    {
		      mutex_unlock (&np->lock);
		      return EINVAL;
		    }
		  minor = strtol (arg, 0, 0);

		  error = diskfs_validate_rdev_change (np,
						       makedev (major, minor));
		  if (error)
		    {
		      mutex_unlock (&np->lock);
		      return error;
		    }
		  np->dn_stat.st_rdev = makedev (major, minor);
		}

	      diskfs_truncate (np, 0);
	      if (newmode == S_IFLNK)
		{
		  char *arg = passive + strlen (passive) + 1;
		  assert (arg <= passive + passivelen);
		  if (arg == passive + passivelen)
		    {
		      mutex_unlock (&np->lock);
		      return EINVAL;
		    }

		  if (diskfs_create_symlink_hook)
		    error = (*diskfs_create_symlink_hook)(np, arg);
		  if (!diskfs_create_symlink_hook || error == EINVAL)
		    /* Store the argument in the file as the
		       target of the link */
		    error = diskfs_node_rdwr (np, arg, 0, strlen (arg),
					      1, cred, 0);
		  if (error)
		    {
		      mutex_unlock (&np->lock);
		      return error;
		    }
		}
	      newmode = (np->dn_stat.st_mode & ~S_IFMT) | newmode;
	      error = diskfs_validate_mode_change (np, newmode);
	      if (!error)
		{
		  np->dn_stat.st_mode = newmode;
		  diskfs_node_update (np, 1);
		}
	      mutex_unlock (&np->lock);
	      return error;
	    }
	}
      error = diskfs_set_translator (np, passive, passivelen, cred);
    }

  mutex_unlock (&np->lock);
  return error;
}
