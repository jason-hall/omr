/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/


#include "MetronomeDelegate.hpp"

#if defined(OMR_GC_REALTIME)

#include "omr.h"
#include "omrvm.h"

#include "EnvironmentRealtime.hpp"
#include "GCExtensionsBase.hpp"
#include "Heap.hpp"
#include "HeapRegionDescriptorRealtime.hpp"
#include "MetronomeAlarmThread.hpp"
#include "RealtimeGC.hpp"
#include "RealtimeMarkingScheme.hpp"
#include "Scheduler.hpp"

void
MM_MetronomeDelegate::yieldWhenRequested(MM_EnvironmentBase *env)
{
}

/**
 * C entrypoint for the newly created alarm thread.
 */
int J9THREAD_PROC
MM_MetronomeDelegate::metronomeAlarmThreadWrapper(void* userData)
{
	MM_MetronomeAlarmThread *alarmThread = (MM_MetronomeAlarmThread *)userData;
	OMR_VM *omrVM = alarmThread->getScheduler()->_extensions->getOmrVM();
	OMRPORT_ACCESS_FROM_OMRVM(omrVM);
	UDATA rc;

	omrsig_protect(MM_MetronomeDelegate::signalProtectedFunction, (void*)userData,
		((MM_ParallelDispatcher *)alarmThread->getScheduler()->_extensions->dispatcher)->getSignalHandler(), omrVM,
		OMRPORT_SIG_FLAG_SIGALLSYNC | OMRPORT_SIG_FLAG_MAY_CONTINUE_EXECUTION, 
		&rc);
		
	omrthread_monitor_enter(alarmThread->_mutex);
	alarmThread->_alarmThreadActive = MM_MetronomeAlarmThread::ALARM_THREAD_SHUTDOWN;
	omrthread_monitor_notify(alarmThread->_mutex);
	omrthread_exit(alarmThread->_mutex);
		
	return 0;
}

uintptr_t
MM_MetronomeDelegate::signalProtectedFunction(OMRPortLibrary *privatePortLibrary, void* userData)
{
	MM_MetronomeAlarmThread *alarmThread = (MM_MetronomeAlarmThread *)userData;
	OMR_VM *omrVM = alarmThread->getScheduler()->_extensions->getOmrVM();
	OMR_VMThread *vmthread = NULL;

	if (OMR_ERROR_NONE != OMR_Thread_Init(omrVM, NULL, &vmthread, "GC Alarm")) {
		return 0;
	}

	MM_EnvironmentRealtime *env = MM_EnvironmentRealtime::getEnvironment(vmthread);

	alarmThread->run(env);

	OMR_Thread_Free(vmthread);

	return 0;
}

void
MM_MetronomeDelegate::clearGCStats(MM_EnvironmentBase *env)
{
	_extensions->globalGCStats.clear();
}

bool
MM_MetronomeDelegate::initialize(MM_EnvironmentBase *env)
{
	return true;
}

void
MM_MetronomeDelegate::tearDown(MM_EnvironmentBase *env)
{
}

void
MM_MetronomeDelegate::masterSetupForGC(MM_EnvironmentBase *env)
{
}

void
MM_MetronomeDelegate::masterCleanupAfterGC(MM_EnvironmentBase *env)
{
}

void
MM_MetronomeDelegate::incrementalCollectStart(MM_EnvironmentRealtime *env)
{
}

void
MM_MetronomeDelegate::incrementalCollect(MM_EnvironmentRealtime *env)
{
}

void
MM_MetronomeDelegate::doAuxilaryGCWork(MM_EnvironmentBase *env)
{
}

void
MM_MetronomeDelegate::reportSyncGCEnd(MM_EnvironmentBase *env)
{
}

/**
 * Iterates over all threads and enables the double barrier for each thread by setting the
 * remebered set fragment index to the reserved index.
 */
void
MM_MetronomeDelegate::enableDoubleBarrier(MM_EnvironmentBase *env)
{
}

/**
 * Disables the double barrier for the specified thread.
 */
void
MM_MetronomeDelegate::disableDoubleBarrierOnThread(MM_EnvironmentBase* env, OMR_VMThread* vmThread)
{
}

/**
 * Disables the global double barrier flag. This should be called after all threads have been scanned
 * and disableDoubleBarrierOnThread has been called on each of them.
 */
void
MM_MetronomeDelegate::disableDoubleBarrier(MM_EnvironmentBase* env)
{
}

bool
MM_MetronomeDelegate::doTracing(MM_EnvironmentRealtime* env)
{
	return false;
}

void
MM_MetronomeDelegate::defaultMemorySpaceAllocated(MM_GCExtensionsBase *extensions, void* defaultMemorySpace)
{
}

void
MM_MetronomeDelegate::clearGCStatsEnvironment(MM_EnvironmentRealtime *env)
{
}

void
MM_MetronomeDelegate::mergeGCStats(MM_EnvironmentRealtime *env)
{
}

uintptr_t
MM_MetronomeDelegate::getSplitArraysProcessed(MM_EnvironmentRealtime *env)
{
	return 0;
}

uintptr_t
scanPointerArraylet(MM_EnvironmentRealtime *env, fomrobject_t *arraylet)
{
	return 0;
}

uintptr_t
scanObject(MM_EnvironmentRealtime *env, omrobjectptr_t objectPtr)
{
	return 0;
}

void
markLiveObjects(MM_EnvironmentRealtime *env)
{
}

#endif /* defined(OMR_GC_REALTIME) */

