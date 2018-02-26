
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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h> // or <climits> for CHAR_BIT
#include <string.h> // memcpy

#include "RootScanner.hpp"
#include "Debug.hpp"
#include "Dispatcher.hpp"
#include "EnvironmentBase.hpp"
#include "GlobalCollector.hpp"
#include "Heap.hpp"
#include "HeapRegionDescriptor.hpp"
#include "HeapRegionIterator.hpp"
#include "HeapRegionManager.hpp"
#include "MemoryPool.hpp"
#include "MemorySubSpace.hpp"
#include "MemorySpace.hpp"
#include "ModronTypes.hpp"
#include "ObjectHeapIteratorAddressOrderedList.hpp"
#include "ObjectModel.hpp"
#include "omrExampleVM.hpp"
#include "SlotObject.hpp"
#include "Task.hpp"
#include "VMThreadIterator.hpp"

#if defined(OMR_GC_MODRON_SCAVENGER)
/**
 * @todo Provide function documentation
 */
void
MM_RootScanner::doRememberedSetSlot(omrobjectptr_t *slotPtr, GC_RememberedSetSlotIterator *rememberedSetSlotIterator)
{
	doSlot(slotPtr);
}
#endif /* defined(OMR_GC_MODRON_SCAVENGER) */

/**
 * General object field slot handler to be reimplemented by specializing class. This handler is called
 * for every reference through an instance field.
 * Implementation for slotObject input format
 * @param slotObject Input field for scan in slotObject format
 */
void
MM_RootScanner::doFieldSlot(GC_SlotObject *slotObject)
{
	omrobjectptr_t object = slotObject->readReferenceFromSlot();
	doSlot(&object);
	slotObject->writeReferenceToSlot(object);
}

/**
 * @todo Provide function documentation
 */
void
MM_RootScanner::doStackSlot(omrobjectptr_t *slotPtr, void *walkState, const void* stackLocation)
{
	/* ensure that this isn't a slot pointing into the gap (only matters for split heap VMs) */
	if (!_extensions->heap->objectIsInGap(*slotPtr)) {
		doSlot(slotPtr);
	}
}

/**
 * @todo Provide function documentation
 */
void
MM_RootScanner::doVMThreadSlot(omrobjectptr_t *slotPtr, GC_VMThreadIterator *vmThreadIterator)
{
	doSlot(slotPtr);
}

/**
 * @todo Provide function documentation
 */
void
stackSlotIterator(OMR_VM *vm, omrobjectptr_t *slot, void *localData, void *walkState, const void *stackLocation)
{
	StackIteratorData *data = (StackIteratorData *)localData;
	data->rootScanner->doStackSlot(slot, walkState, stackLocation);
}

/**
 * @todo Provide function documentation
 *
 * This function iterates through all the threads, calling scanOneThread on each one that
 * should be scanned.  The scanOneThread function scans exactly one thread and returns
 * either true (if it took an action that requires the thread list iterator to return to
 * the beginning) or false (if the thread list iterator should just continue with the next
 * thread).
 */
void
MM_RootScanner::scanThreads(MM_EnvironmentBase *env)
{
	reportScanningStarted(RootScannerEntity_Threads);

	/* TODO This assumes that while exclusive vm access is held the thread
	 * list is also locked.
	 */

	GC_OMRVMThreadListIterator vmThreadListIterator(_omrVM);
	StackIteratorData localData;

	localData.rootScanner = this;
	localData.env = env;

	while(OMR_VMThread *walkThread = vmThreadListIterator.nextOMRVMThread()) {
		if (_singleThread || J9MODRON_HANDLE_NEXT_WORK_UNIT(env)) {
			if (scanOneThread(env, walkThread, (void*) &localData)) {
				vmThreadListIterator.reset(_omrVM->_vmThreadList);
			}
		}
	}

	reportScanningEnded(RootScannerEntity_Threads);
}

/**
 * This function scans exactly one thread for potential roots.  It is designed as
 *    an overrideable subroutine of the primary functions scanThreads and scanSingleThread.
 * @param walkThead the thread to be scanned
 * @param localData opaque data to be passed to the stack walker callback function.
 *   The root scanner fixes that callback function to the stackSlotIterator function
 *   defined above
 * @return true if the thread scan included an action that requires the thread list
 *   iterator to begin over again at the beginning, false otherwise.  This implementation
 *   always returns false but an overriding implementation may take actions that require
 *   a true return (for example, RealtimeRootScanner returns true if it yielded after the
 *   scan, allowing other threads to run and perhaps be created or terminated).
 **/
