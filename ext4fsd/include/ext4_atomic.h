#pragma once

typedef struct {
	volatile LONG counter;
} atomic_t;

/**
 * @brief	Initialize atomic variable with @p i
 *
 * @param v	Atomic variable to be initialized
 * @param i	Value
 */
#define ext4_atomic_init(v, i)	((v)->counter = (i))

/**
 * @brief	Read atomic variable
 *
 * Atomically reads the value of @p v.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 *
 * @param[in] v	Address to atomic variable
 *
 * @return Value of atomic variable
 */
#define ext4_atomic_read(v)	\
	((v)->counter)

/**
 * @brief	Set atomic variable
 *
 * @param v	pointer of type atomic_t
 * @param i	required value
 *
 * Atomically sets the value of @p v to @p i.
 */
#define ext4_atomic_set(v,i)	\
	InterlockedExchange((PLONG)(&(v)->counter), (LONG)(i))

/**
 * @brief	Add integer to atomic variable
 *
 * @param v	pointer of type atomic_t
 * @param i	integer value to add
 *
 * Atomically adds @p i to @p v.
 */
static inline void ext4_atomic_add(volatile atomic_t *v, volatile int i)
{
	InterlockedExchangeAdd((PLONG)(&v->counter), (LONG)i);
}

/**
 * ext4_atomic_sub - subtract the atomic variable
 *
 * @param v	pointer of type atomic_t
 * @param i	integer value to subtract
 *
 * Atomically subtracts @p i from @p v.
 */
static inline void ext4_atomic_sub(volatile atomic_t *v, volatile int i)
{
	InterlockedExchangeAdd((PLONG)(&v->counter), (LONG)(-1 * i));
}

/**
 * @brief	Subtract value from variable and test result
 *
 * @param v	pointer of type atomic_t
 * @param i	integer value to subtract
 *
 * Atomically subtracts @i from @v.
 *
 * @return	TRUE if the result is zero, or FALSE for all
 *			other cases.
 */
static inline int ext4_atomic_sub_and_test(volatile atomic_t *v, volatile int i)
{
	int counter, result;

	do {
		counter = v->counter;
		result = counter - i;
	} while (InterlockedCompareExchange(
				(PLONG)(&v->counter),
				(LONG)result,
				(LONG)counter) != counter);

	return (result == 0);
}

/**
 * @brief	Increment atomic variable
 *
 * @param v	pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static inline void ext4_atomic_inc(volatile atomic_t *v)
{
	InterlockedIncrement((PLONG)(&v->counter));
}

/**
 * @brief	Decrement atomic variable
 *
 * @param v	pointer of type atomic_t
 *
 * Atomically decrements @p v by 1.
 */
static inline void ext4_atomic_dec(volatile atomic_t *v)
{
	InterlockedDecrement((PLONG)(&v->counter));
}

/**
* @brief	Decrement and test
*
* @param v	pointer of type atomic_t
*
* Atomically decrements @v by 1.
*
* @return		TRUE if the result is 0, or FALSE for all other
*			cases.
*/
static inline int ext4_atomic_dec_and_test(volatile atomic_t *v)
{
	return (0 == InterlockedDecrement((PLONG)(&v->counter)));
}

/**
 * @brief	Increment and test
 *
 * @param v	pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 *
 * @return	TRUE if the result is zero, or FALSE for all
 *			other cases.
 */
static inline int ext4_atomic_inc_and_test(volatile atomic_t *v)
{
	return (0 == InterlockedIncrement((PLONG)(&v->counter)));
}

/**
* ext4_atomic_add_negative - add and test if negative
* @v: pointer of type atomic_t
* @i: integer value to add
*
* Atomically adds @i to @v.
*
* @return		TRUE if the result is negative, or FALSE when
*			result is greater than or equal to zero.
*/
static inline int ext4_atomic_add_negative(volatile int i, volatile atomic_t *v)
{
	return (InterlockedExchangeAdd((PLONG)(&v->counter), (LONG)i) + i);
}