
/*******************************************************************************
 * Copyright (c) 1991, 2014 IBM Corp. and others
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
 * @ingroup GC_Modron_Standard
 */

#if !defined(VLHGCACCESSBARRIER_HPP_)
#define VLHGCACCESSBARRIER_HPP_


#include "omr.h"
#include "omrcfg.h"	

#include "ObjectAccessBarrier.hpp"
#include "GenerationalAccessBarrierComponent.hpp"

/**
 * Access barrier for Modron collector.
 */
 
class MM_VLHGCAccessBarrier : public MM_ObjectAccessBarrier
{
private:
	void postObjectStoreImpl(OMR_VMThread *vmThread, fomrobject_t *dstObject, fomrobject_t *srcObject);
	void preBatchObjectStoreImpl(OMR_VMThread *vmThread, fomrobject_t *dstObject);

protected:
	virtual bool initialize(MM_EnvironmentBase *env);
	virtual void tearDown(MM_EnvironmentBase *env);
	
public:
	static MM_VLHGCAccessBarrier *newInstance(MM_EnvironmentBase *env);
	virtual void kill(MM_EnvironmentBase *env);

	MM_VLHGCAccessBarrier(MM_EnvironmentBase *env) :
		MM_ObjectAccessBarrier(env)
	{
		_typeId = __FUNCTION__;
	}

	virtual void postObjectStore(OMR_VMThread *vmThread, fomrobject_t *destObject, fomrobject_t *destAddress, fomrobject_t *value, bool isVolatile=false);
	virtual void postObjectStore(OMR_VMThread *vmThread, J9Class *destClass, fomrobject_t **destAddress, fomrobject_t *value, bool isVolatile=false);
	virtual bool preBatchObjectStore(OMR_VMThread *vmThread, fomrobject_t *destObject, bool isVolatile=false);
	virtual bool preBatchObjectStore(OMR_VMThread *vmThread, J9Class *destClass, bool isVolatile=false);
	virtual void recentlyAllocatedObject(OMR_VMThread *vmThread, fomrobject_t *object); 
	virtual void postStoreClassToClassLoader(OMR_VMThread *vmThread, J9ClassLoader* destClassLoader, J9Class* srcClass);

#if defined(J9VM_GC_ARRAYLETS)	
	virtual I_32 backwardReferenceArrayCopyIndex(OMR_VMThread *vmThread, fomrobject_t *srcObject, fomrobject_t *destObject, I_32 srcIndex, I_32 destIndex, I_32 lengthInSlots);
	virtual I_32 forwardReferenceArrayCopyIndex(OMR_VMThread *vmThread, fomrobject_t *srcObject, fomrobject_t *destObject, I_32 srcIndex, I_32 destIndex, I_32 lengthInSlots);
#endif	

	virtual void* jniGetPrimitiveArrayCritical(OMR_VMThread* vmThread, jarray array, jboolean *isCopy);
	virtual void jniReleasePrimitiveArrayCritical(OMR_VMThread* vmThread, jarray array, void * elems, jint mode);
	virtual const jchar* jniGetStringCritical(OMR_VMThread* vmThread, jstring str, jboolean *isCopy);
	virtual void jniReleaseStringCritical(OMR_VMThread* vmThread, jstring str, const jchar* elems);

};

#endif /* VLHGCACCESSBARRIER_HPP_ */
