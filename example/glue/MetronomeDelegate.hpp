/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
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

#if !defined(METRONOMEDELEGATE_HPP_)
#define METRONOMEDELEGATE_HPP_

#include "omr.h"
#include "omrcfg.h"

#if defined(OMR_GC_REALTIME)

#include "BaseNonVirtual.hpp"
#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"

class MM_EnvironmentRealtime;

class MM_MetronomeDelegate : public MM_BaseNonVirtual
{
private:
	MM_GCExtensionsBase *_extensions;
	MM_RealtimeGC *_realtimeGC;

public:
	void yieldWhenRequested(MM_EnvironmentBase *env);
	static int J9THREAD_PROC metronomeAlarmThreadWrapper(void* userData);
	static uintptr_t signalProtectedFunction(OMRPortLibrary *privatePortLibrary, void* userData);	

	MM_MetronomeDelegate(MM_EnvironmentBase *env) :
		_extensions(env->getExtensions()),
		_realtimeGC(NULL) {}

	bool initialize(MM_EnvironmentBase *env);
	void tearDown(MM_EnvironmentBase *env);


	void masterSetupForGC(MM_EnvironmentBase *env);
	void masterCleanupAfterGC(MM_EnvironmentBase *env);
	void incrementalCollectStart(MM_EnvironmentRealtime *env);
	void incrementalCollect(MM_EnvironmentRealtime *env);
	void doAuxilaryGCWork(MM_EnvironmentBase *env);
	void clearGCStats(MM_EnvironmentBase *env);
	void clearGCStatsEnvironment(MM_EnvironmentRealtime *env);
	void mergeGCStats(MM_EnvironmentRealtime *env);
	uintptr_t getSplitArraysProcessed(MM_EnvironmentRealtime *env);
	void reportSyncGCEnd(MM_EnvironmentBase *env);

	void defaultMemorySpaceAllocated(MM_GCExtensionsBase *extensions, void* defaultMemorySpace);
	void enableDoubleBarrier(MM_EnvironmentBase* env);
	void disableDoubleBarrierOnThread(MM_EnvironmentBase* env, OMR_VMThread* vmThread);
	void disableDoubleBarrier(MM_EnvironmentBase* env);

	/* New methods */
	bool doTracing(MM_EnvironmentRealtime* env);

	void markLiveObjectsRoots(MM_EnvironmentRealtime *env);
	void markLiveObjectsScan(MM_EnvironmentRealtime *env);
	void markLiveObjectsComplete(MM_EnvironmentRealtime *env);
	void checkReferenceBuffer(MM_EnvironmentRealtime *env);
	void setUnmarkedImpliesCleared();
	void unsetUnmarkedImpliesCleared();

	MMINLINE uintptr_t
	scanPointerArraylet(MM_EnvironmentRealtime *env, fomrobject_t *arraylet)
	{
		fomrobject_t *startScanPtr = arraylet;
		fomrobject_t *endScanPtr = startScanPtr + (env->getOmrVM()->arrayletLeafSize / sizeof(fj9object_t));
		return scanPointerRange(env, startScanPtr, endScanPtr);
	}

	MMINLINE uintptr_t
	scanPointerRange(MM_EnvironmentRealtime *env, fomrobject_t *startScanPtr, fomrobject_t *endScanPtr)
	{
		fomrobject_t *scanPtr = startScanPtr;
		uintptr_t pointerFieldBytes = (uintptr_t)(endScanPtr - scanPtr);
		uintptr_t pointerField = pointerFieldBytes / sizeof(fj9object_t);
		while(scanPtr < endScanPtr) {
			GC_SlotObject slotObject(env->getOmrVM(), scanPtr);
			_markingScheme->markObject(env, slotObject.readReferenceFromSlot());
			scanPtr++;
		}

		env->addScannedBytes(pointerFieldBytes);
		env->addScannedPointerFields(pointerField);

		return pointerField;
	}

	MMINLINE GC_ObjectScanner *
	getObjectScanner(MM_EnvironmentRealtime *env, omrobjectptr_t objectPtr, void *scannerSpace, MM_MarkingSchemeScanReason reason, uintptr_t *sizeToDo)
	{
		GC_ObjectScanner *objectScanner = NULL;
		switch(_extensions->objectModel.getScanType(objectPtr)) {
		case GC_ObjectModel::SCAN_POINTER_ARRAY_OBJECT:
		{
			/* Very small arrays cannot be set as scanned (no scanned bit in Mark Map reserved for them) */
			bool canSetAsScanned = _extensions->minArraySizeToSetAsScanned <= _extensions->indexableObjectModel.arrayletSize((omrarrayptr_t)objectPtr, 0));

			/* Already scanned by ref array copy optimization */
			if (!canSetAsScanned || !_markingScheme->isScanned(objectPtr)) {
				objectScanner = GC_MetronomeArrayObjectScanner::newInstance(env, objectPtr, scannerSpace, GC_ObjectScanner::indexableObject);
			}
			break;
		}
		default:
			objectScanner = _realtimeGC->getMarkingScheme()->_delegate.getObjectScanner(env, objectPtr, scannerSpace, SCAN_REASON_PACKET, sizeToDo);
		}

		return objectScanner;
	}

	/*
	 * Friends
	 */
	friend class MM_RealtimeGC;
};

#endif /* defined(OMR_GC_REALTIME) */

#endif /* defined(METRONOMEDELEGATE_HPP_) */

