#ifndef _UPNP_LIST_H_
#define _UPNP_LIST_H_

/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (C) 2011-2012 France Telecom All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * * Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/** Trivial list management interface, patterned on std::list. It aims more at
 * being familiar than at being minimal. The implementation does not perform
 * any allocation or deallocation. 
 */

#include "UpnpGlobal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** List anchor structure. This should be the *first* entry in list
 *  member objects, except if you want to do member offset arithmetic
 *  instead of simple casts (look up "containerof"). The list code itself 
 *  does not care. */
typedef struct UpnpListHead {
	struct UpnpListHead *next, *prev;
} UpnpListHead;

/** List iterator. Not strictly necessary, but clarifies the interface. */
typedef UpnpListHead *UpnpListIter;

/** Initialize empty list */
EXPORT_SPEC void UpnpListInit(UpnpListHead *list);

/** Return iterator pointing to the first list element, or
 *  UpnpListEnd(list) if the list is empty */
EXPORT_SPEC UpnpListIter UpnpListBegin(UpnpListHead *list);

/** Return end of list sentinel iterator (not an element) */
EXPORT_SPEC UpnpListIter UpnpListEnd(UpnpListHead *list);

/** Return iterator pointing to element after pos, or end() */
EXPORT_SPEC UpnpListIter UpnpListNext(UpnpListHead *list, UpnpListIter pos);

/** Insert element before pos, returns iterator pointing to inserted element. */
EXPORT_SPEC UpnpListIter UpnpListInsert(UpnpListHead *list, UpnpListIter pos,
                                        UpnpListHead *elt);

/** Erase element at pos, return next one, or end()*/
EXPORT_SPEC UpnpListIter UpnpListErase(UpnpListHead *list, UpnpListIter pos);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UPNPLISTH_ */