bool
MM_RootScanner::scanOneThread(MM_EnvironmentBase *env, OMR_VMThread* walkThread, void* localData)
{
	GC_VMThreadIterator vmThreadIterator(walkThread);

	while(omrobjectptr_t slot = vmThreadIterator.nextSlot()) {
		doVMThreadSlot(&slot, &vmThreadIterator);
	}

	return false;
}

/**
 * This function scans exactly one thread for potential roots.
 * @param walkThead the thread to be scanned
 **/
void
MM_RootScanner::scanSingleThread(MM_EnvironmentBase *env, OMR_VMThread* walkThread)
{
	StackIteratorData localData;
	localData.rootScanner = this;
	localData.env = env;
	scanOneThread(env, walkThread, &localData);
}

#if defined(OMR_GC_MODRON_SCAVENGER)
/**
 * @todo Provide function documentation
 */
void
MM_RootScanner::scanRememberedSet(MM_EnvironmentBase *env)
{
	if(_singleThread || J9MODRON_HANDLE_NEXT_WORK_UNIT(env)) {
		reportScanningStarted(RootScannerEntity_RememberedSet);

		MM_SublistPuddle *puddle;
		omrobjectptr_t *slotPtr;
		GC_RememberedSetIterator rememberedSetIterator(&_extensions->rememberedSet);

		while((puddle = rememberedSetIterator.nextList()) != NULL) {
			GC_RememberedSetSlotIterator rememberedSetSlotIterator(puddle);
			while((slotPtr = (omrobjectptr_t *)rememberedSetSlotIterator.nextSlot()) != NULL) {
				doRememberedSetSlot(slotPtr, &rememberedSetSlotIterator);
			}
		}

		reportScanningEnded(RootScannerEntity_RememberedSet);
	}
}
#endif /* OMR_GC_MODRON_SCAVENGER */

/**
 * Scan all root set references from the VM into the heap.
 * For all slots that are hard root references into the heap, the appropriate slot handler will be called.
 * @note This includes all references to classes.
 */
void
MM_RootScanner::scanRoots(MM_EnvironmentBase *env)
{
	scanThreads(env);

	OMR_VM_Example *omrVM = (OMR_VM_Example *)env->getOmrVM()->_language_vm;
	env->_currentTask->synchronizeGCThreads(env, UNIQUE_ID);
	J9HashTableState state;
	if (NULL != omrVM->rootTable) {
		RootEntry *rootEntry = (RootEntry *)hashTableStartDo(omrVM->rootTable, &state);
		while (NULL != rootEntry) {
			if (NULL != rootEntry->rootPtr) {
				doSlot(&rootEntry->rootPtr);
			}
			rootEntry = (RootEntry *)hashTableNextDo(&state);
		}
	}
	env->_currentTask->releaseSynchronizedGCThreads(env);
}

/**
 * Scan all clearable root set references from the VM into the heap.
 * For all slots that are clearable root references into the heap, the appropriate slot handler will be
 * called.
 * @note This includes all references to classes.
 */
void
MM_RootScanner::scanClearable(MM_EnvironmentBase *env)
{
#if defined(OMR_GC_MODRON_SCAVENGER)
	/* Remembered set is clearable in a generational system -- if an object in old
	 * space dies, and it pointed to an object in new space, it needs to be removed
	 * from the remembered set.
	 * This must after any other marking might occur, e.g. phantom references.
	 */
	if(_includeRememberedSetReferences && !_nurseryReferencesOnly && !_nurseryReferencesPossibly) {
		scanRememberedSet(env);
	}
#endif /* OMR_GC_MODRON_SCAVENGER */
}

/**
 * Scan all slots which contain references into the heap.
 * @note this includes class references.
 */
void
MM_RootScanner::scanAllSlots(MM_EnvironmentBase *env)
{
	scanThreads(env);

#if defined(OMR_GC_MODRON_SCAVENGER)
	if(_includeRememberedSetReferences && !_nurseryReferencesOnly && !_nurseryReferencesPossibly) {
		scanRememberedSet(env);
	}
#endif /* OMR_GC_MODRON_SCAVENGER */
}

bool
MM_RootScanner::shouldYield()
{
	return false;
}

void
MM_RootScanner::yield()
{
}

bool
MM_RootScanner::condYield(U_64 timeSlackNanoSec)
{
	bool yielded = shouldYield();

	if (yielded) {
		yield();
	}

	return yielded;
}
