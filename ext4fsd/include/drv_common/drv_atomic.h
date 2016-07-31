#pragma once

typedef struct {
	volatile LONG counter;
} drv_atomic_t;

/**
 * @brief	Initialize atomic variable with @p i
 *
 * @param v	Atomic variable to be initialized
 * @param i	Value
 */
#define drv_atomic_init(v, i)	((v)->counter = (i))

/**
 * @brief	Read atomic variable
 *
 * Atomically reads the value of @p v.  Note that the guaranteed
 * useful range of an drv_atomic_t is only 24 bits.
 *
 * @param[in] v	Address to atomic variable
 *
 * @return Value of atomic variable
 */
#define drv_atomic_read(v)	\
	((v)->counter)

/**
 * @brief	Set atomic variable
 *
 * @param v	pointer of type drv_atomic_t
 * @param i	required value
 *
 * Atomically sets the value of @p v to @p i.
 */
#define drv_atomic_set(v,i)	\
	InterlockedExchange((PLONG)(&(v)->counter), (LONG)(i))

/**
 * @brief	Add integer to atomic variable
 *
 * @param v	pointer of type drv_atomic_t
 * @param i	integer value to add
 *
 * Atomically adds @p i to @p v.
 */
static inline void drv_atomic_add(volatile drv_atomic_t *v, volatile int i)
{
	InterlockedExchangeAdd((PLONG)(&v->counter), (LONG)i);
}

/**
 * drv_atomic_sub - subtract the atomic variable
 *
 * @param v	pointer of type drv_atomic_t
 * @param i	integer value to subtract
 *
 * Atomically subtracts @p i from @p v.
 */
static inline void drv_atomic_sub(volatile drv_atomic_t *v, volatile int i)
{
	InterlockedExchangeAdd((PLONG)(&v->counter), (LONG)(-1 * i));
}

/**
 * @brief	Subtract value from variable and test result
 *
 * @param v	pointer of type drv_atomic_t
 * @param i	integer value to subtract
 *
 * Atomically subtracts @i from @v.
 *
 * @return	TRUE if the result is zero, or FALSE for all
 *			other cases.
 */
static inline int drv_atomic_sub_and_test(volatile drv_atomic_t *v, volatile int i)
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
 * @param v	pointer of type drv_atomic_t
 *
 * Atomically increments @v by 1.
 */
static inline void drv_atomic_inc(volatile drv_atomic_t *v)
{
	InterlockedIncrement((PLONG)(&v->counter));
}

/**
 * @brief	Decrement atomic variable
 *
 * @param v	pointer of type drv_atomic_t
 *
 * Atomically decrements @p v by 1.
 */
static inline void drv_atomic_dec(volatile drv_atomic_t *v)
{
	InterlockedDecrement((PLONG)(&v->counter));
}

/**
* @brief	Decrement and test
*
* @param v	pointer of type drv_atomic_t
*
* Atomically decrements @v by 1.
*
* @return		TRUE if the result is 0, or FALSE for all other
*			cases.
*/
static inline int drv_atomic_dec_and_test(volatile drv_atomic_t *v)
{
	return (0 == InterlockedDecrement((PLONG)(&v->counter)));
}

/**
 * @brief	Increment and test
 *
 * @param v	pointer of type drv_atomic_t
 *
 * Atomically increments @v by 1.
 *
 * @return	TRUE if the result is zero, or FALSE for all
 *			other cases.
 */
static inline int drv_atomic_inc_and_test(volatile drv_atomic_t *v)
{
	return (0 == InterlockedIncrement((PLONG)(&v->counter)));
}

/**
* drv_atomic_add_negative - add and test if negative
* @v: pointer of type drv_atomic_t
* @i: integer value to add
*
* Atomically adds @i to @v.
*
* @return		TRUE if the result is negative, or FALSE when
*			result is greater than or equal to zero.
*/
static inline int drv_atomic_add_negative(volatile int i, volatile drv_atomic_t *v)
{
	return (InterlockedExchangeAdd((PLONG)(&v->counter), (LONG)i) + i);
}