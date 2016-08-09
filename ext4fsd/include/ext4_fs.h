/*
 * All the on-disk metadata formats reside in this header file
 */

#pragma once

#pragma pack(push, 1)

/*
 * Max physical block we can address w/o extents
 */
#define EXT4_MAX_BLOCK_FILE_PHYS	0xFFFFFFFF

/*
* Structure of a blocks group descriptor
*/
struct ext4_group_desc {
	__le32	bg_block_bitmap_lo;			/* Blocks bitmap block */
	__le32	bg_inode_bitmap_lo;			/* Inodes bitmap block */
	__le32	bg_inode_table_lo;			/* Inodes table block */
	__le16	bg_free_blocks_count_lo;		/* Free blocks count */
	__le16	bg_free_inodes_count_lo;		/* Free inodes count */
	__le16	bg_used_dirs_count_lo;		/* Directories count */
	__le16	bg_flags;					/* EXT4_BG_flags (INODE_UNINIT, etc) */
	__le32	bg_exclude_bitmap_lo;		/* Exclude bitmap for snapshots */
	__le16	bg_block_bitmap_csum_lo;	/* crc32c(s_uuid+grp_num+bbitmap) LE */
	__le16	bg_inode_bitmap_csum_lo;	/* crc32c(s_uuid+grp_num+ibitmap) LE */
	__le16 	bg_itable_unused_lo;			/* Unused inodes count */
	__le16	bg_checksum;				/* crc16(sb_uuid+group+desc) */
	__le32	bg_block_bitmap_hi;			/* Blocks bitmap block MSB */
	__le32	bg_inode_bitmap_hi;			/* Inodes bitmap block MSB */
	__le32	bg_inode_table_hi;			/* Inodes table block MSB */
	__le16	bg_free_blocks_count_hi;		/* Free blocks count MSB */
	__le16	bg_free_inodes_count_hi;		/* Free inodes count MSB */
	__le16	bg_used_dirs_count_hi;		/* Directories count MSB */
	__le16	bg_itable_unused_hi;			/* Unused inodes count MSB */
	__le32	bg_exclude_bitmap_hi;		/* Exclude bitmap block MSB */
	__le16	bg_block_bitmap_csum_hi;	/* crc32c(s_uuid+grp_num+bbitmap) BE */
	__le16	bg_inode_bitmap_csum_hi;	/* crc32c(s_uuid+grp_num+ibitmap) BE */
	__u32	bg_reserved;
};

#define EXT4_BG_INODE_BITMAP_CSUM_HI_END	\
	(FIELD_OFFSET(struct ext4_group_desc, bg_inode_bitmap_csum_hi) + sizeof(__le16))
#define EXT4_BG_BLOCK_BITMAP_CSUM_HI_END	\
	(FIELD_OFFSET(struct ext4_group_desc, bg_block_bitmap_csum_hi) + sizeof(__le16))

#define EXT4_BG_INODE_UNINIT	0x0001 /* Inode table/bitmap not in use */
#define EXT4_BG_BLOCK_UNINIT	0x0002 /* Block bitmap not in use */
#define EXT4_BG_INODE_ZEROED	0x0004 /* On-disk itable initialized to zero */

/*
* Macro-instructions used to manage group descriptors
*/
#define EXT4_MIN_DESC_SIZE			32
#define EXT4_MIN_DESC_SIZE_64BIT	64
#define EXT4_MAX_DESC_SIZE			EXT4_MIN_BLOCK_SIZE
#define EXT4_DESC_SIZE(s)			((s)->s_desc_size)
#define EXT4_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
#define EXT4_DESC_PER_BLOCK(s)		(EXT4_BLOCK_SIZE(s) / EXT4_DESC_SIZE(s))
#define EXT4_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)

/*
 * Constants relative to the data blocks
 */
#define EXT4_NDIR_BLOCKS		12
#define EXT4_IND_BLOCK		EXT4_NDIR_BLOCKS
#define EXT4_DIND_BLOCK		(EXT4_IND_BLOCK + 1)
#define EXT4_TIND_BLOCK		(EXT4_DIND_BLOCK + 1)
#define EXT4_N_BLOCKS			(EXT4_TIND_BLOCK + 1)

 /*
 * Inode flags
 */
#define EXT4_SECRM_FL			0x00000001 /* Secure deletion */
#define EXT4_UNRM_FL			0x00000002 /* Undelete */
#define EXT4_COMPR_FL			0x00000004 /* Compress file */
#define EXT4_SYNC_FL			0x00000008 /* Synchronous updates */
#define EXT4_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define EXT4_APPEND_FL			0x00000020 /* writes to file may only append */
#define EXT4_NODUMP_FL		0x00000040 /* do not dump file */
#define EXT4_NOATIME_FL		0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT4_DIRTY_FL			0x00000100
#define EXT4_COMPRBLK_FL		0x00000200 /* One or more compressed clusters */
#define EXT4_NOCOMPR_FL		0x00000400 /* Don't compress */
/* nb: was previously EXT2_ECOMPR_FL */
#define EXT4_ENCRYPT_FL		0x00000800 /* encrypted file */
/* End compression flags --- maybe not all used */
#define EXT4_INDEX_FL			0x00001000 /* hash-indexed directory */
#define EXT4_IMAGIC_FL			0x00002000 /* AFS directory */
#define EXT4_JOURNAL_DATA_FL	0x00004000 /* file data should be journaled */
#define EXT4_NOTAIL_FL			0x00008000 /* file tail should not be merged */
#define EXT4_DIRSYNC_FL		0x00010000 /* dirsync behaviour (directories only) */
#define EXT4_TOPDIR_FL			0x00020000 /* Top of directory hierarchies*/
#define EXT4_HUGE_FILE_FL		0x00040000 /* Set to each huge file */
#define EXT4_EXTENTS_FL		0x00080000 /* Inode uses extents */
#define EXT4_EA_INODE_FL	        0x00200000 /* Inode used for large EA */
#define EXT4_EOFBLOCKS_FL		0x00400000 /* Blocks allocated beyond EOF */
#define EXT4_INLINE_DATA_FL		0x10000000 /* Inode has inline data. */
#define EXT4_PROJINHERIT_FL		0x20000000 /* Create with parents projid */
#define EXT4_RESERVED_FL		0x80000000 /* reserved for ext4 lib */

