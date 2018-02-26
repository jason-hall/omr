
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

#ifndef ROOTSCANNER_HPP_
#define ROOTSCANNER_HPP_

#include "omr.h"

#include "BaseVirtual.hpp"

#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"
#include "ModronTypes.hpp"
#include "RootScannerTypes.h"
#include "Task.hpp"

class GC_SlotObject;
class MM_MemoryPool;
class MM_CollectorLanguageInterfaceImpl;

/**
 * General interface for scanning all object and class slots in the system that are not part of the heap.
 * 
 * MM_RootScanner provides an abstract class that can be specialized to scan particular slots
 * in the system, including all, root specific and clearable slots.  The purpose of the class is
 * to provide a central location for general slot scanners within Modron (e.g., root scanning,
 * all slots do, etc).
 * 
 * There are two levels of specialization for the scanner, structure walking and handling of elements.
 * Structure walking specialization, where the implementator can override the way in which we walk elements,
 * should be done rarely and in only extreme circumstances.  Handling of elements can be specialized for all
 * elements as well as for specific types of structures.
 * 
 * The core routine to be reimplemented is doSlot(omrobjectptr_t*).
 * All other slot types are forwarded by default to these routines for processing.  To handle structure slots in 
 * specific ways, the slot handler for that type should be overridden.
 * 
 * @ingroup GC_Base
 */
class MM_RootScanner : public MM_BaseVirtual
{
	/*
	 * Data members
	 */
private:

protected:
	MM_EnvironmentBase *_env;
	MM_GCExtensionsBase *_extensions;
	MM_CollectorLanguageInterfaceImpl *_clij;
	OMR_VM *_omrVM;

	bool _singleThread;  /**< Should the iterator operate in single threaded mode */

	bool _nurseryReferencesOnly;  /**< Should the iterator only scan structures that currently contain nursery references */
	bool _nurseryReferencesPossibly;  /**< Should the iterator only scan structures that may contain nursery references */
#if defined(OMR_GC_MODRON_SCAVENGER)		 
	bool _includeRememberedSetReferences;  /**< Should the iterator include references in the Remembered Set (if applicable) */
#endif /* OMR_GC_MODRON_SCAVENGER */	 	
	bool _includeJVMTIObjectTagTables; /**< Should the iterator include the JVMTIObjectTagTables. Default true, should set to false when doing JVMTI object walks */
	bool _trackVisibleStackFrameDepth; /**< Should the stack walker be told to track the visible frame depth. Default false, should set to true when doing JVMTI walks that report stack slots */

	U_64 _entityStartScanTime; /**< The start time of the scan of the current scanning entity, or 0 if no entity is being scanned.  Defaults to 0. */
	RootScannerEntity _scanningEntity; /**< The root scanner entity that is currently being scanned. Defaults to RootScannerEntity_None. */ 
	RootScannerEntity _lastScannedEntity; /**< The root scanner entity that was last scanned. Defaults to RootScannerEntity_None. */

	/*
	 * Function members
	 */
private:

	/**
	 * Scan all fields of pointer array object
	 * @param objectPtr address of object to scan
	 * @param memoryPool current memory pool
	 * @param manager current region manager
	 * @param memoryType memory type
	 */
	void scanArrayObject(MM_EnvironmentBase *env, omrobjectptr_t objectPtr, MM_MemoryPool *memoryPool, MM_HeapRegionManager *manager, UDATA memoryType);

protected:
	/**
	 * Root scanning methods that have been incrementalized are responsible for
	 * calling this method periodically to check when they should yield.
	 * @return true if the GC should yield, false otherwise
	 */
	virtual bool shouldYield();

	/**
	 * Root scanning methods that have been incrementalized should call this method
	 * after determining that it should yield and after releasing locks held during
	 * root processing.  Upon returning from this method, the root scanner is
	 * responsible for re-acquiring necessary locks.  The integrity of iterators
	 * and the semantics of atomicity are not ensured and must be examined on a
	 * case-by-case basis.
	 */
	virtual void yield();

	/**
	 * Two in one method which amounts to yield if shouldYield is true.
	 * @return true if yielded, false otherwise
	 */
	virtual bool condYield(U_64 timeSlackNanoSec = 0);
	
	virtual void flushRequiredNonAllocationCaches(MM_EnvironmentBase *envModron){}
	
	/**
	 * Sets the currently scanned root entity to scanningEntity. This is mainly for
	 * debug purposes.
	 * @param scanningEntity The entity for which scanning has begun.
	 */
	MMINLINE void
	reportScanningStarted(RootScannerEntity scanningEntity)
	{
		/* Ensures reportScanningEnded was called previously. */
		assume0(RootScannerEntity_None == _scanningEntity);
		_scanningEntity = scanningEntity;
		
		if (_extensions->rootScannerStatsEnabled) {
			OMRPORT_ACCESS_FROM_OMRVM(_omrVM);
			_entityStartScanTime = omrtime_hires_clock();	
		}
	}
	
