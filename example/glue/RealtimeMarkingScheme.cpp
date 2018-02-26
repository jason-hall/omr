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

#include "RealtimeMarkingScheme.hpp"

#if defined(OMR_GC_REALTIME)
#include "CycleState.hpp"
#include "EnvironmentRealtime.hpp"
#include "HeapRegionDescriptorRealtime.hpp"
#include "MarkMap.hpp"
#include "ObjectAllocationInterface.hpp"
#include "omrExampleVM.hpp"
#include "RealtimeGC.hpp"
#include "RealtimeRootScanner.hpp"
#include "SlotObject.hpp"
#include "StackSlotValidator.hpp"

/**
 * This scanner will mark objects that pass through its doSlot.
 */
class MM_RealtimeMarkingSchemeRootMarker : public MM_RealtimeRootScanner
{
/* Data memebers / types */
public:
protected:
private:
	
/* Methods */
public:

	/**
	 * Simple chained constructor.
	 */
	MM_RealtimeMarkingSchemeRootMarker(MM_EnvironmentRealtime *env, MM_RealtimeGC *realtimeGC) :
		MM_RealtimeRootScanner(env, realtimeGC)
	{
		_typeId = __FUNCTION__;
	}
	
	/**
	 * This scanner can be instantiated so we must give it a name.
	 */
	virtual const char*
	scannerName()
	{
		return "Mark";
	}
	
	/**
	 * Wraps the scanning of one thread to only happen if it hasn't already occured in this phase of this GC,
	 * also sets the thread up for the upcoming forwarding phase.
	 * @return true if the thread was scanned (the caller should offer to yield), false otherwise.
	 * @see MM_RootScanner::scanOneThread()
	 */
	virtual void
	scanOneThreadImpl(MM_EnvironmentRealtime *env, OMR_VMThread* walkThread, void* localData)
	{
		MM_EnvironmentRealtime* walkThreadEnv = MM_EnvironmentRealtime::getEnvironment(walkThread);
		/* Scan the thread by invoking superclass */
		MM_RootScanner::scanOneThread(env, walkThread, localData);

		/*
		 * TODO CRGTMP we should be able to premark the cache instead of flushing the cache
		 * but this causes problems in overflow.  When we have some time we should look into
		 * this again.
		 */
		/*((MM_SegregatedAllocationInterface *)walkThreadEnv->_objectAllocationInterface)->preMarkCache(walkThreadEnv);*/
		walkThreadEnv->_objectAllocationInterface->flushCache(walkThreadEnv);
		/* Disable the double barrier on the scanned thread. */
		_realtimeGC->disableDoubleBarrierOnThread(env, walkThread);
	}

	/**
	 * Simply pass the call on to the RealtimeGC.
	 * @see MM_Metronome::markObject()
	 */
	virtual void
	doSlot(omrobjectptr_t *slot)
	{
		_markingScheme->markObject(_env, *slot);
	}
	
	virtual void
	doStackSlot(omrobjectptr_t *slotPtr, void *walkState, const void* stackLocation)
	{
		omrobjectptr_t object = *slotPtr;
		if (_markingScheme->isHeapObject(object)) {
			/* heap object - validate and mark */
			Assert_MM_validStackSlot(MM_StackSlotValidator(0, object, stackLocation, walkState).validate(_env));
			_markingScheme->markObject(_env, object);
		} else if (NULL != object) {
			/* stack object - just validate */
			Assert_MM_validStackSlot(MM_StackSlotValidator(MM_StackSlotValidator::NOT_ON_HEAP, object, stackLocation, walkState).validate(_env));
		}
	}

	virtual void 
	doVMThreadSlot(omrobjectptr_t *slotPtr, GC_VMThreadIterator *vmThreadIterator) {
		omrobjectptr_t object = *slotPtr;
		if (_markingScheme->isHeapObject(object)) {
			_markingScheme->markObject(_env, object);
		}
	}

	/**
	 * Scans non-collectable internal objects (immortal)
	 */
	virtual void
	scanIncrementalRoots(MM_EnvironmentRealtime *env)
	{
		condYield(_realtimeGC->_sched->beatNanos);
	}
protected:
private:
};

/**
 * This scanner will mark objects that pass through its doSlot.
 */
class MM_RealtimeMarkingSchemeRootClearer : public MM_RealtimeRootScanner
{
/* Data memebers / types */
public:
protected:
private:
	
/* Methods */
public:

	/**
	 * Simple chained constructor.
	 */
	MM_RealtimeMarkingSchemeRootClearer(MM_EnvironmentRealtime *env, MM_RealtimeGC *realtimeGC) :
		MM_RealtimeRootScanner(env, realtimeGC)
	{
		_typeId = __FUNCTION__;
	}
	
