
/*******************************************************************************
 * Copyright (c) 1991, 2018 IBM Corp. and others
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


#include "RealtimeRootScanner.hpp"

#if defined(OMR_GC_REALTIME)
#include "EnvironmentBase.hpp"
#include "RootScanner.hpp"
#include "Scheduler.hpp"

#include <string.h>

/**
 * This function iterates through all the threads, calling scanOneThread on each one that
 * should be scanned.  The scanOneThread function scans exactly one thread and returns
 * either true (if it took an action that requires the thread list iterator to return to
 * the beginning) or false (if the thread list iterator should just continue with the next
 * thread).
 */
void
MM_RealtimeRootScanner::scanThreads(MM_EnvironmentBase *env)
{
	reportScanningStarted(RootScannerEntity_Threads);

	GC_OMRVMThreadListIterator vmThreadListIterator(_omrVM);
	StackIteratorData localData;

	localData.rootScanner = this;
	localData.env = env;

	while(OMR_VMThread *walkThread = vmThreadListIterator.nextOMRVMThread()) {
 		MM_EnvironmentRealtime* walkThreadEnv = MM_EnvironmentRealtime::getEnvironment(walkThread);
		if (GC_UNMARK == walkThreadEnv->_allocationColor) {
			if (GC_UNMARK == MM_AtomicOperations::lockCompareExchangeU32(&walkThreadEnv->_allocationColor, GC_UNMARK, GC_MARK)) {
				if (scanOneThread(env, walkThread, (void*) &localData)) {
					vmThreadListIterator.reset(_omrVM->_vmThreadList);
				}
			}
		}
 	}

	reportScanningEnded(RootScannerEntity_Threads);
}

/**
 * The following override of scanOneThread performs metronome-specific processing before
 * and after the scanning of each thread.  Scanning is skipped if the thread has already
 * been scanned in this cycle.
 **/
bool
MM_RealtimeRootScanner::scanOneThread(MM_EnvironmentBase *envBase, OMR_VMThread* walkThread, void* localData)
{
	MM_EnvironmentRealtime *env = MM_EnvironmentRealtime::getEnvironment(envBase);
	
	scanOneThreadImpl(env, walkThread, localData);

	/* Thead count is used under verbose only.
	 * Avoid the atomic add in the regular path.
	 */
	if (_realtimeGC->_sched->verbose() >= 3) {
		MM_AtomicOperations::add(&_threadCount, 1);
	}
	
	if (condYield()) {
		/* Optionally issue verbose message */
		if (_realtimeGC->_sched->verbose() >= 3) {
			OMRPORT_ACCESS_FROM_ENVIRONMENT(env);
			omrtty_printf("Yielded during %s after scanning %d threads\n", scannerName(), _threadCount);
		}
		
		return true;
	}

	return false;
}

void
MM_RealtimeRootScanner::scanOneThreadImpl(MM_EnvironmentRealtime *env, OMR_VMThread* walkThread, void* localData)
{
}

void
MM_RealtimeRootScanner::reportThreadCount(MM_EnvironmentBase* env)
{
	OMRPORT_ACCESS_FROM_ENVIRONMENT(env);
	omrtty_printf("Scanned %d threads for %s\n", _threadCount, scannerName());
}

void
MM_RealtimeRootScanner::scanAtomicRoots(MM_EnvironmentRealtime *env)
{
}

/**
 * Calls the Scheduler's yielding API to determine if the GC should yield.
 * @return true if the GC should yield, false otherwise
 */ 
bool
MM_RealtimeRootScanner::shouldYield()
{
	return _realtimeGC->_sched->shouldGCYield(_env, 0);
}

/**
 * Yield from GC by calling the Scheduler's API. Also resets the yield count.
 * @note this does the same thing as condYield(). It should probably just call
 * the Scheduler's yield() method to clear up ambiguity but it's been left
 * untouched for reasons motivated purely by touching the least amount of code.
 */
void
MM_RealtimeRootScanner::yield()
{
	_realtimeGC->_sched->condYieldFromGC(_env);
	_yieldCount = ROOT_GRANULARITY;
}

/**
 * Yield only if the Scheduler deems yielding should occur at the time of the
 * call to this method.
 */ 
bool
MM_RealtimeRootScanner::condYield(U_64 timeSlackNanoSec)
{
	bool yielded = _realtimeGC->_sched->condYieldFromGC(_env, timeSlackNanoSec);
	_yieldCount = ROOT_GRANULARITY;
	return yielded;
}

#endif  /* defined(OMR_GC_REALTIME) */

