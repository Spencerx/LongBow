/*
 * Copyright (c) 2013-2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ################################################################################
 * #
 * # PATENT NOTICE
 * #
 * # This software is distributed under the BSD 2-clause License (see LICENSE
 * # file).  This BSD License does not make any patent claims and as such, does
 * # not act as a patent grant.  The purpose of this section is for each contributor
 * # to define their intentions with respect to intellectual property.
 * #
 * # Each contributor to this source code is encouraged to state their patent
 * # claims and licensing mechanisms for any contributions made. At the end of
 * # this section contributors may each make their own statements.  Contributor's
 * # claims and grants only apply to the pieces (source code, programs, text,
 * # media, etc) that they have contributed directly to this software.
 * #
 * # There is no guarantee that this section is complete, up to date or accurate. It
 * # is up to the contributors to maintain their portion of this section and up to
 * # the user of the software to verify any claims herein.
 * #
 * # Do not remove this header notification.  The contents of this section must be
 * # present in all distributions of the software.  You may only modify your own
 * # intellectual property statements.  Please provide contact information.
 * 
 * - Palo Alto Research Center, Inc
 * This software distribution does not grant any rights to patents owned by Palo
 * Alto Research Center, Inc (PARC). Rights to these patents are available via
 * various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
 * intellectual property used by its contributions to this software. You may
 * contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
 */
/**
 * @author Glenn Scott, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2013-2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */
#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>

#include <LongBow/longBow_Runtime.h>
#include <LongBow/longBow_Backtrace.h>
#include <LongBow/longBow_Location.h>
#include <LongBow/longBow_Event.h>
#include <LongBow/longBow_Config.h>

#include <LongBow/Reporting/longBowReport_Runtime.h>

#include <LongBow/private/longBow_Memory.h>

static unsigned int longBowStackTraceDepth = 128;

struct longbow_runtime {
    LongBowConfig *config;
    LongBowRuntimeResult expectedResult;
    LongBowRuntimeResult actualResult;
};

static LongBowRuntime longBowRuntimeGlobal;

static LongBowRuntime *longBowCurrentRuntime = &longBowRuntimeGlobal;


static char *
_longBowRuntime_FormatErrnoMessage(void)
{
    char *result = NULL;
    if (errno != 0) {
        if (asprintf(&result, "%s: ", strerror(errno)) == -1) {
            return NULL;
        }
    }
    return result;
}

static char *
_longBowRuntime_FormatMessage(const char *format, va_list args)
{
    char *errnoMessage = _longBowRuntime_FormatErrnoMessage();

    char *string;
    if (vasprintf(&string, format, args) == -1) {
        return NULL;
    }

    char *result = NULL;

    if (errnoMessage != NULL) {
        int check = asprintf(&result, "%s%s", errnoMessage, string);
        free(errnoMessage);
        if (check == -1) {
            return NULL;
        }
    } else {
        result = strndup(string, strlen(string));
    }

    free(string);

    return result;
}

LongBowRuntime *
longBowRuntime_Create(const LongBowRuntimeResult *expectedResultTemplate, LongBowConfig *config)
{
    assert(expectedResultTemplate != NULL);

    LongBowRuntime *result = longBowMemory_Allocate(sizeof(LongBowRuntime));
    if (result != NULL) {
        result->expectedResult = *expectedResultTemplate;
        result->config = config;
    }
    return result;
}

size_t
longBowRuntime_GetActualEventEvaluationCount(LongBowRuntime *runtime)
{
    return longBowRuntimeResult_GetEventEvaluationCount(longBowRuntime_GetActualTestCaseResult(runtime));
}

void
longBowRuntime_Destroy(LongBowRuntime **runtimePtr)
{
    longBowMemory_Deallocate((void **) runtimePtr);
}

LongBowRuntimeResult *
longBowRuntime_GetExpectedTestCaseResult(const LongBowRuntime *const runtime)
{
    return (LongBowRuntimeResult *) &(runtime->expectedResult);
}

LongBowRuntimeResult *
longBowRuntime_GetActualTestCaseResult(const LongBowRuntime *runtime)
{
    return (LongBowRuntimeResult *) &(runtime->actualResult);
}