	/**
	 * This should not be called
	 */
	virtual void
	doSlot(omrobjectptr_t* slot)
	{
		OMRPORT_ACCESS_FROM_ENVIRONMENT(_env);
		omrtty_printf("MM_RealtimeMarkingSchemeRootClearer::doSlot should not be called\n");
		assert(false);
	}

	/**
	 * This scanner can be instantiated so we must give it a name.
	 */
	virtual const char*
	scannerName()
	{
		return "Clearable";
	}
	
	/**
	 * Clears slots holding unmarked objects.
	 */
	virtual void
	doJNIWeakGlobalReference(omrobjectptr_t *slotPtr)
	{
		omrobjectptr_t objectPtr = *slotPtr;
		if(objectPtr && !_markingScheme->isMarked(objectPtr)) {
			*slotPtr = NULL;
		}
	}

	virtual void scanClearable(MM_EnvironmentBase *env)
	{
		MM_RealtimeRootScanner::scanClearable(env);
		//OMRPORT_ACCESS_FROM_OMRVM(env->getOmrVM());
		OMR_VM_Example *omrVM = (OMR_VM_Example *)env->getOmrVM()->_language_vm;
		if (NULL != omrVM->objectTable) {
			env->_currentTask->synchronizeGCThreads(env, UNIQUE_ID);
			J9HashTableState state;
			ObjectEntry *objectEntry = (ObjectEntry *)hashTableStartDo(omrVM->objectTable, &state);
			while (NULL != objectEntry) {
				if (!((MM_RealtimeGC*)_extensions->getGlobalCollector())->getMarkingScheme()->isMarked(objectEntry->objPtr)) {
				/*omrmem_free_memory((void *)objectEntry->name);
				objectEntry->name = NULL;
				hashTableDoRemove(&state);*/
				}
				objectEntry = (ObjectEntry *)hashTableNextDo(&state);
			}
			env->_currentTask->releaseSynchronizedGCThreads(env);
		}
	}
};

/**
 * Allocate and initialize a new instance of the receiver.
 * @return a new instance of the receiver, or NULL on failure.
 */
MM_RealtimeMarkingScheme *
MM_RealtimeMarkingScheme::newInstance(MM_EnvironmentBase *env, MM_RealtimeGC *realtimeGC)
{
	MM_RealtimeMarkingScheme *instance;
	
	instance = (MM_RealtimeMarkingScheme *)env->getForge()->allocate(sizeof(MM_RealtimeMarkingScheme), MM_AllocationCategory::FIXED, OMR_GET_CALLSITE());
	if (instance) {
		new(instance) MM_RealtimeMarkingScheme(env, realtimeGC);
		if (!instance->initialize(env)) { 
			instance->kill(env);
			instance = NULL;
		}
	}

	return instance;
}

/**
 * Free the receiver and all associated resources.
 */
void
MM_RealtimeMarkingScheme::kill(MM_EnvironmentBase *env)
{
	tearDown(env); 
	env->getForge()->free(this);
}

/**
 * Intialize the RealtimeMarkingScheme instance.
 * 
 */
bool
MM_RealtimeMarkingScheme::initialize(MM_EnvironmentBase *env)
{
	if (!MM_SegregatedMarkingScheme::initialize(env)) {
		return false;
	}

	_omrVM = env->getOmrVM();
	_scheduler = _realtimeGC->_sched;
	_gcExtensions = (MM_GCExtensionsBase *)_extensions;

	return true;
}

/**
 * Teardown the RealtimeMarkingScheme instance.
 * 
 */
void
MM_RealtimeMarkingScheme::tearDown(MM_EnvironmentBase *env)
{
	MM_SegregatedMarkingScheme::tearDown(env);
}

/**
 * Mark all of the roots.  The system and application classloaders need to be set
 * to marked/scanned before root marking begins.  
 * 
 * @note Once the root lists all have barriers this code may change to call rootScanner.scanRoots();
 * 
 */
void 
MM_RealtimeMarkingScheme::markRoots(MM_EnvironmentRealtime *env, MM_RealtimeMarkingSchemeRootMarker *rootScanner)
{
	rootScanner->scanRoots(env);

	if (env->_currentTask->synchronizeGCThreadsAndReleaseMaster(env, UNIQUE_ID)) {
		_gcExtensions->newThreadAllocationColor = GC_MARK;
		_realtimeGC->disableDoubleBarrier(env);
		if (_realtimeGC->verbose(env) >= 3) {
			rootScanner->reportThreadCount(env);
		}

		/* Note: if iterators are safe for some or all remaining atomic root categories,
		 * disableYield() could be removed or moved inside scanAtomicRoots.
		 */
		env->disableYield();
		rootScanner->scanAtomicRoots(env);
		env->enableYield();
		rootScanner->scanIncrementalRoots(env);
	
		env->_currentTask->releaseSynchronizedGCThreads(env);
	}
}

