
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

#include "StackSlotValidator.hpp"

#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"

void 
MM_StackSlotValidator::threadCrash(MM_EnvironmentBase* env)
{
	reportStackSlot(env, "Unhandled exception while validating object in stack frame");
}

void
MM_StackSlotValidator::reportStackSlot(MM_EnvironmentBase* env, const char* message)
{
	OMRPORT_ACCESS_FROM_ENVIRONMENT(env);
	OMR_VMThread *vmthread = env->getOmrVMThread();
	Trc_MM_StackSlotValidator_reportStackSlot_Entry(env->getLanguageVMThread(), NULL);

	char* threadName = getOMRVMThreadName(vmthread);
	omrtty_printf("%p: %s in thread %s\n", env->getLanguageVMThread(), message, NULL == threadName ? "NULL" : threadName);
	Trc_MM_StackSlotValidator_thread(env->getLanguageVMThread(), message, NULL == threadName ? "NULL" : threadName);

	omrtty_printf("%p:\tO-Slot=%p\n", env->getLanguageVMThread(), _stackLocation);
	Trc_MM_StackSlotValidator_OSlot(env->getLanguageVMThread(), _stackLocation);

	omrtty_printf("%p:\tO-Slot value=%p\n", env->getLanguageVMThread(), _slotValue);
	Trc_MM_StackSlotValidator_OSlotValue(env->getLanguageVMThread(), _slotValue);

	releaseOMRVMThreadName(vmthread);

	Trc_MM_StackSlotValidator_reportStackSlot_Exit(env->getLanguageVMThread());
}

