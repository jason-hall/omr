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

#if !defined(METRONOMEDELEGATE_HPP_)
#define METRONOMEDELEGATE_HPP_

#include "omr.h"
#include "omrcfg.h"

#if defined(OMR_GC_REALTIME)

#include "BaseNonVirtual.hpp"
#include "EnvironmentBase.hpp"
#include "EnvironmentRealtime.hpp"
#include "GCExtensionsBase"

class MM_MetronomeDelegate : public MM_BaseNonVirtual
{
private:
	MM_GCExtensionsBase *_extensions;
public:
	MM_MetronomeDelegate(GCExtensionsBase *extensions) :
		_extensions(extensions) {}

	static void yieldWhenRequested(MM_EnvironmentBase *env);
	static void reportStopGCIncrement(MM_EnvironmentRealtime *env);
	static int J9THREAD_PROC metronomeAlarmThreadWrapper(void* userData);
	static uintptr_t signalProtectedFunction(OMRPortLibrary *privatePortLibrary, void* userData);

	bool initialize(MM_EnvironmentBase *env) { return true; }
	bool doTracing(MM_EnvironmentRealtime* env, MM_RealtimeGCDelegate *realtimeDelegate);
	void enableDoubleBarrier(MM_EnvironmentBase* env) {}
	void disableDoubleBarrierOnThread(MM_EnvironmentBase* env, OMR_VMThread *vmThread) {}
	void disableDoubleBarrier(MM_EnvironmentBase* env) {}
	void tearDown(MM_EnvironmentBase *env) {}
	void masterSetupForGC(MM_EnvironmentBase *env) {}
	void masterCleanupAfterGC(MM_EnvironmentBase *env) {}
	void incrementalCollect(MM_EnvironmentRealtime *env) {}
	void doAuxilaryGCWork(MM_EnvironmentBase *env) {}
	void clearGCStats(MM_EnvironmentBase *env);
};

#endif /* defined(OMR_GC_REALTIME) */

#endif /* defined(MEMORYSUBSPACEMETRONOMEDELEGATE_HPP_) */