LongBowRuntime *
longBowRuntime_SetCurrentRuntime(LongBowRuntime *runtime)
{
    LongBowRuntime *result = longBowCurrentRuntime;
    longBowCurrentRuntime = runtime;
    return result;
}

LongBowRuntime *
longBowRuntime_GetCurrentRuntime(void)
{
    return longBowCurrentRuntime;
}

LongBowConfig *
longBowRuntime_GetCurrentConfig(void)
{
    return longBowCurrentRuntime->config;
}

void
longBowRuntime_SetCurrentConfig(LongBowConfig *config)
{
    longBowCurrentRuntime->config = config;
}

LongBowEventType *
longBowRuntime_GetActualEventType(const LongBowRuntime *runtime)
{
    return runtime->actualResult.event;
}

LongBowEventType *
longBowRuntime_GetExpectedEventType(const LongBowRuntime *runtime)
{
    return runtime->expectedResult.event;
}

void
longBowRuntime_SetActualEventType(LongBowRuntime *runtime, LongBowEventType *eventType)
{
    runtime->actualResult.event = eventType;
}

bool
longBowRuntime_EventEvaluation(const LongBowEventType *type)
{
    longBowCurrentRuntime->actualResult.eventEvaluationCount++;
    return true;
}

unsigned int
longBowRuntime_SetStackTraceDepth(unsigned int newDepth)
{
    unsigned int previousValue = longBowStackTraceDepth;
    longBowStackTraceDepth = newDepth;
    return previousValue;
}

unsigned int
longBowRuntime_GetStackTraceDepth(void)
{
    return longBowStackTraceDepth;
}

bool
longBowRuntime_EventTrigger(LongBowEventType *eventType, LongBowLocation *location, const char *kind, const char *format, ...)
{
    LongBowRuntime *runtime = longBowRuntime_GetCurrentRuntime();

    longBowRuntime_SetActualEventType(runtime, eventType);

    if (runtime->expectedResult.status == LONGBOW_STATUS_FAILED) {
        return true;
    }

    if (longBowEventType_Equals(longBowRuntime_GetActualEventType(runtime), longBowRuntime_GetExpectedEventType(runtime))) {
        return true;
    }

    va_list args;
    va_start(args, format);

    char *messageString = _longBowRuntime_FormatMessage(format, args);

    va_end(args);

    LongBowBacktrace *stackTrace = longBowBacktrace_Create(longBowRuntime_GetStackTraceDepth(), 2);

    LongBowEvent *event = longBowEvent_Create(eventType, location, kind, messageString, stackTrace);

    free(messageString);
    longBowReportRuntime_Event(event);
    longBowEvent_Destroy(&event);
    return true;
}

void
longBowRuntime_StackTrace(int fileDescriptor)
{
    LongBowBacktrace *backtrace = longBowBacktrace_Create(longBowRuntime_GetStackTraceDepth(), 1);
    char *string = longBowBacktrace_ToString(backtrace);
    fwrite(string, strlen(string), 1, stdout);
    longBowMemory_Deallocate((void **) &string);
    longBowBacktrace_Destroy(&backtrace);
}

bool
longBowRuntime_TestAddressIsAligned(const void *address, size_t alignment)
{
    if ((alignment & (~alignment + 1)) == alignment) {
        return (((uintptr_t) address) % alignment) == 0 ? true : false;
    }
    return false;
}

void
longBowRuntime_CoreDump(void)
{
    struct rlimit coreDumpLimit;

    coreDumpLimit.rlim_cur = RLIM_INFINITY;
    coreDumpLimit.rlim_max = RLIM_INFINITY;

    if (setrlimit(RLIMIT_CORE, &coreDumpLimit) < 0) {
        fprintf(stderr, "setrlimit: %s\n", strerror(errno));
        exit(1);
    }
    kill(0, SIGTRAP);
}

void
longBowRuntime_Abort(void)
{
    bool coreDump = longBowConfig_GetBoolean(longBowRuntime_GetCurrentConfig(), false, "core-dump");
    if (coreDump == false) {
        abort();
    } else {
        longBowRuntime_CoreDump();
    }
}
