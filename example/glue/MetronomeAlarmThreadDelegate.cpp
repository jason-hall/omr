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

#include "MetronomeAlarmThreadDelegate.hpp"

#if defined(J9VM_GC_REALTIME)

#include "omr.h"
#include "EnvironmentRealtime.hpp"
#include "GCExtensions.hpp"
#include "MetronomeAlarmThread.hpp"

uintptr_t
MM_MetronomeAlarmThreadDelegate::signalProtectedFunction(OMRPortLibrary *privatePortLibrary, void* userData)
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

/**
 * C entrypoint for the newly created alarm thread.
 */
int J9THREAD_PROC
MM_MetronomeAlarmThread::metronomeAlarmThreadWrapper(void* userData)
{
	MM_MetronomeAlarmThread *alarmThread = (MM_MetronomeAlarmThread *)userData;
	OMR_VM *omrVM = alarmThread->getScheduler()->_extensions->getOmrVM();
	OMRPORT_ACCESS_FROM_OMRVM(omrVM);
	UDATA rc;

	omrsig_protect(MM_MetronomeAlarmThread::signalProtectedFunction, (void*)userData, 
		((MM_ParallelDispatcher *)alarmThread->getScheduler()->_extensions->dispatcher)->getSignalHandler(), omrVM,
		OMRPORT_SIG_FLAG_SIGALLSYNC | OMRPORT_SIG_FLAG_MAY_CONTINUE_EXECUTION, 
		&rc);
		
	omrthread_monitor_enter(alarmThread->_mutex);
	alarmThread->_alarmThreadActive = ALARM_THREAD_SHUTDOWN;
	omrthread_monitor_notify(alarmThread->_mutex);
	omrthread_exit(alarmThread->_mutex);
		
	return 0;
}

#endif /* defined(J9VM_GC_REALTIME) */