#define EXT4_FL_USER_VISIBLE			0x304BDFFF /* User visible flags */
#define EXT4_FL_USER_MODIFIABLE		0x204380FF /* User modifiable flags */

#define EXT4_FL_XFLAG_VISIBLE		(EXT4_SYNC_FL | \
					 EXT4_IMMUTABLE_FL | \
					 EXT4_APPEND_FL | \
					 EXT4_NODUMP_FL | \
					 EXT4_NOATIME_FL | \
					 EXT4_PROJINHERIT_FL)

 /* Flags that should be inherited by new inodes from their parent. */
#define EXT4_FL_INHERITED (EXT4_SECRM_FL | EXT4_UNRM_FL | EXT4_COMPR_FL |\
			   EXT4_SYNC_FL | EXT4_NODUMP_FL | EXT4_NOATIME_FL |\
			   EXT4_NOCOMPR_FL | EXT4_JOURNAL_DATA_FL |\
			   EXT4_NOTAIL_FL | EXT4_DIRSYNC_FL |\
			   EXT4_PROJINHERIT_FL)

 /* Flags that are appropriate for regular files (all but dir-specific ones). */
#define EXT4_REG_FLMASK (~(EXT4_DIRSYNC_FL | EXT4_TOPDIR_FL))

 /* Flags that are appropriate for non-directories/regular files. */
#define EXT4_OTHER_FLMASK (EXT4_NODUMP_FL | EXT4_NOATIME_FL)

 /* Mask out flags that are inappropriate for the given type of inode. */
 #if 0
static inline __u32 ext4_mask_flags(umode_t mode, __u32 flags)
{
	if (S_ISDIR(mode))
		return flags;
	else if (S_ISREG(mode))
		return flags & EXT4_REG_FLMASK;
	else
		return flags & EXT4_OTHER_FLMASK;
}
#endif /* 0 */

