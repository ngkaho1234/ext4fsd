#pragma once

#include <ntifs.h>

#define cpu_to_le16(x)	((__le16)(x))
#define cpu_to_le32(x)	((__le32)(x))
#define cpu_to_le64(x)	((__le64)(x))

#define cpu_to_be16(x)	((__be16)RtlUshortByteSwap(x))
#define cpu_to_be32(x)	((__be32)RtlUlongByteSwap(x))
#define cpu_to_be64(x)	((__be64)RtlUlonglongByteSwap(x))

#define le16_to_cpu(x)	((__u16)(x))
#define le32_to_cpu(x)	((__u32)(x))
#define le64_to_cpu(x)	((__u64)(x))

#define be16_to_cpu(x)	((__u16)RtlUshortByteSwap(x))
#define be32_to_cpu(x)	((__u32)RtlUlongByteSwap(x))
#define be64_to_cpu(x)	((__u64)RtlUlonglongByteSwap(x))

static inline void le16_add_cpu(__le16 *var, __u16 val)
{
	*var = cpu_to_le16(le16_to_cpu(*var) + val);
}

static inline void le32_add_cpu(__le32 *var, __u32 val)
{
	*var = cpu_to_le32(le32_to_cpu(*var) + val);
}

static inline void le64_add_cpu(__le64 *var, __u64 val)
{
	*var = cpu_to_le64(le64_to_cpu(*var) + val);
}

static inline void be16_add_cpu(__be16 *var, __u16 val)
{
	*var = cpu_to_be16(be16_to_cpu(*var) + val);
}

static inline void be32_add_cpu(__be32 *var, __u32 val)
{
	*var = cpu_to_be32(be32_to_cpu(*var) + val);
}

static inline void be64_add_cpu(__be64 *var, __u64 val)
{
	*var = cpu_to_be64(be64_to_cpu(*var) + val);
}