/**
 * This function marks all of the live objects on the heap and handles all of the clearable
 * objects.
 * 
 */
void
MM_RealtimeMarkingScheme::markLiveObjects(MM_EnvironmentRealtime *env)
{
	env->getWorkStack()->reset(env, _realtimeGC->_workPackets);
	
	/* These are thread-local stats that should probably be moved
	 * into the MM_MarkStats structure.
	 */
	env->resetScannedCounters();
	
	/* The write barrier must be enabled before any scanning begins. The double barrier will
	 * be enabled for the duration of the thread scans. It gets disabled on a per thread basis
	 * as the threads get scanned. It also gets "disabled" on a global basis once all threads
	 * are scanned.
	 */
	if (env->_currentTask->synchronizeGCThreadsAndReleaseMaster(env, UNIQUE_ID)) {
		_realtimeGC->enableWriteBarrier(env);
		_realtimeGC->enableDoubleBarrier(env);
		/* BEN TODO: Ragged barrier here */
		env->_currentTask->releaseSynchronizedGCThreads(env);
	}
	
	MM_RealtimeMarkingSchemeRootMarker rootMarker(env, _realtimeGC);
	markRoots(env, &rootMarker);
	_scheduler->condYieldFromGC(env);
	/* Heap Marking and barrier processing. Cannot delay barrier processing until the end.*/
	_realtimeGC->doTracing(env);
	
	if (env->_currentTask->synchronizeGCThreadsAndReleaseMaster(env, UNIQUE_ID)) {
		_realtimeGC->_unmarkedImpliesCleared = true;
		env->_currentTask->releaseSynchronizedGCThreads(env);
	}

	/* Process reference objects and finalizable objects. */
	MM_RealtimeMarkingSchemeRootClearer rootScanner(env, _realtimeGC);
	rootScanner.scanClearable(env);

	_scheduler->condYieldFromGC(env);
	
	/* Do a final tracing phase to complete the marking phase.  It should not be possible for any thread,
	 * including NHRT's, to add elements to the rememberedSet between the end of this doTracing call and when
	 * we disable the write barrier since the entire live set will be completed.
	 */
	_realtimeGC->doTracing(env);

	if (env->_currentTask->synchronizeGCThreadsAndReleaseMaster(env, UNIQUE_ID)) {
		/* This flag is set during the soft reference scanning just before unmarked references are to be
		 * cleared. It's used to prevent objects that are going to be cleared (e.g. referent that is not marked,
		 * or unmarked string constant) from escaping.
		 */
		_realtimeGC->_unmarkedImpliesCleared = false;
		_realtimeGC->_unmarkedImpliesStringsCleared = false;

		/* This is the symmetric call to the enabling of the write barrier that happens at the top of this method. */
		_realtimeGC->disableWriteBarrier(env);
		/* BEN TODO: Ragged barrier here */
		
		/* reset flag "overflow happened this GC cycle" */
		_realtimeGC->_workPackets->getIncrementalOverflowHandler()->resetOverflowThisGCCycle();
		
		Assert_MM_true(_realtimeGC->_workPackets->isAllPacketsEmpty());
		
		env->_currentTask->releaseSynchronizedGCThreads(env);
	}
}

MMINLINE UDATA
MM_RealtimeMarkingScheme::scanPointerArraylet(MM_EnvironmentRealtime *env, fomrobject_t *arraylet)
{
	fomrobject_t *startScanPtr = arraylet;
	fomrobject_t *endScanPtr = startScanPtr + env->getOmrVM()->_arrayletLeafSize / sizeof(omrobjectptr_t);
	return scanPointerRange(env, startScanPtr, endScanPtr);
}

MMINLINE UDATA
MM_RealtimeMarkingScheme::scanPointerRange(MM_EnvironmentRealtime *env, fomrobject_t *startScanPtr, fomrobject_t *endScanPtr)
{
	fomrobject_t *scanPtr = startScanPtr;
	UDATA pointerFieldBytes = (UDATA)(endScanPtr - scanPtr);
	UDATA pointerField = pointerFieldBytes / sizeof(omrobjectptr_t);
	while(scanPtr < endScanPtr) {
		GC_SlotObject slotObject(_omrVM, scanPtr);
		markObject(env, slotObject.readReferenceFromSlot());
		scanPtr++;
	}

	env->addScannedBytes(pointerFieldBytes);
	env->addScannedPointerFields(pointerField);

	return pointerField;
}

