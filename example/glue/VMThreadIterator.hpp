
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

/**
 * @file
 * @ingroup GC_Structs
 */

#if !defined(VMTHREADITERATOR_HPP_)
#define VMTHREADITERATOR_HPP_

#include "omrgcconsts.h"
#include "VMThreadSlotIterator.hpp"

/**
 * State constants representing the current stage of the iteration process
 * @anchor VMThreadIteratorState
 */
enum {
	vmthreaditerator_state_start = 0,
	vmthreaditerator_state_slots,
	vmthreaditerator_state_end
};


/**
 * Iterate over slots in a VM thread which contain object refereces.
 * Internally, uses GC_VMThreadSlotIterator on the given thread.
 * @note Does not include references on the thread's stack (see GC_VMThreadStackSlotIterator)
 * @ingroup GC_Structs
 */
class GC_VMThreadIterator
{
	OMR_VMThread *_vmThread;
	int _state;

	GC_VMThreadSlotIterator _threadSlotIterator;

public:
	GC_VMThreadIterator(OMR_VMThread *vmThread) :
		_vmThread(vmThread),
		_state(vmthreaditerator_state_start),
		_threadSlotIterator(_vmThread)
		{};

	/**
	 * @return @ref VMThreadIteratorState representing the current state (stage
	 * of the iteration process)
	 */
	MMINLINE int 
	getState() 
	{ 
		return _state; 
	}

	/**
	 * Get the OMR_VMThread * for the thread being iterated
	 * @return _vmThread - the OMR_VMThread being iterated.
	 */
	MMINLINE OMR_VMThread *
	getVMThread()
	{
		return _vmThread;
	}

	omrobjectptr_t nextSlot();
};

#endif /* VMTHREADITERATOR_HPP_ */

