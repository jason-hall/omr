/*******************************************************************************
 * Copyright (c) 1991, 2016 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#if !defined(ARRAYLETOBJECTMODEL_)
#define ARRAYLETOBJECTMODEL_

#include "omrcfg.h"
#include "ModronAssertions.h"


#if defined(OMR_GC_ARRAYLETS)

class MM_GCExtensionsBase;
class MM_MemorySubSpace;

class GC_ArrayletObjectModel
{
	/*
	 * Function members
	 */
private:
protected:
public:
	bool
	initialize(MM_GCExtensionsBase *extensions)
	{
		return true;
	}

	void tearDown(MM_GCExtensionsBase *extensions) {}

	MMINLINE fomrobject_t *
	getArrayoidPointer(omrarrayptr_t arrayPtr)
	{
		return (fomrobject_t *)NULL;
	}

	MMINLINE void
	expandArrayletSubSpaceRange(MM_MemorySubSpace * subSpace, void * rangeBase, void * rangeTop, uintptr_t largestDesirableArraySpineSize)
	{
		/* No-op */
	}

	/**
	 * Returns the address of first data slot in the array
	 * @param arrayPtr Ptr to an array
	 * @return Address of first data slot in the array
	 */
	MMINLINE void *
	getDataPointerForContiguous(omrarrayptr_t arrayPtr)
	{
		Assert_MM_unimplemented();
		return NULL;
	}
	
	/**
	 * Returns the size of an indexable object, in bytes, including the header.
	 * @param arrayPtr Pointer to the indexable object whose size is required
	 * @return Size of object in bytes including the header
	 */
	MMINLINE uintptr_t
	getSizeInBytesWithHeader(omrarrayptr_t arrayPtr)
	{
		Assert_MM_unimplemented();
		return 0;
	}

	/**
	 * Returns the size of an indexable object in elements.
	 * @param arrayPtr Pointer to the indexable object whose size is required
	 * @return Size of object in elements
	 */
	MMINLINE UDATA
	getSizeInElements(omrobjectptr_t arrayPtr)
	{
		Assert_MM_unimplemented();
		return 0;
	}

	/**
	 * Return the total number of arraylets for the given indexable object
	 * @param objPtr Pointer to an array object
	 * @return the number of arraylets used for an array of dataSizeInBytes bytes
	 */
	MMINLINE UDATA
	numArraylets(omrarrayptr_t objPtr)
	{
		Assert_MM_unimplemented();
		return 0;
	}

	MMINLINE UDATA
	numArraylets(UDATA unadjustedDataSizeInBytes)
	{
		Assert_MM_unimplemented();
		return 0;
	}

	MMINLINE UDATA
	arrayletSize(omrarrayptr_t objPtr, UDATA index)
	{
		Assert_MM_unimplemented();
		return 0;
	}
};

#endif /*OMR_GC_ARRAYLETS */
#endif /* ARRAYLETOBJECTMODEL_ */
