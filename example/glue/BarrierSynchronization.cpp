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

#include "omr.h"

#include "BarrierSynchronization.hpp"

/**
 * Static method for instance creation.
 */
MM_BarrierSynchronization*
MM_BarrierSynchronization::newInstance(MM_EnvironmentBase *env)
{
	MM_BarrierSynchronization *barrierSync;
	
	barrierSync = (MM_BarrierSynchronization *)env->getForge()->allocate(sizeof(MM_BarrierSynchronization), MM_AllocationCategory::FIXED, OMR_GET_CALLSITE());
	if (barrierSync) {
		new(barrierSync) MM_BarrierSynchronization(env);
		if (!barrierSync->initialize(env)) {
			barrierSync->kill(env);		
			barrierSync = NULL;   			
		}
	}
	return barrierSync;
}

/**
 * Initialization of internal fields.
 */
bool
MM_BarrierSynchronization::initialize(MM_EnvironmentBase *env)
{
	return true;
}

/**
 * Release resources.
 */
void
MM_BarrierSynchronization::kill(MM_EnvironmentBase *env)
{
	tearDown(env);
	env->getForge()->free(this);
}

/**
 * Teardown
 */
void
MM_BarrierSynchronization::tearDown(MM_EnvironmentBase *env)
{
}

/**
 * This function has to be called at the beginning of continueGC because requestExclusiveVMAccess 
 * assumes the current OMR_VMThread does not have VM Access.  All java threads that cause a GC (either
 * System.gc or allocation failure) will have VM access when entering the GC so we have to give it up.
 * 
 * @param threadRequestingExclusive the OMR_VMThread for the MetronomeGCThread that will
 * be requesting exclusive vm access.
 */
void
MM_BarrierSynchronization::preRequestExclusiveVMAccess(OMR_VMThread *threadRequestingExclusive)
{
}

/**
 * This function is called when leaving continueGC so the OMR_VMThread associated with current 
 * MetronomeGCThread will get its VM Access back before returning to run Java code.
 * 
 * @param threadRequestingExclusive the OMR_VMThread for the MetronomeGCThread that requested 
 * exclusive vm access.
 */
void
MM_BarrierSynchronization::postRequestExclusiveVMAccess(OMR_VMThread *threadRequestingExclusive)
{
}



/**
 * A call to requestExclusiveVMAccess must be followed by a call to waitForExclusiveVMAccess,
 * but not necessarily by the same thread.
 *
 * @param env the requesting thread.
 * @param block boolean input paramter specifing whether we should block and wait, if another party is requesting at the same time, or we return
 * @return boolean returning whether request was successful or not (make sense only if block is set to FALSE)
 */
UDATA
MM_BarrierSynchronization::requestExclusiveVMAccess(MM_EnvironmentBase *env, UDATA block, UDATA *gcPriority)
{
	return TRUE;
}

/*
 * Block until the earlier request for exclusive VM access completes.
 * @note This can only be called by the MasterGC thread.
 * @param The requesting thread.
 */
void 
MM_BarrierSynchronization::waitForExclusiveVMAccess(MM_EnvironmentBase *env, bool waitRequired)
{
	++(omr_vmthread_getCurrent(env->getOmrVM())->exclusiveCount);
}

/* 
 * Acquire (request and block until success) exclusive VM access.
 * @note This can only be called by the MasterGC thread.
 * @param The requesting thread.
 */
void 
MM_BarrierSynchronization::acquireExclusiveVMAccess(MM_EnvironmentBase *env, bool waitRequired)
{
	++(omr_vmthread_getCurrent(env->getOmrVM())->exclusiveCount);
}

/* 
 * Release the held exclusive VM access.
 * @note This can only be called by the MasterGC thread.
 * @param The requesting thread.
 */
void 
MM_BarrierSynchronization::releaseExclusiveVMAccess(MM_EnvironmentBase *env, bool releaseRequired)
{
	--(omr_vmthread_getCurrent(env->getOmrVM())->exclusiveCount);
	if (releaseRequired) {
		/* Set the exclusive access response counts to an unusual value,
		 * just for debug purposes, so we can detect scenarios, when master
		 * thread is waiting for Xaccess with noone requesting it before.
		 */
		_vmResponsesRequiredForExclusiveVMAccess = 0x7fffffff;
		_jniResponsesRequiredForExclusiveVMAccess = 0x7fffffff;
	}
}