/*
* Structure of an inode on the disk
*/
struct ext4_inode {
	__le16	i_mode;					/* File mode */
	__le16	i_uid;						/* Low 16 bits of Owner Uid */
	__le32	i_size_lo;					/* Size in bytes */
	__le32	i_atime;					/* Access time */
	__le32	i_ctime;					/* Inode Change time */
	__le32	i_mtime;					/* Modification time */
	__le32	i_dtime;					/* Deletion Time */
	__le16	i_gid;						/* Low 16 bits of Group Id */
	__le16	i_links_count;				/* Links count */
	__le32	i_blocks_lo;				/* Blocks count */
	__le32	i_flags;					/* File flags */
	union {
		struct {
			__le32  l_i_version;
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
		struct {
			__u32  m_i_reserved1;
		} masix1;
	} osd1;							/* OS dependent 1 */
	__le32	i_block[EXT4_N_BLOCKS];		/* Pointers to blocks */
	__le32	i_generation;				/* File version (for NFS) */
	__le32	i_file_acl_lo;				/* File ACL */
	__le32	i_size_high;
	__le32	i_obso_faddr;				/* Obsoleted fragment address */
	union {
		struct {
			__le16	l_i_blocks_high;		/* were l_i_reserved1 */
			__le16	l_i_file_acl_high;
			__le16	l_i_uid_high;		/* these 2 fields */
			__le16	l_i_gid_high;		/* were reserved2[0] */
			__le16	l_i_checksum_lo;	/* crc32c(uuid+inum+inode) LE */
			__le16	l_i_reserved;
		} linux2;
		struct {
			__le16	h_i_reserved1;		/* Obsoleted fragment number/size which are removed in ext4 */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
		struct {
			__le16	h_i_reserved1;		/* Obsoleted fragment number/size which are removed in ext4 */
			__le16	m_i_file_acl_high;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;							/* OS dependent 2 */
	__le16	i_extra_isize;
	__le16	i_checksum_hi;				/* crc32c(uuid+inum+inode) BE */
	__le32	i_ctime_extra;				/* extra Change time      (nsec << 2 | epoch) */
	__le32	i_mtime_extra;				/* extra Modification time(nsec << 2 | epoch) */
	__le32	i_atime_extra;				/* extra Access time      (nsec << 2 | epoch) */
	__le32	i_crtime;					/* File Creation time */
	__le32	i_crtime_extra;				/* extra FileCreationtime (nsec << 2 | epoch) */
	__le32	i_version_hi;				/* high 32 bits for 64-bit version */
	__le32	i_projid;					/* Project ID */
};

#define EXT4_EPOCH_BITS		2
#define EXT4_EPOCH_MASK		((1 << EXT4_EPOCH_BITS) - 1)
#define EXT4_NSEC_MASK		(~0UL << EXT4_EPOCH_BITS)

/*
* Extended fields will fit into an inode if the filesystem was formatted
* with large inodes (-I 256 or larger) and there are not currently any EAs
* consuming all of the available space. For new inodes we always reserve
* enough space for the kernel's known extended fields, but for inodes
* created with an old kernel this might not have been the case. None of
* the extended inode fields is critical for correct filesystem operation.
* This macro checks if a certain field fits in the inode. Note that
* inode-size = GOOD_OLD_INODE_SIZE + i_extra_isize
*/
#define EXT4_FITS_IN_INODE(ext4_inode, einode, field)	\
	((FIELD_OFFSET(struct ext4_inode, field) +	\
	  sizeof((ext4_inode)->field))			\
	<= (EXT4_GOOD_OLD_INODE_SIZE +			\
	    (einode)->i_extra_isize))			\

/*
 * We use an encoding that preserves the times for extra epoch "00":
 *
 * extra  msb of                         adjust for signed
 * epoch  32-bit                         32-bit tv_sec to
 * bits   time    decoded 64-bit tv_sec  64-bit tv_sec      valid time range
 * 0 0    1    -0x80000000..-0x00000001  0x000000000 1901-12-13..1969-12-31
 * 0 0    0    0x000000000..0x07fffffff  0x000000000 1970-01-01..2038-01-19
 * 0 1    1    0x080000000..0x0ffffffff  0x100000000 2038-01-19..2106-02-07
 * 0 1    0    0x100000000..0x17fffffff  0x100000000 2106-02-07..2174-02-25
 * 1 0    1    0x180000000..0x1ffffffff  0x200000000 2174-02-25..2242-03-16
 * 1 0    0    0x200000000..0x27fffffff  0x200000000 2242-03-16..2310-04-04
 * 1 1    1    0x280000000..0x2ffffffff  0x300000000 2310-04-04..2378-04-22
 * 1 1    0    0x300000000..0x37fffffff  0x300000000 2378-04-22..2446-05-10
 *
 * Note that previous versions of the kernel on 64-bit systems would
 * incorrectly use extra epoch bits 1,1 for dates between 1901 and
 * 1970.  e2fsck will correct this, assuming that it is run on the
 * affected filesystem before 2242.
 */

static inline __le32 ext4_encode_extra_time(struct drv_timespec *time)
{
	__u32 extra = sizeof(time->tv_sec) > 4 ?
		((time->tv_sec - (__s32)time->tv_sec) >> 32) & EXT4_EPOCH_MASK : 0;
	return cpu_to_le32(extra | (time->tv_nsec << EXT4_EPOCH_BITS));
}

static inline void ext4_decode_extra_time(struct drv_timespec *time, __le32 extra)
{
	if (sizeof(time->tv_sec) > 4 &&
		(extra & cpu_to_le32(EXT4_EPOCH_MASK))) {
#if 0 /* Do not enable this before kernel version hits 4.20 */
		/* Handle legacy encoding of pre-1970 dates with epoch
		* bits 1,1.  We assume that by kernel version 4.20,
		* everyone will have run fsck over the affected
		* filesystems to correct the problem.  (This
		* backwards compatibility may be removed before this
		* time, at the discretion of the ext4 developers.)
		*/
		__u64 extra_bits = le32_to_cpu(extra) & EXT4_EPOCH_MASK;
		if (extra_bits == 3 && ((time->tv_sec) & 0x80000000) != 0)
			extra_bits = 0;
		time->tv_sec += extra_bits << 32;
#else
		time->tv_sec += (__u64)(le32_to_cpu(extra) & EXT4_EPOCH_MASK) << 32;
#endif
	}
	time->tv_nsec = (le32_to_cpu(extra) & EXT4_NSEC_MASK) >> EXT4_EPOCH_BITS;
}

#define EXT4_INODE_SET_XTIME(xtime, inode, raw_inode)			       \
do {									       \
	(raw_inode)->xtime = cpu_to_le32((inode)->xtime.tv_sec);	       \
	if (EXT4_FITS_IN_INODE(raw_inode, EXT4_I(inode), xtime ## _extra))     \
		(raw_inode)->xtime ## _extra =				       \
				ext4_encode_extra_time(&(inode)->xtime);       \
} while (0)

#define EXT4_EINODE_SET_XTIME(xtime, einode, raw_inode)			       \
do {									       \
	if (EXT4_FITS_IN_INODE(raw_inode, einode, xtime))		       \
		(raw_inode)->xtime = cpu_to_le32((einode)->xtime.tv_sec);      \
	if (EXT4_FITS_IN_INODE(raw_inode, einode, xtime ## _extra))	       \
		(raw_inode)->xtime ## _extra =				       \
				ext4_encode_extra_time(&(einode)->xtime);      \
} while (0)

#define EXT4_INODE_GET_XTIME(xtime, inode, raw_inode)			       \
do {									       \
	(inode)->xtime.tv_sec = (signed)le32_to_cpu((raw_inode)->xtime);       \
	if (EXT4_FITS_IN_INODE(raw_inode, EXT4_I(inode), xtime ## _extra))     \
		ext4_decode_extra_time(&(inode)->xtime,			       \
				       raw_inode->xtime ## _extra);	       \
	else								       \
		(inode)->xtime.tv_nsec = 0;				       \
} while (0)

#define EXT4_EINODE_GET_XTIME(xtime, einode, raw_inode)			       \
do {									       \
	if (EXT4_FITS_IN_INODE(raw_inode, einode, xtime))		       \
		(einode)->xtime.tv_sec = 				       \
			(signed)le32_to_cpu((raw_inode)->xtime);	       \
	else								       \
		(einode)->xtime.tv_sec = 0;				       \
	if (EXT4_FITS_IN_INODE(raw_inode, einode, xtime ## _extra))	       \
		ext4_decode_extra_time(&(einode)->xtime,		       \
				       raw_inode->xtime ## _extra);	       \
	else								       \
		(einode)->xtime.tv_nsec = 0;				       \
} while (0)

/*
* Maximal mount counts between two filesystem checks
*/
#define EXT4_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
#define EXT4_DFL_CHECKINTERVAL			0	/* Don't use interval check */

/*
* Behaviour when detecting errors
*/
#define EXT4_ERRORS_CONTINUE		1	/* Continue execution */
#define EXT4_ERRORS_RO			2	/* Remount fs read-only */
#define EXT4_ERRORS_PANIC			3	/* Panic */
#define EXT4_ERRORS_DEFAULT		EXT4_ERRORS_CONTINUE

/* Metadata checksum algorithm codes */
#define EXT4_CRC32C_CHKSUM		1

/*
* Structure of the super block
*/
struct ext4_super_block {
	/*00*/
	__le32	s_inodes_count;			/* Inodes count */
	__le32	s_blocks_count_lo;			/* Blocks count */
	__le32	s_r_blocks_count_lo;			/* Reserved blocks count */
	__le32	s_free_blocks_count_lo;		/* Free blocks count */
	/*10*/
	__le32	s_free_inodes_count;			/* Free inodes count */
	__le32	s_first_data_block;			/* First Data Block */
	__le32	s_log_block_size;			/* Block size */
	__le32	s_log_cluster_size;			/* Allocation cluster size */
	/*20*/
	__le32	s_blocks_per_group;			/* # Blocks per group */
	__le32	s_clusters_per_group;		/* # Clusters per group */
	__le32	s_inodes_per_group;			/* # Inodes per group */
	__le32	s_mtime;					/* Mount time */
	/*30*/
	__le32	s_wtime;					/* Write time */
	__le16	s_mnt_count;				/* Mount count */
	__le16	s_max_mnt_count;			/* Maximal mount count */
	__le16	s_magic;					/* Magic signature */
	__le16	s_state;					/* File system state */
	__le16	s_errors;					/* Behaviour when detecting errors */
	__le16	s_minor_rev_level;			/* minor revision level */
	/*40*/
	__le32	s_lastcheck;				/* time of last check */
	__le32	s_checkinterval;			/* max. time between checks */
	__le32	s_creator_os;				/* OS */
	__le32	s_rev_level;				/* Revision level */
	/*50*/
	__le16	s_def_resuid;				/* Default uid for reserved blocks */
	__le16	s_def_resgid;				/* Default gid for reserved blocks */
									/*
									 * These fields are for EXT4_DYNAMIC_REV superblocks only.
									 *
									 * Note: the difference between the compatible feature set and
									 * the incompatible feature set is that if there is a bit set
									 * in the incompatible feature set that the kernel doesn't
									 * know about, it should refuse to mount the filesystem.
									 *
									 * e2fsck's requirements are more strict; if it doesn't know
									 * about a feature in either the compatible or incompatible
									 * feature set, it must abort and not try to meddle with
									 * things it doesn't understand...
									 */
	__le32	s_first_ino;				/* First non-reserved inode */
	__le16	s_inode_size;				/* size of inode structure */
	__le16	s_block_group_nr;			/* block group # of this superblock */
	__le32	s_feature_compat;			/* compatible feature set */
	/*60*/
	__le32	s_feature_incompat;			/* incompatible feature set */
	__le32	s_feature_ro_compat;		/* readonly-compatible feature set */
	/*68*/
	__u8		s_uuid[16];				/* 128-bit uuid for volume */
	/*78*/
	char		s_volume_name[16];			/* volume name */
	/*88*/
	char		s_last_mounted[64];			/* directory where last mounted */
	/*C8*/
	__le32	s_algorithm_usage_bitmap;	/* For compression */
									/*
									 * Performance hints.  Directory preallocation should only
									 * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
									 */
	__u8		s_prealloc_blocks;			/* Nr of blocks to try to preallocate*/
	__u8		s_prealloc_dir_blocks;		/* Nr to preallocate for dirs */
	__le16	s_reserved_gdt_blocks;		/* Per group desc for online growth */
									/*
									 * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
									 */
	/*D0*/
	__u8		s_journal_uuid[16];			/* uuid of journal superblock */
	/*E0*/
	__le32	s_journal_inum;				/* inode number of journal file */
	__le32	s_journal_dev;				/* device number of journal file */
	__le32	s_last_orphan;				/* start of list of inodes to delete */
	__le32	s_hash_seed[4];				/* HTREE hash seed */
	__u8		s_def_hash_version;			/* Default hash version to use */
	__u8		s_jnl_backup_type;
	__le16	s_desc_size;				/* size of group descriptor */
	/*100*/
	__le32	s_default_mount_opts;
	__le32	s_first_meta_bg;			/* First metablock block group */
	__le32	s_mkfs_time;				/* When the filesystem was created */
	__le32	s_jnl_blocks[17];			/* Backup of the journal inode */
									/* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
	/*150*/
	__le32	s_blocks_count_hi;			/* Blocks count */
	__le32	s_r_blocks_count_hi;			/* Reserved blocks count */
	__le32	s_free_blocks_count_hi;		/* Free blocks count */
	__le16	s_min_extra_isize;			/* All inodes have at least # bytes */
	__le16	s_want_extra_isize;			/* New inodes should reserve # bytes */
	__le32	s_flags;					/* Miscellaneous flags */
	__le16	s_raid_stride;				/* RAID stride */
	__le16	s_mmp_update_interval;		/* # seconds to wait in MMP checking */
	__le64	s_mmp_block;				/* Block for multi-mount protection */
	__le32	s_raid_stripe_width;			/* blocks on all data disks (N*stride)*/
	__u8		s_log_groups_per_flex;		/* FLEX_BG group size */
	__u8		s_checksum_type;			/* metadata checksum algorithm used */
	__u8		s_encryption_level;			/* versioning level for encryption */
	__u8		s_reserved_pad;			/* Padding to next 32bits */
	__le64	s_kbytes_written;			/* nr of lifetime kilobytes written */
	__le32	s_snapshot_inum;			/* Inode number of active snapshot */
	__le32	s_snapshot_id;				/* sequential ID of active snapshot */
	__le64	s_snapshot_r_blocks_count;	/* reserved blocks for active snapshot's future use */
	__le32	s_snapshot_list;				/* inode number of the head of the on-disk snapshot list */
#define EXT4_S_ERR_START offsetof(struct ext4_super_block, s_error_count)
	__le32	s_error_count;				/* number of fs errors */
	__le32	s_first_error_time;			/* first time an error happened */
	__le32	s_first_error_ino;			/* inode involved in first error */
	__le64	s_first_error_block;			/* block involved of first error */
	__u8		s_first_error_func[32];		/* function where the error happened */
	__le32	s_first_error_line;			/* line number where error happened */
	__le32	s_last_error_time;			/* most recent time of an error */
	__le32	s_last_error_ino;			/* inode involved in last error */
	__le32	s_last_error_line;			/* line number where error happened */
	__le64	s_last_error_block;			/* block involved of last error */
	__u8		s_last_error_func[32];		/* function where the error happened */
#define EXT4_S_ERR_END offsetof(struct ext4_super_block, s_mount_opts)
	__u8		s_mount_opts[64];
	__le32	s_usr_quota_inum;			/* inode for tracking user quota */
	__le32	s_grp_quota_inum;			/* inode for tracking group quota */
	__le32	s_overhead_clusters;			/* overhead blocks/clusters in fs */
	__le32	s_backup_bgs[2];			/* groups with sparse_super2 SBs */
	__u8		s_encrypt_algos[4];			/* Encryption algorithms in use  */
	__u8		s_encrypt_pw_salt[16];		/* Salt used for string2key algorithm */
	__le32	s_lpf_ino;					/* Location of the lost+found inode */
	__le32	s_prj_quota_inum;			/* inode for tracking project quota */
	__le32	s_checksum_seed;			/* crc32c(uuid) if csum_seed set */
	__le32	s_reserved[98];				/* Padding to the end of the block */
	__le32	s_checksum;				/* crc32c(superblock) */
};

#define EXT4_FEATURE_COMPAT_DIR_PREALLOC		0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES		0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR			0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE		0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX			0x0020
#define EXT4_FEATURE_COMPAT_SPARSE_SUPER2		0x0200

#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE		0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR		0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE		0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM		0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK		0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE		0x0040
#define EXT4_FEATURE_RO_COMPAT_QUOTA			0x0100
#define EXT4_FEATURE_RO_COMPAT_BIGALLOC		0x0200
/*
* METADATA_CSUM also enables group descriptor checksums (GDT_CSUM).  When
* METADATA_CSUM is set, group descriptor checksums use the same algorithm as
* all other data structures' checksums.  However, the METADATA_CSUM and
* GDT_CSUM bits are mutually exclusive.
*/
#define EXT4_FEATURE_RO_COMPAT_METADATA_CSUM	0x0400
#define EXT4_FEATURE_RO_COMPAT_READONLY			0x1000
#define EXT4_FEATURE_RO_COMPAT_PROJECT			0x2000

#define EXT4_FEATURE_INCOMPAT_COMPRESSION		0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE				0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER				0x0004 /* Needs recovery */
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV			0x0008 /* Journal device */
#define EXT4_FEATURE_INCOMPAT_META_BG				0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS				0x0040 /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT				0x0080
#define EXT4_FEATURE_INCOMPAT_MMP				0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG				0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE			0x0400 /* EA in inode */
#define EXT4_FEATURE_INCOMPAT_DIRDATA				0x1000 /* data in dirent */
#define EXT4_FEATURE_INCOMPAT_CSUM_SEED			0x2000
#define EXT4_FEATURE_INCOMPAT_LARGEDIR			0x4000 /* >2GB or 3-lvl htree */
#define EXT4_FEATURE_INCOMPAT_INLINE_DATA			0x8000 /* data in inode */
#define EXT4_FEATURE_INCOMPAT_ENCRYPT				0x10000

#define EXT4_FEATURE_COMPAT_FUNCS(name, flagname) \
static inline __bool ext4_has_feature_##name(struct ext4_super_block *s) \
{ \
	return ((s->s_feature_compat & \
		cpu_to_le32(EXT4_FEATURE_COMPAT_##flagname)) != 0); \
} \
static inline void ext4_set_feature_##name(struct ext4_super_block *s) \
{ \
	s->s_feature_compat |= \
		cpu_to_le32(EXT4_FEATURE_COMPAT_##flagname); \
} \
static inline void ext4_clear_feature_##name(struct ext4_super_block *s) \
{ \
	s->s_feature_compat &= \
		~cpu_to_le32(EXT4_FEATURE_COMPAT_##flagname); \
}

#define EXT4_FEATURE_RO_COMPAT_FUNCS(name, flagname) \
static inline __bool ext4_has_feature_##name(struct ext4_super_block *s) \
{ \
	return ((s->s_feature_ro_compat & \
		cpu_to_le32(EXT4_FEATURE_RO_COMPAT_##flagname)) != 0); \
} \
static inline void ext4_set_feature_##name(struct ext4_super_block *s) \
{ \
	s->s_feature_ro_compat |= \
		cpu_to_le32(EXT4_FEATURE_RO_COMPAT_##flagname); \
} \
static inline void ext4_clear_feature_##name(struct ext4_super_block *s) \
{ \
	s->s_feature_ro_compat &= \
		~cpu_to_le32(EXT4_FEATURE_RO_COMPAT_##flagname); \
}

#define EXT4_FEATURE_INCOMPAT_FUNCS(name, flagname) \
static inline __bool ext4_has_feature_##name(struct ext4_super_block *s) \
{ \
	return ((s->s_feature_incompat & \
		cpu_to_le32(EXT4_FEATURE_INCOMPAT_##flagname)) != 0); \
} \
static inline void ext4_set_feature_##name(struct ext4_super_block *s) \
{ \
	s->s_feature_incompat |= \
		cpu_to_le32(EXT4_FEATURE_INCOMPAT_##flagname); \
} \
static inline void ext4_clear_feature_##name(struct ext4_super_block *s) \
{ \
	s->s_feature_incompat &= \
		~cpu_to_le32(EXT4_FEATURE_INCOMPAT_##flagname); \
}

EXT4_FEATURE_COMPAT_FUNCS(dir_prealloc, DIR_PREALLOC)
EXT4_FEATURE_COMPAT_FUNCS(imagic_inodes, IMAGIC_INODES)
EXT4_FEATURE_COMPAT_FUNCS(journal, HAS_JOURNAL)
EXT4_FEATURE_COMPAT_FUNCS(xattr, EXT_ATTR)
EXT4_FEATURE_COMPAT_FUNCS(resize_inode, RESIZE_INODE)
EXT4_FEATURE_COMPAT_FUNCS(dir_index, DIR_INDEX)
EXT4_FEATURE_COMPAT_FUNCS(sparse_super2, SPARSE_SUPER2)

EXT4_FEATURE_RO_COMPAT_FUNCS(sparse_super, SPARSE_SUPER)
EXT4_FEATURE_RO_COMPAT_FUNCS(large_file, LARGE_FILE)
EXT4_FEATURE_RO_COMPAT_FUNCS(btree_dir, BTREE_DIR)
EXT4_FEATURE_RO_COMPAT_FUNCS(huge_file, HUGE_FILE)
EXT4_FEATURE_RO_COMPAT_FUNCS(gdt_csum, GDT_CSUM)
EXT4_FEATURE_RO_COMPAT_FUNCS(dir_nlink, DIR_NLINK)
EXT4_FEATURE_RO_COMPAT_FUNCS(extra_isize, EXTRA_ISIZE)
EXT4_FEATURE_RO_COMPAT_FUNCS(quota, QUOTA)
EXT4_FEATURE_RO_COMPAT_FUNCS(bigalloc, BIGALLOC)
EXT4_FEATURE_RO_COMPAT_FUNCS(metadata_csum, METADATA_CSUM)
EXT4_FEATURE_RO_COMPAT_FUNCS(readonly, READONLY)
EXT4_FEATURE_RO_COMPAT_FUNCS(project, PROJECT)

EXT4_FEATURE_INCOMPAT_FUNCS(compression, COMPRESSION)
EXT4_FEATURE_INCOMPAT_FUNCS(filetype, FILETYPE)
EXT4_FEATURE_INCOMPAT_FUNCS(journal_needs_recovery, RECOVER)
EXT4_FEATURE_INCOMPAT_FUNCS(journal_dev, JOURNAL_DEV)
EXT4_FEATURE_INCOMPAT_FUNCS(meta_bg, META_BG)
EXT4_FEATURE_INCOMPAT_FUNCS(extents, EXTENTS)
EXT4_FEATURE_INCOMPAT_FUNCS(64bit, 64BIT)
EXT4_FEATURE_INCOMPAT_FUNCS(mmp, MMP)
EXT4_FEATURE_INCOMPAT_FUNCS(flex_bg, FLEX_BG)
EXT4_FEATURE_INCOMPAT_FUNCS(ea_inode, EA_INODE)
EXT4_FEATURE_INCOMPAT_FUNCS(dirdata, DIRDATA)
EXT4_FEATURE_INCOMPAT_FUNCS(csum_seed, CSUM_SEED)
EXT4_FEATURE_INCOMPAT_FUNCS(largedir, LARGEDIR)
EXT4_FEATURE_INCOMPAT_FUNCS(inline_data, INLINE_DATA)
EXT4_FEATURE_INCOMPAT_FUNCS(encrypt, ENCRYPT)

#define EXT2_FEATURE_COMPAT_SUPP	EXT4_FEATURE_COMPAT_EXT_ATTR
#define EXT2_FEATURE_INCOMPAT_SUPP	(EXT4_FEATURE_INCOMPAT_FILETYPE| \
					 EXT4_FEATURE_INCOMPAT_META_BG)
#define EXT2_FEATURE_RO_COMPAT_SUPP	(EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 EXT4_FEATURE_RO_COMPAT_LARGE_FILE| \
					 EXT4_FEATURE_RO_COMPAT_BTREE_DIR)

#define EXT3_FEATURE_COMPAT_SUPP	EXT4_FEATURE_COMPAT_EXT_ATTR
#define EXT3_FEATURE_INCOMPAT_SUPP	(EXT4_FEATURE_INCOMPAT_FILETYPE| \
					 EXT4_FEATURE_INCOMPAT_RECOVER| \
					 EXT4_FEATURE_INCOMPAT_META_BG)
#define EXT3_FEATURE_RO_COMPAT_SUPP	(EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 EXT4_FEATURE_RO_COMPAT_LARGE_FILE| \
					 EXT4_FEATURE_RO_COMPAT_BTREE_DIR)

#define EXT4_FEATURE_COMPAT_SUPP	EXT4_FEATURE_COMPAT_EXT_ATTR
#define EXT4_FEATURE_INCOMPAT_SUPP	(EXT4_FEATURE_INCOMPAT_FILETYPE| \
					 EXT4_FEATURE_INCOMPAT_RECOVER| \
					 EXT4_FEATURE_INCOMPAT_META_BG| \
					 EXT4_FEATURE_INCOMPAT_EXTENTS| \
					 EXT4_FEATURE_INCOMPAT_64BIT| \
					 EXT4_FEATURE_INCOMPAT_FLEX_BG| \
					 EXT4_FEATURE_INCOMPAT_MMP | \
					 EXT4_FEATURE_INCOMPAT_INLINE_DATA | \
					 EXT4_FEATURE_INCOMPAT_ENCRYPT | \
					 EXT4_FEATURE_INCOMPAT_CSUM_SEED)
#define EXT4_FEATURE_RO_COMPAT_SUPP	(EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 EXT4_FEATURE_RO_COMPAT_LARGE_FILE| \
					 EXT4_FEATURE_RO_COMPAT_GDT_CSUM| \
					 EXT4_FEATURE_RO_COMPAT_DIR_NLINK | \
					 EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE | \
					 EXT4_FEATURE_RO_COMPAT_BTREE_DIR |\
					 EXT4_FEATURE_RO_COMPAT_HUGE_FILE |\
					 EXT4_FEATURE_RO_COMPAT_BIGALLOC |\
					 EXT4_FEATURE_RO_COMPAT_METADATA_CSUM|\
					 EXT4_FEATURE_RO_COMPAT_QUOTA |\
					 EXT4_FEATURE_RO_COMPAT_PROJECT)

#define EXTN_FEATURE_FUNCS(ver) \
static inline __bool ext4_has_unknown_ext##ver##_compat_features(struct ext4_super_block *s) \
{ \
	return ((s->s_feature_compat & \
		cpu_to_le32(~EXT##ver##_FEATURE_COMPAT_SUPP)) != 0); \
} \
static inline __bool ext4_has_unknown_ext##ver##_ro_compat_features(struct ext4_super_block *s) \
{ \
	return ((s->s_feature_ro_compat & \
		cpu_to_le32(~EXT##ver##_FEATURE_RO_COMPAT_SUPP)) != 0); \
} \
static inline __bool ext4_has_unknown_ext##ver##_incompat_features(struct ext4_super_block *s) \
{ \
	return ((s->s_feature_incompat & \
		cpu_to_le32(~EXT##ver##_FEATURE_INCOMPAT_SUPP)) != 0); \
}

EXTN_FEATURE_FUNCS(2)
EXTN_FEATURE_FUNCS(3)
EXTN_FEATURE_FUNCS(4)

static inline __bool ext4_has_compat_features(struct ext4_super_block *s)
{
	return (s->s_feature_compat != 0);
}
static inline __bool ext4_has_ro_compat_features(struct ext4_super_block *s)
{
	return (s->s_feature_ro_compat != 0);
}
static inline __bool ext4_has_incompat_features(struct ext4_super_block *s)
{
	return (s->s_feature_incompat != 0);
}

/*
* Special inodes numbers
*/
#define EXT4_BAD_INO			1	/* Bad blocks inode */
#define EXT4_ROOT_INO			2	/* Root inode */
#define EXT4_USR_QUOTA_INO	3	/* User quota inode */
#define EXT4_GRP_QUOTA_INO	4	/* Group quota inode */
#define EXT4_BOOT_LOADER_INO	5	/* Boot loader inode */
#define EXT4_UNDEL_DIR_INO		6	/* Undelete directory inode */
#define EXT4_RESIZE_INO		7	/* Reserved group descriptors inode */
#define EXT4_JOURNAL_INO		8	/* Journal inode */

/*
 * First non-reserved inode for old ext4 filesystems
 */
#define EXT4_GOOD_OLD_FIRST_INO	11

/*
 * Maximal count of links to a file
 */
#define EXT4_LINK_MAX				65000

/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT4_MIN_BLOCK_SIZE			1024
#define EXT4_MAX_BLOCK_SIZE			65536
#define EXT4_MIN_BLOCK_LOG_SIZE		10
#define EXT4_MAX_BLOCK_LOG_SIZE		16
#define EXT4_BLOCK_SIZE(s)				\
	(EXT4_MIN_BLOCK_SIZE << le32_to_cpu((s)->s_log_block_size))
#define EXT4_ADDR_PER_BLOCK(s)			\
	(EXT4_BLOCK_SIZE(s) / sizeof(__u32))
#define EXT4_BLOCK_SIZE_BITS(s)			\
	(le32_to_cpu((s)->s_log_block_size) + 10)
#define EXT4_INODE_SIZE(s)				\
	((le32_to_cpu((s)->s_rev_level) == EXT4_GOOD_OLD_REV) ? \
		EXT4_GOOD_OLD_INODE_SIZE :	\
		le16_to_cpu((s)->s_inode_size))
#define EXT4_FIRST_INO(s)				\
	((le32_to_cpu((s)->s_rev_level) == EXT4_GOOD_OLD_REV) ? \
		EXT4_GOOD_OLD_FIRST_INO : \
		le16_to_cpu((s)->s_first_ino))

/*
 * ext4_inode has i_block array (60 bytes total).
 * The first 12 bytes store ext4_extent_header;
 * the remainder stores an array of ext4_extent.
 * For non-inode extent blocks, ext4_extent_tail
 * follows the array.
 */

/*
 * This is the extent tail on-disk structure.
 * All other extent structures are 12 bytes long.  It turns out that
 * block_size % 12 >= 4 for at least all powers of 2 greater than 512, which
 * covers all valid ext4 block sizes.  Therefore, this tail structure can be
 * crammed into the end of the block without having to rebalance the tree.
 */
struct ext4_extent_tail {
	__le32	et_checksum;	/* crc32c(uuid+inum+extent_block) */
};

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
struct ext4_extent {
	__le32	ee_block;		/* first logical block extent covers */
	__le16	ee_len;		/* number of blocks covered by extent */
	__le16	ee_start_hi;	/* high 16 bits of physical block */
	__le32	ee_start_lo;	/* low 32 bits of physical block */
};

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
struct ext4_extent_idx {
	__le32	ei_block;		/* index covers logical blocks from 'block' */
	__le32	ei_leaf_lo;		/* pointer to the physical block of the next level */
	__le16	ei_leaf_hi;		/* high 16 bits of physical block */
	__u16	ei_unused;
};

/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
struct ext4_extent_header {
	__le16	eh_magic;		/* probably will support different formats */
	__le16	eh_entries;	/* number of valid entries */
	__le16	eh_max;		/* capacity of store in entries */
	__le16	eh_depth;		/* has tree real underlying blocks? */
	__le32	eh_generation;	/* generation of the tree */
};

#define EXT4_EXT_MAGIC		cpu_to_le16(0xf30a)

#define EXT4_EXTENT_TAIL_OFFSET(hdr) \
	(sizeof(struct ext4_extent_header) + \
	 (sizeof(struct ext4_extent) * le16_to_cpu((hdr)->eh_max)))

static inline struct ext4_extent_tail *
find_ext4_extent_tail(struct ext4_extent_header *eh)
{
	return (struct ext4_extent_tail *)(((char *)eh) +
		EXT4_EXTENT_TAIL_OFFSET(eh));
}

/*
 * EXT_INIT_MAX_LEN is the maximum number of blocks we can have in an
 * initialized extent. This is 2^15 and not (2^16 - 1), since we use the
 * MSB of ee_len field in the extent datastructure to signify if this
 * particular extent is an initialized extent or an unwritten (i.e.
 * preallocated).
 * EXT_UNWRITTEN_MAX_LEN is the maximum number of blocks we can have in an
 * unwritten extent.
 * If ee_len is <= 0x8000, it is an initialized extent. Otherwise, it is an
 * unwritten one. In other words, if MSB of ee_len is set, it is an
 * unwritten extent with only one special scenario when ee_len = 0x8000.
 * In this case we can not have an unwritten extent of zero length and
 * thus we make it as a special case of initialized extent with 0x8000 length.
 * This way we get better extent-to-group alignment for initialized extents.
 * Hence, the maximum number of blocks we can have in an *initialized*
 * extent is 2^15 (32768) and in an *unwritten* extent is 2^15-1 (32767).
 */
#define EXT_INIT_MAX_LEN	(1UL << 15)
#define EXT_UNWRITTEN_MAX_LEN	(EXT_INIT_MAX_LEN - 1)

#define EXT_FIRST_EXTENT(__hdr__) \
	((struct ext4_extent *) (((char *) (__hdr__)) +		\
				 sizeof(struct ext4_extent_header)))
#define EXT_FIRST_INDEX(__hdr__) \
	((struct ext4_extent_idx *) (((char *) (__hdr__)) +	\
				     sizeof(struct ext4_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__) \
	(le16_to_cpu((__path__)->p_hdr->eh_entries) \
				     < le16_to_cpu((__path__)->p_hdr->eh_max))
#define EXT_LAST_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + le16_to_cpu((__hdr__)->eh_entries) - 1)
#define EXT_LAST_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + le16_to_cpu((__hdr__)->eh_entries) - 1)
#define EXT_MAX_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + le16_to_cpu((__hdr__)->eh_max) - 1)
#define EXT_MAX_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + le16_to_cpu((__hdr__)->eh_max) - 1)

static inline struct ext4_extent_header *ext_inode_hdr(struct ext4_inode *inode)
{
	return (struct ext4_extent_header *)inode->i_block;
}

static inline struct ext4_extent_header *ext_block_hdr(void *data)
{
	return (struct ext4_extent_header *)data;
}

static inline unsigned short ext_depth(struct ext4_inode *inode)
{
	return le16_to_cpu(ext_inode_hdr(inode)->eh_depth);
}

static inline void ext4_ext_mark_unwritten(struct ext4_extent *ext)
{
	/* We can not have an unwritten extent of zero length! */
	NT_ASSERT(!((le16_to_cpu(ext->ee_len) & ~EXT_INIT_MAX_LEN) == 0));
	ext->ee_len |= cpu_to_le16(EXT_INIT_MAX_LEN);
}

static inline int ext4_ext_is_unwritten(struct ext4_extent *ext)
{
	/* Extent with ee_len of 0x8000 is treated as an initialized extent */
	return (le16_to_cpu(ext->ee_len) > EXT_INIT_MAX_LEN);
}

static inline int ext4_ext_get_actual_len(struct ext4_extent *ext)
{
	return (le16_to_cpu(ext->ee_len) <= EXT_INIT_MAX_LEN ?
		le16_to_cpu(ext->ee_len) :
		(le16_to_cpu(ext->ee_len) - EXT_INIT_MAX_LEN));
}

static inline void ext4_ext_mark_initialized(struct ext4_extent *ext)
{
	ext->ee_len = cpu_to_le16(ext4_ext_get_actual_len(ext));
}

/*
 * ext4_ext_pblk:
 * combine low and high parts of physical block number into ext4_fsblk_t
 */
static inline ext4_fsblk_t ext4_ext_pblk(struct ext4_extent *ex)
{
	ext4_fsblk_t block;

	block = le32_to_cpu(ex->ee_start_lo);
	block |= ((ext4_fsblk_t)le16_to_cpu(ex->ee_start_hi) << 31) << 1;
	return block;
}

/*
 * ext4_idx_pblk:
 * combine low and high parts of a leaf physical block number into ext4_fsblk_t
 */
static inline ext4_fsblk_t ext4_idx_pblk(struct ext4_extent_idx *ix)
{
	ext4_fsblk_t block;

	block = le32_to_cpu(ix->ei_leaf_lo);
	block |= ((ext4_fsblk_t)le16_to_cpu(ix->ei_leaf_hi) << 31) << 1;
	return block;
}

/*
 * ext4_ext_lblk:
 * combine low and high parts of physical block number into ext4_fsblk_t
 */
static inline ext4_lblk_t ext4_ext_lblk(struct ext4_extent *ex)
{
	return le32_to_cpu(ex->ee_block);
}

/*
* ext4_idx_lblk:
* combine low and high parts of a leaf physical block number into ext4_fsblk_t
*/
static inline ext4_lblk_t ext4_idx_lblk(struct ext4_extent_idx *ix)
{
	return le32_to_cpu(ix->ei_block);
}

/*
* ext4_ext_store_pblk:
* stores a large physical block number into an extent struct,
* breaking it into parts
*/
static inline void ext4_ext_store_pblk(struct ext4_extent *ex, ext4_fsblk_t pb)
{
	ex->ee_start_lo = cpu_to_le32((unsigned long)(pb & 0xffffffff));
	ex->ee_start_hi = cpu_to_le16((unsigned long)((pb >> 31) >> 1) &
		0xffff);
}

/*
* ext4_idx_store_pblk:
* stores a large physical block number into an index struct,
* breaking it into parts
*/
static inline void ext4_idx_store_pblk(struct ext4_extent_idx *ix, ext4_fsblk_t pb)
{
	ix->ei_leaf_lo = cpu_to_le32((unsigned long)(pb & 0xffffffff));
	ix->ei_leaf_hi = cpu_to_le16((unsigned long)((pb >> 31) >> 1) &
		0xffff);
}

/*
 * ext4_ext_store_lblk:
 * stores a large physical block number into an extent struct,
 * breaking it into parts
 */
static inline void ext4_ext_store_lblk(struct ext4_extent *ex, ext4_lblk_t lb)
{
	ex->ee_block = cpu_to_le32(lb);
}

/*
 * ext4_idx_store_lblk:
 * stores a large physical block number into an index struct,
 * breaking it into parts
 */
static inline void ext4_idx_store_lblk(struct ext4_extent_idx *ix, ext4_lblk_t lb)
{
	ix->ei_block = cpu_to_le32(lb);
}

#pragma pack(pop)