	/**
	 * Sets the currently scanned root entity to None and sets the last scanned root
	 * entity to scannedEntity. This is mainly for debug purposes.
	 * @param scannedEntity The entity for which scanning has ended.
	 */
	MMINLINE void
	reportScanningEnded(RootScannerEntity scannedEntity)
	{
		/* Ensures scanning ended for the currently scanned entity. */
		assume0(_scanningEntity == scannedEntity);
		_lastScannedEntity = _scanningEntity;
		_scanningEntity = RootScannerEntity_None;
		
		if (_extensions->rootScannerStatsEnabled) {
			OMRPORT_ACCESS_FROM_OMRVM(_omrVM);
			U_64 entityEndScanTime = omrtime_hires_clock();
			
			if (_entityStartScanTime >= entityEndScanTime) {
				_env->_rootScannerStats._entityScanTime[scannedEntity] += 1;
			} else {
				_env->_rootScannerStats._entityScanTime[scannedEntity] += entityEndScanTime - _entityStartScanTime;	
			}
			
			_entityStartScanTime = 0;
		}
	}

public:
	MM_RootScanner(MM_EnvironmentBase *env, bool singleThread = false)
		: MM_BaseVirtual()
		, _env(env)
		, _extensions(env->getExtensions())
		, _clij((MM_CollectorLanguageInterfaceImpl *)_extensions->collectorLanguageInterface)
		, _omrVM(env->getOmrVM())
		, _singleThread(singleThread)
		, _nurseryReferencesOnly(false)
		, _nurseryReferencesPossibly(false)
#if defined(OMR_GC_MODRON_SCAVENGER)		 
		, _includeRememberedSetReferences(_extensions->scavengerEnabled ? true : false)
#endif /* OMR_GC_MODRON_SCAVENGER */
		, _includeJVMTIObjectTagTables(true)
		, _trackVisibleStackFrameDepth(false)
		, _entityStartScanTime(0)
		, _scanningEntity(RootScannerEntity_None)
		, _lastScannedEntity(RootScannerEntity_None)
	{
		_typeId = __FUNCTION__;
	}

	/**
	 * Return codes from root scanner complete phase calls that are allowable by implementators.
	 */
	typedef enum {
		complete_phase_OK = 0,  /**< Continue scanning */
		complete_phase_ABORT,  /**< Abort all further scanning */
	} CompletePhaseCode;

#if defined(OMR_GC_MODRON_SCAVENGER)
	/** Set whether the iterator will only scan structures which contain nursery references */
	void setNurseryReferencesOnly(bool nurseryReferencesOnly) {
		_nurseryReferencesOnly = nurseryReferencesOnly;
	}

	/** Set whether the iterator will only scan structures which may contain nursery references */
	void setNurseryReferencesPossibly(bool nurseryReferencesPossibly) {
		_nurseryReferencesPossibly = nurseryReferencesPossibly;
	}

	/** Set whether the iterator will scan the remembered set references (if applicable to the scan type) */
	void setIncludeRememberedSetReferences(bool includeRememberedSetReferences) {
		_includeRememberedSetReferences = includeRememberedSetReferences;
	}
#endif /* OMR_GC_MODRON_SCAVENGER */

	/** Set whether the iterator will scan the JVMTIObjectTagTables (if applicable to the scan type) */
	void setTrackVisibleStackFrameDepth(bool trackVisibleStackFrameDepth) {
		 _trackVisibleStackFrameDepth = trackVisibleStackFrameDepth;
	}

	/** General object slot handler to be reimplemented by specializing class. This handler is called for every reference to a omrobjectptr_t. */
	virtual void doSlot(omrobjectptr_t *slotPtr) = 0;

	/**
	 * Scan object field
	 * @param slotObject for field
	 */
	virtual void doFieldSlot(GC_SlotObject * slotObject);
	
	virtual void scanRoots(MM_EnvironmentBase *env);
	virtual void scanClearable(MM_EnvironmentBase *env);
	virtual void scanAllSlots(MM_EnvironmentBase *env);

#if defined(OMR_GC_MODRON_SCAVENGER)
	virtual void scanRememberedSet(MM_EnvironmentBase *env);
#endif /* OMR_GC_MODRON_SCAVENGER */

 	virtual bool scanOneThread(MM_EnvironmentBase *env, OMR_VMThread* walkThread, void* localData);
	
	virtual void scanThreads(MM_EnvironmentBase *env);
 	virtual void scanSingleThread(MM_EnvironmentBase *env, OMR_VMThread* walkThread);

#if defined(OMR_GC_MODRON_SCAVENGER)
	virtual void doRememberedSetSlot(omrobjectptr_t *slotPtr, GC_RememberedSetSlotIterator *rememberedSetSlotIterator);
#endif /* defined(OMR_GC_MODRON_SCAVENGER) */

	virtual void doVMThreadSlot(omrobjectptr_t *slotPtr, GC_VMThreadIterator *vmThreadIterator);
	
	/**
	 * Called for each object stack slot. Subclasses may override.
	 * 
	 * @param slotPtr[in/out] a pointer to the stack slot or a copy of the stack slot.
	 * @param walkState[in] the J9StackWalkState
	 * @param stackLocation[in] the actual pointer to the stack slot. Use only for reporting. 
	 */
	virtual void doStackSlot(omrobjectptr_t *slotPtr, void *walkState, const void *stackLocation);
};

typedef struct StackIteratorData {
    MM_RootScanner *rootScanner;
    MM_EnvironmentBase *env;
} StackIteratorData;

#endif /* ROOTSCANNER_HPP_ */
