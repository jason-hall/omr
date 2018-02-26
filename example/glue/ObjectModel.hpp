/*******************************************************************************
 * Copyright (c) 1991, 2018 IBM Corp. and others
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

#if !defined(OBJECTMODEL_HPP_)
#define OBJECTMODEL_HPP_

/*
 * @ddr_namespace: default
 */

#include "Object.hpp"
#include "Bits.hpp"
#include "ObjectModelBase.hpp"
#include "ObjectModelDelegate.hpp"

#if defined(OMR_GC_REALTIME)
/* this bit is set in the object header slot if an overflow condition is raised */
#define GC_OVERFLOW	 0x4
#endif /* defined(OMR_GC_REALTIME) */

class MM_GCExtensionsBase;

/**
 * Provides an example of an object model, sufficient to support basic GC test code (GCConfigTest)
 * and to illustrate some of the required facets of an OMR object model.
 */
class GC_ObjectModel : public GC_ObjectModelBase
{
/*
 * Member data and types
 */
private:
protected:
public:

/*
 * Member functions
 */
private:
protected:
public:
	/**
	 * Initialize the receiver, a new instance of GC_ObjectModel. An implementation of this method is
	 * required. Object models that require initialization after instantiation should perform this
	 * initialization here. Otherwise, the implementation should simply return true.
	 *
	 * @return TRUE on success, FALSE on failure
	 */
	virtual bool initialize(MM_GCExtensionsBase *extensions) { return true; }

	/**
	 * Tear down the receiver. A (possibly empty) implementation of this method is required. Any resources
	 * acquired for the object model in the initialize() implementation should be disposed of here.
	 */
	virtual void tearDown(MM_GCExtensionsBase *extensions) {}

	/**
	 * Set size in object header, with header flags.
	 * @param objectPtr Pointer to an object
	 * @param size consumed size in bytes of object
	 * @param flags flag bits to set
	 */
	MMINLINE void
	setObjectSizeAndFlags(omrobjectptr_t objectPtr, uintptr_t size, uintptr_t flags)
	{
		uintptr_t sizeBits = size << OMR_OBJECT_METADATA_FLAGS_BIT_COUNT;
		uintptr_t flagsBits = flags & (uintptr_t)OMR_OBJECT_METADATA_FLAGS_MASK;
		*(getObjectHeaderSlotAddress(objectPtr)) = (fomrobject_t)(sizeBits | flagsBits);
	}

	/**
	 * Set size in object header, preserving header flags
	 * @param objectPtr Pointer to an object
	 * @param size consumed size in bytes of object
	 */
	MMINLINE void
	setObjectSize(omrobjectptr_t objectPtr, uintptr_t size)
	{
		setObjectSizeAndFlags(objectPtr, size, getObjectFlags(objectPtr));
	}

#if defined(OMR_GC_REALTIME)
	MMINLINE bool
	atomicSetOverflowBit(omrobjectptr_t objectPtr)
	{
		return atomicSetObjectFlags(objectPtr, 0, GC_OVERFLOW);
	}

	MMINLINE bool
	atomicClearOverflowBit(omrobjectptr_t objectPtr)
	{
		return atomicSetObjectFlags(objectPtr, GC_OVERFLOW, 0);
	}

	MMINLINE bool
	isOverflowBitSet(omrobjectptr_t objectPtr)
	{
		return (GC_OVERFLOW == (getObjectFlags(objectPtr) & GC_OVERFLOW));
	}
#endif /* defined(OMR_GC_REALTIME) */

	MMINLINE bool
	isObjectArray(omrobjectptr_t objectPtr)
	{
		return false;
	}

	/**
	 * Constructor.
	 */
	GC_ObjectModel()
		: GC_ObjectModelBase()
	{}

	/**
	* Return values for getScanType().
	*/
	enum ScanType {
		SCAN_INVALID_OBJECT = 0,
		SCAN_MIXED_OBJECT = 1,
		SCAN_POINTER_ARRAY_OBJECT = 2,
		SCAN_PRIMITIVE_ARRAY_OBJECT = 3,
		SCAN_REFERENCE_MIXED_OBJECT = 4,
		SCAN_CLASS_OBJECT = 5,
		SCAN_CLASSLOADER_OBJECT = 6,
		SCAN_ATOMIC_MARKABLE_REFERENCE_OBJECT = 7,
		SCAN_OWNABLESYNCHRONIZER_OBJECT = 8,
	};

	/**
	 * Determine the ScanType code for objects of the specified class. This code determines how instances should be scanned.
	 * @param clazz[in] the class of the object to be scanned
	 * @return a ScanType code, SCAN_INVALID_OBJECT if the code cannot be determined due to an error
	 */
	MMINLINE ScanType
	getScanType(omrobjectptr_t objectPtr)
	{
		return SCAN_INVALID_OBJECT;
	}
};
#endif /* OBJECTMODEL_HPP_ */