MMINLINE UDATA
MM_RealtimeMarkingScheme::scanOmrObject(MM_EnvironmentRealtime *env, omrobjectptr_t objectPtr)
{
	uintptr_t sizeToDo = UDATA_MAX;
	UDATA pointersScanned = 0;
	GC_ObjectScannerState objectScannerState;
	GC_ObjectScanner *objectScanner = _delegate.getObjectScanner(env, objectPtr, &objectScannerState, SCAN_REASON_PACKET, &sizeToDo);
	if (NULL != objectScanner) {
		GC_SlotObject *slotObject;
#if defined(OMR_GC_LEAF_BITS)
		while (NULL != (slotObject = objectScanner->getNextSlot(isLeafSlot))) {
#else /* defined(OMR_GC_LEAF_BITS) */
			while (NULL != (slotObject = objectScanner->getNextSlot())) {
#endif /* defined(OMR_GC_LEAF_BITS) */
			markObject(env, slotObject->readReferenceFromSlot(), false);
			pointersScanned += 1;
		}

	}
	return pointersScanned;
}


MMINLINE UDATA
MM_RealtimeMarkingScheme::scanObject(MM_EnvironmentRealtime *env, omrobjectptr_t objectPtr)
{
	UDATA pointersScanned = 0;
	switch(_gcExtensions->objectModel.getScanType(objectPtr)) {
	case GC_ObjectModel::SCAN_ATOMIC_MARKABLE_REFERENCE_OBJECT:
	case GC_ObjectModel::SCAN_MIXED_OBJECT:
	case GC_ObjectModel::SCAN_OWNABLESYNCHRONIZER_OBJECT:
		pointersScanned = scanMixedObject(env, objectPtr);
		break;
	case GC_ObjectModel::SCAN_POINTER_ARRAY_OBJECT:
		pointersScanned = scanPointerArrayObject(env, (omrarrayptr_t)objectPtr);
		break;
	case GC_ObjectModel::SCAN_REFERENCE_MIXED_OBJECT:
		pointersScanned = scanReferenceMixedObject(env, objectPtr);
		break;
	case GC_ObjectModel::SCAN_PRIMITIVE_ARRAY_OBJECT:
	   pointersScanned = 0;
	   break;
	default:
		pointersScanned += scanOmrObject(env, objectPtr);
	}
	
	return pointersScanned;
}

MMINLINE UDATA
MM_RealtimeMarkingScheme::scanMixedObject(MM_EnvironmentRealtime *env, omrobjectptr_t objectPtr)
{
	/* Object slots */

	fomrobject_t *scanPtr = (fomrobject_t *)(objectPtr + 1);
	UDATA objectSize = _gcExtensions->mixedObjectModel.getSizeInBytesWithHeader(objectPtr);
	fomrobject_t *endScanPtr = (fomrobject_t *)(((U_8 *)objectPtr) + objectSize);

	UDATA pointerFields = 0;
	while(scanPtr < endScanPtr) {
		pointerFields++;
		GC_SlotObject slotObject(_omrVM, scanPtr);
		markObject(env, slotObject.readReferenceFromSlot(), false);
		scanPtr += 1;
	}

	env->addScannedBytes(objectSize);
	env->addScannedPointerFields(pointerFields);
	env->incScannedObjects();

	return pointerFields;
}

MMINLINE UDATA
MM_RealtimeMarkingScheme::scanReferenceMixedObject(MM_EnvironmentRealtime *env, omrobjectptr_t objectPtr)
{
	fomrobject_t *scanPtr = (fomrobject_t *)(objectPtr + 1);
	UDATA objectSize = _gcExtensions->mixedObjectModel.getSizeInBytesWithHeader(objectPtr);
	fomrobject_t *endScanPtr = (fomrobject_t *)(((U_8 *)objectPtr) + objectSize);
	
	UDATA pointerFields = 0;
	while(scanPtr < endScanPtr) {
		pointerFields++;
		GC_SlotObject slotObject(_omrVM, scanPtr);
		markObject(env, slotObject.readReferenceFromSlot(), false);
		scanPtr++;
	}
	
	env->addScannedBytes(objectSize);
	env->addScannedPointerFields(pointerFields);
	env->incScannedObjects();
	
	return pointerFields;
}

MMINLINE UDATA
MM_RealtimeMarkingScheme::scanPointerArrayObject(MM_EnvironmentRealtime *env, omrarrayptr_t objectPtr)
{
	UDATA pointerFields = 0;

	/* Very small arrays cannot be set as scanned (no scanned bit in Mark Map reserved for them) */
	bool canSetAsScanned = _gcExtensions->minArraySizeToSetAsScanned <= _gcExtensions->indexableObjectModel.arrayletSize(objectPtr, 0);

	if (canSetAsScanned && isScanned((omrobjectptr_t)objectPtr)) {
		/* Already scanned by ref array copy optimization */
		return pointerFields;
	}

#if defined(OMR_GC_HYBRID_ARRAYLETS)
	/* if NUA is enabled, separate path for contiguous arrays */
	UDATA sizeInElements = _gcExtensions->indexableObjectModel.getSizeInElements(objectPtr);
	if (0 == sizeInElements) {
		fomrobject_t *startScanPtr = (fomrobject_t *)_gcExtensions->indexableObjectModel.getDataPointerForContiguous(objectPtr);
		fomrobject_t *endScanPtr = startScanPtr + sizeInElements;
		pointerFields += scanPointerRange(env, startScanPtr, endScanPtr);
	} else {
#endif /* OMR_GC_HYBRID_ARRAYLETS */
		fomrobject_t *arrayoid = _gcExtensions->indexableObjectModel.getArrayoidPointer(objectPtr);
		UDATA numArraylets = _gcExtensions->indexableObjectModel.numArraylets(objectPtr);
		for (UDATA i=0; i<numArraylets; i++) {
			UDATA arrayletSize = _gcExtensions->indexableObjectModel.arrayletSize(objectPtr, i);
			/* need to check leaf pointer because this can be a partially allocated arraylet (not all leafs are allocated) */
			GC_SlotObject slotObject(_omrVM, &arrayoid[i]);
			fomrobject_t *startScanPtr = (fomrobject_t *)(slotObject.readReferenceFromSlot());
			if (NULL != startScanPtr) {
				fomrobject_t *endScanPtr = startScanPtr + arrayletSize / sizeof(fomrobject_t);
				if (i == (numArraylets - 1)) {
					pointerFields += scanPointerRange(env, startScanPtr, endScanPtr);
					if (canSetAsScanned) {
						setScanAtomic((omrobjectptr_t)objectPtr);
					}
				} else {
					_realtimeGC->enqueuePointerArraylet(env, startScanPtr);
				}
			}
		}
#if defined(OMR_GC_HYBRID_ARRAYLETS)
	}
#endif /* OMR_GC_HYBRID_ARRAYLETS */

	/* check for yield if we've actually scanned a leaf */
	if (0 != pointerFields) {
		_scheduler->condYieldFromGC(env);
	}

	env->incScannedObjects();

	return pointerFields;
}

/**
 * If maxCount == MAX_UNIT, we run till work stack is empty and we return true, if at least one
 * object is marked.
 * Otherwise, mark up to maxCount of objects. If we reached the limit return false, which means we are
 * not finished yet.
 */
bool
MM_RealtimeMarkingScheme::incrementalConsumeQueue(MM_EnvironmentRealtime *env, UDATA maxCount)
{
	UDATA item;
	UDATA count = 0, countSinceLastYieldCheck = 0;
	UDATA scannedPointersSumSinceLastYieldCheck = 0;
  
	while(0 != (item = (UDATA)env->getWorkStack()->pop(env))) {
		UDATA scannedPointers;
		if (IS_ITEM_ARRAYLET(item)) {
			fomrobject_t *arraylet = ITEM_TO_ARRAYLET(item);
			scannedPointers = scanPointerArraylet(env, arraylet);
		} else {
			omrobjectptr_t objectPtr = ITEM_TO_OBJECT(item);
			scannedPointers = scanObject(env, objectPtr);
		}

		countSinceLastYieldCheck += 1;
		scannedPointersSumSinceLastYieldCheck += scannedPointers;
		
		if (((countSinceLastYieldCheck * 2) + scannedPointersSumSinceLastYieldCheck) > _gcExtensions->traceCostToCheckYield) {
			_scheduler->condYieldFromGC(env);
			
			scannedPointersSumSinceLastYieldCheck = 0;
			countSinceLastYieldCheck = 0;
		}
		
		if (++count >= maxCount) {
			return false;
		}
	}
	
	if (maxCount == MAX_UINT) {
		return (count != 0);
	} else {
		return true;
	}
}

#endif  /* defined(OMR_GC_REALTIME) */

