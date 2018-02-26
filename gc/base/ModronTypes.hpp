
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

#if !defined(MODRONTYPES_HPP_)
#define MODRONTYPES_HPP_

#include "modronopt.h"
 
/**
 * @ingroup GC_Base
 * @name Iterators
 * @{
 */

#include "HashTableIterator.hpp"

#include "ObjectHeapIterator.hpp"
#include "PoolIterator.hpp"
#include "SublistIterator.hpp"
#include "SublistSlotIterator.hpp"
#include "VMThreadIterator.hpp"

#if defined(OMR_GC_MODRON_SCAVENGER)
typedef GC_SublistIterator GC_RememberedSetIterator;
typedef GC_SublistSlotIterator GC_RememberedSetSlotIterator;
#endif /* OMR_GC_MODRON_SCAVENGER */

/**
 * @} Iterators
 */
#endif /* MODRONTYPES_HPP_ */
