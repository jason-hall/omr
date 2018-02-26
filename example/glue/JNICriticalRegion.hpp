
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
 * @ingroup GC_Base
 */

#if !defined(JNICRITICALREGION_HPP_)
#define JNICRITICALREGION_HPP_

#include "BaseNonVirtual.hpp"

class MM_JNICriticalRegion : public MM_BaseNonVirtual
{
	/* data members */
private:
protected:
public:

	/* member function */
private:
protected:
public:

	/**
	 * Re-acquire VM and/or JNI critical access as specified by the accessMask.
	 * This will block if another thread holds exclusive VM access.
	 *
	 * This function is intended for use with releaseAccess.
	 *
	 * @param vmThread  the thread requesting access
	 * @param accessMask  the types of access that were held
	 */
	static void reacquireAccess(OMR_VMThread* vmThread, UDATA accessMask)
	{
	}

	/**
	 * Release VM and/or JNI critical access. Record what was held in accessMask.
	 * This will respond to any pending exclusive VM access request in progress.
	 *
	 * This function is intended for use with reacquireAccess.
	 *
	 * @param vmThread  the OMR_VMThread requesting access
	 * @param accessMask  the types of access to re-acquire
	 */
	static void releaseAccess(OMR_VMThread* vmThread, UDATA* accessMask)
	{
	}

	/**
	 * Enter a JNI critical region.
	 * Once a thread has successfully entered a critical region, it has privileges similar
	 * to holding VM access. No object can move while any thread is in a critical region.
	 *
	 * @param vmThread  the thread requesting to enter a critical region
	 */
	static MMINLINE void
	enterCriticalRegion(OMR_VMThread* vmThread)
	{
	}

	/**
	 * Exit a JNI critical region .
	 * Once a thread has successfully exitted a critical region, objects in the java
	 * heap are allowed to move again.
	 *
	 * @param vmThread  the thread requesting to exit a critical region
	 */
	static MMINLINE void
	exitCriticalRegion(OMR_VMThread* vmThread)
	{
	}
};

#endif /* JNICRITICALREGION_HPP_ */
