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

#include "list.h"

void UpnpListInit(UpnpListHead *list)
{
	list->next = list;
	list->prev = list;
}

UpnpListIter UpnpListBegin(UpnpListHead *list)
{
	return list->next;
}

UpnpListIter UpnpListEnd(UpnpListHead *list)
{
	return list;
}

UpnpListIter UpnpListNext(UpnpListHead *list, UpnpListIter pos)
{
	(void)list;
	return pos->next;
}

UpnpListIter UpnpListInsert(UpnpListHead *list, UpnpListIter pos,
							UpnpListHead *elt)
{
	(void)list;
	elt->prev = pos->prev;
	elt->next = pos;
	pos->prev->next = elt;
	pos->prev = elt;
	return elt;
}

UpnpListIter UpnpListErase(UpnpListHead *list, UpnpListIter pos)
{
	(void)list;
	pos->prev->next = pos->next;
	pos->next->prev = pos->prev;
	return pos->next;
}
