/*******************************************************************************
 * Copyright (c) 2019 IBM Corp. and others
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

#if !defined(STACCATOGCDELEGATE_HPP_)
#define STACCATOGCDELEGATE_HPP_

#include "omrcfg.h"
#if defined(OMR_GC_STACCATO)
#include "omr.h"
#include "EnvironmentRealtime.hpp"
#include "RealtimeGCDelegate.hpp"

class MM_RealtimeAccessBarrier;

class MM_StaccatoGC;

class MM_StaccatoGCDelegate
{
private:
	OMR_VM *_vm;
	MM_StaccatoGC *_staccatoGC;

public:
	MM_StaccatoGCDelegate(MM_EnvironmentBase *env, MM_StaccatoGC *staccatoGC) :
		_vm(env->getOmrVM()),
		_staccatoGC(staccatoGC) {}

	virtual MM_RealtimeAccessBarrier *allocateAccessBarrier(MM_EnvironmentBase *env) { return NULL; }
	virtual bool doTracing(MM_EnvironmentRealtime* env, MM_RealtimeGCDelegate *realtimeDelegate);
	virtual void enableDoubleBarrier(MM_EnvironmentBase* env) {}
	virtual void disableDoubleBarrierOnThread(MM_EnvironmentBase* env, OMR_VMThread *vmThread) {}
	virtual void disableDoubleBarrier(MM_EnvironmentBase* env) {}
};

#endif  /* defined(OMR_GC_STACCATO) */

#endif /* defined(STACCATOGCDELEGATE_HPP_) */	
