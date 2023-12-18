#ifndef CDH_ASSERTS_H
#define CDH_ASSERTS_H

/*
Fancy macros for asserts that have behavior in non-debug
mode such as returning a value or setting a default if
the condition fails.
*/

#include <assert.h>

#ifdef DEBUG

    #define assertOrReturn(conditionalExpression) \
        assert((conditionalExpression))
    #define assertOrReturnValue(conditionalExpression, value) \
        assert((conditionalExpression))
    #define assertOrExecute(conditionalExpression, executableExpression) \
        assert((conditionalExpression))

#else

    #define assertOrReturn(conditionalExpression) \
        if(!(conditionalExpression)) return
    #define assertOrReturnValue(conditionalExpression, value) \
        if(!(conditionalExpression)) return (value)
    #define assertOrExecute(conditionalExpression, executableExpression) \
        if(!(conditionalExpression)) (executableExpression)

#endif // DEBUG

#endif // CDH_ASSERTS_H